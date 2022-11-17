/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "GigeClientConfigHandler.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"
#include "AtlanticSerdesTable.h"
#include "../gearbox/GearBoxUtils.h"

#include <InfnTracer/InfnTracer.h>

#include <iostream>
#include <string>
#include <sstream>

using chm6_dataplane::Chm6GigeClientConfig;
using chm6_dataplane::Chm6GigeClientState;
using chm6_dataplane::Chm6GigeClientFault;
using chm6_dataplane::Chm6GigeClientPm;

using lccmn_tom::TomPresenceMap;


using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;


namespace DataPlane {

GigeClientConfigHandler::GigeClientConfigHandler(GigeClientPbTranslator* pbTrans, DpAdapter::DcoGigeClientAdapter* pDcoAd, gearbox::GearBoxAdIf* pGearBoxAd,
        tuning::SerdesTuner* pSerdesTuner)

    : mTomPresenceMap(0x0)
    , mpPbTranslator(pbTrans)
    , mpDcoAd(pDcoAd)
    , mpGearBoxAd(pGearBoxAd)
    , mpSerdesTuner(pSerdesTuner)
{
    INFN_LOG(SeverityLevel::info) << "Contructor";

    assert(pbTrans != NULL);
    assert(pGearBoxAd  != NULL);
    assert(pSerdesTuner != NULL);

    // Read TomPresenceMap in case it was sent when we weren't ready
    readTomPresenceMapFromRedis();
}

ConfigStatus GigeClientConfigHandler::onCreate(Chm6GigeClientConfig* gigeClientCfg)
{
    std::string gigeAid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid;

    std::string cfgDataStr;
    MessageToJsonString(*gigeClientCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    int gigeId = mpDcoAd->aidToPortId(gigeAid);
    if (gigeId < 1 || gigeId > DataPlane::MAX_GIGE_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << gigeAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
        return (status);
    }

    // Create gige
    ConfigStatus status = createGigeClientConfig(gigeClientCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "GigeClient create on DCO failed";
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
        return (status);
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
    return (status);
}

ConfigStatus GigeClientConfigHandler::onModify(Chm6GigeClientConfig* gigeClientCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string gigeAid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid;

    std::string cfgDataStr;
    MessageToJsonString(*gigeClientCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    int gigeId = mpDcoAd->aidToPortId(gigeAid);
    if (gigeId < 1 || gigeId > DataPlane::MAX_GIGE_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << gigeAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
        return (status);
    }

    // Update DCO
    status = updateGigeClientConfig(gigeClientCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "GigeClient update on DCO failed";
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
        return (status);
    }

    // Update Gearbox
    STATUS stat = updateGearBoxGigeClientConfig(gigeClientCfg, false);
    if (STATUS::STATUS_FAIL == stat)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;  // todo: double check

        std::ostringstream log;
        log << "GigeClient update on GearBox failed";
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
        return (status);
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
    return (status);
}


ConfigStatus GigeClientConfigHandler::onDelete(Chm6GigeClientConfig* gigeClientCfg)
{
    std::string gigeAid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid;

    std::string cfgDataStr;
    MessageToJsonString(*gigeClientCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    int gigeId = mpDcoAd->aidToPortId(gigeAid);
    if (gigeId < 1 || gigeId > DataPlane::MAX_GIGE_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << gigeAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
        return (status);
    }

    ConfigStatus status = deleteGigeClientConfig(gigeClientCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "GigeClient delete on DCO failed";
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);
        return (status);
    }

    DataPlaneMgrSingleton::Instance()->clrTomSerdesTuned(gigeId);

    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus GigeClientConfigHandler::createGigeClientConfig(Chm6GigeClientConfig* gigeClientCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " create";


    if (hal_dataplane::PORT_RATE_UNSPECIFIED != gigeClientCfg->hal().rate())
    {
        uint rate = gigeClientCfg->hal().rate();
        gearbox::Bcm81725DataRate_t portRate = (hal_dataplane::PORT_RATE_ETH100G == rate ? gearbox::RATE_100GE : gearbox::RATE_400GE);

        if (mpSerdesTuner->TuneAtlToBcmLinks(aid, portRate) != 0)
        {
            INFN_LOG(SeverityLevel::error) << "AID: " << aid << " PortRate=" <<  gigeClientCfg->hal().rate() << " failed to retrieve serdes values, using defaults.";
            INFN_LOG(SeverityLevel::error) << "AID: " << aid << " SERDES (atl-bcm) status = FAIL";
            status = ConfigStatus::FAIL_OTHER;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " SERDES (atl-bcm) status = SUCCESS";

            std::vector<tuning::AtlBcmSerdesTxCfg> atlBcmTbl;
            if (mpSerdesTuner->GetAtlBcmMapParam(aid, portRate, atlBcmTbl) == true)
            {
                DataPlane::PortRate dataPlanePortRate = (hal_dataplane::PORT_RATE_ETH100G == rate ? DataPlane::PORT_RATE_ETH100G : DataPlane::PORT_RATE_ETH400G);
                mpDcoAd->updateSerdesCfgTbl(aid, dataPlanePortRate, atlBcmTbl);
            }
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "AID: " << aid << " PortRate missing!";
    }


    DpAdapter::GigeCommon adGigeClientCfg;

    mpPbTranslator->configPbToAd(*gigeClientCfg, adGigeClientCfg);

    bool ret = mpDcoAd->createGigeClient(aid, &adGigeClientCfg);
    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::info) << "createGigeClient on DCO failed";
        return status;
    }

    // Configure gearbox
    STATUS stat = updateGearBoxGigeClientConfig(gigeClientCfg, true);
    if (STATUS::STATUS_FAIL == stat)
    {
        status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "GigeClient update on GearBox failed";
        setTransactionErrorString(log.str());
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " status = " << statToString(status);
        return (status);
    }


    return (status);
}

ConfigStatus GigeClientConfigHandler::updateGigeClientConfig(Chm6GigeClientConfig* gigeClientCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " modify";

	bool retValue = true;
	bool ret;

    DpAdapter::GigeCommon adGigeClientCfg;

    mpPbTranslator->configPbToAd(*gigeClientCfg, adGigeClientCfg);

    // Port Rate
    if (hal_dataplane::PORT_RATE_UNSPECIFIED != gigeClientCfg->hal().rate())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " PortRate: " << adGigeClientCfg.rate;
        // no-op
    }

    // LoopBack
    if (hal_dataplane::LOOP_BACK_TYPE_UNSPECIFIED != gigeClientCfg->hal().loopback())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Loopback: " << adGigeClientCfg.lpbk;
        ret = mpDcoAd->setGigeLoopBack(aid, adGigeClientCfg.lpbk);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeLoopBack() failed";
        }
    }

    // FEC Enable
    if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().fec_enable_bool())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FecEnable: " << adGigeClientCfg.fecEnable;
        ret = mpDcoAd->setGigeFecEnable(aid, adGigeClientCfg.fecEnable);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeFecEnable() failed";
        }
    }

    // MTU
    if (gigeClientCfg->hal().has_mtu())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Mtu: " << adGigeClientCfg.mtu;
        ret = mpDcoAd->setGigeMtu(aid, adGigeClientCfg.mtu);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeMtu() failed";
        }
    }

    // Rx/TX Snoop/Drop Enable
    if (gigeClientCfg->hal().has_mode())
    {
        // RX Snoop Enable
        if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().mode().rx_snoop_enable_bool())
        {
			INFN_LOG(SeverityLevel::info) << "AID: " << aid << " RxSnoopEnable: " << adGigeClientCfg.lldpRxSnoop;
            ret = mpDcoAd->setGigeLldpSnoop(aid, DIRECTION_RX, adGigeClientCfg.lldpRxSnoop);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeLldpRxSnoop() failed";
            }
        }

        // TX Snoop Enable
        if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().mode().tx_snoop_enable_bool())
        {
			INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TxSnoopEnable: " << adGigeClientCfg.lldpTxSnoop;
            ret = mpDcoAd->setGigeLldpSnoop(aid, DIRECTION_TX, adGigeClientCfg.lldpTxSnoop);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeLldpTxSnoop() failed";
            }
        }

        // RX Drop Enable
        if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().mode().rx_drop_enable_bool())
        {
			INFN_LOG(SeverityLevel::info) << "AID: " << aid << " RxDropEnable: " << adGigeClientCfg.lldpRxDrop;
            ret = mpDcoAd->setGigeLldpDrop(aid, DIRECTION_RX, adGigeClientCfg.lldpRxDrop);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeLldpRxDrop() failed";
            }
        }

        // TX Drop Enable
        if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().mode().tx_drop_enable_bool())
        {
			INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TxDropEnable: " << adGigeClientCfg.lldpTxDrop;
            ret = mpDcoAd->setGigeLldpDrop(aid, DIRECTION_TX, adGigeClientCfg.lldpTxDrop);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeLldpTxDrop() failed";
            }
        }
    }

    // Auto TX
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != gigeClientCfg->hal().auto_tx())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " AutoTx: " << adGigeClientCfg.autoMsTx;
        ret = mpDcoAd->setGigeAutoMS(aid, DIRECTION_TX, adGigeClientCfg.autoMsTx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeAutoTxMS() failed";
        }
    }

    // Force TX
    DataPlane::MaintenanceSignal cacheOverrideForceMsTx = DataPlane::MAINTENANCE_SIGNAL_UNSPECIFIED;
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != gigeClientCfg->hal().force_tx())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ForceTx: " << adGigeClientCfg.forceMsTx;
        ret = mpDcoAd->setGigeForceMS(aid, DIRECTION_TX, adGigeClientCfg.forceMsTx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeForceTxMS() failed";
        }
        else
        {
            cacheOverrideForceMsTx = adGigeClientCfg.forceMsTx;
        }
    }

    // Auto RX
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != gigeClientCfg->hal().auto_rx())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " AutoRx: " << adGigeClientCfg.autoMsRx;
        ret = mpDcoAd->setGigeAutoMS(aid, DIRECTION_RX, adGigeClientCfg.autoMsRx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeAutoRxMS() failed";
        }
    }

    // Force RX
    DataPlane::MaintenanceSignal cacheOverrideForceMsRx = DataPlane::MAINTENANCE_SIGNAL_UNSPECIFIED;
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != gigeClientCfg->hal().force_rx())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " ForceRx: " << adGigeClientCfg.forceMsRx;
        ret = mpDcoAd->setGigeForceMS(aid, DIRECTION_RX, adGigeClientCfg.forceMsRx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeForceRxMS() failed";
        }
        else
        {
            cacheOverrideForceMsRx = adGigeClientCfg.forceMsRx;
        }
    }

    // Test Signal Gen TX
    if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().test_signal_gen())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TestSignalGenFac: " << adGigeClientCfg.generatorTx;
        ret = mpDcoAd->setGigeTestSignalGen(aid, DIRECTION_TX, adGigeClientCfg.generatorTx, cacheOverrideForceMsTx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeTestSignalGen Tx() failed";
        }
    }

    // Test Signal Gen RX
    if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().test_signal_gen_term())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TestSignalGenTerm: " << adGigeClientCfg.generatorRx;
        ret = mpDcoAd->setGigeTestSignalGen(aid, DIRECTION_RX, adGigeClientCfg.generatorRx, cacheOverrideForceMsRx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeTestSignalGen Rx() failed";
        }
    }

    // Test Signal Mon
    if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().test_signal_mon())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " TestSignalMon: " << adGigeClientCfg.monitorRx;
        ret = mpDcoAd->setGigeTestSignalMon(aid, adGigeClientCfg.monitorRx);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeTestSignalMon() failed";
        }
    }

    // Fwd Defect TDA
    if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().fwd_defect_tda())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FwdDefectTda: " << adGigeClientCfg.fwdTdaTrigger;
        ret = mpDcoAd->setGigeFwdTdaTrigger(aid, adGigeClientCfg.fwdTdaTrigger);
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::info) << "setGigeFwdTdaTrigger() failed";
        }
    }

    // FEC Deg SER
    if (gigeClientCfg->hal().has_alarm_threshold())
    {
        if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().alarm_threshold().fec_deg_ser_en())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FEC Deg SER Enable: " << adGigeClientCfg.fecDegSer.enable;
            ret = mpDcoAd->setGigeFecDegSerEn(aid, adGigeClientCfg.fecDegSer.enable);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeFecDegSerEn() failed";
            }
        }

        if (gigeClientCfg->hal().alarm_threshold().has_fec_deg_ser_act_thresh())
        {
            // FEC Deg SER Activate Threshold
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FEC Deg SER Activate Threshold: " << adGigeClientCfg.fecDegSer.activateThreshold;
            ret = mpDcoAd->setGigeFecDegSerActThresh(aid, adGigeClientCfg.fecDegSer.activateThreshold);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeFecDegSerActThresh() failed";
            }
        }

        if (gigeClientCfg->hal().alarm_threshold().has_fec_deg_ser_deact_thresh())
        {
            // FEC Deg SER Deactivate Threshold
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FEC Deg SER Deactivate Threshold: " << adGigeClientCfg.fecDegSer.deactivateThreshold;
            ret = mpDcoAd->setGigeFecDegSerDeactThresh(aid, adGigeClientCfg.fecDegSer.deactivateThreshold);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeFecDegSerDeActThresh() failed";
            }
        }

        if (gigeClientCfg->hal().alarm_threshold().has_fec_deg_ser_mon_per())
        {
            // FEC Deg SER Monitor Period
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FEC Deg SER Monitor Period: " << adGigeClientCfg.fecDegSer.monitorPeriod;
            ret = mpDcoAd->setGigeFecDegSerMonPeriod(aid, adGigeClientCfg.fecDegSer.monitorPeriod);
            if (false == ret)
            {
                retValue = false;
                INFN_LOG(SeverityLevel::info) << "setGigeFecDegSerMonPeriod() failed";
            }
        }
    }

    if (false == retValue)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::info) << "modifyGigeClient on DCO failed";
    }

    return (status);
}

ConfigStatus GigeClientConfigHandler::deleteGigeClientConfig(Chm6GigeClientConfig* gigeClientCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " delete";

    // Update DCO Adapter
    bool ret = mpDcoAd->deleteGigeClient(aid);
    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::info) << "deleteGigeClient on DCO failed";
    }

    return (status);
}

STATUS GigeClientConfigHandler::deleteGigeClientConfigCleanup(Chm6GigeClientConfig* gigeClientCfg)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    TRACE(
    std::string aid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info)"AID: " << aid << " delete";
    TRACE_TAG("message.aid", aid);
    TRACE_TAG("message.name", gigeClientCfg->GetDescriptor()->full_name());

    Chm6GigeClientState gigeClientState;
    Chm6GigeClientFault gigeClientFault;
    Chm6GigeClientPm    gigeClientPm;

    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    gigeClientState.mutable_base_state()->mutable_config_id()->set_value(aid);
    gigeClientState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    gigeClientState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    gigeClientState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);
    TRACE_STATE(Chm6GigeClientState, gigeClientState, TO_REDIS(),)
    vDelObjects.push_back(gigeClientState);

    gigeClientFault.mutable_base_fault()->mutable_config_id()->set_value(aid);
    gigeClientFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    gigeClientFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    gigeClientFault.mutable_base_fault()->mutable_mark_for_delete()->set_value(true);
    TRACE_FAULT(Chm6GigeClientFault, gigeClientFault, TO_REDIS(),)
    vDelObjects.push_back(gigeClientFault);

    gigeClientPm.mutable_base_pm()->mutable_config_id()->set_value(aid);
    gigeClientPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    gigeClientPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    gigeClientPm.mutable_base_pm()->mutable_mark_for_delete()->set_value(true);
    TRACE_PM(Chm6GigeClientPm, gigeClientPm, TO_REDIS(),)
    vDelObjects.push_back(gigeClientPm);

    gigeClientCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
    TRACE_CONFIG(Chm6GigeClientConfig, *gigeClientCfg, TO_REDIS(),)
    vDelObjects.push_back(*gigeClientCfg);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);

    ) // TRACE
    return (status);
}

void GigeClientConfigHandler::handleTomPresenceMapUpdate(TomPresenceMap* pTomMsg)
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
        INFN_LOG(SeverityLevel::info) << "tomPresenceMap  = 0x" << std::hex << tomPresenceMap << std::dec;
        INFN_LOG(SeverityLevel::info) << "diffPresenceMap = 0x" << std::hex << diffPresenceMap << std::dec;

        // Determine which tom has changed
        uint32 bitMask = 0x1;
        int ret;

        for (uint i = 1; i <= DataPlane::MAX_TOM_ID; i++)
        {
            if (0 != (diffPresenceMap & bitMask))
            {
                if (0 != (tomPresenceMap & bitMask))
                {
                    INFN_LOG(SeverityLevel::info) << "Tom" << i << " changed to present";

                    std::string tpAid = "";
                    std::string gigeAid = mpPbTranslator->numToAid(i, tpAid);

                    bool isTomSerdesTuned = DataPlaneMgrSingleton::Instance()->isTomSerdesTuned(i);
                    if(isCacheExist(tpAid))
                    {
                        // 4x100GE mode
                        string newChildAid = tpAid;
                        for (uint j = 1; j <= 4; j++)
                        {
                            newChildAid.replace(newChildAid.find(".")+1, 1, std::to_string(j));
                            if (isCacheExist(newChildAid))
                            {
                                INFN_LOG(SeverityLevel::info) << "GigeClient cache object present =" << newChildAid;
                                // Perform Serdes?

                                if (false == isTomSerdesTuned)
                                {
                                    INFN_LOG(SeverityLevel::info) << "TuneLinks from tom presence map aid=" << gigeAid;
                                    ret = mpSerdesTuner->TuneLinks(newChildAid, true);
                                    if (0 != ret)
                                    {
                                        INFN_LOG(SeverityLevel::error) << "AID: " << newChildAid << " SERDES (tom-bcm) status = FAIL";
                                    }
                                    else
                                    {
                                        INFN_LOG(SeverityLevel::info) << "Tuned TOM serdes aid=" << newChildAid;
                                        INFN_LOG(SeverityLevel::info) << "AID: " << newChildAid << " SERDES (tom-bcm) status = SUCCESS";
                                        DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(i);
                                    }
                                }
                            }

                        }
                    }
                    else
                    {
                        // non 4x100GE
                        if (isCacheExist(gigeAid))
                        {
                            INFN_LOG(SeverityLevel::info) << "GigeClient cache object present";

                            if (false == isTomSerdesTuned)
                            {
                                INFN_LOG(SeverityLevel::info) << "TuneLinks from tom presence map aid=" << gigeAid;
                                ret = mpSerdesTuner->TuneLinks(gigeAid, true);
                                if (0 != ret)
                                {
                                    INFN_LOG(SeverityLevel::error) << "AID: " << gigeAid << " SERDES (tom-bcm) status = FAIL";
                                }
                                else
                                {
                                    INFN_LOG(SeverityLevel::info) << "Tuned TOM serdes aid=" << gigeAid;
                                    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " SERDES (tom-bcm) status = SUCCESS";
                                    DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(i);
                                }
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
        INFN_LOG(SeverityLevel::info) << "tom_serdes_retune_map.value = " << tomSerdesRetuneMap;
        if (!tomSerdesRetuneMap || !(mTomPresenceMap & tomSerdesRetuneMap))
        {
            INFN_LOG(SeverityLevel::error) << "Invalid Tom serdes retune request - Serdes Tuning failed! returning now";
            return;
        }
        uint32_t tomID = 1;
        // Determine which tom has cold rebooted
        uint32 bitMask = 0x1;
        while ((bitMask != tomSerdesRetuneMap) && (tomID <= DataPlane::MAX_TOM_ID)) {
            INFN_LOG(SeverityLevel::info) << "Bumping tomId = " << tomID << std::hex << " tomSerdesRetuneMap = " << tomSerdesRetuneMap << " bitMask = " << bitMask << std::dec;
            tomSerdesRetuneMap >>= 1;
            tomID++;
        }

        INFN_LOG(SeverityLevel::info) << "TuneLinks for tomID = " << tomID;
        if (tomID <= DataPlane::MAX_TOM_ID)
        {
            std::string tpAid = "";
            std::string gigeAid = mpPbTranslator->numToAid(tomID, tpAid);

            if(isCacheExist(tpAid))
            {
                // 4x100GE mode
                string newChildAid = tpAid;
                for (uint j = 1; j <= 4; j++)
                {
                    newChildAid.replace(newChildAid.find(".")+1, 1, std::to_string(j));
                    if (isCacheExist(newChildAid))
                    {
                        INFN_LOG(SeverityLevel::info) << "GigeClient cache object present =" << newChildAid;

                        INFN_LOG(SeverityLevel::info) << "TuneLinks from tom presence map aid=" << gigeAid;
                        int ret = mpSerdesTuner->TuneLinks(newChildAid, true);
                        if (0 != ret)
                        {
                            INFN_LOG(SeverityLevel::error) << "AID: " << newChildAid << " SERDES (tom-bcm) status = FAIL";
                        }
                        else
                        {
                            INFN_LOG(SeverityLevel::info) << "Tuned TOM serdes aid=" << newChildAid;
                            INFN_LOG(SeverityLevel::info) << "AID: " << newChildAid << " SERDES (tom-bcm) status = SUCCESS";
                            DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(tomID);
                        }
                    }

                }
            }
            else
            {
                // non 4x100GE mode
                if(isCacheExist(gigeAid))
                {
                    INFN_LOG(SeverityLevel::info) << "TuneLinks for gigeAid = " << gigeAid;

                    int ret;
                    INFN_LOG(SeverityLevel::info) << "GigeClient cache object present";
                    // Perform Serdes?

                    INFN_LOG(SeverityLevel::info) << "TuneLinks from tom presence map aid=" << gigeAid;
                    ret = mpSerdesTuner->TuneLinks(gigeAid, true);
                    if (0 != ret)
                    {
                        INFN_LOG(SeverityLevel::error) << "AID: " << gigeAid << " SERDES (tom-bcm) status = FAIL";
                    }
                    else
                    {
                        INFN_LOG(SeverityLevel::info) << "Tuned TOM serdes aid=" << gigeAid;
                        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " SERDES (tom-bcm) status = SUCCESS";
                        DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(tomID);
                    }
                }
            }
        }
    }
}

void GigeClientConfigHandler::handleTomTdaHoldOffMapUpdate(TomPresenceMap* pTomMsg)
{
    string data;
    MessageToJsonString(*pTomMsg, &data);
    INFN_LOG(SeverityLevel::info) << data;

    if (!pTomMsg->tda_holdoff_map().empty())
    {
        for (google::protobuf::Map<string, bool>::const_iterator
                elem  = pTomMsg->tda_holdoff_map().begin();
                elem != pTomMsg->tda_holdoff_map().end(); ++elem)
        {
            std::string aid = elem->first;
            bool enable = elem->second;
            bool updateDco = isCacheExist(aid);

            MaintenanceSignal msMode;
            getChm6GigeClientConfigAutoMsFromCache(aid, DIRECTION_TX, msMode);

            INFN_LOG(SeverityLevel::info) << "Update AID: " << aid << " TDA holdoff mode to " << enable << ", msMode = " << msMode;
            bool ret = mpDcoAd->setTdaHoldOff(aid, enable, updateDco, msMode);
            if (false == ret)
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Set TDA holdoff mode failed";
            }
        }
    }
}

void GigeClientConfigHandler::readTomPresenceMapFromRedis()
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

        if (!presenceMap.tda_holdoff_map().empty())
        {
            for (google::protobuf::Map<string, bool>::const_iterator
                elem  = presenceMap.tda_holdoff_map().begin();
                elem != presenceMap.tda_holdoff_map().end(); ++elem)
            {
                std::string aid = elem->first;
                bool enable = elem->second;
                bool updateDco = isCacheExist(aid);

                MaintenanceSignal msMode;
                getChm6GigeClientConfigAutoMsFromCache(aid, DIRECTION_TX, msMode);

                INFN_LOG(SeverityLevel::info) << "Update AID: " << aid << " TDA holdoff mode to " << enable << ", msMode = " << msMode;
                bool ret = mpDcoAd->setTdaHoldOff(aid, enable, updateDco, msMode);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Set TDA holdoff mode failed";
                }
            }
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

STATUS GigeClientConfigHandler::updateGearBoxGigeClientConfig(Chm6GigeClientConfig* gigeClientCfg, bool onCreate)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    std::string aid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " update";

	int ret;

    bool IsBreakOut = false;

    std::string concAid = aid;
    std::string parentAid = aid;
    std::string fullAid = aid;

    lock_guard<recursive_mutex> lock(mMutex);

    // PortRate and FecEnable settings are required parameters on creation; PortRate is not modifiable

    if (hal_dataplane::PORT_RATE_UNSPECIFIED != gigeClientCfg->hal().rate())
    {
        if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().fec_enable_bool())
        {
            uint rate      = gigeClientCfg->hal().rate();
            bool fecEnable = (wrapper::BOOL_TRUE == gigeClientCfg->hal().fec_enable_bool()) ? true : false;


            size_t Tpos = aid.find("T");
            std::string tmp = aid.substr(Tpos+1);
            size_t pos = tmp.find(".");
            if( pos != std::string::npos)
            {
                int parent = stoi(tmp.substr(0,pos), nullptr);
                int child = stoi(tmp.substr(pos+1));
                int der = 0;
                if (parent % 8 == 1)
                {
                    der = (parent + (child - 1));
                }
                else if (parent % 8 == 0)
                {
                    der = (parent + (child - 4));
                }

                concAid = aid.substr(0,Tpos+1) + std::to_string(der) ;
                parentAid = aid.substr(0,Tpos+1) + std::to_string(parent);
                IsBreakOut = true;

            }

            aid = parentAid;

            gearbox::Bcm81725DataRate_t portRate;


            if( IsBreakOut)
            {
                portRate = gearbox::RATE_4_100GE;
            }
            else
            {
                portRate = (hal_dataplane::PORT_RATE_ETH100G == rate ? gearbox::RATE_100GE : gearbox::RATE_400GE);
            }

            INFN_LOG(SeverityLevel::info) << " AID: " << fullAid << " Rate: " << rate << " FecEnable: " << fecEnable << " portRate: " << portRate;

            ret = mpGearBoxAd->setPortRate(fullAid, portRate, fecEnable);
            if (0 != ret)
            {
                status = STATUS::STATUS_FAIL;
                INFN_LOG(SeverityLevel::error) << "GearBox setPortRate() failed";
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << fullAid << " RETIMER status = " << toString(status);
            }

            int gigeId = mpDcoAd->aidToPortId(aid);

            if (gigeId == -1)
            {
                INFN_LOG(SeverityLevel::error) << "Invalid aid=" << aid;
                return STATUS::STATUS_FAIL;
            }

            INFN_LOG(SeverityLevel::info) << "gigeId=" << gigeId;
            bool isTomSerdesTuned = DataPlaneMgrSingleton::Instance()->isTomSerdesTuned(gigeId);
            INFN_LOG(SeverityLevel::info) << "gigeId=" << gigeId << " isTomSerdesTuned=" << isTomSerdesTuned << " mTomPresenceMap=" << mTomPresenceMap;
            if ( (mTomPresenceMap & (1 << ((uint32_t)(gigeId-1)))) && (false == isTomSerdesTuned) )
            {
                ret = mpSerdesTuner->TuneLinks(fullAid, portRate, onCreate);
                if (0 != ret)
                {
                    // See GX-18817. Don't fail to allow a second chance at programming serdes on BCM to TOM side.
                    // Issue is during bring up, if user removes the TOM after entering tom presence = true, then serdes will fail,
                    //  but we still want to allow a true tom presence map to recover serdes.
                    INFN_LOG(SeverityLevel::error) << "AID: " << aid << " SERDES (tom-bcm) status = FAIL";
                }
                else
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " SERDES (tom-bcm) status = SUCCESS";
                    DataPlaneMgrSingleton::Instance()->setTomSerdesTuned(gigeId);
                }
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << "Skipping tuneLinks aid=" << aid << " rate=" << rate;
            }
        }
        else
        {
            status = STATUS::STATUS_FAIL;
            INFN_LOG(SeverityLevel::error) << "PortRate and FecEnable settings are both required";
        }
    }
    else if (wrapper::BOOL_UNSPECIFIED != gigeClientCfg->hal().fec_enable_bool())
    {
        // FEC Enable
        //     100GE - Apply on GearBox
        //     400GE - Apply on DCO

        bool fecEnable = (wrapper::BOOL_TRUE == gigeClientCfg->hal().fec_enable_bool()) ? true : false;
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FecEnable: " << fecEnable;

        ret = mpGearBoxAd->setPortFecEnable(aid, fecEnable);
        if (0 != ret)
        {
            status = STATUS::STATUS_FAIL;
            INFN_LOG(SeverityLevel::error) << "GearBox setPortFecEnable() failed";
        }
    }

    if (STATUS::STATUS_SUCCESS != status)
    {
        INFN_LOG(SeverityLevel::error) << "updateGigeClient on GearBox failed";
    }

    return (status);
}

void GigeClientConfigHandler::sendTransactionStateToInProgress(Chm6GigeClientConfig* gigeClientCfg)
{
    static const std::string status = "in-progress";
    static const std::string statusKey = "config.transaction_status";
    static const std::string operation = "GigeClientConfigHandler/sendTransactionState";

    TRACE(
    std::string aid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction in-progress state";

    Chm6GigeClientState* pState = new Chm6GigeClientState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(gigeClientCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(gigeClientCfg->base_config().timestamp().nanos());
    pState->mutable_base_state()->mutable_trace_context_ls()->CopyFrom(
        gigeClientCfg->base_config().trace_context_ls());

    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_INPROGRESS);

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    TRACE_TAG(statusKey, status);
    TRACE_TAG("message.aid", aid);
    TRACE_TAG("message.name", gigeClientCfg->GetDescriptor()->full_name());

    delete pState;
    )
}

void GigeClientConfigHandler::sendTransactionStateToComplete(Chm6GigeClientConfig* gigeClientCfg,
                                                             STATUS status)
{
    static const std::string tStatus = "complete";
    static const std::string stateKey = "config.transaction_state";
    static const std::string statusKey = "config.transaction_status";
    static const std::string operation = "GigeClientConfigHandler/SendTransactionState";

    TRACE(
    std::string aid = gigeClientCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction complete state " << toString(status);

    Chm6GigeClientState stateObj;

    stateObj.mutable_base_state()->mutable_config_id()->set_value(aid);
    stateObj.mutable_base_state()->mutable_timestamp()->set_seconds(gigeClientCfg->base_config().timestamp().seconds());
    stateObj.mutable_base_state()->mutable_timestamp()->set_nanos(gigeClientCfg->base_config().timestamp().nanos());
    stateObj.mutable_base_state()->mutable_trace_context_ls()->CopyFrom(
        gigeClientCfg->base_config().trace_context_ls());

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
    TRACE_TAG("message.name", gigeClientCfg->GetDescriptor()->full_name());
    )
}

void GigeClientConfigHandler::createCacheObj(chm6_dataplane::Chm6GigeClientConfig& objCfg)
{
    chm6_dataplane::Chm6GigeClientConfig* pObjCfg = new chm6_dataplane::Chm6GigeClientConfig(objCfg);

    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    addCacheObj(configId, pObjCfg);
}

void GigeClientConfigHandler::updateCacheObj(chm6_dataplane::Chm6GigeClientConfig& objCfg)
{
    chm6_dataplane::Chm6GigeClientConfig* pObjCfg;
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
            pObjCfg = (chm6_dataplane::Chm6GigeClientConfig*)pMsg;

            pObjCfg->MergeFrom(objCfg);
        }
    }
}

void GigeClientConfigHandler::deleteCacheObj(chm6_dataplane::Chm6GigeClientConfig& objCfg)
{
    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    clearCache(configId);
}

STATUS GigeClientConfigHandler::sendConfig(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6GigeClientConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6GigeClientConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    // todo: is it ok to always call create here??
    return (createGigeClientConfig(pCfgMsg) == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);
}

STATUS GigeClientConfigHandler::sendDelete(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6GigeClientConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6GigeClientConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    return (deleteGigeClientConfig(pCfgMsg) == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);
}

bool GigeClientConfigHandler::isMarkForDelete(const google::protobuf::Message& msg)
{
    const chm6_dataplane::Chm6GigeClientConfig* pMsg = static_cast<const chm6_dataplane::Chm6GigeClientConfig*>(&msg);

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

void GigeClientConfigHandler::dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6GigeClientConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6GigeClientConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    string jsonStr;
    MessageToJsonString(*pCfgMsg, &jsonStr);

    out << "** Dumping GigeClient ConfigId: " << configId << endl;
    mpPbTranslator->dumpJsonFormat(jsonStr, out);
    out << endl << endl;
}

void GigeClientConfigHandler::getChm6GigeClientConfigForceMsFromCache(std::string aid, Direction dir, MaintenanceSignal& msMode)
{
    chm6_dataplane::Chm6GigeClientConfig* pObjCfg;
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
            pObjCfg = (chm6_dataplane::Chm6GigeClientConfig*)pMsg;

            DpAdapter::GigeCommon adGigeCfg;
            mpPbTranslator->configPbToAd(*pObjCfg, adGigeCfg);

            msMode = (DIRECTION_TX == dir) ? adGigeCfg.forceMsTx : adGigeCfg.forceMsRx;
        }
    }
}

void GigeClientConfigHandler::getChm6GigeClientConfigAutoMsFromCache(std::string aid, Direction dir, MaintenanceSignal& msMode)
{
    chm6_dataplane::Chm6GigeClientConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    msMode = DataPlane::MAINTENANCE_SIGNAL_IDLE;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(aid, pMsg))
    {
        INFN_LOG(SeverityLevel::error) << "Cached Object not found for AID: " << aid;
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_dataplane::Chm6GigeClientConfig*)pMsg;

            DpAdapter::GigeCommon adGigeCfg;
            mpPbTranslator->configPbToAd(*pObjCfg, adGigeCfg);

            msMode = (DIRECTION_TX == dir) ? adGigeCfg.autoMsTx : adGigeCfg.autoMsRx;
        }
    }
}

void GigeClientConfigHandler::getChm6GigeClientConfigTestSigGenFromCache(std::string aid, Direction dir, bool& testSigGen)
{
    chm6_dataplane::Chm6GigeClientConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    testSigGen = false;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(aid, pMsg))
    {
        INFN_LOG(SeverityLevel::error) << "Cached Object not found for AID: " << aid;
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_dataplane::Chm6GigeClientConfig*)pMsg;

            DpAdapter::GigeCommon adGigeCfg;
            mpPbTranslator->configPbToAd(*pObjCfg, adGigeCfg);

            testSigGen = (DIRECTION_TX == dir) ? adGigeCfg.generatorTx : adGigeCfg.generatorRx;
        }
    }
}

void GigeClientConfigHandler::getChm6GigeClientConfigTestSigMonFromCache(std::string aid, bool& testSigMon)
{
    chm6_dataplane::Chm6GigeClientConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    testSigMon = false;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(aid, pMsg))
    {
        INFN_LOG(SeverityLevel::debug) << "Cached Object not found for AID: " << aid;
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_dataplane::Chm6GigeClientConfig*)pMsg;
            if (wrapper::BOOL_UNSPECIFIED != pObjCfg->hal().test_signal_mon())
            {
                testSigMon = (pObjCfg->hal().test_signal_mon() == wrapper::BOOL_TRUE) ? true : false;
            }
        }
    }
}

} // namespace DataPlane
