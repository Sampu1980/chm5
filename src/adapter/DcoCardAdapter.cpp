
/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>      // std::setprecision
#include <boost/format.hpp>

#include <google/protobuf/text_format.h>
#include "CrudService.h"
#include "DcoCardAdapter.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "DcoPmSBAdapter.h"
#include "DcoConnectHandler.h"

using namespace std;
using std::make_shared;
using std::make_unique;

using namespace chrono;
using namespace google::protobuf;
using namespace GnmiClient;
using namespace DataPlane;
using namespace dcoyang::infinera_dco_card;
using namespace dcoyang::infinera_dco_card_fault;

using cardAction = dcoyang::infinera_dco_card::DcoCard_Action;
using capaLineKey = ::dcoyang::infinera_dco_card::DcoCard_Capabilities_SupportedLineModesKey;
using cardBoardState = dcoyang::infinera_dco_card::DcoCard_State_BoardState;
using cardStatePost = dcoyang::infinera_dco_card::DcoCard_State_PostStatus;

namespace DpAdapter {

static const std::string dcoName = "DCO 1";
// Mapping of DCO lanes to client serdes lanes on CHM6
static const int dcoToClientLane[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };

DpMsCardPm DcoCardAdapter::mCardPm;
AdapterCacheFault  DcoCardAdapter::mCardEqptFault;
AdapterCacheFault  DcoCardAdapter::mCardDacEqptFault;
AdapterCacheFault  DcoCardAdapter::mCardPsEqptFault;
AdapterCacheFault  DcoCardAdapter::mCardPostFault;
AdapterCacheFault  DcoCardAdapter::mCardDacPostFault;
AdapterCacheFault  DcoCardAdapter::mCardPsPostFault;
CardStatus         DcoCardAdapter::mCardNotifyState;
int                DcoCardAdapter::mIskSize;

bool DcoCardAdapter::mCardStateSubEnable = false;
bool DcoCardAdapter::mCardIskSubEnable   = false;
bool DcoCardAdapter::mCardFltSubEnable   = false;
bool DcoCardAdapter::mCardPmSubEnable    = false;

//configured client mode from CLI
ClientMode DcoCardAdapter::mCfgClientMode;
//active running dco client mode
ClientMode DcoCardAdapter::mCurClientMode;


// Time period that DCO will take from start reboot to finish
static const uint32 cDcoWarmRebootPeriod_sec = 720; // 12 minutes
static const uint32 cDcoColdRebootPeriod_sec = 1080; // 18 minutes

static const uint32 cDcoConnectUpSoak_sec = 15;

DcoCardAdapter::DcoCardAdapter()
{
    mspSimCrud = DpSim::SimSbGnmiClient::getInstance();

    std::string serverAddrAndPort = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoZynq());
    INFN_LOG(SeverityLevel::info) << "Gnmi Server Address = " << serverAddrAndPort;
    mspSbCrud = std::dynamic_pointer_cast<GnmiClient::SBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_ZYNQ));

    setSimModeEn(dp_env::DpEnvSingleton::Instance()->isSimEnv());

    if (mspCrud == NULL)
    {
        INFN_LOG(SeverityLevel::error) <<  __func__ << "Crud is NULL" << '\n';
    }
    mCardPowerUp = false;
    mDcoCardRebootState = DCO_CARD_REBOOT_NONE;
    mDcoCardRebootTimer = 0;
    mIsConnectUp = false;
    mConnectUpSoakTimer = 0;
}

DcoCardAdapter::~DcoCardAdapter()
{

}

bool DcoCardAdapter::createCard()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " Test run... "  << endl;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    bool status = true;
    grpc::Status gStatus = mspCrud->Create( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " create FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    subscribeStreams();

    return status;
}

void DcoCardAdapter::subscribeStreams()
{
    clearFltNotifyFlag();
    if (!mCardFltSubEnable)
    {
        subCardFault();
        mCardFltSubEnable = true;
    }
    if (!mCardStateSubEnable)
    {
        subCardState();
        mCardStateSubEnable = true;
    }
    if (!mCardIskSubEnable)
    {
        subCardIsk();
        mCardIskSubEnable = true;
    }
    if (!mCardPmSubEnable)
    {
        // Added to subscribe to DCO temperature data events.
        subCardPm();
        mCardPmSubEnable = true;
    }
}

bool DcoCardAdapter::deleteCard()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    bool status = true;
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " create FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCardAdapter::initializeCard()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    bool status = true;

    status = createCard();

    if (status == false)
        return status;

    // Check card state and post status
    dcoCard card;
    setKey(&card);

    // TODO: Bao
    // Wait until we get command to work
    status = getCardState(&card);

    if (status == false)
    {
        return status;
    }
    if (card.state().client_mode() != dcoCardStat::CLIENTMODE_UNSET)
    {
	    switch (card.state().client_mode())
	    {
		    case dcoCardStat::CLIENTMODE_LXTP_E:
			    mCardNotifyState.mode = CLIENTMODE_LXTP_E;
			    break;
		    case dcoCardStat::CLIENTMODE_LXTP_M:
			    mCardNotifyState.mode = CLIENTMODE_LXTP_M;
			    break;
		    case dcoCardStat::CLIENTMODE_LXTP:
			    mCardNotifyState.mode = CLIENTMODE_LXTP;
			    break;
		    default:
			    mCardNotifyState.mode = CLIENTMODE_UNSET;
			    break;
	    }

        INFN_LOG(SeverityLevel::info) << __func__ << " update client mode:  " << EnumTranslate::toString(mCardNotifyState.mode) << endl;
    }

    switch (card.state().active_running_client_mode())
    {
        case dcoCardStat::CLIENTMODE_LXTP_E:
            mCurClientMode = CLIENTMODE_LXTP_E;
            break;
        case dcoCardStat::CLIENTMODE_LXTP_M:
            mCurClientMode= CLIENTMODE_LXTP_M;
            break;
        case dcoCardStat::CLIENTMODE_LXTP:
            mCurClientMode = CLIENTMODE_LXTP;
            break;
        default:
            mCurClientMode = CLIENTMODE_UNSET;
            break;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " update current client mode:  " << EnumTranslate::toString(mCurClientMode) << endl;

    if (card.mutable_state()->post_status() != dcoCardStat::POSTSTATUS_POST_SUCCEDED)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " FAIL PostStatus:  " << card.mutable_state()->post_status() << endl;
        status = false;
    }

    if (card.mutable_state()->board_state() == dcoCardStat::BOARDSTATE_FAULTED)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " FAIL Card State:  " << card.mutable_state()->board_state() << endl;
        status = false;
    }

    // Get Card Fault Capabitites
    if ( getCardFaultCapa() == false )
    {
        INFN_LOG(SeverityLevel::error) << " FAIL to read Card Fault Capabilities.";
        return false;
    }

    return status;
}

bool DcoCardAdapter::warmBoot()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);

	card.mutable_action()->set_boot(cardAction::BOOT_WARM_BOOT);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    INFN_LOG(SeverityLevel::info) << __func__ << " update ... "  << endl;
	bool status = true;
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
        mCardPowerUp = false;
        SetRebootAlarmAndConnectionState(DCO_CARD_REBOOT_WARM, false);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
	// this_thread::sleep_for(seconds(1));
    return status;
}

bool DcoCardAdapter::coldBoot()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);

	card.mutable_action()->set_boot(cardAction::BOOT_COLD_BOOT);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    INFN_LOG(SeverityLevel::info) << __func__ << " update ... "  << endl;
	bool status = true;
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
        mCardPowerUp = false;
        SetRebootAlarmAndConnectionState(DCO_CARD_REBOOT_COLD, false);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
	// this_thread::sleep_for(seconds(1));
    return status;
}

bool DcoCardAdapter::shutdown(bool beforeColdReboot)
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... beforeColdReboot: " << beforeColdReboot << endl;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);

	card.mutable_action()->set_boot(cardAction::BOOT_SHUTDOWN);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    INFN_LOG(SeverityLevel::info) << __func__ << " update ... "  << endl;
	bool status = true;
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
        mCardPowerUp = false;

        if (beforeColdReboot == true)
        {
            SetRebootAlarmAndConnectionState(DCO_CARD_REBOOT_COLD, false);
        }
        else
        {
            setRebootAlarm(DCO_CARD_REBOOT_NONE);
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        setRebootAlarm(DCO_CARD_REBOOT_NONE);
    }
    return status;
}

bool DcoCardAdapter::setCardEnabled(bool enable)
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);
    // Set card enable
    card.mutable_config()->mutable_enable()->set_value(enable);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    INFN_LOG(SeverityLevel::info) << __func__ << " update ... "  << endl;
	bool status = true;
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
	// this_thread::sleep_for(seconds(1));
    return status;
}

bool DcoCardAdapter::setClientMode(ClientMode mode)
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    using dcoLaneMap =  DcoCard_Config_SerdesLaneMappingKey;
    // using dcoLaneMap =   ::dcoyang::infinera_dco_card::DcoCard_Config_SerdesLaneMapping;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);
    // Set Serdes Speed to 50G for CHM6
    // card.mutable_config()->set_serdes_speed(&cardConfig::SP_50G);
    card.mutable_config()->set_serdes_speed(cardConfig::SERDESSPEED_50G);
    // Set Client Lane mapping
    for (int lane = 0; lane < DCO_MAX_SERDES_LANES; lane++)
    {
        dcoLaneMap *key = card.mutable_config()->add_serdes_lane_mapping();
        key->set_dco_serdes_lane(lane+1); // Dco lane
        key->mutable_serdes_lane_mapping()->mutable_client_serdes_lane()->set_value(dcoToClientLane[lane]); // client lane
        // key->mutable_client_serdes_lane()->set_value(dcoToClientLane[lane]); // client lane
    }
    // Set Client mode
    cardConfig::ClientMode dcoMode;
    switch(mode)
    {
        case CLIENTMODE_LXTP_E:
        dcoMode = cardConfig::CLIENTMODE_LXTP_E;
        break;
        case CLIENTMODE_LXTP_M:
        dcoMode = cardConfig::CLIENTMODE_LXTP_M;
        break;
        case CLIENTMODE_LXTP:
        dcoMode = cardConfig::CLIENTMODE_LXTP;
        break;
        default:
        dcoMode = cardConfig::CLIENTMODE_LXTP_E;
        INFN_LOG(SeverityLevel::info) << __func__ << " Unknown client mode set default LXTP_E "  << mode << endl;
        break;
    }
    card.mutable_config()->set_client_mode(dcoMode);
    //for sim case, set dco active Mode same as configured mode to allow new mode provision go through
    if (mIsGnmiSimModeEn)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << ", SIM case, set activeMode to "  << EnumTranslate::toString(mode) << endl;
        mCardNotifyState.activeMode = mode;
    }
    // TODO
    // Set Card Init action
    // Can I send it config and action in one shot?
    card.mutable_action()->mutable_card_init()->set_value(true);
    mCfgClientMode = mode; //update cache configured client mode

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    bool status = true;
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
	// this_thread::sleep_for(seconds(1));
    return status;
}


bool DcoCardAdapter::setClientMaxPacketLength(uint32_t len)
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    dcoCard card;
    setKey(&card);
    // Set Max packet Length
    card.mutable_config()->mutable_max_packet_length()->set_value(len);

    INFN_LOG(SeverityLevel::info) << __func__ << " pkt len: "  << len << endl;

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
	bool status = true;
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

void DcoCardAdapter::dumpAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    dcoCard dcoCardObj;

    setKey(&dcoCardObj);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&dcoCardObj);

    {
        std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

        grpc::Status gStatus = mspCrud->Read( msg );
        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message();

            out << "DCO Read FAIL. Grpc Error Code: " << gStatus.error_code() << ": " << gStatus.error_message() << endl;

            return;
        }
    }

    if (dcoCardObj.has_config())
    {
        CardConfig cardCfg;

        translateConfig(dcoCardObj.config(), cardCfg);

        out << "Dump Card Config..." << endl;
        dumpConfig(out, cardCfg);
    }
    else
    {
        out << "Card Config not present in response from DCO" << endl;
    }

    // Dump Card Fault from cache
    out << "Dump Card Fault Info from Cache..." << endl;
    if (cardFaultInfo(out) == false)
    {
        out << "cardFaultInfo Failed" << endl;
    }

    // Dump Card Status from cache
    out << "Dump Card Status Info from Cache..." << endl;
    if (cardStatusInfo(out, false) == false)
    {
       out << "cardStatusInfo Failed" << endl;
    }

    if (dcoCardObj.has_capabilities())
    {
        CardCapability cardCap;

        translateCapabilities(dcoCardObj.capabilities(), cardCap);

        out << "Dump Card Capabilities..." << endl;
        dumpCapabilities(out, cardCap);
    }

    out << "Dump Card Fault Capability from Cache..." << endl;
    if (cardFaultCapaInfo(out) == false)
    {
        out << "cardFaultCapaInfo Failed" << endl;
    }

    // Dump Card Notify from cache
    out << "Dump Card Notify State from Cache..." << endl;
    cardNotifyState(out);
}

bool DcoCardAdapter::cardConfigInfo(ostream& out)
{
    CardConfig cfg;

    memset(&cfg, 0, sizeof(cfg));

    bool status = getCardConfig(&cfg);
    if (status == false)
    {
        out << __func__ << "Error get Card Config" << '\n';
        return status;
    }

    dumpConfig(out, cfg);

    return status;
}

void DcoCardAdapter::dumpConfig(ostream& out, CardConfig& cfg)
{
    out << " DCO Card Config Info: \n" <<  endl;
    out << " Card name: " << cfg.cardName << endl;
    out << " Client mode: " << EnumTranslate::toString(cfg.mode) << endl;
    out << " Ref Clock Config: " << cfg.refClk << endl;
    out << " Serde Speed: " << cfg.speed << endl;
    out << " Client Max Pkt Len: " << cfg.maxPacketLength << endl;

    out << "\n Serdes Lane Mapping Table: DCO Lane:Client Lane " << endl;
    for ( int i = 0; i < DCO_MAX_SERDES_LANES; i++)
    {
        if (i == 16)
            out << endl;

        out << " " << i+1  << ":"<< cfg.hostToClientLane[i];
    }

    out << endl << endl;
}

// Default force = true. when call from adapter comnmand status_info
bool DcoCardAdapter::cardStatusInfo(ostream& out, bool force)
{
    CardStatus stat;
    bool status = getCardStatus(&stat, force);
    if ( status == false )
    {
        out << __func__ << " Get Dco Card State  FAIL "  << endl;
        return status;
    }
    out << " DCO Card Status Info: \n" <<  endl;
    out << " Card name: " << stat.cardName << endl;
    out << " Card description: " << stat.description << endl;
    out << " Card enable: " << (int) stat.enable << endl;
    out << " Card ref_clock_frequency: " << stat.refClkFrq << endl;
    out << " Card Post Status: " << (int) stat.post << endl;
    out << " Card State: " << EnumTranslate::toString(stat.state) << endl;
    out << " Card status: " << stat.status << endl;
    out << " Card equipment fault: 0x" << hex << stat.eqptFault << dec << endl;
    out << " Card dac equipment fault: 0x" << hex << stat.dacEqptFault << dec << endl;
    out << " Card ps equipment fault: 0x" << hex << stat.psEqptFault << dec << endl;
    out << " Card post fault: 0x" << hex << stat.postFault << dec << endl;
    out << " Card dac post fault: 0x" << hex << stat.dacPostFault << dec << endl;
    out << " Card ps post fault: 0x" << hex << stat.psPostFault << dec << endl;
    out << " Card SN : " << stat.serialNo << endl;
    out << " Card DSP Serial No: " << stat.dspSerialNo << endl;
    out << " Card Module Serial No: " << stat.moduleSerialNo << endl;
    out << " Card hardware revision: " << stat.hwRevision << endl;
    out << " Card API version: " << stat.apiVer << endl;
    out << " Card DSP revision: " << stat.dspRevision << endl;
    out << " Card FW version: " << stat.fwVersion << endl;
    out << " Card FPGA version: " << stat.fpgaVer << endl;
    out << " Card MCU version: " << stat.mcuVer << endl;
    out << " Card Boot Loader version: " << stat.bootLoaderVer << endl;
    out << " Card FW Upgrade Status: " << stat.fwUpgradeStatus << endl;
    out << " Card FW Upgrade State: " << EnumTranslate::toString(stat.fwUpgradeState) << endl;
    out << " Card Boot State: " << EnumTranslate::toString(stat.bootState) << endl;
    out << " Card Temp Hi Fan Increase:"  << (int) stat.tempHiFanIncrease << endl;
    out << " Card Temp Stable Fan Decrease:"  << (int) stat.tempStableFanDecrease << endl;
    out << " Card Operational State: " << (int) stat.operationState << endl;
    out << " Card key show: " << stat.keyShow << endl;
    out << " Card Client Mode : " << EnumTranslate::toString(stat.mode) << endl;
    out << " Client Max Pkt Len: " << stat.maxPacketLength << endl;
    out << " Card Active Client Mode : " << EnumTranslate::toString(stat.activeMode) << endl;
    out << " Card cfgMode: " << EnumTranslate::toString(mCfgClientMode) << endl;
    out << " Card curMode: " << EnumTranslate::toString(mCurClientMode) << endl;
    out << " mCardStateSubEnable: " << mCardStateSubEnable << endl;

    out << "\n ***Dump Card Status ISK: " << endl;
    for (auto& iter:stat.isk)
    {
        out << " ISK Key Name: " << (iter.second).key_name << endl;
        out << " ISK Key SN: " << (iter.second).key_serial_number << endl;
        out << " ISK Issuer Name: " << (iter.second).issuer_name << endl;
        out << " ISK Key Lenghth: " << (iter.second).key_length << endl;
        out << " ISK Key Payload: " << (iter.second).key_payload << endl;
        out << " ISK Key In Use: " << (int) (iter.second).is_key_in_use << endl;
        out << " ISK Key Verified: " << (int) (iter.second).is_key_verified << endl;
        out << " ISK Signature Payload: " << (iter.second).signature_payload << endl;
        out << " ISK Sig Hash Scheme: " << (iter.second).signature_hash_scheme << endl;
        out << " ISK Sig Gen Time: " << (iter.second).signature_gen_time << endl;
        out << " ISK Sig Key ID: " << (iter.second).signature_key_id << endl;
        out << " ISK Sig Algo: " << (iter.second).signature_algorithm << endl << endl;
    }
    out << " ISK idevid Cert: " << stat.idevidCert << endl;
    out << " mCardIskSubEnable: " << mCardIskSubEnable << endl;

    out << " mCardPmSubEnable: " << mCardPmSubEnable << endl;

    return status;
}

bool DcoCardAdapter::cardCapabilityInfo(ostream& out)
{
    CardCapability capa;
    memset(&capa, 0, sizeof(capa));
    bool status = getCardCapabilities(&capa);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Card Capabilities  FAIL "  << endl;
        return status;
    }

    dumpCapabilities(out, capa);

    return status;
}

void DcoCardAdapter::dumpCapabilities(ostream& out, CardCapability& cap)
{
    ios_base::fmtflags flag(out.flags());

    // Get Client Capabilities
    out << " Dco client capabilities: "  << endl;
    for ( auto i = cap.supportedClientRate.begin(); i != cap.supportedClientRate.end(); i++)
    {
        if (*i == PORT_RATE_ETH100G)
            out << " 100GE ";
        else if ( *i == PORT_RATE_ETH400G)
            out << " 400GE ";
        else
            out << " Unknown ";
    }
    out << endl;

    out << " Dco supported line mode; " << endl;
    for ( auto i = cap.supportedLineMode.begin(); i != cap.supportedLineMode.end(); i++)
    {
        out << " Line Mode: " << i->carrierModeDesignation;
        out << " Rate: " << i->dataRate;
        switch (i->clientMode)
        {
            case CLIENTMODE_LXTP_E:
                out << " LXTP_E ";
                break;
            case CLIENTMODE_LXTP_M:
                out << " LXTP_M ";
                break;
            case CLIENTMODE_LXTP:
                out << " LXTP ";
                break;
            default:
                out << " Unknown ";
                break;
        }

        out << " baud: " << fixed << setprecision(CARD_BAUD_RATE) << i->baudRate  << " App: " << i->primaryAppCode << " CompId: " << i->compatabilityId << " MxPwr: " << i->maxDcoPower << " Status: ";

        switch (i->carrierModeStatus)
        {
            case CARRIERMODESTATUS_SUPPORTED:
                out << "SUPPORTED";
                break;
            case CARRIERMODESTATUS_CANDIDATE:
                out << "CANDIDATED";
                break;
            case CARRIERMODESTATUS_EXPERIMENTAL:
                out << "EXPERIMENTAL";
                break;
            case CARRIERMODESTATUS_DEPRECATED:
                out << "DEPRECATED";
                break;
            default:
                out << "Unknown";
                break;
        }
        out << endl;
    }

    out << endl << endl;
    out << " Dco supported Advanced Parameter: " << endl;
    for ( auto i = cap.supportedAp.begin(); i != cap.supportedAp.end(); i++)
    {
        out << " AP Name: " << i->name;
        out << " Data Type: " << i->dataType;
        out << " Value: " << i->value;
        out << " Description: " << i->description;
        switch (i->dir)
        {
            case ADVPARMDIRECTION_TX:
                out << " TX ";
                break;
            case ADVPARMDIRECTION_RX:
                out << " RX ";
                break;
            case ADVPARMDIRECTION_BI_DIR:
                out << " BI_DIR ";
                break;
            default:
                out << " NOT_SET ";
                break;
        }
        out << " multiplicity: " << i->multiplicity;
        out << " Config Impact: ";
        switch (i->cfgImpact)
        {
            case ADVPARMCONFIGIMPACT_NO_CHANGE:
                out << " No Change ";
                break;
            case ADVPARMCONFIGIMPACT_NO_RE_ACQUIRE:
                out << " No Re Acquire ";
                break;
            case ADVPARMCONFIGIMPACT_RE_ACQUIRE:
                out << " Re Acquire ";
                break;
            case ADVPARMCONFIGIMPACT_FULL_CONFIG_PLL_CHANGE:
                out << " FULL Config PLL Change ";
                break;
            case ADVPARMCONFIGIMPACT_FULL_CONFIG_NO_PLL_CHANGE:
                out << " FULL Config No PLL Change ";
                break;
            default:
                out << " NOT_SET ";
                break;
        }
        out << " Service Impact: ";
        switch (i->servImpact)
        {
            case ADVPARMSERVICEIMPACT_SERVICE_EFFECTING:
                out << " Sevice Effecting ";
                break;
            case ADVPARMSERVICEIMPACT_NON_SERVICE_EFFECTING:
                out << " Non Sevice Effecting ";
                break;
            default:
                out << " NOT_SET ";
                break;
        }
        out << endl << endl;
    }
    out << endl;

    out.flags(flag);
}

bool DcoCardAdapter::cardFaultInfo(ostream& out)
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    CardFault fault;

    status = getCardFault( &fault );

    if (status == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Card Fault FAIL "  << endl;
        return status;
    }

    out << endl;
    out << "Card EQPT Fault: 0x" << hex << fault.eqptBmp  << dec << endl;
    for ( auto i = fault.eqpt.begin(); i != fault.eqpt.end(); i++)
    {

        out << " Fault: " << boost::format("%-38s") % i->faultName << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }

    out << endl;
    out << "Card DAC EQPT Fault: 0x" << hex << fault.dacEqptBmp << dec << endl;
    for ( auto i = fault.dacEqpt.begin(); i != fault.dacEqpt.end(); i++)
    {
        out << " Fault: " << boost::format("%-38s") % i->faultName << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }

    out << endl;
    out << "Card PS EQPT Fault: 0x" << hex << fault.psEqptBmp << dec << endl;
    for ( auto i = fault.psEqpt.begin(); i != fault.psEqpt.end(); i++)
    {
        out << " Fault: " << boost::format("%-38s") % i->faultName << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }

    out << endl;
    out << "Card POST Fault: 0x" << hex << fault.postBmp << dec  << endl;
    for ( auto i = fault.post.begin(); i != fault.post.end(); i++)
    {
        out << " Fault: " << boost::format("%-38s") % i->faultName << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }

    out << endl;
    out << "Card DAC POST Fault: 0x" << hex << fault.dacPostBmp << dec  << endl;
    for ( auto i = fault.dacPost.begin(); i != fault.dacPost.end(); i++)
    {
        out << " Fault: " << boost::format("%-38s") % i->faultName << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }

    out << endl;
    out << "Card PS POST Fault: 0x" << hex << fault.psPostBmp << dec  << endl;
    for ( auto i = fault.psPost.begin(); i != fault.psPost.end(); i++)
    {
        out << " Fault: " << boost::format("%-38s") % i->faultName << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }

    out << "DcoCardRebootState: " << fault.rebootState << endl;
    out << "\n mCardFltSubEnable: " << mCardFltSubEnable << endl;

    return status;
}

bool DcoCardAdapter::cardFaultCapaInfo(ostream& out)
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;


    std::ostringstream oss;

    if (mFaultCapa.eqpt.empty() == false)
    {
        out << std::endl << "<<<<<<<<<<<<<<<<<<<<< Equipment Faults >>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl << std::endl;
        dumpDcoFaultCapability(oss, mFaultCapa.eqpt);
        out << oss.str();
        oss.str("");
    }

    if (mFaultCapa.dacEqpt.empty() == false)
    {
        out << std::endl << "<<<<<<<<<<<<<<<<<<<<< Dac Equipment Faults >>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl << std::endl;
        dumpDcoFaultCapability(oss, mFaultCapa.dacEqpt);
        out << oss.str();
        oss.str("");
    }

    if (mFaultCapa.psEqpt.empty() == false)
    {
        out << std::endl << "<<<<<<<<<<<<<<<<<<<<< Ps Equipment Faults >>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl << std::endl;
        dumpDcoFaultCapability(oss, mFaultCapa.psEqpt);
        out << oss.str();
        oss.str("");
    }

    if (mFaultCapa.post.empty() == false)
    {
        out << std::endl << "<<<<<<<<<<<<<<<<<<<<< Post Faults >>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl << std::endl;
        dumpDcoFaultCapability(oss, mFaultCapa.post);
        out << oss.str();
        oss.str("");
    }

    if (mFaultCapa.dacPost.empty() == false)
    {
        out << std::endl << "<<<<<<<<<<<<<<<<<<<<< Dac Post Faults >>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl << std::endl;
        dumpDcoFaultCapability(oss, mFaultCapa.dacPost);
        out << oss.str();
        oss.str("");
    }

    if (mFaultCapa.psPost.empty() == false)
    {
        out << std::endl << "<<<<<<<<<<<<<<<<<<<<< Ps Post Faults >>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl << std::endl;
        dumpDcoFaultCapability(oss, mFaultCapa.psPost);
        out << oss.str();
    }

    return status;
}

bool DcoCardAdapter::cardPmInfo(ostream& out)
{
	out << "name: " << mCardPm.name << endl;
	out << "dsp temp: " << mCardPm.SystemState.DspTemperature << endl;
	out << "pic temp: " << mCardPm.SystemState.PicTemperature << endl;
	out << "modulecase temp: " << mCardPm.SystemState.ModuleCaseTemperature << endl;
	return true;
}

bool DcoCardAdapter::getCardConfig( CardConfig *pCfg)
{
    dcoCard card;
    setKey(&card);
    bool status = getCardState(&card);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Card State  FAIL "  << endl;
        return status;
    }

    if (card.has_config())
    {
        translateConfig(card.config(), *pCfg);
    }

    return status;
}

void DcoCardAdapter::translateConfig(const cardConfig& dcoCfg, CardConfig &cfg)
{
    INFN_LOG(SeverityLevel::info) << __func__ << " DCO Card Config: \n" <<  endl;

    if (dcoCfg.has_name())
    {
        cfg.cardName = dcoCfg.name().value();
    }

    if (dcoCfg.has_max_packet_length())
    {
        cfg.maxPacketLength = dcoCfg.max_packet_length().value();
    }

    switch (dcoCfg.client_mode())
    {
        case cardConfig::CLIENTMODE_LXTP_E:
            cfg.mode = CLIENTMODE_LXTP_E;
            break;
        case cardConfig::CLIENTMODE_LXTP_M:
            cfg.mode = CLIENTMODE_LXTP_M;
            break;
        case cardConfig::CLIENTMODE_LXTP:
            cfg.mode = CLIENTMODE_LXTP;
            break;
        default:
            cfg.mode = CLIENTMODE_UNSET;
            break;
    }
    switch (dcoCfg.ref_clock_config())
    {
        case cardConfig::REFCLOCKCONFIG_E1_E9:
            cfg.refClk = REFCLOCKCONFIG_E1_E9;
            break;
        case cardConfig::REFCLOCKCONFIG_M1_M9:
            cfg.refClk = REFCLOCKCONFIG_M1_M9;
            break;
        case cardConfig::REFCLOCKCONFIG_X1_X9:
            cfg.refClk = REFCLOCKCONFIG_X1_X9;
            break;
        default:
            cfg.refClk = REFCLOCKCONFIG_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.serdes_speed())
    {
        case cardConfig::SERDESSPEED_25G:
            cfg.speed = SERDES_SPEED_25G;
            break;
        case cardConfig::SERDESSPEED_50G:
            cfg.speed = SERDES_SPEED_50G;
            break;
        default:
            cfg.speed = SERDES_SPEED_UNSPECIFIED;
            break;
    }

    for ( int i = 0; i < dcoCfg.serdes_lane_mapping_size(); i++)
    {
        uint32 dcoLane = dcoCfg.serdes_lane_mapping(i).dco_serdes_lane();

        if (dcoLane >= DCO_MAX_SERDES_LANES)
        {
            continue;
            INFN_LOG(SeverityLevel::error) << " ERROR: dco serdes lane too big: " << dcoLane << " idx " << i << '\n';;
        }
        cfg.hostToClientLane[dcoLane] = dcoCfg.serdes_lane_mapping(i).serdes_lane_mapping().client_serdes_lane().value() ;
    }
}

bool DcoCardAdapter::getCardStatus(CardStatus *stat, bool force)
{
    dcoCardStat state;
    bool status = true;

    // Decide whether to get card status from DCO card state object or
    // from SW cache of card state/isk notify
    if ( mCardPowerUp == false || force == true )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get CardStatus from DCO not cached "  << endl;
        status = getCardStateObj(&state);
    }
    else
    {
        // return cached from Card State notify and Card ISK notify
        *stat = mCardNotifyState;
        return status;
    }

    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Card State  FAIL "  << endl;
        return status;
    }

    if (state.has_name())
    {
        stat->cardName = state.name().value();
    }
    if (state.has_card_description())
    {
        stat->description = state.card_description().value();
    }
    if (state.has_enable())
    {
        stat->enable = state.enable().value();
    }
    switch (state.client_mode())
    {
        case dcoCardStat::CLIENTMODE_LXTP_E:
            stat->mode = CLIENTMODE_LXTP_E;
            break;
        case dcoCardStat::CLIENTMODE_LXTP_M:
            stat->mode = CLIENTMODE_LXTP_M;
            break;
        case dcoCardStat::CLIENTMODE_LXTP:
            stat->mode = CLIENTMODE_LXTP;
            break;
        default:
            stat->mode = CLIENTMODE_UNSET;
            break;
    }
    if (state.has_ref_clock_frequency())
    {
        int64 digit = state.ref_clock_frequency().digits();
        uint32 precision = state.ref_clock_frequency().precision();
        stat->refClkFrq = digit / pow(10, precision);
    }
    // TODO Convert to proper Enum
    switch (state.post_status())
    {
        case dcoCardStat::POSTSTATUS_POST_FAULTED:
            stat->post = POSTSTATUS_FAULTED;
            break;
        case dcoCardStat::POSTSTATUS_POST_SUCCEDED:
            stat->post = POSTSTATUS_SUCCECED;
            break;
        default:
            stat->post = POSTSTATUS_UNSPECIFIED;
            break;
    }
    switch(state.board_state())
    {
        case dcoCardStat::BOARDSTATE_BOARD_INIT:
            stat->state = DCOSTATE_BRD_INIT;
            break;
        case dcoCardStat::BOARDSTATE_DSP_CONFIG:
            stat->state = DCOSTATE_DSP_CONFIG;
            break;
        case dcoCardStat::BOARDSTATE_LOW_POWER:
            stat->state = DCOSTATE_LOW_POWER;
            break;
        case dcoCardStat::BOARDSTATE_POWER_UP:
            stat->state = DCOSTATE_POWER_UP;
            break;
        case dcoCardStat::BOARDSTATE_POWER_DOWN:
            stat->state = DCOSTATE_POWER_DOWN;
            break;
        case dcoCardStat::BOARDSTATE_FAULTED:
            stat->state = DCOSTATE_FAULTED;
            break;
        default:
            stat->state = DCOSTATE_UNSPECIFIED;
            break;
    }
    // Making sure that DCO ready for card enable
    if ( mCardPowerUp == false  && stat->state != DCOSTATE_UNSPECIFIED && stat->state != DCOSTATE_BRD_INIT)
    {
        mCardPowerUp = true;
        INFN_LOG(SeverityLevel::info) << __func__ << " Power Up Dco Card "  << endl;
    }

    if (state.has_status())
    {
        stat->status = state.status().value();
    }
    if (state.has_equipment_fault())
    {
        stat->eqptFault = state.equipment_fault().value();
        uint64_t tmp = mCardEqptFault.faultBitmap;
        mCardEqptFault.faultBitmap = stat->eqptFault;
        if (tmp != mCardEqptFault.faultBitmap)
        {
            INFN_LOG(SeverityLevel::info) << " New mEqptFaultBitmap = 0x" << std::hex << mCardEqptFault.faultBitmap << std::dec;
        }
    }
    if (state.has_dac_equipment_fault())
    {
        stat->dacEqptFault = state.dac_equipment_fault().value();
        uint64_t tmp = mCardDacEqptFault.faultBitmap;
        mCardDacEqptFault.faultBitmap = stat->dacEqptFault;
        if (tmp != mCardDacEqptFault.faultBitmap)
        {
            INFN_LOG(SeverityLevel::info) << " New mDacEqptFaultBitmap = 0x" << std::hex << mCardDacEqptFault.faultBitmap << std::dec;
        }
    }
    if (state.has_ps_equipment_fault())
    {
        stat->psEqptFault = state.ps_equipment_fault().value();
        uint64_t tmp = mCardPsEqptFault.faultBitmap;
        mCardPsEqptFault.faultBitmap = stat->psEqptFault;
        if (tmp != mCardPsEqptFault.faultBitmap)
        {
            INFN_LOG(SeverityLevel::info) << " New mPsEqptFaultBitmap = 0x" << std::hex << mCardPsEqptFault.faultBitmap << std::dec;
        }
    }
    if (state.has_post_fault())
    {
        stat->postFault = state.post_fault().value();
        uint64_t tmp = mCardPostFault.faultBitmap;
        mCardPostFault.faultBitmap= stat->postFault;
        if (tmp != mCardPostFault.faultBitmap)
        {
            INFN_LOG(SeverityLevel::info) << " New mPostFaultBitmap = 0x" << std::hex << mCardPostFault.faultBitmap << std::dec;
        }
    }
    if (state.has_dac_post_fault())
    {
        stat->dacPostFault = state.dac_post_fault().value();
        uint64_t tmp = mCardDacPostFault.faultBitmap;
        mCardDacPostFault.faultBitmap= stat->dacPostFault;
        if (tmp != mCardDacPostFault.faultBitmap)
        {
            INFN_LOG(SeverityLevel::info) << " New mDacPostFaultBitmap = 0x" << std::hex << mCardDacPostFault.faultBitmap << std::dec;
        }
    }
    if (state.has_ps_post_fault())
    {
        stat->psPostFault = state.ps_post_fault().value();
        uint64_t tmp = mCardPsPostFault.faultBitmap;
        mCardPsPostFault.faultBitmap= stat->psPostFault;
        if (tmp != mCardPsPostFault.faultBitmap)
        {
            INFN_LOG(SeverityLevel::info) << " New mPsPostFaultBitmap = 0x" << std::hex << mCardPsPostFault.faultBitmap << std::dec;
        }
    }
    if (state.has_serial_no())
    {
        stat->serialNo = state.serial_no().value();
    }
    if (state.has_dsp_serial_no())
    {
        stat->dspSerialNo = state.dsp_serial_no().value();
    }
    if (state.has_module_serial_no())
    {
        stat->moduleSerialNo = state.module_serial_no().value();
    }
    if (state.has_hardware_revision())
    {
        stat->hwRevision = state.hardware_revision().value();
    }
    if (state.has_api_version())
    {
        stat->apiVer = state.api_version().value();
    }
    if (state.has_dsp_revision())
    {
        stat->dspRevision = state.dsp_revision().value();
    }
    if (state.has_firmware_version())
    {
        stat->fwVersion = state.firmware_version().value();
    }
    if (state.has_fpga_version())
    {
        stat->fpgaVer = state.fpga_version().value();
    }
    if (state.has_mcu_version())
    {
        stat->mcuVer = state.mcu_version().value();
    }
    if (state.has_bootloader_version())
    {
        stat->bootLoaderVer = state.bootloader_version().value();
    }
    if (state.has_firmware_upgrade_status())
    {
        stat->fwUpgradeStatus = state.firmware_upgrade_status().value();
    }
    switch(state.firmware_upgrade_state())
    {
        case dcoCardStat::FIRMWAREUPGRADESTATE_IDLE:
            stat->fwUpgradeState = FWUPGRADESTATE_IDLE;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_DOWNLOAD_IN_PROGRESS:
            stat->fwUpgradeState = FWUPGRADESTATE_DOWNLOAD_IN_PROGESS;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_DOWNLOAD_COMPLETE:
            stat->fwUpgradeState = FWUPGRADESTATE_DOWNLOAD_COMPLETE;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_INSTALL_COMPLETE:
            stat->fwUpgradeState = FWUPGRADESTATE_INSTALL_COMPLETE;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_ACTIVATE_IN_PROGRESS:
            stat->fwUpgradeState = FWUPGRADESTATE_ACTIVATE_IN_PROGRESS;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_ACTIVATE_COMPLETE:
            stat->fwUpgradeState = FWUPGRADESTATE_ACTIVATE_COMPLETE;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_ISK_OP_IN_PROGRESS:
            stat->fwUpgradeState = FWUPGRADESTATE_ISK_OP_IN_PROGRESS;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_ISK_OP_COMPLETE:
            stat->fwUpgradeState = FWUPGRADESTATE_ISK_OP_COMPLETE;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_DOWNLOAD_FAILED:
            stat->fwUpgradeState = FWUPGRADESTATE_DOWNLOAD_FAILED;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_INSTALL_FAILED:
            stat->fwUpgradeState = FWUPGRADESTATE_INSTALL_FAILED;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_ACTIVATE_FAILED:
            stat->fwUpgradeState = FWUPGRADESTATE_ACTIVATE_FAILED;
            break;
        case dcoCardStat::FIRMWAREUPGRADESTATE_ISK_OP_FAILED:
            stat->fwUpgradeState = FWUPGRADESTATE_ISK_OP_FAILED;
            break;
        default:
            stat->fwUpgradeState = FWUPGRADESTATE_UNSPECIFIED;
            break;
    }
    switch(state.boot_state())
    {
        case dcoCardStat::BOOTSTATE_CARD_UP:
            stat->bootState = BOOTSTATE_CARD_UP;
            break;
        case dcoCardStat::BOOTSTATE_IN_PROGRESS:
            stat->bootState = BOOTSTATE_IN_PROGRESS;
            break;
        case dcoCardStat::BOOTSTATE_REJECTED:
            stat->bootState = BOOTSTATE_REJECTED;
            break;
        default:
            stat->bootState = BOOTSTATE_UNSPECIFIED;
            break;
    }

    if (state.has_temp_high_fan_increase())
    {
	    stat->tempHiFanIncrease = state.temp_high_fan_increase().value();
    }
    else
    {
	    stat->tempHiFanIncrease = false;
    }

    if (state.has_temp_stable_fan_decrease())
    {
	    stat->tempStableFanDecrease = state.temp_stable_fan_decrease().value();
    }
    else
    {
	    stat->tempStableFanDecrease = false;
    }

    if (state.has_operational_state())
    {
	    stat->operationState = state.operational_state().value();
    }
    else
    {
	    stat->operationState = false;
    }

    if (state.has_key_show())
    {
	    stat->keyShow = state.key_show().value();
    }

    if (state.has_max_packet_length())
    {
        stat->maxPacketLength = state.max_packet_length().value();
    }

    switch (state.active_running_client_mode())
    {
        case dcoCardStat::CLIENTMODE_LXTP_E:
            stat->activeMode= CLIENTMODE_LXTP_E;
            break;
        case dcoCardStat::CLIENTMODE_LXTP_M:
            stat->activeMode= CLIENTMODE_LXTP_M;
            break;
        case dcoCardStat::CLIENTMODE_LXTP:
            stat->activeMode = CLIENTMODE_LXTP;
            break;
        default:
            stat->activeMode = CLIENTMODE_UNSET;
            break;
    }

    if (dp_env::DpEnvSingleton::Instance()->isSimEnv() && mIsGnmiSimModeEn)
    {
        stat->activeMode= CLIENTMODE_LXTP_E;
    }

    string isk_key_length_str;
    uint32 isk_key_length = 0;
    string isk_is_key_in_use ;
    string isk_is_key_verified;
    int prevIskSize= mIskSize;

    // for isk-keys size refer  "/dco-card/state/isk-keys" in infinera_dco_card_pb.h
    mIskSize = state.isk_keys_size();

    int isk_sigs_size = 0;

    if ( prevIskSize != mIskSize )
    {
        INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: size ***  " << mIskSize << endl;
    }


	for(int i = 0; i < mIskSize; ++i)
	{
        ImageSigningKey isk;

        isk.key_name           = state.isk_keys(i).isk_name();
        if ( prevIskSize != mIskSize )
        {
            INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: isk.key_name  *** " << isk.key_name   << endl;
        }
        isk.key_serial_number  = state.isk_keys(i).isk_keys().isk_serial().value();

        isk.issuer_name        = state.isk_keys(i).isk_keys().isk_issuer_name().value();

        isk_key_length_str     = state.isk_keys(i).isk_keys().isk_length().value();
        isk_key_length         = std::stoul (isk_key_length_str, nullptr, 0);
        isk.key_length         = isk_key_length;
        // INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: length proto uint32 *** " << isk.key_length << endl;

        isk.key_payload        = state.isk_keys(i).isk_keys().isk_payload().value();

        isk.is_key_in_use      = ( ( state.isk_keys(i).isk_keys().isk_inuse().value() == "true" ) ? true : false);

        isk.is_key_verified    = (( state.isk_keys(i).isk_keys().isk_isverified().value() == "true" ) ? true : false);
        // INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: isk.is_key_verified *** " << isk.is_key_verified  << endl;

        isk_sigs_size = state.isk_keys(i).isk_keys().isk_sigs_size();
        // INFN_LOG(SeverityLevel::info) <<__func__ << "isk sigs: size ***  " << isk_sigs_size << endl;

        // TODO: Remove the following after the ISKRing file is properly formed. Otherwise size may be 0 causing a sigabort.
        if (isk_sigs_size != 1)
        {
            isk_sigs_size = 1;
            INFN_LOG(SeverityLevel::info) <<__func__ << " Setting isk_sigs_size to " << isk_sigs_size << " *** "<< endl;
        }

        for (int j = 0; j <  isk_sigs_size; j++)
        {
            isk.signature_key_id = state.isk_keys(i).isk_keys().isk_sigs(j).isk_sig_id();

            if ( prevIskSize != mIskSize )
            {
                INFN_LOG(SeverityLevel::info) <<__func__ << " isk sig: signature key id ***  " << isk.signature_key_id << endl;
            }

            auto iskSigKey = state.isk_keys(i).isk_keys().isk_sigs(j).isk_sigs();
            isk.signature_hash_scheme = iskSigKey.isk_sig_hash_scheme().value();
            isk.signature_payload = iskSigKey.isk_sig_payload().value();
            isk.signature_gen_time = iskSigKey.isk_sig_gen_time().value();

            // Hard code the algorithm for now as the DCO Yang does not have it.
            // TODO: Remove the hard coded value when DCO Yang has this in the ISK definition
            isk.signature_algorithm = "ECDSA";
        }
        stat->isk.insert({isk.key_name, isk});
    }

	stat->idevidCert = state.dev_id().value();
    if ( prevIskSize != mIskSize )
    {
        INFN_LOG(SeverityLevel::info) <<__func__ << " isk dev id cert  ***  " << stat->idevidCert << endl;
    }

    // Copy CardStatus from DCO card state to our SW cache.
    mCardNotifyState = *stat;

    return status;
}

bool DcoCardAdapter::getCardCapabilities(CardCapability *capa)
{
    dcoCard card;
    setKey(&card);
    bool status = getCardState(&card);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Card object  FAIL "  << endl;
        return status;
    }
    if (card.has_capabilities() == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Card capabilities is empty "  << endl;
        return status;
    }

    translateCapabilities(card.capabilities(), *capa);

    return status;
}

void DcoCardAdapter::translateCapabilities(const dcoCardCapa& dcoCap, CardCapability& cap)
{
    // Get Client Capabilities
    for ( int i = 0; i < dcoCap.supported_clients_size(); i++ )
    {
        DcoCard_Capabilities_SupportedClients clientRate = dcoCap.supported_clients(i);
        switch (clientRate)
        {
            case DcoCard_Capabilities::SUPPORTEDCLIENTS_100GB_ELAN:
                cap.supportedClientRate.push_back(PORT_RATE_ETH100G);
                break;
            case DcoCard_Capabilities::SUPPORTEDCLIENTS_400GB_ELAN:
                cap.supportedClientRate.push_back(PORT_RATE_ETH400G);
                break;
            default:
                continue;
        }
    }

    // Get Line Mode Capabilities
    for ( int i = 0; i < dcoCap.supported_line_modes_size(); i++ )
    {

        LineMode mode;

        mode.carrierModeDesignation = dcoCap.supported_line_modes(i).carrier_mode_designation();

        DcoCard_Capabilities_SupportedLineModes_ClientMode clientMode = dcoCap.supported_line_modes(i).supported_line_modes().client_mode();

        switch(clientMode)
        {
            case DcoCard_Capabilities_SupportedLineModes::CLIENTMODE_LXTF20E:
                mode.clientMode = CLIENTMODE_LXTP_E;
                break;
            case DcoCard_Capabilities_SupportedLineModes::CLIENTMODE_LXTF20M:
                mode.clientMode = CLIENTMODE_LXTP_M;
                break;
            case DcoCard_Capabilities_SupportedLineModes::CLIENTMODE_LXTF20:
                mode.clientMode = CLIENTMODE_LXTP;
                break;
            default:
                mode.clientMode = CLIENTMODE_UNSET;
                break;
        }

        mode.dataRate = dcoCap.supported_line_modes(i).supported_line_modes().capacity().value();
        int64 digit = dcoCap.supported_line_modes(i).supported_line_modes().baud_rate().digits();
        uint32 precision = dcoCap.supported_line_modes(i).supported_line_modes().baud_rate().precision();
        mode.baudRate = digit/pow(10, precision);
        getDecimal(mode.baudRate, CARD_BAUD_RATE);
        mode.primaryAppCode = dcoCap.supported_line_modes(i).supported_line_modes().primary_app_code().value();
        mode.compatabilityId = dcoCap.supported_line_modes(i).supported_line_modes().compatability_id().value();
        mode.maxDcoPower = dcoCap.supported_line_modes(i).supported_line_modes().max_dco_power().value();

        ::dcoyang::infinera_dco_card::DcoCard_Capabilities_SupportedLineModes_CarrierModeStatus modeStatus = dcoCap.supported_line_modes(i).supported_line_modes().carrier_mode_status();
        switch(modeStatus)
        {
            case DcoCard_Capabilities_SupportedLineModes::CARRIERMODESTATUS_SUPPORTED:
                mode.carrierModeStatus = CARRIERMODESTATUS_SUPPORTED;
                break;
            case DcoCard_Capabilities_SupportedLineModes::CARRIERMODESTATUS_CANDIDATE:
                mode.carrierModeStatus = CARRIERMODESTATUS_CANDIDATE;
                break;
            case DcoCard_Capabilities_SupportedLineModes::CARRIERMODESTATUS_EXPERIMENTAL:
                mode.carrierModeStatus = CARRIERMODESTATUS_EXPERIMENTAL;
                break;
            case DcoCard_Capabilities_SupportedLineModes::CARRIERMODESTATUS_DEPRECATED:
                mode.carrierModeStatus = CARRIERMODESTATUS_DEPRECATED;
                break;
            default:
                mode.carrierModeStatus = CARRIERMODESTATUS_UNSPECIFIED;
                break;
        }

        cap.supportedLineMode.push_back(mode);
    }

    // Get Advanced Parameters Capabilities
    for ( int i = 0; i < dcoCap.supported_advanced_parameter_size(); i++ )
    {
        SupportedAdvancedParameter ap;

        ap.name = dcoCap.supported_advanced_parameter(i).ap_key();
        ap.dataType = dcoCap.supported_advanced_parameter(i).supported_advanced_parameter().ap_type().value();
        ap.value = dcoCap.supported_advanced_parameter(i).supported_advanced_parameter().supported_values().value();
        ap.description = dcoCap.supported_advanced_parameter(i).supported_advanced_parameter().ap_description().value();

        DcoCard_Capabilities_SupportedAdvancedParameter_ApDirection dir = dcoCap.supported_advanced_parameter(i).supported_advanced_parameter().ap_direction();
        switch(dir)
        {
            case DcoCard_Capabilities_SupportedAdvancedParameter::APDIRECTION_TX:
                ap.dir = ADVPARMDIRECTION_TX;
                break;
            case DcoCard_Capabilities_SupportedAdvancedParameter::APDIRECTION_RX:
                ap.dir = ADVPARMDIRECTION_RX;
                break;
            case DcoCard_Capabilities_SupportedAdvancedParameter::APDIRECTION_BI_DIRECTIONAL:
                ap.dir = ADVPARMDIRECTION_BI_DIR;
                break;
            default:
                ap.dir = ADVPARMDIRECTION_UNSPECIFIED;
                break;
        }
        ap.multiplicity = dcoCap.supported_advanced_parameter(i).supported_advanced_parameter().ap_multiplicity().value();

        DcoCard_Capabilities_SupportedAdvancedParameter_ApConfigurationImpact cfg = dcoCap.supported_advanced_parameter(i).supported_advanced_parameter().ap_configuration_impact();
        switch(cfg)
        {
            case DcoCard_Capabilities_SupportedAdvancedParameter::APCONFIGURATIONIMPACT_NO_CHANGE:
                ap.cfgImpact = ADVPARMCONFIGIMPACT_NO_CHANGE;
                break;
            case DcoCard_Capabilities_SupportedAdvancedParameter::APCONFIGURATIONIMPACT_NO_RE_ACQUIRE:
                ap.cfgImpact = ADVPARMCONFIGIMPACT_NO_RE_ACQUIRE;
                break;
            case DcoCard_Capabilities_SupportedAdvancedParameter::APCONFIGURATIONIMPACT_RE_ACQUIRE:
                ap.cfgImpact = ADVPARMCONFIGIMPACT_RE_ACQUIRE;
                break;
            case DcoCard_Capabilities_SupportedAdvancedParameter::APCONFIGURATIONIMPACT_FULL_CONFIG_PLL_CHANGE:
                ap.cfgImpact = ADVPARMCONFIGIMPACT_FULL_CONFIG_PLL_CHANGE;
                break;
            case DcoCard_Capabilities_SupportedAdvancedParameter::APCONFIGURATIONIMPACT_FULL_CONFIG_NO_PLL_CHANGE:
                ap.cfgImpact = ADVPARMCONFIGIMPACT_FULL_CONFIG_NO_PLL_CHANGE;
                break;
            default:
                ap.cfgImpact = ADVPARMCONFIGIMPACT_UNSPECIFIED;
                break;
        }

        DcoCard_Capabilities_SupportedAdvancedParameter_ApServiceImpact serv = dcoCap.supported_advanced_parameter(i).supported_advanced_parameter().ap_service_impact();
        switch(serv)
        {
            case DcoCard_Capabilities_SupportedAdvancedParameter::APSERVICEIMPACT_SERVICE_EFFECTING:
                ap.servImpact = ADVPARMSERVICEIMPACT_SERVICE_EFFECTING;
                break;
            case DcoCard_Capabilities_SupportedAdvancedParameter::APSERVICEIMPACT_NON_SERVICE_EFFECTING:
                ap.servImpact = ADVPARMSERVICEIMPACT_NON_SERVICE_EFFECTING;
                break;
            default:
                ap.servImpact = ADVPARMSERVICEIMPACT_UNSPECIFIED;
                break;
        }

        cap.supportedAp.push_back(ap);
    }
}

bool DcoCardAdapter::getCardFault()
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = false;
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    DcoCardFault fault;

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&fault);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        DcoCardFault *pFault;
        pFault = static_cast<DcoCardFault*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }
    return true;
}

// Parse the capability tables and fault bitmap to return a list for faultName.
bool DcoCardAdapter::getCardFault(CardFault *fault, bool force)
{
    checkRebootTimer();

    fault->rebootState = mDcoCardRebootState;

    if (mDcoCardRebootState != DpAdapter::DCO_CARD_REBOOT_NONE)
    {
        return true;
    }

    bool status = true;

    if (mFaultCapa.eqpt.empty()
    || (mFaultCapa.dacEqpt.empty())
    || (mFaultCapa.psEqpt.empty())
    || (mFaultCapa.post.empty())
    || (mFaultCapa.dacPost.empty())
    || (mFaultCapa.psPost.empty()))
    {
        INFN_LOG(SeverityLevel::debug) <<"Re-try getCardFaultCapa()"  << endl;
        status = getCardFaultCapa();
        if (status == false)
        {
            INFN_LOG(SeverityLevel::debug) << " Get Dco Card Fault Capabilities  FAIL ";
            return status;
        }
    }

    dcoCard card;
    setKey(&card);

    uint64_t eqptFault, dacEqptFault, psEqptFault, postFault, dacPostFault, psPostFault;
    eqptFault = dacEqptFault = psEqptFault = postFault = dacPostFault = psPostFault = 0ULL;

    fault->eqpt.clear();
    fault->dacEqpt.clear();
    fault->psEqpt.clear();
    fault->post.clear();
    fault->dacPost.clear();
    fault->psPost.clear();

    if (force)
    {
	    status = getCardState(&card);
	    if ( status == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Card State  FAIL "  << endl;
		    return status;
	    }

	    if (card.state().has_equipment_fault())
	    {
		    eqptFault = card.state().equipment_fault().value();
		    mCardEqptFault.faultBitmap = eqptFault;
		    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card eqpt fault 0x" << hex << mCardEqptFault.faultBitmap << dec << endl;
	    }
	    if (card.state().has_dac_equipment_fault())
	    {
		    dacEqptFault = card.state().dac_equipment_fault().value();
		    mCardDacEqptFault.faultBitmap = dacEqptFault;
		    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card dac eqpt fault 0x" << hex << mCardDacEqptFault.faultBitmap << dec << endl;
	    }
	    if (card.state().has_ps_equipment_fault())
	    {
		    psEqptFault = card.state().ps_equipment_fault().value();
		    mCardPsEqptFault.faultBitmap = psEqptFault;
		    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card ps eqpt fault 0x" << hex << mCardPsEqptFault.faultBitmap << dec << endl;
	    }
	    if (card.state().has_post_fault())
	    {
		    postFault = card.state().post_fault().value();
		    mCardPostFault.faultBitmap = postFault;
		    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card post fault 0x" << hex << mCardPostFault.faultBitmap << dec << endl;
	    }
	    if (card.state().has_dac_post_fault())
	    {
		    dacPostFault = card.state().dac_post_fault().value();
		    mCardDacPostFault.faultBitmap = dacPostFault;
		    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card dac post fault 0x" << hex << mCardDacPostFault.faultBitmap << dec << endl;
	    }
	    if (card.state().has_ps_post_fault())
	    {
		    psPostFault = card.state().ps_post_fault().value();
		    mCardPsPostFault.faultBitmap = psPostFault;
		    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card ps post fault 0x" << hex << mCardPsPostFault.faultBitmap << dec << endl;
	    }
	    fault->eqptBmp = eqptFault;
	    fault->dacEqptBmp = dacEqptFault;
	    fault->psEqptBmp = psEqptFault;
	    fault->postBmp = postFault;
	    fault->dacPostBmp = dacPostFault;
	    fault->psPostBmp = psPostFault;
    }
    else
    {
	    fault->eqptBmp = mCardEqptFault.faultBitmap;
	    fault->dacEqptBmp = mCardDacEqptFault.faultBitmap;
	    fault->psEqptBmp = mCardPsEqptFault.faultBitmap;
	    fault->postBmp = mCardPostFault.faultBitmap;
	    fault->dacPostBmp = mCardDacPostFault.faultBitmap;
	    fault->psPostBmp = mCardPsPostFault.faultBitmap;
	    eqptFault = mCardEqptFault.faultBitmap;
	    dacEqptFault = mCardDacEqptFault.faultBitmap;
	    psEqptFault = mCardPsEqptFault.faultBitmap;
	    postFault= mCardPostFault.faultBitmap;
	    dacPostFault = mCardDacPostFault.faultBitmap;
	    psPostFault = mCardPsPostFault.faultBitmap;
    }

    AdapterFault tmpFault;
    for ( auto i = mFaultCapa.eqpt.begin(); i != mFaultCapa.eqpt.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (eqptFault & (0x1ULL << i->bitPos))
        {
            tmpFault.faulted = true;
        }
        fault->eqpt.push_back(tmpFault);
    }

    for ( auto i = mFaultCapa.dacEqpt.begin(); i != mFaultCapa.dacEqpt.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (dacEqptFault & (0x1ULL << i->bitPos))
        {
            tmpFault.faulted = true;
        }
        fault->dacEqpt.push_back(tmpFault);
    }

    for ( auto i = mFaultCapa.psEqpt.begin(); i != mFaultCapa.psEqpt.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (psEqptFault & (0x1ULL << i->bitPos))
        {
            tmpFault.faulted = true;
        }
        fault->psEqpt.push_back(tmpFault);
    }

    for ( auto i = mFaultCapa.post.begin(); i != mFaultCapa.post.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (postFault & (0x1ULL << i->bitPos))
        {
            tmpFault.faulted = true;
        }
        fault->post.push_back(tmpFault);
    }

    for ( auto i = mFaultCapa.dacPost.begin(); i != mFaultCapa.dacPost.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (dacPostFault & (0x1ULL << i->bitPos))
        {
            tmpFault.faulted = true;
        }
        fault->dacPost.push_back(tmpFault);
    }

    for ( auto i = mFaultCapa.psPost.begin(); i != mFaultCapa.psPost.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (psPostFault & (0x1ULL << i->bitPos))
        {
            tmpFault.faulted = true;
        }
        fault->psPost.push_back(tmpFault);
    }
    return status;
}

bool DcoCardAdapter::getCardPm()
{
	INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
	return true;
}

void DcoCardAdapter::subCardFault()
{
	CardFms os;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&os);
	::GnmiClient::AsyncSubCallback* cb = new CardFaultResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " card fault sub request sent to server.."  << endl;
}

void DcoCardAdapter::subCardState()
{
	CardState cs;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&cs);
	::GnmiClient::AsyncSubCallback* cb = new CardStateResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " card state sub request sent to server.."  << endl;
}

void DcoCardAdapter::subCardIsk()
{
	CardIsk ci;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&ci);
	::GnmiClient::AsyncSubCallback* cb = new CardIskResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " card isk sub request sent to server.."  << endl;
}

bool DcoCardAdapter::subCardPm()
{
	if (dp_env::DpEnvSingleton::Instance()->isSimEnv() && mIsGnmiSimModeEn) {
		//spawn a thread to feed up pm data for sim
		if (mSimCardPmThread.joinable() == false) {
			mSimCardPmThread = std::thread(&DcoCardAdapter::SetSimPmData, this);
		}
	}
	else {
		INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
		dcoCardPm cp;
		google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&cp);
		::GnmiClient::AsyncSubCallback* cb = new CardPmResponseContext(); //cleaned up in grpc client
		std::string cbId = mspCrud->Subscribe(msg, cb);
		INFN_LOG(SeverityLevel::info) << " card pm sub request sent to server.."  << endl;
	}
	return true;
}

void DcoCardAdapter::SetSimPmData()
{
	while (mIsGnmiSimModeEn)
	{
		this_thread::sleep_for(seconds(1));
		mCardPm.name = 1;
		mCardPm.SystemState.DspTemperature =  101.1;
		mCardPm.SystemState.PicTemperature =  98.7;
		mCardPm.SystemState.ModuleCaseTemperature =	89.45;
		DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
		dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(mCardPm), &mCardPm);
	}
}

void DcoCardAdapter::SetConnectionState(bool isConnectUp)
{
    std::lock_guard<std::mutex> guard(mRebootStateMutex);

    if (mIsConnectUp != isConnectUp)
    {
        mIsConnectUp = isConnectUp;
        INFN_LOG(SeverityLevel::info) << "mIsConnectUp is updated to: "  << std::boolalpha << mIsConnectUp;

        if (mIsConnectUp == true && mDcoCardRebootState != DCO_CARD_REBOOT_NONE)
        {
            mConnectUpSoakTimer = cDcoConnectUpSoak_sec;
            INFN_LOG(SeverityLevel::info) << "Set soaking mIsConnectUp(TRUE) timer to: "  << cDcoConnectUpSoak_sec << " seconds.";
        }
    }
}

void DcoCardAdapter::SetRebootAlarmAndConnectionState(DcoCardReboot rebootReason, bool isConnectUp)
{
    std::lock_guard<std::mutex> guard(mRebootStateMutex);

    mDcoCardRebootState = rebootReason;
    std::string rebootStr;

    switch(rebootReason)
    {
        case DCO_CARD_REBOOT_WARM:
            rebootStr = "DCO_CARD_REBOOT_WARM";
            mDcoCardRebootTimer = cDcoWarmRebootPeriod_sec;
            break;
        case DCO_CARD_REBOOT_COLD:
            rebootStr = "DCO_CARD_REBOOT_COLD";
            mDcoCardRebootTimer = cDcoColdRebootPeriod_sec;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Invalid input";
            return;
    }

    INFN_LOG(SeverityLevel::info) << "Set " <<  rebootStr << " alarm. Set mDcoCardRebootTimer to " << mDcoCardRebootTimer << " seconds.";

    if (mIsConnectUp != isConnectUp)
    {
        mIsConnectUp = isConnectUp;
        INFN_LOG(SeverityLevel::info) << "mIsConnectUp is updated to: "  << std::boolalpha << mIsConnectUp;

        if (mIsConnectUp == true && mDcoCardRebootState != DCO_CARD_REBOOT_NONE)
        {
            mConnectUpSoakTimer = cDcoConnectUpSoak_sec;
            INFN_LOG(SeverityLevel::info) << "Set soaking mIsConnectUp(TRUE) timer to: "  << cDcoConnectUpSoak_sec << " seconds.";
        }
    }

}

bool DcoCardAdapter::updateNotifyCache()
{
    dcoCard card;
    setKey(&card);
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&card);
    bool status = true;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    if (card.state().has_equipment_fault())
    {
        mCardEqptFault.faultBitmap = card.state().equipment_fault().value();
        INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card eqpt fault 0x" << hex << mCardEqptFault.faultBitmap << dec << endl;
    }
    if (card.state().has_dac_equipment_fault())
    {
	    mCardDacEqptFault.faultBitmap = card.state().dac_equipment_fault().value();
	    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card dac eqpt fault 0x" << hex << mCardDacEqptFault.faultBitmap << dec << endl;
    }
    if (card.state().has_ps_equipment_fault())
    {
	    mCardPsEqptFault.faultBitmap = card.state().ps_equipment_fault().value();
	    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card ps eqpt fault 0x" << hex << mCardPsEqptFault.faultBitmap << dec << endl;
    }
    if (card.state().has_post_fault())
    {
	    mCardPostFault.faultBitmap = card.state().post_fault().value();
	    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card post fault 0x" << hex << mCardPostFault.faultBitmap << dec << endl;
    }
    if (card.state().has_dac_post_fault())
    {
	    mCardDacPostFault.faultBitmap = card.state().dac_post_fault().value();
	    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card dac post fault 0x" << hex << mCardDacPostFault.faultBitmap << dec << endl;
    }
    if (card.state().has_ps_post_fault())
    {
	    mCardPsPostFault.faultBitmap = card.state().ps_post_fault().value();
	    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve card ps post fault 0x" << hex << mCardPsPostFault.faultBitmap << dec << endl;
    }
    return status;
}

// ====== DcoCardAdapter private =========

void DcoCardAdapter::setKey (dcoCard *card)
{
	if (card == NULL) return;
    // Set config name
	card->mutable_config()->mutable_name()->set_value(dcoName);
}

bool DcoCardAdapter::getCardState( dcoCard *card)
{
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(card);
    //INFN_LOG(SeverityLevel::info) << __func__ << " \n\n ****Read ... \n"  << endl;
	bool status = true;
    // For now no retry
    int count = 1;
    // Wait unit DCO init card state
    while (card->has_state() == false)
    {
        std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

        grpc::Status gStatus = mspCrud->Read( msg );
        if (gStatus.ok())
        {
            status = true;
            card = static_cast<dcoCard*>(msg);
            //INFN_LOG(SeverityLevel::info) << "card enable: " << card->config().enable().value() << std::endl;
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
            status = false;
            return status;
        }

        if (card->has_state() == false)
        {
            if (--count <=0)
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " Card State not ready .... QUIT "  << endl;
                // status = false;
                // TODO return true until we can create state object
                status = true;
                break;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " Wait for Card State to be ready .... WAIT "  << endl;
                this_thread::sleep_for(seconds(2));
            }
        }
    }
    return status;
}

bool DcoCardAdapter::setCardConfig( dcoCard *card )
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(card);
	mspCrud->Update( msg );
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
	// this_thread::sleep_for(seconds(2));
    return true;
}

void DcoCardAdapter::setSimModeEn(bool enable)
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    if (enable)
    {
        mspCrud = mspSimCrud;
        mIsGnmiSimModeEn = true;
        bool status = getCardFaultCapa();
        if (status == false)
        {
            INFN_LOG(SeverityLevel::info) << "Read SIM DCO fault failed!! "  << endl;
        }
    }
    else
    {
        mspCrud = mspSbCrud;
        mIsGnmiSimModeEn = false;
        bool status = getCardFaultCapa();
        if (status == false)
        {
            INFN_LOG(SeverityLevel::info) << "Read HW DCO fault failed!! "  << endl;
        }
    }
}

bool DcoCardAdapter::getCardFaultCapa()
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = false;
    DcoCardFault fault;
    DcoCardFault *pFault;

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&fault);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        pFault = static_cast<DcoCardFault*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::debug) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }
    mFaultCapa.eqpt.clear();
    mFaultCapa.dacEqpt.clear();
    mFaultCapa.psEqpt.clear();
    mFaultCapa.post.clear();
    mFaultCapa.dacPost.clear();
    mFaultCapa.psPost.clear();

    for ( int i = 0; i < pFault->capabilities().equipment_faults_size(); i++ )
    {
        FaultCapability fault;
        DcoCardFault_Capabilities_EquipmentFaultsKey key;

        key = pFault->capabilities().equipment_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.equipment_faults().fault_name().value();
        if (key.equipment_faults().has_service_affecting())
        {
            fault.serviceAffecting = key.equipment_faults().service_affecting().value();
        }
        else
        {
            fault.serviceAffecting = false;
        }
        if (key.equipment_faults().has_bit_pos())
        {
            fault.bitPos = key.equipment_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.equipment_faults().has_fault_description())
        {
            fault.faultDescription = key.equipment_faults().fault_description().value();
        }
        using eqptSeverity = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.equipment_faults().severity())
        {
            case eqptSeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case eqptSeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case eqptSeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.equipment_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.equipment_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }

        mFaultCapa.eqpt.push_back(fault);
    }

    for ( int i = 0; i < pFault->capabilities().dac_equipment_faults_size(); i++ )
    {
        FaultCapability fault;
        DcoCardFault_Capabilities_DacEquipmentFaultsKey key;

        key = pFault->capabilities().dac_equipment_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.dac_equipment_faults().fault_name().value();
        if (key.dac_equipment_faults().has_service_affecting())
        {
            fault.serviceAffecting = key.dac_equipment_faults().service_affecting().value();
        }
        else
        {
            fault.serviceAffecting = false;
        }
        if (key.dac_equipment_faults().has_bit_pos())
        {
            fault.bitPos = key.dac_equipment_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.dac_equipment_faults().has_fault_description())
        {
            fault.faultDescription = key.dac_equipment_faults().fault_description().value();
        }
        using eqptSeverity = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.dac_equipment_faults().severity())
        {
            case eqptSeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case eqptSeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case eqptSeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.dac_equipment_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.dac_equipment_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }

        mFaultCapa.dacEqpt.push_back(fault);
    }

    for ( int i = 0; i < pFault->capabilities().ps_equipment_faults_size(); i++ )
    {
        FaultCapability fault;
        DcoCardFault_Capabilities_PsEquipmentFaultsKey key;

        key = pFault->capabilities().ps_equipment_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.ps_equipment_faults().fault_name().value();
        if (key.ps_equipment_faults().has_service_affecting())
        {
            fault.serviceAffecting = key.ps_equipment_faults().service_affecting().value();
        }
        else
        {
            fault.serviceAffecting = false;
        }
        if (key.ps_equipment_faults().has_bit_pos())
        {
            fault.bitPos = key.ps_equipment_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.ps_equipment_faults().has_fault_description())
        {
            fault.faultDescription = key.ps_equipment_faults().fault_description().value();
        }
        using eqptSeverity = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.ps_equipment_faults().severity())
        {
            case eqptSeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case eqptSeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case eqptSeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.ps_equipment_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;
        switch( key.ps_equipment_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }

        mFaultCapa.psEqpt.push_back(fault);
    }

    for ( int i = 0; i < pFault->capabilities().post_faults_size(); i++ )
    {
        FaultCapability fault;
        fault.serviceAffecting = false;
        DcoCardFault_Capabilities_PostFaultsKey key;

        key = pFault->capabilities().post_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.post_faults().fault_name().value();

        if (key.post_faults().has_bit_pos())
        {
            fault.bitPos = key.post_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.post_faults().has_fault_description())
        {
            fault.faultDescription = key.post_faults().fault_description().value();
        }

        using postSeverity = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.post_faults().severity())
        {
            case postSeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case postSeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case postSeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.post_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.post_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }
        mFaultCapa.post.push_back(fault);
    }

    for ( int i = 0; i < pFault->capabilities().dac_post_faults_size(); i++ )
    {
        FaultCapability fault;
        fault.serviceAffecting = false;
        DcoCardFault_Capabilities_DacPostFaultsKey key;

        key = pFault->capabilities().dac_post_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.dac_post_faults().fault_name().value();

        if (key.dac_post_faults().has_bit_pos())
        {
            fault.bitPos = key.dac_post_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.dac_post_faults().has_fault_description())
        {
            fault.faultDescription = key.dac_post_faults().fault_description().value();
        }

        using postSeverity = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.dac_post_faults().severity())
        {
            case postSeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case postSeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case postSeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.dac_post_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.dac_post_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }
        mFaultCapa.dacPost.push_back(fault);
    }

    for ( int i = 0; i < pFault->capabilities().ps_post_faults_size(); i++ )
    {
        FaultCapability fault;
        fault.serviceAffecting = false;
        DcoCardFault_Capabilities_PsPostFaultsKey key;

        key = pFault->capabilities().ps_post_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.ps_post_faults().fault_name().value();

        if (key.ps_post_faults().has_bit_pos())
        {
            fault.bitPos = key.ps_post_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.ps_post_faults().has_fault_description())
        {
            fault.faultDescription = key.ps_post_faults().fault_description().value();
        }

        using postSeverity = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.ps_post_faults().severity())
        {
            case postSeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case postSeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case postSeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.ps_post_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;
        switch( key.ps_post_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }
        mFaultCapa.psPost.push_back(fault);
    }

    return true;
}

void DcoCardAdapter::cardNotifyState(ostream& out)
{
    // Dump cached card status, set force = false to fecth cached CardStatus
    // from Card State notify and Card ISK notify updates
    out << "\n ***Dump Cached CardStatus from Notify: \n" <<  endl;
    bool status = cardStatusInfo(out, false);
}

bool DcoCardAdapter::addIsk(string& keyPath)
{
    INFN_LOG(SeverityLevel::info) << __func__ << " keyPath=" << keyPath;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    grpc::Status gStatus;
    google::protobuf::Message* msg;

    {
        // use scope on DCO card as we dont want to send same cardAction
        // again when we send the key as DCO will act on the same action
        // again.
        dcoCard card;
        setKey(&card);

        card.mutable_action()->set_firmware_upgrade(cardAction::FIRMWAREUPGRADE_CLEARUGSTATE);
        msg = static_cast<google::protobuf::Message*>(&card);
        gStatus = mspCrud->Update(msg);

        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Update  FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
            return false;
        }
    }
    // DO NOT remove this delay it is intentional to allow DCO to have enough
    // time to process the previous command
    // Corverity may complain this code is intentional
    this_thread::sleep_for(seconds(1));

    {
        dcoCard card;
        setKey(&card);

        card.mutable_action()->mutable_isk_path()->set_value(keyPath);
        msg = static_cast<google::protobuf::Message*>(&card);
        gStatus = mspCrud->Update(msg);

        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Update  FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
            return false;
        }
        card.mutable_action()->set_firmware_upgrade(cardAction::FIRMWAREUPGRADE_ISK_ADD);
        msg = static_cast<google::protobuf::Message*>(&card);
        gStatus = mspCrud->Update(msg);

        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Update  FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
            return false;
        }
        else
        {
            return true;
        }
    }
}

bool DcoCardAdapter::removeIsk(string& keyName)
{
    INFN_LOG(SeverityLevel::info) << __func__ << " " << keyName;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    grpc::Status gStatus;
    google::protobuf::Message* msg;

    {
        // use scope on DCO card as we dont want to send same cardAction
        // again when we send the key as DCO will act on the same action
        // again.
        dcoCard card;
        setKey(&card);

        card.mutable_action()->set_firmware_upgrade(cardAction::FIRMWAREUPGRADE_CLEARUGSTATE);
        msg = static_cast<google::protobuf::Message*>(&card);
        gStatus = mspCrud->Update(msg);

        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Update  FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
            return false;
        }
    }

    // DO NOT remove this delay it is intentional to allow DCO to have enough
    // time to process the previous command
    // Corverity may complain this code is intentional
    this_thread::sleep_for(seconds(1));

    {
        dcoCard card;
        setKey(&card);

        card.mutable_action()->mutable_key_name()->set_value(keyName);
        msg = static_cast<google::protobuf::Message*>(&card);
        gStatus = mspCrud->Update(msg);

        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Update  FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
            return false;
        }

        card.mutable_action()->set_firmware_upgrade(cardAction::FIRMWAREUPGRADE_ISK_REMOVE);
        msg = static_cast<google::protobuf::Message*>(&card);
        gStatus = mspCrud->Update(msg);


        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Update  FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
            return false;
        }
        else
        {
            return true;
        }
    }

}

bool DcoCardAdapter::getCardStateObj( dcoCardStat *state)
{
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(state);
    //INFN_LOG(SeverityLevel::info) << __func__ << " \n\n ****Read ... \n"  << endl;
	bool status = true;
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
            status = true;
            state = static_cast<dcoCardStat*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }
    return status;
}

void DcoCardAdapter::dumpDcoFaultCapability(std::ostringstream& out, vector<DpAdapter::FaultCapability>& fltCapa)
{
    static const std::string severityStr[] = {
            "UNSPECIFIED",
            "Degraded",
            "Critical",
            "Failure"
    };

    static const std::string directionStr[] = {
            "UNSPECIFIED",
            "RX",
            "TX",
            "NA"
    };

    static const std::string locationStr[] = {
            "UNSPECIFIED",
            "Near_end",
            "Far_end",
            "NA"
    };

    boost::format fmter("%-44s %-38s %-12s %-6d %-6b %-12s %-12s %-60s");
    out << fmter
        % "faultKey"
        % "faultName"
        % "severity"
        % "bitPos"
        % "SA"
        % "direction"
        % "location"
        % "faultDescription"
       << endl << endl;

    for ( auto i = fltCapa.begin(); i != fltCapa.end(); i++)
    {
        out << fmter
            % i->faultKey
            % i->faultName
            % severityStr[i->severity]
            % i->bitPos
            % i->serviceAffecting
            % directionStr[i->direction]
            % locationStr[i->location]
            % i->faultDescription
            << endl;
    }
}

void DcoCardAdapter::setRebootAlarm(DcoCardReboot rebootReason)
{
    std::lock_guard<std::mutex> guard(mRebootStateMutex);

    mDcoCardRebootState = rebootReason;
    std::string rebootStr;

    switch(rebootReason)
    {
        case DCO_CARD_REBOOT_WARM:
            rebootStr = "DCO_CARD_REBOOT_WARM";
            mDcoCardRebootTimer = cDcoWarmRebootPeriod_sec;
            break;
        case DCO_CARD_REBOOT_COLD:
            rebootStr = "DCO_CARD_REBOOT_COLD";
            mDcoCardRebootTimer = cDcoColdRebootPeriod_sec;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Invalid input";
            return;
    }

    INFN_LOG(SeverityLevel::info) << "Set " <<  rebootStr << " alarm. Set mDcoCardRebootTimer to " << mDcoCardRebootTimer << " seconds.";
}

void DcoCardAdapter::checkRebootTimer()
{
    std::lock_guard<std::mutex> guard(mRebootStateMutex);

    if (mDcoCardRebootState != DCO_CARD_REBOOT_NONE)
    {
        if (mDcoCardRebootTimer != 0)
        {
            //debug print
            if (mDcoCardRebootTimer % 30 == 0)
            {
                INFN_LOG(SeverityLevel::info) << "mDcoCardRebootTimer is " << mDcoCardRebootTimer << " now.";
            }

            mDcoCardRebootTimer --;
        }
        else
        {
            mDcoCardRebootState = DCO_CARD_REBOOT_NONE;
            INFN_LOG(SeverityLevel::info) << "Clear DCO reboot alarm because reboot timer expired. mIsConnectUp is " << std::boolalpha << mIsConnectUp;
            return;
        }

        if (mIsConnectUp == true)
        {
            if ( mConnectUpSoakTimer != 0)
            {
                mConnectUpSoakTimer--;
            }
            else
            {
                mDcoCardRebootState = DCO_CARD_REBOOT_NONE;
                INFN_LOG(SeverityLevel::info) << "Clear DCO reboot alarm because DCO connection is up. mDcoCardRebootTimer is " << mDcoCardRebootTimer;
                mDcoCardRebootTimer = 0;
            }
        }
    }
}

void DcoCardAdapter::clearFltNotifyFlag()
{
    INFN_LOG(SeverityLevel::info) << __func__ << ", clear notify flag" << endl;
    mCardEqptFault.isNotified = false;
    mCardDacEqptFault.isNotified = false;
    mCardPsEqptFault.isNotified = false;
    mCardPostFault.isNotified = false;
    mCardDacPostFault.isNotified = false;
    mCardPsPostFault.isNotified = false;
}

} /* DpAdapter */
