/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>

#include <google/protobuf/text_format.h>
#include "CrudService.h"
#include "DcoGigeClientAdapter.h"
#include "DpGigeClientMgr.h"
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
using namespace dcoyang::infinera_dco_client_gige;
using namespace dcoyang::infinera_dco_client_gige_fault;

using clientConfig = ClientsGige_ClientGige_Config;
using loopBack = ClientsGige_ClientGige_Config_LoopbackType;
using intfType = ClientsGige_ClientGige_Config_InterfaceType;
using ms = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal;
using prbsmode = ::dcoyang::enums::InfineraDcoClientGigePrbsMode;

namespace DpAdapter {

DpMsClientGigePms DcoGigeClientAdapter::mGigePms;
high_resolution_clock::time_point DcoGigeClientAdapter::mStart;
high_resolution_clock::time_point DcoGigeClientAdapter::mEnd;
int64_t DcoGigeClientAdapter::mStartSpan;
int64_t DcoGigeClientAdapter::mEndSpan;
int64_t DcoGigeClientAdapter::mDelta;
std::deque<int64_t> DcoGigeClientAdapter::mStartSpanArray(spanTime);
std::deque<int64_t> DcoGigeClientAdapter::mEndSpanArray(spanTime);
std::deque<int64_t> DcoGigeClientAdapter::mDeltaArray(spanTime);
bool DcoGigeClientAdapter::mGigePmSubEnable = false;
bool DcoGigeClientAdapter::mGigeFltSubEnable = false;
bool DcoGigeClientAdapter::mGigeOpStatusSubEnable = false;

AdapterCacheFault DcoGigeClientAdapter::mGigeFault[MAX_CLIENTS];
AdapterCacheOpStatus DcoGigeClientAdapter::mOpStatus[MAX_CLIENTS];



#if 1
const clientPortEntry DcoGigeClientAdapter::mChm6_PortMap[] =
{
// CHM6 Serdes speed to DCO is fixed at 50G per lane
// Lane map in HW are 0 based.  Yang Data Model is 1 based
//   FP------Rate------------Lane
    { 1  , PORT_RATE_ETH100G, 15 },
    { 1  , PORT_RATE_ETH400G, 9  }, // 400GE
    { 2  , PORT_RATE_ETH100G, 13 },
    { 3  , PORT_RATE_ETH100G, 9  },
    { 4  , PORT_RATE_ETH100G, 11 },
    { 5  , PORT_RATE_ETH100G, 1  },
    { 6  , PORT_RATE_ETH100G, 3  },
    { 7  , PORT_RATE_ETH100G, 5  },
    { 8  , PORT_RATE_ETH100G, 7  },
    { 8  , PORT_RATE_ETH400G, 1  }, // 400GE
    { 9  , PORT_RATE_ETH100G, 25 },
    { 9  , PORT_RATE_ETH400G, 25 }, // 400GE
    { 10 , PORT_RATE_ETH100G, 27 },
    { 11 , PORT_RATE_ETH100G, 31 },
    { 12 , PORT_RATE_ETH100G, 29 },
    { 13 , PORT_RATE_ETH100G, 23 },
    { 14 , PORT_RATE_ETH100G, 21 },
    { 15 , PORT_RATE_ETH100G, 19 },
    { 16 , PORT_RATE_ETH100G, 17 },
    { 16 , PORT_RATE_ETH400G, 17 }, // 400GE
    { 0  , PORT_RATE_UNSET  , 0  }  // End
}; // mChm6_PortMap
#else
const clientPortEntry DcoGigeClientAdapter::mChm6_PortMap[] =
{
// CHM6 Serdes speed to DCO is fixed at 50G per lane
//   FP------Rate------------Lane
    { 1  , PORT_RATE_ETH100G, 1 , },
    { 1  , PORT_RATE_ETH400G, 1 },
    { 2  , PORT_RATE_ETH100G, 3 },
    { 3  , PORT_RATE_ETH100G, 5 },
    { 4  , PORT_RATE_ETH100G, 7 },
    { 5  , PORT_RATE_ETH100G, 9 },
    { 6  , PORT_RATE_ETH100G, 11 },
    { 7  , PORT_RATE_ETH100G, 13 },
    { 8  , PORT_RATE_ETH100G, 15 },
    { 8  , PORT_RATE_ETH400G, 9 },
    { 9  , PORT_RATE_ETH100G, 17 },
    { 9  , PORT_RATE_ETH400G, 17 },
    { 10 , PORT_RATE_ETH100G, 19 },
    { 11 , PORT_RATE_ETH100G, 21 },
    { 12 , PORT_RATE_ETH100G, 23 },
    { 13 , PORT_RATE_ETH100G, 25 },
    { 14 , PORT_RATE_ETH100G, 27 },
    { 15 , PORT_RATE_ETH100G, 29 },
    { 16 , PORT_RATE_ETH100G, 31 },
    { 16 , PORT_RATE_ETH400G, 25 },
    { 0  , PORT_RATE_UNSET  , 0 }  // End
}; // mChm6_PortMap
#endif


DcoGigeClientAdapter::DcoGigeClientAdapter()
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

    for (unsigned int i = 0; i <= MAX_CLIENTS; i++)
    {
        mPortRate[i] = PORT_RATE_UNSET; // goes to MAX_CLIENT+1
        mTdaHoldOff[i] = false;
    }
}

DcoGigeClientAdapter::~DcoGigeClientAdapter()
{

}

// Returns the number of lanes between BCM and DCO based on port rate.
unsigned int DcoGigeClientAdapter::getDcoLanesPerPort(DataPlane::PortRate portRate)
{
    unsigned int totalLanes = 0;

    if (portRate == PORT_RATE_ETH100G)
    {
        totalLanes = tuning::cAtlNumLanes100Ge;
    }
    else if (portRate == PORT_RATE_ETH400G)
    {
        totalLanes = tuning::cAtlNumLanes400Ge;
    }
    else if (portRate == PORT_RATE_ETH200G)
    {
        totalLanes = tuning::cAtlNumLanes200Ge;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) <<  __func__ << "Unknown DCO lanes per port for rate=" << portRate;
    }
    return totalLanes;
}

unsigned int DcoGigeClientAdapter::getAtlBcmSerdesTxOffset(int portId, DataPlane::PortRate portRate, unsigned int lane)
{

    if (portRate == PORT_RATE_ETH100G)
    {
        return tuning::atlPortLanes100Ge[portId].laneNum[lane];
    }
    else if (portRate == PORT_RATE_ETH400G)
    {
        return tuning::atlPortLanes400Ge[portId].laneNum[lane];
    }
    else if (portRate == PORT_RATE_ETH200G)
    {
        return tuning::atlPortLanes200Ge[portId].laneNum[lane];
    }
    else
    {
        // default 100GE
        return tuning::atlPortLanes100Ge[portId].laneNum[lane];
    }
}



// Updates serdesCfgTbl from serdesTuner. Updates occur when each port is configured.
void DcoGigeClientAdapter::updateSerdesCfgTbl(string aid, DataPlane::PortRate portRate, const vector<tuning::AtlBcmSerdesTxCfg> & atlBcmTbl)
{
    // Convert AID to laneNum
    int portId = aidToPortId(aid);
    unsigned int laneNum = 0;
    unsigned int totalLanes = 0;

    totalLanes = getDcoLanesPerPort(portRate);

    if (atlBcmTbl.size() > totalLanes)
    {
        INFN_LOG(SeverityLevel::error) <<  __func__ << "GetAtlBcmMapParam() failed or has invalid number of lanes=" << totalLanes << " for rate=" << portRate << " for AID=" << aid;
        return;
    }

    for (unsigned int i = 0; i < totalLanes; i++)
    {

        laneNum = getAtlBcmSerdesTxOffset(portId, portRate, i);
        memcpy(&serdesCfgTbl[laneNum], &atlBcmTbl[i], sizeof(serdesCfgTbl[laneNum]));
        INFN_LOG(SeverityLevel::info) <<  "UpdateSerdesCfgTbl AID=" << aid << " serdesCfgTbl[" << laneNum << "]=" << serdesCfgTbl[laneNum].txPolarity << " " <<
                serdesCfgTbl[laneNum].rxPolarity << " " << serdesCfgTbl[laneNum].txPre2 << " " << serdesCfgTbl[laneNum].txPre1 << " " << serdesCfgTbl[laneNum].txMain <<
                " " << serdesCfgTbl[laneNum].txPost1 << " " << serdesCfgTbl[laneNum].txPost2 << " " << serdesCfgTbl[laneNum].txPost3;
    }
}

// To create client internally in DCO to use for Xpress Xcon.  The client
//  will be in TERM loopback.
bool DcoGigeClientAdapter::createGigeClientXpress(string aid, DataPlane::PortRate portRate)
{
    GigeCommon cfg;
    cfg.rate = portRate;
    cfg.lpbk = LOOP_BACK_TYPE_TERMINAL;
    cfg.regen = true;

    // Default value for the rest of config
    cfg.fecEnable = true;
    cfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsTx = MAINTENANCE_SIGNAL_LF;
    cfg.autoMsRx = MAINTENANCE_SIGNAL_LF;
    cfg.fecDegSer.activateThreshold = 0.0000001;
    cfg.fecDegSer.deactivateThreshold = 0.00000001;
    cfg.fecDegSer.monitorPeriod = 10;
    // Set transactionId to wait for DCO ack.
    cfg.transactionId = getTimeNanos();

    INFN_LOG(SeverityLevel::info) << __func__ << " XPress case: Create Rate: " << cfg.rate << " aid=" << aid << " transId: 0x" << hex << cfg.transactionId <<  dec  << endl;

    if ( createGigeClient (aid, &cfg) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Error: Failed create  aid: " << aid << '\n';
        return false;
    }
    return true;
}

bool DcoGigeClientAdapter::createGigeClient(string aid, GigeCommon *cfg)
{
	bool status = false;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Create portId: " << portId  << " Rate: " << cfg->rate << " aid=" << aid << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_name()->set_value(aid);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fwd_tda_trigger()->set_value(cfg->fwdTdaTrigger);

    const clientPortEntry *pPort = getFpPortEntry(portId, cfg->rate);
    if (pPort == NULL)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << "Error cannot find matching port/rate. rate :  " << cfg->rate  << " portId: " << portId << endl;
        return false;
    }
    uint16 lane = pPort->clientLane;
    if (lane > DCO_MAX_SERDES_LANES)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << "Error Bad serdes lane :  " << lane << " portId: " << portId << endl;
        return false;
    }

    int laneNum = 0;
    // Set Port Rate.
    mPortRate[portId] = cfg->rate;
    switch(cfg->rate)
    {
    case PORT_RATE_ETH100G:
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_payload_type(clientConfig::PAYLOADTYPE_100GB_ELAN);
        laneNum = 2;  // 50G serdes lane
        break;
    case PORT_RATE_ETH400G:
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_payload_type(clientConfig::PAYLOADTYPE_400GB_ELAN);
        laneNum = 8;  // 50G serdes lane
        break;
    // todo: add 200G?
    default:
        INFN_LOG(SeverityLevel::error) << __func__ << " unknown port rate ... " << cfg->rate << endl;
        return false;
        break;
    }

    for (uint16 i = 0; i < laneNum; i++)
    {
        // Create a pointer and use C++ type inference to determine the type
        // and assert it to remove redundant code statements.
         auto p = gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config();
        assert( (NULL != p) );

        // Assert a valid index value into serdesCfgTbl.
        // Valid index values for serdesCfgTbl 1 to 32.
        const auto ciLane = lane + i;
        assert( (ciLane < (DCO_MAX_SERDES_LANES + 1)) );
        assert( (ciLane > 0) );

        p->add_client_serdes_lanes()->set_client_serdes_lane(ciLane);

        auto item = serdesCfgTbl[ciLane];

        auto pp = p->mutable_client_serdes_lanes(i)->mutable_client_serdes_lanes();
        assert( (NULL != pp) );

        pp->mutable_tx_lane_polarity()->set_value(item.txPolarity); // One base table
        pp->mutable_rx_lane_polarity()->set_value(item.rxPolarity);
        pp->mutable_tx_fir_precursor_2()->set_value(item.txPre2);
        pp->mutable_tx_fir_precursor_1()->set_value(item.txPre1);
        pp->mutable_tx_fir_main_tap()->set_value(item.txMain);
        pp->mutable_tx_fir_postcursor_1()->set_value(item.txPost1);
        pp->mutable_tx_fir_postcursor_2()->set_value(item.txPost2);
        pp->mutable_tx_fir_postcursor_3()->set_value(item.txPost3);

        INFN_LOG(SeverityLevel::info) << "Setting ATL serdes portId: " << portId << " ciLane=" << ciLane <<  "txPol=" <<
                          item.txPolarity << " rxPol=" << item.rxPolarity << " txPre2=" << item.txPre2 <<
                          " txPre1=" << item.txPre1 << " txMain=" << item.txMain << " txPost1=" <<
                          item.txPost1 << " txPost2=" << item.txPost2 << " txPost3=" << item.txPost3;
    }

    // Set interface type to GAUI per SDK team
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_interface_type(clientConfig::INTERFACETYPE_INTF_GAUI);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ethernet_flex_type(clientConfig::ETHERNETFLEXTYPE_FLEX_NONE);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ethernet_mode(clientConfig::ETHERNETMODE_TRANSPARENT);

    loopBack lpbk;
    switch(cfg->lpbk)
    {
    case LOOP_BACK_TYPE_FACILITY:
        lpbk = clientConfig::LOOPBACKTYPE_FACILITY;
        break;
    case LOOP_BACK_TYPE_TERMINAL:
        lpbk = clientConfig::LOOPBACKTYPE_TERMINAL;
        break;
    case LOOP_BACK_TYPE_OFF:
    default:
        lpbk = clientConfig::LOOPBACKTYPE_NONE;
        break;
    }

    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_loopback_type(lpbk);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_enable()->set_value(cfg->fecEnable);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_rx_drop()->set_value(cfg->lldpRxDrop);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_tx_drop()->set_value(cfg->lldpTxDrop);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_rx_snooping_enable()->set_value(cfg->lldpRxSnoop);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_tx_snooping_enable()->set_value(cfg->lldpTxSnoop);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_regen()->set_value(cfg->regen);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_tid()->set_value(cfg->transactionId);


    ms signal;

    if (true == cfg->generatorTx)
    {
        signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
    }
    else
    {
        switch(cfg->forceMsTx)
        {
            case MAINTENANCE_SIGNAL_LF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                break;
            case MAINTENANCE_SIGNAL_RF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
                break;
            case MAINTENANCE_SIGNAL_IDLE:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                break;
            case MAINTENANCE_SIGNAL_NOREPLACE:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
            case MAINTENANCE_SIGNAL_LASEROFF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
            default:
                INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_force_signal_tx(signal);

    if (true == cfg->generatorRx)
    {
        signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
    }
    else
    {
        switch(cfg->forceMsRx)
        {
            case MAINTENANCE_SIGNAL_LF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                break;
            case MAINTENANCE_SIGNAL_RF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
                break;
            case MAINTENANCE_SIGNAL_IDLE:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                break;
            case MAINTENANCE_SIGNAL_NOREPLACE:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
            case MAINTENANCE_SIGNAL_LASEROFF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
            default:
                INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_force_signal_rx(signal);

    switch(cfg->autoMsTx)
    {
        case MAINTENANCE_SIGNAL_LF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
            break;
        case MAINTENANCE_SIGNAL_RF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
            break;
        case MAINTENANCE_SIGNAL_IDLE:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
            break;
            //R5.1 : Change the current internal TDA in Atlantic to SendLF instead of SendIDLE pattern
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = (true == mTdaHoldOff[portId]) ?
                     ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE :
                     ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
            break;
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_auto_signal_tx(signal);

    switch(cfg->autoMsRx)
    {
        case MAINTENANCE_SIGNAL_LF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
            break;
        case MAINTENANCE_SIGNAL_RF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
            break;
        case MAINTENANCE_SIGNAL_IDLE:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
            break;
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
            break;
            // YANG does not have LASER_OFF option.  Set IDLE; 4x100G should send_lf
        case MAINTENANCE_SIGNAL_LASEROFF:
        	{
        		std::size_t temp = aid.find(".");
        		if( temp != std::string::npos)
        		{
        			signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
        		}
        		else
        		{
        			signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
        		}
        		break;
        	}
        default:
            INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
            break;
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_auto_signal_rx(signal);

    if (true == cfg->monitorRx)
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_prbs_mon_rx(prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_GIGE_IDLE);
    }
    else
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_prbs_mon_rx(prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_PRBS_NONE);
    }

    if (PORT_RATE_ETH400G == cfg->rate)
    {
        // FEC DEG SER (send to DCO YANG)
        double fecDegSerActOffset   = cfg->fecDegSer.activateThreshold;
        double fecDegSerDeActOffset = cfg->fecDegSer.deactivateThreshold;

        INFN_LOG(SeverityLevel::info) << __func__ << " Create gige, cfg FEC Deg SER" << " act thresh=" << fecDegSerActOffset << " deact thres=" << fecDegSerDeActOffset <<  endl;

        fecDegSerActOffset         *= pow(10, DCO_FEC_DEG_TH_PRECISION);
        fecDegSerDeActOffset       *= pow(10, DCO_FEC_DEG_TH_PRECISION);

        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_mon()->set_value(cfg->fecDegSer.enable);

        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_activate_th()->set_digits(fecDegSerActOffset);
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_activate_th()->set_precision(DCO_FEC_DEG_TH_PRECISION);

        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_deactivate_th()->set_digits(fecDegSerDeActOffset);
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_deactivate_th()->set_precision(DCO_FEC_DEG_TH_PRECISION);

        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_mon_period()->set_value(cfg->fecDegSer.monitorPeriod);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    // Clear Fault cache
    mGigeFault[portId - 1].ClearFaultCache();

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

    mGigePms.AddPmData(aid, portId);

    // One Xpress XCON command from host would create 10 - 12 commands
    // to DCO.  DCO could not keep up.  Dpms will wait for DCO to complete
    // status = false or TransactionId = 0 ==> don't wait
    if (status == true &&  mOpStatus[portId-1].waitForDcoAck(cfg->transactionId) == false)
    {
        return false;
    }
    return status;
}

bool DcoGigeClientAdapter::deleteGigeClient(string aid)
{
	bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Delete portId: " << portId << endl;

    gigeClient gige;
    //only set id in Gige level, do not set id in Gige config level for gige obj deletion
    gige.add_client_gige()->set_client_port_id(portId);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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
    mGigePms.RemovePmData(portId);
    // Clear Fault cache
    mGigeFault[portId - 1].ClearFaultCache();
    mOpStatus[portId-1].ClearOpStatusCache();
    mTdaHoldOff[portId] = false;
    return status;
}

bool DcoGigeClientAdapter::initializeGigeClient()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    bool status = false;
    // string aid = "1-4-T1";
    // PortRate gRate = PORT_RATE_ETH100G;

    // Get Gige Fault Capabitites
    status = getGigeFaultCapa();

    subscribeStreams();

    return status;
}

void DcoGigeClientAdapter::subscribeStreams()
{
    clearFltNotifyFlag();

    //subscribe to gige pm
    if(!mGigePmSubEnable)
    {
        subGigePm();
	mGigePmSubEnable = true;
    }

    //subscribe to gige fault
    if (!mGigeFltSubEnable)
    {
        subGigeFault();
        mGigeFltSubEnable = true;
    }
    //subscribe to gige op status
    if(!mGigeOpStatusSubEnable)
    {
        subGigeOpStatus();
        mGigeOpStatusSubEnable = true;
    }
}

bool DcoGigeClientAdapter::setGigeMtu(string aid, int mtu)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MTU :  " << mtu  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_mtu()->set_value(mtu);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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
bool DcoGigeClientAdapter::setGigeFecEnable(string aid, bool enable)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " fec_enable :  " << (int) enable  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_enable()->set_value(enable);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeLldpDrop(string aid, Direction dir, bool enable)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__  << " LLDP Drop dir: " << dir << " enable :  " << (int) enable  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    if (dir == DIRECTION_TX )
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_tx_drop()->set_value(enable);
    }
    else if (dir == DIRECTION_RX )
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_rx_drop()->set_value(enable);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << __func__  << " ERROR!!! NO LLDP Drop dir: " << dir << " enable :  " << (int) enable  << " portId: " << portId << endl;

        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);
    grpc::Status gStatus = mspCrud->Update( msg );

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeLldpSnoop(string aid, Direction dir, bool enable)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__  << " LLDP SNOOP dir: " << dir << " enable :  " << (int) enable  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    if (dir == DIRECTION_TX )
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_tx_snooping_enable()->set_value(enable);
    }
    else if (dir == DIRECTION_RX )
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_lldp_rx_snooping_enable()->set_value(enable);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << __func__  << " ERROR!!! NO LLDP SNOOP dir: " << dir  << " portId: " << portId << endl;

        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeLoopBack(string aid, LoopBackType mode)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " LoopBackType :  " << (int) mode  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    loopBack lpbk;

    switch(mode)
    {
    case LOOP_BACK_TYPE_FACILITY:
        lpbk = clientConfig::LOOPBACKTYPE_FACILITY;
        break;
    case LOOP_BACK_TYPE_TERMINAL:
        lpbk = clientConfig::LOOPBACKTYPE_TERMINAL;
        break;
    case LOOP_BACK_TYPE_OFF:
    default:
        lpbk = clientConfig::LOOPBACKTYPE_NONE;
        break;
    }

    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_loopback_type(lpbk);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeAutoMS(string aid, Direction dir, MaintenanceSignal mode)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " portId: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MS :  " << (int) mode  << " AID: " << aid << " portId: " << portId << ", dir: " << dir << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }
    ms signal;

    switch (mode)
    {
        case MAINTENANCE_SIGNAL_LASEROFF:
            {
                //R5.1 : Change the current internal TDA in Atlantic to SendLF instead of SendIDLE pattern
                if (dir == DIRECTION_TX)
                {
                    signal = (true == mTdaHoldOff[portId]) ?
                             ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE :
                             ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                }
                else
                {
                    std::size_t temp = aid.find("."); //4x100G should send_lf
                    if( temp != std::string::npos)
                    {
                        signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                    }
                    else
                    {
                        signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                    }
                }
                break;
	    }
        case MAINTENANCE_SIGNAL_LF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
            break;
        case MAINTENANCE_SIGNAL_IDLE:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " portId: " << portId << '\n';
            return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MS :  " << (int) mode  << " AID: " << aid << " portId: " << portId << "Signal: " << signal << endl;


    if (dir == DIRECTION_TX)
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_auto_signal_tx(signal);
    }
    else
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_auto_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeForceMS(string aid, Direction dir, MaintenanceSignal mode)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " portId: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " MS :  " << (int) mode  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    MaintenanceSignal msMode = mode;

    bool testSigGen;
    DpGigeClientMgrSingleton::Instance()->getMspCfgHndlr()->getChm6GigeClientConfigTestSigGenFromCache(aid, dir, testSigGen);

    if (true == testSigGen)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Switch " << toString(dir) << " ForceMs due to Test Signal Gen enabled for portId: " << portId << endl;
        msMode = MAINTENANCE_SIGNAL_IDLE;
    }

    ms signal;

    switch (msMode)
    {
        case MAINTENANCE_SIGNAL_NOREPLACE:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
            break;
        case MAINTENANCE_SIGNAL_RF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
            break;
        case MAINTENANCE_SIGNAL_LF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
            break;
        case MAINTENANCE_SIGNAL_IDLE:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
            break;
        case MAINTENANCE_SIGNAL_LASEROFF:
            signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad MS Mode: " << mode << " portId: " << portId << '\n';
            return false;
    }

    if (dir == DIRECTION_TX)
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_force_signal_tx(signal);
    }
    else
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_force_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeTestSignalGen(string aid, Direction dir, bool mode, MaintenanceSignal cacheOverrideForceMs)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    if (dir != DIRECTION_TX && dir != DIRECTION_RX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Direction: " << dir << " Port Id: " << portId << '\n';
        return false;
    }

    MaintenanceSignal msMode;
    if (MAINTENANCE_SIGNAL_UNSPECIFIED == cacheOverrideForceMs)
    {
        DpGigeClientMgrSingleton::Instance()->getMspCfgHndlr()->getChm6GigeClientConfigForceMsFromCache(aid, dir, msMode);
        INFN_LOG(SeverityLevel::info) << __func__ << " " << toString(dir) << " ForceMs set " << EnumTranslate::toString(msMode) << " from cache due to Test Signal Gen disabled for Port Id: " << portId << endl;
    }
    else
    {
        msMode = cacheOverrideForceMs;
        INFN_LOG(SeverityLevel::info) << __func__ << " " << toString(dir) << " ForceMs set " << EnumTranslate::toString(msMode) << " from override due to Test Signal Gen disabled for Port Id: " << portId << endl;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " mode : " << mode << ", dir: " << (int)dir << " Port Id: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }
    ms signal;

    if (false == mode)
    {
        switch(msMode)
        {
            case MAINTENANCE_SIGNAL_LF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                break;
            case MAINTENANCE_SIGNAL_RF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
                break;
            case MAINTENANCE_SIGNAL_IDLE:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                break;
            case MAINTENANCE_SIGNAL_NOREPLACE:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
            case MAINTENANCE_SIGNAL_LASEROFF:
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
            default:
                INFN_LOG(SeverityLevel::info) << __func__ << " Warning unknown set MS default" << endl;
                signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }
    }
    else
    {
        signal = ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
    }

    if (dir == DIRECTION_TX)
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_force_signal_tx(signal);
    }
    else
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_ms_force_signal_rx(signal);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeTestSignalMon(string aid, bool enable)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__  << " Test Signal Monitor enable :  " << (int) enable  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    if (true == enable)
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_prbs_mon_rx(prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_GIGE_IDLE);
    }
    else
    {
        gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->set_prbs_mon_rx(prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_PRBS_NONE);
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeFwdTdaTrigger(string aid, bool enable)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " FWD TDA trigger :  " << (int) enable  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fwd_tda_trigger()->set_value(enable);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeFecDegSerEn(string aid, bool enable)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " FEC Deg SER enable:  " << enable  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_mon()->set_value(enable);


    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeFecDegSerActThresh(string aid, double actThresh)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " FEC Deg SER activate threshold:  " << actThresh  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    double fecDegSerActOffset   = actThresh;
    fecDegSerActOffset         *= pow(10, DCO_FEC_DEG_TH_PRECISION);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_activate_th()->set_digits(fecDegSerActOffset);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_activate_th()->set_precision(DCO_FEC_DEG_TH_PRECISION);


    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::setGigeFecDegSerDeactThresh(string aid, double deactThresh)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " FEC Deg SER deactivate threshold:  " << deactThresh  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    double fecDegSerDeActOffset = deactThresh;
    fecDegSerDeActOffset       *= pow(10, DCO_FEC_DEG_TH_PRECISION);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_deactivate_th()->set_digits(fecDegSerDeActOffset);
    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_deactivate_th()->set_precision(DCO_FEC_DEG_TH_PRECISION);


    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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




bool DcoGigeClientAdapter::setGigeFecDegSerMonPeriod(string aid, uint32_t monPer)
{
    bool status = true;
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " FEC Deg SER monitoring period:  " << monPer  << " portId: " << portId << endl;

    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    gige.mutable_client_gige(0)->mutable_client_gige()->mutable_config()->mutable_fec_deg_ser_mon_period()->set_value(monPer);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

//
//  Get/Read methods
//

bool DcoGigeClientAdapter::gigeConfigInfo(ostream& out, string aid)
{
    GigeCommon cfg;
    memset(&cfg, 0, sizeof(cfg));
    bool status = getGigeConfig(aid, &cfg);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }
    out << " Gige Client Config Info: " << aid  << endl;

    dumpConfigInfo(out, cfg);

    return status;
}

void DcoGigeClientAdapter::dumpConfigInfo(std::ostream& out, const GigeCommon& cfg)
{
    out << " Name (AID)       : " << cfg.name << endl;
    out << " Client Serdes Lane Config: " << endl;
    out << " Lane txPol rxPol Pre2 Pre1 main post1 post2 post3" << endl;
    for (int i = 0; i < 8; i++ )
    {
        out << cfg.clientSerdesLane[i] << " " << cfg.serdes[i].txPolarity << " " << cfg.serdes[i].rxPolarity << " " << cfg.serdes[i].txPre2 << " " << cfg.serdes[i].txPre1 << " " << cfg.serdes[i].txMain << " " << cfg.serdes[i].txPost1 << " " << cfg.serdes[i].txPost2 << " " << cfg.serdes[i].txPost3 << endl;
    }
    out << endl;
    out << " MTU              : " << cfg.mtu << endl;
    out << " LLDP Drop  Tx    : " << EnumTranslate::toString(cfg.lldpTxDrop) << endl;
    out << " LLDP Drop  Rx    : " << EnumTranslate::toString(cfg.lldpRxDrop) << endl;
    out << " LLDP Snoop Tx    : " << EnumTranslate::toString(cfg.lldpTxSnoop) << endl;
    out << " LLDP Snoop Rx    : " << EnumTranslate::toString(cfg.lldpRxSnoop) << endl;
    out << " FWD TDA          : " << EnumTranslate::toString(cfg.fwdTdaTrigger) << endl;
    out << " FEC enable       : " << EnumTranslate::toString(cfg.fecEnable) << endl;
    out << " ETH Mode         : " << EnumTranslate::toString(cfg.ethMode) << endl;
    out << " INTF Type        : " << EnumTranslate::toString(cfg.intfType) << endl;
    out << " Service Mode     : " << EnumTranslate::toString(cfg.srvMode) << endl;
    out << " Service Qualifier: " << EnumTranslate::toString(cfg.srvQMode) << endl;
    out << " Force MS Tx      : " << cfg.forceMsTx << "  " << EnumTranslate::toString(cfg.forceMsTx) << endl;
    out << " Force MS Rx      : " << cfg.forceMsRx << "  " << EnumTranslate::toString(cfg.forceMsRx) << endl;
    out << " Auto MS Tx       : " << cfg.autoMsTx  << "  " << EnumTranslate::toString(cfg.autoMsTx)  << endl;
    out << " Auto MS Rx       : " << cfg.autoMsRx  << "  " << EnumTranslate::toString(cfg.autoMsRx)  << endl;
    out << " Test Signal Mon  : " << EnumTranslate::toString(cfg.monitorRx) << endl;
    out << " LoopBack         : " << cfg.lpbk      << "  " << EnumTranslate::toString(cfg.lpbk )<< endl;
    out << " Rate             : " << cfg.rate      << "  " << EnumTranslate::toString(cfg.rate) << endl;
    out << " FEC Deg SER Ena  : " << EnumTranslate::toString(cfg.fecDegSer.enable) << endl;
    out << " FEC Deg SER Act  : " << fixed << setprecision(DCO_FEC_DEG_TH_PRECISION) << cfg.fecDegSer.activateThreshold   << endl;
    out << " FEC Deg SER DeAct: " << fixed << setprecision(DCO_FEC_DEG_TH_PRECISION) << cfg.fecDegSer.deactivateThreshold << endl;
    out << " FEC Deg SER Mon  : " << cfg.fecDegSer.monitorPeriod       << endl;
    out << " Regen            : " << EnumTranslate::toString(cfg.regen) << endl;
    out << " transactionId    : 0x" << hex <<cfg.transactionId << dec << endl;

    int gigeId = aidToPortId(cfg.name);
    if ((gigeId > 0) &&  (gigeId <= MAX_CLIENTS))
    {
        out << " TDA HoldOff      : " << EnumTranslate::toString(mTdaHoldOff[gigeId]) << endl;
    }
    
    INFN_LOG(SeverityLevel::info) << __func__ << " fec deg=" <<  cfg.fecDegSer.enable << endl;
    INFN_LOG(SeverityLevel::info) << __func__ << " fec deg act=" <<  cfg.fecDegSer.activateThreshold << endl;

#if TEST_LLDP
    // LLDP Flow Id test code
    for (uint32_t flowId = 0; flowId < DCO_MAX_SERDES_LANES; flowId++)
    {
        uint32_t portId;
        if (getLldpPortIdFromFlowId(flowId,  &portId))
        {
            // Dump out one base flow id
            out << " LLDP flowId: " << flowId << " PortId: " << portId << endl;
        }
    }
#endif
}

bool DcoGigeClientAdapter::gigeConfigInfo(ostream& out)
{
    TypeMapGigeConfig mapCfg;

    if (!getGigeConfig(mapCfg))
    {
        out << "*** FAILED to get Gige Status!!" << endl;
        return false;
    }

    out << "*** Gige Config Info ALL: " << endl;

    for(auto& it : mapCfg)
    {
        out << " Gige Config Info: "  << it.first << endl;
        dumpConfigInfo(out, *it.second);
        out << endl;
    }

    return true;
}

bool DcoGigeClientAdapter::gigeStatusInfo(ostream& out, string aid)
{
    GigeStatus stat;
    memset(&stat, 0, sizeof(stat));
    bool status = getGigeStatus(aid, &stat);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }

    dumpStatusInfo(out, stat);

    return status;
}

void DcoGigeClientAdapter::dumpStatusInfo(std::ostream& out, const GigeStatus& stat)
{
    out << " Gige Client Status Info: "  << endl;
    out << " Client Serdes Lane: " << endl;
    for (int i = 0; i < 8; i++ )
    {
        out << stat.eth.clientSerdesLane[i] << " " << stat.eth.serdes[i].txPolarity << " " << stat.eth.serdes[i].rxPolarity << " " << stat.eth.serdes[i].txPre2 << " " << stat.eth.serdes[i].txPre1 << " " << stat.eth.serdes[i].txMain << " " << stat.eth.serdes[i].txPost1 << " " << stat.eth.serdes[i].txPost2 << " " << stat.eth.serdes[i].txPost3 << endl;
    }
    out << endl;
    out << " MTU: " << stat.eth.mtu << endl;
    out << " LLDP Drop  Tx: " << EnumTranslate::toString(stat.eth.lldpTxDrop) << endl;
    out << " LLDP Drop  Rx: " << EnumTranslate::toString(stat.eth.lldpRxDrop) << endl;
    out << " LLDP Snoop Tx: " << EnumTranslate::toString(stat.eth.lldpTxSnoop) << endl;
    out << " LLDP Snoop Rx: " << EnumTranslate::toString(stat.eth.lldpRxSnoop) << endl;
    out << " FWD TDA: " << EnumTranslate::toString(stat.eth.fwdTdaTrigger) << endl;
    out << " FEC enable: " << EnumTranslate::toString(stat.eth.fecEnable) << endl;
    out << " ETH Mode: " << EnumTranslate::toString(stat.eth.ethMode) << endl;
    out << " INTF Type: " << EnumTranslate::toString(stat.eth.intfType) << endl;
    out << " Service Mode: " << EnumTranslate::toString(stat.eth.srvMode) << endl;
    out << " Service Qualifier: " << EnumTranslate::toString(stat.eth.srvQMode) << endl;
    out << " Force MS Tx: " << EnumTranslate::toString(stat.eth.forceMsTx) << endl;
    out << " Force MS Rx: " << EnumTranslate::toString(stat.eth.forceMsRx) << endl;
    out << " Auto MS Tx: " << EnumTranslate::toString(stat.eth.autoMsTx) << endl;
    out << " Auto MS Rx: " << EnumTranslate::toString(stat.eth.autoMsRx) << endl;
    out << " Test Signal Mon: " << EnumTranslate::toString(stat.eth.monitorRx) << endl;
    out << " LoopBack: " << EnumTranslate::toString(stat.eth.lpbk) << endl;
    out << " Rate: " << EnumTranslate::toString(stat.eth.rate) << endl;
    out << " Max MTU: " << stat.maxMtu << endl;
    out << " Port Status: " << EnumTranslate::toString(stat.status) << endl;
    out << " Port Fac Fault: 0x" << hex << stat.facFault << dec << endl;
    out << " Regen            : " << EnumTranslate::toString(stat.eth.regen) << endl;
    out << " TransactionId    : 0x" << hex << stat.eth.transactionId << dec << endl;
}

bool DcoGigeClientAdapter::gigeFaultInfo(ostream& out, string aid)
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;
    int portId = aidToPortId(aid);

    GigeFault fault;
    status = getGigeFault( aid, &fault );

    if (status == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Gige Fault FAIL "  << endl;
        return status;
    }

    out << endl;
    out << "GIGE FAC Fault: 0x" << hex << fault.facBmp << dec  << endl;
    uint bitPos = 0;
    for ( auto i = fault.fac.begin(); i != fault.fac.end(); i++, bitPos++)
    {
        out << " Fault: " << left << std::setw(17) << i->faultName << " Bit" << left << std::setw(2) << bitPos << " Faulted: " << (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }
    out << "\n mGigeFltSubEnable: " << EnumTranslate::toString(mGigeFltSubEnable);
    out << "\n mGigePmSubEnable: " << EnumTranslate::toString(mGigePmSubEnable) << endl;
    out << "\n mGigeOpStatusSubEnable: " << EnumTranslate::toString(mGigeOpStatusSubEnable) << endl;
    out << "\n mopStatus.opStatus: " << EnumTranslate::toString(mOpStatus[portId - 1].opStatus) << endl;
    out << "\n mOpStatus.transactionId: " << mOpStatus[portId - 1].transactionId << endl;
    out << endl;
    return status;
}

void DcoGigeClientAdapter::dumpAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    gigeClient gige;

    uint32 cfgId = 1;
    if (setKey(&gige, cfgId) == false)
    {
        out << "Failed to set key" << endl;

        return;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

    {
        std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

        grpc::Status gStatus = mspCrud->Read( msg );
        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;

            out << " Dco Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;

            return;
        }
    }

    for (int idx = 0; idx < gige.client_gige_size(); idx++)
    {
        if (!gige.client_gige(idx).client_gige().has_config())
        {
            out << "*** Gige Config not present for idx: " << idx << " Skipping!" << endl;

            continue;
        }

        uint32 objId = gige.client_gige(idx).client_gige().config().client_port_id().value();

        string gigeAid;
        if (gige.client_gige(idx).client_gige().config().has_name())
        {
            gigeAid = gige.client_gige(idx).client_gige().config().name().value();

            // todo: remove print
            INFN_LOG(SeverityLevel::debug) << "Gige has name: " << gigeAid;
        }
        else
        {
            gigeAid = IdGen::genGigeAid(objId);

            INFN_LOG(SeverityLevel::debug) << "Gige has NO NAME. Using old genID created AID: " << gigeAid;
        }

        GigeCommon gigeCmn;

        translateConfig(gige.client_gige(idx).client_gige().config(), gigeCmn);

        out << "*** Gige Config Info for PortID: " << objId << " Aid: " << gigeAid << endl;

        dumpConfigInfo(out, gigeCmn);

        out << "*** Gige Fault Info for PortID: " << objId << " Aid: " << gigeAid << endl;

        if (gigeFaultInfo(out, gigeAid) == false)
        {
            out << "gigeFaultInfo Failed for Aid " << gigeAid << endl;
        }

        if (gige.client_gige(idx).client_gige().has_state())
        {
            GigeStatus gigeStat;

            translateStatus(gige.client_gige(idx).client_gige().state(), gigeStat);

            out << "*** Gige Status Info for PortID: " << objId << " Aid: " << gigeAid << endl;

            dumpStatusInfo(out, gigeStat);
        }
        else
        {
            out << "*** Gige State not present for PortID: " << objId << " Aid: " << gigeAid << endl;
        }

        out << "*** Gige Pm Info for PortID: " << objId << " Aid: " << gigeAid << endl;

        if (gigePmInfo(out, objId) == false)
        {
            out << "gigePmInfo Failed for Aid " << gigeAid << endl;
        }
    }

}

bool DcoGigeClientAdapter::getGigeConfig(string aid, GigeCommon *cfg)
{
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " portId: " << portId << endl;
    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getGigeObj( &gige, portId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco GigeClient Object  FAIL "  << endl;
        return status;
    }

    if (gige.client_gige(idx).client_gige().has_config() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO Config Empty ... PortId: " <<  portId << endl;
        return false;
    }

    translateConfig(gige.client_gige(idx).client_gige().config(), *cfg);

    return status;
} // getGigeConfig

void DcoGigeClientAdapter::translateConfig(const gigeClientConfig& dcoCfg, GigeCommon& adCfg)
{
    for (int i = 0; i < dcoCfg.client_serdes_lanes_size(); i++)
    {
        adCfg.clientSerdesLane[i] = dcoCfg.client_serdes_lanes(i).client_serdes_lane();
        adCfg.serdes[i].txPolarity = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().tx_lane_polarity().value();
        adCfg.serdes[i].rxPolarity = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().rx_lane_polarity().value();
        adCfg.serdes[i].txPre2 = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().tx_fir_precursor_2().value();
        adCfg.serdes[i].txPre1 = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().tx_fir_precursor_1().value();
        adCfg.serdes[i].txMain = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().tx_fir_main_tap().value();
        adCfg.serdes[i].txPost1 = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().tx_fir_postcursor_1().value();
        adCfg.serdes[i].txPost2 = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().tx_fir_postcursor_2().value();
        adCfg.serdes[i].txPost3 = dcoCfg.client_serdes_lanes(i).client_serdes_lanes().tx_fir_postcursor_3().value();
    }
    if (dcoCfg.has_name())
    {
        adCfg.name = dcoCfg.name().value();
    }
    if (dcoCfg.has_mtu())
    {
        adCfg.mtu = dcoCfg.mtu().value();
    }
    if (dcoCfg.has_lldp_tx_drop())
    {
        adCfg.lldpTxDrop = dcoCfg.lldp_tx_drop().value();
    }
    if (dcoCfg.has_lldp_rx_drop())
    {
        adCfg.lldpRxDrop = dcoCfg.lldp_rx_drop().value();
    }
    if (dcoCfg.has_lldp_tx_snooping_enable())
    {
        adCfg.lldpTxSnoop = dcoCfg.lldp_tx_snooping_enable().value();
    }
    if (dcoCfg.has_lldp_rx_snooping_enable())
    {
        adCfg.lldpRxSnoop = dcoCfg.lldp_rx_snooping_enable().value();
    }
    if (dcoCfg.has_fwd_tda_trigger())
    {
        adCfg.fwdTdaTrigger = dcoCfg.fwd_tda_trigger().value();
    }
    if (dcoCfg.has_fec_enable())
    {
        adCfg.fecEnable = dcoCfg.fec_enable().value();
    }

    if (dcoCfg.has_fec_deg_ser_mon())
    {
        adCfg.fecDegSer.enable = dcoCfg.fec_deg_ser_mon().value();
    }

    // Translate back from DCO yang
    if (dcoCfg.has_fec_deg_ser_activate_th())
    {
        int64 digit                       = dcoCfg.fec_deg_ser_activate_th().digits();
        uint32 precision                  = dcoCfg.fec_deg_ser_activate_th().precision();
        adCfg.fecDegSer.activateThreshold = digit/pow(10, precision);
    }

    if (dcoCfg.has_fec_deg_ser_deactivate_th())
    {
        int64 digit                         = dcoCfg.fec_deg_ser_deactivate_th().digits();
        uint32 precision                    = dcoCfg.fec_deg_ser_deactivate_th().precision();
        adCfg.fecDegSer.deactivateThreshold = digit/pow(10, precision);
    }

    if (dcoCfg.has_fec_deg_ser_mon_period())
    {
        adCfg.fecDegSer.monitorPeriod = dcoCfg.fec_deg_ser_mon_period().value();
    }


    switch (dcoCfg.ethernet_mode())
    {
        case clientConfig::ETHERNETMODE_TRANSPARENT:
            adCfg.ethMode = ETHERNETMODE_TRANSPARENT;
            break;
        case clientConfig::ETHERNETMODE_RETIMED:
            adCfg.ethMode = ETHERNETMODE_RETIMED;
            break;
        default:
            adCfg.ethMode = ETHERNETMODE_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.interface_type())
    {
        case clientConfig::INTERFACETYPE_INTF_CAUI_4:
            adCfg.intfType = INTERFACETYPE_CAUI_4;
            break;
        case clientConfig::INTERFACETYPE_INTF_GAUI:
            adCfg.intfType = INTERFACETYPE_GAUI;
            break;
        default:
            adCfg.intfType = INTERFACETYPE_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ethernet_flex_type())
    {
        case clientConfig::ETHERNETFLEXTYPE_FLEX_NONE:
            adCfg.fType = ETHERNETFLEXTYPE_NONE;
            break;
        default:
            adCfg.fType = ETHERNETFLEXTYPE_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.service_mode())
    {
        case clientConfig::SERVICEMODE_MODE_NONE:
            adCfg.srvMode = SERVICEMODE_NONE;
            break;
        case clientConfig::SERVICEMODE_MODE_TRANSPORT:
            adCfg.srvMode = SERVICEMODE_TRANSPORT;
            break;
        case clientConfig::SERVICEMODE_MODE_SWITCHING:
            adCfg.srvMode = SERVICEMODE_SWITCHING;
            break;
        default:
            adCfg.srvMode = SERVICEMODE_UNSPECIFIED;
            break;
    }
    switch ( dcoCfg.service_mode_qualifier())
    {
        case clientConfig::SERVICEMODEQUALIFIER_SMQ_NONE:
            adCfg.srvQMode = SERVICEMODEQUALIFIER_NONE;
            break;
        default:
            adCfg.srvQMode = SERVICEMODEQUALIFIER_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_force_signal_tx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            adCfg.forceMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_force_signal_rx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            adCfg.forceMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_auto_signal_rx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            adCfg.autoMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.ms_auto_signal_tx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            adCfg.autoMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.prbs_mon_rx())
    {
        case prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_PRBS_NONE:
            adCfg.monitorRx = false;
            break;
        case prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_GIGE_IDLE:
            adCfg.monitorRx = true;
            break;
        default:
            adCfg.monitorRx = false;
            break;
    }
    switch (dcoCfg.loopback_type())
    {
        case clientConfig::LOOPBACKTYPE_NONE:
            adCfg.lpbk = LOOP_BACK_TYPE_OFF;
            break;
        case clientConfig::LOOPBACKTYPE_FACILITY:
            adCfg.lpbk = LOOP_BACK_TYPE_FACILITY;
            break;
        case clientConfig::LOOPBACKTYPE_TERMINAL:
            adCfg.lpbk = LOOP_BACK_TYPE_TERMINAL;
            break;
        default:
            adCfg.lpbk = LOOP_BACK_TYPE_UNSPECIFIED;
            break;
    }
    switch (dcoCfg.payload_type())
    {
        case clientConfig::PAYLOADTYPE_400GB_ELAN:
            adCfg.rate = PORT_RATE_ETH400G;
            break;
        case clientConfig::PAYLOADTYPE_100GB_ELAN:
            adCfg.rate = PORT_RATE_ETH100G;
            break;
        default:
            adCfg.rate = PORT_RATE_UNSET;
            break;
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

bool DcoGigeClientAdapter::getGigeConfig(TypeMapGigeConfig& mapCfg)
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    gigeClient gige;

    uint32 cfgId = 1;
    if (setKey(&gige, cfgId) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);

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

    INFN_LOG(SeverityLevel::info) << "size: " << gige.client_gige_size();

    for (int idx = 0; idx < gige.client_gige_size(); idx++)
    {
        string gigeAid;
        if (gige.client_gige(idx).client_gige().config().has_name())
        {
            gigeAid = gige.client_gige(idx).client_gige().config().name().value();

            INFN_LOG(SeverityLevel::info) << "Gige has name: " << gigeAid;
        }
        else
        {
            gigeAid = IdGen::genGigeAid(gige.client_gige(idx).client_gige().config().client_port_id().value());

            INFN_LOG(SeverityLevel::info) << "Gige has NO NAME. Using old genID created AID: " << gigeAid;
        }

        if (gige.client_gige(idx).client_gige().config().has_regen())
        {
            if (gige.client_gige(idx).client_gige().config().regen().value() == true)
            {
                INFN_LOG(SeverityLevel::info) << "Skip update: This is HAL create Gige Regen AID: " << gigeAid << " IDX: " << idx << endl;
                // This is a regen client, don't update InstallConfig
                continue;
            }
        }

        mapCfg[gigeAid] = make_shared<GigeCommon>();
        translateConfig(gige.client_gige(idx).client_gige().config(), *mapCfg[gigeAid]);
    }

    return status;
}

bool DcoGigeClientAdapter::getGigeStatus(string aid, GigeStatus *stat)
{
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " portId: " << portId << endl;
    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getGigeObj( &gige, portId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco GigeClient Object  FAIL "  << endl;
        return status;
    }

    if (gige.client_gige(idx).client_gige().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... PortId: " <<  portId << endl;
        return false;
    }

    translateStatus(gige.client_gige(idx).client_gige().state(), *stat);

    return status;
} // getGigeStatus

void DcoGigeClientAdapter::translateStatus(const gigeClientState& gigeState, GigeStatus& stat)
{
    for (int i = 0; i < gigeState.client_serdes_lanes_size(); i++)
    {
        stat.eth.clientSerdesLane[i] = gigeState.client_serdes_lanes(i).client_serdes_lane();
        stat.eth.serdes[i].txPolarity = gigeState.client_serdes_lanes(i).client_serdes_lanes().tx_lane_polarity().value();
        stat.eth.serdes[i].rxPolarity = gigeState.client_serdes_lanes(i).client_serdes_lanes().rx_lane_polarity().value();
        stat.eth.serdes[i].txPre2 = gigeState.client_serdes_lanes(i).client_serdes_lanes().tx_fir_precursor_2().value();
        stat.eth.serdes[i].txPre1 = gigeState.client_serdes_lanes(i).client_serdes_lanes().tx_fir_precursor_1().value();
        stat.eth.serdes[i].txMain = gigeState.client_serdes_lanes(i).client_serdes_lanes().tx_fir_main_tap().value();
        stat.eth.serdes[i].txPost1 = gigeState.client_serdes_lanes(i).client_serdes_lanes().tx_fir_postcursor_1().value();
        stat.eth.serdes[i].txPost2 = gigeState.client_serdes_lanes(i).client_serdes_lanes().tx_fir_postcursor_2().value();
        stat.eth.serdes[i].txPost3 = gigeState.client_serdes_lanes(i).client_serdes_lanes().tx_fir_postcursor_3().value();
    }
    if (gigeState.has_mtu())
    {
        stat.eth.mtu = gigeState.mtu().value();
    }
    if (gigeState.has_lldp_tx_drop())
    {
        stat.eth.lldpTxDrop = gigeState.lldp_tx_drop().value();
    }
    if (gigeState.has_lldp_rx_drop())
    {
        stat.eth.lldpRxDrop = gigeState.lldp_rx_drop().value();
    }
    if (gigeState.has_lldp_tx_snooping_enable())
    {
        stat.eth.lldpTxSnoop = gigeState.lldp_tx_snooping_enable().value();
    }
    if (gigeState.has_lldp_rx_snooping_enable())
    {
        stat.eth.lldpRxSnoop = gigeState.lldp_rx_snooping_enable().value();
    }
    if (gigeState.has_fwd_tda_trigger())
    {
        stat.eth.fwdTdaTrigger = gigeState.fwd_tda_trigger().value();
    }
    if (gigeState.has_fec_enable())
    {
        stat.eth.fecEnable = gigeState.fec_enable().value();
    }
    switch (gigeState.ethernet_mode())
    {
        case ClientsGige_ClientGige_State::ETHERNETMODE_TRANSPARENT:
            stat.eth.ethMode = ETHERNETMODE_TRANSPARENT;
            break;
        case ClientsGige_ClientGige_State::ETHERNETMODE_RETIMED:
            stat.eth.ethMode = ETHERNETMODE_RETIMED;
            break;
        default:
            stat.eth.ethMode = ETHERNETMODE_UNSPECIFIED;
            break;
    }
    switch (gigeState.interface_type())
    {
        case ClientsGige_ClientGige_State::INTERFACETYPE_INTF_GAUI:
            stat.eth.intfType = INTERFACETYPE_GAUI;
            break;
        case ClientsGige_ClientGige_State::INTERFACETYPE_INTF_CAUI_4:
            stat.eth.intfType = INTERFACETYPE_CAUI_4;
            break;
        default:
            stat.eth.intfType = INTERFACETYPE_UNSPECIFIED;
            break;
    }
    switch (gigeState.ethernet_flex_type())
    {
        case ClientsGige_ClientGige_State::ETHERNETFLEXTYPE_FLEX_NONE:
            stat.eth.fType = ETHERNETFLEXTYPE_NONE;
            break;
        default:
            stat.eth.fType = ETHERNETFLEXTYPE_UNSPECIFIED;
            break;
    }
    switch (gigeState.service_mode())
    {
        case ClientsGige_ClientGige_State::SERVICEMODE_MODE_NONE:
            stat.eth.srvMode = SERVICEMODE_NONE;
            break;
        case ClientsGige_ClientGige_State::SERVICEMODE_MODE_TRANSPORT:
            stat.eth.srvMode = SERVICEMODE_TRANSPORT;
            break;
        case ClientsGige_ClientGige_State::SERVICEMODE_MODE_SWITCHING:
            stat.eth.srvMode = SERVICEMODE_SWITCHING;
            break;
        default:
            stat.eth.srvMode = SERVICEMODE_UNSPECIFIED;
            break;
    }
    switch ( gigeState.service_mode_qualifier())
    {
        case ClientsGige_ClientGige_State::SERVICEMODEQUALIFIER_SMQ_NONE:
            stat.eth.srvQMode = SERVICEMODEQUALIFIER_NONE;
            break;
        default:
            stat.eth.srvQMode = SERVICEMODEQUALIFIER_UNSPECIFIED;
            break;
    }
    switch (gigeState.ms_force_signal_tx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            stat.eth.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            stat.eth.forceMsTx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            stat.eth.forceMsTx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            stat.eth.forceMsTx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            stat.eth.forceMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (gigeState.ms_force_signal_rx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            stat.eth.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            stat.eth.forceMsRx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            stat.eth.forceMsRx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            stat.eth.forceMsRx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            stat.eth.forceMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (gigeState.ms_auto_signal_rx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            stat.eth.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            stat.eth.autoMsRx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            stat.eth.autoMsRx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            stat.eth.autoMsRx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            stat.eth.autoMsRx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (gigeState.ms_auto_signal_tx())
    {
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE:
            stat.eth.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
            stat.eth.autoMsTx = MAINTENANCE_SIGNAL_RF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
            stat.eth.autoMsTx = MAINTENANCE_SIGNAL_LF;
            break;
        case ms::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
            stat.eth.autoMsTx = MAINTENANCE_SIGNAL_IDLE;
            break;
        default:
            stat.eth.autoMsTx = MAINTENANCE_SIGNAL_UNSPECIFIED;
            break;
    }
    switch (gigeState.prbs_mon_rx())
    {
        case prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_PRBS_NONE:
            stat.eth.monitorRx = false;
            break;
        case prbsmode::INFINERADCOCLIENTGIGEPRBSMODE_GIGE_IDLE:
            stat.eth.monitorRx = true;
            break;
        default:
            stat.eth.monitorRx = false;
            break;
    }
    switch (gigeState.loopback_type())
    {
        case ClientsGige_ClientGige_State::LOOPBACKTYPE_NONE:
            stat.eth.lpbk = LOOP_BACK_TYPE_OFF;
            break;
        case ClientsGige_ClientGige_State::LOOPBACKTYPE_FACILITY:
            stat.eth.lpbk = LOOP_BACK_TYPE_FACILITY;
            break;
        case ClientsGige_ClientGige_State::LOOPBACKTYPE_TERMINAL:
            stat.eth.lpbk = LOOP_BACK_TYPE_TERMINAL;
            break;
        default:
            stat.eth.lpbk = LOOP_BACK_TYPE_UNSPECIFIED;
            break;
    }
    switch (gigeState.payload_type())
    {
        case ClientsGige_ClientGige_State::PAYLOADTYPE_400GB_ELAN:
            stat.eth.rate = PORT_RATE_ETH400G;
            break;
        case ClientsGige_ClientGige_State::PAYLOADTYPE_100GB_ELAN:
            stat.eth.rate = PORT_RATE_ETH100G;
            break;
        default:
            stat.eth.rate = PORT_RATE_UNSET;
            break;
    }
    switch(gigeState.client_status())
    {
        case ClientsGige_ClientGige_State::CLIENTSTATUS_UP:
            stat.status = CLIENTSTATUS_UP;
            break;
        case ClientsGige_ClientGige_State::CLIENTSTATUS_DOWN:
            stat.status = CLIENTSTATUS_DOWN;
            break;
        case ClientsGige_ClientGige_State::CLIENTSTATUS_FAULTED:
            stat.status = CLIENTSTATUS_FAULTED;
            break;
        default:
            stat.status = CLIENTSTATUS_UNSPECIFIED;
            break;
    }
    if (gigeState.has_max_mtu_size())
    {
        stat.maxMtu = gigeState.max_mtu_size().value();
    }

    if (gigeState.has_facility_fault())
    {
        // todo: is this needed ??

        stat.facFault = gigeState.facility_fault().value();

        uint32 portId = gigeState.client_port_id().value();

        //todo: remove print
        INFN_LOG(SeverityLevel::info) << "Fac Fault Update. Gige PortId: " << portId;

        std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

        if ((portId - 1) < MAX_CLIENTS)
        {
            mGigeFault[portId - 1].faultBitmap = stat.facFault;
        }
    }

    if (gigeState.has_regen())
    {
        stat.eth.regen = gigeState.regen().value();
    }

    if (gigeState.has_tid())
    {
        stat.eth.transactionId = gigeState.tid().value();
    }
}

bool DcoGigeClientAdapter::getGigeMaxMtu(string aid, int *maxMtu)
{
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " portId: " << portId << endl;
    gigeClient gige;
    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getGigeObj( &gige, portId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco GigeClient Object  FAIL "  << endl;
        return status;
    }
    if (gige.client_gige(idx).client_gige().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... PortId: " <<  portId << endl;
        return false;
    }
    if (gige.client_gige(idx).client_gige().state().has_max_mtu_size())
    {
        *maxMtu = gige.client_gige(idx).client_gige().state().max_mtu_size().value();
    }

    return status;
}
bool DcoGigeClientAdapter::getGigeLinkStatus(string aid, ClientStatus *linkStat)
{
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " portId: " << portId << endl;
    gigeClient gige;
    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getGigeObj( &gige, portId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco GigeClient Object  FAIL "  << endl;
        return status;
    }

    if (gige.client_gige(idx).client_gige().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... PortId: " <<  portId << endl;
        return false;
    }

    switch(gige.client_gige(idx).client_gige().state().client_status())
    {
        case ClientsGige_ClientGige_State::CLIENTSTATUS_UP:
            *linkStat = CLIENTSTATUS_UP;
            break;
        case ClientsGige_ClientGige_State::CLIENTSTATUS_DOWN:
            *linkStat = CLIENTSTATUS_DOWN;
            break;
        case ClientsGige_ClientGige_State::CLIENTSTATUS_FAULTED:
            *linkStat = CLIENTSTATUS_FAULTED;
            break;
        default:
            *linkStat = CLIENTSTATUS_UNSPECIFIED;
            break;
    }
    return status;
}

//
// Get LLDP Snoop Packet TX or RX
bool DcoGigeClientAdapter::getGigeLldpPkt(string aid, Direction dir, uint8_t *ptk)
{
    bool status = true;
    return status;
}

bool DcoGigeClientAdapter::getGigeFault(string aid, uint64_t *fault)
{
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " portId: " << portId << " aid=" << aid << endl;
    gigeClient gige;
    if (setKey(&gige, portId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getGigeObj( &gige, portId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco GigeClient Object  FAIL "  << endl;
        return status;
    }

    if (gige.client_gige(idx).client_gige().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... PortId: " <<  portId << endl;
        return false;
    }
    if (gige.client_gige(idx).client_gige().state().has_facility_fault())
    {
        *fault = gige.client_gige(idx).client_gige().state().facility_fault().value();
    }

    return status;
}

//force equal to true case should only be used for warm boot query
bool DcoGigeClientAdapter::getGigeFault(string aid, GigeFault *faults, bool force)
{
    int portId = aidToPortId(aid);
    if (portId < 1 || portId > 16)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << portId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " portId: " << portId << " aid=" << aid << endl;
    gigeClient gige;

    if (setKey(&gige, portId) == false)
    {

        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    bool status = true;
    //if gige fault is already been updated via notification, skip the retrieval
    if (force && (mGigeFault[portId - 1].isNotified == false))
    {
	    int idx = 0;
	    status = getGigeObj( &gige, portId, &idx);
	    if ( status == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco GigeClient Object  FAIL "  << endl;
		    return status;
	    }

	    if (gige.client_gige(idx).client_gige().has_state() == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... PortId: " <<  portId << endl;
		    return false;
	    }
	    if (gige.client_gige(idx).client_gige().state().has_facility_fault())
	    {
		    faults->facBmp = gige.client_gige(idx).client_gige().state().facility_fault().value();
		    if (mGigeFault[portId - 1].isNotified == false)
		    {
			    mGigeFault[portId - 1].faultBitmap = faults->facBmp;
			    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve gige fault " << portId << " with bitmap: 0x" <<  hex << mGigeFault[portId - 1].faultBitmap << dec << endl;
		    }
		    else
		    {
			    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve gige fault " << portId << " with fault notify flag:" <<  mGigeFault[portId - 1].isNotified << endl;
		    }
	    }
	    else
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no gige fault in state, portId: " << portId;
	    }
    }
    else
    {
	    faults->facBmp = mGigeFault[portId - 1].faultBitmap;
    }

    AdapterFault tmpFault;
    uint64_t faultBmp = faults->facBmp;
    for ( auto i = mFaultCapa.begin(); i != mFaultCapa.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (faultBmp & 0x1)
        {
            tmpFault.faulted = true;
        }
        faults->fac.push_back(tmpFault);
        faultBmp >>= 0x1;
    }
    return status;
}

bool DcoGigeClientAdapter::getLldpPortIdFromFlowId(uint32_t flowId,  uint32_t *portId)
{
    if  (flowId == 0 ||  flowId >= DCO_MAX_SERDES_LANES)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " ERROR: BAD flowId: "  << flowId << endl;
    }

    const clientPortEntry *pPortMap = &mChm6_PortMap[0];
    while ( pPortMap->fpPort != 0 )
    {
        if ( (pPortMap->clientLane        == flowId         ) &&
             (mPortRate[pPortMap->fpPort] == pPortMap->pRate) )  // check port rate to match for 100/400GE table entries
        {
            *portId = pPortMap->fpPort;
            return true;
        }

        ++pPortMap;
    }
    INFN_LOG(SeverityLevel::error) << __func__ << " cannot find portMapEntry ... flowId: "  << flowId << endl;
    return false;
}

bool DcoGigeClientAdapter::getGigePm(string aid)
{
	INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    return true;
}

void DcoGigeClientAdapter::subGigeFault()
{
	ClientGigeFms gs;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gs);
	::GnmiClient::AsyncSubCallback* cb = new GigeFaultResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << __func__ << " gige fault sub request sent to server.."  << endl;
}

void DcoGigeClientAdapter::subGigePm()
{
	if (dp_env::DpEnvSingleton::Instance()->isSimEnv() && mIsGnmiSimModeEn) {
		//spawn a thread to feed up pm data for sim
		if (mSimGigePmThread.joinable() == false) { //check whether the sim pm thread is running
			INFN_LOG(SeverityLevel::info) << "create mSimGigePmThread " << endl;
			mSimGigePmThread = std::thread(&DcoGigeClientAdapter::setSimPmData, this);
		}
		else {
			INFN_LOG(SeverityLevel::info) << "mSimGigePmThread is already running..." << endl;
		}
	}
	else {
		INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
		dcoGigePm cp;
		google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&cp);
		::GnmiClient::AsyncSubCallback* cb = new GigePmResponseContext(); //cleaned up in grpc client
		std::string cbId = mspCrud->Subscribe(msg, cb);
		INFN_LOG(SeverityLevel::info) << " gige pm sub request sent to server.."  << endl;
	}
}

void DcoGigeClientAdapter::subGigeOpStatus()
{
	dcoGigeOpStatus ops;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&ops);
	::GnmiClient::AsyncSubCallback* cb = new GigeOpStatusResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " gige op status sub request sent to server.."  << endl;
}

//set same sim pm data for all objects..
void DcoGigeClientAdapter::setSimPmData()
{
    while (1)
	{
		this_thread::sleep_for(seconds(1));

        bool isSimDcoConnect = DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)DCC_ZYNQ).isSimDcoConnect();
        if (!mIsGnmiSimModeEn || !isSimDcoConnect)
        {
            continue;
        }

		for (int i = 1; i <= MAX_CLIENTS; i++) {
			if (mGigePms.HasPmData(i) == false) continue;
			auto gigePm = mGigePms.GetPmData(i);
			int gigeId= gigePm.gigeId;
			INFN_LOG(SeverityLevel::debug) << __func__ << " gigeId: " << gigeId << ", gigePm.aid: " << gigePm.aid << ", i: " << i << endl;

			gigePm.DpMsClientGigePmMacTx.packets = 12345;
			gigePm.DpMsClientGigePmMacTx.error_packets = 67;
			gigePm.DpMsClientGigePmMacTx.ok_packets = 12278;
			gigePm.DpMsClientGigePmMacTx.octets = 12345;
			gigePm.DpMsClientGigePmMacTx.error_octets = 67;
			gigePm.DpMsClientGigePmMacTx.jabber_frames = 12;
			gigePm.DpMsClientGigePmMacTx.fragmented_frames = 34;
			gigePm.DpMsClientGigePmMacTx.crc_aligned_error = 123;
			gigePm.DpMsClientGigePmMacTx.under_sized = 345;
			gigePm.DpMsClientGigePmMacTx.over_sized = 456;
			gigePm.DpMsClientGigePmMacTx.broadcast_frames = 567;
			gigePm.DpMsClientGigePmMacTx.multicast_frames = 12;
			gigePm.DpMsClientGigePmMacTx.unicast_frames = 89;
			gigePm.DpMsClientGigePmMacTx.pause_frames = 78;
			gigePm.DpMsClientGigePmMacTx.bpdu_frames = 1;
			gigePm.DpMsClientGigePmMacTx.lacp_frames = 2;
			gigePm.DpMsClientGigePmMacTx.oam_frames = 3;
			gigePm.DpMsClientGigePmMacTx.fcs_error = 4;
			gigePm.DpMsClientGigePmMacTx.vlan_ok = 6;
			gigePm.DpMsClientGigePmMacTx.size_64 = 8;
			gigePm.DpMsClientGigePmMacTx.size_65_to_127 = 9;
			gigePm.DpMsClientGigePmMacTx.size_128_to_255 = 10;
			gigePm.DpMsClientGigePmMacTx.size_256_to_511 = 11;
			gigePm.DpMsClientGigePmMacTx.size_512_to_1023 = 12;
			gigePm.DpMsClientGigePmMacTx.size_1024_to_1518 = 13;
			gigePm.DpMsClientGigePmMacTx.jumbo_frames = 14;
			gigePm.DpMsClientGigePmMacTx.jumbo_to_extreme_jumbo_frames = 15;

			gigePm.DpMsClientGigePmMacRx.packets = 100;
			gigePm.DpMsClientGigePmMacRx.error_packets = 1;
			gigePm.DpMsClientGigePmMacRx.ok_packets = 99;
			gigePm.DpMsClientGigePmMacRx.octets = 100;
			gigePm.DpMsClientGigePmMacRx.error_octets = 1;
			gigePm.DpMsClientGigePmMacRx.jabber_frames = 2;
			gigePm.DpMsClientGigePmMacRx.fragmented_frames = 3;
			gigePm.DpMsClientGigePmMacRx.crc_aligned_error = 4;
			gigePm.DpMsClientGigePmMacRx.under_sized = 5;
			gigePm.DpMsClientGigePmMacRx.over_sized = 6;
			gigePm.DpMsClientGigePmMacRx.broadcast_frames = 7;
			gigePm.DpMsClientGigePmMacRx.multicast_frames = 8;
			gigePm.DpMsClientGigePmMacRx.unicast_frames = 9;
			gigePm.DpMsClientGigePmMacRx.pause_frames = 10;
			gigePm.DpMsClientGigePmMacRx.bpdu_frames = 11;
			gigePm.DpMsClientGigePmMacRx.lacp_frames = 12;
			gigePm.DpMsClientGigePmMacRx.oam_frames = 13;
			gigePm.DpMsClientGigePmMacRx.fcs_error = 14;
			gigePm.DpMsClientGigePmMacRx.vlan_ok = 16;
			gigePm.DpMsClientGigePmMacRx.size_64 = 18;
			gigePm.DpMsClientGigePmMacRx.size_65_to_127 = 19;
			gigePm.DpMsClientGigePmMacRx.size_128_to_255 = 20;
			gigePm.DpMsClientGigePmMacRx.size_256_to_511 = 21;
			gigePm.DpMsClientGigePmMacRx.size_512_to_1023 = 22;
			gigePm.DpMsClientGigePmMacRx.size_1024_to_1518 = 23;
			gigePm.DpMsClientGigePmMacRx.jumbo_frames = 24;
			gigePm.DpMsClientGigePmMacRx.jumbo_to_extreme_jumbo_frames = 25;

			gigePm.DpMsClientGigePmPcsTx.control_block = 101;
			gigePm.DpMsClientGigePmPcsTx.errored_blocks = 2;
			gigePm.DpMsClientGigePmPcsTx.idle_error = 5;
			gigePm.DpMsClientGigePmPcsTx.bip_total = 10;
			gigePm.DpMsClientGigePmPcsTx.test_pattern_error = 11;

			gigePm.DpMsClientGigePmPcsRx.control_block = 12;
			gigePm.DpMsClientGigePmPcsRx.errored_blocks = 13;
			gigePm.DpMsClientGigePmPcsRx.idle_error = 14;
			gigePm.DpMsClientGigePmPcsRx.bip_total = 15;
			gigePm.DpMsClientGigePmPcsRx.test_pattern_error = 16;

			gigePm.DpMsClientGigePmPhyFecRx.fec_corrected = 17;
			gigePm.DpMsClientGigePmPhyFecRx.fec_un_corrected = 18;
			gigePm.DpMsClientGigePmPhyFecRx.bit_error = 3;
			gigePm.DpMsClientGigePmPhyFecRx.invalid_transcode_block = 5;
			gigePm.DpMsClientGigePmPhyFecRx.bip_8_error = 1;

#if 0
			gigePm.DpMsClientGigePmPhyFec100Gige.fec_corrected = 1000;
			gigePm.DpMsClientGigePmPhyFec100Gige.fec_un_corrected = 2;
			gigePm.DpMsClientGigePmPhyFec100Gige.bit_error = 3;
			gigePm.DpMsClientGigePmPhyFec100Gige.invalid_transcode_block = 4;
			gigePm.DpMsClientGigePmPhyFec100Gige.bip_8_error = 9;

			gigePm.DpMsClientGigePmPhyFec200Gige.fec_corrected = 8002;
			gigePm.DpMsClientGigePmPhyFec200Gige.fec_un_corrected = 11;
			gigePm.DpMsClientGigePmPhyFec200Gige.bit_error = 21;
			gigePm.DpMsClientGigePmPhyFec200Gige.invalid_transcode_block = 13;
			gigePm.DpMsClientGigePmPhyFec200Gige.bip_8_error = 15;

			gigePm.DpMsClientGigePmPhyFec400Gige.fec_corrected = 3000;
			gigePm.DpMsClientGigePmPhyFec400Gige.fec_un_corrected = 12;
			gigePm.DpMsClientGigePmPhyFec400Gige.bit_error = 23;
			gigePm.DpMsClientGigePmPhyFec400Gige.invalid_transcode_block = 56;
			gigePm.DpMsClientGigePmPhyFec400Gige.bip_8_error = 47;
#endif

			DcoGigeClientAdapter::mGigePms.UpdatePmData(gigePm, gigeId); //pmdata map key is 1 based
		}

	    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	    dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoGigeClientAdapter::mGigePms), &DcoGigeClientAdapter::mGigePms);
	}
}


//only used for cli
bool DcoGigeClientAdapter::gigePmInfo(ostream& out, int portId)
{
	if (mGigePms.HasPmData(portId) == false)
	{
		out << " no pm cache data for portId: " << portId << endl;
		return false;
	}

	auto gigePm = mGigePms.GetPmData(portId);
	if (gigePm.gigeId != portId)
	{
		out << " not matching.. pm cache data gigeId is: " << gigePm.gigeId << endl;
		return false;
	}
	auto macTx = gigePm.DpMsClientGigePmMacTx;
	auto macRx = gigePm.DpMsClientGigePmMacRx;
	auto pcsTx = gigePm.DpMsClientGigePmPcsTx;
	auto pcsRx = gigePm.DpMsClientGigePmPcsRx;
	auto phyFecRx = gigePm.DpMsClientGigePmPhyFecRx;

	out << " Gige aid: " << gigePm.aid << ", id: " << gigePm.gigeId << ", Pm Info: " << endl;

	out << " mac tx packets: " << macTx.packets << endl;
	out << " mac tx error_packets: " << macTx.error_packets    << endl;
	out << " mac tx ok_packets: " << macTx.ok_packets       << endl;
	out << " mac tx octets: " << macTx.octets << endl;
	out << " mac tx error_octets: " << macTx.error_octets     << endl;
	out << " mac tx jabber_frames: " << macTx.jabber_frames    << endl;
	out << " mac tx fragmented_frames: " << macTx.fragmented_frames << endl;
	out << " mac tx crc_aligned_error: " << macTx.crc_aligned_error << endl;
	out << " mac tx under_sized: " << macTx.under_sized    << endl;
	out << " mac tx over_sized: " << macTx.over_sized       << endl;
	out << " mac tx broadcast_frames: " << macTx.broadcast_frames  << endl;
	out << " mac tx multicast_frames: " << macTx.multicast_frames     << endl;
	out << " mac tx unicast_frames: " << macTx.unicast_frames    << endl;
	out << " mac tx pause_frames: " << macTx.pause_frames << endl;
	out << " mac tx bpdu_frames: " << macTx.bpdu_frames << endl;
	out << " mac tx lacp_frames: " << macTx.lacp_frames    << endl;
	out << " mac tx oam_frames: " << macTx.oam_frames       << endl;
	out << " mac tx fcs_error: " << macTx.fcs_error  << endl;
	out << " mac tx vlan_ok: " << macTx.vlan_ok    << endl;
	out << " mac tx size_64: " << macTx.size_64 << endl;
	out << " mac tx size_65_to_127: " << macTx.size_65_to_127    << endl;
	out << " mac tx size_128_to_255: " << macTx.size_128_to_255       << endl;
	out << " mac tx size_256_to_511: " << macTx.size_256_to_511  << endl;
	out << " mac tx size_512_to_1023: " << macTx.size_512_to_1023     << endl;
	out << " mac tx size_1024_to_1518: " << macTx.size_1024_to_1518    << endl;
	out << " mac tx jumbo_frames: " << macTx.jumbo_frames << endl;
	out << " mac tx jumbo_to_extreme_jumbo_frames: " << macTx.jumbo_to_extreme_jumbo_frames << endl;

	out << " mac rx packets: " << macRx.packets << endl;
	out << " mac rx error_packets: " << macRx.error_packets    << endl;
	out << " mac rx ok_packets:  " << macRx.ok_packets       << endl;
	out << " mac rx octets: " << macRx.octets << endl;
	out << " mac rx error_octets: " << macRx.error_octets     << endl;
	out << " mac rx jabber_frames: " << macRx.jabber_frames    << endl;
	out << " mac rx fragmented_frames: " << macRx.fragmented_frames << endl;
	out << " mac rx crc_aligned_error: " << macRx.crc_aligned_error << endl;
	out << " mac rx under_sized: " << macRx.under_sized    << endl;
	out << " mac rx over_sized: " << macRx.over_sized       << endl;
	out << " mac rx broadcast_frames: " << macRx.broadcast_frames  << endl;
	out << " mac rx multicast_frames: " << macRx.multicast_frames     << endl;
	out << " mac rx unicast_frames: " << macRx.unicast_frames    << endl;
	out << " mac rx pause_frames: " << macRx.pause_frames << endl;
	out << " mac rx bpdu_frames: " << macRx.bpdu_frames << endl;
	out << " mac rx lacp_frames: " << macRx.lacp_frames    << endl;
	out << " mac rx oam_frames: " << macRx.oam_frames       << endl;
	out << " mac rx fcs_error: " << macRx.fcs_error  << endl;
	out << " mac rx vlan_ok: " << macRx.vlan_ok    << endl;
	out << " mac rx size_64: " << macRx.size_64 << endl;
	out << " mac rx size_65_to_127: " << macRx.size_65_to_127    << endl;
	out << " mac rx size_128_to_255: " << macRx.size_128_to_255       << endl;
	out << " mac rx size_256_to_511: " << macRx.size_256_to_511  << endl;
	out << " mac rx size_512_to_1023: " << macRx.size_512_to_1023     << endl;
	out << " mac rx size_1024_to_1518: " << macRx.size_1024_to_1518    << endl;
	out << " mac rx jumbo_frames: " << macRx.jumbo_frames << endl;
	out << " mac rx jumbo_to_extreme_jumbo_frames: " << macRx.jumbo_to_extreme_jumbo_frames << endl;

	out << " pcs tx control_block: " << pcsTx.control_block << endl;
	out << " pcs tx errored_blocks : " << pcsTx.errored_blocks << endl;
	out << " pcs tx idle_error: " << pcsTx.idle_error       << endl;
	out << " pcs tx bip_total: " << pcsTx.bip_total  << endl;
	out << " pcs tx test_pattern_error: " << pcsTx.test_pattern_error     << endl;

	out << " pcs rx control_block: " << pcsRx.control_block << endl;
	out << " pcs rx errored_blocks : " << pcsRx.errored_blocks << endl;
	out << " pcs rx idle_error: " << pcsRx.idle_error       << endl;
	out << " pcs rx bip_total: " << pcsRx.bip_total  << endl;
	out << " pcs rx test_pattern_error: " << pcsRx.test_pattern_error     << endl;

	out << " phyfec rx fec_corrected: " << phyFecRx.fec_corrected << endl;
	out << " phyfec rx fec_un_corrected: " << phyFecRx.fec_un_corrected << endl;
	out << " phyfec rx bit_error: " << phyFecRx.bit_error << endl;
	out << " phyfec rx pre_fec_ber: " << phyFecRx.pre_fec_ber << endl;
	out << " phyfec rx fec_symbol_error: " << phyFecRx.fec_symbol_error << endl;
	out << " phyfec rx fec_symbol_error_rate: " << phyFecRx.fec_symbol_error_rate << endl;
	out << " phyfec rx invalid_transcode_block: " << phyFecRx.invalid_transcode_block << endl;
	out << " phyfec rx bip_8_error: " << phyFecRx.bip_8_error << endl;
        out << "mGigePmSubEnable: " << EnumTranslate::toString(mGigePmSubEnable) << endl;

        std::time_t last_c = std::chrono::system_clock::to_time_t(gigePm.updateTime);
        std::string ts(ctime(&last_c));
        ts.erase(std::remove(ts.begin(), ts.end(), '\n'), ts.end());
        out << "Update Time: " << ts << endl;
        out << "Update Count: " << gigePm.updateCount << endl;

	return true;
}

//only used for cli
bool DcoGigeClientAdapter::gigePmAccumInfo(ostream& out, int portId)
{
    if (mGigePms.HasPmData(portId) == false)
    {
        out << " no pm cache data for portId: " << portId << endl;
        return false;
    }

    auto gigePmAccum = mGigePms.GetPmDataAccum(portId);
    if (gigePmAccum.gigeId != portId)
    {
        out << " not matching.. pm accumulated cache data gigeId is: " << gigePmAccum.gigeId << endl;
        return false;
    }

	auto macTx = gigePmAccum.DpMsClientGigePmMacTx;
	auto macRx = gigePmAccum.DpMsClientGigePmMacRx;
	auto pcsTx = gigePmAccum.DpMsClientGigePmPcsTx;
	auto pcsRx = gigePmAccum.DpMsClientGigePmPcsRx;
	auto phyFecRx = gigePmAccum.DpMsClientGigePmPhyFecRx;

	out << " Gige aid: " << gigePmAccum.aid << ", id: " << gigePmAccum.gigeId << ", Pm Info: " << endl;
        out << " Accumulation Mode: " << (true == gigePmAccum.accumEnable ? "enabled" : "disabled") << endl;

	out << " mac tx packets: " << macTx.packets << endl;
	out << " mac tx error_packets: " << macTx.error_packets    << endl;
	out << " mac tx ok_packets: " << macTx.ok_packets       << endl;
	out << " mac tx octets: " << macTx.octets << endl;
	out << " mac tx error_octets: " << macTx.error_octets     << endl;
	out << " mac tx jabber_frames: " << macTx.jabber_frames    << endl;
	out << " mac tx fragmented_frames: " << macTx.fragmented_frames << endl;
	out << " mac tx crc_aligned_error: " << macTx.crc_aligned_error << endl;
	out << " mac tx under_sized: " << macTx.under_sized    << endl;
	out << " mac tx over_sized: " << macTx.over_sized       << endl;
	out << " mac tx broadcast_frames: " << macTx.broadcast_frames  << endl;
	out << " mac tx multicast_frames: " << macTx.multicast_frames     << endl;
	out << " mac tx unicast_frames: " << macTx.unicast_frames    << endl;
	out << " mac tx pause_frames: " << macTx.pause_frames << endl;
	out << " mac tx bpdu_frames: " << macTx.bpdu_frames << endl;
	out << " mac tx lacp_frames: " << macTx.lacp_frames    << endl;
	out << " mac tx oam_frames: " << macTx.oam_frames       << endl;
	out << " mac tx fcs_error: " << macTx.fcs_error  << endl;
	out << " mac tx vlan_ok: " << macTx.vlan_ok    << endl;
	out << " mac tx size_64: " << macTx.size_64 << endl;
	out << " mac tx size_65_to_127: " << macTx.size_65_to_127    << endl;
	out << " mac tx size_128_to_255: " << macTx.size_128_to_255       << endl;
	out << " mac tx size_256_to_511: " << macTx.size_256_to_511  << endl;
	out << " mac tx size_512_to_1023: " << macTx.size_512_to_1023     << endl;
	out << " mac tx size_1024_to_1518: " << macTx.size_1024_to_1518    << endl;
	out << " mac tx jumbo_frames: " << macTx.jumbo_frames << endl;
	out << " mac tx jumbo_to_extreme_jumbo_frames: " << macTx.jumbo_to_extreme_jumbo_frames << endl;

	out << " mac rx packets: " << macRx.packets << endl;
	out << " mac rx error_packets: " << macRx.error_packets    << endl;
	out << " mac rx ok_packets:  " << macRx.ok_packets       << endl;
	out << " mac rx octets: " << macRx.octets << endl;
	out << " mac rx error_octets: " << macRx.error_octets     << endl;
	out << " mac rx jabber_frames: " << macRx.jabber_frames    << endl;
	out << " mac rx fragmented_frames: " << macRx.fragmented_frames << endl;
	out << " mac rx crc_aligned_error: " << macRx.crc_aligned_error << endl;
	out << " mac rx under_sized: " << macRx.under_sized    << endl;
	out << " mac rx over_sized: " << macRx.over_sized       << endl;
	out << " mac rx broadcast_frames: " << macRx.broadcast_frames  << endl;
	out << " mac rx multicast_frames: " << macRx.multicast_frames     << endl;
	out << " mac rx unicast_frames: " << macRx.unicast_frames    << endl;
	out << " mac rx pause_frames: " << macRx.pause_frames << endl;
	out << " mac rx bpdu_frames: " << macRx.bpdu_frames << endl;
	out << " mac rx lacp_frames: " << macRx.lacp_frames    << endl;
	out << " mac rx oam_frames: " << macRx.oam_frames       << endl;
	out << " mac rx fcs_error: " << macRx.fcs_error  << endl;
	out << " mac rx vlan_ok: " << macRx.vlan_ok    << endl;
	out << " mac rx size_64: " << macRx.size_64 << endl;
	out << " mac rx size_65_to_127: " << macRx.size_65_to_127    << endl;
	out << " mac rx size_128_to_255: " << macRx.size_128_to_255       << endl;
	out << " mac rx size_256_to_511: " << macRx.size_256_to_511  << endl;
	out << " mac rx size_512_to_1023: " << macRx.size_512_to_1023     << endl;
	out << " mac rx size_1024_to_1518: " << macRx.size_1024_to_1518    << endl;
	out << " mac rx jumbo_frames: " << macRx.jumbo_frames << endl;
	out << " mac rx jumbo_to_extreme_jumbo_frames: " << macRx.jumbo_to_extreme_jumbo_frames << endl;

	out << " pcs tx control_block: " << pcsTx.control_block << endl;
	out << " pcs tx errored_blocks: " << pcsTx.errored_blocks << endl;
	out << " pcs tx idle_error: " << pcsTx.idle_error       << endl;
	out << " pcs tx bip_total: " << pcsTx.bip_total  << endl;
	out << " pcs tx test_pattern_error: " << pcsTx.test_pattern_error     << endl;

	out << " pcs rx control_block: " << pcsRx.control_block << endl;
	out << " pcs rx errored_blocks: " << pcsRx.errored_blocks << endl;
	out << " pcs rx idle_error: " << pcsRx.idle_error       << endl;
	out << " pcs rx bip_total: " << pcsRx.bip_total  << endl;
	out << " pcs rx test_pattern_error: " << pcsRx.test_pattern_error     << endl;

	out << " phyfec rx fec_corrected: " << phyFecRx.fec_corrected << endl;
	out << " phyfec rx fec_un_corrected: " << phyFecRx.fec_un_corrected << endl;
	out << " phyfec rx bit_error: " << phyFecRx.bit_error << endl;
	out << " phyfec rx pre_fec_ber: " << phyFecRx.pre_fec_ber << endl;
	out << " phyfec rx fec_symbol_error: " << phyFecRx.fec_symbol_error << endl;
	out << " phyfec rx fec_symbol_error_rate: " << phyFecRx.fec_symbol_error_rate << endl;
	out << " phyfec rx invalid_transcode_block: " << phyFecRx.invalid_transcode_block << endl;
	out << " phyfec rx bip_8_error: " << phyFecRx.bip_8_error << endl;

    return true;
}

//only used for cli
bool DcoGigeClientAdapter::gigePmAccumClear(ostream& out, int portId)
{
    mGigePms.ClearPmDataAccum(portId);
    return true;
}

bool DcoGigeClientAdapter::gigePmAccumEnable(ostream& out, int portId, bool enable)
{
    mGigePms.SetAccumEnable(portId, enable);
    return true;
}

//spanTime for each dpms gige pm collection start and end and execution time
void DcoGigeClientAdapter::gigePmTimeInfo(ostream& out)
{
	out << "gige pm time info in last 20 seconds: " << endl;
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

bool DcoGigeClientAdapter::updateNotifyCache()
{
    gigeClient gige;
    if (setKey(&gige, 1) == false)
    {
        return false;
    }
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gige);
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

    INFN_LOG(SeverityLevel::info) << __func__ << " gige size " << gige.client_gige_size() << endl;

    for (int idx = 0; idx < gige.client_gige_size(); idx++)
    {
        int portId = 1;
        if (gige.client_gige(idx).client_gige().state().has_client_port_id())
        {
            portId = gige.client_gige(idx).client_gige().state().client_port_id().value();
            INFN_LOG(SeverityLevel::info) << " gige has portId: " << portId;
        }
        if (gige.client_gige(idx).client_gige().state().has_facility_fault())
        {
            //if gige fault is already been updated via notification, skip the retrieval
            if ((mGigeFault[portId - 1].isNotified == false))
            {
                mGigeFault[portId - 1].faultBitmap = gige.client_gige(idx).client_gige().state().facility_fault().value();
                INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve gige fault " << portId << " with bitmap: 0x" <<  hex << mGigeFault[portId - 1].faultBitmap << dec << endl;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve gige fault " << portId << " with fault notify flag:" <<  mGigeFault[portId - 1].isNotified << endl;
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no gige fault in state, portId: " << portId;
        }
    }
    return status;
}

// ====== DcoClientAdapter private =========

int DcoGigeClientAdapter::aidToPortId (string aid)
{
    INFN_LOG(SeverityLevel::debug) << __func__ << " run... AID: "  << aid << endl;
    if (aid == NULL)
        return -1;
    // AID format: 1-4-T1
    string sId;
    string aidPort = "-T";
    std::size_t pos = aid.find(aidPort);
    if (pos != std::string::npos)
    {
        sId = aid.substr(pos + aidPort.length());
        std::size_t temp = sId.find(".");
        if( temp != std::string::npos)
        {
            int parent = stoi(sId.substr(0,temp), nullptr);
            int child = stoi(sId.substr(temp+1));


            if (parent == 1)
            {

                return DataPlane::portLow0[child-1];

            }

            else if (parent == 8)
            {

                return DataPlane::portHigh0[child-1];

            }
            else if (parent == 9)
            {

                return DataPlane::portLow1[child-1];

            }
            else if (parent == 16)
            {

                return DataPlane::portHigh1[child-1];

            }
        }
    }
    else
    {
        string cliPort = "Port ";
        pos = aid.find(cliPort);
        if (pos == std::string::npos)
            return -1;
        sId = aid.substr(cliPort.length());
    }

    return(stoi(sId,nullptr));
}


bool DcoGigeClientAdapter::setKey (gigeClient *gige, int portId)
{
    bool status = true;
	if (gige == NULL)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Fail *gige = NULL... "  << endl;
        return false;
    }

    ClientsGige_ClientGigeKey *cKey;

    // Always use the first object in the ClientGige list, since we
    // create new gige object every time we send.
    cKey = gige->add_client_gige();
    cKey->set_client_port_id(portId);
    cKey->mutable_client_gige()->mutable_config()->mutable_client_port_id()->set_value(portId);

    return status;
}

const clientPortEntry *DcoGigeClientAdapter::getFpPortEntry( int portId, PortRate rate)
{
    const clientPortEntry *pPortMap = &mChm6_PortMap[0];
    while ( pPortMap->fpPort != 0 )
    {
        if ((pPortMap->fpPort == portId) && (pPortMap->pRate == rate))
        {
            return pPortMap;
        }
        ++pPortMap;
    }
    INFN_LOG(SeverityLevel::error) << __func__ << " cannot find portMapEntry ... "  << portId << endl;
    return NULL;
}

//
// Fetch Gige containter object and find the object that match with portId in
// list
//
bool DcoGigeClientAdapter::getGigeObj( gigeClient *gige, int portId, int *idx)
{
    // Save the PortId from setKey
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(gige);
    INFN_LOG(SeverityLevel::debug) << __func__ << " \n\n ****Read ... \n"  << endl;
    bool status = true;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        gige = static_cast<gigeClient*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    // Search for the correct PortId.
    bool found = false;

    INFN_LOG(SeverityLevel::debug) << __func__ << " gige size " << gige->client_gige_size() << endl;

    for (*idx = 0; *idx < gige->client_gige_size(); (*idx)++)
    {
        INFN_LOG(SeverityLevel::debug) << __func__ << " gige id " << gige->client_gige(*idx).client_port_id() << endl;
        if (gige->client_gige(*idx).client_port_id() == portId )
        {
            found = true;
            break;
        }
    }
    if (found == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " cannot find PortId in DCO  FAIL... PortId: " <<  portId << endl;
        return false;
    }
    return status;
}

void DcoGigeClientAdapter::setSimModeEn(bool enable)
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

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

bool DcoGigeClientAdapter::getGigeFaultCapa()
{
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = false;
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    DcoClientGigeFault fault;
    DcoClientGigeFault *pFault;

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&fault);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        pFault = static_cast<DcoClientGigeFault*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }
    mFaultCapa.clear();

    for ( int i = 0; i < pFault->capabilities().facility_faults_size(); i++ )
    {
        FaultCapability fault;
        DcoClientGigeFault_Capabilities_FacilityFaultsKey key;

        key = pFault->capabilities().facility_faults(i);

        fault.faultKey = key.fault();
        fault.serviceAffecting = false;
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
        using severity = dcoyang::infinera_dco_client_gige_fault::DcoClientGigeFault_Capabilities_FacilityFaults;
        switch( key.facility_faults().severity())
        {
            case severity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case severity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case severity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_client_gige_fault::DcoClientGigeFault_Capabilities_FacilityFaults;
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

        using loc = dcoyang::infinera_dco_client_gige_fault::DcoClientGigeFault_Capabilities_FacilityFaults;
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

        mFaultCapa.push_back(fault);
    }

    return true;
}

void DcoGigeClientAdapter::clearFltNotifyFlag()
{
    INFN_LOG(SeverityLevel::info) << __func__ << ", clear notify flag" << endl;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        mGigeFault[i].isNotified = false;
    }
}

bool DcoGigeClientAdapter::setTdaHoldOff(string aid, bool enable, bool updateDco, MaintenanceSignal mode)
{
    int portId = aidToPortId(aid);
    if ((portId < 1) || (portId > MAX_CLIENTS))
    {
        INFN_LOG(SeverityLevel::error) << __func__ << ", failed to set Tom HoldOff mode for port " << portId;
        return false;
    }

    DataPlane::Direction dir = DIRECTION_TX;

    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    mTdaHoldOff[portId] = enable;

    if (updateDco)
    {
        setGigeAutoMS(aid, dir, mode);
    }

    return true;
}

std::string DcoGigeClientAdapter::toString(const DataPlane::Direction dir) const
{
    std::string retString;
    switch (dir)
    {
        case DIRECTION_UNSET: retString = "UNSET";   break;
        case DIRECTION_TX:    retString = "TX";      break;
        case DIRECTION_RX:    retString = "RX";      break;
        default:              retString = "UNKNOWN"; break;
    }
    return retString;
}
} /* DpAdapter */
