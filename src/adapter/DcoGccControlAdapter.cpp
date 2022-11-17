
/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <string.h>	//strncpy
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>	//ifreq
#include <unistd.h>	//close

#include <google/protobuf/text_format.h>
#include "DcoGccControlAdapter.h"
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
using namespace dcoyang::infinera_dco_gcc_control;
using namespace dcoyang::infinera_dco_gcc_control_fault;

using gccControl = dcoyang::infinera_dco_gcc_control::DcoGccControl;
using gccControlCfg = dcoyang::infinera_dco_gcc_control::DcoGccControl_Config;
using gccControlState = dcoyang::infinera_dco_gcc_control::DcoGccControl_State;

namespace DpAdapter {

DpMsGCCPm DcoGccControlAdapter::mGccPms;

high_resolution_clock::time_point DcoGccControlAdapter::mStart;
high_resolution_clock::time_point DcoGccControlAdapter::mEnd;
int64_t DcoGccControlAdapter::mStartSpan;
int64_t DcoGccControlAdapter::mEndSpan;
int64_t DcoGccControlAdapter::mDelta;
std::deque<int64_t> DcoGccControlAdapter::mStartSpanArray(spanTime);
std::deque<int64_t> DcoGccControlAdapter::mEndSpanArray(spanTime);
std::deque<int64_t> DcoGccControlAdapter::mDeltaArray(spanTime);


DcoGccControlAdapter::DcoGccControlAdapter()
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
}

DcoGccControlAdapter::~DcoGccControlAdapter()
{

}

bool DcoGccControlAdapter::initializeCcVlan()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    bool status = true;

	setLldpVlan();

	subGccPm();

    // Get GCC Fault Capabitites
    // status = getControlChannelFaultCapa();

    return status;
}

bool DcoGccControlAdapter::createCcVlan(ControlChannelVlanIdx vIdx, ControlChannelVlanCommon *cfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    if (0 == cfg->rxVlanTag)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << "Zero RxVlan Tag for Idx: "<< vIdx << '\n';
        return false;
    }

    if (true == cfg->deleteTxTags)
    {
        INFN_LOG(SeverityLevel::info) << "Received request to delete Tx tags";
        bool status = deleteCcVlanTxVlan(vIdx);
        return status;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Create Vlan Idx: " << vIdx << endl;

    gccControl gcc;

    if (setKey(&gcc, vIdx) == false)
    {
        return false;
    }

    gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_enable()->set_value(cfg->enable);
    if (cfg->txVlanReTag != 0)
    {
        gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_tx_vlan_tag()->set_value(cfg->txVlanTag);
        gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_tx_vlan_retag()->set_value(cfg->txVlanReTag);
    }
    gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_rx_vlan_tag()->set_value(cfg->rxVlanTag);
    gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_rx_vlan_retag()->set_value(cfg->rxVlanReTag);

    switch(cfg->type)
    {
        case CONTROLCHANNELTYPE_OFEC:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_gcc_type(DcoGccControl_Config_VlanTable::GCCTYPE_OFEC);
            break;
        case CONTROLCHANNELTYPE_IFEC:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_gcc_type(DcoGccControl_Config_VlanTable::GCCTYPE_IFEC);
            break;
        case CONTROLCHANNELTYPE_NB_DISCOVERY:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_gcc_type(DcoGccControl_Config_VlanTable::GCCTYPE_NB_DISCOVERY);
            break;
        case CONTROLCHANNELTYPE_MANAGEMENT:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_gcc_type(DcoGccControl_Config_VlanTable::GCCTYPE_MANAGEMENT);
            break;
        case CONTROLCHANNELTYPE_BACK_CHANNEL:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_gcc_type(DcoGccControl_Config_VlanTable::GCCTYPE_BACK_CHANNEL);
            break;
        case CONTROLCHANNELTYPE_IKE:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_gcc_type(DcoGccControl_Config_VlanTable::GCCTYPE_IKE);
            break;

        // Currently DCO is only care about BACK_CHANNEL GCC Type.
        // Everything else can be default OFEC.
        // Upper layer app code does not have to change to set other GCC type.
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << " Unknown type. set Default:  "  << cfg->type << endl;
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_gcc_type(DcoGccControl_Config_VlanTable::GCCTYPE_OFEC);
            break;
    }

    switch(cfg->carrier)
    {
        case CONTROLCHANNELCARRIER_1:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_carrier(DcoGccControl_Config_VlanTable::CARRIER_WAVE1);
            break;
        case CONTROLCHANNELCARRIER_2:
            gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->set_carrier(DcoGccControl_Config_VlanTable::CARRIER_WAVE2);
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << " Unknown Carrier. set FAIL:  "  << cfg->carrier << endl;
            return false;
            break;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gcc);
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
    return status;
}

bool DcoGccControlAdapter::enableCcVlan(ControlChannelVlanIdx vIdx, bool enable)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Vlan Idx: " << vIdx << "enable: " << (int) enable << endl;

	gccControl gcc;

    if (setKey(&gcc, vIdx) == false)
    {
        return false;
    }

    gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_enable()->set_value(enable);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gcc);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoGccControlAdapter::deleteCcVlan(ControlChannelVlanIdx vIdx)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Delete Vlan Idx: " << vIdx << endl;

	gccControl gcc;

    if (setKey(&gcc, vIdx) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gcc);
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
    return status;
}

// Detele all Tx Vlan configuration
bool DcoGccControlAdapter::deleteCcVlanTxVlan(ControlChannelVlanIdx vIdx)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    if ( deleteCcVlanTxVlanTag(vIdx) == false )
    {
        status = false;
    }
    if ( deleteCcVlanTxVlanReTag(vIdx) == false )
    {
        status = false;
    }
    return status;
}

// Delete TxVlanTag leaf from Vlan table
bool DcoGccControlAdapter::deleteCcVlanTxVlanTag(ControlChannelVlanIdx vIdx)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Delete Vlan Idx: " << vIdx << endl;

    gccControl gcc;
    gcc.mutable_config()->add_vlan_table()->set_application_id(vIdx);
    gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_tx_vlan_tag();
    // gcc.mutable_config()->mutable_vlan_table(tIdx)->mutable_vlan_table()->mutable_tx_vlan_tag();

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gcc);
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
    return status;
}
// Delete TxVlanReTag leaf from Vlan table
bool DcoGccControlAdapter::deleteCcVlanTxVlanReTag(ControlChannelVlanIdx vIdx)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Delete Vlan Idx: " << vIdx << endl;

    gccControl gcc;
    gcc.mutable_config()->add_vlan_table()->set_application_id(vIdx);
    gcc.mutable_config()->mutable_vlan_table(0)->mutable_vlan_table()->mutable_tx_vlan_retag();
    // gcc.mutable_config()->mutable_vlan_table(tIdx)->mutable_vlan_table()->mutable_tx_vlan_retag();

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gcc);
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
    return status;
}

//
//  Get/Read methods
//

void DcoGccControlAdapter::dumpAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    gccControl gccMsg;

    ControlChannelVlanIdx vIdx = CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1;
    if (setKey(&gccMsg, vIdx) == false)
    {
        out << "Failed to set key" << endl;

        return;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gccMsg);

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

    if ( gccMsg.has_config() )
    {
        for (int idx = 0; idx < gccMsg.config().vlan_table_size(); idx++)
        {
            INFN_LOG(SeverityLevel::debug) << __func__ << " Vlan id " << gccMsg.config().vlan_table(idx).application_id() << endl;

            ControlChannelVlanCommon cmn;

            translateConfig(gccMsg.config().vlan_table(idx).vlan_table(), cmn);

            out << "*** Gcc Config for application_id: " << gccMsg.config().vlan_table(idx).application_id() << " idx: " << idx << endl;

            dumpConfig(out, cmn);
        }
    }
    else
    {
        out << "*** Gcc config is not present " << endl;
    }

    if ( gccMsg.has_state() )
    {
        for (int idx = 0; idx < gccMsg.state().vlan_table_size(); idx++)
        {
            INFN_LOG(SeverityLevel::debug) << __func__ << " Vlan id " << gccMsg.state().vlan_table(idx).application_id() << endl;

            ControlChannelVlanStatus stat;

            translateStatus(gccMsg.state(), idx, stat);

            out << "*** Gcc Status for application_id: " << gccMsg.state().vlan_table(idx).application_id() << " idx: " << idx << endl;

            dumpStatus(out, stat);
        }
    }
    else
    {
        out << "*** Gcc state is not present " << endl;
    }
}

bool DcoGccControlAdapter::ccVlanConfigInfo(ostream& out, ControlChannelVlanIdx vIdx)
{
    ControlChannelVlanCommon cfg;
    memset(&cfg, 0, sizeof(cfg));
    bool status = getCcVlanConfig(vIdx, &cfg);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }
    out << " CC VLAN Config Info: " << vIdx  << endl;

    dumpConfig(out, cfg);

    return status;
}

void DcoGccControlAdapter::dumpConfig(std::ostream& out, ControlChannelVlanCommon cfg)
{
    out << " enable: " << (int) cfg.enable << endl;
    out << " TX VLAN Tag: " << cfg.txVlanTag << endl;
    out << " TX VLAN Retag: " << cfg.txVlanReTag << endl;
    out << " RX VLAN Tag: " << cfg.rxVlanTag << endl;
    out << " RX VLAN Retag: " << cfg.rxVlanReTag << endl;
    out << " GCC Type: " << cfg.type << endl;
    out << " Carrier : " << cfg.carrier << endl;
}

bool DcoGccControlAdapter::ccVlanStatusInfo(ostream& out, ControlChannelVlanIdx vIdx)
{
    ControlChannelVlanStatus stat;
    memset(&stat, 0, sizeof(stat));
    bool status = getCcVlanStatus(vIdx, &stat);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }
    out << " CC VLAN Status Info: " << vIdx  << endl;

    dumpStatus(out, stat);

    return status;
}

void DcoGccControlAdapter::dumpStatus(std::ostream& out, ControlChannelVlanStatus stat)
{
    out << " enable: " << (int) stat.cc.enable << endl;
    out << " TX VLAN Tag: " << stat.cc.txVlanTag << endl;
    out << " TX VLAN Retag: " << stat.cc.txVlanReTag << endl;
    out << " RX VLAN Tag: " << stat.cc.rxVlanTag << endl;
    out << " RX VLAN Retag: " << stat.cc.rxVlanReTag << endl;
    out << " GCC Type: " << stat.cc.type << endl;
    out << " Carrier : " << stat.cc.carrier << endl;
    out << " FPGA Version : " << stat.fpgaVersion << endl;
    out << " FAC fault : 0x" << hex <<  stat.facFault  << dec << endl;
}

bool DcoGccControlAdapter::getCcVlanConfig(ControlChannelVlanIdx vIdx, ControlChannelVlanCommon *cfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Config Vlan Idx: " << vIdx << endl;

	gccControl gcc;

    if (setKey(&gcc, vIdx) == false)
    {
        return false;
    }

    bool configObj = true;
    int tIdx = 0;  // Vlan Table index for Config Object in return list
    status = getCcVlanObj(&gcc, vIdx, &tIdx, configObj);
    if (status == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Get Dco Gcc object FAIL Vlan Idx: " << vIdx << '\n';
        return false;
    }

    translateConfig(gcc.config().vlan_table(tIdx).vlan_table(), *cfg);

    return status;
}

void DcoGccControlAdapter::translateConfig(const GccVlanConfig& dcoCfg, ControlChannelVlanCommon& cfg)
{
    if (dcoCfg.has_enable())
    {
        cfg.enable =dcoCfg.enable().value();
    }
    if (dcoCfg.has_tx_vlan_tag())
    {
        cfg.txVlanTag =dcoCfg.tx_vlan_tag().value();
    }
    if (dcoCfg.has_tx_vlan_retag())
    {
        cfg.txVlanReTag =dcoCfg.tx_vlan_retag().value();
    }
    if (dcoCfg.has_rx_vlan_tag())
    {
        cfg.rxVlanTag =dcoCfg.rx_vlan_tag().value();
    }
    if (dcoCfg.has_rx_vlan_retag())
    {
        cfg.rxVlanReTag =dcoCfg.rx_vlan_retag().value();
    }

    switch(dcoCfg.gcc_type())
    {
        case DcoGccControl_Config_VlanTable::GCCTYPE_OFEC:
            cfg.type = CONTROLCHANNELTYPE_OFEC;
            break;
        case DcoGccControl_Config_VlanTable::GCCTYPE_IFEC:
            cfg.type = CONTROLCHANNELTYPE_IFEC;
            break;
        case DcoGccControl_Config_VlanTable::GCCTYPE_NB_DISCOVERY:
            cfg.type = CONTROLCHANNELTYPE_NB_DISCOVERY;
            break;
        case DcoGccControl_Config_VlanTable::GCCTYPE_MANAGEMENT:
            cfg.type = CONTROLCHANNELTYPE_MANAGEMENT;
            break;
        case DcoGccControl_Config_VlanTable::GCCTYPE_BACK_CHANNEL:
            cfg.type = CONTROLCHANNELTYPE_BACK_CHANNEL;
            break;
        case DcoGccControl_Config_VlanTable::GCCTYPE_IKE:
            cfg.type = CONTROLCHANNELTYPE_IKE;
            break;
        default:
            cfg.type = CONTROLCHANNELTYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::error) << __func__ << " Unknown GCCTYPE : " << dcoCfg.gcc_type() << endl;
            break;
    }

    switch(dcoCfg.carrier())
    {
        case DcoGccControl_Config_VlanTable::CARRIER_WAVE1:
            cfg.carrier = CONTROLCHANNELCARRIER_1;
            break;
        case DcoGccControl_Config_VlanTable::CARRIER_WAVE2:
            cfg.carrier = CONTROLCHANNELCARRIER_2;
            break;
        default:
            cfg.carrier = CONTROLCHANNELCARRIER_UNSPECIFIED;
            INFN_LOG(SeverityLevel::error) << __func__ << " Unknown CARRIER : " << dcoCfg.carrier() << endl;
            break;
    }
}

bool DcoGccControlAdapter::getCcVlanStatus(ControlChannelVlanIdx vIdx, ControlChannelVlanStatus *stat)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Vlan Idx: " << vIdx << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Status Vlan Idx: " << vIdx << endl;

	gccControl gcc;

    if (setKey(&gcc, vIdx) == false)
    {
        return false;
    }

    bool configObj = false;  // Get state object
    int tIdx = 0;  // Vlan Table index for State Object in return list
    status = getCcVlanObj(&gcc, vIdx, &tIdx, configObj);
    if (status == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Get Dco Gcc object FAIL Vlan Idx: " << vIdx << '\n';
        return false;
    }

    if ( gcc.has_state() == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " no state object Vlan Idx in DCO  FAIL... vIdx: " <<  vIdx << endl;
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << "Vlan tidx: " << tIdx << " table size: " << gcc.state().vlan_table_size() << endl;

    translateStatus(gcc.state(), tIdx, *stat);

    return status;
}

void DcoGccControlAdapter::translateStatus(const GccState& dcoStat, int idx, ControlChannelVlanStatus& stat)
{
    if (dcoStat.vlan_table_size() > idx)
    {
        if (dcoStat.vlan_table(idx).vlan_table().has_enable())
        {
            stat.cc.enable =dcoStat.vlan_table(idx).vlan_table().enable().value();
        }
        if (dcoStat.vlan_table(idx).vlan_table().has_tx_vlan_tag())
        {
            stat.cc.txVlanTag =dcoStat.vlan_table(idx).vlan_table().tx_vlan_tag().value();
        }
        if (dcoStat.vlan_table(idx).vlan_table().has_tx_vlan_retag())
        {
            stat.cc.txVlanReTag =dcoStat.vlan_table(idx).vlan_table().tx_vlan_retag().value();
        }
        if (dcoStat.vlan_table(idx).vlan_table().has_rx_vlan_tag())
        {
            stat.cc.rxVlanTag =dcoStat.vlan_table(idx).vlan_table().rx_vlan_tag().value();
        }
        if (dcoStat.vlan_table(idx).vlan_table().has_rx_vlan_retag())
        {
            stat.cc.rxVlanReTag =dcoStat.vlan_table(idx).vlan_table().rx_vlan_retag().value();
        }
        switch(dcoStat.vlan_table(idx).vlan_table().gcc_type())
        {
            case DcoGccControl_Config_VlanTable::GCCTYPE_OFEC:
                stat.cc.type = CONTROLCHANNELTYPE_OFEC;
                break;
            case DcoGccControl_Config_VlanTable::GCCTYPE_IFEC:
                stat.cc.type = CONTROLCHANNELTYPE_IFEC;
                break;
            case DcoGccControl_Config_VlanTable::GCCTYPE_NB_DISCOVERY:
                stat.cc.type = CONTROLCHANNELTYPE_NB_DISCOVERY;
                break;
            case DcoGccControl_Config_VlanTable::GCCTYPE_MANAGEMENT:
                stat.cc.type = CONTROLCHANNELTYPE_MANAGEMENT;
                break;
            case DcoGccControl_Config_VlanTable::GCCTYPE_BACK_CHANNEL:
                stat.cc.type = CONTROLCHANNELTYPE_BACK_CHANNEL;
                break;
            case DcoGccControl_Config_VlanTable::GCCTYPE_IKE:
                stat.cc.type = CONTROLCHANNELTYPE_IKE;
                break;
            default:
                stat.cc.type = CONTROLCHANNELTYPE_UNSPECIFIED;
                INFN_LOG(SeverityLevel::error) << __func__ << " Unknown GCCTYPE : " << dcoStat.vlan_table(idx).vlan_table().gcc_type() << endl;
                break;
        }

        switch(dcoStat.vlan_table(idx).vlan_table().carrier())
        {
            case DcoGccControl_Config_VlanTable::CARRIER_WAVE1:
                stat.cc.carrier = CONTROLCHANNELCARRIER_1;
                break;
            case DcoGccControl_Config_VlanTable::CARRIER_WAVE2:
                stat.cc.carrier = CONTROLCHANNELCARRIER_2;
                break;
            default:
                stat.cc.carrier = CONTROLCHANNELCARRIER_UNSPECIFIED;
                INFN_LOG(SeverityLevel::error) << __func__ << " Unknown CARRIER : " << dcoStat.vlan_table(idx).vlan_table().carrier() << endl;
            break;
        }
    }

    if (dcoStat.has_fpga_version())
    {
        stat.fpgaVersion =dcoStat.fpga_version().value();
    }
    if (dcoStat.has_facility_fault())
    {
        stat.facFault =dcoStat.facility_fault().value();
    }
}

bool DcoGccControlAdapter::getCcVlanFault(ControlChannelVlanIdx vIdx, ControlChannelFault *faults, bool force)
{
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    bool status = true;
	INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;

    return status;

}

/*
 * LLDP VLAN supports
 */

//set mac, ethtype and vlan tag for lldp vlan
bool DcoGccControlAdapter::setLldpVlan()
{
	gccControl gcc;
	uint64_t dstMac = getLldpDstMacAddr();
	INFN_LOG(SeverityLevel::info) << __func__ << ", eth2 dstMac: 0x" << std::hex << dstMac << std::endl;
	gcc.mutable_config()->mutable_dest_mac()->set_value(dstMac);
	gcc.mutable_config()->mutable_ethernet_type()->set_value(0x88B5);
	gcc.mutable_config()->mutable_vlan_tag()->set_value(81);

	bool status = true;
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gcc);
    std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
    grpc::Status gStatus = mspCrud->Create( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " set FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

//get lldp object
bool DcoGccControlAdapter::getLldp(gccControl* gcc)
{
	bool status = true;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(gcc);

    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);
	grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        gcc = static_cast<gccControl*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }
	return status;
}

//cli for lldp vlan cfg
bool DcoGccControlAdapter::getLldpVlanConfig(ostream& out)
{
	gccControl gcc;
    bool status = getLldp(&gcc);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Gcc Object  FAIL "  << endl;
        return status;
    }
	out << "dst mac : 0x" << std::hex << gcc.config().dest_mac().value() << endl;
	out << "eth type : 0x" << std::hex << gcc.config().ethernet_type().value() << endl;
	out << "vlan tag : " << std::dec << gcc.config().vlan_tag().value() << endl;
    return status;
}

//cli for lldp vlan state
bool DcoGccControlAdapter::getLldpVlanState(ostream& out)
{
	gccControl gcc;
    bool status = getLldp(&gcc);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Gcc Object  FAIL "  << endl;
        return status;
    }
	out << "src mac : 0x" << std::hex << gcc.state().source_mac().value() << endl;
	out << "dst mac : 0x" << std::hex << gcc.state().dest_mac().value() << endl;
	out << "eth type : 0x" << std::hex << gcc.state().ethernet_type().value() << endl;
	out << "vlan tag : " << std::dec << gcc.state().vlan_tag().value() << endl;

    return status;
}

//set sim mode
void DcoGccControlAdapter::setSimModeEn(bool enable)
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


/*
 *  Private functions start here
 */

//get mac address for vlan81 bridge
uint64_t DcoGccControlAdapter::getLldpDstMacAddr()
{
	int fd;
	struct ifreq ifr;
	const char *iface = "eth2";
	unsigned char *mac;
	uint64_t res = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
	ioctl(fd, SIOCGIFHWADDR, &ifr);
	close(fd);

	mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
	for (int i = 0; i < 6; i++)
	{
		res <<= 8;
		res |= (uint64_t)mac[i];
	}
	return res;
}

bool DcoGccControlAdapter::setKey (gccControl *gcc, ControlChannelVlanIdx vIdx)
{
    bool status = true;
	if (gcc == NULL)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Fail *gcc = NULL... "  << endl;
        return false;
    }

    DcoGccControl_Config_VlanTableKey     *cKey;

    // Always use the first object in the list, since we
    // create new odu object every time we send.
    cKey = gcc->mutable_config()->add_vlan_table();
    cKey->set_application_id(vIdx);

    return status;
}
//
// Fetch CCVLan containter object and find the object that match with vlanIdx in  list
bool DcoGccControlAdapter::getCcVlanObj( gccControl *gcc, ControlChannelVlanIdx vIdx, int *tIdx, bool configObj )
{

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(gcc);
    bool status = true;
    const std::lock_guard<std::recursive_mutex> lock(mCrudPtrMtx);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        gcc = static_cast<gccControl*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    if ( gcc->has_config() == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " no config object Vlan Idx in DCO  FAIL... vIdx: " <<  vIdx << endl;
        return false;
    }

    // Search for the correct vIdx.
    bool found = false;

    if ( configObj == true )
    {
        for (int idx = 0; idx < gcc->config().vlan_table_size(); idx++)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Vlan id " << gcc->config().vlan_table(idx).application_id() << endl;
            if (gcc->config().vlan_table(idx).application_id() == vIdx )
            {
                found = true;
                *tIdx = idx;
                break;
            }
        }
    }
    else
    {
        for (int idx = 0; idx < gcc->state().vlan_table_size(); idx++)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Vlan id " << gcc->state().vlan_table(idx).application_id() << endl;
            if (gcc->state().vlan_table(idx).application_id() == vIdx )
            {
                found = true;
                *tIdx = idx;
                break;
            }
        }
    }

    if (found == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " cannot find Vlan Idx in DCO  FAIL... vIdx: " <<  vIdx << endl;
        return false;
    }
    return status;
}

//subscribe gcc pm, for sim case, spawn a thread to pump data to dp mgr
void DcoGccControlAdapter::subGccPm()
{
	if (dp_env::DpEnvSingleton::Instance()->isSimEnv() && mIsGnmiSimModeEn) {
		//spawn a thread to feed up pm data for sim
		if (mSimGccPmThread.joinable() == false) { //check whether the sim pm thread is running
			INFN_LOG(SeverityLevel::info) << "create mSimGccPmThread for gcc " << endl;
			mSimGccPmThread = std::thread(&DcoGccControlAdapter::setSimPmData, this);
		}
		else {
			INFN_LOG(SeverityLevel::info) << "mSimGccPmThread is already running..." << endl;
		}
	}
	else {
		INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
		dcoGccPm gp;
	    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&gp);
		::GnmiClient::AsyncSubCallback* cb = new GccPmResponseContext(); //cleaned up in grpc client
		std::string cbId = mspCrud->Subscribe(msg, cb);
		INFN_LOG(SeverityLevel::info) << " gcc pm sub request sent to server.."  << endl;
	}
}

//set same sim pm data for all objects..
void DcoGccControlAdapter::setSimPmData()
{
    while (1)
	{
		this_thread::sleep_for(seconds(1));

        bool isSimDcoConnect = DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)DCC_ZYNQ).isSimDcoConnect();
        if (!mIsGnmiSimModeEn || !isSimDcoConnect)
        {
            continue;
        }

		for (int i = 1; i <= static_cast<int>(CONTROLCHANNELVLANIDX_MAX); i++) {
			if (mGccPms.HasPmData(i) == false) continue;
			auto gccPm = mGccPms.GetPmData(i);
			int carrierId= gccPm.carrierId;
			INFN_LOG(SeverityLevel::debug) << __func__ << " carrierId: " << carrierId << ", aid: " << gccPm.aid << ", i: " << i << endl;

			gccPm.controlpm.tx_packets = 12345;
			gccPm.controlpm.rx_packets = 45678;
			gccPm.controlpm.tx_packets_dropped = 23456;
			gccPm.controlpm.rx_packets_dropped = 78901;
			gccPm.controlpm.tx_bytes = 345678;
			gccPm.controlpm.rx_bytes = 567890;

			DcoGccControlAdapter::mGccPms.UpdatePmData(gccPm, carrierId); //pmdata map key is 1 based
		}

	    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	    dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoGccControlAdapter::mGccPms), &DcoGccControlAdapter::mGccPms);
	}
}

bool DcoGccControlAdapter::gccPmInfo(ostream& out, int carrierId)
{
	if (mGccPms.HasPmData(carrierId) == false)
	{
		out << " no pm cache data for carrierId: " << carrierId<< endl;
		return false;
	}
	auto gccPm = mGccPms.GetPmData(carrierId);
	if (gccPm.carrierId != carrierId)
	{
		out << " not matching.. pm cache gccPm carrierId is: " << gccPm.carrierId << endl;
		return false;
	}
	out << " Gcc aid: " << gccPm.aid << ", Pm Info: " << endl;
	out << " Gcc carrierId: " << gccPm.carrierId << endl;
	out << " Tx packets: " << gccPm.controlpm.tx_packets << endl;
	out << " Rx packets: " << gccPm.controlpm.rx_packets << endl;
	out << " Tx packets dropped : " << gccPm.controlpm.tx_packets_dropped << endl;
	out << " Rx packets dropped: " << gccPm.controlpm.rx_packets_dropped << endl;
	out << " Tx bytes : " << gccPm.controlpm.tx_bytes << endl;
	out << " Rx bytes: " << gccPm.controlpm.rx_bytes << endl;
	return true;
}

//only used for cli
bool DcoGccControlAdapter::gccPmAccumInfo(ostream& out, int carrierId)
{
	if (mGccPms.HasPmData(carrierId) == false)
	{
		out << " no pm cache data for carrierId: " << carrierId<< endl;
		return false;
	}
	auto gccPmAccum = mGccPms.GetPmDataAccum(carrierId);
	if (gccPmAccum.carrierId != carrierId)
	{
		out << " not matching.. pm cache data carrierId is: " << gccPmAccum.carrierId << endl;
		return false;
	}
	out << " Gcc aid: " << gccPmAccum.aid << ", Pm Info: " << endl;
	out << " Gcc carrierId: " << gccPmAccum.carrierId << endl;
	out << " Tx packets: " << gccPmAccum.controlpm.tx_packets << endl;
	out << " Rx packets: " << gccPmAccum.controlpm.rx_packets << endl;
	out << " Tx packets dropped : " << gccPmAccum.controlpm.tx_packets_dropped << endl;
	out << " Rx packets dropped: " << gccPmAccum.controlpm.rx_packets_dropped << endl;
	out << " Tx bytes : " << gccPmAccum.controlpm.tx_bytes << endl;
	out << " Rx bytes: " << gccPmAccum.controlpm.rx_bytes << endl;
	return true;
}

//only used for cli
bool DcoGccControlAdapter::gccPmAccumClear(ostream& out, int carrierId)
{
    if (mGccPms.HasPmData(carrierId) == false)
    {
        out << " no pm cache data for carrierId: " << carrierId << endl;
        return false;
    }

    auto gccPmAccum = mGccPms.GetPmDataAccum(carrierId);
    if (gccPmAccum.carrierId != carrierId)
    {
        out << " not matching.. pm accumulated cache data carrierId is: " << gccPmAccum.carrierId << endl;
        return false;
    }

    std::string aid = gccPmAccum.aid;

    memset(&gccPmAccum, 0, sizeof(gccPmAccum));
    gccPmAccum.aid = aid;
    gccPmAccum.carrierId = carrierId;

    mGccPms.UpdatePmDataAccum(gccPmAccum, carrierId);

    return true;
}

//spanTime for each dpms gcc pm collection start and end and execution time
void DcoGccControlAdapter::gccPmTimeInfo(ostream& out)
{
	out << "gcc pm time info in last 20 seconds: " << endl;
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



} /* DpAdapter */
