/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "CarrierConfigHandler.h"
#include "DataPlaneMgr.h"
#include "DpCardMgr.h"
#include "DcoCardAdapter.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"
#include "DpPeerDiscoveryMgr.h"
#include "DpGccMgr.h"
#include "GccConfigHandler.h"

#include <iostream>
#include <string>
#include <sstream>

using chm6_dataplane::Chm6CarrierConfig;
using chm6_dataplane::Chm6CarrierState;
using chm6_dataplane::Chm6CarrierFault;
using chm6_dataplane::Chm6CarrierPm;
using chm6_dataplane::Chm6ScgConfig;
using chm6_dataplane::Chm6ScgState;
using chm6_dataplane::Chm6ScgFault;
using chm6_dataplane::Chm6ScgPm;
using chm6_pcp::VlanConfig;

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace ::std;

const uint32_t CHM6_LOCAL_VLANID_OFEC_CC_BC_L1 = (3100);

namespace DataPlane {

CarrierConfigHandler::CarrierConfigHandler(CarrierPbTranslator *pbTrans,
                                           DpAdapter::DcoCarrierAdapter *pCarAd,
                                           std::shared_ptr<DcoSecProcCarrierCfgHdlr> spSecProcHndlr)
    : mpPbTranslator(pbTrans)
    , mpDcoCarrierAd(pCarAd)
    , mspSecProcCfgHndlr(spSecProcHndlr)
{
    INFN_LOG(SeverityLevel::info) << "Constructor";

    assert(pbTrans != NULL);
    assert(pCarAd  != NULL);
    assert(mspSecProcCfgHndlr != nullptr);
}

ConfigStatus CarrierConfigHandler::onCreate(Chm6CarrierConfig* carrierCfg)
{
    std::string carAid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << carAid;

    string cfgData;
    MessageToJsonString(*carrierCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << cfgData;

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);
    if (carId < 1 || carId > DataPlane::MAX_CARRIER_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << carAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " status = " << statToString(status);
        return (status);
    }

    ConfigStatus status = createCarrierConfig(carrierCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Carrier create on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " status = " << statToString(status);

    VlanConfig vlanCfg;
    std::string vlanAid = carAid + "-BC";
    vlanCfg.mutable_base_config()->mutable_config_id()->set_value(vlanAid);
    vlanCfg.mutable_hal()->mutable_vlanid()->set_value(CHM6_LOCAL_VLANID_OFEC_CC_BC_L1 + carId);
    vlanCfg.mutable_hal()->set_apptype(PcpAppType::APP_TYPE_OFEC_CC_BC);
    vlanCfg.mutable_hal()->mutable_normalized_vlanid()->set_value(NORMALIZED_VLANID_OFEC_CC_BC);
    vlanCfg.mutable_hal()->mutable_normalized_vlan_tx_carrier()->set_value((uint32_t)carId);

    DpGccMgrSingleton::Instance()->getCfgHndlrPtr()->createPcpVlanConfig(&vlanCfg);

    ConfigStatus vlanStatus = DpGccMgrSingleton::Instance()->getCfgHndlrPtr()->createCcVlanConfig(&vlanCfg) ? ConfigStatus::SUCCESS : ConfigStatus::FAIL_CONNECT;
    INFN_LOG(SeverityLevel::info) << "VLAN AID: " << vlanAid << " status = " << statToString(vlanStatus);

    return (status);
}

ConfigStatus CarrierConfigHandler::onModify(Chm6CarrierConfig* carrierCfg)
{
    string carAid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << carAid;

    string cfgData;
    MessageToJsonString(*carrierCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << cfgData;

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);
    if (carId < 1 || carId > DataPlane::MAX_CARRIER_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << carAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " status = " << statToString(status);
        return (status);
    }

    ConfigStatus status = updateCarrierConfig(carrierCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Carrier update on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus CarrierConfigHandler::onDelete(Chm6CarrierConfig* carrierCfg)
{
    std::string carAid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << carAid;

    std::string cfgDataStr;
    MessageToJsonString(*carrierCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);
    if (carId < 1 || carId > DataPlane::MAX_CARRIER_ID)
    {
        ConfigStatus status = ConfigStatus::FAIL_DATA;

        std::ostringstream log;
        log << "Invalid AID " << carAid;
        setTransactionErrorString(log.str());

        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " status = " << statToString(status);
        return (status);
    }

    ConfigStatus status = deleteCarrierConfig(carrierCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Carrier delete on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " status = " << statToString(status);

    VlanConfig vlanCfg;
    std::string vlanAid = carAid + "-BC";
    vlanCfg.mutable_base_config()->mutable_config_id()->set_value(vlanAid);
    vlanCfg.mutable_hal()->mutable_vlanid()->set_value(CHM6_LOCAL_VLANID_OFEC_CC_BC_L1 + carId);
    vlanCfg.mutable_hal()->set_apptype(PcpAppType::APP_TYPE_OFEC_CC_BC);
    vlanCfg.mutable_hal()->mutable_normalized_vlanid()->set_value(NORMALIZED_VLANID_OFEC_CC_BC);
    vlanCfg.mutable_hal()->mutable_normalized_vlan_tx_carrier()->set_value((uint32_t)carId);

    DpGccMgrSingleton::Instance()->getCfgHndlrPtr()->deletePcpVlanConfig(&vlanCfg);

    ConfigStatus vlanStatus = DpGccMgrSingleton::Instance()->getCfgHndlrPtr()->deleteCcVlanConfig(&vlanCfg) ? ConfigStatus::SUCCESS : ConfigStatus::FAIL_CONNECT;
    INFN_LOG(SeverityLevel::info) << "VLAN AID: " << vlanAid << " status = " << statToString(vlanStatus);

    return (status);
}

ConfigStatus CarrierConfigHandler::onCreate(Chm6ScgConfig* scgCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string scgAid = scgCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "SCG AID: " << scgAid;

    string cfgData;
    MessageToJsonString(*scgCfg, &cfgData);
    INFN_LOG(SeverityLevel::info) << cfgData;

    if (wrapper::BOOL_UNSPECIFIED != scgCfg->hal().supported_facilities())
    {
        if (wrapper::BOOL_FALSE == scgCfg->hal().supported_facilities())
        {
            INFN_LOG(SeverityLevel::info) << "createScg supported facilities not present";

            // Create a default carrier object for reporting faults
            std::string carAid = mpPbTranslator->aidScgToCar(scgAid);

            DpAdapter::CarrierCommon adCarCfg;
            memset(&adCarCfg, 0, sizeof(adCarCfg));

            adCarCfg.capacity = 400;
            adCarCfg.baudRate = 95.6390657;
            adCarCfg.appCode = "P";
            adCarCfg.enable = false;
            // Set the
            // provisioned flag as way to indicate to DCO if the SuperChannel
            // has been created = true or deleted = false.
            // In this case we there is no super channel therefore
            // the provisiond flag must be set to false.
            // We only create default carrier to get SCG data.
            adCarCfg.provisioned = false;

            bool ret = mpDcoCarrierAd->createCarrier(carAid, &adCarCfg);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;
                INFN_LOG(SeverityLevel::info) << "createScg on DCO failed";
            }
            // After we create carrier default object for SCG monitor
            // We need to remove the carrier config leaf in sysrepo.
            // to allow sysrepo to detect the "real" configuration changes
            ret = mpDcoCarrierAd->deleteCarrierConfigAttributes(carAid);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;
                INFN_LOG(SeverityLevel::info) << "delete carrier config on DCO failed";
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "createScg supported facilities present";
        }
    }
    else
    {
        status = ConfigStatus::FAIL_DATA;
        INFN_LOG(SeverityLevel::info) << "createScg missing supported facilities attribute";
    }

    INFN_LOG(SeverityLevel::info) << "SCG AID: " << scgAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus CarrierConfigHandler::createCarrierConfig(Chm6CarrierConfig* carrierCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " create";

    DpAdapter::CarrierCommon adCarCfg;
    mpPbTranslator->configPbToAd(*carrierCfg, adCarCfg);

    DpAdapter::CardConfig adCardCfg;
    mpPbTranslator->configPbToAd(*carrierCfg, adCardCfg);
    ClientMode curClientMode = DpCardMgrSingleton::Instance()->getAdPtr()->getCurClientMode();

    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Config Client Mode: " << adCardCfg.mode << " Current Client Mode: " << curClientMode;

    // If client mode has changed, skip configure carrier as we are going
    // to cold boot DCO anyway.  Optimized for DCO performance.
    bool ret = true;

    if ( adCardCfg.mode == curClientMode || curClientMode == CLIENTMODE_UNSET )
    {
        // Set the
        // provisioned flag as way to indicate to DCO if the SuperChannel
        // has been created = true or deleted = false.
        // IF we get here, a super-channel has been created, set to true
        adCarCfg.provisioned = true;
        ret = mpDcoCarrierAd->createCarrier(aid, &adCarCfg);
        if (false == ret)
        {
            // todo: distinguish failure types from adapter layer
            status = ConfigStatus::FAIL_CONNECT;

            INFN_LOG(SeverityLevel::info) << "createCarrier on DCO failed";
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Client Mode Changed, Skip Carrier create  ";
    }

    if ( status == ConfigStatus::SUCCESS )
    {
        ret = DpCardMgrSingleton::Instance()->getAdPtr()->setClientMode(adCardCfg.mode);
    }

    if (false == ret)
    {
        // todo: distinguish failure types from adapter layer
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "setClientMode on DCO Card failed";
    }

    toggleSyncReadyInSimMode();
    return (status);
}

ConfigStatus CarrierConfigHandler::updateCarrierConfig(Chm6CarrierConfig* carrierCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " modify";

    bool ret;

    DpAdapter::CarrierCommon adCarCfg;

    mpPbTranslator->configPbToAd(*carrierCfg, adCarCfg);

    // Frequency
    if (carrierCfg->hal().has_frequency())
    {
        // Update Frequency
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Frequency: " << adCarCfg.frequency;

        ret = mpDcoCarrierAd->setCarrierFrequency(aid, adCarCfg.frequency);
        if (false == ret)
        {
            // todo: distinguish failure types from adapter layer
            status = ConfigStatus::FAIL_CONNECT;

            INFN_LOG(SeverityLevel::error) << "setCarrierFrequency() failed";
        }
    }

    // FrequencyOffset
    if (carrierCfg->hal().has_frequency_offset())
    {
        // Update Frequency Offset
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Frequency Offset: " << adCarCfg.freqOffset;

        ret = mpDcoCarrierAd->setCarrierFreqOffset(aid, adCarCfg.freqOffset);
        if (false == ret)
        {
            // todo: distinguish failure types from adapter layer
            status = ConfigStatus::FAIL_CONNECT;

            INFN_LOG(SeverityLevel::error) << "setCarrierFreqOffset() failed";
        }
    }

    // TargetPower
    if (carrierCfg->hal().has_target_power())
    {
        // Update Target Power
        INFN_LOG(SeverityLevel::info)"AID: " << aid << " Target Power: " << adCarCfg.targetPower ;

        ret = mpDcoCarrierAd->setCarrierTargetPower(aid, adCarCfg.targetPower);
        if (false == ret)
        {
            // todo: distinguish failure types from adapter layer
            status = ConfigStatus::FAIL_CONNECT;

            INFN_LOG(SeverityLevel::error)"setCarrierTargetPower() failed";
        }
    }

    if (carrierCfg->hal().has_tx_cd_pre_comp())
    {
        // Update Tx Chromatic Dispersion Pre-Compensation
        INFN_LOG(SeverityLevel::info)"AID: " << aid << " Tx CD Pre-Compensation: " << adCarCfg.txCdPreComp ;

        ret = mpDcoCarrierAd->setCarrierTxCdPreComp(aid, adCarCfg.txCdPreComp);
        if (false == ret)
        {
            // todo: distinguish failure types from adapter layer
            status = ConfigStatus::FAIL_CONNECT;

            INFN_LOG(SeverityLevel::error)"setCarrierTxCdPreComp() failed";
        }
    }

    // Mode
    if (carrierCfg->hal().has_mode())
    {
        // Update Mode
        if (hal_common::CLIENT_MODE_UNSPECIFIED != carrierCfg->hal().mode().client())
        {
            DpAdapter::CardConfig adCardCfg;
            mpPbTranslator->configPbToAd(*carrierCfg, adCardCfg);

            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Client Mode: " << adCardCfg.mode;

            ret = DpCardMgrSingleton::Instance()->getAdPtr()->setClientMode(adCardCfg.mode);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error) << "setClientMode() on DCO Card failed";
            }
        }

        // App Code
        if (carrierCfg->hal().mode().has_application_code())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Mode App Code : " << adCarCfg.appCode;

            ret = mpDcoCarrierAd->setCarrierAppCode(aid, adCarCfg.appCode);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error)"setCarrierAppCode() failed";
            }
        }

        // Baud Rate
        if (carrierCfg->hal().mode().has_baud())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Mode Baud Rate: " << adCarCfg.baudRate;

            ret = mpDcoCarrierAd->setCarrierBaudRate(aid, adCarCfg.baudRate);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error)"setCarrierBaudRate() failed";
            }
        }

        // Capacity
        if (carrierCfg->hal().mode().has_capacity())
        {
            INFN_LOG(SeverityLevel::info)"AID: " << aid << " Mode Capacity : " << adCarCfg.capacity;

            ret = mpDcoCarrierAd->setCarrierCapacity(aid, adCarCfg.capacity);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error)"setCarrierCapacity() failed";
            }
        }
    }

    // Advanced Parameter
    if (carrierCfg->hal().advanced_param().advanced_param_size() != 0)
    {
        for (uint index = 0; index < carrierCfg->hal().advanced_param().advanced_param_size(); index++)
        {
            INFN_LOG(SeverityLevel::info)"AID: " << aid << " Advanced Parameter : " << adCarCfg.ap[index].apName << ":" << adCarCfg.ap[index].apValue;
        }

        ret = mpDcoCarrierAd->setCarrierAdvParam(aid, adCarCfg.ap);
        if (false == ret)
        {
            // todo: distinguish failure types from adapter layer
            status = ConfigStatus::FAIL_CONNECT;

            INFN_LOG(SeverityLevel::error) << "setCarrierAdvParam() failed";
        }
    }

    if (carrierCfg->hal().has_alarm_threshold())
    {
        // DGD OORH Threshold
        if (carrierCfg->hal().alarm_threshold().has_dgd_oorh_threshold())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " DGD-OORH Threshold: " << adCarCfg.dgdOorhThresh;

            ret = mpDcoCarrierAd->setCarrierDgdOorhThresh(aid, adCarCfg.dgdOorhThresh);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error) << "setCarrierDgdOorhThresh() failed";
            }
        }

        // PostFecQ Signal Degrade Threshold
        if (carrierCfg->hal().alarm_threshold().has_post_fec_q_signal_degrade_threshold())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " POST-FEC-Q-SIGNAL-DEGRADE Threshold: " << adCarCfg.postFecQSdThresh;

            ret = mpDcoCarrierAd->setCarrierPostFecQSdThresh(aid, adCarCfg.postFecQSdThresh);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error) << "setCarrierPostFecQSdThresh() failed";
            }
        }

        // PostFecQ Signal Degrade Hysteresis
        if (carrierCfg->hal().alarm_threshold().has_post_fec_q_signal_degrade_hysteresis())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " POST-FEC-Q-SIGNAL-DEGRADE Hysteresis: " << adCarCfg.postFecQSdHysteresis;

            ret = mpDcoCarrierAd->setCarrierPostFecQSdHyst(aid, adCarCfg.postFecQSdHysteresis);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error) << "setCarrierPostFecQSdHyst() failed";
            }
        }

        // PreFecQ Signal Degrade Threshold
        if (carrierCfg->hal().alarm_threshold().has_pre_fec_q_signal_degrade_threshold())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " PRE-FEC-Q-SIGNAL-DEGRADE Threshold: " << adCarCfg.preFecQSdThresh;

            ret = mpDcoCarrierAd->setCarrierPreFecQSdThresh(aid, adCarCfg.preFecQSdThresh);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error) << "setCarrierPreFecQSdThresh() failed";
            }
        }

        // PreFecQ Signal Degrade Hysteresis
        if (carrierCfg->hal().alarm_threshold().has_pre_fec_q_signal_degrade_hysteresis())
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " PRE-FEC-Q-SIGNAL-DEGRADE Hysteresis: " << adCarCfg.preFecQSdHysteresis;

            ret = mpDcoCarrierAd->setCarrierPreFecQSdHyst(aid, adCarCfg.preFecQSdHysteresis);
            if (false == ret)
            {
                // todo: distinguish failure types from adapter layer
                status = ConfigStatus::FAIL_CONNECT;

                INFN_LOG(SeverityLevel::error) << "setCarrierPreFecQSdHyst() failed";
            }
        }
    }

    // Enable
    if (wrapper::BOOL_UNSPECIFIED != carrierCfg->hal().enable_bool())
    {
        // Update Enable
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Enable: " << adCarCfg.enable;

        ret = mpDcoCarrierAd->setCarrierEnable(aid, adCarCfg.enable);
        if (false == ret)
        {
            // todo: distinguish failure types from adapter layer
            status = ConfigStatus::FAIL_CONNECT;

            INFN_LOG(SeverityLevel::error) << "setCarrierEnable() failed";
        }
    }

    return (status);
}

ConfigStatus CarrierConfigHandler::deleteCarrierConfig(Chm6CarrierConfig* carrierCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info)"AID: " << aid << " delete";

    // Update DCO Adapter - Keep carrier always present in DCO
    bool ret = mpDcoCarrierAd->setCarrierEnable(aid, false);
    if (false == ret)
    {
        // todo: distinguish failure types from adapter layer
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error)"disableCarrier on DCO failed";
    }

    // Since we do not delete carrier once it is created.  Set the
    // provisioned flag as way to indicate to DCO if the SuperChannel has
    // been created = true or deleted = false.
    ret = mpDcoCarrierAd->setCarrierProvisioned(aid, false);
    if (false == ret)
    {
        // todo: distinguish failure types from adapter layer
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error)"Set Provisioned flag on Carrier DCO failed";
    }

    // CLean up APJ on carrier deletion.  Delete APJ after carrier disable
    // to save DCO some uncesscary processing time.
    ret = mpDcoCarrierAd->deleteCarrierAdvParamAll(aid);
    if (false == ret)
    {
        // todo: distinguish failure types from adapter layer
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error)"Delete All APJ Carrier on DCO failed";
    }
    // We need to remove the carrier config leaf in sysrepo.
    // to allow sysrepo to detect the "real" configuration changes
    ret = mpDcoCarrierAd->deleteCarrierConfigAttributes(aid);
    if (false == ret)
    {
        // todo: distinguish failure types from adapter layer
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::info) << "delete carrier config on DCO failed";
    }

    return (status);
}

STATUS CarrierConfigHandler::deleteCarrierConfigCleanup(Chm6CarrierConfig* carrierCfg)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    std::string aid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info)"AID: " << aid << " delete";

    Chm6CarrierState carrierState;
    Chm6CarrierFault carrierFault;
    Chm6CarrierPm    carrierPm;
    Chm6ScgFault     scgFault;
    Chm6ScgPm        scgPm;

    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    carrierState.mutable_base_state()->mutable_config_id()->set_value(aid);
    carrierState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    carrierState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    carrierState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(carrierState);

    carrierFault.mutable_base_fault()->mutable_config_id()->set_value(aid);
    carrierFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    carrierFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    carrierFault.mutable_base_fault()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(carrierFault);

    carrierPm.mutable_base_pm()->mutable_config_id()->set_value(aid);
    carrierPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    carrierPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    carrierPm.mutable_base_pm()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(carrierPm);

    carrierCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(*carrierCfg);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);

    return (status);
}

void CarrierConfigHandler::sendTransactionStateToInProgress(Chm6CarrierConfig* carrierCfg)
{
    std::string aid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction in-progress state";

    Chm6CarrierState* pState = new Chm6CarrierState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(carrierCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(carrierCfg->base_config().timestamp().nanos());

    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_INPROGRESS);

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    delete pState;
}

void CarrierConfigHandler::sendTransactionStateToComplete(Chm6CarrierConfig* carrierCfg,
                                                          STATUS status)
{
    std::string aid = carrierCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction complete state " << toString(status);

    Chm6CarrierState stateObj;

    stateObj.mutable_base_state()->mutable_config_id()->set_value(aid);
    stateObj.mutable_base_state()->mutable_timestamp()->set_seconds(carrierCfg->base_config().timestamp().seconds());
    stateObj.mutable_base_state()->mutable_timestamp()->set_nanos(carrierCfg->base_config().timestamp().nanos());

    stateObj.mutable_base_state()->set_transaction_state(chm6_common::STATE_COMPLETE);
    stateObj.mutable_base_state()->set_transaction_status(status);

    stateObj.mutable_base_state()->mutable_transaction_info()->set_value(getTransactionErrorString());

    string stateDataStr;
    MessageToJsonString(stateObj, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(stateObj);

    setTransactionErrorString("");
}

void CarrierConfigHandler::sendTransactionStateToInProgress(Chm6ScgConfig* scgCfg)
{
    std::string aid = scgCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction in-progress state";

    Chm6ScgState* pState = new Chm6ScgState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(scgCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(scgCfg->base_config().timestamp().nanos());

    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_INPROGRESS);

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    delete pState;
}

void CarrierConfigHandler::sendTransactionStateToComplete(Chm6ScgConfig* scgCfg, STATUS status)
{
    std::string aid = scgCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction complete state " << toString(status);

    Chm6ScgState* pState = new Chm6ScgState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(scgCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(scgCfg->base_config().timestamp().nanos());

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

void CarrierConfigHandler::createCacheObj(chm6_dataplane::Chm6CarrierConfig& carrierMsg)
{
    chm6_dataplane::Chm6CarrierConfig* pCarMsg = new chm6_dataplane::Chm6CarrierConfig(carrierMsg);

    std::string configId = carrierMsg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    addCacheObj(configId, pCarMsg);
}

void CarrierConfigHandler::updateCacheObj(chm6_dataplane::Chm6CarrierConfig& carrierMsg)
{
    chm6_dataplane::Chm6CarrierConfig* pCarMsg;
    google::protobuf::Message* pMsg;

    std::string configId = carrierMsg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(configId, pMsg))
    {
        INFN_LOG(SeverityLevel::info) << "Cached Object not found for configId: " << configId;

        // todo: what to do here??? This should not happen
        createCacheObj(carrierMsg);
    }
    else
    {
        if (pMsg)
        {
            pCarMsg = (chm6_dataplane::Chm6CarrierConfig*)pMsg;

            mpPbTranslator->mergeObj(*pCarMsg, carrierMsg);
        }
    }
}

void CarrierConfigHandler::deleteCacheObj(chm6_dataplane::Chm6CarrierConfig& carrierMsg)
{
    std::string configId = carrierMsg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    clearCache(configId);
}

STATUS CarrierConfigHandler::sendConfig(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6CarrierConfig* pCarMsg = static_cast<chm6_dataplane::Chm6CarrierConfig*>(&msg);

    std::string configId = pCarMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    // todo: check status for SecProc config update?
    ConfigStatus secProcStatus = mspSecProcCfgHndlr->onCreate(pCarMsg);

    if(secProcStatus == ConfigStatus::SUCCESS)
    {
        DpPeerDiscoveryMgrSingleton::Instance()->NotifyCarrierCreate(configId);
    }

    // todo: is it ok to always call create here??
    ConfigStatus status = createCarrierConfig(pCarMsg);

    if( !((status == ConfigStatus::SUCCESS) && (secProcStatus == ConfigStatus::SUCCESS)) )
        return STATUS::STATUS_FAIL;

    VlanConfig vlanCfg;
    std::string vlanAid = configId + "-BC";
    int carId = mpDcoCarrierAd->aidToCarrierId(configId);
    vlanCfg.mutable_base_config()->mutable_config_id()->set_value(vlanAid);
    vlanCfg.mutable_hal()->mutable_vlanid()->set_value(CHM6_LOCAL_VLANID_OFEC_CC_BC_L1 + carId);
    vlanCfg.mutable_hal()->set_apptype(PcpAppType::APP_TYPE_OFEC_CC_BC);
    vlanCfg.mutable_hal()->mutable_normalized_vlanid()->set_value(NORMALIZED_VLANID_OFEC_CC_BC);
    vlanCfg.mutable_hal()->mutable_normalized_vlan_tx_carrier()->set_value((uint32_t)carId);

    DpGccMgrSingleton::Instance()->getCfgHndlrPtr()->createPcpVlanConfig(&vlanCfg);

    ConfigStatus vlanStatus = DpGccMgrSingleton::Instance()->getCfgHndlrPtr()->createCcVlanConfig(&vlanCfg) ? ConfigStatus::SUCCESS : ConfigStatus::FAIL_CONNECT;
    INFN_LOG(SeverityLevel::info) << "VLAN AID: " << vlanAid << " status = " << statToString(vlanStatus);

    return STATUS::STATUS_SUCCESS;

}

// Intended for case of delete from onResync callbacks
STATUS CarrierConfigHandler::sendDelete(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6CarrierConfig* pCarMsg = static_cast<chm6_dataplane::Chm6CarrierConfig*>(&msg);

    std::string configId = pCarMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    // todo: check status for SecProc config update?
    ConfigStatus secProcStatus = mspSecProcCfgHndlr->onDelete(pCarMsg);

    if(secProcStatus == ConfigStatus::SUCCESS)
    {
        DpPeerDiscoveryMgrSingleton::Instance()->NotifyCarrierDelete(configId);
    }

    ConfigStatus status = deleteCarrierConfig(pCarMsg);

    if( !((status == ConfigStatus::SUCCESS) && (secProcStatus == ConfigStatus::SUCCESS)) )
        return STATUS::STATUS_FAIL;

    return STATUS::STATUS_SUCCESS;

}

bool CarrierConfigHandler::isMarkForDelete(const google::protobuf::Message& msg)
{
    const chm6_dataplane::Chm6CarrierConfig* pCarMsg = static_cast<const chm6_dataplane::Chm6CarrierConfig*>(&msg);

    std::string configId = pCarMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    bool retVal = false;

    if (pCarMsg->base_config().has_mark_for_delete())
    {
        if (pCarMsg->base_config().mark_for_delete().value())
        {
            retVal = true;
        }
    }

    return retVal;
}


void CarrierConfigHandler::dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6CarrierConfig* pCarMsg = static_cast<chm6_dataplane::Chm6CarrierConfig*>(&msg);

    std::string configId = pCarMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    string jsonStr;
    MessageToJsonString(*pCarMsg, &jsonStr);

    out << "** Dumping Carrier ConfigId: " << configId << endl;
    mpPbTranslator->dumpJsonFormat(jsonStr, out);
    out << endl << endl;
}

void CarrierConfigHandler::toggleSyncReadyInSimMode()
{
    // Toggle the syncReady flag for L1ServiceAgent in SIM mode to mimic DCO cold boot
    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        string stateDataStr;

        struct timeval tv;
        gettimeofday(&tv, NULL);

        chm6_common::Chm6DcoCardState cardStatePb;
        cardStatePb.mutable_base_state()->mutable_config_id()->set_value("Chm6Internal");
        cardStatePb.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        cardStatePb.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
        cardStatePb.set_sync_ready(wrapper::BOOL_FALSE);

        MessageToJsonString(cardStatePb, &stateDataStr);
        INFN_LOG(SeverityLevel::info) << stateDataStr;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(cardStatePb);

        sleep(5);

        cardStatePb.set_sync_ready(wrapper::BOOL_TRUE);

        MessageToJsonString(cardStatePb, &stateDataStr);
        INFN_LOG(SeverityLevel::info) << stateDataStr;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(cardStatePb);
    }
}

void CarrierConfigHandler::getChm6CarrierConfigFreqFromCache(std::string aid, double& frequency)
{
    chm6_dataplane::Chm6CarrierConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    frequency = 0;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(aid, pMsg))
    {
        INFN_LOG(SeverityLevel::debug) << "Cached Object not found for AID: " << aid;
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_dataplane::Chm6CarrierConfig*)pMsg;
            if (pObjCfg->hal().has_frequency())
            {
                frequency = pObjCfg->hal().frequency().value();
            }
        }
    }
}

} // namespace DataPlane
