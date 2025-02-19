/*******************************************************************************
 * Project:  Nebula
 * @file     IO.hpp
 * @brief    in and out
 * @author   Bwar
 * @date:    2021-10-01
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_IOS_IO_HPP_
#define SRC_IOS_IO_HPP_

#include <memory>
#include "channel/SocketChannel.hpp"
#include "channel/SocketChannelImpl.hpp"
#include "channel/SocketChannelSslImpl.hpp"
#include "ios/ActorWatcher.hpp"
#include "ios/Dispatcher.hpp"
#include "labor/Labor.hpp"
#include "labor/LaborShared.hpp"
#include "actor/Actor.hpp"
#include "actor/ActorBuilder.hpp"
#include "actor/chain/Chain.hpp"
#include "codec/CodecFactory.hpp"
#include "actor/session/sys_session/TimerAddrinfo.hpp"

namespace neb
{

class Actor;
class Dispatcher;

template<typename T>
class IO
{
public:
    IO(){}
    virtual ~IO(){}

    static bool Send(std::shared_ptr<SocketChannel> pChannel);

    template<typename ...Targs>
    static bool SendResponse(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

    template<typename ...Targs>
    static bool SendResponse(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

    template<typename ...Targs>
    static bool SendRequest(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

    template<typename ...Targs>
    static bool SendRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

    static std::shared_ptr<SocketChannel> ApplySocketChannel(Actor* pActor, const ChannelOption& stOption, const std::string& strIdentify);

    static std::shared_ptr<SocketChannel> ApplySocketChannel(Actor* pActor, const ChannelOption& stOption, const std::string& strHost, int iPort);

    // for spec channel
    template<typename ...Targs>
    static bool TransmitTo(Actor* pActor, uint32 uiTargetLaborId, uint32 uiCallbackStepSeq, Targs&&... args);

    // for spec channel, package deliver
    template<typename ...Targs>
    static bool DeliverTo(Actor* pActor, uint32 uiTargetLaborId, uint32 uiCallbackStepSeq, Targs&&... args);

    template<typename ...Targs>
    static bool DeliverReply(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, uint32 uiPeerStepSeq, Targs&&... args);

    template <typename ...Targs>
    static bool SendWithoutOption(Actor* pActor, const std::string& strIdentify, Targs&&... args);

    template <typename ...Targs>
    static bool SendTo(Actor* pActor, const std::string& strIdentify, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strIdentify, Targs&&... args);

    template <typename ...Targs>
    static bool SendTo(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strIdentify, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendWithoutOption(Actor* pActor, const std::string& strHost, int iPort, Targs&&... args);

    template <typename ...Targs>
    static bool SendTo(Actor* pActor, const std::string& strHost, int iPort, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strHost, int iPort, Targs&&... args);

    template <typename ...Targs>
    static bool SendTo(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strHost, int iPort, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendRoundRobinWithoutOption(Actor* pActor, const std::string& strNodeType, Targs&&... args);

    template <typename ...Targs>
    static bool SendRoundRobin(Actor* pActor, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendRoundRobinWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, Targs&&... args);

    template <typename ...Targs>
    static bool SendRoundRobin(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendOrientedWithoutOption(Actor* pActor, const std::string& strNodeType,
            uint32 uiFactor, Targs&&... args);

    template <typename ...Targs>
    static bool SendOriented(Actor* pActor, const std::string& strNodeType,
            uint32 uiFactor, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendOrientedWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq,
            const std::string& strNodeType, uint32 uiFactor, Targs&&... args);

    template <typename ...Targs>
    static bool SendOriented(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType,
            uint32 uiFactor, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendOrientedWithoutOption(Actor* pActor, const std::string& strNodeType,
            const std::string& strFactor, Targs&&... args);

    template <typename ...Targs>
    static bool SendOriented(Actor* pActor, const std::string& strNodeType,
            const std::string& strFactor, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool SendOrientedWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType,
            const std::string& strFactor, Targs&&... args);

    template <typename ...Targs>
    static bool SendOriented(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType,
            const std::string& strFactor, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool BroadcastWithoutOption(Actor* pActor, const std::string& strNodeType, Targs&&... args);

    template <typename ...Targs>
    static bool Broadcast(Actor* pActor, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static bool BroadcastWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, Targs&&... args);

    template <typename ...Targs>
    static bool Broadcast(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args);

    template <typename ...Targs>
    static uint32 SendToSelf(Actor* pActor, Targs&&... args);

    template<typename ...Targs>
    static E_CODEC_STATUS Recv(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

    template<typename ...Targs>
    static E_CODEC_STATUS Fetch(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

    template<typename ...Targs>
    static bool OnMessage(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);   // default

    template<typename ...Targs>
    static bool OnRequest(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int32 iCmd, Targs&&... args);

    template<typename ...Targs>
    static bool OnRequest(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, const std::string& strPath, Targs&&... args);

    template<typename ...Targs>
    static bool OnResponse(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, uint32 uiStreamId, E_CODEC_STATUS eCodecStatus, Targs&&... args);

    // SelfChannel response,  SpecChannel response
    template<typename ...Targs>
    static bool OnResponse(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, uint32 uiStepSeq, Targs&&... args);

    template<typename ...Targs>
    static bool OnMessage(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

    template<typename ...Targs>
    static bool OnRequest(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, int32 iCmd, Targs&&... args);

    template<typename ...Targs>
    static bool OnRequest(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, const std::string& strPath, Targs&&... args);

    template<typename ...Targs>
    static bool OnResponse(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, uint32 uiStreamId, E_CODEC_STATUS eCodecStatus, Targs&&... args);

    // SelfChannel response
    template<typename ...Targs>
    static bool OnResponse(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, uint32 uiStepSeq, Targs&&... args);

    template<typename ...Targs>
    static std::shared_ptr<SocketChannel> CreateSocketChannel(Dispatcher* pDispatcher, int iFd, bool bIsClient, bool bWithSsl);

protected:
    template <typename ...Targs>
    static bool AutoSendWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strIdentify, Targs&&... args);
    template <typename ...Targs>
    static bool AutoSend(Dispatcher* pDispatcher, uint32 uiStepSeq,
            const std::string& strHost, int iPort, const ChannelOption& stOption, Targs&&... args);
    template <typename ...Targs>
    static bool AutoSend(Dispatcher* pDispatcher, uint32 uiStepSeq,
            const std::string& strIdentify, const ChannelOption& stOption, Targs&&... args);
    template <typename ...Targs>
    static bool AutoSend(Dispatcher* pDispatcher, uint32 uiStepSeq,
            const std::string& strIdentify, const std::string& strHost, int iPort,
            int iRemoteWorkerIndex, const ChannelOption& stOption, Targs&&... args);
    static std::shared_ptr<SocketChannel> NewSocketChannel(Dispatcher* pDispatcher, const ChannelOption& stOption,
            const std::string& strIdentify, const std::string& strHost, int iPort, int iRemoteWorkerIndex);
    static bool SplitIdentify(const std::string& strIdentify, std::string& strHost, int& iPort, int& iWorkerIndex, std::string& strError);
    static in_addr_t inet_addr(const char* text, uint32 len);
};

template<typename T>
bool IO<T>::Send(std::shared_ptr<SocketChannel> pChannel)
{
    if (pChannel->m_pImpl == nullptr)
    {
        return(false);
    }
    if (pChannel->WithSsl())
    {
#ifdef WITH_OPENSSL
        std::static_pointer_cast<SocketChannelSslImpl<T>>(pChannel->m_pImpl)->Send();
#else
        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->Send();
#endif
    }
    else
    {
        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->Send();
    }
    return(true);
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendResponse(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pChannel->GetCodecType() == CODEC_TRANSFER) // spec channel
        {
            uint32 uiPeerStepSeq = pActor->GetPeerStepSeq();
            if (uiPeerStepSeq == 0)
            {
                pActor->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
                        "no peer step seq for response");
                return(false);
            }
            int iResult = T::Write(pChannel, 0, uiPeerStepSeq, std::forward<Targs>(args)...);
            if (ERR_OK == iResult)
            {
                return(true);
            }
            pActor->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
                    "spec channel error %d", iResult);
            return(false);
        }
        return(SendResponse(pActor->m_pLabor->GetDispatcher(), pChannel, std::forward<Targs>(args)...));
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendResponse(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    if (pChannel->m_pImpl == nullptr)
    {
        pDispatcher->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
                "no channel impl");
        return(false);
    }
    if (CODEC_UNKNOW == pChannel->GetCodecType())
    {
        LOG4_TRACE_DISPATCH("CODEC_UNKNOW is invalid, channel had not been init?");
        return(false);
    }
    if (pChannel->GetCodecType() == CODEC_DIRECT)   // self channel
    {
        auto pSelfChannel = std::dynamic_pointer_cast<SelfChannel>(pChannel);
        if (pSelfChannel == nullptr)
        {
            LOG4_TRACE_DISPATCH("channel is not a self channel.");
            return(false);
        }
        pSelfChannel->SetResponse();
        return(CodecFactory::OnSelfResponse(pDispatcher, pSelfChannel, std::forward<Targs>(args)...));
    }
    if (T::Type() != pChannel->GetCodecType())
    {
        E_CODEC_TYPE eOriginCodecType = pChannel->GetCodecType();
        if (SocketChannelImpl<T>::NewCodec(pChannel, pDispatcher->m_pLabor, pDispatcher->m_pLogger, T::Type()))
        {
            pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                    "channel %s[%d] change codec type from %d to %d",
                    pChannel->GetIdentify().c_str(), pChannel->GetFd(), eOriginCodecType, pChannel->GetCodecType());
        }
        else
        {
            LOG4_TRACE_DISPATCH("failed to change codec type of channel %s[%d] from %d to %d",
                    pChannel->GetIdentify().c_str(), pChannel->GetFd(), eOriginCodecType, pChannel->GetCodecType());
            return(false);
        }
    }
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    if (pChannel->WithSsl())
    {
#ifdef WITH_OPENSSL
        eStatus = std::static_pointer_cast<SocketChannelSslImpl<T>>(
                pChannel->m_pImpl)->SendResponse(std::forward<Targs>(args)...);
#else
        eStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                pChannel->m_pImpl)->SendResponse(std::forward<Targs>(args)...);
#endif
    }
    else
    {
        eStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                pChannel->m_pImpl)->SendResponse(std::forward<Targs>(args)...);
    }
    if (pChannel->IsClient())
    {
        pDispatcher->m_pLabor->IoStatAddSendNum(pChannel->GetFd(), IO_STAT_UPSTREAM_SEND_NUM);
    }
    else
    {
        pDispatcher->m_pLabor->IoStatAddSendNum(pChannel->GetFd(), IO_STAT_DOWNSTREAM_SEND_NUM);
    }
    pDispatcher->m_pLastActivityChannel = pChannel;
    switch (eStatus)
    {
        case CODEC_STATUS_OK:
            return(true);
        case CODEC_STATUS_PAUSE:
        case CODEC_STATUS_WANT_WRITE:
        case CODEC_STATUS_PART_OK:
            pDispatcher->AddIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_WANT_READ:
            pDispatcher->RemoveIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_EOF:      // a case: http1.0 respone and close
            pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                    "eStatus = %d, %s", eStatus, pChannel->GetIdentify().c_str());
            pDispatcher->DiscardSocketChannel(pChannel);
            return(true);
        default:
            pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                    "eStatus = %d, %s", eStatus, pChannel->GetIdentify().c_str());
            pDispatcher->DiscardSocketChannel(pChannel);
            return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendRequest(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendRequest(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    pChannel, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendRequest(pActor->m_pLabor->GetDispatcher(), 0,
                    pChannel, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    if (pChannel->m_pImpl == nullptr)
    {
        pDispatcher->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
                "no channel impl");
        return(false);
    }
    if (CODEC_UNKNOW == pChannel->GetCodecType())
    {
        LOG4_TRACE_DISPATCH("CODEC_UNKNOW is invalid, channel had not been init?");
        return(false);
    }
    if (T::Type() != pChannel->GetCodecType())
    {
        E_CODEC_TYPE eOriginCodecType = pChannel->GetCodecType();
        if (SocketChannelImpl<T>::NewCodec(pChannel, pDispatcher->m_pLabor, pDispatcher->m_pLogger, T::Type()))
        {
            pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                    "channel %s[%d] change codec type from %d to %d",
                    pChannel->GetIdentify().c_str(), eOriginCodecType, pChannel->GetCodecType());
        }
        else
        {
            LOG4_TRACE_DISPATCH("failed to change codec type of channel %s[%d] from %d to %d",
                    pChannel->GetIdentify().c_str(), eOriginCodecType, pChannel->GetCodecType());
            return(false);
        }
    }
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    if (pChannel->WithSsl())
    {
#ifdef WITH_OPENSSL
        eStatus = std::static_pointer_cast<SocketChannelSslImpl<T>>(
                pChannel->m_pImpl)->SendRequest(uiStepSeq, std::forward<Targs>(args)...);
#else
        eStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                pChannel->m_pImpl)->SendRequest(uiStepSeq, std::forward<Targs>(args)...);
#endif
    }
    else
    {
        eStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                pChannel->m_pImpl)->SendRequest(uiStepSeq, std::forward<Targs>(args)...);
    }
    if (pChannel->IsClient())
    {
        pDispatcher->m_pLabor->IoStatAddSendNum(pChannel->GetFd(), IO_STAT_UPSTREAM_SEND_NUM);
    }
    else
    {
        pDispatcher->m_pLabor->IoStatAddSendNum(pChannel->GetFd(), IO_STAT_DOWNSTREAM_SEND_NUM);
    }
    pDispatcher->m_pLastActivityChannel = pChannel;
    switch (eStatus)
    {
        case CODEC_STATUS_OK:
            return(true);
        case CODEC_STATUS_PAUSE:
        case CODEC_STATUS_WANT_WRITE:
        case CODEC_STATUS_PART_OK:
            pDispatcher->AddIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_WANT_READ:
            pDispatcher->RemoveIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_EOF:      // a case: http1.0 respone and close
            pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                    "eStatus = %d, %s", eStatus, pChannel->GetIdentify().c_str());
            pDispatcher->DiscardSocketChannel(pChannel);
            return(true);
        default:
            pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                    "eStatus = %d, %s", eStatus, pChannel->GetIdentify().c_str());
            pDispatcher->DiscardSocketChannel(pChannel);
            return(false);
    }
}

template<typename T>
std::shared_ptr<SocketChannel> IO<T>::ApplySocketChannel(Actor* pActor, const ChannelOption& stOption, const std::string& strIdentify)
{
    if (pActor == nullptr)
    {
        return(nullptr);
    }
    else
    {
        std::string strError;
        std::string strHost;
        int iPort = 0;
        int iRemoteWorkerIndex = -1;
        if (!SplitIdentify(strIdentify, strHost, iPort, iRemoteWorkerIndex, strError))
        {
            pActor->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "%s", strError.c_str());
            return(nullptr);
        }
        auto pDispatcher = pActor->m_pLabor->GetDispatcher();
        auto named_iter = pDispatcher->m_mapNamedSocketChannel.find(strIdentify);
        if (named_iter == pDispatcher->m_mapNamedSocketChannel.end())
        {
            return(NewSocketChannel(pDispatcher, stOption, strIdentify, strHost, iPort, iRemoteWorkerIndex));
        }
        else
        {
            if (named_iter->second.empty())
            {
                return(NewSocketChannel(pDispatcher, stOption, strIdentify, strHost, iPort, iRemoteWorkerIndex));
            }
            else
            {
                auto channel_iter = named_iter->second.begin();
                auto pChannel = *channel_iter;
                if (!pChannel->IsPipeline())
                {
                    named_iter->second.erase(channel_iter);
                }
                return(pChannel);
            }
        }
    }
}

template<typename T>
std::shared_ptr<SocketChannel> IO<T>::ApplySocketChannel(Actor* pActor, const ChannelOption& stOption, const std::string& strHost, int iPort)
{
    if (pActor == nullptr)
    {
        return(nullptr);
    }
    else
    {
        std::ostringstream ossIdentify;
        ossIdentify << strHost << ":" << iPort;
        auto pDispatcher = pActor->m_pLabor->GetDispatcher();
        auto named_iter = pDispatcher->m_mapNamedSocketChannel.find(ossIdentify.str());
        if (named_iter == pDispatcher->m_mapNamedSocketChannel.end())
        {
            return(NewSocketChannel(pDispatcher, stOption, ossIdentify.str(), strHost, iPort, -1));
        }
        else
        {
            if (named_iter->second.empty())
            {
                return(NewSocketChannel(pDispatcher, stOption, ossIdentify.str(), strHost, iPort, -1));
            }
            else
            {
                auto channel_iter = named_iter->second.begin();
                auto pChannel = *channel_iter;
                if (!pChannel->IsPipeline())
                {
                    named_iter->second.erase(channel_iter);
                }
                return(pChannel);
            }
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::TransmitTo(Actor* pActor, uint32 uiTargetLaborId, uint32 uiCallbackStepSeq, Targs&&... args)
{
    int iResult = T::Write(pActor->GetLaborId(), uiTargetLaborId, gc_uiCmdReq, uiCallbackStepSeq, std::forward<Targs>(args)...);
    if (ERR_OK == iResult)
    {
        return(true);
    }
    pActor->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "spec channel error %d", iResult);
    return(false);
}

template<typename T>
template<typename ...Targs>
bool IO<T>::DeliverTo(Actor* pActor, uint32 uiTargetLaborId, uint32 uiCallbackStepSeq, Targs&&... args)
{
    return(TransmitTo(pActor, uiTargetLaborId, uiCallbackStepSeq, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::DeliverReply(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, uint32 uiPeerStepSeq, Targs&&... args)
{
    if (uiPeerStepSeq == 0)
    {
        pActor->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
               "no peer step seq for response");
        return(false);
    }
    int iResult = T::Write(pChannel, 0, uiPeerStepSeq, std::forward<Targs>(args)...);
    if (ERR_OK == iResult)
    {
        return(true);
    }
    pActor->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "spec channel error %d", iResult);
    return(false);
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendWithoutOption(Actor* pActor, const std::string& strIdentify, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendWithoutOption(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strIdentify, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendWithoutOption(pActor->m_pLabor->GetDispatcher(), 0,
                    strIdentify, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendTo(Actor* pActor, const std::string& strIdentify, const ChannelOption& stOption, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendTo(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strIdentify, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendTo(pActor->m_pLabor->GetDispatcher(), 0,
                    strIdentify, stOption, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strIdentify, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("identify: %s", strIdentify.c_str());

    auto named_iter = pDispatcher->m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == pDispatcher->m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE_DISPATCH("no channel match %s.", strIdentify.c_str());
        return(AutoSendWithoutOption(pDispatcher, uiStepSeq, strIdentify, std::forward<Targs>(args)...));
    }
    else
    {
        if (named_iter->second.empty())
        {
            return(AutoSendWithoutOption(pDispatcher, uiStepSeq, strIdentify, std::forward<Targs>(args)...));
        }
        else
        {
            auto channel_iter = named_iter->second.begin();
            auto pChannel = *channel_iter;
            bool bResult = SendRequest(pDispatcher, uiStepSeq, pChannel, std::forward<Targs>(args)...);
            if (!pChannel->IsPipeline() && bResult)
            {
                named_iter->second.erase(channel_iter);
            }
            return(bResult);
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendTo(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strIdentify, const ChannelOption& stOption, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("identify: %s", strIdentify.c_str());

    auto named_iter = pDispatcher->m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == pDispatcher->m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE_DISPATCH("no channel match %s.", strIdentify.c_str());
        return(AutoSend(pDispatcher, uiStepSeq, strIdentify, stOption, std::forward<Targs>(args)...));
    }
    else
    {
        if (named_iter->second.empty())
        {
            return(AutoSend(pDispatcher, uiStepSeq, strIdentify, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            auto channel_iter = named_iter->second.begin();
            auto pChannel = *channel_iter;
            bool bResult = SendRequest(pDispatcher, uiStepSeq, pChannel, std::forward<Targs>(args)...);
            if (!pChannel->IsPipeline() && bResult)
            {
                named_iter->second.erase(channel_iter);
            }
            return(bResult);
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendWithoutOption(Actor* pActor, const std::string& strHost, int iPort, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendWithoutOption(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(), strHost, iPort,
                    std::forward<Targs>(args)...));
        }
        else
        {
            return(SendWithoutOption(pActor->m_pLabor->GetDispatcher(), 0, strHost, iPort,
                    std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendTo(Actor* pActor, const std::string& strHost, int iPort, const ChannelOption& stOption, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendTo(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strHost, iPort, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendTo(pActor->m_pLabor->GetDispatcher(), 0,
                    strHost, iPort, stOption, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strHost, int iPort, Targs&&... args)
{
    pDispatcher->Logger(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "host %s port %d", strHost.c_str(), iPort);
    std::ostringstream ossIdentify;
    ossIdentify << strHost << ":" << iPort;
    auto named_iter = pDispatcher->m_mapNamedSocketChannel.find(ossIdentify.str());
    if (named_iter == pDispatcher->m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE_DISPATCH("no channel match %s.", ossIdentify.str().c_str());
        return(AutoSendWithoutOption(pDispatcher, uiStepSeq, strHost, iPort, std::forward<Targs>(args)...));
    }
    else
    {
        auto channel_iter = named_iter->second.begin();
        if (channel_iter == named_iter->second.end())
        {
            return(AutoSendWithoutOption(pDispatcher, uiStepSeq, strHost, iPort, std::forward<Targs>(args)...));
        }
        auto pChannel = *channel_iter;
        bool bResult = SendRequest(pDispatcher, uiStepSeq, pChannel, std::forward<Targs>(args)...);
        if (!pChannel->IsPipeline() && bResult)
        {
            named_iter->second.erase(channel_iter);
        }
        return(bResult);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendTo(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strHost, int iPort, const ChannelOption& stOption, Targs&&... args)
{
    pDispatcher->Logger(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "host %s port %d", strHost.c_str(), iPort);
    std::ostringstream ossIdentify;
    ossIdentify << strHost << ":" << iPort;
    auto named_iter = pDispatcher->m_mapNamedSocketChannel.find(ossIdentify.str());
    if (named_iter == pDispatcher->m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE_DISPATCH("no channel match %s.", ossIdentify.str().c_str());
        return(AutoSend(pDispatcher, uiStepSeq, strHost, iPort, stOption, std::forward<Targs>(args)...));
    }
    else
    {
        auto channel_iter = named_iter->second.begin();
        if (channel_iter == named_iter->second.end())
        {
            return(AutoSend(pDispatcher, uiStepSeq, strHost, iPort, stOption, std::forward<Targs>(args)...));
        }
        auto pChannel = *channel_iter;
        bool bResult = SendRequest(pDispatcher, uiStepSeq, pChannel, std::forward<Targs>(args)...);
        if (!pChannel->IsPipeline() && bResult)
        {
            named_iter->second.erase(channel_iter);
        }
        return(bResult);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendRoundRobinWithoutOption(Actor* pActor, const std::string& strNodeType, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendRoundRobinWithoutOption(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendRoundRobinWithoutOption(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendRoundRobin(Actor* pActor, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendRoundRobin(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendRoundRobin(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, stOption, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendRoundRobinWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (pDispatcher->m_pSessionNode->NodeDetect(strNodeType, strOnlineNode))
    {
        pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                "NodeDetect(%s, %s)", strNodeType.c_str(), strOnlineNode.c_str());
        SendWithoutOption(pDispatcher, 0, strOnlineNode);
    }
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
        if (pChannelOption == nullptr)
        {
            ChannelOption stOption;
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, *(pChannelOption.get()), std::forward<Targs>(args)...));
        }
    }
    else
    {
        LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
            if (pChannelOption == nullptr)
            {
                ChannelOption stOption;
                return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
            }
            else
            {
                return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, *(pChannelOption.get()), std::forward<Targs>(args)...));
            }
        }
        LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendRoundRobin(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (pDispatcher->m_pSessionNode->NodeDetect(strNodeType, strOnlineNode))
    {
        pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                "NodeDetect(%s, %s)", strNodeType.c_str(), strOnlineNode.c_str());
        SendWithoutOption(pDispatcher, 0, strOnlineNode);
    }
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
    }
    else
    {
        LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
        }
        LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOrientedWithoutOption(Actor* pActor, const std::string& strNodeType,
        uint32 uiFactor, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendOrientedWithoutOption(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, uiFactor, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendOrientedWithoutOption(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, uiFactor, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOriented(Actor* pActor, const std::string& strNodeType,
        uint32 uiFactor, const ChannelOption& stOption, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendOriented(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, uiFactor, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendOriented(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, uiFactor, stOption, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOrientedWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType,
        uint32 uiFactor, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (pDispatcher->m_pSessionNode->NodeDetect(strNodeType, strOnlineNode))
    {
        pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                        "NodeDetect(%s, %s)", strNodeType.c_str(), strOnlineNode.c_str());
        SendWithoutOption(pDispatcher, 0, strOnlineNode);
    }
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, uiFactor, strOnlineNode))
    {
        auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
        if (pChannelOption == nullptr)
        {
            ChannelOption stOption;
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, *(pChannelOption.get()), std::forward<Targs>(args)...));
        }
    }
    else
    {
        LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
            if (pChannelOption == nullptr)
            {
                ChannelOption stOption;
                return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
            }
            else
            {
                return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, *(pChannelOption.get()), std::forward<Targs>(args)...));
            }
        }
        LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOriented(Dispatcher* pDispatcher, uint32 uiStepSeq,
        const std::string& strNodeType, uint32 uiFactor, const ChannelOption& stOption, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (pDispatcher->m_pSessionNode->NodeDetect(strNodeType, strOnlineNode))
    {
        pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                        "NodeDetect(%s, %s)", strNodeType.c_str(), strOnlineNode.c_str());
        SendWithoutOption(pDispatcher, 0, strOnlineNode);
    }
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, uiFactor, strOnlineNode))
    {
        return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
    }
    else
    {
        LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
        }
        LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOrientedWithoutOption(Actor* pActor, const std::string& strNodeType,
        const std::string& strFactor, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendOrientedWithoutOption(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, strFactor, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendOrientedWithoutOption(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, strFactor, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOriented(Actor* pActor, const std::string& strNodeType,
        const std::string& strFactor, const ChannelOption& stOption, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(SendOriented(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, strFactor, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendOriented(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, strFactor, stOption, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOrientedWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType,
        const std::string& strFactor, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (pDispatcher->m_pSessionNode->NodeDetect(strNodeType, strOnlineNode))
    {
        pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                        "NodeDetect(%s, %s)", strNodeType.c_str(), strOnlineNode.c_str());
        SendWithoutOption(pDispatcher, 0, strOnlineNode);
    }
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, strFactor, strOnlineNode))
    {
        auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
        if (pChannelOption == nullptr)
        {
            ChannelOption stOption;
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, *(pChannelOption.get()), std::forward<Targs>(args)...));
        }
    }
    else
    {
        LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
            if (pChannelOption == nullptr)
            {
                ChannelOption stOption;
                return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
            }
            else
            {
                return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, *(pChannelOption.get()), std::forward<Targs>(args)...));
            }
        }
        LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::SendOriented(Dispatcher* pDispatcher, uint32 uiStepSeq,
        const std::string& strNodeType, const std::string& strFactor, const ChannelOption& stOption, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (pDispatcher->m_pSessionNode->NodeDetect(strNodeType, strOnlineNode))
    {
        pDispatcher->Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                        "NodeDetect(%s, %s)", strNodeType.c_str(), strOnlineNode.c_str());
        SendWithoutOption(pDispatcher, 0, strOnlineNode);
    }
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, strFactor, strOnlineNode))
    {
        return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
    }
    else
    {
        LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            return(SendTo(pDispatcher, uiStepSeq, strOnlineNode, stOption, std::forward<Targs>(args)...));
        }
        LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::BroadcastWithoutOption(Actor* pActor, const std::string& strNodeType, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(BroadcastWithoutOption(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, std::forward<Targs>(args)...));
        }
        else
        {
            return(BroadcastWithoutOption(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::Broadcast(Actor* pActor, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args)
{
    if (pActor == nullptr)
    {
        return(false);
    }
    else
    {
        if (pActor->WantResponse())
        {
            return(Broadcast(pActor->m_pLabor->GetDispatcher(), pActor->GetSequence(),
                    strNodeType, stOption, std::forward<Targs>(args)...));
        }
        else
        {
            return(Broadcast(pActor->m_pLabor->GetDispatcher(), 0,
                    strNodeType, stOption, std::forward<Targs>(args)...));
        }
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::BroadcastWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::set<std::string> setOnlineNodes;
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
    {
        bool bSendResult = false;
        auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
        if (pChannelOption == nullptr)
        {
            ChannelOption stOption;
            for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
            {
                bSendResult |= SendTo(pDispatcher, uiStepSeq, *node_iter, stOption, std::forward<Targs>(args)...);
            }
        }
        else
        {
            for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
            {
                bSendResult |= SendTo(pDispatcher, uiStepSeq, *node_iter, *(pChannelOption.get()), std::forward<Targs>(args)...);
            }
        }
        return(bSendResult);
    }
    else
    {
        if ("BEACON" == strNodeType)
        {
            pDispatcher->Logger(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "no beacon config.");
        }
        else
        {
            LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
            std::string strOnlineNode;
            if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
            {
                if (pDispatcher->m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
                {
                    bool bSendResult = false;
                    auto pChannelOption = pDispatcher->GetChannelOption(strNodeType);
                    if (pChannelOption == nullptr)
                    {
                        ChannelOption stOption;
                        for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
                        {
                            bSendResult |= SendTo(pDispatcher, uiStepSeq, *node_iter, stOption, std::forward<Targs>(args)...);
                        }
                    }
                    else
                    {
                        for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
                        {
                            bSendResult |= SendTo(pDispatcher, uiStepSeq, *node_iter, *(pChannelOption.get()), std::forward<Targs>(args)...);
                        }
                    }
                    return(bSendResult);
                }
            }
            LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        }
        return(false);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::Broadcast(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strNodeType, const ChannelOption& stOption, Targs&&... args)
{
    LOG4_TRACE_DISPATCH("node_type: %s", strNodeType.c_str());
    std::set<std::string> setOnlineNodes;
    if (pDispatcher->m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
    {
        bool bSendResult = false;
        for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
        {
            bSendResult |= SendTo(pDispatcher, uiStepSeq, *node_iter, stOption, std::forward<Targs>(args)...);
        }
        return(bSendResult);
    }
    else
    {
        if ("BEACON" == strNodeType)
        {
            pDispatcher->Logger(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "no beacon config.");
        }
        else
        {
            LOG4_TRACE_DISPATCH("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
            std::string strOnlineNode;
            if (pDispatcher->m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
            {
                if (pDispatcher->m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
                {
                    bool bSendResult = false;
                    for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
                    {
                        bSendResult |= SendTo(pDispatcher, uiStepSeq, *node_iter, stOption, std::forward<Targs>(args)...);
                    }
                    return(bSendResult);
                }
            }
            LOG4_TRACE_DISPATCH("no online node match node_type \"%s\"", strNodeType.c_str());
        }
        return(false);
    }
}

template<typename T>
template <typename ...Targs>
uint32 IO<T>::SendToSelf(Actor* pActor, Targs&&... args)
{
    auto pSelfChannel = std::make_shared<SelfChannel>(pActor->m_pLabor->GetSequence());
    if (CodecFactory::OnSelfRequest(pActor->m_pLabor->GetDispatcher(),
            pActor->GetSequence(), pSelfChannel, std::forward<Targs>(args)...))
    {
        return(pSelfChannel->GetSequence());
    }
    return(0); // failed
}

template<typename T>
template<typename ...Targs>
bool IO<T>::AutoSendWithoutOption(Dispatcher* pDispatcher, uint32 uiStepSeq, const std::string& strIdentify, Targs&&... args)
{
    auto pChannelOption = pDispatcher->GetChannelOption(strIdentify);
    if (pChannelOption == nullptr)
    {
        ChannelOption stOption;
        return(AutoSend(pDispatcher, uiStepSeq, strIdentify, stOption, std::forward<Targs>(args)...));
    }
    else
    {
        return(AutoSend(pDispatcher, uiStepSeq, strIdentify, (*pChannelOption.get()), std::forward<Targs>(args)...));
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::AutoSend(Dispatcher* pDispatcher, uint32 uiStepSeq,
        const std::string& strHost, int iPort, const ChannelOption& stOption, Targs&&... args)
{
    std::ostringstream ossIdentify;
    ossIdentify << strHost << ":" << iPort;
    auto pChannelOption = pDispatcher->GetChannelOption(ossIdentify.str());
    if (pChannelOption == nullptr)
    {
        ChannelOption stOption;
        return(AutoSend(pDispatcher, uiStepSeq, ossIdentify.str(),
                strHost, iPort, -1, stOption, std::forward<Targs>(args)...));
    }
    else
    {
        return(AutoSend(pDispatcher, uiStepSeq, ossIdentify.str(),
                strHost, iPort, -1, (*pChannelOption.get()), std::forward<Targs>(args)...));
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::AutoSend(Dispatcher* pDispatcher, uint32 uiStepSeq,
        const std::string& strIdentify, const ChannelOption& stOption, Targs&&... args)
{
    std::string strError;
    std::string strHost;
    int iPort = 0;
    int iRemoteWorkerIndex = -1;
    if (!SplitIdentify(strIdentify, strHost, iPort, iRemoteWorkerIndex, strError))
    {
        pDispatcher->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "%s", strError.c_str());
        return(false);
    }
    return(AutoSend(pDispatcher, uiStepSeq, strIdentify,
            strHost, iPort, iRemoteWorkerIndex, stOption, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::AutoSend(Dispatcher* pDispatcher, uint32 uiStepSeq,
        const std::string& strIdentify, const std::string& strHost, int iPort,
        int iRemoteWorkerIndex, const ChannelOption& stOption, Targs&&... args)
{
    std::shared_ptr<SocketChannel> pChannel = NewSocketChannel(pDispatcher, stOption, strIdentify, strHost, iPort, iRemoteWorkerIndex);
    if (nullptr == pChannel)
    {
        return(false);
    }
    else
    {
        E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
        if (pChannel->WithSsl())
        {
#ifdef WITH_OPENSSL
            eCodecStatus = std::static_pointer_cast<SocketChannelSslImpl<T>>(
                    pChannel->m_pImpl)->SendRequest(uiStepSeq, std::forward<Targs>(args)...);
#else
            eCodecStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                    pChannel->m_pImpl)->SendRequest(uiStepSeq, std::forward<Targs>(args)...);
#endif
        }
        else
        {
            eCodecStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                    pChannel->m_pImpl)->SendRequest(uiStepSeq, std::forward<Targs>(args)...);
        }
        if (pChannel->IsClient())
        {
            pDispatcher->m_pLabor->IoStatAddSendNum(pChannel->GetFd(), IO_STAT_UPSTREAM_SEND_NUM);
        }
        else
        {
            pDispatcher->m_pLabor->IoStatAddSendNum(pChannel->GetFd(), IO_STAT_DOWNSTREAM_SEND_NUM);
        }
        pDispatcher->m_pLastActivityChannel = pChannel;
        if (CODEC_STATUS_OK != eCodecStatus
                && CODEC_STATUS_PAUSE != eCodecStatus
                && CODEC_STATUS_WANT_WRITE != eCodecStatus
                && CODEC_STATUS_WANT_READ != eCodecStatus)
        {
            pDispatcher->DiscardSocketChannel(pChannel);
        }
        return(true);
    }
}

template<typename T>
std::shared_ptr<SocketChannel> IO<T>::NewSocketChannel(Dispatcher* pDispatcher, const ChannelOption& stOption,
        const std::string& strIdentify, const std::string& strHost, int iPort, int iRemoteWorkerIndex)
{
    if (pDispatcher->GetChannelOption(strIdentify) == nullptr)
    {
        pDispatcher->SetChannelOption(strIdentify, stOption);
    }
    struct addrinfo stAddrHints;
    struct addrinfo* pAddrResult = nullptr;
    struct addrinfo* pAddrCurrent = nullptr;
    auto pSessionAddr = std::dynamic_pointer_cast<TimerAddrinfo>(pDispatcher->m_pLabor->GetActorBuilder()->GetSession(strIdentify));
    if (pSessionAddr != nullptr)
    {
        pAddrResult = pSessionAddr->GetAddrinfo();
    }
    if (pAddrResult == nullptr)
    {
        memset(&stAddrHints, 0, sizeof(struct addrinfo));
        stAddrHints.ai_family = AF_UNSPEC;
        stAddrHints.ai_socktype = stOption.iSocketType;
        stAddrHints.ai_protocol = IPPROTO_IP;
        if (inet_addr(strHost.data(), strHost.length()) != 0)
        {
            stAddrHints.ai_flags |= (AI_NUMERICHOST | AI_NUMERICSERV);
        }

        LOG4_TRACE_DISPATCH("getaddrinfo for %s", strIdentify.c_str());
        int iCode = getaddrinfo(strHost.c_str(), std::to_string(iPort).c_str(), &stAddrHints, &pAddrResult);
        if (0 != iCode)
        {
            LOG4_TRACE_DISPATCH("getaddrinfo(\"%s\", \"%d\") error %d: %s",
                    strHost.c_str(), iPort, iCode, gai_strerror(iCode));
            return(nullptr);
        }
        if (pSessionAddr == nullptr)
        {
            pSessionAddr = std::dynamic_pointer_cast<TimerAddrinfo>(
                    pDispatcher->m_pLabor->GetActorBuilder()->MakeSharedSession(nullptr, "neb::TimerAddrinfo", strIdentify));
        }
        if (pSessionAddr != nullptr)
        {
            pSessionAddr->MigrateAddrinfo(pAddrResult);
        }
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

    /* No address succeeded */
//    if (pAddrCurrent == NULL)
//    {
//        LOG4_TRACE_DISPATCH("Could not connect to \"%s:%d\"", strHost.c_str(), iPort);
//        freeaddrinfo(pAddrResult);           /* No longer needed */
//        return(false);
//    }

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
    std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(pDispatcher, iFd, true, stOption.bWithSsl);
    if (nullptr != pChannel)
    {
        connect(iFd, pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen);
        if (pSessionAddr != nullptr && pAddrResult != pSessionAddr->GetAddrinfo())
        {
            freeaddrinfo(pAddrResult);           /* No longer needed */
        }
        pDispatcher->m_pLabor->IoStatAddConnection(IO_STAT_UPSTREAM_NEW_CONNECTION);
        ev_tstamp dIoTimeout = stOption.dKeepAlive;
        pChannel->SetKeepAlive(dIoTimeout);
        pDispatcher->AddIoTimeout(pChannel, dIoTimeout);
        pDispatcher->AddIoReadEvent(pChannel);
        pDispatcher->AddIoWriteEvent(pChannel);
        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->SetIdentify(strIdentify);
        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->SetRemoteAddr(strHost);
        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->SetPipeline(stOption.bPipeline);
        pDispatcher->m_pLastActivityChannel = pChannel;

        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->SetRemoteWorkerIndex(iRemoteWorkerIndex);
        if (stOption.bPipeline)
        {
            pDispatcher->AddNamedSocketChannel(strIdentify, pChannel);
        }
        return(pChannel);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        freeaddrinfo(pAddrResult);           /* No longer needed */
        close(iFd);
        return(nullptr);
    }
}

template<typename T>
in_addr_t IO<T>::inet_addr(const char* text, uint32 len)
{
    const char *p;
    unsigned char c;
    in_addr_t    addr;
    unsigned int   octet, n;

    addr = 0;
    octet = 0;
    n = 0;

    for (p = text; p < text + len; p++) {
        c = *p;

        if (c >= '0' && c <= '9') {
            octet = octet * 10 + (c - '0');

            if (octet > 255) {
                return 0;
            }

            continue;
        }

        if (c == '.') {
            addr = (addr << 8) + octet;
            octet = 0;
            n++;
            continue;
        }

        return 0;
    }

    if (n == 3) {
        addr = (addr << 8) + octet;
        return htonl(addr);
    }

    return 0;
}

template<typename T>
template<typename ...Targs>
E_CODEC_STATUS IO<T>::Recv(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    if (CODEC_UNKNOW == pChannel->GetCodecType())
    {
        LOG4_TRACE_DISPATCH("CODEC_UNKNOW is invalid, channel had not been init?");
        return(CODEC_STATUS_ERR);
    }
    if (T::Type() != pChannel->GetCodecType())
    {
        E_CODEC_TYPE eOriginCodecType = pChannel->GetCodecType();
        LOG4_TRACE_DISPATCH("failed to change codec type of channel %s[%d] from %d to %d",
                pChannel->GetIdentify().c_str(), eOriginCodecType, pChannel->GetCodecType());
        return(CODEC_STATUS_ERR);
    }
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    if (pChannel->WithSsl())
    {
#ifdef WITH_OPENSSL
        eStatus = std::static_pointer_cast<SocketChannelSslImpl<T>>(
                pChannel->m_pImpl)->Recv(std::forward<Targs>(args)...);
#else
        eStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                pChannel->m_pImpl)->Recv(std::forward<Targs>(args)...);
#endif
    }
    else
    {
        eStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
                pChannel->m_pImpl)->Recv(std::forward<Targs>(args)...);
    }
    if (CODEC_STATUS_OK == eStatus)
    {
        if (pChannel->IsClient())
        {
            pDispatcher->m_pLabor->IoStatAddRecvNum(pChannel->GetFd(), IO_STAT_UPSTREAM_RECV_NUM);
        }
        else
        {
            pDispatcher->m_pLabor->IoStatAddRecvNum(pChannel->GetFd(), IO_STAT_DOWNSTREAM_RECV_NUM);
        }
        pDispatcher->m_pLastActivityChannel = pChannel;
    }
    return(eStatus);
}

template<typename T>
template<typename ...Targs>
E_CODEC_STATUS IO<T>::Fetch(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    if (CODEC_UNKNOW == pChannel->GetCodecType())
    {
        LOG4_TRACE_DISPATCH("CODEC_UNKNOW is invalid, channel had not been init?");
        return(CODEC_STATUS_ERR);
    }
    if (T::Type() != pChannel->GetCodecType())
    {
        E_CODEC_TYPE eOriginCodecType = pChannel->GetCodecType();
        LOG4_TRACE_DISPATCH("failed to change codec type of channel %s[%d] from %d to %d",
                pChannel->GetIdentify().c_str(), eOriginCodecType, pChannel->GetCodecType());
        return(CODEC_STATUS_ERR);
    }
    E_CODEC_STATUS eStatus = std::static_pointer_cast<SocketChannelImpl<T>>(
            pChannel->m_pImpl)->Fetch(std::forward<Targs>(args)...);
    if (CODEC_STATUS_OK == eStatus)
    {
        if (pChannel->IsClient())
        {
            pDispatcher->m_pLabor->IoStatAddRecvNum(pChannel->GetFd(), IO_STAT_UPSTREAM_RECV_NUM);
        }
        else
        {
            pDispatcher->m_pLabor->IoStatAddRecvNum(pChannel->GetFd(), IO_STAT_DOWNSTREAM_RECV_NUM);
        }
        pDispatcher->m_pLastActivityChannel = pChannel;
    }
    return(eStatus);
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnMessage(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    return(OnMessage(pDispatcher->m_pLabor->GetActorBuilder(), pChannel, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnMessage(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    return(pBuilder->OnMessage(pChannel, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnRequest(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int32 iCmd, Targs&&... args)
{
    return(OnRequest(pDispatcher->m_pLabor->GetActorBuilder(), pChannel, iCmd, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnRequest(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, int32 iCmd, Targs&&... args)
{
    auto cmd_iter = pBuilder->m_mapCmd.find(iCmd);
    if (cmd_iter != pBuilder->m_mapCmd.end() && cmd_iter->second != nullptr)
    {
        auto pCmd = std::static_pointer_cast<T>(cmd_iter->second);
        return(pCmd->AnyMessage(pChannel, std::forward<Targs>(args)...));
    }
    pBuilder->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "no cmd handler found for cmd %d", iCmd);
    return(false);
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnRequest(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, const std::string& strPath, Targs&&... args)
{
    return(OnRequest(pDispatcher->m_pLabor->GetActorBuilder(), pChannel, strPath, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnRequest(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, const std::string& strPath, Targs&&... args)
{
    auto module_iter = pBuilder->m_mapModule.find(strPath);
    if (module_iter == pBuilder->m_mapModule.end())
    {
        module_iter = pBuilder->m_mapModule.find("/switch");
        if (module_iter == pBuilder->m_mapModule.end())
        {
            module_iter = pBuilder->m_mapModule.find("/route");
            if (module_iter == pBuilder->m_mapModule.end())
            {
                pBuilder->Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
                        "no module to dispose %s!", strPath.c_str());
                return(false);
            }
            else
            {
                std::static_pointer_cast<T>(module_iter->second)->AnyMessage(pChannel, std::forward<Targs>(args)...);
            }
        }
        else
        {
            std::static_pointer_cast<T>(module_iter->second)->AnyMessage(pChannel, std::forward<Targs>(args)...);
        }
    }
    else
    {
        std::static_pointer_cast<T>(module_iter->second)->AnyMessage(pChannel, std::forward<Targs>(args)...);
    }
    return(true);
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnResponse(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, uint32 uiStreamId, E_CODEC_STATUS eCodecStatus, Targs&&... args)
{
    return(OnResponse(pDispatcher->m_pLabor->GetActorBuilder(), pChannel, uiStreamId, eCodecStatus, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnResponse(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, uint32 uiStreamId, E_CODEC_STATUS eCodecStatus, Targs&&... args)
{
    LOG4_TRACE_BUILDER("stream id = %u, eCodecStatus = %d", uiStreamId, eCodecStatus);
    auto uiStepSeq = pChannel->PopStepSeq(uiStreamId, eCodecStatus);
    LOG4_TRACE_BUILDER("stream id = %u, step seq = %u", uiStreamId, uiStepSeq);
    auto step_iter = pBuilder->m_mapCallbackStep.find(uiStepSeq);
    if (!pChannel->IsPipeline() && pChannel->PipelineIsEmpty())
    {
        pBuilder->m_pLabor->GetDispatcher()->AddNamedSocketChannel(pChannel->GetIdentify(), pChannel); // push back to named socket channel pool.
    }
    if (step_iter == pBuilder->m_mapCallbackStep.end())
    {
        LOG4_TRACE_BUILDER("no callback for reply from %s, stream id %u, step seq %u!",
                pChannel->GetIdentify().c_str(), uiStreamId, uiStepSeq);
        return(false);
    }
    else
    {
        E_CMD_STATUS eResult;
        step_iter->second->SetActiveTime(pBuilder->m_pLabor->GetNowTime());
        eResult = std::static_pointer_cast<T>(step_iter->second)->Callback(pChannel, std::forward<Targs>(args)...);
        if (CMD_STATUS_RUNNING != eResult)
        {
            uint32 uiChainId = std::static_pointer_cast<T>(step_iter->second)->GetChainId();
            pBuilder->m_pLabor->GetDispatcher()->DelEvent(step_iter->second->MutableWatcher()->MutableTimerWatcher());
            step_iter->second->MutableWatcher()->Reset();
            pBuilder->m_mapCallbackStep.erase(step_iter);
            if (CMD_STATUS_FAULT != eResult && 0 != uiChainId)
            {
                auto chain_iter = pBuilder->m_mapChain.find(uiChainId);
                if (chain_iter != pBuilder->m_mapChain.end())
                {
                    chain_iter->second->SetActiveTime(pBuilder->m_pLabor->GetNowTime());
                    eResult = chain_iter->second->Next();
                    if (CMD_STATUS_RUNNING != eResult)
                    {
                        pBuilder->RemoveChain(uiChainId);
                    }
                }
            }
        }
        return(eResult);
    }
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnResponse(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, uint32 uiStepSeq, Targs&&... args)
{
    return(OnResponse(pDispatcher->m_pLabor->GetActorBuilder(), pChannel, uiStepSeq, std::forward<Targs>(args)...));
}

template<typename T>
template<typename ...Targs>
bool IO<T>::OnResponse(ActorBuilder* pBuilder, std::shared_ptr<SocketChannel> pChannel, uint32 uiStepSeq, Targs&&... args)
{
    auto step_iter = pBuilder->m_mapCallbackStep.find(uiStepSeq);
    if (step_iter == pBuilder->m_mapCallbackStep.end())
    {
        pBuilder->Logger(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__,
                "no callback for reply from %s!", pChannel->GetIdentify().c_str());
        return(false);
    }
    else
    {
        E_CMD_STATUS eResult;
        step_iter->second->SetActiveTime(pBuilder->m_pLabor->GetNowTime());
        eResult = std::static_pointer_cast<T>(step_iter->second)->Callback(pChannel, std::forward<Targs>(args)...);
        if (CMD_STATUS_RUNNING != eResult)
        {
            uint32 uiChainId = std::static_pointer_cast<T>(step_iter->second)->GetChainId();
            pBuilder->m_pLabor->GetDispatcher()->DelEvent(step_iter->second->MutableWatcher()->MutableTimerWatcher());
            step_iter->second->MutableWatcher()->Reset();
            pBuilder->m_mapCallbackStep.erase(step_iter);
            if (CMD_STATUS_FAULT != eResult && 0 != uiChainId)
            {
                auto chain_iter = pBuilder->m_mapChain.find(uiChainId);
                if (chain_iter != pBuilder->m_mapChain.end())
                {
                    chain_iter->second->SetActiveTime(pBuilder->m_pLabor->GetNowTime());
                    eResult = chain_iter->second->Next();
                    if (CMD_STATUS_RUNNING != eResult)
                    {
                        pBuilder->RemoveChain(uiChainId);
                    }
                }
            }
        }
        return(eResult);
    }
}

template<typename T>
template<typename ...Targs>
std::shared_ptr<SocketChannel> IO<T>::CreateSocketChannel(Dispatcher* pDispatcher, int iFd, bool bIsClient, bool bWithSsl)
{
    LOG4_TRACE_DISPATCH("iFd %d, codec_type %d, with_ssl = %d", iFd, T::Type(), bWithSsl);
    auto iter = pDispatcher->m_mapSocketChannel.find(iFd);
    if (iter == pDispatcher->m_mapSocketChannel.end())
    {
        auto pChannel = CodecFactory::CreateChannel(pDispatcher->m_pLabor, pDispatcher->m_pLogger, iFd, T::Type(), bIsClient, bWithSsl);
        if (pChannel != nullptr)
        {
            pDispatcher->m_mapSocketChannel.insert(std::make_pair(iFd, pChannel));
            LOG4_TRACE_DISPATCH("new channel[%d] with codec type %d", pChannel->GetFd(), pChannel->GetCodecType());
        }
        return(pChannel);
    }
    else
    {
        pDispatcher->Logger(Logger::WARNING, __FILE__, __LINE__, __FUNCTION__,"fd %d is exist!", iFd);
        return(iter->second);
    }
}

template<typename T>
bool IO<T>::SplitIdentify(const std::string& strIdentify, std::string& strHost, int& iPort, int& iWorkerIndex, std::string& strError)
{
    size_t iPosIpPortSeparator = strIdentify.rfind(':');
    size_t iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort;
    if (iPosPortWorkerIndexSeparator != std::string::npos && iPosPortWorkerIndexSeparator > iPosIpPortSeparator)
    {
        strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
        iPort = atoi(strPort.c_str());
        if (iPort == 0)
        {
            return(false);
        }
        std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
        if (strWorkerIndex.size() > 0)
        {
            iWorkerIndex = atoi(strWorkerIndex.c_str());
            if (iWorkerIndex > 200)
            {
                strError = "worker index must smaller than 200";
                return(false);
            }
        }
    }
    else
    {
        strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
        iPort = atoi(strPort.c_str());
        if (iPort == 0)
        {
            return(false);
        }
    }
    return(true);
};

} /* namespace neb */

#endif /* SRC_IOS_IO_HPP_ */

