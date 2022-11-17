/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DcoSecProcCarrierCfgHdlr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"

#include <iostream>
#include <string>
#include <sstream>

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace ::std;


namespace DataPlane {

DcoSecProcCarrierCfgHdlr::DcoSecProcCarrierCfgHdlr(DpAdapter::DcoSecProcCarrierAdapter* pAd): mpCarrierAdapter(pAd)
{
}

ConfigStatus DcoSecProcCarrierCfgHdlr::onCreate(chm6_dataplane::Chm6CarrierConfig* dcoCfg)
{
    string cfgData, CarrierAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::debug) << cfgData;

    CarrierAid = dcoCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << CarrierAid;

    grpc::StatusCode retVal = mpCarrierAdapter->UpdateCarrierConfig(dcoCfg);
    if( grpc::StatusCode::UNAVAILABLE == retVal)
        return ConfigStatus::FAIL_CONNECT;

    return ConfigStatus::SUCCESS;
}

ConfigStatus DcoSecProcCarrierCfgHdlr::onModify(chm6_dataplane::Chm6CarrierConfig* dcoCfg)
{
    string cfgData, CarrierAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::debug) << cfgData;

    CarrierAid = dcoCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << CarrierAid;

    grpc::StatusCode retVal = mpCarrierAdapter->UpdateCarrierConfig(dcoCfg);
    if( grpc::StatusCode::UNAVAILABLE == retVal)
        return ConfigStatus::FAIL_CONNECT;

    return ConfigStatus::SUCCESS;
}

ConfigStatus DcoSecProcCarrierCfgHdlr::onDelete(chm6_dataplane::Chm6CarrierConfig* dcoCfg)
{
    string cfgData, CarrierAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::debug) << cfgData;

    CarrierAid = dcoCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << CarrierAid;
    grpc::StatusCode retVal = mpCarrierAdapter->UpdateCarrierConfig(dcoCfg, true);

    if( grpc::StatusCode::UNAVAILABLE == retVal)
        return ConfigStatus::FAIL_CONNECT;

    return ConfigStatus::SUCCESS;
}

grpc::StatusCode DcoSecProcCarrierCfgHdlr::onCreate(string carrId, bool isOfecUp)
{
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << carrId;

    return(mpCarrierAdapter->UpdateCarrierConfig(carrId, isOfecUp, true));
}

grpc::StatusCode DcoSecProcCarrierCfgHdlr::onModify(string carrId, bool isOfecUp)
{
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << carrId;

    return (mpCarrierAdapter->UpdateCarrierConfig(carrId, isOfecUp));
}

grpc::StatusCode DcoSecProcCarrierCfgHdlr::onDelete(string carrId, bool isOfecUp)
{
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << carrId;
    return (mpCarrierAdapter->UpdateCarrierConfig(carrId, isOfecUp, false, true));
}

grpc::StatusCode DcoSecProcCarrierCfgHdlr::onModifyCarrRx(string carrId, bool isRxAcq)
{

    INFN_LOG(SeverityLevel::info) << "Called for AID: " << carrId;
    return (mpCarrierAdapter->UpdateCarrierRxConfig(carrId, isRxAcq));
}

} // namespace DataPlane

