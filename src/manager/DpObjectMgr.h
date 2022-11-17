/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_OBJECT_MGR_H
#define DP_OBJECT_MGR_H

#include <google/protobuf/message.h>
#include <chrono>

#include "DcoConnectHandler.h"
#include "DpConfigHandler.h"
#include "DpStateCollector.h"
#include "types.h"

namespace DataPlane
{

enum class ConfigType
{
    UNKNOWN = -1,
    CREATE = 0,
    MODIFY,
    DELETE
};

enum class ConfigState
{
    UNKNOWN = -1,
    IDLE = 0,
    WAIT_FOR_SYNC,
    SEND,
    COMPLETION,
    FAIL
};

extern std::string toString(ConfigType type);
extern std::string toString(ConfigState state);

struct ConfigProcessData
{
    ConfigState          mConfigState;
    std::string          mConfigId;
    ConfigType           mConfigType;
    uint32               mConfigSendCount;
    uint32               mConfigSendFailCount;
    std::chrono::seconds mConfigSecs;

    ConfigProcessData()
        : mConfigState(ConfigState::IDLE)
        , mConfigId("")
        , mConfigType(ConfigType::UNKNOWN)
        , mConfigSendCount(0)
        , mConfigSendFailCount(0)
        , mConfigSecs(0)        {}

    void resetData()
    {
        mConfigState = ConfigState::IDLE;
        mConfigId = "";
        mConfigType = ConfigType::UNKNOWN;
        mConfigSendCount = 0;
        mConfigSendFailCount = 0;
        mConfigSecs = std::chrono::seconds::zero();
    }

    void dump(std::ostream& out);
};

class DpObjectMgr
{
public:
    DpObjectMgr(std::string name,
               uint32 bitMaskConnId);
    virtual ~DpObjectMgr() {}

    virtual void initialize() = 0;

    virtual bool checkAndCreate(google::protobuf::Message* objMsg) = 0;
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg) = 0;
    virtual bool checkAndDelete(google::protobuf::Message* objMsg) = 0;
    virtual bool checkOnResync (google::protobuf::Message* objMsg) = 0;

    virtual void updateDcoConnection(DcoCpuConnectionsType connId,
                                     bool isConnectUp);

    std::vector<DcoCpuConnectionsType> getConnectId() { return mvDcoConnId; }
    bool getIsConnectionUp() { return mIsConnectionUp; }

    virtual void syncConfig();

    virtual void updateInstalledConfig();

    void clearConfigCache(std::ostream& out);
    void deleteConfigCacheObj(std::ostream& out, std::string configId);

    virtual int checkConfigReadiness(std::ostringstream& errLog);

    ConfigState getCurrConfigState() { return mCurrConfigProc.mConfigState; }

    void dumpConfigCache(std::ostream& out);
    void dumpConfigCache(std::ostream& out, std::string configId);
    void listConfigCache(std::ostream& out);

    void dumpStateCache(std::ostream& out, std::string stateId);
    void listStateCache(std::ostream& out);

    void dumpCurrConfigProc(std::ostream& out);
    void dumpLastConfigProc(std::ostream& out);

    static bool   isConfigFail()         { return mIsConfigFail; }
    static uint32 getConfigCancelCount() { return mConfigCancelCount; }
    static void   cancelConfigProc()     { mConfigCancelCount++; }

    virtual bool isSubscribeConnUp(DcoCpuConnectionsType connId);

    virtual bool isSubEnabled() { return true; }

    virtual void updateFaultCache() {};

protected:

    virtual void start() = 0;

    // This is trigger for derived class if needed on connection changes
    virtual void connectionUpdate() {}

    virtual void reSubscribe() {}

    virtual ConfigStatus processConfig(google::protobuf::Message* objMsg, ConfigType cfgType) { return ConfigStatus::SUCCESS; }

    virtual ConfigStatus completeConfig(google::protobuf::Message* objMsg,
                                        ConfigType cfgType,
                                        ConfigStatus status,
                                        std::ostringstream& log)     { return ConfigStatus::SUCCESS; }

    virtual void handleConfig(google::protobuf::Message* objMsg,
                              const std::string configId,
                              ConfigType cfgType);

    void updateConfigTime(std::chrono::time_point<std::chrono::steady_clock> startTime);

    /*
    * @todo: Remove this function, add code for manager to handle DCOSecProc and DCO zynq connections together
    * This is trigger for derived class if it needs to see the DCOSecProcConnectionUpdate
    * @param boolean to indicate the status of connection up/down
    */
    virtual void DcoSecProcConnectionUpdate(bool isConnectUp) {};

    // Dco Connection Info
    //   note: if we have non-Dco Object Mgrs in future,
    //     then we should abstract this dco specific data to derived class

    std::vector<DcoCpuConnectionsType> mvDcoConnId;
    std::vector<bool>                  mvIsConnectionUp;
    bool                  mIsConnectionUp;
    bool                  mIsStartCalled;
    bool                  mIsInitDone;
    bool                  mIsNeedSubscribe;
    std::string           mName;

    std::recursive_mutex mMutex;
    std::shared_ptr<DpConfigHandler> mspConfigHandler;
    std::shared_ptr<DpStateCollector> mspStateCollector;

    // Current Config Handling
    ConfigProcessData mCurrConfigProc;
    ConfigProcessData mLastConfigProc;
    static bool   mIsConfigFail;
    static uint32 mConfigCancelCount;

};

} // namespace DataPlane

#endif /* DP_OBJECT_MGR_H */
