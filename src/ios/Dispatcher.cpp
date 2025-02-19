/*******************************************************************************
 * Project:  Nebula
 * @file     Dispatcher.cpp
 * @brief    事件管理、事件分发
 * @author   Bwar
 * @date:    2019年9月7日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Dispatcher.hpp"
#include <algorithm>
#include "Definition.hpp"
#include "labor/Manager.hpp"
#include "labor/Worker.hpp"
#include "IO.hpp"
#include "ChannelWatcher.hpp"
#include "actor/Actor.hpp"
#include "actor/step/Step.hpp"
#include "actor/step/RedisStep.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"
#include "codec/CodecFactory.hpp"
#include "channel/SocketChannelImpl.hpp"
#include "channel/migrate/SocketChannelMigrate.hpp"
#include "pb/neb_sys.pb.h"

namespace neb
{

Dispatcher::Dispatcher(Labor* pLabor, std::shared_ptr<NetLogger> pLogger)
   : m_pErrBuff(NULL), m_pLabor(pLabor), m_loop(NULL), m_lLastCheckNodeTime(0),
     m_pLogger(pLogger), m_pSessionNode(nullptr)
{
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);

    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD);
}

Dispatcher::~Dispatcher()
{
    Destroy();
}

void Dispatcher::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        auto pWatcher = static_cast<ChannelWatcher*>(watcher->data);
        auto pChannel = pWatcher->GetSocketChannel();
        Dispatcher* pDispatcher = pChannel->m_pImpl->GetLabor()->GetDispatcher();
        if (revents & EV_READ)
        {
            pDispatcher->OnIoRead(pChannel);
        }
        if ((revents & EV_WRITE) && (CHANNEL_STATUS_CLOSED != pChannel->GetChannelStatus())) // the channel maybe closed by OnIoRead()
        {
            pDispatcher->OnIoWrite(pChannel);
        }
        if (revents & EV_ERROR)
        {
            pDispatcher->OnIoError(pChannel);
        }
    }
}

void Dispatcher::IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        auto pWatcher = static_cast<ChannelWatcher*>(watcher->data);
        auto pChannel = pWatcher->GetSocketChannel();
        Dispatcher* pDispatcher = pChannel->m_pImpl->GetLabor()->GetDispatcher();
        if (pChannel->GetFd() < 3)      // TODO 查找fd为0回调到这里的原因
        {
            return;
        }
        pDispatcher->OnIoTimeout(pChannel);
    }
}

void Dispatcher::PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Dispatcher* pDispatcher = (Dispatcher*)(watcher->data);
        if (Labor::LABOR_MANAGER == pDispatcher->m_pLabor->GetLaborType())
        {
            ((Manager*)(pDispatcher->m_pLabor))->GetSessionManager()->CheckWorker();
            ((Manager*)(pDispatcher->m_pLabor))->RefreshServer();
        }
        pDispatcher->CheckFailedNode();
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void Dispatcher::SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Labor* pLabor = (Labor*)watcher->data;
        pLabor->OnTerminated(watcher);
    }
}

void Dispatcher::AsyncCallback(struct ev_loop* loop, struct ev_async* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        auto pWatcher = static_cast<SpecChannelWatcher*>(watcher->data);
        auto pChannel = pWatcher->GetSocketChannel();
        CodecFactory::OnEvent(pWatcher, pChannel);
    }
}

void Dispatcher::ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagClientConnWatcherData* pData = (tagClientConnWatcherData*)watcher->data;
        pData->pDispatcher->OnClientConnFrequencyTimeout(pData, watcher);
    }
}

bool Dispatcher::OnIoRead(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d]", pChannel->GetFd());
    m_pLastActivityChannel = pChannel;
    if (Labor::LABOR_MANAGER == m_pLabor->GetLaborType())
    {
        auto& setAccessFd = ((Manager*)m_pLabor)->GetManagerInfo().setAccessFd;
        if (pChannel->GetFd() == ((Manager*)m_pLabor)->GetManagerInfo().iS2SListenFd)
        {
            return(AcceptServerConn(pChannel->GetFd()));
        }
        else if (((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd > 2
                && pChannel->GetFd() == ((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd)
        {
            return(AcceptFdAndTransfer(((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd,
                    ((Manager*)m_pLabor)->GetManagerInfo().iC2SFamily, 1));
        }
        else if (setAccessFd.find(pChannel->GetFd()) != setAccessFd.end())
        {
            return(AcceptFdAndTransfer(pChannel->GetFd(),
                    ((Manager*)m_pLabor)->GetManagerInfo().iC2SFamily, 0));
        }
        else
        {
            return(DataRecvAndHandle(pChannel));
        }
    }
    else
    {
        return(DataRecvAndHandle(pChannel));
    }
}

bool Dispatcher::DataRecvAndHandle(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("codec type %d", pChannel->GetCodecType());
    E_CODEC_STATUS eCodecStatus = CodecFactory::OnEvent(this, pChannel);

    switch (eCodecStatus)
    {
        case CODEC_STATUS_PAUSE:
        case CODEC_STATUS_PART_OK:
        case CODEC_STATUS_PART_ERR:
            return(true);
        case CODEC_STATUS_WANT_WRITE:
            return(true);
        case CODEC_STATUS_WANT_READ:
            RemoveIoWriteEvent(pChannel);
            return(true);
        default: // 编解码出错或连接关闭或连接中断
            if (CHANNEL_STATUS_ESTABLISHED != pChannel->GetChannelStatus())
            {
                if (pChannel->IsClient())
                {
                    LOG4_INFO("NodeFailed(%s)", pChannel->GetIdentify().c_str());
                    m_pSessionNode->NodeFailed(pChannel->GetIdentify());
                }
                if (pChannel->m_pImpl != nullptr)
                {
                    auto& listUncompletedStep = std::static_pointer_cast<SocketChannelImpl<CodecNebula>>(pChannel->m_pImpl)->GetPipelineStepSeq();
                    for (auto it = listUncompletedStep.begin();
                           it != listUncompletedStep.end(); ++it)
                    {
                        m_pLabor->GetActorBuilder()->OnError(pChannel, *it,
                                pChannel->GetErrno(), pChannel->GetErrMsg());
                    }
                    auto& mapUncompletedStep = std::static_pointer_cast<SocketChannelImpl<CodecNebula>>(pChannel->m_pImpl)->GetStreamStepSeq();
                    for (auto it = mapUncompletedStep.begin();
                        it != mapUncompletedStep.end(); ++it)
                    {
                        m_pLabor->GetActorBuilder()->OnError(pChannel, it->second,
                                pChannel->GetErrno(), pChannel->GetErrMsg());
                    }
                }
            }
            LOG4_INFO("eCodecStatus = %d", eCodecStatus);
            DiscardSocketChannel(pChannel);
            return(false);
    }
}

bool Dispatcher::MigrateChannelRecvAndHandle(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("codec type %d", pChannel->GetCodecType());
    E_CODEC_STATUS eCodecStatus = CodecFactory::OnEvent(this, pChannel, CODEC_STATUS_INVALID);

    switch (eCodecStatus)
    {
        case CODEC_STATUS_PAUSE:
        case CODEC_STATUS_PART_OK:
        case CODEC_STATUS_PART_ERR:
            return(true);
        case CODEC_STATUS_WANT_WRITE:
            return(true);
        case CODEC_STATUS_WANT_READ:
            RemoveIoWriteEvent(pChannel);
            return(true);
        default:
            LOG4_INFO("eCodecStatus = %d", eCodecStatus);
            DiscardSocketChannel(pChannel);
            return(false);
    }
}

bool Dispatcher::OnIoWrite(std::shared_ptr<SocketChannel> pChannel)
{
    if (CODEC_NEBULA == pChannel->GetCodecType())  // 系统内部Server间通信
    {
        if (pChannel->GetRemoteWorkerIndex() < 0) // connect to Manager
        {
            std::shared_ptr<Step> pStepTellWorker
                = m_pLabor->GetActorBuilder()->MakeSharedStep(nullptr, "neb::StepTellWorker", pChannel);
            if (nullptr == pStepTellWorker)
            {
                return(false);
            }
            pStepTellWorker->Emit(ERR_OK);
            pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
        }
        else
        {
            if (CHANNEL_STATUS_TRY_CONNECT == pChannel->GetChannelStatus())  // connect之后的第一个写事件
            {
                std::shared_ptr<Step> pStepConnectWorker = m_pLabor->GetActorBuilder()->MakeSharedStep(
                        nullptr, "neb::StepConnectWorker", pChannel,
                        pChannel->GetRemoteWorkerIndex());
                if (nullptr == pStepConnectWorker)
                {
                    LOG4_ERROR("error %d: new StepConnectWorker() error!", ERR_NEW);
                    return(false);
                }
                if (CMD_STATUS_RUNNING != pStepConnectWorker->Emit(ERR_OK))
                {
                    m_pLabor->GetActorBuilder()->RemoveStep(pStepConnectWorker);
                }
                return(true);
            }
        }
    }
    else
    {
        if (CHANNEL_STATUS_CLOSED != pChannel->GetChannelStatus())
        {
            pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
        }
    }

    if (pChannel->IsClient() && pChannel->GetMsgNum() == 0)
    {
        m_pSessionNode->NodeRecover(pChannel->GetIdentify());
    }
    LOG4_TRACE("");
    auto eCodecStatus = pChannel->Send();
    if (CODEC_STATUS_OK == eCodecStatus)
    {
        RemoveIoWriteEvent(pChannel);
    }
    else if (CODEC_STATUS_PAUSE == eCodecStatus || CODEC_STATUS_WANT_WRITE == eCodecStatus)
    {
        AddIoWriteEvent(pChannel);
    }
    else if (CODEC_STATUS_WANT_READ == eCodecStatus)
    {
        RemoveIoWriteEvent(pChannel);
    }
    else
    {
        LOG4_INFO("%s channel[%d]", pChannel->GetIdentify().c_str(), pChannel->GetFd());
        DiscardSocketChannel(pChannel);
    }
    return(true);
}

bool Dispatcher::OnIoError(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE(" ");
    return(false);
}

bool Dispatcher::OnIoTimeout(std::shared_ptr<SocketChannel> pChannel)
{
    ev_tstamp after = pChannel->GetLastRecvTime() - ev_now(m_loop) + pChannel->GetKeepAlive();
    if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, pChannel->MutableWatcher()->MutableTimerWatcher());
        ev_timer_set (pChannel->MutableWatcher()->MutableTimerWatcher(), after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, pChannel->MutableWatcher()->MutableTimerWatcher());
        return(true);
    }

    LOG4_TRACE("fd %d, seq %u, last recv time %f, now time %f, keep alive %f",
            pChannel->GetFd(), pChannel->GetSequence(), pChannel->GetLastRecvTime(),
            ev_now(m_loop), pChannel->GetKeepAlive());
    if (pChannel->IsClient() && pChannel->NeedAliveCheck())     // 需要发送心跳检查
    {
        if (!PingChannel(pChannel))
        {
            LOG4_TRACE("channel timeout!");
            DiscardSocketChannel(pChannel);
        }
    }
    else        // 关闭文件描述符并清理相关资源
    {
        LOG4_TRACE("io timeout!");
        DiscardSocketChannel(pChannel);
    }
    return(true);
}

bool Dispatcher::OnClientConnFrequencyTimeout(tagClientConnWatcherData* pData, ev_timer* watcher)
{
    bool bRes = false;
    auto iter = m_mapClientConnFrequency.find(std::string(pData->pAddr));
    if (iter == m_mapClientConnFrequency.end())
    {
        bRes = false;
    }
    else
    {
        m_mapClientConnFrequency.erase(iter);
        bRes = true;
    }

    DelEvent(watcher);
    pData->pDispatcher = nullptr;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

void Dispatcher::EventRun()
{
    ev_run (m_loop, 0);
}

bool Dispatcher::AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout)
{
    LOG4_TRACE("channel_fd %d, channel_seq %u, timeout %f", pChannel->GetFd(), pChannel->GetSequence(), dTimeout);
    if (dTimeout < 0)
    {
        LOG4_INFO("no io timeout");
        return(true);
    }
    auto pWatcher = pChannel->MutableWatcher();
    pWatcher->Set(pChannel);
    ev_timer* timer_watcher = pWatcher->MutableTimerWatcher();
    if (NULL == timer_watcher)
    {
        return(false);
    }
    else
    {
        if (ev_is_active(timer_watcher))
        {
            ev_timer_stop(m_loop, timer_watcher);
            ev_timer_set(timer_watcher, dTimeout + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, timer_watcher);
        }
        else
        {
            ev_timer_init (timer_watcher, IoTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
            ev_timer_start (m_loop, timer_watcher);
        }
        return(true);
    }
}

bool Dispatcher::SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    if (m_pLabor->GetLaborType() == Labor::LABOR_MANAGER)
    {
        ChannelOption stOption;
        stOption.bPipeline = true;
        return(IO<CodecNebula>::Broadcast(this, 0, "BEACON", stOption, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        MsgHead oMsgHead;
        oMsgHead.set_cmd(iCmd);
        oMsgHead.set_seq(uiSeq);
        uint32 uiManagerLaborId = LaborShared::Instance()->GetManagerLaborId();
        int iResult = CodecNebulaInNode::Write(((Worker*)m_pLabor)->GetWorkerInfo().iWorkerIndex,
                uiManagerLaborId, gc_uiCmdReq, m_pLabor->GetSequence(), oMsgHead, oMsgBody);
        if (ERR_OK == iResult)
        {
            return(true);
        }
        else
        {
            LOG4_ERROR("send data report to manager error %d", iResult);
            return(false);
        }
    }
}

std::shared_ptr<SocketChannel> Dispatcher::StressSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    size_t iPosIpPortSeparator = strIdentify.rfind(":");
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(nullptr);
    }
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(nullptr);
    }

    struct addrinfo stAddrHints;
    struct addrinfo* pAddrResult;
    struct addrinfo* pAddrCurrent;
    memset(&stAddrHints, 0, sizeof(struct addrinfo));
    stAddrHints.ai_family = AF_UNSPEC;
    stAddrHints.ai_socktype = SOCK_STREAM;
    stAddrHints.ai_protocol = IPPROTO_IP;
    int iCode = getaddrinfo(strHost.c_str(), strPort.c_str(), &stAddrHints, &pAddrResult);
    if (0 != iCode)
    {
        LOG4_ERROR("getaddrinfo(\"%s\", \"%s\") error %d: %s",
                strHost.c_str(), strPort.c_str(), iCode, gai_strerror(iCode));
        return(nullptr);
    }
    int iFd = -1;
    for (pAddrCurrent = pAddrResult;
            pAddrCurrent != NULL; pAddrCurrent = pAddrCurrent->ai_next)
    {
        iFd = socket(pAddrCurrent->ai_family,
                pAddrCurrent->ai_socktype, pAddrCurrent->ai_protocol);
        if (iFd == -1)
        {
            continue;
        }
        break;
    }

    /* no address succeeded */
    if (pAddrCurrent == NULL)
    {
        LOG4_ERROR("Could not connect to \"%s:%s\"", strHost.c_str(), strPort.c_str());
        freeaddrinfo(pAddrResult);
        return(nullptr);
    }

    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    int iKeepAlive = 1;
    int iKeepIdle = 60;
    int iKeepInterval = 5;
    int iKeepCount = 3;
    int iTcpNoDelay = 1;
    int iTcpQuickAck = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    setsockopt(iFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive));
    setsockopt(iFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle));
    setsockopt(iFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval));
    setsockopt(iFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount));
    setsockopt(iFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay));
    setsockopt(iFd, IPPROTO_TCP, TCP_QUICKACK, (void*)&iTcpQuickAck, sizeof(iTcpQuickAck));
    std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(iFd, eCodecType, true);
    if (nullptr != pChannel)
    {
        connect(iFd, pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen);
        freeaddrinfo(pAddrResult);  /* no longer needed */
        ev_tstamp dIoTimeout = (m_pLabor->GetNodeInfo().dConnectionProtection > 0)
            ? m_pLabor->GetNodeInfo().dConnectionProtection : m_pLabor->GetNodeInfo().dIoTimeout;
        AddIoTimeout(pChannel, dIoTimeout);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        pChannel->SetRemoteAddr(strHost);
        IO<CodecNebula>::SendRequest(this, 0, pChannel, iCmd, uiSeq, oMsgBody);
        m_pLabor->IoStatAddConnection(IO_STAT_DOWNSTREAM_NEW_CONNECTION);
        return(pChannel);
    }
    else
    {
        freeaddrinfo(pAddrResult);
        close(iFd);
        return(nullptr);
    }
}

std::shared_ptr<SocketChannel> Dispatcher::GetLastActivityChannel()
{
    return(m_pLastActivityChannel);
}

bool Dispatcher::Disconnect(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice)
{
    LOG4_INFO("%s", pChannel->GetIdentify().c_str());
    return(DiscardSocketChannel(pChannel, bChannelNotice));
}

bool Dispatcher::Disconnect(const std::string& strIdentify, bool bChannelNotice)
{
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter != m_mapNamedSocketChannel.end())
    {
        std::unordered_set<std::shared_ptr<SocketChannel>>::iterator channel_iter;
        while (named_iter->second.size() > 1)
        {
            channel_iter = named_iter->second.begin();
            LOG4_INFO("%s", (*channel_iter)->GetIdentify().c_str());
            DiscardSocketChannel(*channel_iter, bChannelNotice);
        }
        channel_iter = named_iter->second.begin();
        LOG4_INFO("%s", (*channel_iter)->GetIdentify().c_str());
        return(DiscardSocketChannel(*channel_iter, bChannelNotice));
    }
    return(false);
}

bool Dispatcher::DiscardNamedChannel(const std::string& strIdentify)
{
    LOG4_TRACE("identify: %s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        for (auto channel_iter = named_iter->second.begin();
                channel_iter != named_iter->second.end(); ++channel_iter)
        {
            (*channel_iter)->SetIdentify("");
            (*channel_iter)->SetClientData("");
        }
        named_iter->second.clear();
        m_mapNamedSocketChannel.erase(named_iter);
        return(true);
    }
}

bool Dispatcher::AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        std::unordered_set<std::shared_ptr<SocketChannel>> setChannel;
        setChannel.insert(pChannel);
        m_mapNamedSocketChannel.insert(std::make_pair(strIdentify, std::move(setChannel)));
    }
    else
    {
        named_iter->second.insert(pChannel);
    }
    pChannel->SetIdentify(strIdentify);
    return(true);
}

void Dispatcher::DelNamedSocketChannel(const std::string& strIdentify)
{
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        ;
    }
    else
    {
        m_mapNamedSocketChannel.erase(named_iter);
    }
}

void Dispatcher::SetChannelIdentify(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify)
{
    pChannel->SetIdentify(strIdentify);
}

void Dispatcher::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    m_pSessionNode->AddNode(strNodeType, strIdentify);

    if (std::string("BEACON") != m_pLabor->GetNodeInfo().strNodeType
            && std::string("LOGGER") != m_pLabor->GetNodeInfo().strNodeType)
    {
        std::string strOnlineNode;
        if (std::string("LOGGER") == strNodeType && m_pSessionNode->GetNode(strNodeType, strOnlineNode))
        {
            m_pLogger->EnableNetLogger(true);
        }
    }
}

void Dispatcher::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    m_pSessionNode->DelNode(strNodeType, strIdentify);

    std::string strOnlineNode;
    if (std::string("LOGGER") == strNodeType
            && !m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        m_pLogger->EnableNetLogger(false);
    }
}

void Dispatcher::CircuitBreak(const std::string& strIdentify)
{
    m_pSessionNode->NodeFailed(strIdentify);
}

void Dispatcher::SetChannelPingStep(int iCodec, const std::string& strStepName)
{
    auto iter = m_mapChannelPingStepName.find(iCodec);
    if (iter == m_mapChannelPingStepName.end())
    {
        m_mapChannelPingStepName.insert(std::make_pair(iCodec, strStepName));
    }
}

void Dispatcher::SetClientData(std::shared_ptr<SocketChannel> pChannel, const std::string& strClientData)
{
    pChannel->SetClientData(strClientData);
}

bool Dispatcher::IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType)
{
    return(m_pSessionNode->IsNodeType(strNodeIdentify, strNodeType));
}

bool Dispatcher::GetAuth(const std::string& strIdentify, std::string& strAuth, std::string& strPassword)
{
    return(m_pSessionNode->GetAuth(strIdentify, strAuth, strPassword));
}

std::shared_ptr<ChannelOption> Dispatcher::GetChannelOption(const std::string& strIdentify)
{
    return(m_pSessionNode->GetChannelOption(strIdentify));
}

void Dispatcher::SetChannelOption(const std::string& strIdentify, const ChannelOption& stOption)
{
    m_pSessionNode->SetChannelOption(strIdentify, stOption);
}

bool Dispatcher::AddEvent(ev_signal* signal_watcher, signal_callback pFunc, int iSignum)
{
    if (NULL == signal_watcher)
    {
        return(false);
    }
    ev_signal_init (signal_watcher, pFunc, iSignum);
    ev_signal_start (m_loop, signal_watcher);
    return(true);
}

bool Dispatcher::AddEvent(ev_timer* timer_watcher, timer_callback pFunc, ev_tstamp dTimeout)
{
    if (NULL == timer_watcher)
    {
        return(false);
    }
    ev_timer_init (timer_watcher, pFunc, dTimeout + ev_time() - ev_now(m_loop), 0.);
    ev_timer_start (m_loop, timer_watcher);
    return(true);
}

bool Dispatcher::AddEvent(ev_idle* idle_watcher, idle_callback pFunc)
{
    if (NULL == idle_watcher)
    {
        return(false);
    }
    ev_idle_init (idle_watcher, pFunc);
    ev_idle_start (m_loop, idle_watcher);
    return(true);
}

bool Dispatcher::AddEvent(ev_async* async_watcher, async_callback pFunc)
{
    if (NULL == async_watcher)
    {
        return(false);
    }
    ev_async_init(async_watcher, pFunc);
    ev_async_start(m_loop, async_watcher);
    return(true);
}

bool Dispatcher::RefreshEvent(ev_timer* timer_watcher, ev_tstamp dTimeout)
{
    if (NULL == timer_watcher)
    {
        return(false);
    }
    ev_timer_stop (m_loop, timer_watcher);
    ev_timer_set (timer_watcher, dTimeout + ev_time() - ev_now(m_loop), 0);
    ev_timer_start (m_loop, timer_watcher);
    return(true);
}

bool Dispatcher::DelEvent(ev_io* io_watcher)
{
    if (NULL == io_watcher)
    {
        return(false);
    }
    ev_io_stop (m_loop, io_watcher);
    return(true);
}

bool Dispatcher::DelEvent(ev_timer* timer_watcher)
{
    if (NULL == timer_watcher)
    {
        return(false);
    }
    ev_timer_stop (m_loop, timer_watcher);
    return(true);
}

void Dispatcher::AddChannelToLoop(std::shared_ptr<SocketChannel> pChannel)
{
    auto iter = m_mapSocketChannel.find(pChannel->GetFd());
    if (iter == m_mapSocketChannel.end())
    {
        pChannel->SetBonding(m_pLabor, GetLogger(), pChannel);
        pChannel->MutableWatcher()->Set(pChannel);
        ev_tstamp dIoTimeout = (m_pLabor->GetNodeInfo().dConnectionProtection > 0)
            ? m_pLabor->GetNodeInfo().dConnectionProtection : m_pLabor->GetNodeInfo().dIoTimeout;
        AddIoTimeout(pChannel, dIoTimeout);
        AddIoReadEvent(pChannel);
        m_mapSocketChannel.insert(std::make_pair(pChannel->GetFd(), pChannel));
    }
}

bool Dispatcher::CreateListenFd(const std::string& strHost, int32 iPort, int iBacklog, int& iFd, int& iFamily)
{
    int reuse = 1;
    int timeout = 1;

    struct addrinfo stAddrHints;
    struct addrinfo* pAddrResult;
    struct addrinfo* pAddrCurrent;
    memset(&stAddrHints, 0, sizeof(struct addrinfo));
    stAddrHints.ai_family = AF_UNSPEC;              /* Allow IPv4 or IPv6 */
    stAddrHints.ai_socktype = m_pLabor->GetNodeInfo().iForClientSocketType;
    stAddrHints.ai_protocol = IPPROTO_IP;
    stAddrHints.ai_flags = AI_PASSIVE;              /* For wildcard IP address */
    int iCode = getaddrinfo(strHost.c_str(),
            std::to_string(iPort).c_str(), &stAddrHints, &pAddrResult);
    if (0 != iCode)
    {
        LOG4_ERROR("getaddrinfo(\"%s\", \"%d\") error %d: %s",
                strHost.c_str(),
                iPort, iCode, gai_strerror(iCode));
        exit(errno);
    }
    for (pAddrCurrent = pAddrResult;
            pAddrCurrent != NULL; pAddrCurrent = pAddrCurrent->ai_next)
    {
        iFd = socket(pAddrCurrent->ai_family,
                pAddrCurrent->ai_socktype, pAddrCurrent->ai_protocol);
        if (-1 == iFd)
        {
            continue;
        }
        if (-1 == ::setsockopt(iFd,
                    SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
        {
            close(iFd);
            iFd = -1;
            continue;
        }
        if (-1 == ::setsockopt(iFd,
                    IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int)))
        {
            close(iFd);
            iFd = -1;
            continue;
        }
        if (-1 == bind(iFd,
                    pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen))
        {
            close(iFd);
            iFd = -1;
            continue;
        }
        if (-1 == listen(iFd, iBacklog))
        {
            close(iFd);
            iFd = -1;
            continue;
        }

        iFamily = pAddrCurrent->ai_family;
        break;
    }
    freeaddrinfo(pAddrResult);           /* No longer needed */

    /* No address succeeded */
    if (-1 == iFd)
    {
        LOG4_ERROR("Could not bind to \"%s:%d\", error %d: %s",
                strHost.c_str(),
                iPort,
                errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        return(false);
    }
    return(true);
}

std::shared_ptr<SocketChannel> Dispatcher::GetChannel(int iFd)
{
    auto iter = m_mapSocketChannel.find(iFd);
    if (iter != m_mapSocketChannel.end())
    {
        return(iter->second);
    }
    return(nullptr);
}

int32 Dispatcher::GetConnectionNum() const
{
    return((int32)m_mapSocketChannel.size());
}

bool Dispatcher::Init()
{
#if __cplusplus >= 201401L
    m_pSessionNode = std::make_unique<Nodes>();
#else
    m_pSessionNode = std::unique_ptr<Nodes>(new Nodes());
#endif
    SetChannelPingStep(CODEC_PROTO, "neb::StepNebulaChannelPing");
    SetChannelPingStep(CODEC_NEBULA, "neb::StepNebulaChannelPing");
    SetChannelPingStep(CODEC_RESP, "neb::StepRedisChannelPing");
    return(true);
}

void Dispatcher::AsyncSend(ev_async* pWatcher)
{
    ev_async_send(m_loop, pWatcher);
}

void Dispatcher::Destroy()
{
    m_mapSocketChannel.clear();
    m_mapNamedSocketChannel.clear();
    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }
    if (m_pErrBuff != NULL)
    {
        free(m_pErrBuff);
        m_pErrBuff = NULL;
    }
    m_pLabor = nullptr;
}

std::shared_ptr<SocketChannel> Dispatcher::CreateSocketChannel(int iFd, E_CODEC_TYPE eCodecType, bool bIsClient, bool bWithSsl)
{
    LOG4_TRACE("iFd %d, codec_type %d, with_ssl = %d", iFd, eCodecType, bWithSsl);

    auto iter = m_mapSocketChannel.find(iFd);
    if (iter == m_mapSocketChannel.end())
    {
        auto pChannel = CodecFactory::CreateChannel(m_pLabor, m_pLogger, iFd, eCodecType, bIsClient, bWithSsl);
        if (pChannel != nullptr)
        {
            m_mapSocketChannel.insert(std::make_pair(iFd, pChannel));
            LOG4_TRACE("new channel[%d] with codec type %d", pChannel->GetFd(), pChannel->GetCodecType());
        }
        return(pChannel);
    }
    else
    {
        LOG4_WARNING("fd %d is exist!", iFd);
        return(iter->second);
    }
}

bool Dispatcher::DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice)
{
    if (pChannel == nullptr)
    {
        LOG4_TRACE("pChannel not exist!");
        return(false);
    }

    auto named_iter = m_mapNamedSocketChannel.find(pChannel->GetIdentify());
    if (named_iter != m_mapNamedSocketChannel.end())
    {
       for (auto it = named_iter->second.begin();
           it != named_iter->second.end(); ++it)
       {
           if ((*it)->GetSequence() == pChannel->GetSequence())
           {
               named_iter->second.erase(it);
               LOG4_TRACE("erase channel %d from m_mapNamedSocketChannel.", pChannel->GetFd());
               break;
           }
       }
       if (0 == named_iter->second.size())
       {
           m_mapNamedSocketChannel.erase(named_iter);
       }
    }

    bool bCloseResult = false;
    if (pChannel->WithSsl())
    {
#ifdef WITH_OPENSSL
        bCloseResult = pChannel->Close();
#else
        bCloseResult = pChannel->Close();
#endif
    }
    else
    {
        bCloseResult = pChannel->Close();
    }
    if (bCloseResult)
    {
        LOG4_INFO("%s disconnect, fd %d, channel_seq %u, identify %s",
                pChannel->GetRemoteAddr().c_str(),
                pChannel->GetFd(), pChannel->GetSequence(),
                pChannel->GetIdentify().c_str());
        if (pChannel->IsClient())
        {
            m_pLabor->IoStatAddConnection(IO_STAT_UPSTREAM_DESTROY_CONNECTION);
        }
        else
        {
            m_pLabor->IoStatAddConnection(IO_STAT_DOWNSTREAM_DESTROY_CONNECTION);
        }
        if (bChannelNotice)
        {
            m_pLabor->GetActorBuilder()->ChannelNotice(pChannel, pChannel->GetIdentify(), pChannel->GetClientData());
        }
        ev_io_stop (m_loop, pChannel->MutableWatcher()->MutableIoWatcher());
        if (nullptr != pChannel->MutableWatcher()->MutableTimerWatcher())
        {
            ev_timer_stop (m_loop, pChannel->MutableWatcher()->MutableTimerWatcher());
        }
        pChannel->MutableWatcher()->Reset();

        auto channel_iter = m_mapSocketChannel.find(pChannel->GetFd());
        if (channel_iter != m_mapSocketChannel.end())
        {
            m_mapSocketChannel.erase(channel_iter);
            LOG4_TRACE("erase channel %d channel_seq %u from m_mapSocketChannel.",
                    pChannel->GetFd(), pChannel->GetSequence());
        }
        return(true);
    }
    else
    {
        return(bCloseResult);
    }
}

bool Dispatcher::MigrateSocketChannel(uint32 uiFromLabor, uint32 uiToLabor, std::shared_ptr<SocketChannel> pChannel)
{
    if (pChannel == nullptr)
    {
        LOG4_TRACE("pChannel not exist!");
        return(false);
    }
    if (pChannel->IsClient())
    {
        LOG4_ERROR("client channel can not be migrate.");
        return(false);
    }

    auto named_iter = m_mapNamedSocketChannel.find(pChannel->m_pImpl->GetIdentify());
    if (named_iter != m_mapNamedSocketChannel.end())
    {
       for (auto it = named_iter->second.begin();
           it != named_iter->second.end(); ++it)
       {
           if ((*it)->m_pImpl->GetSequence() == pChannel->m_pImpl->GetSequence())
           {
               named_iter->second.erase(it);
               LOG4_TRACE("erase channel %d from m_mapNamedSocketChannel.", pChannel->GetFd());
               break;
           }
       }
       if (0 == named_iter->second.size())
       {
           m_mapNamedSocketChannel.erase(named_iter);
       }
    }

    ev_io_stop (m_loop, pChannel->MutableWatcher()->MutableIoWatcher());
    if (nullptr != pChannel->MutableWatcher()->MutableTimerWatcher())
    {
        ev_timer_stop (m_loop, pChannel->MutableWatcher()->MutableTimerWatcher());
    }

    auto channel_iter = m_mapSocketChannel.find(pChannel->GetFd());
    if (channel_iter != m_mapSocketChannel.end())
    {
        m_mapSocketChannel.erase(channel_iter);
        LOG4_TRACE("erase channel %d channel_seq %u from m_mapSocketChannel.",
                pChannel->GetFd(), pChannel->GetSequence());
    }
    LOG4_INFO("migrate channel[%d] with codec_type %d from labor %u to labor %u",
            pChannel->GetFd(), pChannel->GetCodecType(), uiFromLabor, uiToLabor);
    int iResult = SocketChannelMigrate::Write(uiFromLabor, uiToLabor, gc_uiCmdReq, m_pLabor->GetSequence(), pChannel);
    if (ERR_OK == iResult)
    {
        return(true);
    }
    LOG4_WARNING("failed to migrate channel");
    // recover from migration failed
    pChannel->SetBonding(m_pLabor, GetLogger(), pChannel);
    pChannel->SetMigrated(false);
    pChannel->MutableWatcher()->Set(pChannel);
    m_mapSocketChannel.insert(std::make_pair(pChannel->GetFd(), pChannel));
    ev_tstamp dIoTimeout = (m_pLabor->GetNodeInfo().dConnectionProtection > 0)
            ? m_pLabor->GetNodeInfo().dConnectionProtection : m_pLabor->GetNodeInfo().dIoTimeout;
    AddIoTimeout(pChannel, dIoTimeout);
    AddIoReadEvent(pChannel);
    return(false);
}

bool Dispatcher::AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d], seq[%u]", pChannel->GetFd(), pChannel->GetSequence());
    auto pWatcher = pChannel->MutableWatcher();
    pWatcher->Set(pChannel);
    ev_io* io_watcher = pWatcher->MutableIoWatcher();
    if (NULL == io_watcher || pChannel->GetFd() < 0)
    {
        return(false);
    }
    else
    {
        if (ev_is_active(io_watcher))
        {
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
            ev_io_start (m_loop, io_watcher);
        }
        else
        {
            ev_io_init (io_watcher, IoCallback, pChannel->GetFd(), EV_READ);
            ev_io_start (m_loop, io_watcher);
        }
        return(true);
    }
}

bool Dispatcher::AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%d, %u", pChannel->GetFd(), pChannel->GetSequence());
    auto pWatcher = pChannel->MutableWatcher();
    pWatcher->Set(pChannel);
    ev_io* io_watcher = pWatcher->MutableIoWatcher();
    if (NULL == io_watcher || pChannel->GetFd() < 0)
    {
        return(false);
    }
    else
    {
        if (ev_is_active(io_watcher))
        {
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
            ev_io_start (m_loop, io_watcher);
        }
        else
        {
            ev_io_init (io_watcher, IoCallback, pChannel->GetFd(), EV_WRITE);
            ev_io_start (m_loop, io_watcher);
        }
        return(true);
    }
}
bool Dispatcher::RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%d, %u", pChannel->GetFd(), pChannel->GetSequence());
    auto pWatcher = pChannel->MutableWatcher();
    pWatcher->Set(pChannel);
    ev_io* io_watcher = pWatcher->MutableIoWatcher();
    if (NULL == io_watcher || pChannel->GetFd() < 0)
    {
        return(false);
    }
    if (EV_WRITE & io_watcher->events)
    {
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & (~EV_WRITE));
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

void Dispatcher::SetChannelStatus(std::shared_ptr<SocketChannel> pChannel, E_CHANNEL_STATUS eStatus)
{
    pChannel->SetChannelStatus(eStatus);
}

bool Dispatcher::AddClientConnFrequencyTimeout(const char* pAddr, ev_tstamp dTimeout)
{
    LOG4_TRACE(" ");
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    tagClientConnWatcherData* pData = new tagClientConnWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagClientConnWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, ClientConnFrequencyTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
    pData->pDispatcher = this;
    snprintf(pData->pAddr, gc_iAddrLen, "%s", pAddr);
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool Dispatcher::AcceptFdAndTransfer(int iFd, int iFamily, int iBonding)
{
    char szClientAddr[64] = {0};
    int iAcceptFd = -1;
    int iClientPort = 0;
    if (AF_INET == iFamily)
    {
        struct sockaddr_in stClientAddr;
        socklen_t clientAddrSize = sizeof(stClientAddr);
        iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
        if (iAcceptFd < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
            return(false);
        }
        int iKeepAlive = 1;
        int iKeepIdle = 60;
        int iKeepInterval = 5;
        int iKeepCount = 3;
        int iTcpNoDelay = 1;
        if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPIDLE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPINTVL");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARNING("fail to set TCP_NODELAY");
        }
        x_sock_set_block(iAcceptFd, 0);
        inet_ntop(AF_INET, &stClientAddr.sin_addr, szClientAddr, sizeof(szClientAddr));
        iClientPort = CodecUtil::N2H(stClientAddr.sin_port);
        LOG4_TRACE("accept connect from \"%s\"", szClientAddr);
    }
    else    // AF_INET6
    {
        struct sockaddr_in6 stClientAddr;
        socklen_t clientAddrSize = sizeof(stClientAddr);
        iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
        if (iAcceptFd < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
            return(false);
        }
        int iKeepAlive = 1;
        int iKeepIdle = 60;
        int iKeepInterval = 5;
        int iKeepCount = 3;
        int iTcpNoDelay = 1;
        if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPIDLE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPINTVL");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARNING("fail to set TCP_NODELAY");
        }
        x_sock_set_block(iAcceptFd, 0);
        inet_ntop(AF_INET6, &stClientAddr.sin6_addr, szClientAddr, sizeof(szClientAddr));
        iClientPort = CodecUtil::N2H(stClientAddr.sin6_port);
        LOG4_TRACE("accept connect from \"%s\"", szClientAddr);
    }

    auto iter = m_mapClientConnFrequency.find(std::string(szClientAddr));
    if (iter == m_mapClientConnFrequency.end())
    {
        m_mapClientConnFrequency.insert(std::make_pair(std::string(szClientAddr), 1));
        AddClientConnFrequencyTimeout(szClientAddr, m_pLabor->GetNodeInfo().dAddrStatInterval);
    }
    else
    {
        iter->second++;
        if (((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd > 2 && iter->second > (uint32)m_pLabor->GetNodeInfo().iAddrPermitNum)
        {
            LOG4_WARNING("client addr %s had been connected more than %u times in %f seconds, it's not permitted",
                            szClientAddr, m_pLabor->GetNodeInfo().iAddrPermitNum, m_pLabor->GetNodeInfo().dAddrStatInterval);
            close(iAcceptFd);
            return(false);
        }
    }

    int iWorkerId = -1;
    switch (m_pLabor->GetNodeInfo().iConnectionDispatch)
    {
        case DISPATCH_ROUND_ROBIN:
            iWorkerId = ((Manager*)m_pLabor)->GetSessionManager()->GetDispatchWorkerId(iFd);
            break;
        case DISPATCH_CLIENT_ADDR_HASH:
            iWorkerId = ((Manager*)m_pLabor)->GetSessionManager()->GetDispatchWorkerId(iFd, szClientAddr, iClientPort);
            break;
        default:
            iWorkerId = ((Manager*)m_pLabor)->GetSessionManager()->GetDispatchWorkerId(iFd);
    }
    if (iWorkerId > 0)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        FdTransfer oFdTransferInfo;
        std::string strFdTransferInfo;
        oFdTransferInfo.set_fd(iAcceptFd);
        oFdTransferInfo.set_addr_family(iFamily);
        oFdTransferInfo.set_client_addr(szClientAddr);
        oFdTransferInfo.set_client_port(iClientPort);
        oFdTransferInfo.set_codec_type(m_pLabor->GetNodeInfo().eCodec);
        oFdTransferInfo.SerializeToString(&strFdTransferInfo);
        oMsgBody.set_data(strFdTransferInfo);
        oMsgHead.set_cmd(CMD_REQ_FD_TRANSFER);
        uint32 uiManagerLaborId = m_pLabor->GetNodeInfo().uiWorkerNum + 1;
        LOG4_INFO("transfer fd %d from labor %u to labor %u", iAcceptFd, uiManagerLaborId, iWorkerId);
        int iResult = CodecNebulaInNode::Write(uiManagerLaborId, iWorkerId, gc_uiCmdReq, m_pLabor->GetSequence(), oMsgHead, oMsgBody);
        if (ERR_OK == iResult)
        {
            return(true);
        }
        else
        {
            LOG4_ERROR("transfer fd %d to worker %d error %d", iAcceptFd, iWorkerId, iResult);
            return(false);
        }
    }
    LOG4_ERROR("no available worker");
    close(iAcceptFd);
    return(false);
}

bool Dispatcher::AcceptServerConn(int iFd)
{
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        return(false);
    }
    else
    {
        /* tcp连接检测 */
        int iKeepAlive = 1;
        int iKeepIdle = 60;
        int iKeepInterval = 5;
        int iKeepCount = 3;
        int iTcpNoDelay = 1;
        if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPIDLE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPINTVL");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARNING("fail to set TCP_NODELAY");
        }
        x_sock_set_block(iAcceptFd, 0);
        std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(iAcceptFd, CODEC_NEBULA);
        if (NULL != pChannel)
        {
            ev_tstamp dIoTimeout = (m_pLabor->GetNodeInfo().dConnectionProtection > 0)
                ? m_pLabor->GetNodeInfo().dConnectionProtection : m_pLabor->GetNodeInfo().dIoTimeout;
            AddIoTimeout(pChannel, dIoTimeout);
            AddIoReadEvent(pChannel);
            m_pLabor->IoStatAddConnection(IO_STAT_DOWNSTREAM_NEW_CONNECTION);
        }
    }
    return(false);
}

bool Dispatcher::PingChannel(std::shared_ptr<SocketChannel> pChannel)
{
    auto iter = m_mapChannelPingStepName.find(pChannel->GetCodecType());
    if (iter == m_mapChannelPingStepName.end())
    {
        return(false);
    }
    else
    {
        auto pStep = m_pLabor->GetActorBuilder()->MakeSharedStep(nullptr, iter->second, pChannel);
        if (nullptr == pStep)
        {
            LOG4_ERROR("failed to ping channel.");
            return(false);
        }
        else
        {
            E_CMD_STATUS eStatus = pStep->Emit();
            if (CMD_STATUS_RUNNING != eStatus)
            {
                m_pLabor->GetActorBuilder()->RemoveStep(pStep);
                return(false);
            }
            return(true);
        }
    }
}

void Dispatcher::CheckFailedNode()
{
    if (GetNowTime() - m_lLastCheckNodeTime >= 60)
    {
        m_pSessionNode->CheckFailedNode();
        m_lLastCheckNodeTime = GetNowTime();
    }
}

void Dispatcher::EvBreak()
{
    ev_break (m_loop, EVBREAK_ALL);
}

}

