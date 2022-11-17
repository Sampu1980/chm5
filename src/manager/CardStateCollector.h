/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef CARD_STATE_COLLECTOR_H
#define CARD_STATE_COLLECTOR_H

#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <chrono>

#include "DpProtoDefs.h"

#include "DpStateCollector.h"
#include "CardPbTranslator.h"
#include "DcoCardAdapter.h"
#include "FaultCacheMap.h"

namespace DataPlane {


class CardStateCollector : public DpStateCollector
{
public:

    CardStateCollector(CardPbTranslator* pbTrans, DpAdapter::DcoCardAdapter *pCarAd);

    ~CardStateCollector() { if (mpCardState) delete mpCardState; }

    void createCard(std::string aid);
    void deleteCard(std::string aid);

    void startFault();
    void startStatus(bool isHighPowerMode);

    void forceDcoState(uint32_t state) { mForceDcoState = static_cast<DcoState>(state); }
    void dumpDcoState(std::ostream& os);

    void setCardStateSyncReady(bool isSyncReady);

    bool isDcoSyncReady();
    bool isConnectFaultLatch();

    virtual void setIsConnectionUp(bool val);

    void forceFanTempFlags(uint32_t tempHi, uint32_t tempStable);
    void dumpFanTempFlags(std::ostream& os);

    void dumpCacheState(std::ostream& os);
    void dumpCacheDcoCapabilities(std::ostream& os);
    void dumpCacheFault(std::ostream& os);
    void dumpPollInfo(std::ostream& os);
    void setIsDcoFaultReportEn(std::ostream& os, bool val);

    void setLinkFaultCache( const std::string strLinkPort,
                            const std::string strFaultLD,
                            const bool bFaultValueLC,
                            const std::string strFaultCRC,
                            const bool bFaultValueCRC );

    bool checkIsDcoLowPower();

private:

    bool isPCPModified() const;
    void setPCPModified();

    void collectCardFaults();
    void collectCardPm();
    void collectCardStatus();

    void getAndUpdateCardFaults();
    bool getCardFaults(DpAdapter::CardFault& cardFault);
    void updateCardFaults(DpAdapter::CardFault& cardFault, bool isConnectFault);
    void updatePCPFaults();
    void createCardFault();
    
    //Get secproc faults from secproc adapter
    bool checkSecprocCardFaults(hal_common::FaultType& tmpFault);

    void getAndUpdateCardPm();
    void getAndUpdateCardStatus();

    bool checkConnectionFaults(bool isFirstConnect, bool& isFaultSet);

    void checkCardStatusLowPower(bool& enPrint);

    std::string genKeyMgmtConfigId();

    std::string toString(const DataPlane::DcoState) const;

    void initShmMutex();

    boost::thread mThrFaults;
    boost::thread mThrPm;
    boost::thread mThrState;
    std::string   mName;

    uint64_t mCardEqptFaultBitmap;
    uint64_t mCardDacEqptFaultBitmap;
    uint64_t mCardPsEqptFaultBitmap;
    uint64_t mCardPostFaultBitmap;
    uint64_t mCardDacPostFaultBitmap;
    uint64_t mCardPsPostFaultBitmap;
    DpAdapter::DcoCardReboot mDcoCardRebootState;

    std::mutex mCardDataMtx;
    chm6_common::Chm6DcoCardState* mpCardState;
    chm6_common::Chm6DcoCardFault* mpCardFault;
    chm6_dataplane::DcoCapabilityState* mpDcoCapa;

    CardPbTranslator          *mpPbTranslator;
    infinera::chm6::keymgmt::v2::KeyMgmtState* mpOecKeyMgmtState;
	
    DpAdapter::DcoCardAdapter *mpDcoCardAd;

    DcoState mDcoState;
    DcoState mForceDcoState;

    // CLI set fan temperature flags
    wrapper::Bool mForceTempHiFanIncrease;
    wrapper::Bool mForceTempStableFanDecrease;

    // todo: change to DcoConnectionStatus
    bool     mConnectFault;
    bool     mConnectFaultLatch;
    bool     mIsFirstFaultCheck;
    std::chrono::time_point<std::chrono::steady_clock> mConnectFltChgTime;

    // Fault Cache Map
    FaultCacheMap mFaultCache;
    bool          mPCPModified;

    //DCO secproc state collector fault cache
    hal_common::FaultType mSecprocFault;
    bool mIsSecprocFaultChanged;

    // Flag to enable or disable DCO fault collection
    bool mIsDcoFaultReportEn;
    string mKeyName;
    bool mIsKeyMgmtStateChanged;

    bool mIsChm6HighPowerMode;

    void *mpShmMtxFcp;
};

} // namespace DataPlane

#endif /* CARD_STATE_COLLECTOR_H */


