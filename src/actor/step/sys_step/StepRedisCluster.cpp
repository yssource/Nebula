/*******************************************************************************
 * Project:  Nebula
 * @file     StepRedisCluster.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-12-12
 * @note
 * Modify history:
 ******************************************************************************/

#include "StepRedisCluster.hpp"
#include <sstream>
#include <algorithm>
#include "util/StringCoder.hpp"
#include "ios/Dispatcher.hpp"
#include "ios/IO.hpp"
#include "util/StringCoder.hpp"
#include "util/encrypt/crc16.h"

namespace neb
{

const uint16 StepRedisCluster::sc_unClusterSlots = 16384;

const std::unordered_set<std::string> StepRedisCluster::s_setSupportExtractCmd =
{
    "PING","ECHO","QUIT","SELECT",
    // strings
    "APPEND","BITCOUNT","BITFIELD","BITPOS","DECR","DECRBY","GET",
    "GETBIT","GETRANGE","GETSET","INCR","INCRBY","INCRBYFLOAT","MGET",
    "MSET","MSETNX","PSETEX","SET","SETBIT","SETEX","SETNX","SETRANGE",
    "STRLEN",
    // hashes
    "HDEL","HEXISTS","HGET","HGETALL","HINCRBY","HINCRBYFLOAT","HKEYS",
    "HLEN","HMGET","HMSET","HSET","HSETNX","HSTRLEN","HVALS","HSCAN",
    // lists
    "LINDEX","LINSERT","LLEN","LPOP","LPOS","LPUSH","LPUSHX","LRANGE",
    "LREM","LSET","LTRIM","RPOP","PROPLPUSH","RPUSH","RPUSHX",
    // sets
    "SADD","SCARD","SISMEMBER","SMISMEMBER","SMEMBERS","SPOP",
    "SRANDMEMBER","SREM","SSCAN",
    // sorted sets
    "ZADD","ZCARD","ZCOUNT","ZINCRBY","ZLEXCOUNT","ZPOPMAX","ZPOPMIN",
    "ZRANGE","ZRANGEBYLEX","ZREVRANGEBYLEX","ZRANGEBYSCORE","ZRANK",
    "ZREM","ZREMRANGEBYLEX","ZREMRANGEBYRANK","ZREMRANGEBYSCORE",
    "ZREVRANGE","ZREVRANGEBYSCORE","ZREVRANK","ZSCORE","ZMSCORE","ZSCAN",
    // keys
    "DEL","DUMP","EXISTS","EXPIRE","EXPIREAT","MOVE","PERSIST","PEXPIRE",
    "PEXPIREAT","PTTL","RANDOMKEY","RESTORE","SORT","TOUCH","TTL","TYPE",
    "UNLINK",
    // servers
    "ACL","COMMAND","CONFIG","DBSIZE","DEBUG","FLUSHALL","FLUSHDB","INFO",
    "LOLWUT","LASTSAVE","MEMORY"
};

const std::unordered_set<std::string> StepRedisCluster::s_setReadCmd =
{
    // strings
    "BITCOUNT","BITPOS","GET","GETBIT","GETRANGE","MGET","STRLEN",
    // hashes
    "HEXISTS","HGET","HGETALL","HKEYS","HLEN","HMGET","HSTRLEN","HVALS","HSCAN",
    // lists
    "LINDEX","LLEN","LRANGE",
    // sets
    "SCARD","SISMEMBER","SMISMEMBER","SMEMBERS","SRANDMEMBER","SSCAN",
    // sorted sets
    "ZCARD","ZCOUNT","ZLEXCOUNT","ZRANGE","ZRANGEBYLEX","ZRANGEBYSCORE","ZRANK",
    "ZREVRANGE","ZREVRANGEBYLEX","ZREVRANGEBYSCORE","ZREVRANK",
    "ZSCORE","ZMSCORE","ZSCAN",
    // keys
    "DUMP","EXISTS","PTTL","RANDOMKEY","TTL","TYPE"
};

const std::unordered_set<std::string> StepRedisCluster::s_setWriteCmd =
{
    // strings
    "APPEND","BITFIELD","DECR","DECRBY","GETSET","INCR","INCRBY","INCRBYFLOAT",
    "MSET","MSETNX","PSETEX","SET","SETBIT","SETEX","SETNX","SETRANGE",
    // hashes
    "HDEL","HINCRBY","HINCRBYFLOAT","HMSET","HSET","HSETNX",
    // lists
    "LINSERT","LPOS","LPUSH","LPUSHX","LREM","LSET","LTRIM","RPOP",
    "PROPLPUSH","RPUSH","RPUSHX",
    // sets
    "SADD","SPOP","SREM",
    // sorted sets
    "ZADD","ZINCRBY","ZPOPMAX","ZPOPMIN",
    "ZREM","ZREMRANGEBYLEX","ZREMRANGEBYRANK","ZREMRANGEBYSCORE",
    // keys
    "DEL","EXPIRE","EXPIREAT","MOVE","PERSIST","PEXPIRE","PEXPIREAT",
    "RESTORE","SORT","TOUCH","UNLINK"
};

const std::unordered_set<std::string> StepRedisCluster::s_setMultipleKeyCmd =
{
    "MGET","DEL","EXISTS","TOUCH","UNLINK"
};

const std::unordered_set<std::string> StepRedisCluster::s_setMultipleKeyValueCmd =
{
    "MSET","MSETNX"
};

StepRedisCluster::StepRedisCluster(
        const std::string& strIdentify,  bool bWithSsl, bool bPipeline, bool bEnableReadOnly)
    : RedisStep(7.0),
      m_bWithSsl(bWithSsl), m_bPipeline(bPipeline), m_bEnableReadOnly(bEnableReadOnly),
      m_uiAddressIndex(0), m_lLastCheckTime(0),
      m_strIdentify(strIdentify)
{
    Split(strIdentify, ",", m_vecAddress);
}

StepRedisCluster::~StepRedisCluster()
{
}

E_CMD_STATUS StepRedisCluster::Emit(int iErrno, const std::string& strErrMsg, void* data)
{
    SendCmdClusterSlots();
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepRedisCluster::Callback(
        std::shared_ptr<SocketChannel> pChannel, const RedisReply& oRedisReply)
{
    std::shared_ptr<RedisRequest> pRedisRequest = nullptr;
    uint32 uiRealStepSeq = 0;
    if (m_bPipeline)
    {
        auto step_iter = m_mapPipelineRequest.find(pChannel->GetIdentify());
        if (step_iter == m_mapPipelineRequest.end())
        {
            LOG4_ERROR("no \"%s\" found in m_mapPipelineRequest", pChannel->GetIdentify().c_str());
            return(CMD_STATUS_FAULT);
        }
        if (step_iter->second.empty())
        {
            LOG4_ERROR("channel \"%s\"'s m_mapPipelineRequest is empty", pChannel->GetIdentify().c_str());
            return(CMD_STATUS_FAULT);
        }
        pRedisRequest = step_iter->second.front();
        uiRealStepSeq = (uint32)pRedisRequest->integer();
        step_iter->second.pop();
    }
    else
    {
        uint32 uiChannelSeq = pChannel->GetSequence();
        auto iter = m_mapRequest.find(uiChannelSeq);
        if (iter == m_mapRequest.end())
        {
            LOG4_ERROR("no \"%u\" found in m_mapRequest", pChannel->GetSequence());
            return(CMD_STATUS_FAULT);
        }
        else
        {
            pRedisRequest = iter->second;
            uiRealStepSeq = (uint32)pRedisRequest->integer();
            m_mapRequest.erase(iter);
        }
    }
    if (uiRealStepSeq == GetSequence())
    {
        if (pRedisRequest->element(0).str() == "ASK")
        {
            CmdAskingCallback(pChannel, pChannel->GetIdentify(), oRedisReply);
        }
        else if (pRedisRequest->element(0).str() == "PING")
        {
            CmdPingCallback(pChannel, pChannel->GetIdentify(), oRedisReply);
        }
        else if (pRedisRequest->element(0).str() == "READONLY")
        {
            CmdReadOnlyCallback(pChannel, pChannel->GetIdentify(), oRedisReply);
        }
        else if (pRedisRequest->element(0).str() == "AUTH")
        {
            if (REDIS_REPLY_ERROR == oRedisReply.type())
            {
                CmdErrBack(pChannel, oRedisReply.type(), oRedisReply.str());
            }
            else
            {
                if (m_mapSlot2Node.size() == 0)
                {
                    SendCmdClusterSlots();
                }
            }
        }
        else
        {
            if (REDIS_REPLY_ERROR == oRedisReply.type())
            {
                std::vector<std::string> vecMsg;
                Split(oRedisReply.str(), " ", vecMsg);
                if (vecMsg.size() >= 3)
                {
                    if (vecMsg[0] == "NOAUTH")
                    {
                        Auth(pChannel->GetIdentify(), nullptr);
                        return(CMD_STATUS_RUNNING);
                    }
                }
            }
            else
            {
                CmdClusterSlotsCallback(oRedisReply);
                SendWaittingRequest();
            }
        }
    }
    else
    {
        auto num_iter = m_mapStepEmitNum.find(uiRealStepSeq);
        if (num_iter == m_mapStepEmitNum.end()) // 单key请求的响应
        {
            if (REDIS_REPLY_ERROR == oRedisReply.type())
            {
                std::vector<std::string> vecMsg;
                Split(oRedisReply.str(), " ", vecMsg);
                if (vecMsg.size() >= 3)
                {
                    if (vecMsg[0] == "MOVED")
                    {
                        SendTo(vecMsg[2], pRedisRequest);
                        SendCmdClusterSlots();
                        return(CMD_STATUS_RUNNING);
                    }
                    if (vecMsg[0] == "ASK")
                    {
                        SendCmdAsking(vecMsg[2]);
                        SendTo(vecMsg[2], pRedisRequest);
                        return(CMD_STATUS_RUNNING);
                    }
                    if (vecMsg[0] == "NOAUTH")
                    {
                        Auth(pChannel->GetIdentify(), pRedisRequest);
                        return(CMD_STATUS_RUNNING);
                    }
                }
            }
            IO<RedisStep>::OnResponse(GetLabor(this)->GetActorBuilder(), pChannel, uiRealStepSeq, oRedisReply);
        }
        else    // 多key请求的响应
        {
            if (REDIS_REPLY_ARRAY == oRedisReply.type())
            {
                auto reply_iter = m_mapReply.find(uiRealStepSeq);
                if (reply_iter == m_mapReply.end())
                {
                    LOG4_ERROR("no m_mapReply found for step %u", uiRealStepSeq);
                    return(CMD_STATUS_RUNNING);
                }
                std::vector<uint32> vecElementIndex;
                for (int i = 1; i < pRedisRequest->element_size(); ++i)  // element(0).str() is cmd
                {
                    if (pRedisRequest->element(i).integer() > 0 || i == 1)
                    {
                        vecElementIndex.push_back((uint32)pRedisRequest->element(i).integer());
                    }
                }
                for (uint32 j = 0; j < vecElementIndex.size(); ++j)
                {
                    if (vecElementIndex[j] < reply_iter->second.size())
                    {
                        if (reply_iter->second[vecElementIndex[j]] == nullptr)
                        {
                            reply_iter->second[vecElementIndex[j]] = new RedisReply();
                        }
                        reply_iter->second[vecElementIndex[j]]->CopyFrom(oRedisReply.element(j));
                    }
                    else
                    {
                        LOG4_ERROR("element index %u larger than reply size %u", vecElementIndex[j], reply_iter->second.size());
                    }
                }
                uint32 uiReqReplyOdd = (pRedisRequest->element_size() - 1) % oRedisReply.element_size();
                if (uiReqReplyOdd != 0)
                {
                    LOG4_ERROR("request and reply not match for %s", pRedisRequest->element(0).str().c_str());
                }
                num_iter->second--;
                if (num_iter->second == 0)
                {
                    RedisReply oFinalReply;
                    oFinalReply.set_type(oRedisReply.type());
                    for (size_t k = 0; k < reply_iter->second.size(); ++k)
                    {
                        oFinalReply.mutable_element()->AddAllocated(reply_iter->second[k]);
                        reply_iter->second[k] = nullptr;
                    }
                    IO<RedisStep>::OnResponse(GetLabor(this)->GetActorBuilder(), pChannel, uiRealStepSeq, oFinalReply);
                    m_mapStepEmitNum.erase(num_iter);
                    m_mapReply.erase(reply_iter);
                }
            }
            else
            {
                if (REDIS_REPLY_ERROR == oRedisReply.type())
                {
                    std::vector<std::string> vecMsg;
                    Split(oRedisReply.str(), " ", vecMsg);
                    if (vecMsg.size() >= 3)
                    {
                        if (vecMsg[0] == "MOVED")
                        {
                            SendTo(vecMsg[2], pRedisRequest);
                            SendCmdClusterSlots();
                            return(CMD_STATUS_RUNNING);
                        }
                        if (vecMsg[0] == "ASK")
                        {
                            SendCmdAsking(vecMsg[2]);
                            SendTo(vecMsg[2], pRedisRequest);
                            return(CMD_STATUS_RUNNING);
                        }
                        if (vecMsg[0] == "CROSSSLOT")
                        {
                            SendCmdClusterSlots();
                        }
                    }
                }
                auto reply_iter = m_mapReply.find(uiRealStepSeq);
                if (reply_iter == m_mapReply.end())
                {
                    LOG4_ERROR("no m_mapReply found for step %u", uiRealStepSeq);
                    return(CMD_STATUS_RUNNING);
                }
                // error or write cmd callback
                num_iter->second--;
                if (num_iter->second == 0)
                {
                    IO<RedisStep>::OnResponse(GetLabor(this)->GetActorBuilder(), pChannel, uiRealStepSeq, oRedisReply);
                    m_mapStepEmitNum.erase(num_iter);
                    m_mapReply.erase(reply_iter);
                }
            }
        }
    }
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepRedisCluster::Timeout()
{
    SendCmdClusterSlots();
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepRedisCluster::ErrBack(
        std::shared_ptr<SocketChannel> pChannel, int iErrno, const std::string& strErrMsg)
{
    LOG4_ERROR("step %u: channel %s error %d: %s", GetSequence(), pChannel->GetIdentify().c_str(), iErrno, strErrMsg.c_str());
    m_setFailedNode.insert(pChannel->GetIdentify());
    HealthCheck();
    CmdErrBack(pChannel, iErrno, strErrMsg);
    return(CMD_STATUS_RUNNING);
}

void StepRedisCluster::CmdErrBack(
        std::shared_ptr<SocketChannel> pChannel, int iErrno, const std::string& strErrMsg)
{
    if (m_bPipeline)
    {
        auto step_iter = m_mapPipelineRequest.find(pChannel->GetIdentify());
        if (step_iter == m_mapPipelineRequest.end())
        {
            LOG4_TRACE("no \"%s\" found in m_mapPipelineRequest", pChannel->GetIdentify().c_str());
        }
        else
        {
            uint32 uiErrNum = step_iter->second.size();
            for (uint32 i = 0; i < uiErrNum; ++i)
            {
                auto pRedisRequest = step_iter->second.front();
                uint32 uiRealStepSeq = (uint32)pRedisRequest->integer();
                step_iter->second.pop();
                CmdErrBack(pChannel, iErrno, strErrMsg, uiRealStepSeq, pRedisRequest);
            }
        }
    }
    else
    {
        uint32 uiChannelSeq = pChannel->GetSequence();
        auto iter = m_mapRequest.find(uiChannelSeq);
        if (iter == m_mapRequest.end())
        {
            LOG4_TRACE("no \"%u\" found in m_mapRequest", pChannel->GetSequence());
        }
        else
        {
            auto pRedisRequest = iter->second;
            auto uiRealStepSeq = (uint32)pRedisRequest->integer();
            m_mapRequest.erase(iter);
            CmdErrBack(pChannel, iErrno, strErrMsg, uiRealStepSeq, pRedisRequest);
        }
    }
}

void StepRedisCluster::CmdErrBack(
        std::shared_ptr<SocketChannel> pChannel, int iErrno, const std::string& strErrMsg,
        uint32 uiRealStepSeq, std::shared_ptr<RedisRequest> pRequest)
{
    if (uiRealStepSeq == GetSequence())
    {
        ; // SendCmdClusterSlots();
    }
    else
    {
        auto num_iter = m_mapStepEmitNum.find(uiRealStepSeq);
        if (num_iter == m_mapStepEmitNum.end()) // 单key请求的响应
        {
            GetLabor(this)->GetActorBuilder()->OnError(pChannel, uiRealStepSeq, iErrno, strErrMsg);
        }
        else
        {
            auto reply_iter = m_mapReply.find(uiRealStepSeq);
            if (reply_iter == m_mapReply.end())
            {
                LOG4_ERROR("no m_mapReply found for step %u", uiRealStepSeq);
                return;
            }
            std::vector<uint32> vecElementIndex;
            for (int i = 1; i < pRequest->element_size(); ++i)  // element(0).str() is cmd
            {
                if (pRequest->element(i).integer() > 0 || i == 1)
                {
                    vecElementIndex.push_back((uint32)pRequest->element(i).integer());
                }
            }
            RedisReply oRedisReply;
            oRedisReply.set_type(REDIS_REPLY_ERROR);
            oRedisReply.set_str(strErrMsg);
            for (uint32 j = 0; j < vecElementIndex.size(); ++j)
            {
                if (vecElementIndex[j] < reply_iter->second.size())
                {
                    if (reply_iter->second[vecElementIndex[j]] == nullptr)
                    {
                        reply_iter->second[vecElementIndex[j]] = new RedisReply();
                    }
                    reply_iter->second[vecElementIndex[j]]->CopyFrom(oRedisReply);
                }
                else
                {
                    LOG4_ERROR("element index %u larger than reply size %u", vecElementIndex[j], reply_iter->second.size());
                }
            }
            num_iter->second--;
            if (num_iter->second == 0)
            {
                RedisReply oFinalReply;
                oFinalReply.set_type(REDIS_REPLY_ARRAY);
                for (size_t k = 0; k < reply_iter->second.size(); ++k)
                {
                    oFinalReply.mutable_element()->AddAllocated(reply_iter->second[k]);
                    reply_iter->second[k] = nullptr;
                }
                IO<RedisStep>::OnResponse(GetLabor(this)->GetActorBuilder(), pChannel, uiRealStepSeq, oFinalReply);
                m_mapStepEmitNum.erase(num_iter);
                m_mapReply.erase(reply_iter);
            }
        }
   }
}

bool StepRedisCluster::SendTo(const std::string& strIdentify, const RedisMsg& oRedisMsg,
            bool bWithSsl, bool bPipeline, uint32 uiStepSeq)
{
    if (m_mapSlot2Node.size() > 0)
    {
        return(Dispatch(oRedisMsg, uiStepSeq));
    }
    else
    {
        ChannelOption stOption;
        stOption.bWithSsl = bWithSsl;
        stOption.bPipeline = bPipeline;
        stOption.dKeepAlive = GetTimeout();
        GetLabor(this)->GetDispatcher()->SetChannelOption(strIdentify, stOption);
        m_vecWaittingRequest.push_back(std::make_pair(uiStepSeq, oRedisMsg));
        return(SendCmdClusterSlots());
    }
}

bool StepRedisCluster::SendTo(const std::string& strIdentify, std::shared_ptr<RedisMsg> pRedisMsg)
{
    bool bResult = IO<CodecResp>::SendWithoutOption(this, strIdentify, (*pRedisMsg.get()));
    if (bResult)
    {
        auto pChannel = GetLastActivityChannel();
        auto dPenultimateActiveTime = pChannel->GetPenultimateActiveTime();
        auto dLastRecvTime = pChannel->GetLastRecvTime();
        auto dCheckTime = GetNowTime() - GetTimeout();
        if (dPenultimateActiveTime > dCheckTime
                && (dLastRecvTime > 0 && dLastRecvTime < dCheckTime)) // 有请求但长时间未收到响应包
        {
            LOG4_ERROR("%s channel[%d] may be a death connection, last_active_time = %lf,"
                    "last_recv_time = %lf, redis_cluster_check_time = %lf, close it",
                    strIdentify.c_str(), pChannel->GetFd(),
                    dPenultimateActiveTime, dLastRecvTime, GetTimeout());
            CmdErrBack(pChannel, ERR_DISCONNECT, "death connection closed");
            GetLabor(this)->GetDispatcher()->Disconnect(pChannel);
            return(false);
        }
        if (m_bPipeline)
        {
            auto iter = m_mapPipelineRequest.find(strIdentify);
            if (iter == m_mapPipelineRequest.end())
            {
                std::queue<std::shared_ptr<RedisMsg>> queStep;
                queStep.push(pRedisMsg);
                m_mapPipelineRequest.insert(std::make_pair(strIdentify, std::move(queStep)));
            }
            else
            {
                iter->second.push(pRedisMsg);
            }
        }
        else
        {
            uint32 uiChannelSeq = pChannel->GetSequence();
            auto iter = m_mapRequest.find(uiChannelSeq);
            if (iter == m_mapRequest.end())
            {
                m_mapRequest.insert(std::make_pair(uiChannelSeq, pRedisMsg));
            }
            else
            {
                LOG4_ERROR("not a pipeline channel.");
            }
        }
    }
    return(bResult);
}

bool StepRedisCluster::ExtractCmd(const RedisMsg& oRedisMsg,
        std::string& strCmd, std::vector<uint16>& vecSlotId,
        int& iReadOrWrite, int& iKeyInterval)
{
    auto CalcSlotId = [](const char* szKey, uint32 uiKeyLength)->uint16
    {
        uint32 s,e;
        for (s = 0; s < uiKeyLength; s++)
        {
            if (szKey[s] == '{')
            {
                break;
            }
        }
        if (s == uiKeyLength)
        {
            return(crc16(szKey, uiKeyLength) & 16383);
        }
        for (e = s + 1; e < uiKeyLength; e++)
        {
            if (szKey[e] == '}')
            {
                break;
            }
        }
        if (e == uiKeyLength || e == s + 1)
        {
            return(crc16(szKey, uiKeyLength) & 16383);
        }
        return(crc16(szKey + s + 1, e - s - 1) & 16383);
    };

    if (REDIS_REPLY_ARRAY == oRedisMsg.type())
    {
        if (oRedisMsg.element_size() == 0)
        {
            LOG4_ERROR("cmd element size is 0, invalid redis cmd: %s", oRedisMsg.DebugString().c_str());
            return(false);
        }
        if (REDIS_REPLY_STRING != oRedisMsg.element(0).type() || oRedisMsg.element(0).str().size() == 0)
        {
            LOG4_ERROR("cmd element be a REDIS_REPLY_STRING and key length can not be 0, "
                    "invalid redis cmd: %s", oRedisMsg.DebugString().c_str());
            return(false);
        }
        strCmd = oRedisMsg.element(0).str();
        std::transform(strCmd.begin(), strCmd.end(), strCmd.begin(), [](unsigned char c)->unsigned char{return std::toupper(c);});
        if (s_setSupportExtractCmd.find(strCmd) == s_setSupportExtractCmd.end())
        {
            LOG4_ERROR("cmd %s not supported by StepRedisCluster", strCmd.c_str());
            return(false);
        }
        if (s_setMultipleKeyCmd.find(strCmd) != s_setMultipleKeyCmd.end())
        {
            if (oRedisMsg.element_size() < 2)
            {
                LOG4_ERROR("param num fault of %s, invalid redis cmd: %s",
                        strCmd.c_str(), oRedisMsg.DebugString().c_str());
                return(false);
            }
            for (int i = 1; i < oRedisMsg.element_size(); ++i)
            {
                if (REDIS_REPLY_STRING != oRedisMsg.element(i).type() || oRedisMsg.element(i).str().size() == 0)
                {
                    LOG4_ERROR("cmd element be a REDIS_REPLY_STRING and key length can not be 0, "
                            "invalid redis cmd: %s", oRedisMsg.DebugString().c_str());
                    return(false);
                }
                vecSlotId.push_back(CalcSlotId(oRedisMsg.element(i).str().data(), oRedisMsg.element(i).str().length()));
            }
            iKeyInterval = 1;
            if (s_setWriteCmd.find(strCmd) == s_setWriteCmd.end())
            {
                iReadOrWrite = REDIS_CMD_READ;
            }
            else
            {
                iReadOrWrite = REDIS_CMD_WRITE;
            }
            return(true);
        }
        else if (s_setMultipleKeyValueCmd.find(strCmd) != s_setMultipleKeyValueCmd.end())
        {
            if (!(oRedisMsg.element_size() & 0x0001)) // (oRedisMsg.element_size() % 2 == 0)
            {
                LOG4_ERROR("param num must be even of %s, invalid redis cmd: %s",
                        strCmd.c_str(), oRedisMsg.DebugString().c_str());
                return(false);
            }
            for (int i = 1; i < oRedisMsg.element_size(); ++i)
            {
                if (i & 0x0001) // (i % 2 == 1)
                {
                    if (REDIS_REPLY_STRING != oRedisMsg.element(i).type() || oRedisMsg.element(i).str().size() == 0)
                    {
                        LOG4_ERROR("cmd element be a REDIS_REPLY_STRING and key length can not be 0, "
                                "invalid redis cmd: %s", oRedisMsg.DebugString().c_str());
                        return(false);
                    }
                    vecSlotId.push_back(CalcSlotId(oRedisMsg.element(i).str().data(), oRedisMsg.element(i).str().length()));
                }
            }
            iKeyInterval = 2;
            if (s_setWriteCmd.find(strCmd) == s_setWriteCmd.end())
            {
                iReadOrWrite = REDIS_CMD_READ;
            }
            else
            {
                iReadOrWrite = REDIS_CMD_WRITE;
            }
            LOG4_TRACE("oRedisMsg.element_size() = %d", oRedisMsg.element_size());
            return(true);
        }
        else
        {
            if (oRedisMsg.element_size() < 2)
            {
                vecSlotId.push_back(0);
            }
            else
            {
                if (REDIS_REPLY_STRING != oRedisMsg.element(1).type() || oRedisMsg.element(1).str().size() == 0)
                {
                    LOG4_ERROR("cmd element be a REDIS_REPLY_STRING, invalid redis cmd: %s", oRedisMsg.DebugString().c_str());
                    return(false);
                }
            }
            vecSlotId.push_back(CalcSlotId(oRedisMsg.element(1).str().data(), oRedisMsg.element(1).str().length()));
            if (s_setWriteCmd.find(strCmd) == s_setWriteCmd.end())
            {
                iReadOrWrite = REDIS_CMD_WRITE;
            }
            else
            {
                iReadOrWrite = REDIS_CMD_READ;
            }
            return(true);
        }
    }
    else
    {
        LOG4_ERROR("cmd must be a REDIS_REPLY_ARRAY, invalid redis cmd: %s", oRedisMsg.DebugString().c_str());
        return(false);
    }
}

bool StepRedisCluster::GetRedisNode(int iSlotId, int iReadOrWrite, std::string& strNodeIdentify, bool& bIsMaster) const
{
    auto slot_iter = m_mapSlot2Node.find(iSlotId);
    if (slot_iter == m_mapSlot2Node.end())
    {
        LOG4_ERROR("no redis node found for slot %d", iSlotId);
        return(false);
    }
    if ((REDIS_CMD_WRITE == iReadOrWrite)
            || (slot_iter->second->setFllower.size() == 0)
            || (!m_bEnableReadOnly))
    {
        strNodeIdentify = slot_iter->second->strMaster;
        bIsMaster = true;
    }
    else
    {
        for (uint32 i = 0; i < slot_iter->second->setFllower.size(); ++i)
        {
            slot_iter->second->iterFllower++;
            if (slot_iter->second->iterFllower == slot_iter->second->setFllower.end())
            {
                slot_iter->second->iterFllower = slot_iter->second->setFllower.begin();
            }
            strNodeIdentify = *(slot_iter->second->iterFllower);
            bIsMaster = false;
            auto node_iter = m_setFailedNode.find(strNodeIdentify);
            if (node_iter == m_setFailedNode.end())
            {
                return(true);
            }
        }
        strNodeIdentify = slot_iter->second->strMaster;
        bIsMaster = true;
    }
    return(true);
}

bool StepRedisCluster::NeedSetReadOnly(const std::string& strNode) const
{
    auto iter = m_mapPipelineRequest.find(strNode);
    if (iter == m_mapPipelineRequest.end() || iter->second.empty())
    {
        return(true);
    }
    return(false);
}

bool StepRedisCluster::SendCmdClusterSlots()
{
    SetCmd("CLUSTER");
    Append("SLOTS");
    if (m_vecAddress.size() == 0)
    {
        LOG4_ERROR("no redis node address!");
        return(false);
    }
    auto pRedisRequest = MutableRedisRequest();
    pRedisRequest->set_integer(GetSequence());    // 借用integer暂存seq
    if (m_uiAddressIndex >= m_vecAddress.size())
    {
        m_uiAddressIndex = 0;
    }
    bool bResult = SendTo(m_vecAddress[m_uiAddressIndex++], pRedisRequest);
    ++m_uiAddressIndex;
    return(bResult);
}

bool StepRedisCluster::CmdClusterSlotsCallback(const RedisReply& oRedisReply)
{
    if (REDIS_REPLY_ARRAY == oRedisReply.type())
    {
        m_setAllNode.clear();
        int iFromSlot = 0;
        int iToSlot = 0;
        for (int i = 0; i < oRedisReply.element_size(); ++i)
        {
            if (REDIS_REPLY_ARRAY == oRedisReply.element(i).type())
            {
                if (oRedisReply.element(i).element_size() < 3)
                {
                    LOG4_ERROR("invalid element num %d in element(%d)", oRedisReply.element(i).element_size(), i);
                    continue;
                }
                if (REDIS_REPLY_INTEGER != oRedisReply.element(i).element(0).type()
                        || REDIS_REPLY_INTEGER != oRedisReply.element(i).element(1).type())
                {
                    LOG4_ERROR("invalid element type %d or %d in element(%d)",
                            oRedisReply.element(i).element(0).type(),
                            oRedisReply.element(i).element(1).type(), i);
                    continue;
                }
                iFromSlot = oRedisReply.element(i).element(0).integer();
                iToSlot = oRedisReply.element(i).element(1).integer();
                auto pRedisNode = std::make_shared<RedisNode>();
                for (int j = 2; j < oRedisReply.element(i).element_size(); ++j)
                {
                    if (REDIS_REPLY_STRING != oRedisReply.element(i).element(j).element(0).type()
                        || REDIS_REPLY_INTEGER != oRedisReply.element(i).element(j).element(1).type())
                    {
                        LOG4_ERROR("invalid element type %d or %d in element(%d).element(%d)",
                                oRedisReply.element(i).element(j).element(0).type(),
                                oRedisReply.element(i).element(j).element(1).type(), i, j);
                        break;
                    }
                    std::ostringstream oss;
                    oss << oRedisReply.element(i).element(j).element(0).str()
                            << ":"
                            << oRedisReply.element(i).element(j).element(1).integer();
                    if (j == 2)
                    {
                        pRedisNode->strMaster = oss.str();
                    }
                    else
                    {
                        pRedisNode->setFllower.insert(oss.str());
                    }
                    m_setAllNode.insert(oss.str());
                }
                pRedisNode->iterFllower = pRedisNode->setFllower.begin();
                for (int iSlotId = iFromSlot; iSlotId <= iToSlot; ++iSlotId)
                {
                    auto it = m_mapSlot2Node.find(iSlotId);
                    if (it == m_mapSlot2Node.end())
                    {
                        m_mapSlot2Node.insert(std::make_pair(iSlotId, pRedisNode));
                    }
                    else
                    {
                        it->second = pRedisNode;
                    }
                }
            }
            else
            {
                LOG4_ERROR("redis reply type %d is invalid for CLUSTER SLOTS in element(%d)", i);
            }
        }
        return(true);
    }
    LOG4_ERROR("redis reply type %d is invalid for CLUSTER SLOTS");
    return(false);
}

bool StepRedisCluster::SendCmdAsking(const std::string& strIdentify)
{
    SetCmd("ASKING");
    auto pRedisRequest = MutableRedisRequest();
    pRedisRequest->set_integer(GetSequence());    // 借用integer暂存seq
    return(SendTo(strIdentify, pRedisRequest));
}

bool StepRedisCluster::SendCmdPing(const std::string& strIdentify)
{
    SetCmd("PING");
    auto pRedisRequest = MutableRedisRequest();
    pRedisRequest->set_integer(GetSequence());    // 借用integer暂存seq
    return(SendTo(strIdentify, pRedisRequest));
}

void StepRedisCluster::CmdAskingCallback(std::shared_ptr<SocketChannel> pChannel,
        const std::string& strIdentify, const RedisReply& oRedisReply)
{
    if (REDIS_REPLY_STATUS == oRedisReply.type())
    {
        if ("OK" == oRedisReply.str())
        {
            ;
        }
    }
    else
    {
        LOG4_ERROR("%s unexpected asking errmsg: %s", strIdentify.c_str(), oRedisReply.str().c_str());
    }
    LOG4_ERROR("%s unexpected error type %d: %s", strIdentify.c_str(), oRedisReply.type(), oRedisReply.str().c_str());
}

void StepRedisCluster::CmdPingCallback(std::shared_ptr<SocketChannel> pChannel,
        const std::string& strIdentify, const RedisReply& oRedisReply)
{
    if (REDIS_REPLY_STATUS == oRedisReply.type() || REDIS_REPLY_STRING == oRedisReply.type())
    {
        auto iter = m_setFailedNode.find(strIdentify);
        if (iter != m_setFailedNode.end())
        {
            m_setFailedNode.erase(iter);
        }
    }
}

bool StepRedisCluster::SendCmdReadOnly(const std::string& strIdentify)
{
    SetCmd("READONLY");
    auto pRedisRequest = MutableRedisRequest();
    pRedisRequest->set_integer(GetSequence());    // 借用integer暂存seq
    return(SendTo(strIdentify, pRedisRequest));
}

void StepRedisCluster::CmdReadOnlyCallback(std::shared_ptr<SocketChannel> pChannel,
        const std::string& strIdentify, const RedisReply& oRedisReply)
{
    if (REDIS_REPLY_STATUS == oRedisReply.type())
    {
        if ("OK" == oRedisReply.str())
        {
            ;
        }
        else
        {
            LOG4_ERROR("set %s read only failed: %s", pChannel->GetIdentify().c_str(), oRedisReply.str().c_str());
        }
    }
    else if (REDIS_REPLY_ERROR == oRedisReply.type())
    {
        LOG4_ERROR("set %s read only failed: %s", pChannel->GetIdentify().c_str(), oRedisReply.str().c_str());
    }
    else
    {
        LOG4_ERROR("set %s read only failed: unexpected reply type %d", pChannel->GetIdentify().c_str(), oRedisReply.type());
    }
}

bool StepRedisCluster::Dispatch(const RedisMsg& oRedisMsg, uint32 uiStepSeq)
{
    std::string strCmd;
    std::vector<uint16> vecSlotId;
    int iKeyInterval = 0;
    int iReadOrWrite = 0;
    if (!ExtractCmd(oRedisMsg, strCmd, vecSlotId, iReadOrWrite, iKeyInterval))
    {
        return(false);
    }
    bool bIsMasterNode = false;
    std::string strRedisNode;
    if (vecSlotId.size() == 1)
    {
        if (!GetRedisNode(vecSlotId[0], iReadOrWrite, strRedisNode, bIsMasterNode))
        {
            return(false);
        }
        auto pRedisRequest = std::make_shared<RedisMsg>(oRedisMsg);
        pRedisRequest->set_integer(uiStepSeq);    // 借用integer暂存seq
        if (!bIsMasterNode && NeedSetReadOnly(strRedisNode))
        {
            SendCmdReadOnly(strRedisNode);
        }
        return(SendTo(strRedisNode, pRedisRequest));
    }
    else if (vecSlotId.size() > 1)
    {
        if ((int)vecSlotId.size() * iKeyInterval >= oRedisMsg.element_size())
        {
            LOG4_ERROR("element size error.");
            return(false);
        }
        std::unordered_map<int, std::shared_ptr<RedisMsg>> mapRedisRequest;
        for (uint32 i = 0; i < vecSlotId.size(); ++i)
        {
            auto req_iter = mapRedisRequest.find(vecSlotId[i]);
            if (req_iter == mapRedisRequest.end())
            {
                auto pSubRequest = std::make_shared<RedisMsg>();
                pSubRequest->set_type(oRedisMsg.type());
                auto pElement = pSubRequest->add_element();
                pElement->CopyFrom(oRedisMsg.element(0));   // cmd
                pElement = pSubRequest->add_element();
                pElement->CopyFrom(oRedisMsg.element(i * iKeyInterval + 1)); // key
                pElement->set_integer(i);    // 借用integer暂存key的序号
                for (int j = 2; j <= iKeyInterval; ++j)
                {
                    pElement = pSubRequest->add_element();
                    pElement->CopyFrom(oRedisMsg.element(i * iKeyInterval + j));  // value
                }
                pSubRequest->set_integer(uiStepSeq);    // 借用integer暂存seq
                mapRedisRequest.insert(std::make_pair(vecSlotId[i], pSubRequest));
            }
            else
            {
                auto pElement = req_iter->second->add_element();
                pElement->CopyFrom(oRedisMsg.element(i * iKeyInterval + 1));  // key
                pElement->set_integer(i);    // 借用integer暂存key的序号
                for (int j = 2; j <= iKeyInterval; ++j)
                {
                    pElement = req_iter->second->add_element();
                    pElement->CopyFrom(oRedisMsg.element(i * iKeyInterval + j));   // value
                }
            }
        }
        for (auto req_iter = mapRedisRequest.begin(); req_iter != mapRedisRequest.end(); ++req_iter)
        {
            if (!GetRedisNode(req_iter->first, iReadOrWrite, strRedisNode, bIsMasterNode))
            {
                return(false);
            }
            if (!bIsMasterNode && NeedSetReadOnly(strRedisNode))
            {
                SendCmdReadOnly(strRedisNode);
            }
            SendTo(strRedisNode, req_iter->second);
        }
        m_mapStepEmitNum[uiStepSeq] = mapRedisRequest.size();
        std::vector<RedisReply*> vecReply;
        auto iter_bool = m_mapReply.insert(std::make_pair(uiStepSeq, vecReply));
        if (iter_bool.second == true)
        {
            iter_bool.first->second.resize(vecSlotId.size(), nullptr);
        }
        return(iter_bool.second);
    }
    else
    {
        LOG4_ERROR("no hash key for %s", strCmd.c_str());
        return(false);
    }
}

void StepRedisCluster::SendWaittingRequest()
{
    for (size_t i = 0; i < m_vecWaittingRequest.size(); ++i)
    {
        Dispatch(m_vecWaittingRequest[i].second, m_vecWaittingRequest[i].first);
    }
    m_vecWaittingRequest.clear();
}

void StepRedisCluster::RegisterStep(uint32 uiStepSeq)
{
    auto iter = m_mapTimeoutStep.find(uiStepSeq);
    if (iter == m_mapTimeoutStep.end())
    {
        std::vector<uint32> vecStep;
        vecStep.push_back(uiStepSeq);
        m_mapTimeoutStep.insert(std::make_pair(GetNowTime(), std::move(vecStep)));
    }
    else
    {
        iter->second.push_back(uiStepSeq);
    }
}

bool StepRedisCluster::Auth(const std::string& strIdentify, std::shared_ptr<RedisMsg> pRedisMsg)
{
    std::string strAuth;
    std::string strPassword;
    GetLabor(this)->GetDispatcher()->GetAuth(m_strIdentify, strAuth, strPassword);
    SetCmd("AUTH");
    Append(strPassword);
    auto pRedisRequest = MutableRedisRequest();
    pRedisRequest->set_integer(GetSequence());
    if (pRedisMsg == nullptr)
    {
        return(SendTo(strIdentify, pRedisRequest));
    }
    else
    {
        SendTo(strIdentify, pRedisRequest);
        return(SendTo(strIdentify, pRedisMsg));
    }
}

void StepRedisCluster::HealthCheck()
{
    time_t lNowTime = GetNowTime();
    if (lNowTime - m_lLastCheckTime < 2)
    {
        return;
    }
    m_lLastCheckTime = lNowTime;

    if (m_setFailedNode.size() > 0)
    {
        SendCmdClusterSlots();
    }
    for (auto iter = m_setFailedNode.begin(); iter != m_setFailedNode.end(); )
    {
        if (m_setAllNode.find(*iter) == m_setAllNode.end())
        {
            m_setFailedNode.erase(iter);
            iter = m_setFailedNode.begin();
        }
        else
        {
            SendCmdPing(*iter);
            ++iter;
        }
    }
    for (auto timeout_iter = m_mapTimeoutStep.begin(); timeout_iter != m_mapTimeoutStep.end(); )
    {
        if (GetNowTime() - timeout_iter->first >= GetTimeout())
        {
            for (uint32 i = 0; i < timeout_iter->second.size(); ++i)
            {
                auto reply_iter = m_mapReply.find(timeout_iter->second[i]);
                if (reply_iter != m_mapReply.end())
                {
                    m_mapReply.erase(reply_iter);
                }
                auto num_iter = m_mapStepEmitNum.find(timeout_iter->second[i]);
                if (num_iter != m_mapStepEmitNum.end())
                {
                    m_mapStepEmitNum.erase(num_iter);
                }
            }
            timeout_iter->second.clear();
            m_mapTimeoutStep.erase(timeout_iter);
            timeout_iter = m_mapTimeoutStep.begin();
        }
        else
        {
            break;
        }
    }
}

} /* namespace neb */

