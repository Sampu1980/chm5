/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "GccConfigHandler.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"

#include <iostream>
#include <string>
#include <sstream>

using chm6_pcp::VlanConfig;
using chm6_pcp::VlanState;

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace ::std;


namespace DataPlane {

#define MIN_CARRIER_ID  1
#define MAX_CARRIER_ID  2


GccConfigHandler::GccConfigHandler(DpAdapter::DcoGccControlAdapter* pGccAd)
    : mpGccAd(pGccAd)
{
    INFN_LOG(SeverityLevel::info) << "Constructor";

    assert(pGccAd != NULL);
}

int GccConfigHandler::aidToCarrierId(std::string vlanAid)
{
    int vlanValue = -1;
    std::size_t found = vlanAid.find_last_of("L");
    if(found != std::string::npos)
    {
        vlanAid = vlanAid.substr(found+1,1);
        if (vlanAid == "1")
        {
            vlanValue = 1;
        }
        else if (vlanAid == "2")
        {
            vlanValue = 2;
        }
    }
    return vlanValue;
}

ConfigStatus GccConfigHandler::onCreate(VlanConfig* vlanCfg)
{
    std::string vlanAid = vlanCfg->base_config().config_id().value();
    string cfgData;
    MessageToJsonString( *vlanCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << "Vlan AID [" << vlanAid << "] GccConfig onCreate data [" << cfgData << "]";

    int carId = aidToCarrierId(vlanAid);
    if (carId < MIN_CARRIER_ID || carId > MAX_CARRIER_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid carrierId in VLAN AID " << vlanAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "VLAN AID [" << vlanAid << "] status [" << statToString(status) << "]";
        return status;
    }

    createPcpVlanConfig(vlanCfg);

    bool cfgSuccess = createCcVlanConfig(vlanCfg);

    ConfigStatus status = cfgSuccess ? ConfigStatus::SUCCESS : ConfigStatus::FAIL_CONNECT;
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "vlan create on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "VLAN AID [" << vlanAid << "] status [" << statToString(status) << "]";

    return status;
}

ConfigStatus GccConfigHandler::onModify(VlanConfig* vlanCfg)
{
    std::string vlanAid = vlanCfg->base_config().config_id().value();
    string cfgData;
    MessageToJsonString( *vlanCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << "Vlan AID [" << vlanAid << "] GccConfig onModify data [" << cfgData << "]";

    ConfigStatus status = ConfigStatus::FAIL_DATA;

    return status;
}

ConfigStatus GccConfigHandler::onDelete(VlanConfig* vlanCfg)
{
    std::string vlanAid = vlanCfg->base_config().config_id().value();
    string cfgData;
    MessageToJsonString( *vlanCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << "Vlan AID [" << vlanAid << "] GccConfig onDelete data [" << cfgData << "]";

    int carId = aidToCarrierId(vlanAid);
    if (carId < MIN_CARRIER_ID || carId > MAX_CARRIER_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid carrierId in VLAN AID " << vlanAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "VLAN AID [" << vlanAid << "] status [" << statToString(status) << "]";
        return status;
    }

    deletePcpVlanConfig(vlanCfg);

    bool cfgSuccess = deleteCcVlanConfig(vlanCfg);

    ConfigStatus status = cfgSuccess ? ConfigStatus::SUCCESS : ConfigStatus::FAIL_CONNECT;
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "vlan delete on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "VLAN AID [" << vlanAid << "] status [" << statToString(status) << "]";

    return status;
}

    // To create CC_MGMT, CC_BC vlan in pcp_ms 
    // Note: DpPeerDiscoveryMgr::CreatePcpVlan creates CC_PEERDISC in pcp_ms
void GccConfigHandler::createPcpVlanConfig(VlanConfig* vlanCfg)
{
    Chm6PeerDiscVlanState pcpVlanState;

    (pcpVlanState.mutable_base_state())->mutable_config_id()->set_value(vlanCfg->base_config().config_id().value());

    (pcpVlanState.mutable_state())->mutable_vlanid()->set_value(vlanCfg->hal().vlanid().value());

    (pcpVlanState.mutable_state())->set_apptype((PcpAppType)vlanCfg->hal().apptype());

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(pcpVlanState);
}

    // To delete CC_MGMT, CC_BC vlan in pcp_ms 
    // Note: DpPeerDiscoveryMgr::DeletePcpVlan deletes CC_PEERDISC in pcp_ms
void GccConfigHandler::deletePcpVlanConfig(VlanConfig* vlanCfg)
{
    Chm6PeerDiscVlanState pcpVlanState;

    (pcpVlanState.mutable_base_state())->mutable_config_id()->set_value(vlanCfg->base_config().config_id().value());

    (pcpVlanState.mutable_state())->mutable_vlanid()->set_value(vlanCfg->hal().vlanid().value());

    (pcpVlanState.mutable_state())->set_apptype((PcpAppType)vlanCfg->hal().apptype());

    //Calling PCP MS - update vlan state object in redis for pcp_ms to pick up
    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    (pcpVlanState.mutable_base_state())->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(pcpVlanState);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);
}

    // To create CC_MGMT, CC_BC vlan remap entry in dco fpga 
    // Note: DpPeerDiscoveryMgr::CreateVlan creates CC_PEERDISC vlan remap entry in dco fpga
bool GccConfigHandler::createCcVlanConfig(VlanConfig* vlanCfg)
{
    bool cfgSuccess = false;

    DataPlane::ControlChannelVlanIdx idx;
    DpAdapter::ControlChannelVlanCommon cmn;

    const uint32_t normalizedVlan1 = DataPlane::NORMALIZED_VLANID_OFEC_CC_MGMT;
    const uint32_t normalizedVlan2 = DataPlane::NORMALIZED_VLANID_OFEC_CC_MGMT;
    uint32_t normalizedVlan = DataPlane::NORMALIZED_VLANID_OFEC_CC_UNSPECIFIED;

    DataPlane::ControlChannelType gccType = DataPlane::CONTROLCHANNELTYPE_OFEC;

    std::string carrierAid = vlanCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << __func__ << "Create vlan called, Carrier AID: " << carrierAid;

    std::size_t found = carrierAid.find_last_of("L");
    if(found != std::string::npos)
    {
        std::string carrierId = carrierAid.substr(found+1,1);
        if (carrierId == "1")
        {
            if ( string::npos != carrierAid.find("BC") )
            {
                idx = DataPlane::CONTROLCHANNELVLANIDX_BACK_CHANNEL_CARRIER_1;
                normalizedVlan = NORMALIZED_VLANID_OFEC_CC_BC;
                gccType = CONTROLCHANNELTYPE_BACK_CHANNEL;
            }
            else
            {
                idx = DataPlane::CONTROLCHANNELVLANIDX_MGMT_CARRIER_1;
                normalizedVlan = normalizedVlan1;
            }
            cmn.carrier = DataPlane::CONTROLCHANNELCARRIER_1;
        }
        else if (carrierId == "2")
        {
            if ( string::npos != carrierAid.find("BC") )
            {
                idx = DataPlane::CONTROLCHANNELVLANIDX_BACK_CHANNEL_CARRIER_2;
                normalizedVlan = NORMALIZED_VLANID_OFEC_CC_BC;
                gccType = CONTROLCHANNELTYPE_BACK_CHANNEL;
            }
            else
            {
                idx = DataPlane::CONTROLCHANNELVLANIDX_MGMT_CARRIER_2;
                normalizedVlan = normalizedVlan2;
            }
            cmn.carrier = DataPlane::CONTROLCHANNELCARRIER_2;
        }
        else
        {
            return false;
        }
    }
    cmn.type = gccType;
    uint32_t localVlan = vlanCfg->hal().vlanid().value();
    cmn.enable = true;
    cmn.rxVlanTag = normalizedVlan;
    cmn.rxVlanReTag = localVlan;
    cmn.txVlanTag = localVlan;
    cmn.txVlanReTag = normalizedVlan;
    cmn.deleteTxTags = false;

    //Calling DCO Gcc Adapter for configuration on DCO zynq
    if(mpGccAd)
    {
        cfgSuccess = mpGccAd->createCcVlan(idx, &cmn);
    }

    return cfgSuccess;
}

    // To delete CC_MGMT, CC_BC vlan remap entry in dco fpga 
    // Note: DpPeerDiscoveryMgr::DeleteVlan deletes CC_PEERDISC vlan remap entry in dco fpga
bool GccConfigHandler::deleteCcVlanConfig(VlanConfig* vlanCfg)
{
    bool cfgSuccess = false;
    INFN_LOG(SeverityLevel::info) << __func__ << "Delete vlan called";

    DataPlane::ControlChannelVlanIdx idx;
    DpAdapter::ControlChannelVlanCommon cmn;

    DataPlane::ControlChannelType gccType = DataPlane::CONTROLCHANNELTYPE_OFEC;

    std::string carrierAid = vlanCfg->base_config().config_id().value();
    std::size_t found = carrierAid.find_last_of("L");
    if(found != std::string::npos)
    {
        std::string carrierId = carrierAid.substr(found+1,1);
        if (carrierId == "1")
        {
            if ( string::npos != carrierAid.find("BC") )
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "Carrier AID " << carrierAid << " BC matched";
                idx = DataPlane::CONTROLCHANNELVLANIDX_BACK_CHANNEL_CARRIER_1;
                gccType = CONTROLCHANNELTYPE_BACK_CHANNEL;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "Carrier AID " << carrierAid;
                idx = DataPlane::CONTROLCHANNELVLANIDX_MGMT_CARRIER_1;
            }
            cmn.carrier = DataPlane::CONTROLCHANNELCARRIER_1;
        }
        else if (carrierId == "2")
        {
            if ( string::npos != carrierAid.find("BC") )
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "Carrier AID " << carrierAid << " BC matched";
                idx = DataPlane::CONTROLCHANNELVLANIDX_BACK_CHANNEL_CARRIER_2;
                gccType = CONTROLCHANNELTYPE_BACK_CHANNEL;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "Carrier AID " << carrierAid;
                idx = DataPlane::CONTROLCHANNELVLANIDX_MGMT_CARRIER_2;
            }
            cmn.carrier = DataPlane::CONTROLCHANNELCARRIER_2;
        }
        else
        {
            return false;
        }
    }
    cmn.type = gccType;
    uint32_t localVlan = vlanCfg->hal().vlanid().value();
    uint32_t normalizedVlan = DataPlane::NORMALIZED_VLANID_OFEC_CC_UNSPECIFIED;
    cmn.enable = true;
    cmn.rxVlanTag = normalizedVlan;
    cmn.rxVlanReTag = localVlan;
    cmn.txVlanTag = localVlan;
    cmn.txVlanReTag = normalizedVlan;
    cmn.deleteTxTags = false;

    //Calling DCO Gcc Adapter for configuration on DCO zynq
    if(mpGccAd)
    {
        cfgSuccess = mpGccAd->deleteCcVlan(idx);
    }
    
    // Delete Redis entry
    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;
    vlanCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(*vlanCfg);
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);

    return cfgSuccess;
}

STATUS GccConfigHandler::deleteGccConfigCleanup(VlanConfig* vlanCfg)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    std::string aid = vlanCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info)"AID: " << aid << " delete";

    VlanState vlanState;

    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    vlanState.mutable_base_state()->mutable_config_id()->set_value(aid);
    vlanState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    vlanState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    vlanState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(vlanState);

    vlanCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(*vlanCfg);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);

    return (status);
}

void GccConfigHandler::sendTransactionStateToInProgress(VlanConfig* vlanCfg)
{
    std::string aid = vlanCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction in-progress state";

    VlanState* pState = new VlanState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(vlanCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(vlanCfg->base_config().timestamp().nanos());

    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_INPROGRESS);

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    delete pState;
}

void GccConfigHandler::sendTransactionStateToComplete(VlanConfig* vlanCfg, STATUS status)
{
    std::string aid = vlanCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction complete state " << toString(status);

    VlanState* pState = new VlanState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(vlanCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(vlanCfg->base_config().timestamp().nanos());

    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_COMPLETE);
    pState->mutable_base_state()->set_transaction_status(status);

    pState->mutable_base_state()->mutable_transaction_info()->set_value(getTransactionErrorString());

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    setTransactionErrorString("");

    delete pState;
}

void GccConfigHandler::createCacheObj(chm6_pcp::VlanConfig& vlanMsg)
{
    chm6_pcp::VlanConfig* pVlanMsg = new chm6_pcp::VlanConfig(vlanMsg);

    std::string configId = vlanMsg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    addCacheObj(configId, pVlanMsg);
}

void GccConfigHandler::updateCacheObj(chm6_pcp::VlanConfig& vlanMsg)
{
    chm6_pcp::VlanConfig* pVlanMsg;
    google::protobuf::Message* pMsg;

    std::string configId = vlanMsg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    if (getCacheObj(configId, pMsg))
    {
        INFN_LOG(SeverityLevel::info) << "Cached Object not found for configId: " << configId;

        // todo: what to do here??? This should not happen
        createCacheObj(vlanMsg);
    }
    else
    {
        if (pMsg)
        {
            pVlanMsg = (chm6_pcp::VlanConfig*)pMsg;

            pVlanMsg->MergeFrom(vlanMsg);
        }
    }
}

void GccConfigHandler::deleteCacheObj(chm6_pcp::VlanConfig& vlanMsg)
{
    std::string configId = vlanMsg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    clearCache(configId);
}

STATUS GccConfigHandler::sendConfig(google::protobuf::Message& msg)
{
    chm6_pcp::VlanConfig* pVlanMsg = static_cast<chm6_pcp::VlanConfig*>(&msg);

    std::string configId = pVlanMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;


    // todo: is it ok to always call create here??
    createPcpVlanConfig(pVlanMsg);
    bool cfgSuccess = createCcVlanConfig(pVlanMsg);
    STATUS status = cfgSuccess ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL;

    return status;
}

STATUS GccConfigHandler::sendDelete(google::protobuf::Message& msg)
{
    chm6_pcp::VlanConfig* pVlanMsg = static_cast<chm6_pcp::VlanConfig*>(&msg);

    std::string configId = pVlanMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    deletePcpVlanConfig(pVlanMsg);
    bool cfgSuccess = deleteCcVlanConfig(pVlanMsg);
    STATUS status = cfgSuccess ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL;

    return status;
}

bool GccConfigHandler::isMarkForDelete(const google::protobuf::Message& msg)
{
    const chm6_pcp::VlanConfig* pMsg = static_cast<const chm6_pcp::VlanConfig*>(&msg);

    std::string configId = pMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    bool retVal = false;

    if (pMsg->base_config().has_mark_for_delete())
    {
        if (pMsg->base_config().mark_for_delete().value())
        {
            retVal = true;
        }
    }

    return retVal;
}

void GccConfigHandler::dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg)
{
    chm6_pcp::VlanConfig* pVlanMsg = static_cast<chm6_pcp::VlanConfig*>(&msg);

    std::string configId = pVlanMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    string jsonStr;
    MessageToJsonString(*pVlanMsg, &jsonStr);

    out << "** Dumping Vlan ConfigId: " << configId << endl;
    out << jsonStr << endl << endl;
}



} // namespace DataPlane
