/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DcoSecProcCardCfgHdlr.h"
#include "chm6/redis_adapter/application_servicer.h"
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

DcoSecProcCardCfgHdlr::DcoSecProcCardCfgHdlr( )
{
}

void DcoSecProcCardCfgHdlr::onCreate(chm6_common::Chm6DcoConfig* dcoCfg)
{
    string cfgData, cardAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::debug) << cfgData;

    cardAid = dcoCfg->base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << cardAid;

    (DpAdapter::DcoSecProcAdapter::GetInstance())->UpdateCardConfig(dcoCfg);
}

void DcoSecProcCardCfgHdlr::onModify(chm6_common::Chm6DcoConfig* dcoCfg)
{
    string cfgData, cardAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::debug) << cfgData;

    cardAid = dcoCfg->base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << cardAid;

    (DpAdapter::DcoSecProcAdapter::GetInstance())->UpdateCardConfig(dcoCfg);
}


void DcoSecProcCardCfgHdlr::onDelete(chm6_common::Chm6DcoConfig* dcoCfg)
{
    string cfgData, cardAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::debug) << cfgData;

    cardAid = dcoCfg->base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << cardAid;

}



} // namespace DataPlane

