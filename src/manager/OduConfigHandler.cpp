/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "OduConfigHandler.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"

#include <iostream>
#include <string>
#include <sstream>
#include <sys/time.h>

#include <InfnTracer/InfnTracer.h>
#include <InfnTracer/MessageTracer.h>

using chm6_dataplane::Chm6OduConfig;
using chm6_dataplane::Chm6OduState;
using chm6_dataplane::Chm6OduFault;
using chm6_dataplane::Chm6OduPm;

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace DpAdapter;


namespace DataPlane {

OduConfigHandler::OduConfigHandler(OduPbTranslator* pbTrans, DpAdapter::DcoOduAdapter *pAd, std::shared_ptr<OduStateCollector> spStateCollector)
    : mpPbTranslator(pbTrans)
    , mpDcoAd(pAd)
    , mspOduCollector(spStateCollector)
{
    INFN_LOG(SeverityLevel::info) << "Constructor";

    assert(pbTrans != NULL);
    assert(pAd  != NULL);
}

ConfigStatus OduConfigHandler::onCreate(Chm6OduConfig* oduCfg)
{
    std::string oduAid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid;

    std::string cfgDataStr;
    MessageToJsonString(*oduCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    ConfigStatus status = createOduConfig(oduCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Odu create on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus OduConfigHandler::onModify(Chm6OduConfig* oduCfg)
{
    std::string oduAid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid;

    std::string cfgDataStr;
    MessageToJsonString(*oduCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    ConfigStatus status = updateOduConfig(oduCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Odu update on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " status = " << statToString(status);

    return (status);
}


ConfigStatus OduConfigHandler::onDelete(Chm6OduConfig* oduCfg)
{
    std::string oduAid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid;

    std::string cfgDataStr;
    MessageToJsonString(*oduCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    ConfigStatus status = deleteOduConfig(oduCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Odu delete on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus OduConfigHandler::createOduConfig(Chm6OduConfig* oduCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " create";

    DpAdapter::OduCommon adOduCfg;

    mpPbTranslator->configPbToAd(*oduCfg, adOduCfg);

    bool ret = mpDcoAd->createOdu(aid, &adOduCfg);
    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "createOdu on DCO failed";
    }

    return (status);
}

ConfigStatus OduConfigHandler::updateOduConfig(Chm6OduConfig* oduCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " modify";

    bool retValue = true;
    bool ret;

    DpAdapter::OduCommon adOduCfg;

    mpPbTranslator->configPbToAd(*oduCfg, adOduCfg);

    // ConfigSvcType
    if (hal_common::BAND_WIDTH_UNSPECIFIED != oduCfg->hal().config_svc_type())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ConfigSvgType: ?";
        ret = true;  // Not modifiable
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setConfigSvcType() failed";
        }
    }

    // Regen Enable
    if (wrapper::BOOL_UNSPECIFIED != oduCfg->hal().regen_enable())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " regen: " << (int) adOduCfg.regen;
        ret = mpDcoAd->setOduRegen(aid, adOduCfg.regen);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setOduRegen() failed";
        }
    }

    // OduPrbsConfig
    if (oduCfg->hal().has_odu_prbs_config())
    {
        // FacPrbsGen
        if (wrapper::BOOL_UNSPECIFIED != oduCfg->hal().odu_prbs_config().fac_prbs_gen_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FacPrbsGen: " << adOduCfg.generatorTx;
            ret = mpDcoAd->setOduPrbsGen(aid, DataPlane::DIRECTION_TX, adOduCfg.generatorTx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOduPrbsGenTx() failed";
            }
        }

        // FacPrbsMon
        if (wrapper::BOOL_UNSPECIFIED != oduCfg->hal().odu_prbs_config().fac_prbs_mon_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FacPrbsMon: " << adOduCfg.monitorTx;
            ret = mpDcoAd->setOduPrbsMon(aid, DataPlane::DIRECTION_TX, adOduCfg.monitorTx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOduPrbsMonTx() failed";
            }
        }

        // TermPrbsGen
        if (wrapper::BOOL_UNSPECIFIED != oduCfg->hal().odu_prbs_config().term_prbs_gen_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TermPrbsGen: " << adOduCfg.generatorRx;
            ret = mpDcoAd->setOduPrbsGen(aid, DataPlane::DIRECTION_RX, adOduCfg.generatorRx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOduPrbsGenRx() failed";
            }
        }

        // TermPrbsMon
        if (wrapper::BOOL_UNSPECIFIED != oduCfg->hal().odu_prbs_config().term_prbs_mon_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TermPrbsMon: " << adOduCfg.monitorRx;
            ret = mpDcoAd->setOduPrbsMon(aid, DataPlane::DIRECTION_RX, adOduCfg.monitorRx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOduPrbsMonRx() failed";
            }
        }
    }

    // ServiceMode
    if (hal_common::SERVICE_MODE_UNSPECIFIED != oduCfg->hal().service_mode())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ServiceMode: ?";
        ret = true;  // Not modifiable
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setServiceMode() failed";
        }
    }

    // OduMsConfig
    if (oduCfg->hal().has_odu_ms_config())
    {
        // FacForceAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != oduCfg->hal().odu_ms_config().fac_forceaction())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ForceMsTx: " << adOduCfg.forceMsTx;
            ret = mpDcoAd->setOduForceMS(aid, DataPlane::DIRECTION_TX, adOduCfg.forceMsTx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setForceMsTx failed";
            }
        }

        // FacAutoAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != oduCfg->hal().odu_ms_config().fac_autoaction())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " AutoMsTx: " << adOduCfg.autoMsTx;
            ret = mpDcoAd->setOduAutoMS(aid, DataPlane::DIRECTION_TX, adOduCfg.autoMsTx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setAutoMsTx() failed";
            }
        }

        // TermForceAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != oduCfg->hal().odu_ms_config().term_forceaction())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ForceMsRx: " << adOduCfg.forceMsRx;
            ret = mpDcoAd->setOduForceMS(aid, DataPlane::DIRECTION_RX, adOduCfg.forceMsRx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setForceMsRx() failed";
            }
        }

        // TermAutoAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != oduCfg->hal().odu_ms_config().term_autoaction())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " AutoMsRx: " << adOduCfg.autoMsRx;
            ret = mpDcoAd->setOduAutoMS(aid, DataPlane::DIRECTION_RX, adOduCfg.autoMsRx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setAutoMsRx() failed";
            }
        }
    }

    // TxTtiConfig - TODO: Deprecate when new TTI map is implemented on Agent
    if (oduCfg->hal().tx_tti_config().transmit_tti_size() != 0)
    {
        uint8_t ttiArray[MAX_TTI_LENGTH];
        for (uint index = 0; index < oduCfg->hal().tx_tti_config().transmit_tti_size(); index++)
        {
            ttiArray[index] = oduCfg->hal().tx_tti_config().transmit_tti(index).value();
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TxTtiConfig[" << index << "] = " << ttiArray[index];
        }
        ret = mpDcoAd->setOduTti(aid, ttiArray);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setTxTtiConfig() failed";
        }
    }

    // TxTtiConfig
    if (oduCfg->hal().tx_tti_cfg().tti_size() != 0)
    {
        ret = mpDcoAd->setOduTti(aid, adOduCfg.ttiTx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setOduTti() failed";
        }
    }

    // Ts
    if (oduCfg->hal().timeslot().ts_size() != 0 )
    {
        ret = mpDcoAd->setOduTimeSlot(aid, adOduCfg.tS);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setOduTimeSlot() failed";
        }
    }

    // TsGranularity - Not modifiable

    // Loopback
    if (hal_common::LOOPBACK_TYPE_UNSPECIFIED != oduCfg->hal().loopback())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Loopback: " << adOduCfg.lpbk;
        ret = mpDcoAd->setOduLoopBack(aid, adOduCfg.lpbk);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setLoopback() failed";
        }
    }

    // Alarm Threshold Ingress
    if (oduCfg->hal().has_alarm_threshold())
    {
        // Signal Degrade Interval - Ingress
        if (oduCfg->hal().alarm_threshold().has_signal_degrade_interval())
        {
            // Used by Dataplane Mgr, not by DCO Adapter
        }

        // Signal Degrade Threshold - Ingress
        if (oduCfg->hal().alarm_threshold().has_signal_degrade_threshold())
        {
            // Used by Dataplane Mgr, not by DCO Adapter
        }
    }

    // Alarm Threshold Egress
    if (oduCfg->hal().has_alarm_threshold_egress())
    {
        // Signal Degrade Interval - Egress
        if (oduCfg->hal().alarm_threshold_egress().has_signal_degrade_interval())
        {
            // Used by Dataplane Mgr, not by DCO Adapter
        }

        // Signal Degrade Threshold - Egress
        if (oduCfg->hal().alarm_threshold_egress().has_signal_degrade_threshold())
        {
            // Used by Dataplane Mgr, not by DCO Adapter
        }
    }

    if (false == retValue)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::info) << "modifyOdu on DCO failed";
    }

    return (status);
}

ConfigStatus OduConfigHandler::deleteOduConfig(Chm6OduConfig* oduCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " delete";

    // Update DCO Adapter
    bool ret = mpDcoAd->deleteOdu(aid);
    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "deleteOdu on DCO failed";
    }

    return (status);
}

STATUS OduConfigHandler::deleteOduConfigCleanup(Chm6OduConfig* oduCfg)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    std::string aid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info)"AID: " << aid << " delete";

    Chm6OduState oduState;
    Chm6OduFault oduFault;
    Chm6OduPm    oduPm;

    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    oduState.mutable_base_state()->mutable_config_id()->set_value(aid);
    oduState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    oduState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    oduState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(oduState);

    oduFault.mutable_base_fault()->mutable_config_id()->set_value(aid);
    oduFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    oduFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    oduFault.mutable_base_fault()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(oduFault);

    oduPm.mutable_base_pm()->mutable_config_id()->set_value(aid);
    oduPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    oduPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    oduPm.mutable_base_pm()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(oduPm);

    oduCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(*oduCfg);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);

    return (status);
}

void OduConfigHandler::sendTransactionStateToInProgress(Chm6OduConfig* oduCfg)
{
    std::string aid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction in-progress state";

    Chm6OduState* pState = new Chm6OduState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(oduCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(oduCfg->base_config().timestamp().nanos());

    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_INPROGRESS);

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    delete pState;
}

void OduConfigHandler::sendTransactionStateToComplete(Chm6OduConfig* oduCfg,
                                                      STATUS status)
{
    std::string aid = oduCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction complete state " << toString(status);

    Chm6OduState stateObj;

    stateObj.mutable_base_state()->mutable_config_id()->set_value(aid);
    stateObj.mutable_base_state()->mutable_timestamp()->set_seconds(oduCfg->base_config().timestamp().seconds());
    stateObj.mutable_base_state()->mutable_timestamp()->set_nanos(oduCfg->base_config().timestamp().nanos());

    stateObj.mutable_base_state()->set_transaction_state(chm6_common::STATE_COMPLETE);
    stateObj.mutable_base_state()->set_transaction_status(status);

    stateObj.mutable_base_state()->mutable_transaction_info()->set_value(getTransactionErrorString());

    string stateDataStr;
    MessageToJsonString(stateObj, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(stateObj);

    setTransactionErrorString("");
}

void OduConfigHandler::createCacheObj(chm6_dataplane::Chm6OduConfig& objCfg)
{
    chm6_dataplane::Chm6OduConfig* pObjCfg = new chm6_dataplane::Chm6OduConfig(objCfg);

    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    addCacheObj(configId, pObjCfg);
}

void OduConfigHandler::updateCacheObj(chm6_dataplane::Chm6OduConfig& objCfg)
{
    chm6_dataplane::Chm6OduConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    std::string configId = objCfg.base_config().config_id().value();

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
            pObjCfg = (chm6_dataplane::Chm6OduConfig*)pMsg;

            pObjCfg->MergeFrom(objCfg);
        }
    }
}

void OduConfigHandler::deleteCacheObj(chm6_dataplane::Chm6OduConfig& objCfg)
{
    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    clearCache(configId);
}

STATUS OduConfigHandler::sendConfig(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6OduConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6OduConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    STATUS stat = (createOduConfig(pCfgMsg) == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);

    int oduIndex = mpDcoAd->getCacheOduId(configId);
    if (oduIndex < 0)
    {
        INFN_LOG(SeverityLevel::error) << "Failed to get OduId from oduAid " << configId;
    }
    else
    {
        mspOduCollector->updateOduIdx(configId, oduIndex);
    }

    return stat;
}

STATUS OduConfigHandler::sendDelete(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6OduConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6OduConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    return (deleteOduConfig(pCfgMsg) == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);
}

bool OduConfigHandler::isMarkForDelete(const google::protobuf::Message& msg)
{
    const chm6_dataplane::Chm6OduConfig* pMsg = static_cast<const chm6_dataplane::Chm6OduConfig*>(&msg);

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

void OduConfigHandler::dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6OduConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6OduConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    string jsonStr;
    MessageToJsonString(*pCfgMsg, &jsonStr);

    out << "** Dumping Odu ConfigId: " << configId << endl;
    mpPbTranslator->dumpJsonFormat(jsonStr, out);
    out << endl << endl;
}

void OduConfigHandler::getChm6OduConfigForceMsFromCache(std::string aid, Direction dir, MaintenanceSignal& msMode)
{
    chm6_dataplane::Chm6OduConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    msMode = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(aid, pMsg))
    {
        INFN_LOG(SeverityLevel::error) << "Cached Object not found for AID: " << aid;
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_dataplane::Chm6OduConfig*)pMsg;

            DpAdapter::OduCommon adOduCfg;
            mpPbTranslator->configPbToAd(*pObjCfg, adOduCfg);

            msMode = (DIRECTION_TX == dir) ? adOduCfg.forceMsTx : adOduCfg.forceMsRx;
        }
    }
}

void OduConfigHandler::getChm6OduConfigPrbsGenFromCache(std::string aid, Direction dir, PrbsMode& prbsMode)
{
    chm6_dataplane::Chm6OduConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    prbsMode = DataPlane::PRBSMODE_PRBS_NONE;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(aid, pMsg))
    {
        INFN_LOG(SeverityLevel::error) << "Cached Object not found for AID: " << aid;
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_dataplane::Chm6OduConfig*)pMsg;

            DpAdapter::OduCommon adOduCfg;
            mpPbTranslator->configPbToAd(*pObjCfg, adOduCfg);

            prbsMode = (DIRECTION_TX == dir) ? adOduCfg.generatorTx : adOduCfg.generatorRx;
        }
    }
}

} // namespace DataPlane
