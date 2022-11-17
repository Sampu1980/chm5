
/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_CARD_ADAPTER_H
#define DCO_CARD_ADAPTER_H
#include <google/protobuf/text_format.h>
#include "dcoyang/infinera_dco_card/infinera_dco_card.pb.h"
#include "dcoyang/infinera_dco_card_fault/infinera_dco_card_fault.pb.h"

#include "CardAdapter.h"

#include <SimCrudService.h>
#include "DpPm.h"

using dcoCard      = dcoyang::infinera_dco_card::DcoCard;
using dcoCardStat  = dcoyang::infinera_dco_card::DcoCard_State;
using cardConfig   = dcoyang::infinera_dco_card::DcoCard_Config;
using dcoCardCapa  = dcoyang::infinera_dco_card::DcoCard_Capabilities;

namespace DpAdapter {

// Card Adapter Base class
class DcoCardAdapter: public CardAdapter
{
public:
    DcoCardAdapter();
    virtual ~DcoCardAdapter();

    virtual bool createCard();
    virtual bool deleteCard();
    virtual bool initializeCard();

    virtual bool warmBoot();
    virtual bool coldBoot();    // Only use for test not support in CHM6 product release
    virtual bool shutdown(bool beforeColdReboot = false);    // DCO gracefull shutdown
    virtual bool setCardEnabled(bool enable);
    virtual bool setClientMode(ClientMode mode);
    virtual bool setClientMaxPacketLength(uint32_t len);

    virtual bool getCardConfig( CardConfig *cfg);
    virtual bool getCardStatus( CardStatus *stat, bool force = false);
    virtual bool getCardCapabilities(CardCapability *capa);
    virtual bool getCardFault();
    virtual bool getCardFault(CardFault *faults, bool force = false);
    virtual bool getCardPm();
    virtual ClientMode getCurClientMode() { return mCurClientMode; }

    virtual bool subCardPm();
    virtual void subCardFault();
    virtual void subCardState();
    virtual void subCardIsk();
    virtual void subscribeStreams();
    virtual void clearFltNotifyFlag();

    virtual bool cardConfigInfo(ostream& out);
    virtual bool cardStatusInfo(ostream& out, bool force = true);
    virtual bool cardCapabilityInfo(ostream& out);
    virtual bool cardFaultInfo(ostream& out);
    virtual bool cardFaultCapaInfo(ostream& out);
    virtual bool cardPmInfo(ostream& out);
    virtual void cardNotifyState(ostream& out);

    void dumpAll(std::ostream& out);

    void translateConfig      (const cardConfig& dcoCfg, CardConfig &cfg);
    void translateCapabilities(const dcoCardCapa& dcoCapa  , CardCapability& capa);

    void dumpConfig      (ostream& out, CardConfig& cfg);
    void dumpCapabilities(ostream& out, CardCapability& cap);

    virtual bool addIsk(string& keyPath);
    virtual bool removeIsk(string& keyName);

    bool isSimModeEn() { return mIsGnmiSimModeEn; }
    void setSimModeEn(bool enable);
    void SetSimPmData();

    void SetConnectionState(bool isConnectUp);
    void SetRebootAlarmAndConnectionState(DcoCardReboot rebootReason, bool isConnectUp);

    bool updateNotifyCache();

    static DataPlane::DpMsCardPm mCardPm;
    static AdapterCacheFault mCardEqptFault;
    static AdapterCacheFault mCardDacEqptFault;
    static AdapterCacheFault mCardPsEqptFault;
    static AdapterCacheFault mCardPostFault;
    static AdapterCacheFault mCardDacPostFault;
    static AdapterCacheFault mCardPsPostFault;

    static CardStatus        mCardNotifyState;
    static int               mIskSize;
    static ClientMode        mCfgClientMode;
    static ClientMode        mCurClientMode;
    static bool              mCardStateSubEnable;
    static bool              mCardIskSubEnable;
    static bool              mCardFltSubEnable;
    static bool              mCardPmSubEnable;

private:
    static const int CARD_REFCLK_FREQ_PRECISION = 6;
    static const int CARD_VERSION_PRECISION = 2;
    static const int CARD_BAUD_RATE = 7;

    bool setCardConfig(dcoCard *card);
    bool getCardState(dcoCard *card);
    void setKey (dcoCard *card);
    bool getCardFaultCapa();
    bool getCardStateObj( dcoyang::infinera_dco_card::DcoCard_State  *state);

    void dumpDcoFaultCapability(std::ostringstream & out, vector<DpAdapter::FaultCapability>& fltCapa);

    void setRebootAlarm(DcoCardReboot rebootReason);
    void clearRebootAlarm();
    void checkRebootTimer();

    std::recursive_mutex       mCrudPtrMtx;
    bool                       mIsGnmiSimModeEn;
    std::shared_ptr<::DpSim::SimSbGnmiClient>   mspSimCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspSbCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspCrud;
    CardFaultCapability         mFaultCapa;
    bool                        mCardPowerUp;
    std::thread					mSimCardPmThread;

    DcoCardReboot               mDcoCardRebootState;
    uint32                      mDcoCardRebootTimer;
    std::mutex                  mRebootStateMutex;

    bool                        mIsConnectUp;
    uint32                      mConnectUpSoakTimer;
};

}
#endif // DCO_CARD_ADAPTER_H
