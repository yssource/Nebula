/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.cpp
 * @brief 
 * @author   bwar
 * @date:    Feb 27, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include <algorithm>
#include <sched.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include "util/process_helper.h"
#include "util/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include "Worker.hpp"
#include "LaborShared.hpp"
#include "ios/Dispatcher.hpp"
#include "ios/IO.hpp"
#include "actor/ActorBuilder.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"
#include "actor/session/sys_session/SessionDataReport.hpp"
#include "pb/report.pb.h"

namespace neb
{

Worker::Worker(const std::string& strWorkPath,
        int iWorkerIndex, Labor::LABOR_TYPE eLaborType)
    : Labor(eLaborType)
{
    m_stWorkerInfo.iWorkerIndex = iWorkerIndex;
    m_stWorkerInfo.iWorkerPid = getpid();
    m_stNodeInfo.strWorkPath = strWorkPath;
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);
}

Worker::~Worker()
{
    Destroy();
}

void Worker::Run()
{
    LOG4_TRACE("%s:%d", __FILE__, __LINE__);
    if (m_stNodeInfo.bThreadMode)
    {
        char szThreadName[64] = {0};
        snprintf(szThreadName, sizeof(szThreadName), "%s_W%d", m_oNodeConf("server_name").c_str(), m_stWorkerInfo.iWorkerIndex);
        pthread_setname_np(pthread_self(), szThreadName);
        LaborShared::Instance()->AddWorkerThreadId(gettid());
    }

    if (!CreateEvents())
    {
        Destroy();
        exit(-2);
    }

#ifndef __CYGWIN__
    bool bCpuAffinity = false;
    m_oNodeConf.Get("cpu_affinity", bCpuAffinity);
    if (bCpuAffinity && m_stNodeInfo.bThreadMode)
    {
        int iCpuNum = sysconf(_SC_NPROCESSORS_CONF);
        cpu_set_t stCpuMask;
        CPU_ZERO(&stCpuMask);
        CPU_SET(m_stWorkerInfo.iWorkerIndex % iCpuNum, &stCpuMask);
        if (pthread_setaffinity_np(pthread_self(),
                    sizeof(cpu_set_t), &stCpuMask) == -1)
        {
            LOG4_WARNING("pthread_setaffinity_np thread %d failed, errno %d", pthread_self(), errno);
        }
    }
#endif

    StartService();
    m_pDispatcher->EventRun();
}

void Worker::OnTerminated(struct ev_signal* watcher)
{
    int iSignum = watcher->signum;
    delete watcher;
    LOG4_FATAL("terminated by signal %d!", iSignum);
    m_pDispatcher->EvBreak();
    Destroy();
    exit(iSignum);
}

void Worker::DataReport()
{
    m_stWorkerInfo.uiConnection = m_pDispatcher->GetConnectionNum();
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    ReportRecord oWorkerStatus;
    std::string strWorkerStatus;
    oWorkerStatus.add_value(m_pActorBuilder->GetStepNum()); // load
    oWorkerStatus.add_value(m_stWorkerInfo.uiConnection);  // conection
    oWorkerStatus.add_value(m_stWorkerInfo.uiRecvNum);
    oWorkerStatus.add_value(m_stWorkerInfo.uiRecvByte);
    oWorkerStatus.add_value(m_stWorkerInfo.uiSendNum);
    oWorkerStatus.add_value(m_stWorkerInfo.uiSendByte);
    oWorkerStatus.SerializeToString(&strWorkerStatus);
    oMsgBody.set_data(strWorkerStatus);
    oMsgHead.set_cmd(CMD_REQ_UPDATE_WORKER_LOAD);
    uint32 uiManagerLaborId = LaborShared::Instance()->GetManagerLaborId();
    CodecNebulaInNode::Write(m_stWorkerInfo.iWorkerIndex, uiManagerLaborId, gc_uiCmdReq, 0, oMsgHead, oMsgBody);
    auto pSessionDataReport = std::dynamic_pointer_cast<SessionDataReport>(m_pActorBuilder->GetSession("neb::SessionDataReport"));
    if (pSessionDataReport != nullptr)
    {
        auto pReport = std::make_shared<neb::Report>();
        auto pRecord = pReport->add_records();
        pRecord->set_key("recv_num");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiRecvNum);
        pRecord = pReport->add_records();
        pRecord->set_key("recv_byte");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiRecvByte);
        pRecord = pReport->add_records();
        pRecord->set_key("send_num");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiSendNum);
        pRecord = pReport->add_records();
        pRecord->set_key("send_byte");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiSendByte);
        pRecord = pReport->add_records();
        pRecord->set_key("new_upstream_connection");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiNewUpStreamConnection);
        pRecord = pReport->add_records();
        pRecord->set_key("destroy_upstream_connection");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiDestroyUpStreamConnection);
        pRecord = pReport->add_records();
        pRecord->set_key("new_downstream_connection");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiNewDownStreamConnection);
        pRecord = pReport->add_records();
        pRecord->set_key("destroy_downstream_connection");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiDestroyDownStreamConnection);
        pRecord = pReport->add_records();
        pRecord->set_key("upstream_recv_num");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiUpStreamRecvNum);
        pRecord = pReport->add_records();
        pRecord->set_key("upstream_recv_byte");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiUpStreamRecvByte);
        pRecord = pReport->add_records();
        pRecord->set_key("upstream_send_num");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiUpStreamSendNum);
        pRecord = pReport->add_records();
        pRecord->set_key("upstream_send_byte");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiUpStreamSendByte);
        pRecord = pReport->add_records();
        pRecord->set_key("downstream_recv_num");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiDownStreamRecvNum);
        pRecord = pReport->add_records();
        pRecord->set_key("downstream_recv_byte");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiDownStreamRecvByte);
        pRecord = pReport->add_records();
        pRecord->set_key("downstream_send_num");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiDownStreamSendNum);
        pRecord = pReport->add_records();
        pRecord->set_key("downstream_send_byte");
        pRecord->set_item("nebula");
        pRecord->add_value(m_stWorkerInfo.uiDownStreamSendByte);
        pSessionDataReport->AddReport(pReport);
    }
    m_stWorkerInfo.ResetStat();
}

bool Worker::Init(CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_stWorkerInfo.iWorkerIndex);
    oJsonConf.Get("thread_mode", m_stNodeInfo.bThreadMode);
    if (m_stNodeInfo.bThreadMode)
    {
        oJsonConf.Get("async_logger", m_stNodeInfo.bAsyncLogger);
    }
    else
    {
        ngx_setproctitle(szProcessName);
    }
    oJsonConf.Get("io_timeout", m_stNodeInfo.dIoTimeout);
    if (!oJsonConf.Get("step_timeout", m_stNodeInfo.dStepTimeout))
    {
        m_stNodeInfo.dStepTimeout = 0.5;
    }
    m_stNodeInfo.uiWorkerNum = strtoul(oJsonConf("worker_num").c_str(), NULL, 10);
    oJsonConf.Get("data_report", m_stNodeInfo.dDataReportInterval);
    oJsonConf.Get("node_type", m_stNodeInfo.strNodeType);
    oJsonConf.Get("host", m_stNodeInfo.strHostForServer);
    oJsonConf.Get("port", m_stNodeInfo.iPortForServer);
    oJsonConf.Get("access_host", m_stNodeInfo.strHostForClient);
    oJsonConf.Get("access_port", m_stNodeInfo.iPortForClient);
    oJsonConf.Get("need_channel_verify", m_stNodeInfo.bChannelVerify);
    oJsonConf.Get("gateway", m_stNodeInfo.strGateway);
    oJsonConf.Get("gateway_port", m_stNodeInfo.iGatewayPort);
    m_oNodeConf = oJsonConf;
    m_oCustomConf = oJsonConf["custom"];
    std::ostringstream oss;
    oss << m_stNodeInfo.strHostForServer << ":" << m_stNodeInfo.iPortForServer << "." << m_stWorkerInfo.iWorkerIndex;
    m_stNodeInfo.strNodeIdentify = std::move(oss.str());

    if (oJsonConf("access_host").size() > 0 && oJsonConf("access_port").size() > 0)
    {
        m_stNodeInfo.bIsAccess = true;
        oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_stNodeInfo.dMsgStatInterval);
        oJsonConf["permission"]["uin_permit"].Get("permit_num", m_stNodeInfo.iMsgPermitNum);
    }
    if (!InitLogger(oJsonConf, szProcessName))
    {
        return(false);
    }

    bool bCpuAffinity = false;
    oJsonConf.Get("cpu_affinity", bCpuAffinity);
    if (bCpuAffinity && !m_stNodeInfo.bThreadMode)
    {
#ifndef __CYGWIN__
        /* get logical cpu number */
        int iCpuNum = sysconf(_SC_NPROCESSORS_CONF);;                               ///< cpu数量
        cpu_set_t stCpuMask;                                                        ///< cpu set
        CPU_ZERO(&stCpuMask);
        CPU_SET(m_stWorkerInfo.iWorkerIndex % iCpuNum, &stCpuMask);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &stCpuMask) == -1)
        {
            LOG4_WARNING("sched_setaffinity failed.");
        }
#endif
    }

    if (oJsonConf["with_ssl"]("config_path").length() > 0)
    {
#ifdef WITH_OPENSSL
        if (ERR_OK != SslContext::SslInit(m_pLogger))
        {
            LOG4_FATAL("SslInit() failed!");
            return(false);
        }
        if (ERR_OK != SslContext::SslClientCtxCreate(m_pLogger))
        {
            LOG4_FATAL("SslClientCtxCreate() failed!");
            return(false);
        }
        if (ERR_OK != SslContext::SslServerCtxCreate(m_pLogger))
        {
            LOG4_FATAL("SslServerCtxCreate() failed!");
            return(false);
        }
        std::string strCertFile = m_stNodeInfo.strWorkPath + "/" + oJsonConf["with_ssl"]("config_path") + "/" + oJsonConf["with_ssl"]("cert_file");
        std::string strKeyFile = m_stNodeInfo.strWorkPath + "/" + oJsonConf["with_ssl"]("config_path") + "/" + oJsonConf["with_ssl"]("key_file");
        if (ERR_OK != SslContext::SslServerCertificate(m_pLogger, strCertFile, strKeyFile))
        {
            LOG4_FATAL("SslServerCertificate() failed!");
            return(false);
        }
#endif
    }

    if (!InitDispatcher() || !InitActorBuilder())
    {
        return(false);
    }

    std::string strChainKey;
    while (oJsonConf["runtime"]["chains"].GetKey(strChainKey))
    {
        std::queue<std::vector<std::string> > queChainBlocks;
        for (int i = 0; i < oJsonConf["runtime"]["chains"][strChainKey].GetArraySize(); ++i)
        {
            std::vector<std::string> vecBlock;
            if (oJsonConf["runtime"]["chains"][strChainKey][i].IsArray())
            {
                for (int j = 0; j < oJsonConf["runtime"]["chains"][strChainKey][i].GetArraySize(); ++j)
                {
                    vecBlock.push_back(std::move(oJsonConf["runtime"]["chains"][strChainKey][i](j)));
                }
            }
            else
            {
                vecBlock.push_back(std::move(oJsonConf["runtime"]["chains"][strChainKey](i)));
            }
            queChainBlocks.push(std::move(vecBlock));
        }
        m_pActorBuilder->AddChainConf(strChainKey, std::move(queChainBlocks));
    }
    return(true);
}

bool Worker::InitLogger(const CJsonObject& oJsonConf, const std::string& strLogNameBase)
{
    if (nullptr != m_pLogger)  // 已经被初始化过，只修改日志级别
    {
        int32 iLogLevel = 0;
        int32 iNetLogLevel = 0;
        oJsonConf.Get("log_level", iLogLevel);
        oJsonConf.Get("net_log_level", iNetLogLevel);
        m_pLogger->SetLogLevel(iLogLevel);
        m_pLogger->SetNetLogLevel(iNetLogLevel);
        return(true);
    }
    else
    {
        int32 iMaxLogFileSize = 0;
        int32 iMaxLogFileNum = 0;
        int32 iMaxLogLineLen = 1024;
        int32 iLogLevel = 0;
        int32 iNetLogLevel = 0;
        bool bAlwaysFlushLog = true;
        std::string strLogname;
        std::string strLogPath;
        if (oJsonConf.Get("log_path", strLogPath))
        {
            if (strLogPath[0] == '/')
            {
                strLogname = strLogPath + std::string("/") + strLogNameBase + std::string(".log");
            }
            else
            {
                strLogname = m_stNodeInfo.strWorkPath + std::string("/")
                        + strLogPath + std::string("/") + strLogNameBase + std::string(".log");
            }
        }
        else
        {
            strLogname = m_stNodeInfo.strWorkPath + std::string("/logs/") + strLogNameBase + std::string(".log");
        }
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("log_max_line_len", iMaxLogLineLen);
        oJsonConf.Get("net_log_level", iNetLogLevel);
        oJsonConf.Get("log_level", iLogLevel);
        oJsonConf.Get("always_flush_log", bAlwaysFlushLog);
        if (m_stNodeInfo.bAsyncLogger)
        {
            m_pLogger = std::make_shared<NetLogger>(m_stWorkerInfo.iWorkerIndex, strLogname, iLogLevel, iMaxLogFileSize, iMaxLogFileNum, this);
        }
        else
        {
            bool bConsoleLog = false;
            oJsonConf.Get("console_log", bConsoleLog);
            m_pLogger = std::make_shared<NetLogger>(strLogname, iLogLevel, iMaxLogFileSize, iMaxLogFileNum, bAlwaysFlushLog, bConsoleLog, this);
        }
        m_pLogger->SetNetLogLevel(iNetLogLevel);
        LOG4_NOTICE("%s program begin...", getproctitle());
        return(true);
    }
}

bool Worker::InitDispatcher()
{
    if (NewDispatcher())
    {
        return(m_pDispatcher->Init());
    }
    return(false);
}

bool Worker::InitActorBuilder()
{
    if (NewActorBuilder())
    {
        return(m_pActorBuilder->Init(
                m_oNodeConf["load_config"]["worker"]["boot_load"],
                m_oNodeConf["load_config"]["worker"]["dynamic_loading"]));
    }
    return(false);
}

bool Worker::NewDispatcher()
{
    try
    {
        m_pDispatcher = new Dispatcher(this, m_pLogger);
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new Dispatcher error: %s", e.what());
        return(false);
    }
    LaborShared::Instance(m_stNodeInfo.uiWorkerNum + 2)->AddDispatcher(m_stWorkerInfo.iWorkerIndex, m_pDispatcher);
    return(true);
}

bool Worker::NewActorBuilder()
{
    try
    {
        m_pActorBuilder = new ActorBuilder(this, m_pLogger);
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new ActorBuilder error: %s", e.what());
        return(false);
    }
    return(true);
}

bool Worker::CreateEvents()
{
    if (!m_stNodeInfo.bThreadMode)
    {
        signal(SIGPIPE, SIG_IGN);
        // 注册信号事件
        ev_signal* signal_watcher = (ev_signal*)malloc(sizeof(ev_signal));
        signal_watcher->data = (void*)this;
        m_pDispatcher->AddEvent(signal_watcher, Dispatcher::SignalCallback, SIGINT);
    }
    AddPeriodicTaskEvent();
    return(true);
}

void Worker::StartService()
{
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    oMsgHead.set_cmd(CMD_REQ_START_SERVICE);
    oMsgBody.set_data(std::to_string(m_stWorkerInfo.iWorkerIndex));
    uint32 uiManagerLaborId = LaborShared::Instance()->GetManagerLaborId();
    int iResult = CodecNebulaInNode::Write(m_stWorkerInfo.iWorkerIndex, uiManagerLaborId, gc_uiCmdReq, GetSequence(), oMsgHead, oMsgBody);
    if (ERR_OK != iResult)
    {
        LOG4_ERROR("send to manager error %d", iResult);
    }
}

void Worker::Destroy()
{
    LOG4_TRACE(" ");

#ifdef WITH_OPENSSL
    SslContext::SslFree();
#endif
    if (m_pDispatcher != nullptr)
    {
        delete m_pDispatcher;
        m_pDispatcher = nullptr;
    }
    if (m_pActorBuilder != nullptr)
    {
        delete m_pActorBuilder;
        m_pActorBuilder = nullptr;
    }
    if (m_pErrBuff != nullptr)
    {
        free(m_pErrBuff);
        m_pErrBuff = nullptr;
    }
}

bool Worker::AddPeriodicTaskEvent()
{
    LOG4_TRACE(" ");
    ev_timer* timeout_watcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("malloc timeout_watcher error!");
        return(false);
    }
    timeout_watcher->data = (void*)this->GetDispatcher();
    m_pDispatcher->AddEvent(timeout_watcher, Dispatcher::PeriodicTaskCallback, NODE_BEAT);
    return(true);
}

time_t Worker::GetNowTime() const
{
    return(m_pDispatcher->GetNowTime());
}

long Worker::GetNowTimeMs() const
{
    return(m_pDispatcher->GetNowTimeMs());
}

const CJsonObject& Worker::GetNodeConf() const
{
    return(m_oNodeConf);
}

void Worker::SetNodeConf(const CJsonObject& oJsonConf)
{
    m_oNodeConf = oJsonConf;
}

const NodeInfo& Worker::GetNodeInfo() const
{
    return(m_stNodeInfo);
}

void Worker::SetNodeId(uint32 uiNodeId)
{
    m_stNodeInfo.uiNodeId = uiNodeId;
}

bool Worker::AddNetLogMsg(const MsgBody& oMsgBody)
{
    // 此函数不能写日志，不然可能会导致写日志函数与此函数无限递归
    m_pActorBuilder->AddNetLogMsg(oMsgBody);
    return(true);
}

const WorkerInfo& Worker::GetWorkerInfo() const
{
    return(m_stWorkerInfo);
}

const CJsonObject& Worker::GetCustomConf() const
{
    return(m_oCustomConf);
}

bool Worker::SetCustomConf(const CJsonObject& oJsonConf)
{
    m_oCustomConf = oJsonConf;
    return(m_oNodeConf.Replace("custom", oJsonConf));
}

bool Worker::WithSsl()
{
    if (m_oNodeConf["with_ssl"]("config_path").length() > 0)
    {
        return(true);
    }
    return(false);
}

} /* namespace neb */
