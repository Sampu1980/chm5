/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DATA_PLANE_MGR_H
#define DATA_PLANE_MGR_H

#include "chm6/redis_adapter/callback_handler.h"
#include <google/protobuf/message.h>
#include <list>

#include "CardAdapter.h"
#include "GearBoxAdapter.h"
#include "GearBoxAdapterSim.h"
#include "GearBoxAdIf.h"
#include "SerdesTuner.h"

#include "FpgaRegIf.h"
#include "RegIfFactory.h"

#include "DpObjectMgr.h"
#include "DpProtoDefs.h"

#include <SingletonService.h>

namespace DataPlane {

static const uint SERDES_SCRATCHPAD_MAGIC_NUMBER = 0x5AFE;

enum ObjectMgrs
{
    OBJ_MGR_CARD = 0, // note: Card must be first index
    OBJ_MGR_GIGE,     // note: Order is based on config sync ordering
    OBJ_MGR_CARRIER,
    OBJ_MGR_OTU,
    OBJ_MGR_ODU,
    OBJ_MGR_XCON,
    OBJ_MGR_GCC,
    OBJ_MGR_PEER_DISC,
    OBJ_MGR_KEY_MGMT,
    NUM_OBJ_MGRS
};

enum BootReason
{
    BOOT_REASON_UNSPECIFIED = 0,
    BOOT_REASON_COLD        = 1,
    BOOT_REASON_WARM        = 2
};

enum DpSyncState
{
    DP_SYNC_STATE_UNSPECIFIED = 0,
    DP_SYNC_STATE_START       = 1,
    DP_SYNC_STATE_END         = 2
};

class DataPlaneMgr : public ICallbackHandler
{
public:
    DataPlaneMgr();
    virtual ~DataPlaneMgr();

    void initRedisCallbacks();

    // Callback functions to handle config messages
    void onCreate(google::protobuf::Message* objMsg);
    void onModify(google::protobuf::Message* objMsg);
    void onDelete(google::protobuf::Message* objMsg);
    void onResync(google::protobuf::Message* objMsg);

    void completeInit();

    bool getSyncReady();

    std::string getChassisIdStr() { return to_string(mChassisId); }
    bool getIsBrdInitDone()    { return mIsBrdInitDone;   }
    bool getIsBrdInitFail()    { return mIsBrdInitFail;   }
    bool getIsOnResyncDone()   { return mIsOnResyncDone;  }
    bool getIsInitDone()       { return mIsInitDone;      }
    std::string getPwrModeStr() { return chm6_common::PowerMode_Name(mPowerMode); }

    bool getIsForceSyncReady() { return mIsForceSyncReady;}
    void setIsForceSyncReady(bool val) { mIsForceSyncReady = val; }

    bool getIsAgentSyncConfigDone() { return mIsAgentSyncConfigDone; }
    void setIsAgentSyncConfigDone(bool val) { mIsAgentSyncConfigDone = val; }

    // Redis Communication
    void DpMgrRedisObjectCreate(google::protobuf::Message& objMsg);
    void DpMgrRedisObjectUpdate(google::protobuf::Message& objMsg);
    void DpMgrRedisObjectDelete(std::vector<std::reference_wrapper<google::protobuf::Message>>& vObjMsg);
    void DpMgrRedisObjectStream(google::protobuf::Message& objMsg);

    void populateInstalledConfig();

    // CLI Cmds ...
    void listObjMgrs(std::ostream& out);

    void clearConfigCache(std::ostream& out, uint32 mgrIdx);
    void clearConfigCache(std::ostream& out, uint32 mgrIdx, std::string configId);
    void listConfigCache (std::ostream& out, uint32 mgrIdx);
    void dumpConfigCache (std::ostream& out, uint32 mgrIdx);
    void dumpConfigCache (std::ostream& out, uint32 mgrIdx, std::string configId);

    void syncConfigCache (std::ostream& out, uint32 mgrIdx);

    void listStateCache (std::ostream& out, uint32 mgrIdx);
    void dumpStateCache (std::ostream& out, uint32 mgrIdx, std::string stateId);

    void dumpConfigProcess    (std::ostream& out, uint32 mgrIdx);
    void dumpConfigProcessLast(std::ostream& out, uint32 mgrIdx);

    // BootReason
    bool isColdReboot() { return (mBootReason == BOOT_REASON_COLD); }

    bool getBootUpTraffic() { return mBootUpTraffic; }
    uint32 getBootUpTrafficTimer() { return mBootUpTrafficTimer; }

    void setFaultStable(bool state) { mFaultStable = state; }
    bool getFaultStable() { return mFaultStable; }
    uint32 getFaultStableTimer() { return mFaultStableTimer; }

    void setDcoShutdownInProgress(bool state) { mDcoShutdownInProgress = state; }
    bool getDcoShutdownInProgress() { return mDcoShutdownInProgress; }

    DpSyncState getDpSyncState() { return mDpSyncState; }
    std::string toString(DpSyncState syncState);

    gearbox::GearBoxAdIf* getGearBoxAd()   { return mpGearBoxAd; }
    tuning::SerdesTuner*  getSerdesTuner() { return mpSerdesTuner; }
    shared_ptr<FpgaSramIf> getFpgaSramIf() { return mspFpgaSramIf; }
    shared_ptr<FpgaSacIf>  getFpgaSacIf()  { return mspFpgaSacIf; }
    shared_ptr<FpgaMiscIf> getFpgaMiscIf() { return mspFpgaMiscIf; }

    bool isTomSerdesTuned(uint32 tomId);
    void setTomSerdesTuned(uint32 tomId);
    void clrTomSerdesTuned(uint32 tomId);

    bool isDpMgrInitDone     (std::ostream& out);
    bool isDpMgrBoardInitDone(std::ostream& out);

    bool isConfigFailure();

    uint32 getConfigCancelCount();

    void cancelConfigProcess();

    void triggerDcoColdBoot();

private:

    // Initialize the Data Plane Manager
    void initializeDpMgr();

    void createObjMgrs();
    void initializeCardObjMgr();
    void initializeObjMgrs();
    void initRedisConnection();
    void initAndRegisterCallbacks();
    void PollConnectUpdates();
    void HndlConnectChgLowPwr();
    void PollBootUpTraffic();
    void writeBootUpMarkerFile(bool trafficUp);
    void initializeFpgaSramIf();
    void initializeFpgaSacIf();
    void initializeFpgaMiscIf();

    void handleBoardInitStateUpdate(chm6_common::Chm6BoardInitState* pBrdState);

    void setSyncReady(bool isReady);
    void setDpSyncState(DpSyncState syncState);
    void sendDpSyncState();

    void initSerdesInitMarkerScratchpad();
    void updateSerdesInitMarkerScratchpad(uint32 serdesInitMarkerBitmap);

    void syncAllConfig();

    std::string toString(ObjectMgrs objId);

    std::string printBootReason(BootReason bootreason);

    std::vector<DpObjectMgr*>  mvDpObjMgr;

    std::recursive_mutex  mDpMgrMtx;
    std::mutex mSrdsMrkrMtx;

    bool        mIsInitDone;
    bool        mIsSyncReady;
    bool        mIsBrdInitDone;
    bool        mIsBrdInitFail;
    bool        mIsOnResyncDone;
    bool        mIsForceSyncReady;
    bool        mIsAgentSyncConfigDone;
    DpSyncState mDpSyncState;;
    std::thread mConnectTh;
    std::thread mBootUpTrafficTh;
    bool        mBootUpTraffic;
    uint32      mBootUpTrafficTimer;

    bool                   mIsConnected[NUM_CPU_CONN_TYPES];
    BootReason             mBootReason;
    chm6_common::PowerMode mPowerMode;
    uint32                 mChassisId;
    gearbox::GearBoxAdIf*  mpGearBoxAd;
    tuning::SerdesTuner*   mpSerdesTuner;
    unique_ptr<RegIfFactory> mupRegIfFactory;
    shared_ptr<FpgaSramIf>   mspFpgaSramIf;
    uint32                 mTomInitMarker;
    uint32                 mSerdesInitMarker;
    shared_ptr<FpgaSacIf>  mspFpgaSacIf;
    shared_ptr<FpgaMiscIf> mspFpgaMiscIf;
    bool                   mFaultStable;
    uint32                 mFaultStableTimer;
    bool                   mDcoShutdownInProgress;
};

typedef SingletonService<DataPlaneMgr> DataPlaneMgrSingleton;

} // namespace DataPlane
#endif // DATA_PLANE_MGR_H
