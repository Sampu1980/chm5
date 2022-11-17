/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "OtuConfigHandler.h"
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

using chm6_dataplane::Chm6OtuConfig;
using chm6_dataplane::Chm6OtuState;
using chm6_dataplane::Chm6OtuFault;
using chm6_dataplane::Chm6OtuPm;
using lccmn_tom::TomPresenceMap;

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace DpAdapter;


namespace DataPlane {

#define MIN_TOM_ID   1
#define MAX_TOM_ID  16

OtuConfigHandler::OtuConfigHandler(OtuPbTranslator* pbTrans, DpAdapter::DcoOtuAdapter *pAd, gearbox::GearBoxAdIf* pGearBoxAd, tuning::SerdesTuner* pSerdesTuner)
    : mTomPresenceMap(0x0)
    , mpPbTranslator(pbTrans)
    , mpDcoAd(pAd)
    , mpGearBoxAd(pGearBoxAd)
    , mpSerdesTuner(pSerdesTuner)
{
    INFN_LOG(SeverityLevel::info) << "Constructor";

    assert(pbTrans != NULL);
    assert(pAd  != NULL);

    // Read TomPresenceMap in case it was sent when we weren't ready
    readTomPresenceMapFromRedis();
}

ConfigStatus OtuConfigHandler::onCreate(Chm6OtuConfig* otuCfg)
{
    std::string otuAid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid;

    std::string cfgDataStr;
    MessageToJsonString(*otuCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    int otuId = mpDcoAd->aidToOtuId(otuAid);
    if (otuId < 1 || otuId > DataPlane::MAX_OTU_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << otuAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " status = " << statToString(status);
        return (status);
    }

    ConfigStatus status = createOtuConfig(otuCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Otu create on DCO failed";
        setTransactionErrorString(log.str());
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " status = " << statToString(status);
        return (status);
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus OtuConfigHandler::onModify(Chm6OtuConfig* otuCfg)
{
    std::string otuAid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid;

    string cfgDataStr;
    MessageToJsonString(*otuCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    int otuId = mpDcoAd->aidToOtuId(otuAid);
    if (otuId < 1 || otuId > DataPlane::MAX_OTU_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << otuAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " status = " << statToString(status);
        return (status);
    }

    ConfigStatus status = updateOtuConfig(otuCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Otu update on DCO failed";
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus OtuConfigHandler::onDelete(Chm6OtuConfig* otuCfg)
{
    std::string otuAid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid;

    std::string cfgDataStr;
    MessageToJsonString(*otuCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    int otuId = mpDcoAd->aidToOtuId(otuAid);
    if (otuId < 1 || otuId > DataPlane::MAX_OTU_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << otuAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " status = " << statToString(status);
        return (status);
    }

    ConfigStatus status = deleteOtuConfig(otuCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Otu delete on DCO failed";
        setTransactionErrorString(log.str());
    }

    DataPlaneMgrSingleton::Instance()->clrTomSerdesTuned(otuId);

    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus OtuConfigHandler::createOtuConfig(Chm6OtuConfig* otuCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " create";

    // Collect SerDes data
    if (hal_common::BAND_WIDTH_OTU4 == otuCfg->hal().config_svc_type())
    {
        gearbox::Bcm81725DataRate_t portRate = gearbox::RATE_OTU4;

        if (mpSerdesTuner->TuneAtlToBcmLinks(aid, portRate) != 0)
        {
            INFN_LOG(SeverityLevel::error) << "AID: " << aid << " PortRate=" << portRate << " failed to retrieve serdes values, using defaults.";
            INFN_LOG(SeverityLevel::error) << "AID: " << aid << " SERDES (atl-bcm) status = FAIL";
            status = ConfigStatus::FAIL_OTHER;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " SERDES (atl-bcm) status = SUCCESS";

            std::vector<tuning::AtlBcmSerdesTxCfg> atlBcmTbl;
            if (mpSerdesTuner->GetAtlBcmMapParam(aid, portRate, atlBcmTbl) == true)
            {
                INFN_LOG(SeverityLevel::error) << "AID: " << aid << " Update DCO adapter with serdes values";
                mpDcoAd->updateSerdesCfgTbl(aid, atlBcmTbl);
            }
        }
    }

    // Create OTU
    DpAdapter::OtuCommon adOtuCfg;

    mpPbTranslator->configPbToAd(*otuCfg, adOtuCfg);

    bool ret = mpDcoAd->createOtu(aid, &adOtuCfg);
    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "createOtu on DCO failed";
        return status;
    }

    // Configure gearbox
    STATUS stat = updateGearBoxOtuConfig(otuCfg, true);
    if (STATUS::STATUS_FAIL == stat)
    {
        status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "OTU update on GearBox failed";
        setTransactionErrorString(log.str());
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " status = " << statToString(status);
        return (status);
    }

    return (status);
}

ConfigStatus OtuConfigHandler::updateOtuConfig(Chm6OtuConfig* otuCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " modify";

    bool retValue = true;
    bool ret;

    DpAdapter::OtuCommon adOtuCfg;
    mpPbTranslator->configPbToAd(*otuCfg, adOtuCfg);

    // ConfigSvcType - Not modifiable
    if (hal_common::BAND_WIDTH_UNSPECIFIED != otuCfg->hal().config_svc_type())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ConfigSvgType: ?";
        ret = false;  // ?
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setConfigSvcType() failed";
        }
    }

    // ServiceMode - Not modifiable
    if (hal_common::SERVICE_MODE_UNSPECIFIED != otuCfg->hal().service_mode())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ServiceMode: " << adOtuCfg.srvMode;
        ret = false;  // ?
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setServiceMode() failed";
        }
    }

    // OtuMsConfig
    if (otuCfg->hal().has_otu_ms_config())
    {
        // FacMs
        if (otuCfg->hal().otu_ms_config().has_fac_ms())
        {
            // ForcedMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != otuCfg->hal().otu_ms_config().fac_ms().forced_ms())
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ForcedMsTx: " << adOtuCfg.forceMsTx;
                ret = mpDcoAd->setOtuForceMS(aid, DataPlane::DIRECTION_TX, adOtuCfg.forceMsTx);
                if (false == ret)
                {
                    retValue = false;
                    INFN_LOG(SeverityLevel::error) << "setForcedMsTx() failed";
                }
            }

            // AutoMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != otuCfg->hal().otu_ms_config().fac_ms().auto_ms())
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " AutoMsTx: " << adOtuCfg.autoMsTx;
                ret = mpDcoAd->setOtuAutoMS(aid, DataPlane::DIRECTION_TX, adOtuCfg.autoMsTx);
                if (false == ret)
                {
                    retValue = false;
                    INFN_LOG(SeverityLevel::error) << "setAutoMsTx() failed";
                }
            }
        }

        // TermMs
        if (otuCfg->hal().otu_ms_config().has_term_ms())
        {
            // ForcedMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != otuCfg->hal().otu_ms_config().term_ms().forced_ms())
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ForcedMsRx: " << adOtuCfg.forceMsRx;
                ret = mpDcoAd->setOtuForceMS(aid, DataPlane::DIRECTION_RX, adOtuCfg.forceMsRx);
                if (false == ret)
                {
                    retValue = false;
                    INFN_LOG(SeverityLevel::error) << "setForcedMsRx() failed";
                }
            }

            // AutoMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != otuCfg->hal().otu_ms_config().term_ms().auto_ms())
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " AutoMsRx: " << adOtuCfg.autoMsRx;
                ret = mpDcoAd->setOtuAutoMS(aid, DataPlane::DIRECTION_RX, adOtuCfg.autoMsRx);
                if (false == ret)
                {
                    retValue = false;
                    INFN_LOG(SeverityLevel::error) << "setAutoMsRx() failed";
                }
            }
        }
    }

    // Loopback
    if (hal_common::LOOPBACK_TYPE_UNSPECIFIED != otuCfg->hal().loopback())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Loopback: " << adOtuCfg.lpbk;
        ret = mpDcoAd->setOtuLoopBack(aid, adOtuCfg.lpbk);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setLoopback() failed";
        }
    }

    // FecGenEnable
    if (wrapper::BOOL_UNSPECIFIED != otuCfg->hal().fec_gen_mode())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FecGenMode: " << adOtuCfg.fecGenEnable;
        ret = mpDcoAd->setOtuFecGenEnable(aid, adOtuCfg.fecGenEnable);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setOtuFecGenEnable() failed";
        }
    }

    // FecCorrEnable
    if (wrapper::BOOL_UNSPECIFIED != otuCfg->hal().fec_correction())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FecCorrEnable: " << adOtuCfg.fecCorrEnable;
        ret = mpDcoAd->setOtuFecCorrEnable(aid, adOtuCfg.fecCorrEnable);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setOtuFecCorrEnable() failed";
        }
    }

#if 0
    // LoopbackBehavior - Not used in R1.0
    if (hal_common::TERM_LOOPBACK_BEHAVIOR_UNSPECIFIED != otuCfg->hal().loopback_behavior())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " LoopbackBehavior: ?";
        ret = false;  // ?
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setLoopbackBehavior() failed";
        }
    }
#endif

    // OtuPrbsConfig
    if (otuCfg->hal().has_otu_prbs_config())
    {
        // FacPrbsGen
        if (wrapper::BOOL_UNSPECIFIED != otuCfg->hal().otu_prbs_config().fac_prbs_gen_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FacPrbsGen: " << adOtuCfg.generatorTx;
            ret = mpDcoAd->setOtuPrbsGen(aid, DataPlane::DIRECTION_TX, adOtuCfg.generatorTx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOtuPrbsGenTx() failed";
            }
        }

        // FacPrbsMon
        if (wrapper::BOOL_UNSPECIFIED != otuCfg->hal().otu_prbs_config().fac_prbs_mon_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FacPrbsMon: " << adOtuCfg.monitorTx;
            ret = mpDcoAd->setOtuPrbsMon(aid, DataPlane::DIRECTION_TX, adOtuCfg.monitorTx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOtuPrbsMonTx() failed";
            }
        }

        // TermPrbsGen
        if (wrapper::BOOL_UNSPECIFIED != otuCfg->hal().otu_prbs_config().term_prbs_gen_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TermPrbsGen: " << adOtuCfg.generatorRx;
            ret = mpDcoAd->setOtuPrbsGen(aid, DataPlane::DIRECTION_RX, adOtuCfg.generatorRx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOtuPrbsGenRx() failed";
            }
        }

        // TermPrbsMon
        if (wrapper::BOOL_UNSPECIFIED != otuCfg->hal().otu_prbs_config().term_prbs_mon_bool())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TermPrbsMon: " << adOtuCfg.monitorRx;
            ret = mpDcoAd->setOtuPrbsMon(aid, DataPlane::DIRECTION_RX, adOtuCfg.monitorRx);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::error) << "setOtuPrbsMonRx() failed";
            }
        }
    }

    // TxTtiConfig
    if (otuCfg->hal().tx_tti_cfg().tti_size() != 0)
    {
        ret = mpDcoAd->setOtuTti(aid, adOtuCfg.ttiTx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setOtuTti() failed";
        }
    }

    // AlarmThreshold
    if (otuCfg->hal().has_alarm_threshold())
    {
        // SignalDegradeInterval
        if (otuCfg->hal().alarm_threshold().has_signal_degrade_interval())
        {
            // Used by Dataplane Mgr, not by DCO Adapter
        }

        // SignalDegradeThreshold
        if (otuCfg->hal().alarm_threshold().has_signal_degrade_threshold())
        {
            // Used by Dataplane Mgr, not by DCO Adapter
        }
    }

    if (false == retValue)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "modifyOtu on DCO failed";
    }

    return (status);
}

ConfigStatus OtuConfigHandler::deleteOtuConfig(Chm6OtuConfig* otuCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " delete";

    // Update DCO Adapter
    bool ret = mpDcoAd->deleteOtu(aid);
    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "deleteOtu on DCO failed";
    }

    return (status);
}

STATUS OtuConfigHandler::deleteOtuConfigCleanup(Chm6OtuConfig* otuCfg)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    TRACE(

    std::string aid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info)"AID: " << aid << " delete";
    TRACE_TAG("message.aid", aid);
    TRACE_TAG("message.name", otuCfg->GetDescriptor()->full_name());

    Chm6OtuState otuState;
    Chm6OtuFault otuFault;
    Chm6OtuPm    otuPm;

    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    otuState.mutable_base_state()->mutable_config_id()->set_value(aid);
    otuState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    otuState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    otuState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);
    TRACE_STATE(Chm6OtuState, otuState, TO_REDIS(),)
    vDelObjects.push_back(otuState);

    otuFault.mutable_base_fault()->mutable_config_id()->set_value(aid);
    otuFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    otuFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    otuFault.mutable_base_fault()->mutable_mark_for_delete()->set_value(true);
    TRACE_FAULT(Chm6OtuFault, otuFault, TO_REDIS(),)
    vDelObjects.push_back(otuFault);

    otuPm.mutable_base_pm()->mutable_config_id()->set_value(aid);
    otuPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    otuPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    otuPm.mutable_base_pm()->mutable_mark_for_delete()->set_value(true);
    TRACE_PM(Chm6OtuPm, otuPm, TO_REDIS(),)
    vDelObjects.push_back(otuPm);

    otuCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
    TRACE_CONFIG(Chm6OtuConfig, *otuCfg, TO_REDIS(),);
    vDelObjects.push_back(*otuCfg);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);

    )

    return (status);
}

void OtuConfigHandler::handleTomPresenceMapUpdate(TomPresenceMap* pTomMsg)
{
    string data;
    MessageToJsonString(*pTomMsg, &data);
    INFN_LOG(SeverityLevel::info) << data;

    lock_guard<recursive_mutex> lock(mMutex);

    if (pTomMsg->has_tom_presence_map())
    {
        uint32_t tomPresenceMap = pTomMsg->tom_presence_map().value();
        uint32_t diffPresenceMap = mTomPresenceMap ^ tomPresenceMap;

        INFN_LOG(SeverityLevel::info) << "mTomPresenceMap = 0x" << std::hex << mTomPresenceMap << std::dec;
        INFN_LOG(SeverityLevel::info) << "TomPresenceMap  = 0x" << std::hex << tomPresenceMap << std::dec;
        INFN_LOG(SeverityLevel::info) << "DiffPresenceMap = 0x" << std::hex << diffPresenceMap << std::dec;

        // Determine which tom has changed
        uint32 bitMask = 0x1;
        for (uint i = MIN_TOM_ID; i <= MAX_TOM_ID; i++)
        {
            if (0 != (diffPresenceMap & bitMask))
            {
                if (0 != (tomPresenceMap & bitMask))
                {
                    INFN_LOG(SeverityLevel::info) << "Tom" << i << " changed to present";

                    std::string otuOtnAid = mpPbTranslator->tomToOtnAid(i);
                    if (isCacheExist(otuOtnAid))
                    {
                        bool isTomSerdesTuned = DataPlaneMgrSingleton::Instance()->isTomSerdesTuned(i);
                        if (false == isTomSerdesTuned)
                        {
                            // Perform Serdes
                            INFN_LOG(SeverityLevel::info) << "AID: " << otuOtnAid << " TuneLinks";
                            int ret = mpSerdesTuner->TuneLinks(otuOtnAid, true);
                            if (0 != ret)
                            {
                                INFN_LOG(SeverityLevel::error) << "AID: " << otuOtnAid << " SERDES (tom-bcm) status = FAIL";
                            }
                            else
                            {
                                INFN_LOG(SeverityLevel::info) << "AID: " << otuOtnAid << " SERDES (tom-bcm) status = SUCCESS";
                                DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(i);
                            }
                        }
                    }
                }
                else
                {
                    INFN_LOG(SeverityLevel::info) << "Tom" << i << " changed to not present";
                    DataPlaneMgrSingleton::Instance()->clrTomSerdesTuned(i);
                }
            }
            bitMask <<= 1;
        }

        mTomPresenceMap = tomPresenceMap;
    }

    if (pTomMsg->has_tom_serdes_retune_map())
    {
        uint32_t tomSerdesRetuneMap =  pTomMsg->tom_serdes_retune_map().value();
        INFN_LOG(SeverityLevel::info) << "TomSerdesRetuneMap value = 0x" << std::hex << tomSerdesRetuneMap << std::dec;

        if (!tomSerdesRetuneMap || !(mTomPresenceMap & tomSerdesRetuneMap))
        {
            INFN_LOG(SeverityLevel::error) << "Invalid Tom serdes retune request - Serdes Tuning FAIL returning now";
            return;
        }

        uint32_t tomId = 1;

        // Determine which tom has cold rebooted
        uint32 bitMask = 0x1;
        while ((bitMask != tomSerdesRetuneMap) && (tomId <= MAX_TOM_ID))
        {
            INFN_LOG(SeverityLevel::info) << "Bumping tomId = " << tomId << std::hex << " tomSerdesRetuneMap = " << tomSerdesRetuneMap << " bitMask = " << bitMask << std::dec;
            tomSerdesRetuneMap >>= 1;
            tomId++;
        }

        if (tomId <= MAX_TOM_ID)
        {
            std::string otuOtnAid = mpPbTranslator->tomToOtnAid(tomId);
            INFN_LOG(SeverityLevel::info) << "AID: " << otuOtnAid << " TuneLinks";
            if (isCacheExist(otuOtnAid))
            {
                // Perform Serdes
                int ret = mpSerdesTuner->TuneLinks(otuOtnAid, true);
                if (0 != ret)
                {
                    INFN_LOG(SeverityLevel::error) << "AID: " << otuOtnAid << " SERDES (tom-bcm) status = FAIL";
                }
                else
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << otuOtnAid << " SERDES (tom-bcm) status = SUCCESS";
                    DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(tomId);
                }
            }
        }
    }
}

void OtuConfigHandler::readTomPresenceMapFromRedis()
{
    TomPresenceMap presenceMap;

    lock_guard<recursive_mutex> lock(mMutex);

    try
    {
        presenceMap.mutable_base_state()->mutable_config_id()->set_value("Chm6Internal");

        TRACE_STATE(TomPresenceMap, presenceMap, TO_REDIS(),

        AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectRead(presenceMap);

        if (presenceMap.has_tom_presence_map())
        {
            INFN_LOG(SeverityLevel::info) << "has tom presence map!";

            mTomPresenceMap = presenceMap.tom_presence_map().value();
            INFN_LOG(SeverityLevel::info) << "mTomPresenceMap=" << std::hex << "0x" << mTomPresenceMap << std::dec;

        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "has no tom presence map!";
        }
	)
    }
    catch(std::exception const &excp)
    {
        INFN_LOG(SeverityLevel::info) << "Exception caught in getTomPresenceMap";

    }
    catch(...)
    {
        INFN_LOG(SeverityLevel::info) << "Redis DB may NOT have TomPresenceMap message.";

    }
}

STATUS OtuConfigHandler::updateGearBoxOtuConfig(Chm6OtuConfig* otuCfg, bool onCreate)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    std::string aid = otuCfg->base_config().config_id().value();

    int ret;

    lock_guard<recursive_mutex> lock(mMutex);

    if (hal_common::BAND_WIDTH_OTU4 == otuCfg->hal().config_svc_type())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid;

        gearbox::Bcm81725DataRate_t portRate = gearbox::RATE_OTU4;
        bool fecEnable = false;

        ret = mpGearBoxAd->setPortRate(aid, portRate, fecEnable);
        if (0 != ret)
        {
            status = STATUS::STATUS_FAIL;
        }

        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " RETIMER GearBox status = " << toString(status);

        int tomId = mpPbTranslator->otnAidToTom(aid);
        if (tomId < 1)
        {
            INFN_LOG(SeverityLevel::error) << "AID: " << aid << "Invalid aid";
            return STATUS::STATUS_FAIL;
        }

        bool isTomSerdesTuned = DataPlaneMgrSingleton::Instance()->isTomSerdesTuned(tomId);
        if ( (mTomPresenceMap & (1 << ((uint32_t)(tomId-1)))) && (false == isTomSerdesTuned) )
        {
            ret = mpSerdesTuner->TuneLinks(aid, portRate, onCreate);
            if (0 != ret)
            {
                INFN_LOG(SeverityLevel::error) << "AID: " << aid << " SERDES (tom-bcm) status = FAIL";
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " SERDES (tom-bcm) status = SUCCESS";
                DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(tomId);
            }
        }
    }

    return (status);
}

void OtuConfigHandler::sendTransactionStateToInProgress(Chm6OtuConfig* otuCfg)
{
    static const std::string status = "in-progress";
    static const std::string statusKey = "config.transaction_status";
    static const std::string operation = "OtuConfigHandler/sendTransactionState";

    TRACE(

    std::string aid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction in-progress state";

    Chm6OtuState* pState = new Chm6OtuState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(otuCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(otuCfg->base_config().timestamp().nanos());
    pState->mutable_base_state()->mutable_trace_context_ls()->CopyFrom(
        otuCfg->base_config().trace_context_ls());
    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_INPROGRESS);

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    TRACE_TAG(statusKey, status);
    TRACE_TAG("message.aid", aid);
    TRACE_TAG("message.name", otuCfg->GetDescriptor()->full_name());

    delete pState;
    )
}

void OtuConfigHandler::sendTransactionStateToComplete(Chm6OtuConfig* otuCfg,
                                                      STATUS status)
{
    static const std::string tStatus = "complete";
    static const std::string stateKey = "config.transaction_state";
    static const std::string statusKey = "config.transaction_status";
    static const std::string operation = "OtuConfigHandler/SendTransactionState";

    TRACE(

    std::string aid = otuCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction complete state " << toString(status);

    Chm6OtuState stateObj;

    stateObj.mutable_base_state()->mutable_config_id()->set_value(aid);
    stateObj.mutable_base_state()->mutable_timestamp()->set_seconds(otuCfg->base_config().timestamp().seconds());
    stateObj.mutable_base_state()->mutable_timestamp()->set_nanos(otuCfg->base_config().timestamp().nanos());
    stateObj.mutable_base_state()->mutable_trace_context_ls()->CopyFrom(
        otuCfg->base_config().trace_context_ls());

    stateObj.mutable_base_state()->set_transaction_state(chm6_common::STATE_COMPLETE);
    stateObj.mutable_base_state()->set_transaction_status(status);

    stateObj.mutable_base_state()->mutable_transaction_info()->set_value(getTransactionErrorString());

    string stateDataStr;
    MessageToJsonString(stateObj, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(stateObj);

    setTransactionErrorString("");

    TRACE_TAG(statusKey, tStatus);
    TRACE_TAG(stateKey, toString(status));
    TRACE_TAG("message.aid", aid);
    TRACE_TAG("message.name", otuCfg->GetDescriptor()->full_name());
    )
}

void OtuConfigHandler::createCacheObj(chm6_dataplane::Chm6OtuConfig& objCfg)
{
    chm6_dataplane::Chm6OtuConfig* pObjCfg = new chm6_dataplane::Chm6OtuConfig(objCfg);

    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    addCacheObj(configId, pObjCfg);
}

void OtuConfigHandler::updateCacheObj(chm6_dataplane::Chm6OtuConfig& objCfg)
{
    chm6_dataplane::Chm6OtuConfig* pObjCfg;
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
            pObjCfg = (chm6_dataplane::Chm6OtuConfig*)pMsg;

            pObjCfg->MergeFrom(objCfg);
        }
    }
}

void OtuConfigHandler::deleteCacheObj(chm6_dataplane::Chm6OtuConfig& objCfg)
{
    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    clearCache(configId);
}

STATUS OtuConfigHandler::sendConfig(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6OtuConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6OtuConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    // todo: is it ok to always call create here??
    return (createOtuConfig(pCfgMsg) == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);
}

STATUS OtuConfigHandler::sendDelete(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6OtuConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6OtuConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    return (deleteOtuConfig(pCfgMsg) == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);
}

bool OtuConfigHandler::isMarkForDelete(const google::protobuf::Message& msg)
{
    const chm6_dataplane::Chm6OtuConfig* pMsg = static_cast<const chm6_dataplane::Chm6OtuConfig*>(&msg);

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

void OtuConfigHandler::dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6OtuConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6OtuConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    string jsonStr;
    MessageToJsonString(*pCfgMsg, &jsonStr);

    out << "** Dumping Otu ConfigId: " << configId << endl;
    mpPbTranslator->dumpJsonFormat(jsonStr, out);
    out << endl << endl;
}

} // namespace DataPlane
