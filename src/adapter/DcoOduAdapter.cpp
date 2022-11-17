
/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>

#include <google/protobuf/text_format.h>
#include "DcoOduAdapter.h"
#include "DcoCardAdapter.h"
#include "DpOduMgr.h"
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
using namespace dcoyang::infinera_dco_odu;
using namespace dcoyang::infinera_dco_odu_fault;

using oduConfig = Odus_Odu_Config;
using loopBack = Odus_Odu_Config_LoopbackType;
using ms = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal;
using prbs = ::dcoyang::enums::InfineraDcoOduPrbsMode;

namespace DpAdapter {

std::recursive_mutex DcoOduAdapter::mMutex;
DpMsODUPm DcoOduAdapter::mOduPms;
string    DcoOduAdapter::mOduIdToAid[MAX_ODU_ID+1];
AdapterCacheFault  DcoOduAdapter::mOduFault[MAX_ODU_ID];
const std::vector<uint32_t> DcoOduAdapter::mClientOduTimeSlot = {1, 2};
AdapterCacheTTI   DcoOduAdapter::mTtiRx[MAX_ODU_ID];
const std::string DcoOduAdapter::DCO_ODU_SAPI_MISM_NAME_STR = "SAPI-MISMATCH";
uint8_t           DcoOduAdapter::mCdi[MAX_ODU_ID];
AdapterCacheOpStatus DcoOduAdapter::mOpStatus[MAX_ODU_ID];

high_resolution_clock::time_point DcoOduAdapter::mStart;
high_resolution_clock::time_point DcoOduAdapter::mEnd;
int64_t DcoOduAdapter::mStartSpan;
int64_t DcoOduAdapter::mEndSpan;
int64_t DcoOduAdapter::mDelta;
std::deque<int64_t> DcoOduAdapter::mStartSpanArray(spanTime);
std::deque<int64_t> DcoOduAdapter::mEndSpanArray(spanTime);
std::deque<int64_t> DcoOduAdapter::mDeltaArray(spanTime);
bool DcoOduAdapter::mOduStateSubEnable = false;
bool DcoOduAdapter::mOduFltSubEnable   = false;
bool DcoOduAdapter::mOduPmSubEnable    = false;
bool DcoOduAdapter::mOduOpStatusSubEnable = false;
unordered_map<string, vector<std::time_t>> DcoOduAdapter::ebOverflowMap;


DcoOduAdapter::DcoOduAdapter()
: mRxSapiMismBitmap(0)
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
    memset(mOduPayLoad, 0, sizeof (mOduPayLoad));
    memset(mbBdiInject, 0, sizeof (mbBdiInject));

    for (int i = 0; i < MAX_ODU_ID; i++)
    {
        mTtiRx[i].ClearTTICache();
    }
}

DcoOduAdapter::~DcoOduAdapter()
 {

 }

// To create client internally in DCO to use for Xpress Xcon.  The client will
// be in TERM loopback.
bool DcoOduAdapter::createOduClientXpress(string aid)
{
    OduCommon cfg;

    cfg.regen = true;
    // We set term loopback on OTU4 client for the regen case.
    cfg.lpbk = LOOP_BACK_TYPE_OFF;
    // Rest is default settings
    cfg.type = ODUSUBTYPE_CLIENT;
    cfg.payLoad = ODUPAYLOAD_ODU4;
    cfg.generatorRx = PRBSMODE_PRBS_NONE;
    cfg.generatorTx = PRBSMODE_PRBS_NONE;
    cfg.monitorTx = PRBSMODE_PRBS_NONE;
    cfg.monitorRx = PRBSMODE_PRBS_NONE;
    cfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.srvMode = SERVICEMODE_SWITCHING;
    // TimeSlot is not used for ODU4 Client, set some default ts
    cfg.tS.push_back(1);
    cfg.tS.push_back(2);

    memset(&cfg.ttiTx[0], 0, sizeof(cfg.ttiTx) );
    // Set transactionId to wait for DCO ack.
    cfg.transactionId = getTimeNanos();

    INFN_LOG(SeverityLevel::info) << __func__ << " XPress case: Create ODU4 Client: " << " aid=" << aid << " transId: 0x" << hex << cfg.transactionId <<  dec << endl;

    if ( createOdu (aid, &cfg) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Fail Xpress create ODU4 Client  aid: " << aid << '\n';
        return false;
    }

    return true;
}

bool DcoOduAdapter::createOdu(string aid, OduCommon *cfg)
{
	bool status = false;

#ifndef NEW_ODU_AID_FORMAT
    status = cacheOduAid(aid, cfg);

    if (status == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad AID: " << aid << '\n';
        return false;
    }
#endif

    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

#ifdef NEW_ODU_AID_FORMAT
    status = cacheOduAid(aid, oduId);

    if (status == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad AID: " << aid << '\n';
        return false;
    }
#endif
    INFN_LOG(SeverityLevel::info) << __func__ << " Create oduId: " << oduId << ", aid: " << aid << " transId: 0x" << hex << cfg->transactionId << dec << endl;

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    int hoOtuId = aidToHoOtuId(aid);
    INFN_LOG(SeverityLevel::info) << __func__ << " hoOtuId: " << hoOtuId << endl;
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->mutable_otu_id()->set_value(hoOtuId);

    odu.mutable_odu(0)->mutable_odu()->mutable_config()->mutable_name()->set_value(aid);
    // Set to default
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->mutable_fwd_tda_trigger()->set_value(false);
    //TODO: LHONG, for client odu, ts granularity is 50G
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_time_slot_granularity(oduConfig::TIMESLOTGRANULARITY_25G);
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->mutable_regen()->set_value(cfg->regen);
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->mutable_tid()->set_value(cfg->transactionId);

    switch (cfg->type)
    {
        case ODUSUBTYPE_LINE:
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_sub_type(oduConfig::SUBTYPE_LINE);
            break;
        case ODUSUBTYPE_CLIENT:
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_sub_type(oduConfig::SUBTYPE_CLIENT);
            cfg->tS = mClientOduTimeSlot;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Error Unknown SubType " << cfg->type <<  endl;
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_sub_type(oduConfig::SUBTYPE_LINE);
            break;
    }

    // TODO: Derive MSI pay load type from pay load type, as we only have
    //  two MSI payload.  FOR R5.0 we need to revisit this as we have OTU4
    // and ODU4
    switch (cfg->payLoad)
    {
        case ODUPAYLOAD_ODU4:
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_payload_type(oduConfig::PAYLOADTYPE_ODU4);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_interface_type(oduConfig::INTERFACETYPE_INTF_OTL_4_2);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_msi_payload_type(oduConfig::MSIPAYLOADTYPE_ODU4);
            break;
        case ODUPAYLOAD_LO_ODU4I:
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_payload_type(oduConfig::PAYLOADTYPE_LO_ODU4I);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_interface_type(oduConfig::INTERFACETYPE_INTF_GAUI);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_msi_payload_type(oduConfig::MSIPAYLOADTYPE_ODU4I_100GE);
            break;
        case ODUPAYLOAD_LO_ODU4:
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_payload_type(oduConfig::PAYLOADTYPE_LO_ODU4I);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_interface_type(oduConfig::INTERFACETYPE_INTF_GAUI);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_msi_payload_type(oduConfig::MSIPAYLOADTYPE_ODU4);
            break;
        case ODUPAYLOAD_LO_FLEXI:
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_payload_type(oduConfig::PAYLOADTYPE_LO_FLEXI);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_interface_type(oduConfig::INTERFACETYPE_INTF_GAUI);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_msi_payload_type(oduConfig::MSIPAYLOADTYPE_ODUFLEXI_400GE);
            break;
        case ODUPAYLOAD_HO_ODUCNI:
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_payload_type(oduConfig::PAYLOADTYPE_ODUCNI);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_interface_type(oduConfig::INTERFACETYPE_INTF_GAUI);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_msi_payload_type(oduConfig::MSIPAYLOADTYPE_UNALLOCATED);
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Error Unknown PayLoad " << cfg->payLoad <<  endl;
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_payload_type(oduConfig::PAYLOADTYPE_LO_ODU4I);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_interface_type(oduConfig::INTERFACETYPE_INTF_GAUI);
            odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_msi_payload_type(oduConfig::MSIPAYLOADTYPE_ODU4I_100GE);
            break;
    }

    loopBack lpbk;
    switch(cfg->lpbk)
    {
    case LOOP_BACK_TYPE_FACILITY:
        lpbk = oduConfig::LOOPBACKTYPE_FACILITY;
        break;
    case LOOP_BACK_TYPE_TERMINAL:
        lpbk = oduConfig::LOOPBACKTYPE_TERMINAL;
        break;
    case LOOP_BACK_TYPE_OFF:
    default:
        lpbk = oduConfig::LOOPBACKTYPE_NONE;
        break;
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_loopback_type(lpbk);

    Odus_Odu_Config_PayloadType payLoad;
    switch (cfg->payLoad)
    {
        case ODUPAYLOAD_ODU4:
            payLoad = oduConfig::PAYLOADTYPE_ODU4;
            break;
        case ODUPAYLOAD_LO_ODU4I:
        case ODUPAYLOAD_LO_ODU4:
            payLoad = oduConfig::PAYLOADTYPE_LO_ODU4I;
            break;
        case ODUPAYLOAD_LO_FLEXI:
            payLoad = oduConfig::PAYLOADTYPE_LO_FLEXI;
            break;
        case ODUPAYLOAD_HO_ODUCNI:
            payLoad = oduConfig::PAYLOADTYPE_ODUCNI;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << " Unknown PAYLOAD. Not set FAIL: "  << cfg->payLoad << endl;
            return false;
    }
    mOduPayLoad[oduId] = cfg->payLoad;  // Cache to return frame rate

    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_payload_type(payLoad);

    ms signal;

    if ((PRBSMODE_PRBS_X31 == cfg->generatorTx) || (PRBSMODE_PRBS_X31_INV == cfg->generatorTx))
    {
        signal = (PRBSMODE_PRBS_X31 == cfg->generatorTx) ? ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31 : ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
    }
    else
    {
        switch(cfg->forceMsTx)
        {
            case MAINTENANCE_SIGNAL_LASEROFF:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case MAINTENANCE_SIGNAL_OCI:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
                break;
            case MAINTENANCE_SIGNAL_AIS:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
                break;
            case MAINTENANCE_SIGNAL_LCK:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
                break;
            case MAINTENANCE_SIGNAL_NOREPLACE:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
            default:
                INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_force_signal_tx(signal);

    if ((PRBSMODE_PRBS_X31 == cfg->generatorRx) || (PRBSMODE_PRBS_X31_INV == cfg->generatorRx))
    {
        signal = (PRBSMODE_PRBS_X31 == cfg->generatorRx) ? ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31 : ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
    }
    else
    {
        switch(cfg->forceMsRx)
        {
            case MAINTENANCE_SIGNAL_LASEROFF:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case MAINTENANCE_SIGNAL_OCI:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
                break;
            case MAINTENANCE_SIGNAL_AIS:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
                break;
            case MAINTENANCE_SIGNAL_LCK:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
                break;
            case MAINTENANCE_SIGNAL_NOREPLACE:
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
            default:
                INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
                signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_force_signal_rx(signal);

    switch(cfg->autoMsTx)
    {
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
            break;
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_auto_signal_tx(signal);

    switch(cfg->autoMsRx)
    {
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
            break;
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_auto_signal_rx(signal);

    Odus_Odu_Config_ServiceMode sMode;
    switch(cfg->srvMode)
    {
        case SERVICEMODE_SWITCHING:
            sMode = oduConfig::SERVICEMODE_MODE_SWITCHING;
            break;
        case SERVICEMODE_TRANSPORT:
            sMode = oduConfig::SERVICEMODE_MODE_TRANSPORT;
            break;
        case SERVICEMODE_NONE:
        case SERVICEMODE_ADAPTATION:
            sMode = oduConfig::SERVICEMODE_MODE_NONE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set ServiceMode default: " << cfg->srvMode << endl;
            sMode = oduConfig::SERVICEMODE_MODE_NONE;
            break;
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_service_mode(sMode);

    for (auto i = cfg->tS.begin(); i != cfg->tS.end(); i++)
    {
        ::ywrapper::UintValue* ts = odu.mutable_odu(0)->mutable_odu()->mutable_config()->add_time_slot_id();
        ts->set_value(*i);
    }

    prbs prbsMode;

    switch (cfg->monitorTx)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            cout << __func__ << " Warning unknown set PrbsMonTx default" << endl;
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_NONE;
            break;
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_prbs_mon_tx(prbsMode);

    switch (cfg->monitorRx)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            cout << __func__ << " Warning unknown set PrbsMonRx default" << endl;
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_NONE;
            break;
    }
    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_prbs_mon_rx(prbsMode);

    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        Odus_Odu_Config_TxTtiKey *ttiKey = odu.mutable_odu(0)->mutable_odu()->mutable_config()->add_tx_tti();
        ttiKey->set_tti_byte_index(i+1);
        ttiKey->mutable_tx_tti()->mutable_tti()->set_value(cfg->ttiTx[i]);
    }
    // odu.mutable_odu(0)->mutable_odu()->mutable_config()->mutable_rx_fault_status()->set_value(cfg->injectRxFaultStatus);

    // Clear Fault cache
    mOduFault[oduId - 1].ClearFaultCache();

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
    grpc::Status gStatus = mspCrud->Create( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Create FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }

    mOduPms.AddPmData(aid, oduId);

    INFN_LOG(SeverityLevel::info) << "Clear ODU TIM BDI cache for AID: " << aid;
    mbBdiInject[oduId] = false;

    // One Xpress XCON command from host would create 10 - 12 commands
    // to DCO.  DCO could not keep up, to throtle the Xpress Xcon commands.
    // status = false or TransactionId = 0 ==> don't wait
    if ( status == true && mOpStatus[oduId-1].waitForDcoAck(cfg->transactionId) == false)
    {
        return false;
    }
    return status;
}

bool DcoOduAdapter::deleteOdu(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Delete oduId: " << oduId << endl;

    Odu odu;
    //only set id in Odu level, do not set id in Odu config level for Odu obj deletion
    odu.add_odu()->set_odu_id(oduId);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }

    mOduPms.RemovePmData(oduId);
    // Clear Aid cache
    mOduIdToAid[oduId].clear();
    // Clear Fault cache
    mOduFault[oduId - 1].ClearFaultCache();
    // Clear TTI cache
    mTtiRx[oduId - 1].ClearTTICache();
    mOpStatus[oduId - 1].ClearOpStatusCache();

    mCdi[oduId - 1] = 0;

    mOduPayLoad[oduId] = ODUPAYLOAD_UNSPECIFIED;
    mbBdiInject[oduId] = false;
    return status;
}

bool DcoOduAdapter::initializeOdu()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    bool status = false;

    for ( int i=0; i <= MAX_ODU_ID ; i++)
    {
        mOduIdToAid[i].clear();
    }
    // Get ODU fault capabilities
    status = getOduFaultCapa();

    subscribeStreams();

    return status;
}

void DcoOduAdapter::subscribeStreams()
{
    clearNotifyFlag();

    //subscribe to odu pm
    if(!mOduPmSubEnable)
    {
        subOduPm();
        mOduPmSubEnable = true;
    }

    //subscribe to odu fault and state
    if(!mOduFltSubEnable)
    {
        subOduFault();
        mOduFltSubEnable = true;
    }
    if(!mOduStateSubEnable)
    {
        subOduState();
        mOduStateSubEnable = true;
    }
    //subscribe to odu op status
    if(!mOduOpStatusSubEnable)
    {
        subOduOpStatus();
        mOduOpStatusSubEnable = true;
    }
}

bool DcoOduAdapter::setOduLoopBack(string aid, LoopBackType mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " LoopBackType :  " << (int) mode  << " oduId: " << oduId << endl;

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    loopBack lpbk;

    switch(mode)
    {
    case LOOP_BACK_TYPE_FACILITY:
        lpbk = oduConfig::LOOPBACKTYPE_FACILITY;
        break;
    case LOOP_BACK_TYPE_TERMINAL:
        lpbk = oduConfig::LOOPBACKTYPE_TERMINAL;
        break;
    case LOOP_BACK_TYPE_OFF:
    default:
        lpbk = oduConfig::LOOPBACKTYPE_NONE;
        break;
    }

    odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_loopback_type(lpbk);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
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

bool DcoOduAdapter::setOduTimeSlot(string aid, std::vector<uint32_t> tS)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    if ( tS.size() == 0 || (tS.size() != MAX_ODU4_TS && tS.size() != MAX_ODUFLEX_TS) )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " Bad TS size: " << tS.size() << endl;
        return false;
    }

    // Fetch the current ODU config
    OduCommon cfg;
    memset(&cfg, 0, sizeof(cfg));
    if ( getOduConfig(aid, &cfg) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " Fetch current ODU config failed " << endl;
        return false;
    }

    // Modify TS in current config
    INFN_LOG(SeverityLevel::info) << __func__ << " oduId: " << oduId << " TS: ";
    bool match = true;
    for ( auto i = tS.begin(), j = cfg.tS.begin(); i != tS.end(); i++, j++)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " " << *i;
        if ( *i != *j )
        {
            match = false;
        }
        *j = *i;
    }

    // If time slot the same skip modify operation
    if (match == true)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " oduId: " << oduId << " Same TS SKIP modify  " << endl;
        return status;
    }

    // Delete the current ODU
    if (deleteOdu(aid) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " delete Current ODU failed " << endl;
        return false;
    }

    cfg.loOduId = oduId;
    // Create new ODU with new time slot
    if (createOdu(aid, &cfg) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " create new ODU w new TS failed " << endl;
        return false;
    }
    return status;
}

bool DcoOduAdapter::setOduAutoMS(string aid, Direction dir, MaintenanceSignal mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " oduId: " << oduId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MS :  " << (int) mode  << " oduId: " << oduId << endl;

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }
    ms signal;

    switch (mode)
    {
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " oduId: " << oduId << '\n';
            return false;
    }

    if (dir == DIRECTION_TX)
    {
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_auto_signal_tx(signal);
    }
    else
    {
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_auto_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
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

bool DcoOduAdapter::setOduForceMS(string aid, Direction dir, MaintenanceSignal mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " oduId: " << oduId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MS :  " << (int) mode  << " oduId: " << oduId << endl;

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    MaintenanceSignal msMode = mode;

    PrbsMode prbsMode;
    DpOduMgrSingleton::Instance()->getMspCfgHndlr()->getChm6OduConfigPrbsGenFromCache(aid, dir, prbsMode);

    if ((MAINTENANCE_SIGNAL_LCK != mode) &&
        ((PRBSMODE_PRBS_X31 == prbsMode) || (PRBSMODE_PRBS_X31_INV == prbsMode)))
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Switch " << EnumTranslate::toString(dir) << " ForceMs due to Prbs Gen enabled for oduId: " << oduId << endl;
        msMode = (PRBSMODE_PRBS_X31 == prbsMode) ? MAINTENANCE_SIGNAL_PRBS_X31 : MAINTENANCE_SIGNAL_PRBS_X31_INV;
    }

    ms signal;

    switch (msMode)
    {
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " oduId: " << oduId << '\n';
            return false;
    }
    if (dir == DIRECTION_TX)
    {
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_force_signal_tx(signal);
    }
    else
    {
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_force_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
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

bool DcoOduAdapter::setOduPrbsMon(string aid, Direction dir, PrbsMode mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " oduId: " << oduId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " mode :  " << EnumTranslate::toString(mode) << ", dir: " << (int)dir << " oduId: " << oduId << endl;

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }
    prbs prbsMode;

    switch (mode)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOODUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " oduId: " << oduId << '\n';
            return false;
    }

    if (dir == DIRECTION_TX)
    {
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_prbs_mon_tx(prbsMode);
    }
    else
    {
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_prbs_mon_rx(prbsMode);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
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

bool DcoOduAdapter::setOduPrbsGen(string aid, Direction dir, PrbsMode mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " oduId: " << oduId << '\n';
        return false;
    }
    MaintenanceSignal msMode;
    DpOduMgrSingleton::Instance()->getMspCfgHndlr()->getChm6OduConfigForceMsFromCache(aid, dir, msMode);
    if (MAINTENANCE_SIGNAL_LCK == msMode)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " " << EnumTranslate::toString(dir) << " Prbs Gen not set due to LCK for oduId: " << oduId << endl;
        return true;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " mode :  " << EnumTranslate::toString(mode) << ", dir: " << (int)dir << " oduId: " << oduId << endl;

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }
    ms signal;

    switch (mode)
    {
        case PRBSMODE_PRBS_NONE:
            switch(msMode)
            {
                case MAINTENANCE_SIGNAL_LASEROFF:
                    signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
                    break;
                case MAINTENANCE_SIGNAL_OCI:
                    signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
                    break;
                case MAINTENANCE_SIGNAL_AIS:
                    signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
                    break;
                case MAINTENANCE_SIGNAL_NOREPLACE:
                    signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                    break;
                default:
                    INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
                    signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                    break;
            }
            INFN_LOG(SeverityLevel::info) << __func__ << " " << EnumTranslate::toString(dir) << " ForceMs set " << EnumTranslate::toString(msMode) << " due to PRBS Gen disabled for oduId: " << oduId << endl;
            break;
        case PRBSMODE_PRBS_X31:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            signal = ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " oduId: " << oduId << '\n';
            return false;
    }

    if (dir == DIRECTION_TX)
    {
	//per FW folks, prbs-gen-tx is Deprecated, should use ms-force-signal-tx
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_force_signal_tx(signal);
    }
    else
    {
	//per FW folks, prbs-gen-rx is Deprecated, should use ms-force-signal-rx
        odu.mutable_odu(0)->mutable_odu()->mutable_config()->set_ms_force_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
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

bool DcoOduAdapter::setOduTti(string aid, uint8_t *tti)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        Odus_Odu_Config_TxTtiKey *ttiKey = odu.mutable_odu(0)->mutable_odu()->mutable_config()->add_tx_tti();
        ttiKey->set_tti_byte_index(i+1);
        ttiKey->mutable_tx_tti()->mutable_tti()->set_value(tti[i]);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
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
// Inject fault
// the fault inject bitmap is based the fault capabilities bit position.
bool DcoOduAdapter::setOduFaultStatus(string aid, uint64_t injectFaultBitmap)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    // Fix timing issue. This function is being call from collector thread
    // when an ODU is deleted from config thread, it may get call after the
    // ODU is already deleted from DCO.

    // We set mOduPayLoad on create and clear on delete.
    if ( mOduPayLoad[oduId] == ODUPAYLOAD_UNSPECIFIED)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << "ODU is already deleted/not exist: " << oduId << '\n';
        return true;
    }

    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " fault bitmap :0x" << hex << injectFaultBitmap  << dec << " oduId: " << oduId << endl;

    odu.mutable_odu(0)->mutable_odu()->mutable_config()->mutable_rx_fault_status()->set_value(injectFaultBitmap);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);
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

bool DcoOduAdapter::setOduTimBdi(string aid, bool bInject)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    // Fix timing issue. This function is being call from collector thread
    // when an ODU is deleted from config thread, it may get call after the
    // ODU is already deleted from DCO.

    // We set mOduPayLoad on create and clear on delete.
    if ( mOduPayLoad[oduId] == ODUPAYLOAD_UNSPECIFIED)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << "ODU is already deleted/not exist: " << oduId << '\n';
        return true;
    }

    // DpMgr evaluate fault every seconds cache flag so we don't send same
    //  thing down to DCO
    if (mbBdiInject[oduId] == bInject)
    {
        return true;
    }

    mbBdiInject[oduId] = bInject;

    if (bInject == true)
    {
        if (setOduFaultStatus( aid, mRxSapiMismBitmap) == false)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Fail to inject BDI " << " Odu: "  << oduId << endl;
            return false;
        }
        INFN_LOG(SeverityLevel::info) << "Set ODU TIM BDI state for AID: " << aid;
    }
    else
    {
        if (setOduFaultStatus( aid, 0) == false)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Fail to clear BDI injection" << " Odu: "  << oduId << endl;
            return false;
        }
        INFN_LOG(SeverityLevel::info) << "Clear ODU TIM BDI state for AID: " << aid;
    }
    return status;
}

// Since DCO SDK does not support modify of Regen attribute.  When this regen
// flag get update we need to delete the existing ODU and create new one.
bool DcoOduAdapter::setOduRegen(string aid, bool bRegen)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    // Fetch the current ODU config
    OduCommon cfg;
    memset(&cfg, 0, sizeof(cfg));
    if ( getOduConfig(aid, &cfg) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " Fetch current ODU config failed " << endl;
        return false;
    }

    if ( cfg.regen == bRegen)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " DCO ODU regen matches with update regen.  Skip Regen update.  Regen:  " << (int) bRegen  << endl;
        return true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " Regen change, delete and re-create ODU.  Regen:  " << (int) bRegen  << endl;
        cfg.regen = bRegen;
        if (cfg.regen == true)
        {
            cfg.srvMode = SERVICEMODE_SWITCHING;
        }
        else
        {
            if (cfg.payLoad != DataPlane::ODUPAYLOAD_LO_ODU4)
            {
                cfg.srvMode = SERVICEMODE_ADAPTATION;
            }
        }
    }

    // Delete the current ODU
    if (deleteOdu(aid) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " delete Current ODU failed " << endl;
        return false;
    }

    cfg.loOduId = oduId;
    // Set transactionId to wait for DCO ack.
    cfg.transactionId = getTimeNanos();
    // Create new ODU with new time slot
    if (createOdu(aid, &cfg) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " oduId: " << oduId << " create new ODU new regen setting failed " << endl;
        return false;
    }

    // Work-around for timing issue with DCO.
    // IF fault update come in the middle of HAL delete and re-created
    // same LO-ODU, DCO won't send update for the same RX TTI
    //  or save fault again.  We need to give DCO sometime updates
    // the new ODU state before update our stale cache.
    // getOduFault() will check for time and force the update.

	mOduFault[oduId - 1].forceUpdateTime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now()) + ODU_FORCE_UPDATE_TIME_SEC;

    return status;
}


//
//  Get/Read methods
//

void DcoOduAdapter::dumpAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    Odu oduMsg;

    uint32 cfgId = 1;
    if (setKey(&oduMsg, cfgId) == false)
    {
        out << "Failed to set key" << endl;

        return;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&oduMsg);

    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);

        grpc::Status gStatus = mspCrud->Read( msg );
        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message();

            out << "DCO Read FAIL. Grpc Error Code: " << gStatus.error_code() << ": " << gStatus.error_message() << endl;

            return;
        }
    }

    for (int idx = 0; idx < oduMsg.odu_size(); idx++)
    {
        if (!oduMsg.odu(idx).odu().has_config())
        {
            out << "*** Odu Config not present for idx: " << idx << " Skipping!" << endl;

            continue;
        }

        OduCommon oduCmn;

        translateConfig(oduMsg.odu(idx).odu().config(), oduCmn);

        string oduAid;
        if (oduMsg.odu(idx).odu().config().has_name())
        {
            oduAid = oduMsg.odu(idx).odu().config().name().value();

            INFN_LOG(SeverityLevel::debug) << "Odu has name: " << oduAid;
        }
        else
        {
            oduAid = IdGen::genOduAid(oduCmn.loOduId,
                                      oduCmn.hoOtuId,
                                      oduCmn.payLoad);

            INFN_LOG(SeverityLevel::debug) << "Odu has NO NAME. Using old genID created AID: " << oduAid;
        }

        out << "*** Odu Config Info for OduId: " << oduCmn.loOduId << " Aid: " << oduAid << endl;

        dumpConfigInfo(out, oduCmn);

        out << "*** Odu Fault Info from cache for OduId: " << oduCmn.loOduId << " Aid: " << oduAid << endl;

        if (oduFaultInfo(out, oduAid) == false)
        {
            out << "oduFaultInfo Failed for Aid " << oduAid << endl;
        }

        if (oduMsg.odu(idx).odu().has_state())
        {
            OduStatus oduStat;

            translateStatus(oduMsg.odu(idx).odu().state(), oduStat);

            out << "*** Odu State Info for OduId: " << oduCmn.loOduId << " Aid: " << oduAid << endl;

            dumpOduStatusInfo(out, oduStat, (uint32)oduMsg.odu(idx).odu_id());
        }
        else
        {
            out << "*** Odu State not present for OduId: " << oduCmn.loOduId << " Aid: " << oduAid << endl;
        }

        out << "*** Odu Notify State from cache for OduId: " << oduCmn.loOduId << " Aid: " << oduAid << endl;

        oduNotifyState(out, oduMsg.odu(idx).odu_id());

        out << "*** Odu PM Info from cache for OduId: " << oduCmn.loOduId << " Aid: " << oduAid << endl;

        if (oduPmInfo(out, oduMsg.odu(idx).odu_id()) == false)
        {
            out << "oduPmInfo Failed for Aid " << oduAid << endl;
        }
    }
}

bool DcoOduAdapter::oduConfigInfo(ostream& out, string aid)
{
    OduCommon cfg;
    memset(&cfg, 0, sizeof(cfg));
    bool status = getOduConfig(aid, &cfg);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }
    out << " Odu Config Info: " << aid  << endl;

    dumpConfigInfo(out, cfg);

    return status;
}

void DcoOduAdapter::dumpConfigInfo(std::ostream& out, const OduCommon& cfg)
{
    int oduId;
    out << " Name (AID): " << cfg.name << endl;
    out << " HO OTUCni ID: " << cfg.hoOtuId << endl;
    out << " LO Odu ID:    " << cfg.loOduId << endl;
    out << " Sub Type:     " << EnumTranslate::toString(cfg.type) << endl;
    out << " Pay Load:     " << EnumTranslate::toString(cfg.payLoad) << endl;
    out << " MSI Pay Load: " << EnumTranslate::toString(cfg.msiPayLoad) << endl;
    out << " LoopBack:     " << EnumTranslate::toString(cfg.lpbk) << endl;
    out << " Service Mode: " << EnumTranslate::toString(cfg.srvMode) << endl;
    out << " Force MS Tx:  " << EnumTranslate::toString(cfg.forceMsTx) << endl;
    out << " Force MS Rx:  " << EnumTranslate::toString(cfg.forceMsRx) << endl;
    out << " Auto MS Tx:   " << EnumTranslate::toString(cfg.autoMsTx) << endl;
    out << " Auto MS Rx:   " << EnumTranslate::toString(cfg.autoMsRx) << endl;
    out << " PRBS Mon Tx:  " << EnumTranslate::toString(cfg.monitorTx) << endl;
    out << " PRBS Mon Rx:  " << EnumTranslate::toString(cfg.monitorRx) << endl;
    out << " TTI Tx:       ";
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        out << " 0x" << std::hex << (int)cfg.ttiTx[i];
        if (((i+1) % 16) == 0)
        {
            out << endl << "                 ";
        }
    }
    out << std::dec << endl;
    out << " TSG:          " << cfg.tsg << endl;
    out << " Time Slot:    ";
    for ( auto i = cfg.tS.begin(); i != cfg.tS.end(); i++)
    {
        out << *i << " ";
    }
    out << endl;

    out << " Rx Fault Status (inject): 0x" << hex << cfg.injectRxFaultStatus << dec << endl;
    out << endl;
    out << " Regen            : " << EnumTranslate::toString(cfg.regen) << endl;
    out << " transactionId    : 0x" << hex <<cfg.transactionId << dec << endl;

    for (oduId = 1; oduId <= MAX_ODU_ID; oduId++)
    {
	if ( mOduIdToAid[oduId].empty() )
	{
	    continue;
	}
	out << " Odu Id: " << oduId << " AID : "  << mOduIdToAid[oduId] << endl;
    }
    out << endl;
}

bool DcoOduAdapter::oduConfigInfo(ostream& out)
{
    //Temporarily block pending proper AID conversion support
    out << "Currently not available" << endl;

    TypeMapOduConfig mapCfg;

    if (!getOduConfig(mapCfg))
    {
        out << "*** FAILED to get Odu Status!!" << endl;
        return false;
    }

    out << "*** Odu Config Info ALL: " << endl;

    for(auto& it : mapCfg)
    {
        out << " Odu Config Info: "  << it.first << endl;
        dumpConfigInfo(out, *it.second);
        out << endl;
    }

    return true;
}

bool DcoOduAdapter::oduStatusInfo(ostream& out, string aid)
{
    OduStatus stat;
    memset(&stat, 0, sizeof(stat));
    bool status = getOduStatus(aid, &stat);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }
    out << " Odu Status Info: "  << aid << endl;
    int oduId = aidToOduId(aid);
    dumpOduStatusInfo(out, stat, oduId);

    return status;
}

bool DcoOduAdapter::oduStatusInfoAll(ostream& out)
{
    TypeMapOduStatus oduStatMap;

    bool status = getOduStatus(oduStatMap);
    if ( status == false )
    {
        out << __func__ << " FAIL to get OduStatusAll "  << endl;
        return status;
    }

    for(auto& it : oduStatMap)
    {
        out << " Odu Status Info for OduId: "  << it.first << endl;
        dumpOduStatusInfo(out, *it.second, it.first);
        out << endl;
    }

    return status;
}

void DcoOduAdapter::dumpOduStatusInfo(std::ostream& out, OduStatus& oduStat, uint32 loOduId)
{
    std::string oduAid("UNKNOWN");
    if (loOduId < (MAX_ODU_ID + 1))
    {
        oduAid = mOduIdToAid[loOduId];
    }
    out << " Aid:             " << oduAid << endl;
    out << " LO OduId:        " << loOduId << endl;
    out << " HO OTUCni ID:    " << oduStat.odu.hoOtuId << endl;
    out << " Sub Type:        " << EnumTranslate::toString(oduStat.odu.type) << endl;
    out << " Pay Load:        " << EnumTranslate::toString(oduStat.odu.payLoad) << endl;
    out << " MSI Pay Load:    " << EnumTranslate::toString(oduStat.odu.msiPayLoad) << endl;
    out << " Service Mode:    " << EnumTranslate::toString(oduStat.odu.srvMode) << endl;
    out << " Force MS Tx:     " << EnumTranslate::toString(oduStat.odu.forceMsTx) << endl;
    out << " Force MS Rx:     " << EnumTranslate::toString(oduStat.odu.forceMsRx) << endl;
    out << " Auto MS Tx:      " << EnumTranslate::toString(oduStat.odu.autoMsTx) << endl;
    out << " Auto MS Rx:      " << EnumTranslate::toString(oduStat.odu.autoMsRx) << endl;
    out << " PRBS Mon Tx:     " << EnumTranslate::toString(oduStat.odu.monitorTx) << endl;
    out << " PRBS Mon Rx:     " << EnumTranslate::toString(oduStat.odu.monitorRx) << endl;
    out << " TTI Tx:         ";
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        //out << " 0x" << setfill('0') << setw(2) << hex << (int)oduStat.odu.ttiTx[i];
        out << " 0x" << std::hex << (int)oduStat.odu.ttiTx[i];
        if (((i+1) % 16) == 0)
        {
            out << endl << "                 ";
        }
    }
    out << std::dec << endl;
    out << " TTI Rx:         ";
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        //out << " 0x" << setfill('0') << setw(2) << hex << (int)oduStat.ttiRx[i];
        out << " 0x" << std::hex << (int)oduStat.ttiRx[i];
        if (((i+1) % 16) == 0)
        {
            out << endl << "                 ";
        }
    }
    out << std::dec << endl;
    out << " Time Slot Granularity : " << oduStat.odu.tsg << endl;
    out << " Time Slot:              ";
    for ( auto i = oduStat.odu.tS.begin(); i != oduStat.odu.tS.end(); i++)
    {
        out << *i << " ";
    }
    out << dec << endl;
    out << " TX Fault:               0x" << hex << oduStat.faultTx  << dec << endl;
    out << " RX Fault:               0x" << hex << oduStat.faultRx << dec <<  endl;
    out << " Rate:                   " << oduStat.rate << endl;
    out << " Frame Rate:             " << getOduFrameRate(oduAid) << endl;
    out << " Rx Fault Status (inject): 0x" << hex << oduStat.odu.injectRxFaultStatus << dec << endl;
    out << " CDI mask : 0x" << hex << (int)oduStat.cdiMask << dec << endl;
    out << "\n mOduStateSubEnable: " << EnumTranslate::toString(mOduStateSubEnable) << endl;
    out << "\n mOduOpStatusSubEnable: " << EnumTranslate::toString(mOduOpStatusSubEnable) << endl;
    out << " Regen            : " << EnumTranslate::toString(oduStat.odu.regen) << endl;
    out << " TransactionId    : 0x" << hex << oduStat.odu.transactionId << dec << endl;
}

bool DcoOduAdapter::oduFaultInfo(ostream& out, string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    OduFault fault;
    status = getOduFault(aid, &fault);
    if ( status == false )
    {
        out << __func__ << " Get ODU fault FAIL "  << endl;
        return status;
    }

    out << endl;
    dumpOduFaultInfo(out, fault);

    return status;
}

bool DcoOduAdapter::oduFaultInfoAll(ostream& out)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    TypeMapOduFault oduFaultMap;
    status = getOduFault(oduFaultMap);
    if ( status == false )
    {
        out << __func__ << " Get ODU fault FAIL "  << endl;
        return status;
    }

    for(auto& it : oduFaultMap)
    {
        out << "Odu Fault Info for OduId: "  << it.first << " Aid: " << mOduIdToAid[it.first] << endl;
        dumpOduFaultInfo(out, *it.second);
        out << endl;
    }

    return status;
}

void DcoOduAdapter::dumpOduFaultInfo(std::ostream& out, OduFault& oduFault)
{
    out << "ODU FACILITY Fault: 0x" << hex << oduFault.facBmp << dec << endl;
    uint bitPos = 0;
    for ( auto i = oduFault.fac.begin(); i != oduFault.fac.end(); i++, bitPos++)
    {
        out << " Fault: " << left << std::setw(17) << i->faultName << " Bit" << left << std::setw(2) << bitPos << " Faulted: " << (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }
    out << "\n mOduFltSubEnable: " << EnumTranslate::toString(mOduFltSubEnable) << endl;
}


bool DcoOduAdapter::oduCdiInfo(ostream& out, string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    out << endl;
    out << "ODU CDI Capabilities Table" << endl;
    uint bitPos = 0;
    for ( auto i = mCdiCapa.cdiCapa.begin(); i != mCdiCapa.cdiCapa.end(); i++, bitPos++)
    {
        out << " CdiName: " << left << std::setw(17) << i->cdiName << " BitPos" << left << std::setw(2) << i->bitPos <<  " cdiEnum: " << i->cdiEnum << endl;
    }

    return status;
}


bool DcoOduAdapter::getOduConfig(string aid, OduCommon *cfg)
{
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " oduId: " << oduId << endl;
    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOduObj( &odu, oduId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Odu Object  FAIL "  << endl;
        return status;
    }

    if (odu.odu(idx).odu().has_config() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO Config Empty ... OduId: " <<  oduId << endl;
        return false;
    }

    translateConfig(odu.odu(idx).odu().config(), *cfg);

    return status;

} // getOduConfig

void DcoOduAdapter::translateConfig(const OduConfig& dcoCfg, OduCommon& adCfg)
{
    if (dcoCfg.has_name())
    {
            adCfg.name = dcoCfg.name().value();
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu has NO NAME";

        adCfg.name = "";
    }

    if (dcoCfg.has_otu_id())
    {
            adCfg.hoOtuId = dcoCfg.otu_id().value();
    }

    if (dcoCfg.has_odu_id())
    {
        adCfg.loOduId = dcoCfg.odu_id().value();
    }

    switch (dcoCfg.sub_type())
    {
        case oduConfig::SUBTYPE_LINE:
            adCfg.type = ODUSUBTYPE_LINE;
            break;
        case oduConfig::SUBTYPE_CLIENT:
            adCfg.type = ODUSUBTYPE_CLIENT;
            break;
        default:
            adCfg.type = ODUSUBTYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown SubType: " << dcoCfg.sub_type() << endl;
            break;
    }

    switch (dcoCfg.payload_type())
    {
        case oduConfig::PAYLOADTYPE_ODU4:
            adCfg.payLoad = ODUPAYLOAD_ODU4;
            break;
        case oduConfig::PAYLOADTYPE_LO_ODU4I:
            // In R5.x ODU Switching have LO-ODU4 Service Mode set
            // to  SWITCHING.  We checked Service Mode to set the
            // payload
            //  In R6.0 Xpress XCON case, LO-ODU4I can also have
            //  Service Mode set to SWITCHING.
            //  Checking Service Mode won't work anymore. We need to
            //  check the AID to make sure we don't set the wrong payload
            {
                string Odu4i = "ODU4i";
                std::size_t tpos = adCfg.name.find(Odu4i);

                // AID is not LO-ODU4I, it is for LO-ODU4 Switching
                if (tpos == std::string::npos)
                {
                    adCfg.payLoad = DataPlane::ODUPAYLOAD_LO_ODU4;
                }
                else
                {
                    adCfg.payLoad = DataPlane::ODUPAYLOAD_LO_ODU4I;
                }
            }
            break;
        case oduConfig::PAYLOADTYPE_LO_FLEXI:
            adCfg.payLoad = ODUPAYLOAD_LO_FLEXI;
            break;
        case oduConfig::PAYLOADTYPE_ODUCNI:
            adCfg.payLoad = ODUPAYLOAD_HO_ODUCNI;
            break;
        default:
            adCfg.payLoad = ODUPAYLOAD_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown Payload: " << dcoCfg.payload_type() << endl;
            break;
    }

    switch (dcoCfg.msi_payload_type())
    {
        case oduConfig::MSIPAYLOADTYPE_UNALLOCATED:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_UNALLOCATED;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4I_OTU4:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODU4I_OTU4;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4I_ODU4:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODU4I_ODU4;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODU4;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4I_100GE:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODU4I_100GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_25GE:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_25GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_50GE:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_50GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_75GE:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_75GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_100GE:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_100GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_200GE:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_200GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_400GE:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_400GE;
            break;
        default:
            adCfg.msiPayLoad = ODUMSIPAYLOAD_UNALLOCATED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown Payload: " << dcoCfg.payload_type() << endl;
            break;
    }

    switch (dcoCfg.time_slot_granularity())
    {
        case oduConfig::TIMESLOTGRANULARITY_25G:
            adCfg.tsg = TSGRANULARITY_25G;
            break;
        default:
            adCfg.tsg = TSGRANULARITY_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown TimeSlotGranularity: " << dcoCfg.time_slot_granularity() << endl;
            break;
    }

    adCfg.tS.clear();
    if (dcoCfg.sub_type() != oduConfig::SUBTYPE_CLIENT)
    {
        for (int i = 0; i < dcoCfg.time_slot_id_size(); i++)
        {
            adCfg.tS.push_back( dcoCfg.time_slot_id(i).value());
        }
    }

    for (int i = 0; i < dcoCfg.tx_tti_size(); i++)
    {
        adCfg.ttiTx[i] = dcoCfg.tx_tti(i).tx_tti().tti().value();
    }

    switch (dcoCfg.loopback_type())
    {
        case oduConfig::LOOPBACKTYPE_NONE:
            adCfg.lpbk = LOOP_BACK_TYPE_OFF;
            break;
        case oduConfig::LOOPBACKTYPE_FACILITY:
            adCfg.lpbk = LOOP_BACK_TYPE_FACILITY;
            break;
        case oduConfig::LOOPBACKTYPE_TERMINAL:
            adCfg.lpbk = LOOP_BACK_TYPE_TERMINAL;
            break;
        default:
            adCfg.lpbk = LOOP_BACK_TYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unkown Loopback: " << dcoCfg.loopback_type() << endl;
            break;
    }

    switch (dcoCfg.ms_force_signal_tx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unkown forceMsTx: " << dcoCfg.ms_force_signal_tx() << endl;
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_force_signal_rx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unkown forceMsRx: " << dcoCfg.ms_force_signal_rx() << endl;
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoCfg.ms_auto_signal_rx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unkown: " << dcoCfg.ms_auto_signal_rx() << endl;
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_auto_signal_tx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unkown autoMsTx: " << dcoCfg.ms_auto_signal_tx() << endl;
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoCfg.service_mode())
    {
        case oduConfig::SERVICEMODE_MODE_SWITCHING:
            adCfg.srvMode = SERVICEMODE_SWITCHING;
            break;
        case oduConfig::SERVICEMODE_MODE_TRANSPORT:
            adCfg.srvMode = SERVICEMODE_TRANSPORT;
            break;
        case oduConfig::SERVICEMODE_MODE_NONE:
            adCfg.srvMode = SERVICEMODE_ADAPTATION;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.service_mode() << endl;
            adCfg.srvMode = SERVICEMODE_UNSPECIFIED;
            break;
    }

    switch (dcoCfg.prbs_mon_tx())
    {
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31:
            adCfg.monitorTx = PRBSMODE_PRBS_X31;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31_INV:
            adCfg.monitorTx = PRBSMODE_PRBS_X31_INV;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_NONE:
            adCfg.monitorTx = PRBSMODE_PRBS_NONE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.prbs_mon_tx() << endl;
            break;

    }

    switch (dcoCfg.prbs_mon_rx())
    {
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31:
            adCfg.monitorRx = PRBSMODE_PRBS_X31;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31_INV:
            adCfg.monitorRx = PRBSMODE_PRBS_X31_INV;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_NONE:
            adCfg.monitorRx = PRBSMODE_PRBS_NONE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.prbs_mon_rx() << endl;
            break;

    }

    if (dcoCfg.has_rx_fault_status())
    {
        adCfg.injectRxFaultStatus = dcoCfg.rx_fault_status().value();
    }

    if (dcoCfg.has_regen())
    {
        adCfg.regen = dcoCfg.regen().value();
    }

    if (dcoCfg.has_tid())
    {
        adCfg.transactionId = dcoCfg.tid().value();
    }

} // translateConfig

bool DcoOduAdapter::getOduConfig(TypeMapOduConfig& mapCfg)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    Odu odu;

    uint32 cfgId = 1;
    if (setKey(&odu, cfgId) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);

    bool status = true;

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

    INFN_LOG(SeverityLevel::info) << "size: " << odu.odu_size();

    for (int idx = 0; idx < odu.odu_size(); idx++)
    {
        std::shared_ptr<OduCommon> spOduCmn = make_shared<OduCommon>();
        translateConfig(odu.odu(idx).odu().config(), *spOduCmn);

        string oduAid = spOduCmn->name;
        if (oduAid == "")
        {
            oduAid = IdGen::genOduAid(spOduCmn->loOduId,
                                      spOduCmn->hoOtuId,
                                      spOduCmn->payLoad);
        }

        if (spOduCmn->regen == true && spOduCmn->type == ODUSUBTYPE_CLIENT)
        {
            INFN_LOG(SeverityLevel::info) << "Skip update: This is HAL create ODU client Regen AID: " << oduAid << " IDX: " << idx << endl;
            // This is a regen client, don't update InstallConfig
            continue;
        }

        mapCfg[oduAid] = spOduCmn;
#ifdef NEW_ODU_AID_FORMAT
        bool status = cacheOduAid(oduAid, spOduCmn->loOduId);
#else
        bool status = cacheOduAid(oduAid, (OduCommon *)mapCfg[oduAid].get());
#endif
        if (status == false)
        {
            INFN_LOG(SeverityLevel::error) << "Failed to cache Odu Aid " << oduAid << '\n';
            return false;
        }

        mOduPayLoad[spOduCmn->loOduId] = spOduCmn->payLoad;  // Cache to return frame rate
    }

    return status;
}

bool DcoOduAdapter::getOduStatus(string aid, OduStatus *stat)
{
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " oduId: " << oduId << endl;
    Odu odu;

    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOduObj( &odu, oduId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::debug) << __func__ << " Get Dco Odu Object  FAIL "  << endl;
        return status;
    }

    if (odu.odu(idx).odu().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OduId: " <<  oduId << endl;
        return false;
    }

    translateStatus(odu.odu(idx).odu().state(), *stat, true);

    return status;
} // getOduStatus

bool DcoOduAdapter::getOduStatus(TypeMapOduStatus& mapStat)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    Odu odu;

    uint32 cfgId = 1;
    if (setKey(&odu, cfgId) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);

    bool status = true;

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

    INFN_LOG(SeverityLevel::info) << "size: " << odu.odu_size();

    for (int idx = 0; idx < odu.odu_size(); idx++)
    {
        if (odu.odu(idx).odu().has_state() == false )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OduId: " <<  idx << endl;
            continue;
        }

        std::shared_ptr<OduStatus> spOduStat = make_shared<OduStatus>();

        translateStatus(odu.odu(idx).odu().state(), *spOduStat);

        mapStat[odu.odu(idx).odu().config().odu_id().value()] = spOduStat;
    }

    return true;
}
void DcoOduAdapter::translateStatus(const DcoOduState& dcoStat, OduStatus& adStat, bool updateCache)
{
    uint32 oduId = dcoStat.odu_id().value();

    switch (dcoStat.sub_type())
    {
        case oduConfig::SUBTYPE_LINE:
            adStat.odu.type = ODUSUBTYPE_LINE;
            break;
        case oduConfig::SUBTYPE_CLIENT:
            adStat.odu.type = ODUSUBTYPE_CLIENT;
            break;
        default:
            adStat.odu.type = ODUSUBTYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown SubType: " << dcoStat.sub_type() << endl;
            break;
    }

    switch (dcoStat.payload_type())
    {
        case oduConfig::PAYLOADTYPE_ODU4:
            adStat.odu.payLoad = ODUPAYLOAD_ODU4;   //100G - TODO
            break;
        case oduConfig::PAYLOADTYPE_LO_ODU4I:
            adStat.odu.payLoad = ((static_cast<int>(dcoStat.service_mode())) == oduConfig::SERVICEMODE_MODE_SWITCHING) ? ODUPAYLOAD_LO_ODU4 : ODUPAYLOAD_LO_ODU4I;
            break;
        case oduConfig::PAYLOADTYPE_LO_FLEXI:
            adStat.odu.payLoad = ODUPAYLOAD_LO_FLEXI;  //400G
            break;
        case oduConfig::PAYLOADTYPE_ODUCNI:
            adStat.odu.payLoad = ODUPAYLOAD_HO_ODUCNI;  //Not supported in HAL layer
            break;
        default:
            adStat.odu.payLoad = ODUPAYLOAD_UNSPECIFIED;
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown Payload: " << dcoStat.payload_type() << endl;
            break;
    }

    switch (dcoStat.msi_payload_type())
    {
        case oduConfig::MSIPAYLOADTYPE_UNALLOCATED:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_UNALLOCATED;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4I_OTU4:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODU4I_OTU4;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4I_ODU4:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODU4I_ODU4;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODU4;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODU4I_100GE:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODU4I_100GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_25GE:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_25GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_50GE:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_50GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_75GE:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_75GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_100GE:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_100GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_200GE:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_200GE;
            break;
        case oduConfig::MSIPAYLOADTYPE_ODUFLEXI_400GE:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_ODUFLEXI_400GE;
            break;
        default:
            adStat.odu.msiPayLoad = ODUMSIPAYLOAD_UNALLOCATED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown Payload: " << dcoStat.msi_payload_type() << endl;
            break;
    }

    switch (dcoStat.time_slot_granularity())
    {
        case oduConfig::TIMESLOTGRANULARITY_25G:
            adStat.odu.tsg = TSGRANULARITY_25G;
            break;
        default:
            adStat.odu.tsg = TSGRANULARITY_UNSPECIFIED;
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown TimeSlotGranularity: " << dcoStat.time_slot_granularity() << endl;
            break;
    }

    adStat.odu.tS.clear();
    for (int i = 0; i < dcoStat.time_slot_id_size(); i++)
    {
        adStat.odu.tS.push_back( dcoStat.time_slot_id(i).value());
    }

    for (int i = 0; i < dcoStat.tx_tti_size(); i++)
    {
        adStat.odu.ttiTx[i] = dcoStat.tx_tti(i).tx_tti().tti().value();
    }

    switch (dcoStat.loopback_type())
    {
        case oduConfig::LOOPBACKTYPE_NONE:
            adStat.odu.lpbk = LOOP_BACK_TYPE_OFF;
            break;
        case oduConfig::LOOPBACKTYPE_FACILITY:
            adStat.odu.lpbk = LOOP_BACK_TYPE_FACILITY;
            break;
        case oduConfig::LOOPBACKTYPE_TERMINAL:
            adStat.odu.lpbk = LOOP_BACK_TYPE_TERMINAL;
            break;
        default:
            adStat.odu.lpbk = LOOP_BACK_TYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unkown Loopback: " << dcoStat.loopback_type() << endl;
            break;
    }

    switch (dcoStat.ms_force_signal_tx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unkown forceMsTx: " << dcoStat.ms_force_signal_tx() << endl;
            adStat.odu.forceMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoStat.ms_force_signal_rx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unkown forceMsRx: " << dcoStat.ms_force_signal_rx() << endl;
            adStat.odu.forceMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoStat.ms_auto_signal_rx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unkown: " << dcoStat.ms_auto_signal_rx() << endl;
            adStat.odu.autoMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoStat.ms_auto_signal_tx())
    {
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE:
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unkown autoMsTx: " << dcoStat.ms_auto_signal_tx() << endl;
            adStat.odu.autoMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoStat.service_mode())
    {
        case oduConfig::SERVICEMODE_MODE_SWITCHING:
            adStat.odu.srvMode = SERVICEMODE_SWITCHING;
            break;
        case oduConfig::SERVICEMODE_MODE_TRANSPORT:
            adStat.odu.srvMode = SERVICEMODE_TRANSPORT;
            break;
        case oduConfig::SERVICEMODE_MODE_NONE:
            adStat.odu.srvMode = SERVICEMODE_NONE;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.service_mode() << endl;
            adStat.odu.srvMode = SERVICEMODE_UNSPECIFIED;
            break;
    }

    // TODO: Free time slot leaf-list

    if (dcoStat.has_rate())
    {
        int64 digit = dcoStat.rate().digits();
        uint32 precision = dcoStat.rate().precision();
        adStat.rate = digit/pow(10, precision);
    }

    for (int i = 0; i < dcoStat.rx_tti_size(); i++)
    {
        adStat.ttiRx[i] = dcoStat.rx_tti(i).rx_tti().tti().value();
    }

    // TODO change name when ODU fault is finalize
    if (dcoStat.has_facility_fault())
    {
        if ((oduId - 1) < MAX_ODU_ID)
        {
            adStat.faultRx = dcoStat.facility_fault().value();
            if (oduId > 0 && updateCache)
            {
                mOduFault[oduId - 1].faultBitmap = adStat.faultRx;
                INFN_LOG(SeverityLevel::info) << __func__ << " oduId: " << oduId << " fault: 0x" << hex << mOduFault[oduId - 1].faultBitmap << dec << endl;
            }
        }
    }

    if (dcoStat.has_otu_id())
    {
        adStat.odu.hoOtuId = dcoStat.otu_id().value();
    }

    if (dcoStat.has_rx_fault_status())
    {
        adStat.odu.injectRxFaultStatus = dcoStat.rx_fault_status().value();
    }

    switch (dcoStat.prbs_mon_tx())
    {
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31:
            adStat.odu.monitorTx = PRBSMODE_PRBS_X31;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31_INV:
            adStat.odu.monitorTx = PRBSMODE_PRBS_X31_INV;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_NONE:
            adStat.odu.monitorTx = PRBSMODE_PRBS_NONE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown prbs mon tx: " << adStat.odu.monitorTx << endl;
            break;

    }

    switch (dcoStat.prbs_mon_rx())
    {
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31:
            adStat.odu.monitorRx = PRBSMODE_PRBS_X31;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_X31_INV:
            adStat.odu.monitorRx = PRBSMODE_PRBS_X31_INV;
            break;
        case prbs::INFINERADCOODUPRBSMODE_PRBS_NONE:
            adStat.odu.monitorRx = PRBSMODE_PRBS_NONE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown prbs mon rx: " << adStat.odu.monitorRx << endl;
            break;

    }

    if (dcoStat.has_cdi())
    {
        if ((oduId - 1) < MAX_ODU_ID)
        {
            adStat.cdiMask = dcoStat.cdi().value();
            if (oduId > 0 && updateCache)
            {
                mCdi[oduId - 1] = adStat.cdiMask;
            }
        }
    }

    if (dcoStat.has_regen())
    {
        adStat.odu.regen = dcoStat.regen().value();
    }

    if (dcoStat.has_tid())
    {
        adStat.odu.transactionId = dcoStat.tid().value();
    }
}

bool DcoOduAdapter::getOduFaultTx(string aid, uint64_t *fault)
{
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " oduId: " << oduId << endl;
    Odu odu;
    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOduObj( &odu, oduId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Odu Object  FAIL "  << endl;
        return status;
    }

    if (odu.odu(idx).odu().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OduId: " <<  oduId << endl;
        return false;
    }

    return status;
}

bool DcoOduAdapter::getOduFaultRx(string aid, uint64_t *fault)
{
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " oduId: " << oduId << endl;
    Odu odu;
    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOduObj( &odu, oduId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Odu Object  FAIL "  << endl;
        return status;
    }

    if (odu.odu(idx).odu().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OduId: " <<  oduId << endl;
        return false;
    }

    // Hold on the name change rx to facility as DCO fault still changing.
    if (odu.odu(idx).odu().state().has_facility_fault())
    {
        *fault = odu.odu(idx).odu().state().facility_fault().value();
    }
    return status;
}

//force equal to true case should only be used for warm boot query
bool DcoOduAdapter::getOduFault(string aid, OduFault *faults, bool force)
{
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " oduId: " << oduId << endl;
    Odu odu;
    if (setKey(&odu, oduId) == false)
    {
        return false;
    }

    bool status = true;

    // Work-around, When HAL delete and recreate same LO-ODU on
    // regen flag change.  DCO won't send notify for same info.
    // use "force=true" flag to update the notify cache.
    if (mOduFault[oduId - 1].forceUpdateTime > 0 )
    {
        std::time_t currtime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now());

        INFN_LOG(SeverityLevel::info) << __func__ << " oduId: " << oduId << " Force update time: " <<  mOduFault[oduId - 1].forceUpdateTime  << " current time: " << currtime << endl;

        if ( currtime > mOduFault[oduId - 1].forceUpdateTime )
        {
            // force update cache
            force = true;
            mOduFault[oduId - 1].ClearFaultCache();
            mTtiRx[oduId - 1].ClearTTICache();
            mOduFault[oduId - 1].forceUpdateTime = 0;
        }
    }

    if (force)
    {
	    int idx = 0;
	    status = getOduObj( &odu, oduId, &idx);
	    if ( status == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Odu Object  FAIL "  << endl;
		    return status;
	    }

	    if (odu.odu(idx).odu().has_state() == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OduId: " <<  oduId << endl;
		    return false;
	    }

	    // Hold on the name change rx to facility as DCO fault still changing.
	    if (odu.odu(idx).odu().state().has_facility_fault())
	    {
		    faults->facBmp = odu.odu(idx).odu().state().facility_fault().value();
		    //if odu fault is already been updated via notification, skip the retrieval
		    if (mOduFault[oduId - 1].isNotified == false)
		    {
			    mOduFault[oduId - 1].faultBitmap = faults->facBmp;
			    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve odu fault " << oduId << " with bitmap: 0x" <<  hex << mOduFault[oduId - 1].faultBitmap << dec << endl;
		    }
		    else
		    {
			    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve odu fault " << oduId << " with fault notify flag:" <<  mOduFault[oduId - 1].isNotified << endl;
		    }
	    }
	    else
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no odu fault in state, oduId: " << oduId;
	    }

	    // AF0 TODO: Need to take care of notify during warm reboot case, between this initial
	    //  read and a notify
	    if (odu.odu(idx).odu().state().has_cdi())
	    {
	        mCdi[oduId - 1] = odu.odu(idx).odu().state().cdi().value();
	    }

	    //only been triggered when create to handle warm boot case
	    int rxTtiSize = odu.odu(idx).odu().state().rx_tti_size();
	    if ( rxTtiSize != 0 )
	    {
			//if odu tti is already been updated via notification, skip the retrieval
			if (mTtiRx[oduId - 1].isNotified == false)
			{
			    for (int i = 0; i < rxTtiSize; i++)
			    {
			        mTtiRx[oduId - 1].ttiRx[i] = odu.odu(idx).odu().state().rx_tti(i).rx_tti().tti().value();
			    }
			    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve odu tti " << idx << " with size: " << rxTtiSize << endl;
			}
			else
			{
			    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve odu tti " << idx << " with tti notify flag: " << mTtiRx[oduId - 1].isNotified << endl;
			}
	    }
	    else
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve tti since no tti in state, oduId: " << oduId;
	    }

    }
    else
    {
	    faults->facBmp = mOduFault[oduId - 1].faultBitmap;
    }

    uint64_t facFault = faults->facBmp;
    AdapterFault tmpFault;
    for ( auto i = mFaultCapa.fac.begin(); i != mFaultCapa.fac.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (facFault & 0x1)
        {
            tmpFault.faulted = true;
        }
        faults->fac.push_back(tmpFault);
        facFault >>= 0x1;
    }


    return status;
}

bool DcoOduAdapter::getOduFault(TypeMapOduFault& mapFlt)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    Odu odu;

    uint32 cfgId = 1;
    if (setKey(&odu, cfgId) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);

    bool status = true;

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

    INFN_LOG(SeverityLevel::info) << "size: " << odu.odu_size();

    for (int idx = 0; idx < odu.odu_size(); idx++)
    {
        uint32 oduId = odu.odu(idx).odu().config().odu_id().value();

        if (odu.odu(idx).odu().has_state() == false )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OduId: " <<  oduId << endl;
            continue;
        }

        if (odu.odu(idx).odu().state().has_facility_fault())
        {
            std::shared_ptr<OduFault> spOduFlt = make_shared<OduFault>();

            translateFault(odu.odu(idx).odu().state().facility_fault().value(), *spOduFlt);

            mapFlt[odu.odu(idx).odu().config().odu_id().value()] = spOduFlt;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << "No odu fault in state, oduId: " << oduId;
        }
    }

    return true;
}

void DcoOduAdapter::translateFault(uint64 dcoFltBmp, OduFault& adFlt)
{
    adFlt.facBmp = dcoFltBmp;

    uint64_t facFault = adFlt.facBmp;
    AdapterFault tmpFault;
    for ( auto i = mFaultCapa.fac.begin(); i != mFaultCapa.fac.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (facFault & 0x1)
        {
            tmpFault.faulted = true;
        }
        adFlt.fac.push_back(tmpFault);
        facFault >>= 0x1;
    }
}

bool DcoOduAdapter::getOduPm(string aid)
{
	INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    return true;
}

// Calculate frame rate base on GX-PM-Additionnal-Material.xlxs
// https://nam11.safelinks.protection.outlook.com/ap/x-59584e83/?url=https%3A%2F%2Finfinera.sharepoint.com%2F%3Ax%3A%2Fr%2Fteams%2FGX%2F_layouts%2F15%2Fdoc2.aspx%3Faction%3Dedit%26sourcedoc%3D%257BF563CDE4-B892-4D05-819D-A090F5AAE322%257D%26CID%3D20DA8DF5-002E-4F78-9C74-9E30DA3948CA%26wdLOR%3Dc05F57B5E-7EFF-480D-AF3B-08078C343C58&data=04%7C01%7Cbnguyen%40infinera.com%7C79d958dd6b3a459f959908d8b9a112bc%7C285643de5f5b4b03a1530ae2dc8aaf77%7C1%7C0%7C637463448762227690%7CUnknown%7CTWFpbGZsb3d8eyJWIjoiMC4wLjAwMDAiLCJQIjoiV2luMzIiLCJBTiI6Ik1haWwiLCJXVCI6Mn0%3D%7C1000&sdata=OU6hozHcAw6kv6oB4P7dRmwZpNJCquU2h0Lj6%2BJO%2BQY%3D&reserved=0

uint32_t DcoOduAdapter::getOduFrameRate(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    ClientMode mode = DcoCardAdapter::mCfgClientMode;
    double rate = 0;

    switch (mOduPayLoad[oduId])
    {
        case ODUPAYLOAD_LO_ODU4I:
            rate = 100.0;
            break;
        case ODUPAYLOAD_LO_FLEXI:
            rate = 400.0;
            break;
        case ODUPAYLOAD_ODU4:
        case ODUPAYLOAD_LO_ODU4:
            return OTU4_FRAME_RATE;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << " Unknown rate aid: " << aid << ", payload: " << EnumTranslate::toString(mOduPayLoad[oduId]) << '\n';
            return 0;
    }

    if (mode == CLIENTMODE_LXTP_E)
    {
        return ( (uint32_t) round ( (rate/DCO_TS_GRANULARITY) * LXTF20_E_FRAME_RATE) );
    }
    else if (mode == CLIENTMODE_LXTP_M)
    {
        return ( (uint32_t) round ( (rate/DCO_TS_GRANULARITY) * LXTF20_M_FRAME_RATE) );
    }
    else
    {
        return 0;
    }
}

uint64_t DcoOduAdapter::getOduPmErrorBlocks(string aid, FaultDirection direction)
{
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return false;
    }

    if (mOduPms.HasPmData(oduId) == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " no pm cache data for oduId: " << oduId << endl;
        return false;
    }

    auto oduPm = mOduPms.GetPmData(oduId);
    if (oduPm.oduId != oduId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " not matching.. pm cache data oduId is: " << oduPm.oduId << endl;
        return false;
    }

    uint64_t err_blocks = ((FAULTDIRECTION_RX == direction) ? oduPm.rx.err_blocks : oduPm.tx.err_blocks);
    return (err_blocks);
}

void DcoOduAdapter::subOduFault()
{
	OduFms ofm;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&ofm);
	::GnmiClient::AsyncSubCallback* cb = new OduFaultResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " odu fault sub request sent to server.."  << endl;
}

void DcoOduAdapter::subOduState()
{
	OduState os;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&os);
	::GnmiClient::AsyncSubCallback* cb = new OduStateResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " odu state sub request sent to server.."  << endl;
}

void DcoOduAdapter::subOduOpStatus()
{
	dcoOduOpStatus ops;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&ops);
	::GnmiClient::AsyncSubCallback* cb = new OduOpStatusResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " odu op status sub request sent to server.."  << endl;
}

void DcoOduAdapter::subOduPm()
{
	if (dp_env::DpEnvSingleton::Instance()->isSimEnv() && mIsGnmiSimModeEn) {
		//spawn a thread to feed up pm data for sim
		if (mSimOduPmThread.joinable() == false) { //check whether the sim pm thread is running
			INFN_LOG(SeverityLevel::info) << "create mSimOduPmThread for odu " << endl;
			mSimOduPmThread = std::thread(&DcoOduAdapter::setSimPmData, this);
		}
		else {
			INFN_LOG(SeverityLevel::info) << "mSimOduPmThread is already running..." << endl;
		}
	}
	else {
		INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
		dcoOduPm cp;
	    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&cp);
		::GnmiClient::AsyncSubCallback* cb = new OduPmResponseContext(); //cleaned up in grpc client
		std::string cbId = mspCrud->Subscribe(msg, cb);
		INFN_LOG(SeverityLevel::info) << " odu pm sub request sent to server.."  << endl;
	}
}

//set same sim pm data for all objects..
void DcoOduAdapter::setSimPmData()
{
    while (1)
	{
		this_thread::sleep_for(seconds(1));

        bool isSimDcoConnect = DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)DCC_ZYNQ).isSimDcoConnect();
        if (!mIsGnmiSimModeEn || !isSimDcoConnect)
        {
            continue;
        }

		for (int i = 1; i <= MAX_ODU_ID; i++) {
			if (mOduPms.HasPmData(i) == false) continue;
			auto oduPm = mOduPms.GetPmData(i);
			int oduId= oduPm.oduId;
			INFN_LOG(SeverityLevel::debug) << __func__ << " oduId: " << oduId << ", oduPm.aid: " << oduPm.aid << ", i: " << i << endl;

			oduPm.rx.bip_err = 12;
			oduPm.rx.err_blocks = 34;
			oduPm.rx.far_end_err_blocks = 5;
			oduPm.rx.bei = 6;
			oduPm.rx.iae = 7;
			oduPm.rx.biae = 8;
			oduPm.rx.prbs_sync_err_count = 9;
			oduPm.rx.prbs_err_count = 1;
			oduPm.rx.latency = 2;

			oduPm.tx.bip_err = 112;
			oduPm.tx.err_blocks = 334;
			oduPm.tx.far_end_err_blocks = 35;
			oduPm.tx.bei = 46;
			oduPm.tx.iae = 57;
			oduPm.tx.biae = 28;
			oduPm.tx.prbs_sync_err_count = 19;
			oduPm.tx.prbs_err_count = 91;
			oduPm.tx.prbs_err_count = 62;

            oduPm.valid = true;

			DcoOduAdapter::mOduPms.UpdatePmData(oduPm, oduId); //pmdata map key is 1 based
		}

	    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	    dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoOduAdapter::mOduPms), &DcoOduAdapter::mOduPms);
	}
}

//only used for cli
bool DcoOduAdapter::oduPmInfo(ostream& out, int oduId)
{
    if (mOduPms.HasPmData(oduId) == false)
    {
        out << " no pm cache data for oduId: " << oduId << endl;
        return false;
    }

    auto oduPm = mOduPms.GetPmData(oduId);
    if (oduPm.oduId != oduId)
    {
        out << " not matching.. pm cache data oduId is: " << oduPm.oduId << endl;
        return false;
    }

	dumpOduPmInfo(out, oduPm);

	return true;
}

//only used for cli
bool DcoOduAdapter::oduPmInfoAll(ostream& out)
{
    for(uint32 i = 1; i <= MAX_ODU_ID; i++)
    {
        if (mOduPms.HasPmData(i) == false)
        {
            continue;
        }

        auto oduPm = mOduPms.GetPmData(i);
        if (oduPm.oduId != i)
        {
            out << " not matching.. pm cache data oduId is: " << oduPm.oduId << endl;
            return false;
        }

        dumpOduPmInfo(out, oduPm);
    }

    return true;
}

void DcoOduAdapter::dumpOduPmInfo(std::ostream& out, DpMsOduPMContainer& oduPm)
{
    out << " Odu aid: " << oduPm.aid << ", id: " << oduPm.oduId << ", Pm Info: " << endl;

    out << " rx bip_err: " << oduPm.rx.bip_err << endl;
    out << " rx err_blocks: " << oduPm.rx.err_blocks    << endl;
    out << " rx far_end_err_blocks: " << oduPm.rx.far_end_err_blocks << endl;
    out << " rx bei: " << oduPm.rx.bei << endl;
    out << " rx iae: " << oduPm.rx.iae << endl;
    out << " rx biae: " << oduPm.rx.biae << endl;
    out << " rx prbs_sync_err_count: " << oduPm.rx.prbs_sync_err_count << endl;
    out << " rx prbs_err_count: " << oduPm.rx.prbs_err_count << endl;
    out << " rx latency: " << oduPm.rx.latency << endl;

    out << " tx bip_err: " << oduPm.tx.bip_err << endl;
    out << " tx err_blocks: " << oduPm.tx.err_blocks    << endl;
    out << " tx far_end_err_blocks: " << oduPm.tx.far_end_err_blocks << endl;
    out << " tx bei: " << oduPm.tx.bei << endl;
    out << " tx iae: " << oduPm.tx.iae << endl;
    out << " tx biae: " << oduPm.tx.biae << endl;
    out << " tx prbs_sync_err_count: " << oduPm.tx.prbs_sync_err_count << endl;
    out << " tx prbs_err_count: " << oduPm.tx.prbs_err_count << endl;
    out << " tx latency: " << oduPm.tx.latency << endl;
    out << " mOduPmSubEnable: " << EnumTranslate::toString(mOduPmSubEnable) << endl;

    std::time_t last_c = std::chrono::system_clock::to_time_t(oduPm.updateTime);
    std::string ts(ctime(&last_c));
    ts.erase(std::remove(ts.begin(), ts.end(), '\n'), ts.end());
    out << "Update Time: " << ts << endl;
    out << "Update Count: " << oduPm.updateCount << endl;
}

//only used for cli
bool DcoOduAdapter::oduPmAccumInfo(ostream& out, int oduId)
{
	if (mOduPms.HasPmData(oduId) == false)
	{
		out << " no pm cache data for oduId: " << oduId<< endl;
		return false;
	}
	auto oduPmAccum = mOduPms.GetPmDataAccum(oduId);
	if (oduPmAccum.oduId != oduId)
	{
		out << " not matching.. pm cache data oduId is: " << oduPmAccum.oduId << endl;
		return false;
	}

	out << " Odu aid: " << oduPmAccum.aid << ", id: " << oduPmAccum.oduId << ", Pm Info: " << endl;
	out << " Accumulation Mode: " << (true == oduPmAccum.accumEnable ? "enabled" : "disabled") << endl;

	out << " rx bip_err: " << oduPmAccum.rx.bip_err << endl;
	out << " rx err_blocks: " << oduPmAccum.rx.err_blocks    << endl;
	out << " rx far_end_err_blocks: " << oduPmAccum.rx.far_end_err_blocks << endl;
	out << " rx bei: " << oduPmAccum.rx.bei << endl;
	out << " rx prbs_sync_err_count: " << oduPmAccum.rx.prbs_sync_err_count << endl;
	out << " rx prbs_err_count: " << oduPmAccum.rx.prbs_err_count << endl;

	out << " tx bip_err: " << oduPmAccum.tx.bip_err << endl;
	out << " tx err_blocks: " << oduPmAccum.tx.err_blocks    << endl;
	out << " tx far_end_err_blocks: " << oduPmAccum.tx.far_end_err_blocks << endl;
	out << " tx bei: " << oduPmAccum.tx.bei << endl;
	out << " tx prbs_sync_err_count: " << oduPmAccum.tx.prbs_sync_err_count << endl;
	out << " tx prbs_err_count: " << oduPmAccum.tx.prbs_err_count << "\n\n";
	return true;
}

//only used for cli
bool DcoOduAdapter::oduPmAccumClear(ostream& out, int oduId)
{
    mOduPms.ClearPmDataAccum(oduId);
    return true;
}

bool DcoOduAdapter::oduPmAccumEnable(ostream& out, int oduId, bool enable)
{
    mOduPms.SetAccumEnable(oduId, enable);
    return true;
}

//spanTime for each dpms odu pm collection start and end and execution time
void DcoOduAdapter::oduPmTimeInfo(ostream& out)
{
	out << "odu pm time info in last 20 seconds: " << endl;
	for (int i = 0; i < spanTime; i++)
	{
		out << " dco pm strobe delta, " << i << " :" << mStartSpanArray[i] << " ms" << endl;
	}
	for (int i = 0; i < spanTime; i++)
	{
		out << " send to redis delta, " << i << " :" << mEndSpanArray[i] << " ms" << endl;
	}
	for (int i = 0; i < spanTime; i++)
	{
		out << " dpms execution time, " << i << " :" << mDeltaArray[i] << " ms" << endl;
	}
}

bool DcoOduAdapter::updateNotifyCache()
{
    Odu odu;
    uint32 id = 1; //set dummy id as yang require this as key
    if (setKey(&odu, id) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&odu);

    bool status = true;

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

    INFN_LOG(SeverityLevel::info) << __func__ << " odu size: " << odu.odu_size();

    for (int idx = 0; idx < odu.odu_size(); idx++)
    {
        int oduId = 1;
        if (odu.odu(idx).odu().state().has_odu_id())
        {
            oduId = odu.odu(idx).odu().state().odu_id().value();
            INFN_LOG(SeverityLevel::info) << "Odu has oduId: " << oduId;
        }
        if (odu.odu(idx).odu().state().has_facility_fault())
        {
            //if odu fault is already been updated via notification, skip the retrieval
            if (mOduFault[oduId - 1].isNotified == false)
            {
                mOduFault[oduId - 1].faultBitmap = odu.odu(idx).odu().state().facility_fault().value();
	        INFN_LOG(SeverityLevel::info) << "Odu has fac fault: 0x" << hex << mOduFault[oduId - 1].faultBitmap << dec;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve odu fault " << oduId << " with fault notify flag:" <<  mOduFault[oduId - 1].isNotified << endl;
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no odu fault in state, oduId: " << oduId;
        }
        int ttiSize = odu.odu(idx).odu().state().rx_tti_size();
        if ( ttiSize != 0 )
        {
            //if odu tti is already been updated via notification, skip the retrieval
            if (mTtiRx[oduId - 1].isNotified == false)
            {
                for (int i = 0; i < ttiSize; i++)
                {
                    mTtiRx[oduId - 1].ttiRx[i] = odu.odu(idx).odu().state().rx_tti(i).rx_tti().tti().value();
                }
                INFN_LOG(SeverityLevel::info) << "Odu has tti size: " << ttiSize;
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve tti since no tti in state, oduId: " << oduId;
        }
    }
    return status;
}

// ====== DcoClientAdapter private =========
// Client Odu key range: 33-48
int DcoOduAdapter::aidToOduId (string aid)
{
    INFN_LOG(SeverityLevel::debug) << __func__ << " run... AID: "  << aid << endl;
    if (aid == NULL)
        return -1;

    string cliOdu = "Odu ";
    std::size_t found = aid.find(cliOdu);
    if (found != std::string::npos)
    {
        string sId = aid.substr(found+cliOdu.length());
        return(stoi(sId,nullptr));
    }

#ifdef NEW_ODU_AID_FORMAT

    int otuId = aidToHoOtuId(aid);

    if (otuId == -1)
    {
        return -1;
    }

    string aidPort = "-T";
    found = aid.find(aidPort);
    //client odu case
    if (found != std::string::npos)
    {
        string sId = aid.substr(found + aidPort.length());
        int clientOduId = stoi(sId,nullptr) + CLIENT_ODU_ID_OFFSET;
        return clientOduId;
    }
    else
    {
        size_t pos = aid.find_last_of("-");
        string instStr = aid.substr(pos+1, pos+2);

        int instId = stoi(instStr);
        size_t posOdu4i = aid.find(IdGen::cOdu4iAidStr);
        //for odu4 switching, lo-odu type is set to ODU4 to mimic odu4 coming from client interface
        size_t posOdu4 = aid.find(IdGen::cOdu4AidStr);
        bool isOdu4i = (posOdu4i != string::npos || posOdu4 != string::npos);
        uint32 offset;
        if (otuId == (HO_OTU_ID_OFFSET + 1)) //17
        {
            if (isOdu4i)
            {
                offset = IdGen::cOffOdu4i_1;
            }
            else
            {
                offset = IdGen::cOffOduFlex_1;
            }
        }
        else
        {
            if (isOdu4i)
            {
                offset = IdGen::cOffOdu4i_2;
            }
            else
            {
                offset = IdGen::cOffOduFlex_2;
            }
        }

        return instId + offset;
    }

#else
    for ( int oduId = 1; oduId <= MAX_ODU_ID; oduId++)
    {
        if ( mOduIdToAid[oduId].empty() )
        {
            continue;
        }
        if ( mOduIdToAid[oduId] == aid )
        {
            return oduId;
        }
    }

#endif

    return -1;
}

bool DcoOduAdapter::setKey (Odu *odu, int oduId)
{
    bool status = true;
	if (odu == NULL)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Fail *odu = NULL... "  << endl;
        return false;
    }

    Odus_OduKey *cKey;

    // Always use the first object in the ClientOdu list, since we
    // create new odu object every time we send.
    cKey = odu->add_odu();
    cKey->set_odu_id(oduId);
    cKey->mutable_odu()->mutable_config()->mutable_odu_id()->set_value(oduId);

    return status;
}

// Get the HO-OTUCni ID for LO-ODU
int DcoOduAdapter::aidToHoOtuId (string aid)
{
    INFN_LOG(SeverityLevel::debug) << __func__ << " run... AID: "  << aid << endl;
    if (aid == NULL)
        return -1;
    // AID format: 1-4-L1-1 or 1-4-L2-1 of OduCni
    string sId;
    string aidOtu = "-L";
    std::size_t found = aid.find(aidOtu);
    if (found != std::string::npos)
    {
        std::size_t pos = found + aidOtu.length();
        while (pos != std::string::npos)
        {
            if (aid[pos] == '-')
                break;
            sId.push_back(aid[pos]);
            INFN_LOG(SeverityLevel::debug) << __func__ << "sId: " << sId << endl;
            pos++;
        }
	    return( (stoi(sId,nullptr) + HO_OTU_ID_OFFSET) );
    }

    // AID format: 1-5-T1, map to otu id 1; 1-5-T16 map to otu id 16
    string aidPort = "-T";
    found = aid.find(aidPort);
    if (found != std::string::npos)
    {
        sId = aid.substr(found + aidPort.length());
        return(stoi(sId,nullptr));
    }

    // CLI Debug
    {
        string cliOtu = "Ho-Otu ";
        found = aid.find(cliOtu);
        if (found == std::string::npos)
            return -1;
        std::size_t start = found + cliOtu.length();

        string cliOdu = "Odu ";
        found = aid.find(cliOdu);
        if (found == std::string::npos)
            return -1;
        std::size_t end = found;
        sId = aid.substr(start, end - start - 1);
        INFN_LOG(SeverityLevel::info) << __func__ << " sId: " << sId << endl;
    }
    return(stoi(sId,nullptr));
}

//
// Fetch Odu containter object and find the object that match with oduId in
// list
//
bool DcoOduAdapter::getOduObj( Odu *odu, int oduId, int *idx)
{
    // Save the OduId from setKey
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(odu);
    INFN_LOG(SeverityLevel::debug) << __func__ << " \n\n ****Read ... \n"  << endl;
    bool status = true;

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        odu = static_cast<Odu*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    // Search for the correct OduId.
    bool found = false;

    INFN_LOG(SeverityLevel::debug) << __func__ << " odu size " << odu->odu_size() << endl;

    for (*idx = 0; *idx < odu->odu_size(); (*idx)++)
    {
        INFN_LOG(SeverityLevel::debug) << __func__ << " odu id " << odu->odu(*idx).odu_id() << endl;
        if (odu->odu(*idx).odu_id() == oduId )
        {
            found = true;
            break;
        }
    }
    if (found == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " cannot find OduId in DCO  FAIL... OduId: " <<  oduId << endl;
        return false;
    }
    return status;
}

void DcoOduAdapter::setSimModeEn(bool enable)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (enable)
    {
        mspCrud = mspSimCrud;
        mIsGnmiSimModeEn = true;
    }
    else
    {
        mspCrud = mspSbCrud;
        mIsGnmiSimModeEn = false;
    }
}

bool DcoOduAdapter::getOduFaultCapa()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    DcoOduFault fault;
    DcoOduFault *pFault;

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&fault);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        pFault = static_cast<DcoOduFault*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }
    mFaultCapa.fac.clear();


    for ( int i = 0; i < pFault->capabilities().facility_faults_size(); i++ )
    {
        FaultCapability fault;
        fault.serviceAffecting = false;
        DcoOduFault_Capabilities_FacilityFaultsKey key;

        key = pFault->capabilities().facility_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.facility_faults().fault_name().value();
        if (key.facility_faults().has_bit_pos())
        {
            fault.bitPos = key.facility_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.facility_faults().has_fault_description())
        {
            fault.faultDescription = key.facility_faults().fault_description().value();
        }
        using facilitySeverity = dcoyang::infinera_dco_odu_fault::DcoOduFault_Capabilities_FacilityFaults;
        switch( key.facility_faults().severity())
        {
            case facilitySeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case facilitySeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case facilitySeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }


        using dir = dcoyang::infinera_dco_odu_fault::DcoOduFault_Capabilities_FacilityFaults;
        switch( key.facility_faults().direction())
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

        using loc = dcoyang::infinera_dco_odu_fault::DcoOduFault_Capabilities_FacilityFaults;
        switch( key.facility_faults().location())
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

        mFaultCapa.fac.push_back(fault);

    }

    // Get RxSapiMisMatch for TIM BDI injection
    for ( auto i = mFaultCapa.fac.begin(); i != mFaultCapa.fac.end(); i++)
    {
        if ( i->faultName == DCO_ODU_SAPI_MISM_NAME_STR && i->direction == FAULTDIRECTION_RX )
        {
            mRxSapiMismBitmap = 1ULL << i->bitPos;
            INFN_LOG(SeverityLevel::info) << __func__ << " Sapi: 0x"  << hex << mRxSapiMismBitmap << dec << endl;
            break;
        }
    }


    // Begin CDI
    mCdiCapa.cdiCapa.clear();


    for ( int i = 0; i < pFault->capabilities().client_defect_indicators_size(); i++ )
    {
        CdiCapability cdi;
        cdi.serviceAffecting = false;
        DcoOduFault_Capabilities_ClientDefectIndicatorsKey key;

        key = pFault->capabilities().client_defect_indicators(i);

        cdi.cdiKey = key.cdi();
        cdi.cdiName = key.client_defect_indicators().cdi_name().value();
        if (key.client_defect_indicators().has_bit_pos())
        {
            cdi.bitPos = key.client_defect_indicators().bit_pos().value();
        }
        else
        {
            cdi.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.client_defect_indicators().has_cdi_description())
        {
            cdi.faultDescription = key.client_defect_indicators().cdi_description().value();
        }
        using cdiSeverity = dcoyang::infinera_dco_odu_fault::DcoOduFault_Capabilities_ClientDefectIndicators;
        switch( key.client_defect_indicators().severity())
        {
        case cdiSeverity::SEVERITY_DEGRADE:
            cdi.severity = FAULTSEVERITY_DEGRADED;
            break;
        case cdiSeverity::SEVERITY_CRITICAL:
            cdi.severity = FAULTSEVERITY_CRITICAL;
            break;
        case cdiSeverity::SEVERITY_FAIL:
            cdi.severity = FAULTSEVERITY_FAILURE;
            break;
        default:
            cdi.severity = FAULTSEVERITY_UNSPECIFIED;
            break;
        }


        using dir = dcoyang::infinera_dco_odu_fault::DcoOduFault_Capabilities_ClientDefectIndicators;
        switch( key.client_defect_indicators().direction())
        {
        case dir::DIRECTION_RX:
            cdi.direction = FAULTDIRECTION_RX;
            break;
        case dir::DIRECTION_TX:
            cdi.direction = FAULTDIRECTION_TX;
            break;
        case dir::DIRECTION_NA:
            cdi.direction = FAULTDIRECTION_NA;
            break;
        default:
            cdi.direction = FAULTDIRECTION_UNSPECIFIED;
            break;
        }

        using loc = dcoyang::infinera_dco_odu_fault::DcoOduFault_Capabilities_ClientDefectIndicators;
        switch( key.client_defect_indicators().location())
        {
        case loc::LOCATION_NEAR_END:
            cdi.location = FAULTLOCATION_NEAR_END;
            break;
        case loc::LOCATION_FAR_END:
            cdi.location = FAULTLOCATION_FAR_END;
            break;
        case loc::LOCATION_NA:
            cdi.location = FAULTLOCATION_NA;
            break;
        default:
            cdi.location = FAULTLOCATION_UNSPECIFIED;
            break;
        }

        cdi.cdiEnum = CDI_UNSPECIFIED;
        if (cdi.cdiName == "UNKNOWN")
        {
            cdi.cdiEnum = CDI_UNKNOWN;
        }
        else if (cdi.cdiName == "NONE")
        {
            cdi.cdiEnum = CDI_NONE;
        }
        else if (cdi.cdiName == "LD")
        {
            cdi.cdiEnum = CDI_LOCAL_DEGRADED;
        }
        else if (cdi.cdiName == "RD")
        {
            cdi.cdiEnum = CDI_REMOTE_DEGRADED;
        }
        else if (cdi.cdiName == "FEC-SD")
        {
            cdi.cdiEnum = CDI_FEC_SIGNAL_DEGRADED;
        }

        mCdiCapa.cdiCapa.push_back(cdi);

    }


    // AF0 Add pos for CDI bits
    return status;
}

bool DcoOduAdapter::cacheOduAid(string aid, OduCommon *cfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    string cliOdu = "Odu ";
    std::size_t found = aid.find(cliOdu);
    if (found != std::string::npos) //cli case
    {
	    return true;
    }

    // Client ODU AID format: 1-5-T1, map to odu id 33; 1-5-T16 map to odu id 48
    if (cfg->type == ODUSUBTYPE_CLIENT) {
	    string aidPort = "-T";
	    found = aid.find(aidPort);
	    if (found != std::string::npos)
	    {
		    string sId = aid.substr(found + aidPort.length());
		    int clientOduId = stoi(sId,nullptr) + CLIENT_ODU_ID_OFFSET;
		    mOduIdToAid[clientOduId] = aid;
		    INFN_LOG(SeverityLevel::info) << __func__ << " Client Odu aid: " << aid << " with id: " << clientOduId << '\n';
		    return true;
	    }
	    INFN_LOG(SeverityLevel::error) << __func__ << " Bad Client Odu aid: " << aid << '\n';
	    return false;
    }

    //Line ODU
    if (cfg->loOduId < 1 || cfg->loOduId > MAX_ODU_ID)
    {
	    INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << cfg->loOduId << '\n';
	    return false;
    }
    // As agreed with L1 agent we use the LoOduId in config instead of AID
    // Cached it to use in update and delete operation
    // AID based on 1.25G time slot need to cached AID as we don't keep
    // track of 1.25 TS
    // AID 1-4-L1-1-ODUflexi-81
    // AID 1-5-L1-1-ODU4i-81

    INFN_LOG(SeverityLevel::info) << __func__ << " cfg loOduId: " << cfg->loOduId <<  endl;
    mOduIdToAid[cfg->loOduId] = aid;
    return true;
}

bool DcoOduAdapter::cacheOduAid(string aid, uint32 loOduId)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    string cliOdu = "Odu ";
    std::size_t found = aid.find(cliOdu);
    // Not cliOdu
    if (found == std::string::npos)
    {
        if (loOduId < 1 || loOduId > MAX_ODU_ID)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << loOduId << '\n';
            return false;
        }
        // As agreed with L1 agent we use the LoOduId in config instead of AID
        // Cached it to use in update and delete operation
        // AID based on 1.25G time slot need to cached AID as we don't keep
        // track of 1.25 TS
        // AID 1-4-L1-1-ODUflexi-81
        // AID 1-5-L1-1-ODU4i-81

        mOduIdToAid[loOduId] = aid;
    }
    return true;
}

int DcoOduAdapter::getCacheOduId(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    for ( int oduId = 1; oduId <= MAX_ODU_ID; oduId++)
    {
        if ( mOduIdToAid[oduId].empty() )
        {
            continue;
        }
        INFN_LOG(SeverityLevel::debug) << __func__ << " mOduIdToAid[oduId] " << mOduIdToAid[oduId] << endl;
        if ( mOduIdToAid[oduId] == aid )
        {
            return oduId;
        }
    }
    return -1;
}

string DcoOduAdapter::getCacheOduAid(uint32 oduId)
{
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return "UNKNOWN";   // how best to handle?
    }
    if (mOduIdToAid[oduId].empty())
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " NULL aid for oduId: " << oduId << '\n';
    }

    return mOduIdToAid[oduId];
}

void DcoOduAdapter::oduNotifyState(std::ostream& out, int oduId)
{
    out << "odu " << oduId << " TTI Rx: " << endl;
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        out << " 0x" << hex << (int)mTtiRx[oduId - 1].ttiRx[i] << dec;
    }
    out << "\n\nodu " << oduId << " cdi mask: 0x" << hex << (int)mCdi[oduId - 1] << dec << endl;
    out << "\ntti isNotified: " << EnumTranslate::toString(mTtiRx[oduId - 1].isNotified) << endl;
    out << "\nfault isNotified: " << EnumTranslate::toString(mOduFault[oduId - 1].isNotified) << endl;
    out << "notifyCount: " << mTtiRx[oduId - 1].notifyCount << endl;
    out << "eb overflow map size: " << ebOverflowMap.size() << endl;
    for (const auto& elem : ebOverflowMap)
    {
	    for (const auto& time : elem.second)
	    {
		    out << " aid: " << elem.first << " overflow at: " << time << endl;
	    }
    }
    out << "\n mOduPmSubEnable: " << EnumTranslate::toString(mOduPmSubEnable) << endl;
    out << "\n mOpStatus.opStatus: " << EnumTranslate::toString(mOpStatus[oduId - 1].opStatus) << endl;
    out << "\n mOpStatus.transactionId: 0x " << hex << mOpStatus[oduId - 1].transactionId << dec << endl;
    out << endl;
}

void DcoOduAdapter::oduNotifyStateAll(std::ostream& out)
{
    for(uint32 i = 1; i <= MAX_ODU_ID; i++)
    {
        oduNotifyState(out, i);
    }
}

uint8_t* DcoOduAdapter::getOduRxTti(const string aid)
{
	int oduId = aidToOduId(aid);
	if (oduId < 1 || oduId > MAX_ODU_ID)
	{
		INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
		return NULL;
	}
	return mTtiRx[oduId - 1].ttiRx;
}

void DcoOduAdapter::clearNotifyFlag()
{
    INFN_LOG(SeverityLevel::info) << __func__ << ", clear notify flag" << endl;
    for (int i = 0; i < MAX_ODU_ID; i++)
    {
        mOduFault[i].isNotified = false;
        mTtiRx[i].isNotified = false;
        mTtiRx[i].notifyCount = 0;
    }
}

Cdi DcoOduAdapter::getOduCdi(const string aid)
{
    int oduId = aidToOduId(aid);
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Odu Id: " << oduId << '\n';
        return CDI_UNKNOWN;
    }

    uint bitPos = 0x1;
    Cdi cdiStatus = CDI_UNKNOWN;

    for ( auto i = mCdiCapa.cdiCapa.begin(); i != mCdiCapa.cdiCapa.end(); i++)
    {
        if ( mCdi[oduId-1] & bitPos )
        {
            if (i->cdiEnum == CDI_FEC_SIGNAL_DEGRADED)
            {
                i->cdiEnum = CDI_LOCAL_DEGRADED;
            }

            if (i->cdiEnum == CDI_REMOTE_DEGRADED && cdiStatus == CDI_LOCAL_DEGRADED)
            {
                INFN_LOG(SeverityLevel::debug) << "get Odu cache mCdi[" << oduId-1 << "]=0x" << hex << (int)mCdi[oduId-1] << " cdi Enum: " << cdiStatus;
                return CDI_LOCAL_AND_REMOTE_DEGRADED;
            }
            else if (i->cdiEnum == CDI_LOCAL_DEGRADED && cdiStatus == CDI_REMOTE_DEGRADED)
            {
                INFN_LOG(SeverityLevel::debug) << "get Odu cache mCdi[" << oduId-1 << "]=0x" << hex << (int)mCdi[oduId-1] << " cdi Enum: " << cdiStatus;
                return CDI_LOCAL_AND_REMOTE_DEGRADED;
            }
            cdiStatus = i->cdiEnum;
        }
        bitPos <<= 0x1;
    }

    INFN_LOG(SeverityLevel::debug) << "get Odu cache mCdi[" << oduId-1 << "]=0x" << hex << (int)mCdi[oduId-1] << " cdi Enum: " << cdiStatus;

    return cdiStatus;
}

} /* DpAdapter */
