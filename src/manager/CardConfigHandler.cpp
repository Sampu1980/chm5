/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "CardConfigHandler.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DpPeerDiscoveryMgr.h"
#include "DataPlaneMgr.h"

#include <iostream>
#include <string>
#include <sstream>

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace ::std;


namespace DataPlane {

static const int delayForRedisProcessSec = 2;

CardConfigHandler::CardConfigHandler(CardPbTranslator *pbTrans,
                                     DpAdapter::DcoCardAdapter *pCarAd,
                                     std::shared_ptr<DcoSecProcCardCfgHdlr> spSecProcCfgHndlr)
    : mpPbTranslator(pbTrans)
    , mpDcoCardAd(pCarAd)
    , mspDcoSecProcCardCfgHndlr(spSecProcCfgHndlr)
{
    INFN_LOG(SeverityLevel::info) << "";

    assert(pbTrans != NULL);
    assert(pCarAd  != NULL);
}

void CardConfigHandler::onCreate(chm6_common::Chm6DcoConfig* dcoCfg)
{
    string cfgData, cardAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << cfgData;

    cardAid = dcoCfg->base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << cardAid;

    UpdateCardConfig(dcoCfg);
}

void CardConfigHandler::onModify(chm6_common::Chm6DcoConfig* dcoCfg)
{
    string cfgData, cardAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << cfgData;

    cardAid = dcoCfg->base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << cardAid;

    UpdateCardConfig(dcoCfg);
}


void CardConfigHandler::onDelete(chm6_common::Chm6DcoConfig* dcoCfg)
{
    string cfgData, cardAid;
    MessageToJsonString(*dcoCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << cfgData;

    cardAid = dcoCfg->base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << cardAid;

    // what todo here? Delete is not expected

}

void CardConfigHandler::UpdateCardConfig(chm6_common::Chm6DcoConfig* dcoCfg)
{
    INFN_LOG(SeverityLevel::info) << "";

    bool status = true;

    switch (dcoCfg->dco_card_action())
    {
        case hal_common::BOARD_ACTION_ENABLE:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Card Enable";
            //mpDcoCardAd->setCardEnabled(true);
            break;

        case hal_common::BOARD_ACTION_DISABLE:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Card Disable";
            //mpDcoCardAd->setCardEnabled(false);
            break;

        case hal_common::BOARD_ACTION_RESTART_WARM:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Warm Boot";
            status = mpDcoCardAd->warmBoot();
            if (status == false)
            {
                INFN_LOG(SeverityLevel::info) << "DCO card warm boot failed";
            }
            break;

        case hal_common::BOARD_ACTION_RESTART_COLD:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Cold Boot. Sending Shutdown to DCO";
            status = mpDcoCardAd->shutdown(true);
            if (status == false)
            {
                INFN_LOG(SeverityLevel::info) << "DCO card cold boot failed";
            }
            break;

        case hal_common::BOARD_ACTION_GRACEFUL_SHUTDOWN:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Shut Down";
            DataPlaneMgrSingleton::Instance()->setDcoShutdownInProgress(true);
            status = mpDcoCardAd->shutdown(false);
            if (status == false)
            {
                INFN_LOG(SeverityLevel::info) << "DCO card shutdown failed";
                DataPlaneMgrSingleton::Instance()->setDcoShutdownInProgress(false);
            }
            UpdateDcoShutdownCompleteState();
            break;

        case hal_common::BOARD_ACTION_INIT_HW:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Init Hw - Not Implemented";

            break;

        case hal_common::BOARD_ACTION_UPDATE_FW:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Update Fw - Not Implemented";

            break;

        case hal_common::BOARD_ACTION_POWER_UP:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: Power Up - Not Implemented";

            break;

        case hal_common::BOARD_ACTION_UNSPECIFIED:
            INFN_LOG(SeverityLevel::info) << "Dco Card Action: UNSPECIFIED";

            break;
    }

    if (dcoCfg->has_max_packet_length())
    {
        INFN_LOG(SeverityLevel::info) << "Set DCO card pkt len" << dcoCfg->max_packet_length().value();
        status = mpDcoCardAd->setClientMaxPacketLength(dcoCfg->max_packet_length().value());
        if (status == false)
        {
            INFN_LOG(SeverityLevel::info) << "DCO card set pkt len failed";
        }
    }

}

void CardConfigHandler::createCacheObj(chm6_common::Chm6DcoConfig& objCfg)
{
    chm6_common::Chm6DcoConfig* pObjCfg = new chm6_common::Chm6DcoConfig(objCfg);

    std::string configId = objCfg.base_state().config_id().value();
    pObjCfg->set_dco_card_action(hal_common::BOARD_ACTION_UNSPECIFIED);

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    addCacheObj(configId, pObjCfg);
}

void CardConfigHandler::updateCacheObj(chm6_common::Chm6DcoConfig& objCfg)
{
    chm6_common::Chm6DcoConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    std::string configId = objCfg.base_state().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(configId, pMsg))
    {
        INFN_LOG(SeverityLevel::error) << "Cached Object not found for configId: " << configId;

        // todo: what to do here??? This should not happen
        createCacheObj(objCfg);
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_common::Chm6DcoConfig*)pMsg;

            pObjCfg->MergeFrom(objCfg);
            // Set card action explicitly to default value because the card action has been processed already
            pObjCfg->set_dco_card_action(hal_common::BOARD_ACTION_UNSPECIFIED);


        }
    }
}

void CardConfigHandler::deleteCacheObj(chm6_common::Chm6DcoConfig& objCfg)
{
    std::string configId = objCfg.base_state().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    clearCache(configId);
}

STATUS CardConfigHandler::sendConfig(google::protobuf::Message& msg)
{
    chm6_common::Chm6DcoConfig* pCfgMsg = static_cast<chm6_common::Chm6DcoConfig*>(&msg);

    std::string configId = pCfgMsg->base_state().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    // todo - any config for dco zynq?
    //onCreate(pCfgMsg);

    mspDcoSecProcCardCfgHndlr->onCreate(pCfgMsg);

    //Call peer discovery manager to store the icdp node info
    DpPeerDiscoveryMgrSingleton::Instance()->CreateIcdpNodeConfig(pCfgMsg->icdp_node_info());

    return STATUS::STATUS_SUCCESS;  // todo error handling for card
}

void CardConfigHandler::dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg)
{
    chm6_common::Chm6DcoCardConfig* pCfgMsg = static_cast<chm6_common::Chm6DcoCardConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    string jsonStr;
    MessageToJsonString(*pCfgMsg, &jsonStr);

    out << "** Dumping Card ConfigId: " << configId << endl;
    mpPbTranslator->dumpJsonFormat(jsonStr, out);
    out << endl << endl;
}

void CardConfigHandler::UpdateDcoShutdownCompleteState()
{
    chm6_common::Chm6DcoCardState* pCardState = new chm6_common::Chm6DcoCardState();;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    pCardState->mutable_base_state()->mutable_config_id()->set_value("Chm6Internal");
    pCardState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    pCardState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    pCardState->set_dco_shutdown_complete(wrapper::BOOL_TRUE);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pCardState);

    INFN_LOG(SeverityLevel::info) << "Update board_ms that dco_shutdown_complete is TRUE.";

    sleep(delayForRedisProcessSec);

    pCardState->set_dco_shutdown_complete(wrapper::BOOL_UNSPECIFIED);
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pCardState);

    INFN_LOG(SeverityLevel::info) << "Reset dco_shutdown_complete to UNSPECIFIED.";
}

} // namespace DataPlane
