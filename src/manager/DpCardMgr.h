/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_CARD_MGR_H
#define DP_CARD_MGR_H

#include <memory>
#include <google/protobuf/message.h>

#include "CardConfigHandler.h"
#include "CardStateCollector.h"
#include "DpObjectMgr.h"
#include "DcoCardAdapter.h"
#include "DcoSecProcAdapter.h"
#include "DcoSecProcCardCfgHdlr.h"
#include "DpPmHandler.h"

#include <SingletonService.h>

namespace DataPlane
{

class DpCardMgr : public DpObjectMgr
{
public:

    DpCardMgr();
    virtual ~DpCardMgr();

    virtual void initialize();

    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    void forceDcoState(uint32_t state, std::ostream& os) { mspCardCollector->forceDcoState(state); }
    void dumpDcoState(std::ostream& os) { mspCardCollector->dumpDcoState(os); }
    void forceFanTempFlags(uint32_t tempHi, uint32_t tempStable, std::ostream& os) { mspCardCollector->forceFanTempFlags(tempHi, tempStable); }
    void dumpFanTempFlags(std::ostream& os) { mspCardCollector->dumpFanTempFlags(os); }
    void dumpCardCacheState(std::ostream& os) { mspCardCollector->dumpCacheState(os); }
    void dumpDcoCapabilities(std::ostream& os) { mspCardCollector->dumpCacheDcoCapabilities(os); }
    void dumpCardCacheFault(std::ostream& os) { mspCardCollector->dumpCacheFault(os); }
    void setCardStatePollMode(bool mode, std::ostream& os) { mspCardCollector->setStatePollMode(mode, os); }
    void setCardFaultPollMode(bool mode, std::ostream& os) { mspCardCollector->setFaultPollMode(mode, os); }
    void setCardStatePollInterval(uint32_t intvl, std::ostream& os) { mspCardCollector->setStatePollInterval(intvl, os); }
    void setCardFaultPollInterval(uint32_t intvl, std::ostream& os) { mspCardCollector->setFaultPollInterval(intvl, os); }
    void dumpCardPollInfo(std::ostream& os) { mspCardCollector->dumpPollInfo(os); }
    void setDcoFaultReportMode(std::ostream& os, bool mode) { mspCardCollector->setIsDcoFaultReportEn(os, mode); }

    DpAdapter::DcoCardAdapter* getAdPtr() { return mpCardAd; };

    bool isDcoSyncReady();
    bool isConnectFaultLatch();

    void setCardEnable(bool isEnable);

    void setCardStateSyncReady(bool isReady);

    void startFaultCollector();

    /*
    * Dump board subscriptions, retries
    * @param: output stream [out]
    */
    void DumpBoardSubscriptions(std::ostream& out) { (DpAdapter::DcoSecProcAdapter::GetInstance())->DumpBoardSubscriptions(out);};

    virtual bool isSubscribeConnUp(DcoCpuConnectionsType connId);

    virtual bool isSubEnabled();

    virtual void updateFaultCache();

    void setLocalPowerMode(chm6_common::PowerMode pwrMode);

    void updateDcoPowerMode(bool isHighPwr);

protected:

    virtual void start();
    virtual void connectionUpdate();
    virtual void DcoSecProcConnectionUpdate(bool isConnectUp);

    virtual void reSubscribe();

private:

    void handleBCMSwitchFaultStateUpdate(chm6_pcp::Chm6BcmSwitchFault* pBCMSwitchFault);

    // Card PB Translator
    void createCardPbTranslator();

    // Card Config Handler
    void createCardConfigHandler();

    // Card State Collector
    void createCardStateCollector();

    // Card PM Callback registration
    void registerCardPm();
    void unregisterCardPm();

    // GCC Control PM Callback registration
    void registerGccPm();
    void unregisterGccPm();

    std::shared_ptr<CardStateCollector>  mspCardCollector;
    std::shared_ptr<CardConfigHandler>   mspCardCfgHndlr;
    std::shared_ptr<DcoSecProcCardCfgHdlr>  mspDcoSecProcCardCfgHndlr;

    CardPbTranslator             *mpPbTranslator;

    DpAdapter::DcoCardAdapter    *mpCardAd;

    //Adapter to handle communication with Dco security processor
    DpAdapter::DcoSecProcAdapter    *mpDcoSecProcAd;

    chm6_common::PowerMode mPowerMode;  // Chm6 Card Power Mode
};

typedef SingletonService<DpCardMgr> DpCardMgrSingleton;

} // namespace DataPlane

#endif /* DP_CARD_MGR_H */
