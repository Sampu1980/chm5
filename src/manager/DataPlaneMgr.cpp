/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#include <iostream>
#include <thread>

// turn off the specific warning. Can also use "-Wall"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include "DataPlaneMgr.h"
#include "DpCarrierMgr.h"
#include "DpGigeClientMgr.h"
#include "DpOtuMgr.h"
#include "DpOduMgr.h"
#include "DpXcMgr.h"
#include "DpCardMgr.h"
#include "DpPeerDiscoveryMgr.h"
#include "DpKeyMgmtMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DpGccMgr.h"
#include "GearBoxCreator.h"
#include "SerdesTunerCreator.h"

// Protobufs
#include "chm6/redis_adapter/application_servicer.h"
#include <InfnTracer/InfnTracer.h>

// turn the warnings back on
#pragma GCC diagnostic pop

#include <string>
#include <stdlib.h>

using namespace ::std;
using namespace DpAdapter;
using namespace gearbox;
using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;
using chm6_dataplane::Chm6ScgConfig;
using chm6_dataplane::Chm6CarrierConfig;
using chm6_dataplane::Chm6GigeClientConfig;
using chm6_dataplane::Chm6OtuConfig;
using chm6_dataplane::Chm6OduConfig;
using chm6_dataplane::Chm6XCConfig;
using chm6_dataplane::Chm6CarrierState;
using chm6_dataplane::Chm6GigeClientState;
using chm6_dataplane::Chm6OtuState;
using chm6_dataplane::Chm6OduState;
using chm6_dataplane::Chm6XCState;
using chm6_dataplane::Chm6SyncConfig;

using lccmn_tom::TomPresenceMap;
using chm6_dataplane::Chm6SyncState;

#define ILOG INFN_LOG(SeverityLevel::info)

namespace DataPlane
{

DataPlaneMgr::DataPlaneMgr()
    : mIsInitDone(false)
    , mIsSyncReady(false)
    , mPowerMode(chm6_common::POWER_MODE_UNSPECIFIED)
    , mChassisId(0)
    , mIsBrdInitDone(false)
    , mIsBrdInitFail(false)
    , mIsOnResyncDone(false)
    , mIsForceSyncReady(false)
    , mIsAgentSyncConfigDone(false)
    , mBootUpTraffic(false)
    , mBootUpTrafficTimer(cBootUpTrafficTimeout)
    , mDpSyncState(DP_SYNC_STATE_UNSPECIFIED)
    , mupRegIfFactory(nullptr)
    , mspFpgaSramIf(nullptr)
    , mTomInitMarker(0)
    , mSerdesInitMarker(0)
    , mspFpgaSacIf(nullptr)
    , mspFpgaMiscIf(nullptr)
    , mBootReason(BOOT_REASON_UNSPECIFIED)
    , mFaultStable(false)
    , mFaultStableTimer(cFaultStableTimeout)
    , mDcoShutdownInProgress(false)
{
    ILOG << "Constructor ";

    memset(mIsConnected, 0, sizeof(mIsConnected));

    mIsForceSyncReady = ::dp_env::DpEnvSingleton::Instance()->isForceSyncReady();

    createObjMgrs();

    initializeCardObjMgr();

    // Start Connection Update Thread
    ILOG << "Starting Poll Connection Updates Thread for connection changes...";
    mConnectTh = std::thread(&DataPlaneMgr::PollConnectUpdates, std::ref(*this));

    ILOG << "Starting Poll BootUp Traffic Thread to monitor when traffic up on boot up...";
    mBootUpTrafficTh = std::thread(&DataPlaneMgr::PollBootUpTraffic, std::ref(*this));

    initRedisConnection();

    ILOG << "Construction complete!";
}

DataPlaneMgr::~DataPlaneMgr()
{
    std::vector<DpObjectMgr*>::iterator it;
    for(it == mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
    {
        delete *it;
    }
    mvDpObjMgr.clear();
}

void DataPlaneMgr::initRedisCallbacks()
{
    ILOG << "";

    // Start App Servicer (Redis Callback Listener)
    initAndRegisterCallbacks();

    ILOG << "**** Done";
}

void DataPlaneMgr::initializeDpMgr()
{
    std::lock_guard<std::recursive_mutex> lock(mDpMgrMtx);

    DpCardMgr* pCardMgr = static_cast<DpCardMgr*>(mvDpObjMgr[OBJ_MGR_CARD]);
    // Init Sync Ready to false - update to redis
    pCardMgr->setCardStateSyncReady(false);

    initializeFpgaMiscIf();

    pCardMgr->setLocalPowerMode(mPowerMode);

    if (mPowerMode == chm6_common::POWER_MODE_LOW)
    {
        ILOG << "CHM6 running in Low power mode. Setting DCO to Low Power, then doing nothing";

        mIsBrdInitDone = true;

        return;
    }

    mIsBrdInitDone = true;

    if (mIsInitDone)
    {
        ILOG << "Init is already done. Doing nothing here.";
        return;
    }

    gearbox::GearBoxCreatorSingleton::Instance()->CreateGearBox(mpGearBoxAd);
    tuning::SerdesTunerCreatorSingleton::Instance()->CreateSerdesTuner(mpSerdesTuner);

    initializeFpgaSramIf();

    initializeFpgaSacIf();

    initializeObjMgrs();

    mIsInitDone = true;

    ILOG << "*** DONE !!";

    return;
}

void DataPlaneMgr::createObjMgrs()
{
    mvDpObjMgr.resize(NUM_OBJ_MGRS);

    DpCardMgr* pCardMgr = new DpCardMgr();
    DpCardMgrSingleton::InstallInstance(pCardMgr);

    mvDpObjMgr[OBJ_MGR_CARD] = pCardMgr;

    DpCarrierMgr* pCarMgr = new DpCarrierMgr();
    DpCarrierMgrSingleton::InstallInstance(pCarMgr);

    mvDpObjMgr[OBJ_MGR_CARRIER] = pCarMgr;

    DpGigeClientMgr* pGigeMgr = new DpGigeClientMgr();
    DpGigeClientMgrSingleton::InstallInstance(pGigeMgr);

    mvDpObjMgr[OBJ_MGR_GIGE] = pGigeMgr;

    DpOtuMgr* pOtuMgr = new DpOtuMgr();
    DpOtuMgrSingleton::InstallInstance(pOtuMgr);

    mvDpObjMgr[OBJ_MGR_OTU] = pOtuMgr;

    DpOduMgr* pOduMgr = new DpOduMgr();
    DpOduMgrSingleton::InstallInstance(pOduMgr);

    mvDpObjMgr[OBJ_MGR_ODU] = pOduMgr;

    DpXcMgr* pXcMgr = new DpXcMgr();
    DpXcMgrSingleton::InstallInstance(pXcMgr);

    mvDpObjMgr[OBJ_MGR_XCON] = pXcMgr;

    DpGccMgr* pGccMgr = new DpGccMgr();
    DpGccMgrSingleton::InstallInstance(pGccMgr);

    mvDpObjMgr[OBJ_MGR_GCC] = pGccMgr;

    DpPeerDiscoveryMgr* pPeerDiscMgr = new DpPeerDiscoveryMgr();
    DpPeerDiscoveryMgrSingleton::InstallInstance(pPeerDiscMgr);

    mvDpObjMgr[OBJ_MGR_PEER_DISC] = pPeerDiscMgr;

    DpKeyMgmtMgr *pKeyMgmtMgr = new DpKeyMgmtMgr();
    DpKeyMgmtMgrSingleton::InstallInstance(pKeyMgmtMgr);

    mvDpObjMgr[OBJ_MGR_KEY_MGMT] = pKeyMgmtMgr;
}

// Initialize card object mgr. Only init memory, do not start any activity yet
void DataPlaneMgr::initializeCardObjMgr()
{
    ILOG << "";

    mvDpObjMgr[OBJ_MGR_CARD]->initialize();
}

void DataPlaneMgr::initializeObjMgrs()
{
    ILOG << "";

    std::vector<DpObjectMgr*>::iterator it;
    for(it = mvDpObjMgr.begin() + 1; it != mvDpObjMgr.end(); ++it)
    {
        (*it)->initialize();
    }

    if (!mIsBrdInitFail)
    {
        // Start Card Fault Collection
        static_cast<DpCardMgr*>(mvDpObjMgr[OBJ_MGR_CARD])->startFaultCollector();
    }
}

void DataPlaneMgr::initRedisConnection()
{
    ILOG << "";
    AppServicerIntfSingleton::getInstance()->SetTracer(
        InfnTracer::Global() );

#if CONNECT_TO_REDIS
    if (false ==
        AppServicerIntfSingleton::getInstance()->ServicerInit("CHM6_DPMS"))
    {
        INFN_LOG(SeverityLevel::error) << "AppServicerIntfSingleton ServicerInit Failure (failed to connect to Redis). Exiting with Error...";

        exit(1);
    }
#endif
}

void DataPlaneMgr::initAndRegisterCallbacks()
{
    ILOG << "";

    {
        google::protobuf::Message* pConfig = new Chm6ScgConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(handler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pConfig = new Chm6CarrierConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(handler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pConfig = new Chm6GigeClientConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(handler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pConfig = new Chm6OtuConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(handler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pConfig = new Chm6OduConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(handler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pConfig = new Chm6XCConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(handler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pBrdInitMsg = new chm6_common::Chm6BoardInitState;
        std::unique_ptr<ICallbackHandler> brdInitHandler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pBrdInitMsg, std::move(brdInitHandler));
        delete pBrdInitMsg;
    }

    {
        google::protobuf::Message* pConfig = new chm6_common::Chm6DcoConfig;
        std::unique_ptr<ICallbackHandler> initHandler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(initHandler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pConfig = new lccmn_tom::TomPresenceMap;
        std::unique_ptr<ICallbackHandler> initHandler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(initHandler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pConfig = new Chm6KeyMgmtConfig;
        std::unique_ptr<ICallbackHandler> initHandler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pConfig, std::move(initHandler));
        delete pConfig;
    }

    {
        google::protobuf::Message* pVlanConfig = new VlanConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks( pVlanConfig, std::move(handler) );
        delete pVlanConfig;
    }

    {
        google::protobuf::Message* pBcmSwitchFault = new chm6_pcp::Chm6BcmSwitchFault;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks( pBcmSwitchFault, std::move(handler) );
        delete pBcmSwitchFault;
    }

    {
        google::protobuf::Message* pSyncConfig = new chm6_dataplane::Chm6SyncConfig;
        std::unique_ptr<ICallbackHandler> handler = std::unique_ptr<ICallbackHandler>(this);
        AppServicerIntfSingleton::getInstance()->RegisterCallbacks(pSyncConfig, std::move(handler));
        delete pSyncConfig;
    }

    try
    {
#if CONNECT_TO_REDIS
        ILOG << "*** Run AppServicerIntfSingleton";
        AppServicerIntfSingleton::getInstance()->Run();
        ILOG << "*** Done AppServicerIntfSingleton";

        // *** NOTE: Assuming above API only returns after onResync callbacks complete
        mIsOnResyncDone = true;

#else
        ILOG << "*** CONNECTING TO REDIS IS DISABLED!!!!!!!  DOING NOTHING!!!!";
#endif //CONNECT_TO_REDIS
    }
    catch (std::exception const &excp)
    {
        ILOG << "Exception caught while subscribing!" << excp.what() << "\n";
        exit(EXIT_FAILURE);

    }
    catch (...)
    {
        ILOG << "Exception caught while subscribing! Unknown excpetion" << "\n";
        exit(EXIT_FAILURE);
    }
}

void DataPlaneMgr::initSerdesInitMarkerScratchpad()
{
    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        INFN_LOG(SeverityLevel::info) << "Skip SerdesInitMarker FPGA scratchpad update in SIM env";
        return;
    }

    std::lock_guard<std::mutex> lock(mSrdsMrkrMtx);

    uint32 offset = cFpgaSram_SerdesInitMarkerOffset;
    uint32 val = mspFpgaSramIf->Read32(offset);

    INFN_LOG(SeverityLevel::info) << "Read SerdesInitMarker 0x" << std::hex << val << std::dec << " from FPGA scratchpad";

    uint32 serdesScratchpadMagic = (val & 0xFFFF0000) >> 16;

    if (SERDES_SCRATCHPAD_MAGIC_NUMBER == serdesScratchpadMagic)
    {
        mSerdesInitMarker = val & 0xFFFF;

        INFN_LOG(SeverityLevel::info) << "Set mSerdesInitMarker 0x" << std::hex << val << std::dec;

        return;
    }

    uint32 installedClientBitmap = static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->getInstalledGigeClients() |
                                   static_cast<DpOtuMgr*>(mvDpObjMgr[OBJ_MGR_OTU])->getInstalledOtuClients();

    for (int tomId=0; tomId<MAX_CLIENTS; tomId++)
    {
        if ( (mTomInitMarker & (1 << tomId)) && (installedClientBitmap & (1 << tomId)) )
        {
            mSerdesInitMarker |= (1 << tomId);
        }
        else
        {
            mSerdesInitMarker &= ~(1 << tomId);
        }
    }

    updateSerdesInitMarkerScratchpad(mSerdesInitMarker);
}

void DataPlaneMgr::updateSerdesInitMarkerScratchpad(uint32 serdesInitMarkerBitmap)
{
    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        INFN_LOG(SeverityLevel::info) << "Skip SerdesInitMarker FPGA scratchpad update in SIM env";
        return;
    }

    uint32 offset = cFpgaSram_SerdesInitMarkerOffset;

    uint32 val = (SERDES_SCRATCHPAD_MAGIC_NUMBER << 16) | serdesInitMarkerBitmap & 0x0000FFFF;

    INFN_LOG(SeverityLevel::info) << "Write SerdesInitMarker 0x" << std::hex << val << std::dec << " to FPGA scratchpad";

    mspFpgaSramIf->Write32(offset, val);
}

bool DataPlaneMgr::isTomSerdesTuned(uint32 tomId)
{
    if ((0 >= tomId) || (MAX_CLIENTS < tomId))
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mSrdsMrkrMtx);

    return (mSerdesInitMarker & (1 << (tomId-1)));
}

void DataPlaneMgr::setTomSerdesTuned(uint32 tomId)
{
    if ((0 >= tomId) || (MAX_CLIENTS < tomId))
    {
        return;
    }

    INFN_LOG(SeverityLevel::info) << "Set mSerdesInitMarker for Tom " << tomId;

    std::lock_guard<std::mutex> lock(mSrdsMrkrMtx);

    mSerdesInitMarker |= (1 << (tomId-1));

    updateSerdesInitMarkerScratchpad(mSerdesInitMarker);
}

void DataPlaneMgr::clrTomSerdesTuned(uint32 tomId)
{
    if ((0 >= tomId) || (MAX_CLIENTS < tomId))
    {
        return;
    }

    INFN_LOG(SeverityLevel::info) << "Clear mSerdesInitMarker for Tom " << tomId;

    std::lock_guard<std::mutex> lock(mSrdsMrkrMtx);

    mSerdesInitMarker &= ~(1 << (tomId-1));

    updateSerdesInitMarkerScratchpad(mSerdesInitMarker);
}

void DataPlaneMgr::PollConnectUpdates()
{
    ILOG << "Start monitoring for DCO connection updates.................. ";

    DpCardMgr* pCardMgr = static_cast<DpCardMgr*>(mvDpObjMgr[OBJ_MGR_CARD]);

    bool isDcoSyncReady, lastIsDcoSyncReady = false;
    bool isSyncConfigNeeded = false;
    bool isDpSyncStateUpdate = true;

    while (true)
    {
        // todo: check for high power mode also...

        if ((mPowerMode == chm6_common::POWER_MODE_LOW) && (mIsBrdInitDone))
        {
            HndlConnectChgLowPwr();
        }
        else if (mIsInitDone && mIsOnResyncDone)
        {
            INFN_LOG(SeverityLevel::debug) << "Getting Updates";

            std::lock_guard<std::recursive_mutex> lock(mDpMgrMtx);

            // Check for connection changes on both connection types
            for(uint32 i = 0; i < NUM_CPU_CONN_TYPES; i++)
            {
                bool isConnUp =
                    DcoConnectHandlerSingleton::Instance()->isDcoCommReady((DcoCpuConnectionsType)i);

                if (isConnUp       &&
                    !mIsConnected[i])
                {
                    // Connection Coming Up ...


                    ILOG << "Connectivity Change for " << ToString((DcoCpuConnectionsType)i) <<
                        " isConnectionUp: " << isConnUp;

                    ILOG << "Updating CardMgr";

                    // update card mgr only
                    pCardMgr->updateDcoConnection((DcoCpuConnectionsType)i,
                            isConnUp);
                }
                else if (!isConnUp &&
                         mIsConnected[i])
                {
                    // Connection Going Down ...

                    ILOG << "Connectivity Change for " << ToString((DcoCpuConnectionsType)i) <<
                                      " isConnectionUp: " << isConnUp;

                    // Set Sync Ready False
                    setSyncReady(false);

                    // Update all Mgrs
                    ILOG << "Updating All Mgrs";
                    std::vector<DpObjectMgr*>::iterator it;
                    for(it = mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
                    {
                        (*it)->updateDcoConnection((DcoCpuConnectionsType)i,
                                                   isConnUp);
                    }
                }

                mIsConnected[i] = isConnUp;
            }

            if (mIsForceSyncReady)
            {
                // For debug/testing

                isDcoSyncReady = true;
            }
            else
            {
                isDcoSyncReady = pCardMgr->isDcoSyncReady();
            }

            if (isDcoSyncReady != getSyncReady())
            {
                if (isDcoSyncReady)
                {
                    bool doSync = mIsConnected[DCC_ZYNQ] && mIsConnected[DCC_NXP];

                    ILOG << "DcoSyncReady Change to TRUE. doSync: " << doSync
                                                  << " IsZynqConnected: " << mIsConnected[DCC_ZYNQ]
                                                  << " IsNxpConnected: " << mIsConnected[DCC_NXP]
                                                  << " mIsForceSyncReady: " << mIsForceSyncReady;

                    if (doSync)
                    {
                        // Sync Ready Coming Up ...

                        // Update all Mgrs
                        ILOG << "Pushing update to ALL Object Mgrs....";
                        std::vector<DpObjectMgr*>::iterator it;
                        for(it = mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
                        {
                            if (mIsConnected[DCC_ZYNQ])
                            {
                                (*it)->updateDcoConnection(DCC_ZYNQ, mIsConnected[DCC_ZYNQ]);
                            }
                            if (mIsConnected[DCC_NXP])
                            {
                                (*it)->updateDcoConnection(DCC_NXP , mIsConnected[DCC_NXP ]);
                            }
                        }

                        // Make sure card is enabled (high power mode)
                        pCardMgr->updateDcoPowerMode(true);

                        // Sync config for all obj mgrs....
                        ILOG << "Trigger SyncConfig for ALL Object Mgrs....";
                        for(it = mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
                        {
                            (*it)->syncConfig();
                        }

                        initSerdesInitMarkerScratchpad();
                    }
                }
                else    // !isDcoSyncReady
                {
                    ILOG << "Dco SyncReady Change to FALSE. Updating DP SyncReady to FALSE";
                }

                // Set Sync Ready
                setSyncReady(isDcoSyncReady);
            }

            if (DP_SYNC_STATE_END == mDpSyncState)
            {
                if (false == mFaultStable)
                {
                    if ((true == mBootUpTraffic) || (mFaultStableTimer == 0))
                    {
                        INFN_LOG(SeverityLevel::info) << "Send SyncState END: mBootUpTraffic(" << mBootUpTraffic << ") mFaultStableTimer(" << mFaultStableTimer << ")";
                        mFaultStable = true;
                        mFaultStableTimer = 0;

                        sendDpSyncState();
                    }
                    else
                    {
                        mFaultStableTimer--;
                    }
                }
            }

            bool isConnectFaultLatch = pCardMgr->isConnectFaultLatch();
            if (isConnectFaultLatch && isDpSyncStateUpdate)
            {
                INFN_LOG(SeverityLevel::info) << "EQPT alarm present on initialization; Send SyncState END";

                setDpSyncState(DP_SYNC_STATE_END);
                sendDpSyncState();

                isDpSyncStateUpdate = false;
            }
        }

        // todo: make this sleep configurable ???
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // todo: handle thread exiting ???
    }
}

void DataPlaneMgr::HndlConnectChgLowPwr()
{
    INFN_LOG(SeverityLevel::debug) << "";

    std::lock_guard<std::recursive_mutex> lock(mDpMgrMtx);

    bool isConnUp =
            DcoConnectHandlerSingleton::Instance()->isDcoCommReady((DcoCpuConnectionsType)DCC_ZYNQ);

    DpCardMgr* pCardMgr = static_cast<DpCardMgr*>(mvDpObjMgr[OBJ_MGR_CARD]);

    if (isConnUp != mIsConnected[DCC_ZYNQ])
    {

        ILOG << "Connectivity Change for " << ToString((DcoCpuConnectionsType)DCC_ZYNQ) <<
                " isConnectionUp: " << isConnUp;

        if (isConnUp)
        {
            pCardMgr->updateDcoPowerMode(false);
        }

        pCardMgr->updateDcoConnection(DCC_ZYNQ,
                            isConnUp);

        mIsConnected[DCC_ZYNQ] = isConnUp;
    }

    isConnUp =
            DcoConnectHandlerSingleton::Instance()->isDcoCommReady((DcoCpuConnectionsType)DCC_NXP);

    if (isConnUp != mIsConnected[DCC_NXP])
    {

        ILOG << "Connectivity Change for " << ToString((DcoCpuConnectionsType)DCC_NXP) <<
                " isConnectionUp: " << isConnUp;

        pCardMgr->updateDcoConnection(DCC_NXP,
                            isConnUp);

        mIsConnected[DCC_NXP] = isConnUp;
    }
}

void DataPlaneMgr::PollBootUpTraffic()
{
    ILOG << "Start monitoring for BootUp Traffic.................. ";

    bool isTrafficUp = false;

    while (true)
    {
        if (true == mIsAgentSyncConfigDone)
        {
            if (0 < mBootUpTrafficTimer)
            {
                isTrafficUp = static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->checkBootUpTraffic() &&
                              static_cast<DpOtuMgr*>(mvDpObjMgr[OBJ_MGR_OTU])->checkBootUpTraffic() &&
                              static_cast<DpOduMgr*>(mvDpObjMgr[OBJ_MGR_ODU])->checkBootUpTraffic() &&
                              static_cast<DpCarrierMgr*>(mvDpObjMgr[OBJ_MGR_CARRIER])->checkBootUpTraffic();

                if (true == isTrafficUp)
                {
                    ILOG << "BootUp Traffic: Traffic Up";
                    mBootUpTraffic = true;
                    writeBootUpMarkerFile(mBootUpTraffic);

                    mBootUpTrafficTimer = 0;
                    break;
                }

                mBootUpTrafficTimer--;
            }

            if (0 == mBootUpTrafficTimer)
            {
                ILOG << "BootUp Traffic: Timed Out)";
                mBootUpTraffic = false;
                writeBootUpMarkerFile(mBootUpTraffic);
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ILOG << "Finish monitoring for BootUp Traffic.................. ";
}

void DataPlaneMgr::writeBootUpMarkerFile(bool trafficUp)
{
    INFN_LOG(SeverityLevel::info) << "BootUp Traffic: Writing ../log/msapps/bootup_marker.txt";

    std::ofstream output_file;

    output_file.open("/opt/infinera/chm6/log/msapps/bootup_marker.txt", ios::binary | ios::out | ios::trunc);
    output_file.clear();

    if (output_file.is_open())
    {
        if (true == trafficUp)
        {
            if (isColdReboot())
            {
                output_file << "Traffic Up (cold)" << endl;
            }
            else
            {
                output_file << "Traffic Up (warm)" << endl;
            }
        }
        else
        {
            output_file << "Timed Out" << endl;
        }
        output_file.close();
    }
}

void DataPlaneMgr::initializeFpgaSramIf()
{
    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        return;
    }

    if (mupRegIfFactory == nullptr)
    {
        mupRegIfFactory = make_unique<RegIfFactory>();
    }
    mspFpgaSramIf = mupRegIfFactory->CreateFpgaSramRegIf();
}

void DataPlaneMgr::initializeFpgaSacIf()
{
    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        return;
    }

    if (mupRegIfFactory == nullptr)
    {
        mupRegIfFactory = make_unique<RegIfFactory>();
    }
    mspFpgaSacIf = mupRegIfFactory->CreateFpgaSacRegIf();
}

void DataPlaneMgr::initializeFpgaMiscIf()
{
    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        return;
    }

    if (mupRegIfFactory == nullptr)
    {
        mupRegIfFactory = make_unique<RegIfFactory>();
    }
    mspFpgaMiscIf = mupRegIfFactory->CreateFpgaMiscRegIf();
}

// Callback functions to handle gige client config messages
void DataPlaneMgr::onCreate(google::protobuf::Message* objMsg)
{
    ILOG << "DataPlaneMgr::onCreate() called";

    // *** Determine which object message contains and route appropriately

    // Check for Chm6BoardInitState **********
    auto* pInitState = dynamic_cast<chm6_common::Chm6BoardInitState*>(objMsg);
    if (pInitState)
    {
        TRACE_STATE(chm6_common::Chm6BoardInitState, *pInitState, FROM_REDIS(),
            ILOG << "Determined it is BoardInitState Msg";
            handleBoardInitStateUpdate(pInitState);
	)
        return;
    }

    // Check for TomPresenceMap
    TomPresenceMap *pTomMsg = dynamic_cast<TomPresenceMap*>(objMsg);
    if (pTomMsg)
    {
        INFN_LOG(SeverityLevel::info) << "Determined it is a TomPresenceMap Msg";


	TRACE(TRACE_TAG("mIsInitDone", mIsInitDone))
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error TomPresenceMap callback received prior to Init completion. Not expected. Cannot process";
            return;
        }
        TRACE_STATE(TomPresenceMap, *pTomMsg, FROM_REDIS(),
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpOtuMgr*>(mvDpObjMgr[OBJ_MGR_OTU])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomTdaHoldOffMapUpdate(pTomMsg);
	) //TRACE_STATE
        return;
    }

    // Check for SyncConfig
    chm6_dataplane::Chm6SyncConfig *pSyncConfig = dynamic_cast<chm6_dataplane::Chm6SyncConfig*>(objMsg);
    if (pSyncConfig)
    {
        INFN_LOG(SeverityLevel::info) << " Chm6SyncConfig Msg received";


	TRACE(TRACE_TAG("mIsInitDone", mIsInitDone))
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error Chm6SyncConfig callback received prior to Init completion. Not expected. Cannot process";
            return;
        }
        TRACE_CONFIG(chm6_dataplane::Chm6SyncConfig, *pSyncConfig, FROM_REDIS(),

        if (hal_dataplane::SYNC_STATUS_START == pSyncConfig->hal().sync_status())
        {
            INFN_LOG(SeverityLevel::info) << " L1ServiceAgent sync config start";
            mIsAgentSyncConfigDone = false;
            setDpSyncState(DP_SYNC_STATE_START);
        }
        else if (hal_dataplane::SYNC_STATUS_END == pSyncConfig->hal().sync_status())
        {
            INFN_LOG(SeverityLevel::info) << " L1ServiceAgent sync config end";
            //update fault cache when sync transit from false to true
            if (mIsAgentSyncConfigDone == false)
            {
                for (const auto& elem : mvDpObjMgr)
                {
                    elem->updateFaultCache();
                }
            }
            mIsAgentSyncConfigDone = true;
            setDpSyncState(DP_SYNC_STATE_END);
        }
	) //TRACE_CONFIG
        return;
    }

    // Check for rest of objects
    std::vector<DpObjectMgr*>::iterator it;
    for(it = mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
    {
        if ((*it)->checkAndCreate(objMsg))
        {
            return;
        }
    }

    ILOG << "Matching Object Not Found. Obj Type Name: "
         << objMsg->GetTypeName()
         << " Doing Nothing....";
}

void DataPlaneMgr::onModify(google::protobuf::Message* objMsg)
{
    ILOG << "DataPlaneMgr::onModify() called";

    // *** Determine which object message contains and route appropriately

    // Check for Chm6BoardInitState **********
    auto* pInitState = dynamic_cast<chm6_common::Chm6BoardInitState*>(objMsg);
    if (pInitState)
    {

        TRACE_STATE(chm6_common::Chm6BoardInitState, *pInitState, FROM_REDIS(),
            ILOG << "Determined it is BoardInitState Msg";
            handleBoardInitStateUpdate(pInitState);
	)
        return;
    }

    // Check for TomPresenceMap
    TomPresenceMap *pTomMsg = dynamic_cast<TomPresenceMap*>(objMsg);
    if (pTomMsg)
    {
        INFN_LOG(SeverityLevel::info) << "Determined it is a TomPresenceMap Msg";


	TRACE(TRACE_TAG("mIsInitDone", mIsInitDone))
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error TomPresenceMap callback received prior to Init completion. Not expected. Cannot process";
            return;
        }

        TRACE_STATE(TomPresenceMap, *pTomMsg, FROM_REDIS(),
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpOtuMgr*>(mvDpObjMgr[OBJ_MGR_OTU])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomTdaHoldOffMapUpdate(pTomMsg);
	) //TRACE_STATE
        return;
    }

    // Check for SyncConfig
    chm6_dataplane::Chm6SyncConfig *pSyncConfig = dynamic_cast<chm6_dataplane::Chm6SyncConfig*>(objMsg);
    if (pSyncConfig)
    {
        INFN_LOG(SeverityLevel::info) << " Chm6SyncConfig Msg received";


	TRACE(TRACE_TAG("mIsInitDone", mIsInitDone))
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error Chm6SyncConfig callback received prior to Init completion. Not expected. Cannot process";
            return;
        }

        TRACE_CONFIG(chm6_dataplane::Chm6SyncConfig, *pSyncConfig, FROM_REDIS(),
        if (hal_dataplane::SYNC_STATUS_START == pSyncConfig->hal().sync_status())
        {
            INFN_LOG(SeverityLevel::info) << " L1ServiceAgent sync config start";
            mIsAgentSyncConfigDone = false;
            setDpSyncState(DP_SYNC_STATE_START);
        }
        else if (hal_dataplane::SYNC_STATUS_END == pSyncConfig->hal().sync_status())
        {
            INFN_LOG(SeverityLevel::info) << " L1ServiceAgent sync config end";
            if (mIsAgentSyncConfigDone == false)
            {
                for (const auto& elem : mvDpObjMgr)
                {
                    elem->updateFaultCache();
                }
            }
            mIsAgentSyncConfigDone = true;
            setDpSyncState(DP_SYNC_STATE_END);
        }
	) //TRACE_CONFIG
        return;
    }

    std::vector<DpObjectMgr*>::iterator it;
    for(it = mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
    {
        if ((*it)->checkAndUpdate(objMsg))
        {
            return;
        }
    }

    ILOG << "Matching Object Not Found. Obj Type Name: "
         << objMsg->GetTypeName()
         << " Doing Nothing....";
}

void DataPlaneMgr::onDelete(google::protobuf::Message* objMsg)
{
    ILOG << "DataPlaneMgr::onDelete() called";

    // *** Determine which object message contains and route appropriately

    // Check for Chm6BoardInitState **********
    auto* pInitState = dynamic_cast<chm6_common::Chm6BoardInitState*>(objMsg);
    if (pInitState)
    {

        TRACE_STATE(chm6_common::Chm6BoardInitState, *pInitState, FROM_REDIS(),
        ILOG << "Determined it is BoardInitState Msg";
	) //TRACE_STATE

        // Do nothing for delete (shouldn't happen)

        return;
    }

    // Check for TomPresenceMap
    TomPresenceMap *pTomMsg = dynamic_cast<TomPresenceMap*>(objMsg);
    if (pTomMsg)
    {
        INFN_LOG(SeverityLevel::info) << "Determined it is a TomPresenceMap Msg";


	TRACE(TRACE_TAG("mIsInitDone", mIsInitDone))
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error TomPresenceMap callback received prior to Init completion. Not expected. Cannot process";
            return;
        }
        TRACE_STATE(TomPresenceMap, *pTomMsg, FROM_REDIS(),
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpOtuMgr*>(mvDpObjMgr[OBJ_MGR_OTU])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomTdaHoldOffMapUpdate(pTomMsg);
	) //TRACE_STATE
        return;
    }

    // Check for SyncConfig
    chm6_dataplane::Chm6SyncConfig *pSyncConfig = dynamic_cast<chm6_dataplane::Chm6SyncConfig*>(objMsg);
    if (pSyncConfig)
    {
        TRACE_CONFIG(chm6_dataplane::Chm6SyncConfig, *pSyncConfig, FROM_REDIS(),
        INFN_LOG(SeverityLevel::info) << " Chm6SyncConfig Msg received";
	) //TRACE_CONFIG
        return;
    }

    std::vector<DpObjectMgr*>::iterator it;
    for(it = mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
    {
        if ((*it)->checkAndDelete(objMsg))
        {
            return;
        }
    }

    ILOG << "Matching Object Not Found. Obj Type Name: "
         << objMsg->GetTypeName()
         << " Doing Nothing....";
}

void DataPlaneMgr::onResync(google::protobuf::Message* objMsg)
{
    ILOG << "DataPlaneMgr::onResync() called";

    // Check for Chm6BoardInitState **********
    auto* pInitState = dynamic_cast<chm6_common::Chm6BoardInitState*>(objMsg);
    if (pInitState)
    {
        TRACE_STATE(chm6_common::Chm6BoardInitState, *pInitState, FROM_REDIS(),
        ILOG << "Determined it is BoardInitState Msg";
        handleBoardInitStateUpdate(pInitState);
	) //TRACE_STATE
        return;
    }

    // Check for TomPresenceMap
    TomPresenceMap *pTomMsg = dynamic_cast<TomPresenceMap*>(objMsg);
    if (pTomMsg)
    {
        INFN_LOG(SeverityLevel::info) << "Determined it is a TomPresenceMap Msg";

	TRACE(TRACE_TAG("mIsInitDone", mIsInitDone))
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error TomPresenceMap callback received prior to Init completion. Not expected. Cannot process";
            return;
        }

        TRACE_STATE(TomPresenceMap, *pTomMsg, FROM_REDIS(),
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpOtuMgr*>(mvDpObjMgr[OBJ_MGR_OTU])->handleTomPresenceMapUpdate(pTomMsg);
        static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->handleTomTdaHoldOffMapUpdate(pTomMsg);
	) //TRACE_STATE
        return;
    }

    // Check for SyncConfig
    chm6_dataplane::Chm6SyncConfig *pSyncConfig = dynamic_cast<chm6_dataplane::Chm6SyncConfig*>(objMsg);
    if (pSyncConfig)
    {
        INFN_LOG(SeverityLevel::info) << " Chm6SyncConfig Msg received";

	TRACE(TRACE_TAG("mIsInitDone", mIsInitDone))
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error Chm6SyncConfig callback received prior to Init completion. Not expected. Cannot process";
            return;
        }

        TRACE_CONFIG(chm6_dataplane::Chm6SyncConfig, *pSyncConfig, FROM_REDIS(),
        if (hal_dataplane::SYNC_STATUS_END == pSyncConfig->hal().sync_status())
        {
            if (mIsAgentSyncConfigDone == false)
            {
                for (const auto& elem : mvDpObjMgr)
                {
                    elem->updateFaultCache();
                }
            }
            mIsAgentSyncConfigDone = true;
            setDpSyncState(DP_SYNC_STATE_END);
        }
	) //TRACE_CONFIG
        return;
    }

    // Check for rest of objects
    std::vector<DpObjectMgr*>::iterator it;
    for(it = mvDpObjMgr.begin(); it != mvDpObjMgr.end(); ++it)
    {
        if ((*it)->checkOnResync(objMsg))
        {
            return;
        }
    }

    ILOG << "Matching Object Not Found. Obj Type Name: "
         << objMsg->GetTypeName()
         << " Doing Nothing....";
}

std::string DataPlaneMgr::printBootReason(BootReason bootreason)
{
    std::string reason;
    switch(bootreason)
    {
    case BOOT_REASON_UNSPECIFIED:
        reason = "Unspecified";
        break;
    case BOOT_REASON_COLD:
        reason = "Cold";
        break;
    case BOOT_REASON_WARM:
        reason = "Warm";
        break;
    default:
        reason = "Unknown bootreason=" + std::to_string(bootreason);
        break;
    }
    return reason;
}

void DataPlaneMgr::handleBoardInitStateUpdate(chm6_common::Chm6BoardInitState* pBrdState)
{
    string stateData;
    MessageToJsonString(*pBrdState, &stateData);
    ILOG << stateData;

    std::lock_guard<std::recursive_mutex> lock(mDpMgrMtx);

    mPowerMode = pBrdState->power_mode();

    if (pBrdState->init_state() == chm6_common::STATE_COMPLETE)
    {
        ILOG << " Detected Init Complete";

        mBootReason = (BootReason)pBrdState->boot_reason();
        ILOG << "Boot Reason=" << printBootReason(mBootReason);

        if (pBrdState->has_chassis_id())
        {
            mChassisId = static_cast<uint16>(pBrdState->chassis_id().value());
            ILOG << "Chassis ID = " << mChassisId;
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "Chassis ID = none";
        }

        if (pBrdState->init_status() == chm6_common::STATUS_SUCCESS)
        {
            ILOG << "Received init_status SUCCESS";
        }
        else if (pBrdState->init_status() == chm6_common::STATUS_FAIL)
        {
            mIsBrdInitFail = true;

            ILOG << "Received init_status FAIL";
        }

        if (pBrdState->has_tom_init_marker())
        {
            mTomInitMarker = pBrdState->tom_init_marker().value();
            ILOG << "TomInitMarker = 0x" << std::hex << mTomInitMarker << std::dec;
        }

        initializeDpMgr();
    }
}

void DataPlaneMgr::completeInit()
{
    initializeDpMgr();
}

bool DataPlaneMgr::getSyncReady()
{
    return mIsSyncReady;
}

void DataPlaneMgr::setSyncReady(bool isReady)
{
    std::lock_guard<std::recursive_mutex> lock(mDpMgrMtx);

    if (isReady != mIsSyncReady)
    {
        ILOG << " Set SyncReady: " << isReady;

        mIsSyncReady = isReady;

        static_cast<DpCardMgr*>(mvDpObjMgr[OBJ_MGR_CARD])->setCardStateSyncReady(isReady);

        if (isReady == true)
        {
            INFN_LOG(SeverityLevel::info) << "Read TomPresenceMap from Redis";
            static_cast<DpGigeClientMgr*>(mvDpObjMgr[OBJ_MGR_GIGE])->readTomPresenceMapFromRedis();
            static_cast<DpOtuMgr*>(mvDpObjMgr[OBJ_MGR_OTU])->readTomPresenceMapFromRedis();
        }
    }
}

void DataPlaneMgr::setDpSyncState(DpSyncState syncState)
{
    std::lock_guard<std::recursive_mutex> lock(mDpMgrMtx);

    if (syncState != mDpSyncState)
    {
        ILOG << " Set DpSyncState: " << toString(syncState);

        mDpSyncState = syncState;
    }
}

void DataPlaneMgr::sendDpSyncState()
{
    std::lock_guard<std::recursive_mutex> lock(mDpMgrMtx);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    std::string aid = getChassisIdStr() + "-" +
                      dp_env::DpEnvSingleton::Instance()->getSlotNum();

    chm6_dataplane::Chm6SyncState dpSyncState;
    dpSyncState.mutable_base_state()->mutable_config_id()->set_value(aid);
    dpSyncState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    dpSyncState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    hal_dataplane::SyncStatus pbStatus;
    switch (mDpSyncState)
    {
        case DP_SYNC_STATE_START:
            pbStatus = hal_dataplane::SYNC_STATUS_START;
            break;
        case DP_SYNC_STATE_END:
            pbStatus = hal_dataplane::SYNC_STATUS_END;
            break;
        default:
            pbStatus = hal_dataplane::SYNC_STATUS_UNSPECIFIED;
            break;
    }
    dpSyncState.mutable_hal()->set_sync_status(pbStatus);

    string stateDataStr;
    MessageToJsonString(dpSyncState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << "Chm6SyncState update : " << stateDataStr;

    DpMgrRedisObjectUpdate(dpSyncState);
}

void DataPlaneMgr::DpMgrRedisObjectCreate(google::protobuf::Message& objMsg)
{
#if CONNECT_TO_REDIS
    AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectCreate(objMsg);
#endif
}

void DataPlaneMgr::DpMgrRedisObjectUpdate(google::protobuf::Message& objMsg)
{
#if CONNECT_TO_REDIS
   AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectUpdate(objMsg);
#endif
}

void DataPlaneMgr::DpMgrRedisObjectDelete(std::vector<std::reference_wrapper<google::protobuf::Message>>& vObjMsg)
{
#if CONNECT_TO_REDIS
   AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectDelete(vObjMsg);
#endif
}

void DataPlaneMgr::DpMgrRedisObjectStream(google::protobuf::Message& objMsg)
{
#if CONNECT_TO_REDIS
   AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectStream(objMsg);
#endif
}

void DataPlaneMgr::syncAllConfig()
{
    ILOG << "";

    for(auto& obj : mvDpObjMgr)
    {
        obj->syncConfig();
    }
}

void DataPlaneMgr::populateInstalledConfig()
{
    ILOG << "";

    for(auto& obj : mvDpObjMgr)
    {
        obj->updateInstalledConfig();
    }
}

void DataPlaneMgr::listObjMgrs(std::ostream& out)
{
    out << "***** Object Managers *****" << endl << endl;

    for (uint32 i = 0; i < NUM_OBJ_MGRS; i++)
    {
        out << (uint32)i << "\t" << toString((ObjectMgrs)i) << endl;
    }
    out << endl;
}

void DataPlaneMgr::clearConfigCache(ostream& out, uint32 mgrIdx)
{
    ILOG << "Mgr Index: " << mgrIdx;

    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Clearing Config Cache for All Obj Mgrs..." << endl << endl;

        for(auto& obj : mvDpObjMgr)
        {
            obj->clearConfigCache(out);
        }
    }
    else
    {
        out << "***** Clearing Config Cache for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->clearConfigCache(out);
    }
}

void DataPlaneMgr::clearConfigCache(ostream& out, uint32 mgrIdx, string configId)
{
    ILOG << "Mgr Index: " << mgrIdx;

    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** mgrIdx must be less than " << NUM_OBJ_MGRS << endl << endl;
    }
    else
    {
        out << "***** Clearing Config Cache for " << toString((ObjectMgrs)mgrIdx) << " for configId " << configId << endl << endl;

        mvDpObjMgr[mgrIdx]->deleteConfigCacheObj(out, configId);
    }
}

void DataPlaneMgr::listConfigCache(ostream& out, uint32 mgrIdx)
{
    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Listing Config Cache for All Obj Mgrs..." << endl << endl;

        for(auto& obj : mvDpObjMgr)
        {
            obj->listConfigCache(out);
        }

        out << "***** DONE Listing Config Cache for All Obj Mgrs..." << endl << endl;
    }
    else
    {
        out << "***** Listing Config Cache for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->listConfigCache(out);
    }
}

void DataPlaneMgr::dumpConfigCache(ostream& out, uint32 mgrIdx)
{
    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Dumping Config Cache for All Obj Mgrs..." << endl << endl;

        for(auto& obj : mvDpObjMgr)
        {
            obj->dumpConfigCache(out);
        }

        out << "***** DONE Dumping Config Cache for All Obj Mgrs..." << endl << endl;
    }
    else
    {
        out << "***** Dumping Config Cache for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->dumpConfigCache(out);
    }
}

void DataPlaneMgr::dumpConfigCache(ostream& out, uint32 mgrIdx, string configId)
{
    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** mgrIdx must be less than " << NUM_OBJ_MGRS << endl << endl;
    }
    else
    {
        out << "***** Dumping Config Cache for " << toString((ObjectMgrs)mgrIdx) << " for configId " << configId << endl << endl;

        mvDpObjMgr[mgrIdx]->dumpConfigCache(out, configId);
    }
}

void DataPlaneMgr::syncConfigCache(ostream& out, uint32 mgrIdx)
{
    ILOG << "Mgr Index: " << mgrIdx;

    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Syncing Config Cache for All Obj Mgrs..." << endl << endl;

        syncAllConfig();

        out << "***** DONE Syncing Config Cache for All Obj Mgrs..." << endl << endl;
    }
    else
    {
        out << "***** Syncing Config Cache for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->syncConfig();
    }
}

void DataPlaneMgr::listStateCache(ostream& out, uint32 mgrIdx)
{
    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Listing State Cache for All Obj Mgrs..." << endl << endl;

        for(auto& obj : mvDpObjMgr)
        {
            obj->listStateCache(out);
        }

        out << "***** DONE Listing State Cache for All Obj Mgrs..." << endl << endl;
    }
    else
    {
        out << "***** Listing State Cache for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->listStateCache(out);
    }
}

void DataPlaneMgr::dumpStateCache(ostream& out, uint32 mgrIdx, std::string stateId)
{
    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Dumping State Cache for All Obj Mgrs..." << endl << endl;

        for(auto& obj : mvDpObjMgr)
        {
            obj->dumpStateCache(out, "ALL");
        }

        out << "***** DONE Dumping State Cache for All Obj Mgrs..." << endl << endl;
    }
    else
    {
        out << "***** Dumping State Cache for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->dumpStateCache(out, stateId);
    }
}

void DataPlaneMgr::dumpConfigProcess(ostream& out, uint32 mgrIdx)
{
    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Dump Config Process for All Obj Mgrs..." << endl << endl;

        for(auto& obj : mvDpObjMgr)
        {
            obj->dumpCurrConfigProc(out);
        }

        out << "***** DONE Dump Config Process for All Obj Mgrs..." << endl << endl;
    }
    else
    {
        out << "***** Dump Config Process for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->dumpCurrConfigProc(out);
    }
}

void DataPlaneMgr::dumpConfigProcessLast(ostream& out, uint32 mgrIdx)
{
    if (mgrIdx >= NUM_OBJ_MGRS)
    {
        out << "***** Dump Config Process for All Obj Mgrs..." << endl << endl;

        for(auto& obj : mvDpObjMgr)
        {
            obj->dumpLastConfigProc(out);
        }

        out << "***** DONE Dump Config Process for All Obj Mgrs..." << endl << endl;
    }
    else
    {
        out << "***** Dump Config Process for " << toString((ObjectMgrs)mgrIdx) << endl << endl;

        mvDpObjMgr[mgrIdx]->dumpLastConfigProc(out);
    }
}

string DataPlaneMgr::toString(ObjectMgrs objId)
 {
    string retStr;
    switch(objId)
    {
        case OBJ_MGR_CARD:
            retStr = "OBJ_MGR_CARD";
            break;
        case OBJ_MGR_CARRIER:
            retStr = "OBJ_MGR_CARRIER";
            break;
        case OBJ_MGR_GIGE:
            retStr = "OBJ_MGR_GIGE";
            break;
        case OBJ_MGR_OTU:
            retStr = "OBJ_MGR_OTU";
            break;
        case OBJ_MGR_ODU:
            retStr = "OBJ_MGR_ODU";
            break;
        case OBJ_MGR_XCON:
            retStr = "OBJ_MGR_XCON";
            break;
        case OBJ_MGR_GCC:
            retStr = "OBJ_MGR_GCC";
            break;
        case OBJ_MGR_PEER_DISC:
            retStr = "OBJ_MGR_PEER_DISC";
            break;
        case OBJ_MGR_KEY_MGMT:
            retStr = "OBJ_MGR_KEY_MGMT";
            break;
        default:
            retStr = "objId not found";
    }

    return retStr;
}

string DataPlaneMgr::toString(DpSyncState syncState)
 {
    string retStr;

    switch (syncState)
    {
        case DP_SYNC_STATE_START:
            retStr = "START";
            break;
        case DP_SYNC_STATE_END:
            retStr = "END";
            break;
        default:
            retStr = "UNSPECIFIED";
            break;
    }

    return retStr;
}

bool DataPlaneMgr::isDpMgrInitDone(std::ostream& out)
{
    if (false == mIsInitDone)
    {
        out << "DataPlaneMgr initialization not done!" << std::endl;
        return false;
    }

    return true;
}

bool DataPlaneMgr::isDpMgrBoardInitDone(std::ostream& out)
{
    if (false == mIsBrdInitDone)
    {
        out << "DataPlaneMgr board initialization not done!" << std::endl;
        return false;
    }

    return true;
}

bool DataPlaneMgr::isConfigFailure()
{
    return DpObjectMgr::isConfigFail();
}

uint32 DataPlaneMgr::getConfigCancelCount()
{
    return DpObjectMgr::getConfigCancelCount();
}

void DataPlaneMgr::cancelConfigProcess()
{
    DpObjectMgr::cancelConfigProc();
}

void DataPlaneMgr::triggerDcoColdBoot()
{
    chm6_common::Chm6DcoCardState dcoState;
    INFN_LOG(SeverityLevel::info) << " Notify board ms to trigger dco cold boot, update redis!";
    dcoState.mutable_base_state()->mutable_config_id()->set_value("Chm6Internal");
    dcoState.set_request_dco_action(hal_common::BOARD_ACTION_RESTART_COLD);
    DpMgrRedisObjectUpdate(dcoState);
}

} /* DataPlane */
