/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#include <boost/format.hpp>

#include "DpObjectMgr.h"
#include "InfnLogger.h"
#include "DataPlaneMgr.h"

using namespace std;

namespace DataPlane
{

bool   DpObjectMgr::mIsConfigFail = false;
uint32 DpObjectMgr::mConfigCancelCount = 0;

DpObjectMgr::DpObjectMgr(std::string name,
                         uint32 bitMaskConnId)
    : mIsConnectionUp(false)
    , mIsStartCalled(false)
    , mIsInitDone(false)
    , mIsNeedSubscribe(true)
    , mName(name)
    , mCurrConfigProc()
    , mLastConfigProc()
{

    for(uint32 i = 0; i < NUM_CPU_CONN_TYPES; i++)
    {
        if (bitMaskConnId & (1 << i))
        {
            mvDcoConnId.push_back((DcoCpuConnectionsType)i);
            mvIsConnectionUp.push_back(false);
        }
    }

    assert(mvDcoConnId.size() != 0);
}

void DpObjectMgr::updateDcoConnection(DcoCpuConnectionsType connId,
                                      bool isConnectUp)
{
    lock_guard<recursive_mutex> lock(mMutex);

    // Capture new connection state
    bool isConnectChange = false;
    bool tempIsConnectUp = true;
    for(uint32 id = 0; id < mvDcoConnId.size(); id++)
    {
        if (connId == mvDcoConnId[id])
        {
            if (isConnectUp != mvIsConnectionUp[id])
            {
                isConnectChange = true;
                mvIsConnectionUp[id] = isConnectUp;
            }
        }

        // AND all conn states to determine overall conn state for this mgr
        tempIsConnectUp &= mvIsConnectionUp[id];
    }

    if (isConnectChange)
    {
        // todo: remove these prints??
        INFN_LOG(SeverityLevel::info) << "Mgr " << mName << " Connection Change for connId: " << ToString(connId)
                                                         << " isConnectionUp: " << isConnectUp;

        mIsConnectionUp = tempIsConnectUp;

        INFN_LOG(SeverityLevel::info) << "Mgr " << mName << " Num gRPC client connections for this mgr: " << mvDcoConnId.size() <<
                ", Is Mgr Connection Up (Logical AND of all client connections for this mgr): " << mIsConnectionUp;

        connectionUpdate();

        if (mIsStartCalled)
        {
            if (isSubscribeConnUp(connId))
            {
                if ( mIsNeedSubscribe || (isSubEnabled() == false) )
                {
                    reSubscribe();

                    mIsNeedSubscribe = false;
                }
            }
            else
            {
                mIsNeedSubscribe = true;
            }
        }
        else if (mIsConnectionUp)
        {
            start();

            mIsStartCalled = true;

            mIsNeedSubscribe = false;
        }
    }

    if(connId == DcoCpuConnectionsType::DCC_NXP)
    {
        DcoSecProcConnectionUpdate(isConnectUp);
    }
}

void DpObjectMgr::syncConfig()
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspConfigHandler == nullptr)
    {
        INFN_LOG(SeverityLevel::info) << "mspConfigHandler is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::info) << "Syncing Config from Cache for " << mName;

    mspConfigHandler->syncConfig();

    updateInstalledConfig();

    if (mspStateCollector != nullptr)
    {
        mspStateCollector->setIsCollectionEn(true);
    }

    updateFaultCache();
}

void DpObjectMgr::updateInstalledConfig()
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspStateCollector == nullptr)
    {
        INFN_LOG(SeverityLevel::info) << "mspStateCollector is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::info) << mName;

    mspStateCollector->updateInstalledConfig();
}

void DpObjectMgr::clearConfigCache(std::ostream& out)
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspConfigHandler == nullptr)
    {
        // todo: make print debug??
        INFN_LOG(SeverityLevel::info) << "mspConfigHandler is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::info) << "Clearing Config Cache for " << mName;

    out << "**** Clearing Config Cache for " << mName << endl;

    mspConfigHandler->clearCache();
}

void DpObjectMgr::deleteConfigCacheObj(std::ostream& out, std::string configId)
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspConfigHandler == nullptr)
    {
        // todo: make print debug??
        INFN_LOG(SeverityLevel::info) << "mspConfigHandler is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::info) << "Deleting Config Cache for " << mName << " configId: " << configId;

    out << "**** Deleting Config Cache for " << mName << " configId: " << configId << endl;

    mspConfigHandler->clearCache(configId);
}

// Ensure the connection is ready and mgr is ready to receive and relay config
int DpObjectMgr::checkConfigReadiness(std::ostringstream& errLog)
{
    if (mIsStartCalled == false)
    {
        errLog << "Start is NOT called yet for " << mName;
        return -1;
    }

    if (!mIsConnectionUp)
    {
        errLog << "Connection is not up for " << mName;
        return -1;
    }

    if (DataPlaneMgrSingleton::Instance()->getSyncReady() == false)
    {
        errLog << "Sync is not ready for DataplaneMgr yet " << mName;
        return -1;
    }

    return 0;
}

void DpObjectMgr::handleConfig(google::protobuf::Message* objMsg,
                               const std::string configId,
                               ConfigType cfgType)
{
    uint32 currCancelCount;

    {
        lock_guard<recursive_mutex> lock(mMutex);

        currCancelCount = mConfigCancelCount;
        mCurrConfigProc.mConfigId   = configId;
        mCurrConfigProc.mConfigType = cfgType;
    }

    INFN_LOG(SeverityLevel::info) << mName << " " << toString(cfgType)
                                  << " configId " << mCurrConfigProc.mConfigId;

    ConfigStatus status = ConfigStatus::FAIL_CONNECT;
    const uint32 cMaxCount     = 900; //todo max count adjustment??
    const uint32 cMaxSendCount = 3;

    std::chrono::time_point<std::chrono::steady_clock> startTime
        = std::chrono::steady_clock::now();

    uint32 tryCount = 0;
    uint32 configRdyCount = 0;
    uint32 dynamicSendFailCount = 0;
    std::ostringstream errLog;
    while ((tryCount++ < cMaxCount) &&
           (mConfigCancelCount == currCancelCount))
    {
        {   // Lock Scope
            lock_guard<recursive_mutex> lock(mMutex);

            mCurrConfigProc.mConfigState = ConfigState::WAIT_FOR_SYNC;

            errLog.str("");
            errLog.clear();
            // CHECK/WAIT FOR CONFIG READY (SYNC READY, etc) ....
            if (checkConfigReadiness(errLog) == 0)
            {
                INFN_LOG(SeverityLevel::debug) << "Config Pre-reqs Met. Continuing with config processing...";

                mCurrConfigProc.mConfigState = ConfigState::SEND;

                // SEND/PROCESS CONFIG ....
                status = processConfig(objMsg, cfgType);
                mCurrConfigProc.mConfigSendCount++;
                if (status == ConfigStatus::SUCCESS)
                {
                    INFN_LOG(SeverityLevel::debug) << "SEND WORKED!!";
                    break;
                }
                else
                {
                    mCurrConfigProc.mConfigSendFailCount++;
                    if (status == ConfigStatus::FAIL_CONNECT)
                    {
                        if ((configRdyCount == 0) && (dynamicSendFailCount == 0))
                        {
                            // Info print only once per failure
                            INFN_LOG(SeverityLevel::info) << "Config processing (sending) failed retrying...";
                        }
                        else
                        {
                            // Debug print every failure after the 1st
                            INFN_LOG(SeverityLevel::debug) << "Config processing (sending) failed retrying...";
                        }

                        if (dynamicSendFailCount++ >= cMaxSendCount)
                        {
                            INFN_LOG(SeverityLevel::error) << "FAILURE!! Config processing max retries(" << cMaxSendCount <<
                                    ") exhausted while client connectivity remains up. Aborting!!";

                            break;
                        }
                    }
                    else
                    {
                        INFN_LOG(SeverityLevel::error) << "FAIL: Config processing encountered data failure. ABORTING!!";
                        break;
                    }
                }

                configRdyCount = 0;
            }
            else
            {
                dynamicSendFailCount = 0;

                if (configRdyCount++ == 0)  // todo: print logic improvement
                {
                    INFN_LOG(SeverityLevel::info) << "Waiting for Config Pre-reqs. ConfigReadyCount: " << configRdyCount
                                                  << " Overall TryCount: " << tryCount;
                }
            }
        }   // Lock Scope

        if (tryCount == cMaxCount)
        {
            INFN_LOG(SeverityLevel::error) << "FAIL: Config not ready. Giving Up!! Obj Type: " <<
                    mName << " configId: " << mCurrConfigProc.mConfigId;

            break;
        }

        if (dynamicSendFailCount)
        {
            // For config processing failures, sleep longer to give time for connection state updates
            sleep(2);
        }
        else
        {
            // For Config Readiness failures
            sleep(1);
        }

        updateConfigTime(startTime);

    }   // while(tryCount++ < cMaxCount)

    lock_guard<recursive_mutex> lock(mMutex);

    if (status == ConfigStatus::SUCCESS)
    {
        INFN_LOG(SeverityLevel::info) << "Config Success " << mName << " " << toString(cfgType)
                                  << " configId " << mCurrConfigProc.mConfigId;

        mCurrConfigProc.mConfigState = ConfigState::COMPLETION;
    }
    else
    {
        if (mConfigCancelCount != currCancelCount)
        {
            INFN_LOG(SeverityLevel::info) << "Config Process Cancelled. ABORTING config retries and considering as a failure!!";
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "Config FAILURE " <<
                        mName << " configId: " << mCurrConfigProc.mConfigId;
        }

        mCurrConfigProc.mConfigState = ConfigState::FAIL;
        mIsConfigFail = true;
    }

    ConfigStatus cfgStat = completeConfig(objMsg, cfgType, status, errLog);
    if (cfgStat != ConfigStatus::SUCCESS)
    {
        INFN_LOG(SeverityLevel::error) << "FAIL to complete config for configId: " << mCurrConfigProc.mConfigId;
    }

    updateConfigTime(startTime);

    mLastConfigProc = mCurrConfigProc;
    mCurrConfigProc.resetData();
}

void DpObjectMgr::updateConfigTime(std::chrono::time_point<std::chrono::steady_clock> startTime)
{
    lock_guard<recursive_mutex> lock(mMutex);

    auto nowTime = std::chrono::steady_clock::now();
    mCurrConfigProc.mConfigSecs =
        std::chrono::duration_cast<chrono::seconds>(nowTime - startTime);
}

void DpObjectMgr::dumpConfigCache(std::ostream& out)
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspConfigHandler == nullptr)
    {
        // todo: make print debug??
        INFN_LOG(SeverityLevel::info) << "mspConfigHandler is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::debug) << "Dumping Config Cache for " << mName;

    out << "**** Dumping Config Cache for " << mName << endl << endl;

    mspConfigHandler->dumpCache(out);
}

void DpObjectMgr::dumpConfigCache(std::ostream& out, std::string configId)
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspConfigHandler == nullptr)
    {
        INFN_LOG(SeverityLevel::debug) << "mspConfigHandler is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::debug) << "Dumping Config Cache for " << mName << " configId: " << configId;

    out << "**** Dumping Config Cache for " << mName << " configId: " << configId << endl << endl;

    mspConfigHandler->dumpCacheObj(out, configId);
}

void DpObjectMgr::listConfigCache(std::ostream& out)
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspConfigHandler == nullptr)
    {
        INFN_LOG(SeverityLevel::debug) << "mspConfigHandler is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::debug) << "Listing Config Cache for " << mName;

    out << "**** Listing Config Cache for " << mName << endl << endl;

    mspConfigHandler->listCache(out);
}

void DpObjectMgr::dumpStateCache(std::ostream& out, std::string stateId)
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspStateCollector == nullptr)
    {
        INFN_LOG(SeverityLevel::debug) << "mspStateCollector is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::debug) << "Dumping State Cache for " << mName << " stateId: " << stateId;

    out << "**** Dumping State Cache for " << mName << " stateId: " << stateId << endl << endl;

    mspStateCollector->dumpCacheState(stateId, out);
}

void DpObjectMgr::listStateCache(std::ostream& out)
{
    lock_guard<recursive_mutex> lock(mMutex);

    if (mspStateCollector == nullptr)
    {
        INFN_LOG(SeverityLevel::debug) << "mspStateCollector is null for Mgr " << mName;

        return;
    }

    INFN_LOG(SeverityLevel::debug) << "Listing State Cache for " << mName;

    out << "**** Listing State Cache for " << mName << endl << endl;

    mspStateCollector->listCacheState(out);
}

std::string toString(ConfigType type)
{
    switch(type)
    {
        case ConfigType::UNKNOWN:
            return "UNKNOWN";
            break;
        case ConfigType::CREATE:
            return "CREATE";
            break;
        case ConfigType::MODIFY:
            return "MODIFY";
            break;
        case ConfigType::DELETE:
            return "DELETE";
            break;
        default:
            return "UNKNOWN";

    }
}

std::string toString(ConfigState state)
{
    switch(state)
    {
        case ConfigState::UNKNOWN:
            return "UNKNOWN";
            break;
        case ConfigState::IDLE:
            return "IDLE";
            break;
        case ConfigState::WAIT_FOR_SYNC:
            return "WAIT_FOR_SYNC";
            break;
        case ConfigState::SEND:
            return "SEND";
            break;
        case ConfigState::COMPLETION:
            return "COMPLETION";
            break;
        case ConfigState::FAIL:
            return "FAIL";
            break;
        default:
            return "UNKNOWN";

    }
}

void ConfigProcessData::dump(std::ostream& out)
{
    out << boost::format("%-15s : %-15s") % "ConfigId"      % mConfigId              << std::endl;
    out << boost::format("%-15s : %-15s") % "ConfigType"    % toString(mConfigType)  << std::endl;
    out << boost::format("%-15s : %-15s") % "ConfigState"   % toString(mConfigState) << std::endl;
    out << boost::format("%-15s : %-15d") % "SendCount"     % mConfigSendCount       << std::endl;
    out << boost::format("%-15s : %-15d") % "SendFailCount" % mConfigSendFailCount   << std::endl;
    out << boost::format("%-15s : %-15d") % "ElapsedSecs"   % mConfigSecs.count()    << std::endl;
}

void DpObjectMgr::dumpCurrConfigProc(std::ostream& out)
{
    out << "********** Current Config Processing " << mName << "**********" << std::endl;

    mCurrConfigProc.dump(out);
}

void DpObjectMgr::dumpLastConfigProc(std::ostream& out)
{
    out << "********** Last Config Processing " << mName << "**********" << std::endl;

    mLastConfigProc.dump(out);
}

bool DpObjectMgr::isSubscribeConnUp(DcoCpuConnectionsType connId)
{
    return mIsConnectionUp;
}

} // namespace DataPlane

