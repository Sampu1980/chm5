/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>
#include <sys/time.h>

#include <google/protobuf/text_format.h>
#include "DcoCarrierAdapter.h"
#include "DcoOtuAdapter.h"
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
using namespace tuning;
using namespace dcoyang::infinera_dco_otu;
using namespace dcoyang::infinera_dco_otu_fault;

using otuConfig = Otus_Otu_Config;
using loopBack = Otus_Otu_Config_LoopbackType;
using ms = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal;
using prbs = ::dcoyang::enums::InfineraDcoOtuPrbsMode;


namespace DpAdapter {

DpMsOTUPm DcoOtuAdapter::mOtuPms;
AdapterCacheFault  DcoOtuAdapter::mOtuFault[MAX_OTU_ID];
AdapterCacheTTI   DcoOtuAdapter::mTtiRx[MAX_OTU_ID];
const std::string DcoOtuAdapter::DCO_OTU_SAPI_MISM_NAME_STR = "SAPI-MISMATCH";
AdapterCacheOpStatus DcoOtuAdapter::mOpStatus[MAX_OTU_ID];

high_resolution_clock::time_point DcoOtuAdapter::mStart;
high_resolution_clock::time_point DcoOtuAdapter::mEnd;
int64_t DcoOtuAdapter::mStartSpan;
int64_t DcoOtuAdapter::mEndSpan;
int64_t DcoOtuAdapter::mDelta;
std::deque<int64_t> DcoOtuAdapter::mStartSpanArray(spanTime);
std::deque<int64_t> DcoOtuAdapter::mEndSpanArray(spanTime);
std::deque<int64_t> DcoOtuAdapter::mDeltaArray(spanTime);
bool DcoOtuAdapter::mOtuStateSubEnable = false;
bool DcoOtuAdapter::mOtuFltSubEnable   = false;
bool DcoOtuAdapter::mOtuPmSubEnable    = false;
bool DcoOtuAdapter::mOtuOpStatusSubEnable = false;
unordered_map<string, vector<std::time_t>> DcoOtuAdapter::ebOverflowMap;

// CHM6 Serdes speed to DCO is fixed at 50G per lane
// Lane map in HW are 0 based.  Yang Data Model is 1 based
const std::array<uint8_t, MAX_CLIENTS + 1> DcoOtuAdapter::mChm6_PortMap =
{ 0, 15, 13, 9, 11, 1, 3, 5, 7, 25, 27, 31, 29, 23, 21, 19, 17 }; // mChm6_PortMap

DcoOtuAdapter::DcoOtuAdapter()
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
    memset(mOtuRate, 0, sizeof (mOtuRate));
    memset(mbBdiInject, 0, sizeof (mbBdiInject));
    memset(mbFecCorrEnable, 0, sizeof (mbFecCorrEnable));

    for (int i = 0; i < MAX_OTU_ID; i++)
    {
        mTtiRx[i].ClearTTICache();
    }
}

DcoOtuAdapter::~DcoOtuAdapter()
 {

 }

bool DcoOtuAdapter::createOtuClientXpress(string aid)
{
    OtuCommon cfg;
    cfg.regen = true;
    // Set Term looback for Xpress Regen
    cfg.lpbk = LOOP_BACK_TYPE_TERMINAL;
    // Rest is default settings
    cfg.fecGenEnable = true;
    cfg.fecCorrEnable = true;
    cfg.type = OTUSUBTYPE_CLIENT;
    cfg.payLoad = OTUPAYLOAD_OTU4;
    cfg.channel = CARRIERCHANNEL_ONE;
    cfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.srvMode = SERVICEMODE_ADAPTATION;
    cfg.generatorTx = PRBSMODE_PRBS_NONE;
    cfg.generatorRx = PRBSMODE_PRBS_NONE;
    cfg.monitorTx = PRBSMODE_PRBS_NONE;
    cfg.monitorRx = PRBSMODE_PRBS_NONE;
    // Set transactionId to wait for DCO ack.
    cfg.transactionId = getTimeNanos();

    memset(&cfg.ttiTx[0], 0, sizeof(cfg.ttiTx) );

    INFN_LOG(SeverityLevel::info) << __func__ << " XPress case: Create OTU4 Client: " << " aid=" << aid << " transId: 0x" << hex << cfg.transactionId <<  dec << endl;

    if ( createOtu (aid, &cfg) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Fail Xpress create ODU4 Client  aid: " << aid << '\n';
        return false;
    }

    return true;
}

bool DcoOtuAdapter::createOtu(string aid, OtuCommon *cfg)
{
    bool status = false;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Create otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_name()->set_value(aid);

    // Set to default
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_fwd_tda_trigger()->set_value(false);
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_regen()->set_value(cfg->regen);
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_tid()->set_value(cfg->transactionId);

    bool fecGenEnable = false;
    bool fecCorrEnable = false;

    switch (cfg->type)
    {
        case OTUSUBTYPE_LINE:
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_sub_type(otuConfig::SUBTYPE_LINE);
            break;
        case OTUSUBTYPE_CLIENT:
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_sub_type(otuConfig::SUBTYPE_CLIENT);
            fecGenEnable = cfg->fecGenEnable;
            fecCorrEnable = cfg->fecCorrEnable;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Error Unknown SubType " << cfg->type <<  ", set to SUBTYPE_LINE by default" << endl;
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_sub_type(otuConfig::SUBTYPE_LINE);
            break;
    }

    mbFecCorrEnable[otuId] = fecCorrEnable;
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_fec_gen_enable()->set_value(fecGenEnable);
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_fec_correction_enable()->set_value(fecCorrEnable);

    uint16_t clientOtuLaneStart = 0;
    switch (cfg->payLoad)
    {
        case OTUPAYLOAD_OTUCNI:
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_payload_type(otuConfig::PAYLOADTYPE_OTUCNI);
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_interface_type(otuConfig::INTERFACETYPE_INTF_CAUI_4);
            break;
        case OTUPAYLOAD_OTU4:
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_payload_type(otuConfig::PAYLOADTYPE_OTU4);
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_interface_type(otuConfig::INTERFACETYPE_INTF_OTL_4_2);
            try {
                clientOtuLaneStart = mChm6_PortMap.at(otuId);
            } catch (std::out_of_range const& exc) {
                INFN_LOG(SeverityLevel::error) << __func__ << " OTU4 id: " << otuId << " access PortMap failed: " << exc.what() <<  endl;
            }
            break;
        default:
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_payload_type(otuConfig::PAYLOADTYPE_OTUCNI);
            otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_interface_type(otuConfig::INTERFACETYPE_INTF_CAUI_4);
            INFN_LOG(SeverityLevel::info) << __func__ << " Error Unknown PayLoad " << cfg->payLoad << ", set to OTUCNI/CAUI_4 by default" << endl;
            break;
    }

    uint32_t tsStart = 1;
    uint32_t tsNum = 8;
    double rate = 0.0;
    Otus_Otu_Config_CarrierChannel channel;
    switch (cfg->channel)
    {
        case CARRIERCHANNEL_ONE:
            rate = DcoCarrierAdapter::getCacheCarrierDataRate(1);
            mOtuRate[otuId] = (uint32_t) rate;
            if (rate < DCO_OTU_RATE_MIN || rate > DCO_OTU_RATE_MAX)
            {
                tsNum = 32;
            }
            else
            {
                tsNum = rate / DCO_TS_GRANULARITY;
            }
            channel = otuConfig::CARRIERCHANNEL_ONE;
            tsStart = (cfg->type == OTUSUBTYPE_CLIENT) ? clientOtuLaneStart : 1;
            tsNum = (cfg->type == OTUSUBTYPE_CLIENT) ? MAX_SERDES_LANENUM_100G : tsNum; //client otu has 2 timeslots
            INFN_LOG(SeverityLevel::info) << __func__ << " carrier 1, tsStart: " << tsStart << ", tsNum: " << tsNum << endl;
            for (int i = 0; i < tsNum ; i++)
            {
                ::ywrapper::UintValue *cLane = otu.mutable_otu(0)->mutable_otu()->mutable_config()->add_client_serdes_lane();
                cLane->set_value(tsStart+i);
                if (cfg->type == OTUSUBTYPE_CLIENT)
                {
                    configFir(tsStart+i, otu.mutable_otu(0)->mutable_otu()->mutable_config());
                }
            }
            break;
        case CARRIERCHANNEL_TWO:
            rate = DcoCarrierAdapter::getCacheCarrierDataRate(2);
            mOtuRate[otuId] = (uint32_t) rate;
            if (rate < DCO_OTU_RATE_MIN || rate > DCO_OTU_RATE_MAX)
            {
                tsNum = 32;
            }
            else
            {
                tsNum = rate / DCO_TS_GRANULARITY;
            }
            channel = otuConfig::CARRIERCHANNEL_TWO;
            tsStart = (cfg->type == OTUSUBTYPE_CLIENT) ? clientOtuLaneStart : 33;
            tsNum = (cfg->type == OTUSUBTYPE_CLIENT) ? MAX_SERDES_LANENUM_100G : tsNum; //client otu has 2 timeslots
            INFN_LOG(SeverityLevel::info) << __func__ << " carrier 2, tsStart: " << tsStart << ", tsNum: " << tsNum << endl;
            for (int i = 0; i < tsNum ; i++)
            {
                ::ywrapper::UintValue *cLane = otu.mutable_otu(0)->mutable_otu()->mutable_config()->add_client_serdes_lane();
                cLane->set_value(tsStart+i);
                if (cfg->type == OTUSUBTYPE_CLIENT)
                {
                    configFir(tsStart+i, otu.mutable_otu(0)->mutable_otu()->mutable_config());
                }
            }
            break;
        case CARRIERCHANNEL_BOTH:
            rate = DcoCarrierAdapter::getCacheCarrierDataRate(1);
            mOtuRate[otuId] = (uint32_t) rate;
            if (rate < DCO_OTU_RATE_MIN || rate > DCO_OTU_RATE_MAX)
            {
                tsNum = 32;
            }
            else
            {
                tsNum = rate / DCO_TS_GRANULARITY;
            }
            tsStart = (cfg->type == OTUSUBTYPE_CLIENT) ? clientOtuLaneStart : 1;
            tsNum = (cfg->type == OTUSUBTYPE_CLIENT) ? MAX_SERDES_LANENUM_100G : tsNum; //client otu has 2 timeslots
            INFN_LOG(SeverityLevel::info) << __func__ << " carrier both, one, tsStart: " << tsStart << ", tsNum: " << tsNum << endl;
            for (int i = 0; i < tsNum ; i++)
            {
                ::ywrapper::UintValue *cLane = otu.mutable_otu(0)->mutable_otu()->mutable_config()->add_client_serdes_lane();
                cLane->set_value(tsStart+i);
                if (cfg->type == OTUSUBTYPE_CLIENT)
                {
                    configFir(tsStart+i, otu.mutable_otu(0)->mutable_otu()->mutable_config());
                }
            }

            rate = DcoCarrierAdapter::getCacheCarrierDataRate(2);
            mOtuRate[otuId] += (uint32_t) rate;
            if (rate < DCO_OTU_RATE_MIN || rate > DCO_OTU_RATE_MAX)
            {
                tsNum = 32;
            }
            else
            {
                tsNum = rate / DCO_TS_GRANULARITY;
            }
            tsStart = (cfg->type == OTUSUBTYPE_CLIENT) ? clientOtuLaneStart : 33;
            tsNum = (cfg->type == OTUSUBTYPE_CLIENT) ? MAX_SERDES_LANENUM_100G : tsNum; //client otu has 2 timeslots
            INFN_LOG(SeverityLevel::info) << __func__ << " carrier both, two, tsStart: " << tsStart << ", tsNum: " << tsNum << endl;
            for (int i = 0; i < tsNum ; i++)
            {
                ::ywrapper::UintValue *cLane = otu.mutable_otu(0)->mutable_otu()->mutable_config()->add_client_serdes_lane();
                cLane->set_value(tsStart+i);
                if (cfg->type == OTUSUBTYPE_CLIENT)
                {
                    configFir(tsStart+i, otu.mutable_otu(0)->mutable_otu()->mutable_config());
                }
            }
            channel = otuConfig::CARRIERCHANNEL_BOTH;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Error Unknown CarrierChannel " << cfg->channel <<  endl;
            return false;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_carrier_channel(channel);

    loopBack lpbk;
    switch(cfg->lpbk)
    {
    case LOOP_BACK_TYPE_FACILITY:
        lpbk = otuConfig::LOOPBACKTYPE_FACILITY;
        break;
    case LOOP_BACK_TYPE_TERMINAL:
        lpbk = otuConfig::LOOPBACKTYPE_TERMINAL;
        break;
    case LOOP_BACK_TYPE_OFF:
    default:
        lpbk = otuConfig::LOOPBACKTYPE_NONE;
        break;
    }

    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_loopback_type(lpbk);

    ms signal;
    switch(cfg->forceMsTx)
    {
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_force_signal_tx(signal);

    switch(cfg->forceMsRx)
    {
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_force_signal_rx(signal);

    switch(cfg->autoMsTx)
    {
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_auto_signal_tx(signal);

    switch(cfg->autoMsRx)
    {
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_auto_signal_rx(signal);

    Otus_Otu_Config_ServiceMode sMode;
    switch(cfg->srvMode)
    {
        case SERVICEMODE_SWITCHING:
            sMode = otuConfig::SERVICEMODE_MODE_SWITCHING;
            break;
        case SERVICEMODE_TRANSPORT:
            sMode = otuConfig::SERVICEMODE_MODE_TRANSPORT;
            break;
        case SERVICEMODE_NONE:
            sMode = otuConfig::SERVICEMODE_MODE_NONE;
            break;
        case SERVICEMODE_ADAPTATION:
            sMode = otuConfig::SERVICEMODE_ADAPTATION;
            // L1ServiceAgent set OTUCni service_mode as
            // ADAPTATION and translated by DCO Adapter
            // to NONE else DCO create will fail
            if (OTUSUBTYPE_CLIENT != cfg->type)
            {
                sMode = otuConfig::SERVICEMODE_MODE_NONE;
            }
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set ServiceMode default" << endl;
            sMode = otuConfig::SERVICEMODE_MODE_NONE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_service_mode(sMode);

    prbs prbsMode;
    switch (cfg->generatorTx)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            cout << __func__ << " Warning unknown set PrbsGenTx default" << endl;
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_gen_tx(prbsMode);

    switch (cfg->monitorTx)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            cout << __func__ << " Warning unknown set PrbsMonTx default" << endl;
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_mon_tx(prbsMode);

    switch (cfg->generatorRx)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            cout << __func__ << " Warning unknown set PrbsGenRx default" << endl;
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_gen_rx(prbsMode);

    switch (cfg->monitorRx)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            cout << __func__ << " Warning unknown set PrbsMonRx default" << endl;
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
    }
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_mon_rx(prbsMode);

    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {

        Otus_Otu_Config_TxTtiKey *ttiKey = otu.mutable_otu(0)->mutable_otu()->mutable_config()->add_tx_tti();
        ttiKey->set_tti_byte_index(i+1);
        ttiKey->mutable_tx_tti()->mutable_tti()->set_value(cfg->ttiTx[i]);
    }

    // Clear Fault cache
    mOtuFault[otuId - 1].ClearFaultCache();

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);


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
    if (status == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Create FAIL " << '\n';
    }

    mOtuPms.AddPmData(aid, otuId);

    INFN_LOG(SeverityLevel::info) << "Clear OTU TIM BDI cache for AID: " << aid;
    mbBdiInject[otuId] = false;

    // One Xpress XCON command from host would create 10 - 12 commands
    // to DCO.  DCO could not keep up.  Dpms will wait for DCO to complete
    // status = false or TransactionId = 0 ==> don't wait
    if ( status == true && mOpStatus[otuId-1].waitForDcoAck(cfg->transactionId) == false)
    {
        return false;
    }
    return status;
}

bool DcoOtuAdapter::deleteOtu(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Delete otuId: " << otuId << endl;

    Otu otu;
    //only set id in Otu level, do not set id in Otu config level for Otu obj deletion
    otu.add_otu()->set_otu_id(otuId);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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
    mOtuPms.RemovePmData(otuId);
    // Clear Fault cache
    mOtuFault[otuId - 1].ClearFaultCache();
    // Clear TTI cache
    mTtiRx[otuId - 1].ClearTTICache();
    mOpStatus[otuId - 1].ClearOpStatusCache();

    mOtuRate[otuId] = 0;
    mbBdiInject[otuId] = false;
    return status;
}

bool DcoOtuAdapter::initializeOtu()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    bool status = false;

    // Get OTU fault capabilities
    status = getOtuFaultCapa();

    subscribeStreams();

    return status;
}

void DcoOtuAdapter::subscribeStreams()
{
    clearNotifyFlag();

    //subscribe to otu pm
    if(!mOtuPmSubEnable)
    {
        subOtuPm();
        mOtuPmSubEnable = true;
    }

    //subscribe to otu fault and state
    if(!mOtuFltSubEnable)
    {
        subOtuFault();
        mOtuFltSubEnable = true;
    }
    if(!mOtuStateSubEnable)
    {
        subOtuState();
        mOtuStateSubEnable = true;
    }
    //subscribe to otu op status
    if(!mOtuOpStatusSubEnable)
    {
        subOtuOpStatus();
        mOtuOpStatusSubEnable = true;
    }
}

bool DcoOtuAdapter::setOtuLoopBack(string aid, LoopBackType mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " LoopBackType :  " << (int) mode  << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    loopBack lpbk;

    switch(mode)
    {
    case LOOP_BACK_TYPE_FACILITY:
        lpbk = otuConfig::LOOPBACKTYPE_FACILITY;
        break;
    case LOOP_BACK_TYPE_TERMINAL:
        lpbk = otuConfig::LOOPBACKTYPE_TERMINAL;
        break;
    case LOOP_BACK_TYPE_OFF:
    default:
        lpbk = otuConfig::LOOPBACKTYPE_NONE;
        break;
    }

    otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_loopback_type(lpbk);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuAutoMS(string aid, Direction dir, MaintenanceSignal mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " otuId: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MS :  " << (int) mode  << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }
    ms signal;

    switch (mode)
    {
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " otuId: " << otuId << '\n';
            return false;
    }

    if (dir == DIRECTION_TX)
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_auto_signal_tx(signal);
    }
    else
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_auto_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuForceMS(string aid, Direction dir, MaintenanceSignal mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " otuId: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MS :  " << (int) mode  << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }
    ms signal;

    switch (mode)
    {
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
            break;
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
            break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
            break;
        case MAINTENANCE_SIGNAL_AIS:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
            break;
        case MAINTENANCE_SIGNAL_OCI:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
            break;
        case MAINTENANCE_SIGNAL_LCK:
            signal = ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " otuId: " << otuId << '\n';
            return false;
    }
    if (dir == DIRECTION_TX)
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_force_signal_tx(signal);
    }
    else
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_ms_force_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuPrbsMon(string aid, Direction dir, PrbsMode mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " otuId: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " mode :  " << (int) mode  << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }
    prbs prbsMode;

    switch (mode)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " otuId: " << otuId << '\n';
            return false;
    }

    if (dir == DIRECTION_TX)
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_mon_tx(prbsMode);
    }
    else
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_mon_rx(prbsMode);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuPrbsGen(string aid, Direction dir, PrbsMode mode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " otuId: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " mode :  " << (int) mode  << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }
    prbs prbsMode;

    switch (mode)
    {
        case PRBSMODE_PRBS_NONE:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_NONE;
            break;
        case PRBSMODE_PRBS_X31:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31;
            break;
        case PRBSMODE_PRBS_X31_INV:
            prbsMode = prbs::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " otuId: " << otuId << '\n';
            return false;
    }

    if (dir == DIRECTION_TX)
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_gen_tx(prbsMode);
    }
    else
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->set_prbs_gen_rx(prbsMode);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuFecGenEnable(string aid, bool enable)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " fec_gen_enable :  " << (int) enable  << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_fec_gen_enable()->set_value(enable);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuFecCorrEnable(string aid, bool enable)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " fec_corr_enable :  " << (int) enable  << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    mbFecCorrEnable[otuId] = enable;
    otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_fec_correction_enable()->set_value(enable);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuTti(string aid, uint8_t *tti)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        Otus_Otu_Config_TxTtiKey *ttiKey = otu.mutable_otu(0)->mutable_otu()->mutable_config()->add_tx_tti();
        ttiKey->set_tti_byte_index(i+1);  // Yang is one base
        ttiKey->mutable_tx_tti()->mutable_tti()->set_value(tti[i]);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

// the fault inject bitmap is based the fault capabilities bit position.
bool DcoOtuAdapter::setOtuFaultStatus(string aid, DataPlane::Direction dir, uint64_t injectFaultBitmap)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    // Fix timing issue. This function is being call from collector thread
    // when an OTU is deleted from config thread, it may get call after the
    // OTU is already deleted from DCO.

    // We set mOtuRate on create and clear on delete.
    if ( mOtuRate[otuId] == 0)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << "OTU is already deleted/notExist: " << otuId << '\n';
        return true;
    }

    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " otuId: " << otuId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " dir: " << dir << " fault bitmap :0x" << hex << injectFaultBitmap  << dec << " otuId: " << otuId << endl;

    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    if (dir == DIRECTION_TX)
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_tx_fault_status()->set_value(injectFaultBitmap);
    }
    else
    {
        otu.mutable_otu(0)->mutable_otu()->mutable_config()->mutable_rx_fault_status()->set_value(injectFaultBitmap);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);
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

bool DcoOtuAdapter::setOtuTimBdi(string aid, bool bInject)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    // Fix timing issue. This function is being call from collector thread
    // when an OTU is deleted from config thread, it may get call after the
    // OTU is already deleted from DCO.

    // We set mOtuRate on create and clear on delete.
    if ( mOtuRate[otuId] == 0)
    {
        INFN_LOG(SeverityLevel::debug) << __func__ << "OTU is already deleted: " << otuId << '\n';
        return true;
    }

    // DpMgr evaluate fault every seconds cache flag so we don't send same
    //  thing down to DCO
    if (mbBdiInject[otuId] == bInject)
    {
        return true;
    }

    mbBdiInject[otuId] = bInject;

    if (bInject == true)
    {
        if (setOtuFaultStatus( aid, DIRECTION_RX, mRxSapiMismBitmap) == false)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Fail to inject BDI " << " Otu: "  << otuId << endl;
            return false;
        }
        INFN_LOG(SeverityLevel::info) << "Set OTU TIM BDI state for AID: " << aid;
    }
    else
    {
        if (setOtuFaultStatus( aid, DIRECTION_RX, 0) == false)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Fail to clear BDI injection" << " Otu: "  << otuId << endl;
            return false;
        }
        INFN_LOG(SeverityLevel::info) << "Clear OTU TIM BDI state for AID: " << aid;
    }
    return status;
}


//
//  Get/Read methods
//

void DcoOtuAdapter::dumpAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    Otu otuMsg;

    uint32 cfgId = 1;
    if (setKey(&otuMsg, cfgId) == false)
    {
        out << "Failed to set key" << endl;

        return;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otuMsg);

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

    for (int idx = 0; idx < otuMsg.otu_size(); idx++)
    {
        if (!otuMsg.otu(idx).otu().has_config())
        {
            out << "*** Otu Config not present for idx: " << idx << " Skipping!" << endl;

            continue;
        }

        uint32 objId = (uint32)otuMsg.otu(idx).otu_id();

        string otuAid;
        if (otuMsg.otu(idx).otu().config().has_name())
        {
            otuAid = otuMsg.otu(idx).otu().config().name().value();

            INFN_LOG(SeverityLevel::debug) << "Otu has name: " << otuAid;
        }
        else
        {
            otuAid = IdGen::genOtuAid(objId);

            INFN_LOG(SeverityLevel::debug) << "Otu has NO NAME. Using old genID created AID: " << otuAid;
        }

        OtuCommon cmn;

        translateConfig(otuMsg.otu(idx).otu().config(), cmn);

        out << "*** Otu Config Info for OtuID: " << objId << " Aid: " << otuAid << endl;

        dumpConfigInfo(out, cmn);

        out << "*** Otu Fault Info from cache for OtuID: " << objId << " Aid: " << otuAid << endl;

        if (otuFaultInfo(out, otuAid) == false)
        {
            out << "otuFaultInfo Failed for otuAid " << otuAid << endl;
        }

        if (otuMsg.otu(idx).otu().has_state())
        {
            OtuStatus stat;

            translateStatus(otuMsg.otu(idx).otu().state(), stat);

            out << "*** Otu State Info for OtuID: " << objId << " Aid: " << otuAid << endl;

            dumpStatusInfo(out, stat);
        }
        else
        {
            out << "*** Otu State not present for OtuID: " << objId << " Aid: " << otuAid << endl;
        }

        out << "*** Otu Notify State from cache for OtuID: " << objId << " Aid: " << otuAid << endl;

        otuNotifyState(out, otuMsg.otu(idx).otu_id());

        out << "*** Otu PM Info from cache for OtuID: " << objId << " Aid: " << otuAid << endl;

        if (otuPmInfo(out, otuMsg.otu(idx).otu_id()) == false)
        {
            out << "otuPmInfo Failed for otuAid " << otuAid << endl;
        }
    }
}
bool DcoOtuAdapter::otuConfigInfo(ostream& out, string aid)
{
    OtuCommon cfg;
    memset(&cfg, 0, sizeof(cfg));
    bool status = getOtuConfig(aid, &cfg);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }

    out << " Otu Config Info: " << aid  << endl;

    dumpConfigInfo(out, cfg);

    return status;
}

void DcoOtuAdapter::dumpConfigInfo(std::ostream& out, const OtuCommon& cfg)
{
    out << " OTU Name (AID) : " << cfg.name << endl;
    out << " Sub Type:        " << EnumTranslate::toString(cfg.type) << endl;
    out << " Pay Load:        " << EnumTranslate::toString(cfg.payLoad) << endl;
    out << " FEC Gen enable:  " << EnumTranslate::toString(cfg.fecGenEnable) << endl;
    out << " FEC Corr enable: " << EnumTranslate::toString(cfg.fecCorrEnable) << endl;
    out << " LoopBack:        " << EnumTranslate::toString(cfg.lpbk) << endl;
    out << " Channel:         " << EnumTranslate::toString(cfg.channel) << endl;
    out << " Intf Type:       " << EnumTranslate::toString(cfg.intfType) << endl;
    out << " Service Mode:    " << EnumTranslate::toString(cfg.srvMode) << endl;
    out << " Force MS Tx:     " << EnumTranslate::toString(cfg.forceMsTx) << endl;
    out << " Force MS Rx:     " << EnumTranslate::toString(cfg.forceMsRx) << endl;
    out << " Auto MS Tx:      " << EnumTranslate::toString(cfg.autoMsTx) << endl;
    out << " Auto MS Rx:      " << EnumTranslate::toString(cfg.autoMsRx) << endl;
    out << " PRBS Mon Tx:     " << EnumTranslate::toString(cfg.monitorTx) << endl;
    out << " PRBS Mon Rx:     " << EnumTranslate::toString(cfg.monitorRx) << endl;
    out << " PRBS Gen Tx:     " << EnumTranslate::toString(cfg.generatorTx) << endl;
    out << " PRBS Gen Rx:     " << EnumTranslate::toString(cfg.generatorRx) << endl;
    out << " Frame Rate:      " << getOtuFrameRate(cfg.name) << endl;
    out << " TTI Tx:         ";
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        out << " 0x" << std::hex << (int)cfg.ttiTx[i];
        if (((i+1) % 16) == 0)
        {
            out << endl << "                 ";
        }
    }
    out << std::dec << endl;
    out << " Regen            : " << EnumTranslate::toString(cfg.regen) << endl;
    out << " transactionId    : 0x" << hex <<cfg.transactionId << dec << endl;
    if (cfg.type != OTUSUBTYPE_CLIENT) return;
    out << " Serdes Lane:     ";
    for (int i = 0; i < cfg.clientSerdesLane.size(); i++ )
    {
	out << (int)cfg.clientSerdesLane[i] << "   ";
    }
    out << " Client Serdes Lane Config: " << endl << endl;
    out << " txPol rxPol Pre2 Pre1 main post1 post2 post3" << endl;
    for (int i = 0; i < cfg.serdesCfg.size(); i++ )
    {
        out << " " << setw(5) << cfg.serdesCfg[i].txPolarity << " " << setw(5) << cfg.serdesCfg[i].rxPolarity <<
		" " << setw(4) << cfg.serdesCfg[i].txPre2 << " " << setw(4) << cfg.serdesCfg[i].txPre1 <<
		" " << setw(4) << cfg.serdesCfg[i].txMain << " " << setw(5) << cfg.serdesCfg[i].txPost1 <<
		" " << setw(5) << cfg.serdesCfg[i].txPost2 << " " << setw(5) << cfg.serdesCfg[i].txPost3 << endl;
    }
    out << endl;
    out << " Tx Fault Status (inject): 0x" << hex << cfg.injectTxFaultStatus << dec << endl;
    out << " Rx Fault Status (inject): 0x" << hex << cfg.injectRxFaultStatus << dec << endl;
}

bool DcoOtuAdapter::otuConfigInfo(ostream& out)
{
    TypeMapOtuConfig mapCfg;

    if (!getOtuConfig(mapCfg))
    {
        out << "*** FAILED to get Otu Status!!" << endl;
        return false;
    }

    out << "*** Otu Config Info ALL: " << endl;

    for(auto& it : mapCfg)
    {
        out << " Otu Config Info: "  << it.first << endl;
        dumpConfigInfo(out, *it.second);
        out << endl;
    }

    return true;
}

bool DcoOtuAdapter::otuStatusInfo(ostream& out, string aid)
{
    OtuStatus stat;
    memset(&stat, 0, sizeof(stat));
    bool status = getOtuStatus(aid, &stat);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }

    out << " Otu Status Info:   " << aid << endl;

    dumpStatusInfo(out, stat);

    return status;
}

void DcoOtuAdapter::dumpStatusInfo(std::ostream& out, const OtuStatus& stat)
{
    out << " Sub Type:          " << EnumTranslate::toString(stat.otu.type) << endl;
    out << " Pay Load:          " << EnumTranslate::toString(stat.otu.payLoad) << endl;
    out << " FEC Gen enable:    " << EnumTranslate::toString(stat.otu.fecGenEnable) << endl;
    out << " FEC Corr enable:   " << EnumTranslate::toString(stat.otu.fecCorrEnable) << endl;
    out << " LoopBack:          " << EnumTranslate::toString(stat.otu.lpbk) << endl;
    out << " Channel:           " << EnumTranslate::toString(stat.otu.channel) << endl;
    out << " Intf Type:         " << EnumTranslate::toString(stat.otu.intfType) << endl;
    out << " Service Mode:      " << EnumTranslate::toString(stat.otu.srvMode) << endl;
    out << " Force MS Tx:       " << EnumTranslate::toString(stat.otu.forceMsTx) << endl;
    out << " Force MS Rx:       " << EnumTranslate::toString(stat.otu.forceMsRx) << endl;
    out << " Auto MS Tx:        " << EnumTranslate::toString(stat.otu.autoMsTx) << endl;
    out << " Auto MS Rx:        " << EnumTranslate::toString(stat.otu.autoMsRx) << endl;
    out << " PRBS Mon Tx:       " << EnumTranslate::toString(stat.otu.monitorTx) << endl;
    out << " PRBS Mon Rx:       " << EnumTranslate::toString(stat.otu.monitorRx) << endl;
    out << " PRBS Gen Tx:       " << EnumTranslate::toString(stat.otu.generatorTx) << endl;
    out << " PRBS Gen Rx:       " << EnumTranslate::toString(stat.otu.generatorRx) << endl;
    out << " TTI Tx:           ";
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        //out << " 0x" << setfill('0') << setw(2) << hex << (int)stat.otu.ttiTx[i];
        out << " 0x" << std::hex << (int)stat.otu.ttiTx[i];
        if (((i+1) % 16) == 0)
        {
            out << endl << "                   ";
        }
    }
    out << std::dec << endl;
    out << " TTI Rx:           ";
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        //out << " 0x" << setfill('0') << setw(2) << hex << (int)stat.ttiRx[i];
        out << " 0x" << std::hex << (int)stat.ttiRx[i];
        if (((i+1) % 16) == 0)
        {
            out << endl << "                   ";
        }
    }
    out << std::dec << endl;
    out << " Facility fault:    0x" << hex << stat.faultFac << dec << endl;
    out << " Delay Measurement: "   << stat.delayMeasurement << endl;
    out << " Rate:              "   << stat.rate << endl;
    out << " Tx Fault Status (inject): 0x" << hex << stat.otu.injectTxFaultStatus << dec << endl;
    out << " Rx Fault Status (inject): 0x" << hex << stat.otu.injectRxFaultStatus << dec << endl;
    out << " mOtuStateSubEnable: " << EnumTranslate::toString(mOtuStateSubEnable) << endl;
    out << "\n mOtuOpStatusSubEnable: " << EnumTranslate::toString(mOtuOpStatusSubEnable) << endl;

    int otuId = aidToOtuId(stat.otu.name);  // todo: verify....
    if (!(otuId < 1 || otuId > MAX_OTU_ID))
    {
        out << " mbFecCorrEnable:    " << EnumTranslate::toString(mbFecCorrEnable[otuId]) << endl;
    }

    out << " Regen            : " << EnumTranslate::toString(stat.otu.regen) << endl;
    out << " TransactionId    : 0x" << hex << stat.otu.transactionId << dec << endl;
}

bool DcoOtuAdapter::otuFaultInfo(ostream& out, string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    OtuFault fault;
    status = getOtuFault(aid, &fault);
    if ( status == false )
    {
        out << __func__ << " Get OTU FAC fault FAIL "  << endl;
        return status;
    }

    out << endl;
    out << "OTU FACILITY Fault: 0x" << hex << fault.facBmp << dec << endl;
    uint bitPos = 0;
    for ( auto i = fault.fac.begin(); i != fault.fac.end(); i++, bitPos++)
    {
        out << " Fault: " << left << std::setw(13) << i->faultName << " Bit" << left << std::setw(2) << bitPos << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }
    out << "\n mOtuFltSubEnable: " << EnumTranslate::toString(mOtuFltSubEnable) << endl;
    return status;
}

bool DcoOtuAdapter::getOtuConfig(string aid, OtuCommon *cfg)
{
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " otuId: " << otuId << endl;
    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOtuObj( &otu, otuId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Otu Object  FAIL "  << endl;
        return status;
    }

    if (otu.otu(idx).otu().has_config() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO Config Empty ... OtuId: " <<  otuId << endl;
        return false;
    }

    translateConfig(otu.otu(idx).otu().config(), *cfg);

    return status;
} // getOtuConfig

void DcoOtuAdapter::translateConfig(const OtuConfig& dcoCfg, OtuCommon& adCfg)
{
    switch (dcoCfg.sub_type())
    {
        case otuConfig::SUBTYPE_LINE:
            adCfg.type = OTUSUBTYPE_LINE;
            break;
        case otuConfig::SUBTYPE_CLIENT:
            adCfg.type = OTUSUBTYPE_CLIENT;
            break;
        default:
            adCfg.type = OTUSUBTYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.sub_type() << endl;
            return;
    }

    switch (dcoCfg.payload_type())
    {
        case otuConfig::PAYLOADTYPE_OTUCNI:
            adCfg.payLoad = OTUPAYLOAD_OTUCNI;
            break;
        case otuConfig::PAYLOADTYPE_OTU4:
            adCfg.payLoad = OTUPAYLOAD_OTU4;
            break;
        default:
            adCfg.payLoad = OTUPAYLOAD_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.payload_type() << endl;
            return;
    }

    switch (dcoCfg.interface_type())
    {
        case otuConfig::INTERFACETYPE_INTF_GAUI:
            adCfg.intfType = INTERFACETYPE_GAUI;
            break;
        case otuConfig::INTERFACETYPE_INTF_CAUI_4:
            adCfg.intfType = INTERFACETYPE_CAUI_4;
            break;
        case otuConfig::INTERFACETYPE_INTF_LAUI_2:
            adCfg.intfType = INTERFACETYPE_LAUI_2;
            break;
        case otuConfig::INTERFACETYPE_INTF_OTL_4_2:
            adCfg.intfType = INTERFACETYPE_OTL_4_2;
            break;
        default:
            adCfg.intfType = INTERFACETYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.interface_type() << endl;
            return;
    }

    if (dcoCfg.has_name())
    {
        adCfg.name = dcoCfg.name().value();
    }
    if (dcoCfg.has_fec_gen_enable())
    {
        adCfg.fecGenEnable = dcoCfg.fec_gen_enable().value();
    }
    if (dcoCfg.has_fec_correction_enable())
    {
        adCfg.fecCorrEnable = dcoCfg.fec_correction_enable().value();
    }
    switch (dcoCfg.loopback_type())
    {
        case otuConfig::LOOPBACKTYPE_NONE:
            adCfg.lpbk = LOOP_BACK_TYPE_OFF;
            break;
        case otuConfig::LOOPBACKTYPE_FACILITY:
            adCfg.lpbk = LOOP_BACK_TYPE_FACILITY;
            break;
        case otuConfig::LOOPBACKTYPE_TERMINAL:
            adCfg.lpbk = LOOP_BACK_TYPE_TERMINAL;
            break;
        default:
            adCfg.lpbk = LOOP_BACK_TYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.loopback_type() << endl;
            break;
    }
    switch (dcoCfg.carrier_channel())
    {
        case otuConfig::CARRIERCHANNEL_ONE:
            adCfg.channel = CARRIERCHANNEL_ONE;
            break;
        case otuConfig::CARRIERCHANNEL_TWO:
            adCfg.channel = CARRIERCHANNEL_TWO;
            break;
        case otuConfig::CARRIERCHANNEL_BOTH:
            adCfg.channel = CARRIERCHANNEL_BOTH;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.carrier_channel() << endl;
            adCfg.channel = CARRIERCHANNEL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_force_signal_tx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.ms_force_signal_tx() << endl;
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_force_signal_rx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.ms_force_signal_rx() << endl;
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoCfg.ms_auto_signal_rx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.ms_auto_signal_rx() << endl;
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_auto_signal_tx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.ms_auto_signal_tx() << endl;
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoCfg.service_mode())
    {
        case otuConfig::SERVICEMODE_MODE_SWITCHING:
            adCfg.srvMode = SERVICEMODE_SWITCHING;
            break;
        case otuConfig::SERVICEMODE_MODE_TRANSPORT:
            adCfg.srvMode = SERVICEMODE_TRANSPORT;
            break;
        case otuConfig::SERVICEMODE_MODE_NONE:
            adCfg.srvMode = SERVICEMODE_ADAPTATION;
            break;
        case otuConfig::SERVICEMODE_ADAPTATION:
            adCfg.srvMode = SERVICEMODE_ADAPTATION;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoCfg.service_mode() << endl;
            adCfg.srvMode = SERVICEMODE_UNSPECIFIED;
            break;
    }

    for (int i = 0; i < dcoCfg.client_serdes_lane_size(); i++)
    {
        adCfg.clientSerdesLane.push_back(dcoCfg.client_serdes_lane(i).value());
    }

    for (int i = 0; i < dcoCfg.client_serdes_pol_fir_size(); i++)
    {
        auto dcoSerdesCfg = dcoCfg.client_serdes_pol_fir(i).client_serdes_pol_fir();
        ClientSerdesCfg	sdCfg;
        sdCfg.txPolarity = dcoSerdesCfg.tx_lane_polarity().value();
        sdCfg.rxPolarity = dcoSerdesCfg.rx_lane_polarity().value();
        sdCfg.txPre2 = dcoSerdesCfg.tx_fir_precursor_2().value();
        sdCfg.txPre1 = dcoSerdesCfg.tx_fir_precursor_1().value();
        sdCfg.txMain = dcoSerdesCfg.tx_fir_main_tap().value();
        sdCfg.txPost1 = dcoSerdesCfg.tx_fir_postcursor_1().value();
        sdCfg.txPost2 = dcoSerdesCfg.tx_fir_postcursor_2().value();
        sdCfg.txPost3 = dcoSerdesCfg.tx_fir_postcursor_3().value();
        adCfg.serdesCfg.push_back(sdCfg);
    }

    for (int i = 0; i < dcoCfg.tx_tti_size(); i++)
    {
        // TTI yang is one based
        adCfg.ttiTx[i] = dcoCfg.tx_tti(i).tx_tti().tti().value();
    }

    if (dcoCfg.has_tx_fault_status())
    {
        adCfg.injectTxFaultStatus = dcoCfg.tx_fault_status().value();
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

bool DcoOtuAdapter::getOtuConfig(TypeMapOtuConfig& mapCfg)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    Otu otu;

    uint32 cfgId = 1;
    if (setKey(&otu, cfgId) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);

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

    INFN_LOG(SeverityLevel::info) << "size: " << otu.otu_size();

    for (int idx = 0; idx < otu.otu_size(); idx++)
    {
        string otuAid;
        if (otu.otu(idx).otu().config().has_name())
        {
            otuAid = otu.otu(idx).otu().config().name().value();

            INFN_LOG(SeverityLevel::info) << "Otu has name: " << otuAid;
        }
        else
        {
            otuAid = IdGen::genOtuAid((uint32)otu.otu(idx).otu().config().otu_id().value());

            INFN_LOG(SeverityLevel::info) << "Otu has NO NAME. Using old genID created AID: " << otuAid;
        }

        if (otu.otu(idx).otu().config().has_regen())
        {
            if (otu.otu(idx).otu().config().regen().value() == true &&
                otu.otu(idx).otu().config().sub_type() == true)
            {
                INFN_LOG(SeverityLevel::info) << "Skip update: This is HAL create OTU client Regen AID: " << otuAid << " IDX: " << idx << endl;
                // This is a regen client, don't update InstallConfig
                continue;
            }
        }

        mapCfg[otuAid] = make_shared<OtuCommon>();
        translateConfig(otu.otu(idx).otu().config(), *mapCfg[otuAid]);
    }

    return status;
}

bool DcoOtuAdapter::getOtuStatus(string aid, OtuStatus *stat)
{
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " otuId: " << otuId << endl;
    Otu otu;

    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOtuObj( &otu, otuId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Otu Object  FAIL "  << endl;
        return status;
    }

    if (otu.otu(idx).otu().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OtuId: " <<  otuId << endl;
        return false;
    }

    translateStatus(otu.otu(idx).otu().state(), *stat, true);

    return status;
} // getOtuStatus

void DcoOtuAdapter::translateStatus(const OtuStat& dcoStat, OtuStatus& stat, bool updateCache)
{
    uint32 otuId = dcoStat.otu_id().value();

    switch (dcoStat.sub_type())
    {
        case otuConfig::SUBTYPE_LINE:
            stat.otu.type = OTUSUBTYPE_LINE;
            break;
        case otuConfig::SUBTYPE_CLIENT:
            stat.otu.type = OTUSUBTYPE_CLIENT;
            break;
        default:
            stat.otu.type = OTUSUBTYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << " Unknown: " << dcoStat.sub_type() << endl;
    }

    switch (dcoStat.payload_type())
    {
        case otuConfig::PAYLOADTYPE_OTUCNI:
            stat.otu.payLoad = OTUPAYLOAD_OTUCNI;
            break;
        case otuConfig::PAYLOADTYPE_OTU4:
            stat.otu.payLoad = OTUPAYLOAD_OTU4;
            break;
        default:
            stat.otu.payLoad = OTUPAYLOAD_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoStat.payload_type() << endl;
            break;
    }

    switch (dcoStat.interface_type())
    {
        case otuConfig::INTERFACETYPE_INTF_GAUI:
            stat.otu.intfType = INTERFACETYPE_GAUI;
            break;
        case otuConfig::INTERFACETYPE_INTF_CAUI_4:
            stat.otu.intfType = INTERFACETYPE_CAUI_4;
            break;
        case otuConfig::INTERFACETYPE_INTF_LAUI_2:
            stat.otu.intfType = INTERFACETYPE_LAUI_2;
            break;
        case otuConfig::INTERFACETYPE_INTF_OTL_4_2:
            stat.otu.intfType = INTERFACETYPE_OTL_4_2;
            break;
        default:
            stat.otu.intfType = INTERFACETYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown: " << dcoStat.interface_type() << endl;
            break;
    }

    if (dcoStat.has_fec_gen_enable())
    {
        stat.otu.fecGenEnable = dcoStat.fec_gen_enable().value();
    }
    if (dcoStat.has_fec_correction_enable())
    {
        stat.otu.fecCorrEnable = dcoStat.fec_correction_enable().value();
    }
    switch (dcoStat.loopback_type())
    {
        case otuConfig::LOOPBACKTYPE_NONE:
            stat.otu.lpbk = LOOP_BACK_TYPE_OFF;
            break;
        case otuConfig::LOOPBACKTYPE_FACILITY:
            stat.otu.lpbk = LOOP_BACK_TYPE_FACILITY;
            break;
        case otuConfig::LOOPBACKTYPE_TERMINAL:
            stat.otu.lpbk = LOOP_BACK_TYPE_TERMINAL;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.loopback_type() << endl;
            stat.otu.lpbk = LOOP_BACK_TYPE_UNSPECIFIED;
            break;
    }
    switch (dcoStat.carrier_channel())
    {
        case otuConfig::CARRIERCHANNEL_ONE:
            stat.otu.channel = CARRIERCHANNEL_ONE;
            break;
        case otuConfig::CARRIERCHANNEL_TWO:
            stat.otu.channel = CARRIERCHANNEL_TWO;
            break;
        case otuConfig::CARRIERCHANNEL_BOTH:
            stat.otu.channel = CARRIERCHANNEL_BOTH;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.carrier_channel() << endl;
            stat.otu.channel = CARRIERCHANNEL_UNSPECIFIED;
            break;
    }
    switch (dcoStat.ms_force_signal_tx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.ms_force_signal_tx() << endl;
            stat.otu.forceMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoStat.ms_force_signal_rx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.ms_force_signal_rx() << endl;
            stat.otu.forceMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoStat.ms_auto_signal_rx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.ms_auto_signal_rx() << endl;
            stat.otu.autoMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoStat.ms_auto_signal_tx())
    {
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE:
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_LASEROFF;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_PRBS_X31_INV;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_AIS;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_OCI;
            break;
        case ms::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_LCK;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.ms_auto_signal_tx() << endl;
            stat.otu.autoMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }

    switch (dcoStat.service_mode())
    {
        case otuConfig::SERVICEMODE_MODE_SWITCHING:
            stat.otu.srvMode = SERVICEMODE_SWITCHING;
            break;
        case otuConfig::SERVICEMODE_MODE_TRANSPORT:
            stat.otu.srvMode = SERVICEMODE_TRANSPORT;
            break;
        case otuConfig::SERVICEMODE_MODE_NONE:
            stat.otu.srvMode = SERVICEMODE_NONE;
            break;
        case otuConfig::SERVICEMODE_ADAPTATION:
            stat.otu.srvMode = SERVICEMODE_ADAPTATION;
            break;
        default:
            INFN_LOG(SeverityLevel::debug) << __func__ << "Unknown: " << dcoStat.service_mode() << endl;
            stat.otu.srvMode = SERVICEMODE_UNSPECIFIED;
            break;
    }

    for (int i = 0; i < dcoStat.tx_tti_size(); i++)
    {
        stat.otu.ttiTx[i] = dcoStat.tx_tti(i).tx_tti().tti().value();
    }

    for (int i = 0; i < dcoStat.rx_tti_size(); i++)
    {
        stat.ttiRx[i] = dcoStat.rx_tti(i).rx_tti().tti().value();
    }

    // Until DCO support correct bit rate
#if 0
    if (dcoStat.has_rate())
    {
        int64 digit = dcoStat.rate().digits();
        uint32 precision = dcoStat.rate().precision();
        stat.rate = digit/pow(10, precision);
    }
#endif

    // Get OTUCNi cached rate
    stat.rate = mOtuRate[otuId];

    if (dcoStat.has_dm())
    {
        int64 digit = dcoStat.dm().digits();
        uint32 precision = dcoStat.dm().precision();
        stat.delayMeasurement = digit/pow(10, precision);
    }
    if (dcoStat.has_tx_fault_status())
    {
        stat.faultTx = dcoStat.tx_fault_status().value();
    }
    if (dcoStat.has_tx_fault_status())
    {
        stat.otu.injectTxFaultStatus = dcoStat.tx_fault_status().value();
    }
    if (dcoStat.has_rx_fault_status())
    {
        stat.otu.injectRxFaultStatus = dcoStat.rx_fault_status().value();
    }
    if (updateCache && dcoStat.has_facility_fault())
    {
        if ((otuId - 1) < MAX_OTU_ID)
        {
            stat.faultFac = dcoStat.facility_fault().value();
            mOtuFault[otuId - 1].faultBitmap = stat.faultFac;
        }
    }

    if (dcoStat.has_regen())
    {
        stat.otu.regen = dcoStat.regen().value();
    }
    if (dcoStat.has_tid())
    {
        stat.otu.transactionId = dcoStat.tid().value();
    }
}

bool DcoOtuAdapter::getOtuFaultTx(string aid, uint64_t *fault)
{
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " otuId: " << otuId << endl;
    Otu otu;
    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOtuObj( &otu, otuId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Otu Object  FAIL "  << endl;
        return status;
    }

    if (otu.otu(idx).otu().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OtuId: " <<  otuId << endl;
        return false;
    }
    if (otu.otu(idx).otu().state().has_tx_fault_status())
    {
        *fault = otu.otu(idx).otu().state().tx_fault_status().value();
    }

    return status;
}

bool DcoOtuAdapter::getOtuFaultRx(string aid, uint64_t *fault)
{
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " otuId: " << otuId << endl;
    Otu otu;
    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getOtuObj( &otu, otuId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Otu Object  FAIL "  << endl;
        return status;
    }

    if (otu.otu(idx).otu().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OtuId: " <<  otuId << endl;
        return false;
    }

    if (otu.otu(idx).otu().state().has_rx_fault_status())
    {
        *fault = otu.otu(idx).otu().state().rx_fault_status().value();
    }

    return status;
}

//force equal to true case should only be used for warm boot query
bool DcoOtuAdapter::getOtuFault(string aid, OtuFault *faults, bool force)
{
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " otuId: " << otuId << endl;
    Otu otu;
    if (setKey(&otu, otuId) == false)
    {
        return false;
    }

    bool status = true;
    if (force)
    {
	    int idx = 0;
	    status = getOtuObj( &otu, otuId, &idx);
	    if ( status == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Otu Object  FAIL "  << endl;
		    return status;
	    }

	    if (otu.otu(idx).otu().has_state() == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... OtuId: " <<  otuId << endl;
		    return false;
	    }

	    if (otu.otu(idx).otu().state().has_facility_fault())
	    {
		    faults->facBmp = otu.otu(idx).otu().state().facility_fault().value();
		    //if otu fault is already been updated via notification, skip the retrieval
		    if (mOtuFault[otuId - 1].isNotified == false)
		    {
			    mOtuFault[otuId - 1].faultBitmap = faults->facBmp;
			    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve otu fault " << otuId << " with bitmap: 0x" <<  hex << mOtuFault[otuId - 1].faultBitmap << dec << endl;
		    }
		    else
		    {
			    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve otu fault " << otuId << " with fault notify flag:" <<  mOtuFault[otuId - 1].isNotified << endl;
		    }
	    }
	    else
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no otu fault in state, otuId: " << otuId;
	    }

	    //only been triggered when create to handle warm boot case
	    int rxTtiSize = otu.otu(idx).otu().state().rx_tti_size();
	    if ( rxTtiSize != 0 )
	    {
			//if otu tti is already been updated via notification, skip the retrieval
			if (mTtiRx[otuId - 1].isNotified == false)
			{
			    for (int i = 0; i < rxTtiSize; i++)
			    {
			        mTtiRx[otuId - 1].ttiRx[i] = otu.otu(idx).otu().state().rx_tti(i).rx_tti().tti().value();
			    }
			    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve otu tti " << idx << " with size: " << rxTtiSize << endl;
			}
			else
			{
			    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve otu tti " << idx << " with tti notify flag: " << mTtiRx[otuId - 1].isNotified << endl;
			}
	    }
	    else
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve tti since no tti in state, otuId: " << otuId;
	    }
    }
    else
    {
	    faults->facBmp = mOtuFault[otuId - 1].faultBitmap;
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

bool DcoOtuAdapter::getOtuPm(string aid)
{
	INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    return true;
}

// Calculate frame rate base on GX-PM-Additionnal-Material.xlxs
// https://nam11.safelinks.protection.outlook.com/ap/x-59584e83/?url=https%3A%2F%2Finfinera.sharepoint.com%2F%3Ax%3A%2Fr%2Fteams%2FGX%2F_layouts%2F15%2Fdoc2.aspx%3Faction%3Dedit%26sourcedoc%3D%257BF563CDE4-B892-4D05-819D-A090F5AAE322%257D%26CID%3D20DA8DF5-002E-4F78-9C74-9E30DA3948CA%26wdLOR%3Dc05F57B5E-7EFF-480D-AF3B-08078C343C58&data=04%7C01%7Cbnguyen%40infinera.com%7C79d958dd6b3a459f959908d8b9a112bc%7C285643de5f5b4b03a1530ae2dc8aaf77%7C1%7C0%7C637463448762227690%7CUnknown%7CTWFpbGZsb3d8eyJWIjoiMC4wLjAwMDAiLCJQIjoiV2luMzIiLCJBTiI6Ik1haWwiLCJXVCI6Mn0%3D%7C1000&sdata=OU6hozHcAw6kv6oB4P7dRmwZpNJCquU2h0Lj6%2BJO%2BQY%3D&reserved=0

uint32_t DcoOtuAdapter::getOtuFrameRate(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return 0;
    }

    // Test code for all frame rate
#if 0
    for (uint32_t trate = 100; trate <= 1600; trate+= 25)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Rate: " << trate << " frate: " << round ( (trate/DCO_TS_GRANULARITY) * LXTF20_E_FRAME_RATE);
    }
#endif

    ClientMode mode = DcoCardAdapter::mCfgClientMode;
    if (aid.find("-T") != std::string::npos)
    {
	    return OTU4_FRAME_RATE;
    }

    double rate = (double) mOtuRate[otuId];

    if (mode == CLIENTMODE_LXTP_E)
    {
        return ( (uint32_t) round ( (rate/DCO_TS_GRANULARITY ) * LXTF20_E_FRAME_RATE) );
    }
    else if (mode == CLIENTMODE_LXTP_M)
    {
        return ( (uint32_t) round ( (rate/DCO_TS_GRANULARITY ) * LXTF20_M_FRAME_RATE) );
    }
    else
    {
        return 0;
    }
}

uint64_t DcoOtuAdapter::getOtuPmErrorBlocks(string aid)
{
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    if (mOtuPms.HasPmData(otuId) == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " no pm cache data for otuId: " << otuId << endl;
        return false;
    }

    auto otuPm = mOtuPms.GetPmData(otuId);
    if (otuPm.otuId != otuId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " not matching.. pm cache data otuId is: " << otuPm.otuId << endl;
        return false;
    }

    return (otuPm.rx.pmcommon.err_blocks);
}

bool DcoOtuAdapter::getOtuFecCorrEnable(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return false;
    }

    return mbFecCorrEnable[otuId];
}

void DcoOtuAdapter::subOtuFault()
{
	OtuFms ofm;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&ofm);
	::GnmiClient::AsyncSubCallback* cb = new OtuFaultResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " otu fault sub request sent to server.."  << endl;
}

void DcoOtuAdapter::subOtuState()
{
	OtuState os;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&os);
	::GnmiClient::AsyncSubCallback* cb = new OtuStateResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " otu state sub request sent to server.."  << endl;
}

void DcoOtuAdapter::subOtuOpStatus()
{
	dcoOtuOpStatus ops;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&ops);
	::GnmiClient::AsyncSubCallback* cb = new OtuOpStatusResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " otu op status sub request sent to server.."  << endl;
}

void DcoOtuAdapter::subOtuPm()
{
	if (dp_env::DpEnvSingleton::Instance()->isSimEnv() && mIsGnmiSimModeEn) {
		//spawn a thread to feed up pm data for sim
		if (mSimOtuPmThread.joinable() == false) { //check whether the sim pm thread is running
			INFN_LOG(SeverityLevel::info) << "create mSimOtuPmThread " << endl;
			mSimOtuPmThread = std::thread(&DcoOtuAdapter::setSimPmData, this);
		}
		else {
			INFN_LOG(SeverityLevel::info) << "mSimOtuPmThread is already running..." << endl;
		}
	}
	else {
		INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
		dcoOtuPm cp;
	    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&cp);
		::GnmiClient::AsyncSubCallback* cb = new OtuPmResponseContext(); //cleaned up in grpc client
		std::string cbId = mspCrud->Subscribe(msg, cb);
		INFN_LOG(SeverityLevel::info) << " otu pm sub request sent to server.."  << endl;
	}
}

//set same sim pm data for all objects..
void DcoOtuAdapter::setSimPmData()
{
	while (1)
	{
		this_thread::sleep_for(seconds(1));

        bool isSimDcoConnect = DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)DCC_ZYNQ).isSimDcoConnect();
        if (!mIsGnmiSimModeEn || !isSimDcoConnect)
        {
            continue;
        }

	for (int i = 1; i <= MAX_OTU_ID; i++) {
		if (mOtuPms.HasPmData(i) == false) continue;
		auto otuPm = mOtuPms.GetPmData(i);
		int otuId= otuPm.otuId;
		INFN_LOG(SeverityLevel::debug) << __func__ << " otuId: " << otuId << ", otuPm.aid: " << otuPm.aid << ", i: " << i << endl;

		otuPm.rx.pmcommon.bip_err = 12;
		otuPm.rx.pmcommon.err_blocks = 34;
		otuPm.rx.pmcommon.far_end_err_blocks = 5;
		otuPm.rx.pmcommon.bei = 6;
		otuPm.rx.pmcommon.iae = 7;
		otuPm.rx.pmcommon.biae = 8;
		otuPm.rx.pmcommon.prbs_sync_err_count = 9;
		otuPm.rx.pmcommon.prbs_err_count = 1;
		otuPm.rx.pmcommon.latency = 2;

		otuPm.rx.fecstat.num_collected = 102;
		otuPm.rx.fecstat.corrected_bits = 324;
		otuPm.rx.fecstat.corrected_1 = 5678;
		otuPm.rx.fecstat.corrected_0 = 6678;
		otuPm.rx.fecstat.uncorrected_words = 70;
		otuPm.rx.fecstat.uncorrected_bits = 81;
		otuPm.rx.fecstat.total_code_words = 92;
		otuPm.rx.fecstat.code_word_size = 13;
		otuPm.rx.fecstat.total_bits = 25;

		otuPm.tx.bip_err = 112;
		otuPm.tx.err_blocks = 334;
		otuPm.tx.far_end_err_blocks = 35;
		otuPm.tx.bei = 46;
		otuPm.tx.iae = 57;
		otuPm.tx.biae = 28;
		otuPm.tx.prbs_sync_err_count = 19;
		otuPm.tx.prbs_err_count = 91;
		otuPm.tx.prbs_err_count = 62;

        otuPm.valid = true;

		DcoOtuAdapter::mOtuPms.UpdatePmData(otuPm, otuId); //pmdata map key is 1 based
	}

	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoOtuAdapter::mOtuPms), &DcoOtuAdapter::mOtuPms);
	}
}

//only used for cli
bool DcoOtuAdapter::otuPmInfo(ostream& out, int otuId)
{
	if (mOtuPms.HasPmData(otuId) == false)
	{
		out << " no pm cache data for otuId: " << otuId << endl;
		return false;
	}

	auto otuPm = mOtuPms.GetPmData(otuId);
	if (otuPm.otuId != otuId)
	{
		out << " not matching.. pm cache data otuId is: " << otuPm.otuId << endl;
		return false;
	}
	auto rxPmCommon = otuPm.rx.pmcommon;
	auto rxPmFec = otuPm.rx.fecstat;
	auto txPmCommon = otuPm.tx;
	out << " Otu aid: " << otuPm.aid << ", id: " << otuPm.otuId << ", Pm Info: " << endl;

	out << " rx pmcommon bip_err: " << rxPmCommon.bip_err << endl;
	out << " rx pmcommon err_blocks: " << rxPmCommon.err_blocks    << endl;
	out << " rx pmcommon far_end_err_blocks: " << rxPmCommon.far_end_err_blocks << endl;
	out << " rx pmcommon bei: " << rxPmCommon.bei << endl;
	out << " rx pmcommon iae: " << rxPmCommon.iae << endl;
	out << " rx pmcommon biae: " << rxPmCommon.biae << endl;
	out << " rx pmcommon prbs_sync_err_count: " << rxPmCommon.prbs_sync_err_count << endl;
	out << " rx pmcommon prbs_err_count: " << rxPmCommon.prbs_err_count << endl;
	out << " rx pmcommon latency: " << rxPmCommon.latency << endl;

	out << " rx fecstat num_collected: " << rxPmFec.num_collected << endl;
	out << " rx fecstat corrected_bits: " << rxPmFec.corrected_bits << endl;
	out << " rx fecstat corrected_1: " << rxPmFec.corrected_1 << endl;
	out << " rx fecstat corrected_0: " << rxPmFec.corrected_0 << endl;
	out << " rx fecstat uncorrected_words: " << rxPmFec.uncorrected_words << endl;
	out << " rx fecstat uncorrected_bits: " << rxPmFec.uncorrected_bits << endl;
	out << " rx fecstat total_code_words: " << rxPmFec.total_code_words << endl;
	out << " rx fecstat code_word_size: " << rxPmFec.code_word_size << endl;
	out << " rx fecstat total_bits: " << rxPmFec.total_bits << endl;
	out << " rx fecstat pre_fec_ber: " << rxPmFec.pre_fec_ber << endl;

	out << " tx pmcommon bip_err: " << txPmCommon.bip_err << endl;
	out << " tx pmcommon err_blocks: " << txPmCommon.err_blocks    << endl;
	out << " tx pmcommon far_end_err_blocks: " << txPmCommon.far_end_err_blocks << endl;
	out << " tx pmcommon bei: " << txPmCommon.bei << endl;
	out << " tx pmcommon iae: " << txPmCommon.iae << endl;
	out << " tx pmcommon biae: " << txPmCommon.biae << endl;
	out << " tx pmcommon prbs_sync_err_count: " << txPmCommon.prbs_sync_err_count << endl;
	out << " tx pmcommon prbs_err_count: " << txPmCommon.prbs_err_count << endl;
	out << " tx pmcommon latency: " << txPmCommon.latency << endl;
	out << "mOtuPmSubEnable: " << EnumTranslate::toString(mOtuPmSubEnable) << endl;

        std::time_t last_c = std::chrono::system_clock::to_time_t(otuPm.updateTime);
        std::string ts(ctime(&last_c));
        ts.erase(std::remove(ts.begin(), ts.end(), '\n'), ts.end());
	out << "Update Time: " << ts << endl;
	out << "Update Count: " << otuPm.updateCount << endl;

	return true;
}

//only used for cli
bool DcoOtuAdapter::otuPmAccumInfo(ostream& out, int otuId)
{
	if (mOtuPms.HasPmData(otuId) == false)
	{
		out << " no pm cache data for otuId: " << otuId<< endl;
		return false;
	}
	auto otuPmAccum = mOtuPms.GetPmDataAccum(otuId);
	if (otuPmAccum.otuId != otuId)
	{
		out << " not matching.. pm cache data otuId is: " << otuPmAccum.otuId << endl;
		return false;
	}

	auto rxPmCommon = otuPmAccum.rx.pmcommon;
	auto rxPmFec = otuPmAccum.rx.fecstat;
	auto txPmCommon = otuPmAccum.tx;
	out << " Otu aid: " << otuPmAccum.aid << ", id: " << otuPmAccum.otuId << ", Pm Info: " << endl;
        out << " Accumulation Mode: " << (true == otuPmAccum.accumEnable ? "enabled" : "disabled") << endl;

	out << " rx pmcommon bip_err: " << rxPmCommon.bip_err << endl;
	out << " rx pmcommon err_blocks: " << rxPmCommon.err_blocks    << endl;
	out << " rx pmcommon far_end_err_blocks: " << rxPmCommon.far_end_err_blocks << endl;
	out << " rx pmcommon bei: " << rxPmCommon.bei << endl;
	out << " rx pmcommon prbs_sync_err_count: " << rxPmCommon.prbs_sync_err_count << endl;
	out << " rx pmcommon prbs_err_count: " << rxPmCommon.prbs_err_count << endl;

	out << " rx fecstat num_collected: " << rxPmFec.num_collected << endl;
	out << " rx fecstat corrected_bits: " << rxPmFec.corrected_bits << endl;
	out << " rx fecstat corrected_1: " << rxPmFec.corrected_1 << endl;
	out << " rx fecstat corrected_0: " << rxPmFec.corrected_0 << endl;
	out << " rx fecstat uncorrected_words: " << rxPmFec.uncorrected_words << endl;
	out << " rx fecstat uncorrected_bits: " << rxPmFec.uncorrected_bits << endl;
	out << " rx fecstat total_code_words: " << rxPmFec.total_code_words << endl;
	out << " rx fecstat code_word_size: " << rxPmFec.code_word_size << endl;
	out << " rx fecstat total_bits: " << rxPmFec.total_bits << endl;

	out << " tx pmcommon bip_err: " << txPmCommon.bip_err << endl;
	out << " tx pmcommon err_blocks: " << txPmCommon.err_blocks    << endl;
	out << " tx pmcommon far_end_err_blocks: " << txPmCommon.far_end_err_blocks << endl;
	out << " tx pmcommon bei: " << txPmCommon.bei << endl;
	out << " tx pmcommon prbs_sync_err_count: " << txPmCommon.prbs_sync_err_count << endl;
	out << " tx pmcommon prbs_err_count: " << txPmCommon.prbs_err_count << "\n\n";

	return true;
}

//spanTime for each dpms otu pm collection start and end and execution time
void DcoOtuAdapter::otuPmTimeInfo(ostream& out)
{
	out << "otu pm time info in last 20 seconds: " << endl;

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

//only used for cli
bool DcoOtuAdapter::otuPmAccumClear(ostream& out, int otuId)
{
    mOtuPms.ClearPmDataAccum(otuId);
    return true;
}

bool DcoOtuAdapter::otuPmAccumEnable(ostream& out, int otuId, bool enable)
{
    mOtuPms.SetAccumEnable(otuId, enable);
    return true;
}

// Updates serdesCfgTbl from serdesTuner. Updates occur when each port is configured.
void DcoOtuAdapter::updateSerdesCfgTbl(string aid, const vector<tuning::AtlBcmSerdesTxCfg> & atlBcmTbl)
{
    // Convert AID to laneNum
    int otuId = aidToOtuId(aid);
    if (otuId < 1 || otuId > HO_OTU_ID_OFFSET)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
        return;
    }
    unsigned int laneNum = 0;
    int totalLanes = tuning::cAtlNumLanesOtu4;

    if (atlBcmTbl.size() > totalLanes)
    {
        INFN_LOG(SeverityLevel::error) <<  __func__ << "GetAtlBcmMapParam() failed or has invalid number of lanes=" << totalLanes << " for AID=" << aid;
        return;
    }

    for (unsigned int i = 0; i < totalLanes; i++)
    {
        laneNum = tuning::atlPortLanesOtu4[otuId].laneNum[i];
        memcpy(&serdesCfgTbl[laneNum], &atlBcmTbl[i], sizeof(serdesCfgTbl[laneNum]));
        INFN_LOG(SeverityLevel::info) <<  "UpdateSerdesCfgTbl AID=" << aid << " otuId= " << otuId << " serdesCfgTbl[" << laneNum << "]=" << serdesCfgTbl[laneNum].txPolarity <<
		" " << serdesCfgTbl[laneNum].rxPolarity << " " << serdesCfgTbl[laneNum].txPre2 << " " << serdesCfgTbl[laneNum].txPre1 << " " << serdesCfgTbl[laneNum].txMain <<
                " " << serdesCfgTbl[laneNum].txPost1 << " " << serdesCfgTbl[laneNum].txPost2 << " " << serdesCfgTbl[laneNum].txPost3;
    }
}

bool DcoOtuAdapter::updateNotifyCache()
{
    Otu otu;
    uint32 id = 1; //set dummy id as yang require this as key
    if (setKey(&otu, id) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&otu);

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

    INFN_LOG(SeverityLevel::info) << __func__ << " otu size: " << otu.otu_size();

    for (int idx = 0; idx < otu.otu_size(); idx++)
    {
        int otuId = 1;
        if (otu.otu(idx).otu().state().has_otu_id())
        {
            otuId = otu.otu(idx).otu().state().otu_id().value();
            INFN_LOG(SeverityLevel::info) << "Otu has otuId: " << otuId;
        }
        if (otu.otu(idx).otu().state().has_facility_fault())
        {
            //if otu fault is already been updated via notification, skip the retrieval
            if (mOtuFault[otuId - 1].isNotified == false)
            {
                mOtuFault[otuId - 1].faultBitmap = otu.otu(idx).otu().state().facility_fault().value();
	        INFN_LOG(SeverityLevel::info) << "Otu has fac fault: 0x" << hex << mOtuFault[otuId - 1].faultBitmap << dec;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve otu fault " << otuId << " with fault notify flag:" <<  mOtuFault[otuId - 1].isNotified << endl;
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no otu fault in state, otuId: " << otuId;
        }
        int ttiSize = otu.otu(idx).otu().state().rx_tti_size();
        if ( ttiSize != 0 )
        {
            //if otu tti is already been updated via notification, skip the retrieval
            if (mTtiRx[otuId - 1].isNotified == false)
            {
                for (int i = 0; i < ttiSize; i++)
                {
                    mTtiRx[otuId - 1].ttiRx[i] = otu.otu(idx).otu().state().rx_tti(i).rx_tti().tti().value();
                }
                INFN_LOG(SeverityLevel::info) << "Otu has tti size: " << ttiSize;
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve tti since no tti in state, otuId: " << otuId;
        }
    }
    return status;
}

// ====== DcoClientAdapter private =========
// Client Otu key range: 1-16, Line Otu key range: 17-18
int DcoOtuAdapter::aidToOtuId (string aid)
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
        string cliOtu = "Otu ";
        found = aid.find(cliOtu);
        if (found == std::string::npos)
            return -1;
        sId = aid.substr(cliOtu.length());
    }
    return(stoi(sId,nullptr));
}

bool DcoOtuAdapter::setKey (Otu *otu, int otuId)
{
    bool status = true;
	if (otu == NULL)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Fail *otu = NULL... "  << endl;
        return false;
    }

    Otus_OtuKey *cKey;

    // Always use the first object in the ClientOtu list, since we
    // create new otu object every time we send.
    cKey = otu->add_otu();
    cKey->set_otu_id(otuId);
    cKey->mutable_otu()->mutable_config()->mutable_otu_id()->set_value(otuId);

    return status;
}

//
// Fetch Otu containter object and find the object that match with otuId in
// list
//
bool DcoOtuAdapter::getOtuObj( Otu *otu, int otuId, int *idx)
{
    // Save the OtuId from setKey
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(otu);
    INFN_LOG(SeverityLevel::debug) << __func__ << " \n\n ****Read ... \n"  << endl;
    bool status = true;

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        otu = static_cast<Otu*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    // Search for the correct OtuId.
    bool found = false;

    INFN_LOG(SeverityLevel::debug) << __func__ << " otu size " << otu->otu_size() << endl;

    for (*idx = 0; *idx < otu->otu_size(); (*idx)++)
    {
        INFN_LOG(SeverityLevel::debug) << __func__ << " otu id " << otu->otu(*idx).otu_id() << endl;
        if (otu->otu(*idx).otu_id() == otuId )
        {
            found = true;
            break;
        }
    }
    if (found == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " cannot find OtuId in DCO  FAIL... OtuId: " <<  otuId << endl;
        return false;
    }
    return status;
}

void DcoOtuAdapter::setSimModeEn(bool enable)
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

bool DcoOtuAdapter::getOtuFaultCapa()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    DcoOtuFault fault;
    DcoOtuFault *pFault;

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&fault);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        pFault = static_cast<DcoOtuFault*>(msg);
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
        DcoOtuFault_Capabilities_FacilityFaultsKey key;

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
        using facilitySeverity = dcoyang::infinera_dco_otu_fault::DcoOtuFault_Capabilities_FacilityFaults;
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

        using dir = dcoyang::infinera_dco_otu_fault::DcoOtuFault_Capabilities_FacilityFaults;
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

        using loc = dcoyang::infinera_dco_otu_fault::DcoOtuFault_Capabilities_FacilityFaults;
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
        if ( i->faultName == DCO_OTU_SAPI_MISM_NAME_STR && i->direction == FAULTDIRECTION_RX )
        {
            mRxSapiMismBitmap = 1ULL << i->bitPos;
            break;
        }
    }
    return status;
}

void DcoOtuAdapter::otuNotifyState(std::ostream& out, int otuId)
{
    out << " otu " << otuId << " TTI Rx: " << endl;
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        out << " 0x" << hex << (int)mTtiRx[otuId - 1].ttiRx[i] << dec;
    }
    out << "\n\ntti isNotified: " << EnumTranslate::toString(mTtiRx[otuId - 1].isNotified) << endl;
    out << "\nfault isNotified: " << EnumTranslate::toString(mOtuFault[otuId - 1].isNotified) << endl;
    out << "notifyCount: " << mTtiRx[otuId - 1].notifyCount << endl;
    out << "eb overflow map size: " << ebOverflowMap.size() << endl;
    for (const auto& elem : ebOverflowMap)
    {
	    for (const auto& time : elem.second)
	    {
		    out << " aid: " << elem.first << " overflow at: " << time << endl;
	    }
    }
    out << "\n mOtuPmSubEnable: " << EnumTranslate::toString(mOtuPmSubEnable) << endl;
    out << "\n mOpStatus.opStatus: " << EnumTranslate::toString(mOpStatus[otuId - 1].opStatus) << endl;
    out << "\n mOpStatus.transactionId: " << mOpStatus[otuId - 1].transactionId << endl;
    out << endl;
}

uint8_t* DcoOtuAdapter::getOtuRxTti(const string aid)
{
	int otuId = aidToOtuId(aid);
	if (otuId < 1 || otuId > MAX_OTU_ID)
	{
		INFN_LOG(SeverityLevel::error) << __func__ << "Bad Otu Id: " << otuId << '\n';
		return NULL;
	}
	return mTtiRx[otuId - 1].ttiRx;
}

void DcoOtuAdapter::configFir(int serdesLane, OtuConfig* dcoCfg)
{
    auto item = serdesCfgTbl[serdesLane];
    auto firKey = dcoCfg->add_client_serdes_pol_fir();
    firKey->set_client_serdes_lane_key(serdesLane);
    auto firPtr = firKey->mutable_client_serdes_pol_fir();
    if (firPtr == NULL)
    {
	    INFN_LOG(SeverityLevel::info) << __func__ << " firPtr is NULL";
	    return;
    }
    firPtr->mutable_tx_lane_polarity()->set_value(item.txPolarity); // One base table
    firPtr->mutable_rx_lane_polarity()->set_value(item.rxPolarity);
    firPtr->mutable_tx_fir_precursor_2()->set_value(item.txPre2);
    firPtr->mutable_tx_fir_precursor_1()->set_value(item.txPre1);
    firPtr->mutable_tx_fir_main_tap()->set_value(item.txMain);
    firPtr->mutable_tx_fir_postcursor_1()->set_value(item.txPost1);
    firPtr->mutable_tx_fir_postcursor_2()->set_value(item.txPost2);
    firPtr->mutable_tx_fir_postcursor_3()->set_value(item.txPost3);
    INFN_LOG(SeverityLevel::info) << "Setting ATL serdes serdesLane=" << serdesLane <<  " txPol=" <<
	    item.txPolarity << " rxPol=" << item.rxPolarity << " txPre2=" << item.txPre2 <<
	    " txPre1=" << item.txPre1 << " txMain=" << item.txMain << " txPost1=" <<
	    item.txPost1 << " txPost2=" << item.txPost2 << " txPost3=" << item.txPost3;
}

void DcoOtuAdapter::clearNotifyFlag()
{
    INFN_LOG(SeverityLevel::info) << __func__ << ", clear notify flag" << endl;
    for (int i = 0; i < MAX_OTU_ID; i++)
    {
        mOtuFault[i].isNotified = false;
        mTtiRx[i].isNotified = false;
        mTtiRx[i].notifyCount = 0;
    }
}

} /* DpAdapter */
