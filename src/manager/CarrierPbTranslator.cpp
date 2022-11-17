/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include <iostream>
#include <sstream>

#include "DataPlaneMgr.h"
#include "DpCarrierMgr.h"
#include "CarrierPbTranslator.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"

using chm6_dataplane::Chm6CarrierState;
using chm6_dataplane::Chm6CarrierConfig;
using chm6_dataplane::Chm6CarrierFault;
using chm6_dataplane::Chm6CarrierPm;
using chm6_dataplane::Chm6ScgFault;
using chm6_dataplane::Chm6ScgPm;

using namespace ::std;


namespace DataPlane {

void CarrierPbTranslator::configPbToAd(const Chm6CarrierConfig& pbCfg, DpAdapter::CarrierCommon& adCfg) const
{
    std::string carAid = pbCfg.base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << carAid;

    // Enable
    if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().enable_bool())
    {
        wrapper::Bool enable = pbCfg.hal().enable_bool();
        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Enable: " << toString(enable);
        switch (enable)
        {
            case wrapper::BOOL_TRUE:
                adCfg.enable = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.enable = false;
                break;
            default:
                break;
        }
    }

    // Frequency
    if (pbCfg.hal().has_frequency())
    {
        // Update Frequency
        double carFreq = pbCfg.hal().frequency().value();
        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Frequency: " << carFreq;

        // todo: confirm units are the same ???
        adCfg.frequency = carFreq;
    }

    // FrequencyOffset
    if (pbCfg.hal().has_frequency_offset())
    {
        double freqOff = pbCfg.hal().frequency_offset().value();
        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Frequency Offset: " << freqOff;

        // todo: confirm units are the same ???
        adCfg.freqOffset = freqOff;
    }

    // TargetPower
    if (pbCfg.hal().has_target_power())
    {
        // Update Target Power
        double tarPow = pbCfg.hal().target_power().value();
        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Target Power: " << tarPow;

        adCfg.targetPower = tarPow;
    }

    // TxCdPreComp
    if (pbCfg.hal().has_tx_cd_pre_comp())
    {
        // Update Tx Chromatic Dispersion Pre-Compensation
        double txCdPreComp = pbCfg.hal().tx_cd_pre_comp().value();
        INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Tx CD Pre-Compensation: " << txCdPreComp;

        adCfg.txCdPreComp = txCdPreComp;
    }

    // Mode
    if (pbCfg.hal().has_mode())
    {
        // App Code
        if (pbCfg.hal().mode().has_application_code())
        {
            std::string appCode = pbCfg.hal().mode().application_code().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Mode App Code : " << appCode;

            adCfg.appCode = appCode;
        }

        if (pbCfg.hal().mode().has_baud())
        {
            // Update Baud Rate
            double baudRate = pbCfg.hal().mode().baud().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Mode Baud Rate: " << baudRate;

            adCfg.baudRate = baudRate;
        }

        // Capacity
        if (pbCfg.hal().mode().has_capacity())
        {
            uint capacity = pbCfg.hal().mode().capacity().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Mode Capacity : " << capacity;

            adCfg.capacity = capacity;
        }
    }

    // Advanced Parameter
    if (pbCfg.hal().advanced_param().advanced_param_size() != 0)
    {
        DpAdapter::AdvancedParameter adv;
        for (google::protobuf::Map<std::string, hal_common::AdvParamType_AdvParamDataType>::const_iterator
             it = pbCfg.hal().advanced_param().advanced_param().begin();
             it != pbCfg.hal().advanced_param().advanced_param().end(); ++it)
        {
            adv.apName  = it->second.param_name().value();
            adv.apValue = it->second.value().value();
            adCfg.ap.push_back(adv);

            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " AdvParamName: " << adv.apName << ", AdvParamValue: " << adv.apValue;
        }
    }

    if (pbCfg.hal().has_alarm_threshold())
    {
        // DGD OORH Threshold
        if (pbCfg.hal().alarm_threshold().has_dgd_oorh_threshold())
        {
            uint threshold = pbCfg.hal().alarm_threshold().dgd_oorh_threshold().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " DGD-OORH Threshold: " << threshold;
            adCfg.dgdOorhThresh = threshold;
        }

        // PostFecQ Signal Degrade Threshold
        if (pbCfg.hal().alarm_threshold().has_post_fec_q_signal_degrade_threshold())
        {
            double threshold = pbCfg.hal().alarm_threshold().post_fec_q_signal_degrade_threshold().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " POST-FEC-Q-SIGNAL-DEGRADE Threshold: " << threshold;
            adCfg.postFecQSdThresh = threshold;
        }

        // PostFecQ Signal Degrade Hysteresis
        if (pbCfg.hal().alarm_threshold().has_post_fec_q_signal_degrade_hysteresis())
        {
            double hysteresis = pbCfg.hal().alarm_threshold().post_fec_q_signal_degrade_hysteresis().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " POST-FEC-Q-SIGNAL-DEGRADE Hysteresis: " << hysteresis;
            adCfg.postFecQSdHysteresis = hysteresis;
        }

        // PreFecQ Signal Degrade Threshold
        if (pbCfg.hal().alarm_threshold().has_pre_fec_q_signal_degrade_threshold())
        {
            double threshold = pbCfg.hal().alarm_threshold().pre_fec_q_signal_degrade_threshold().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " PRE-FEC-Q-SIGNAL-DEGRADE Threshold: " << threshold;
            adCfg.preFecQSdThresh = threshold;
        }

        // PreFecQ Signal Degrade Hysteresis
        if (pbCfg.hal().alarm_threshold().has_pre_fec_q_signal_degrade_hysteresis())
        {
            double hysteresis = pbCfg.hal().alarm_threshold().pre_fec_q_signal_degrade_hysteresis().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " PRE-FEC-Q-SIGNAL-DEGRADE Hysteresis: " << hysteresis;
            adCfg.preFecQSdHysteresis = hysteresis;
        }
    }
}

void CarrierPbTranslator::configAdToPb(const DpAdapter::CarrierCommon& adCfg, DpAdapter::CardConfig& cardCfg, Chm6CarrierState& pbStat) const
{
    string cfgId = pbStat.base_state().config_id().value();

    INFN_LOG(SeverityLevel::info) << " config Id: " << cfgId;

    hal_dataplane::CarrierConfig_Config* pInstallCfg = pbStat.mutable_hal()->mutable_installed_config();

    // Enable
    wrapper::Bool wEnable = adCfg.enable ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE;
    pInstallCfg->set_enable_bool(wEnable);

    // Frequency
    pInstallCfg->mutable_frequency()->set_value(adCfg.frequency);

    // Frequency Offset
    pInstallCfg->mutable_frequency_offset()->set_value(adCfg.freqOffset);

    // Target Power
    pInstallCfg->mutable_target_power()->set_value(adCfg.targetPower);

    // Mode (4-tuple)
    hal_dataplane::FourTuple* pMode = pInstallCfg->mutable_mode();
    // App Code
    pMode->mutable_application_code()->set_value(adCfg.appCode);

    // Baud Rate
    pMode->mutable_baud()->set_value(adCfg.baudRate);

    // Capacity
    pMode->mutable_capacity()->set_value(adCfg.capacity);

    // Client Mode
    hal_common::ClientMode clientMode;
    switch (cardCfg.mode)
    {
        case DataPlane::CLIENTMODE_LXTP_E:
            clientMode = hal_common::CLIENT_MODE_LXTP_E;
            break;
        case DataPlane::CLIENTMODE_LXTP_M:
            clientMode = hal_common::CLIENT_MODE_LXTP_M;
            break;
        case DataPlane::CLIENTMODE_LXTP:
            clientMode = hal_common::CLIENT_MODE_LXTP;
            break;
        default:
            clientMode = hal_common::CLIENT_MODE_UNSPECIFIED;
            break;
    }
    pMode->set_client(clientMode);

    // Advanced Parameters
    pInstallCfg->clear_advanced_param();
    google::protobuf::Map<std::string, hal_common::AdvParamType_AdvParamDataType>* advParamMap = pInstallCfg->mutable_advanced_param()->mutable_advanced_param();
    for (auto i = adCfg.ap.begin(); i != adCfg.ap.end(); i++)
    {
        std::string advParamKey(i->apName);
        std::string advParamName(advParamKey);

        hal_common::AdvParamType_AdvParamDataType advParamData;
        advParamData.mutable_param_name()->set_value(advParamName);
        advParamData.mutable_value()->set_value(i->apValue);

        (*advParamMap)[advParamKey] = advParamData;
    }

    // Alarm Thresholds
    hal_dataplane::CarrierAlarmThreshold* pThresh = pInstallCfg->mutable_alarm_threshold();
#ifdef PMD_OORH_IMPLMENENTED
    pThresh->mutable_pmd_oorh_threshold()->set_value(adCfg);
#endif
    pThresh->mutable_dgd_oorh_threshold()->set_value(adCfg.dgdOorhThresh);

    pThresh->mutable_post_fec_q_signal_degrade_threshold ()->set_value(adCfg.postFecQSdThresh);
    pThresh->mutable_post_fec_q_signal_degrade_hysteresis()->set_value(adCfg.postFecQSdHysteresis);
    pThresh->mutable_pre_fec_q_signal_degrade_threshold  ()->set_value(adCfg.preFecQSdThresh);
    pThresh->mutable_pre_fec_q_signal_degrade_hysteresis ()->set_value(adCfg.preFecQSdHysteresis);
}

void CarrierPbTranslator::statusAdToPb(const DpAdapter::CarrierStatus& adStat, Chm6CarrierState& pbStat) const
{
    pbStat.mutable_hal()->mutable_fec_iteration()->set_value(adStat.fecIteration);
    pbStat.mutable_hal()->mutable_fec_oh_percentage()->set_value(adStat.fecOhPercentage);
    pbStat.mutable_hal()->mutable_tx_cd_pre_comp_status()->set_value(adStat.txCdPreComp);
    pbStat.mutable_hal()->mutable_rx_cd_status()->set_value(adStat.rxCd);
    pbStat.mutable_hal()->mutable_line_mode()->set_value(adStat.lineMode);
    pbStat.mutable_hal()->mutable_status()->mutable_application_code()->set_value(adStat.appCode);
    pbStat.mutable_hal()->mutable_status()->mutable_baud()->set_value(adStat.baudRate);
    pbStat.mutable_hal()->mutable_status()->mutable_capacity()->set_value(adStat.capacity);

    double frequency;
    std::string aid = pbStat.base_state().config_id().value();
    DpCarrierMgrSingleton::Instance()->getMspCfgHndlr()->getChm6CarrierConfigFreqFromCache(aid, frequency);
    if (0 == frequency)
    {
        pbStat.mutable_hal()->mutable_actual_tx_frequency()->set_value(0);
        pbStat.mutable_hal()->mutable_actual_rx_frequency()->set_value(0);
    }
    else
    {
        pbStat.mutable_hal()->mutable_actual_tx_frequency()->set_value(adStat.txActualFrequency);
        pbStat.mutable_hal()->mutable_actual_rx_frequency()->set_value(adStat.rxActualFrequency);
    }

    hal_dataplane::CarrierOpticalState opticalState;
    switch (adStat.txState)
    {
        case DataPlane::CARRIERSTATE_ACTIVE:
        opticalState = hal_dataplane::CARRIER_OPTICAL_STATE_ACTIVE;
        break;
        case DataPlane::CARRIERSTATE_STANDBY:
        opticalState = hal_dataplane::CARRIER_OPTICAL_STATE_STANDBY;
        break;
        case DataPlane::CARRIERSTATE_AUTODISCOVERY:
        default:
        opticalState = hal_dataplane::CARRIER_OPTICAL_STATE_UNSPECIFIED;
        break;
    }
    pbStat.mutable_hal()->set_tx_state(opticalState);

    switch (adStat.rxState)
    {
        case DataPlane::CARRIERSTATE_ACTIVE:
        opticalState = hal_dataplane::CARRIER_OPTICAL_STATE_ACTIVE;
        break;
        case DataPlane::CARRIERSTATE_STANDBY:
        opticalState = hal_dataplane::CARRIER_OPTICAL_STATE_STANDBY;
        break;
        case DataPlane::CARRIERSTATE_AUTODISCOVERY:
        default:
        opticalState = hal_dataplane::CARRIER_OPTICAL_STATE_UNSPECIFIED;
        break;
    }
    pbStat.mutable_hal()->set_rx_state(opticalState);

    pbStat.mutable_hal()->clear_current_advanced_param();
    google::protobuf::Map<std::string, hal_common::AdvParamType_AdvParamDataType>* advParamMap = pbStat.mutable_hal()->mutable_current_advanced_param()->mutable_advanced_param();

    for (auto i = adStat.currentAp.begin(); i != adStat.currentAp.end(); i++)
    {
        std::string advParamKey(i->apName);
        std::string advParamName(advParamKey);

        hal_common::AdvParamType_AdvParamDataType advParamData;
        advParamData.mutable_param_name()->set_value(advParamName);
        advParamData.mutable_value()->set_value(i->apValue);
        wrapper::Bool bDefault = (true == i->bDefault) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE;
        advParamData.set_default_(bDefault);
        advParamData.mutable_default_value()->set_value(i->defaultValue);
        hal_common::ApStatus apStatus;
        switch (i->apStatus)
        {
            case DataPlane::CARRIERADVPARMSTATUS_SET:
                apStatus = hal_common::AP_STATUS_SET;
                break;
            case DataPlane::CARRIERADVPARMSTATUS_UNKNOWN:
                apStatus = hal_common::AP_STATUS_UNKNOWN;
                break;
            case DataPlane::CARRIERADVPARMSTATUS_INPROGRESS:
                apStatus = hal_common::AP_STATUS_IN_PROGRESS;
                break;
            case DataPlane::CARRIERADVPARMSTATUS_FAILED:
                apStatus = hal_common::AP_STATUS_FAILED;
                break;
            case DataPlane::CARRIERADVPARMSTATUS_NOT_SUPPORTED:
                apStatus = hal_common::AP_STATUS_NOT_SUPPORTED;
                break;
            default:
                apStatus = hal_common::AP_STATUS_UNSPECIFIED;
                break;
        }
        advParamData.set_ap_status(apStatus);

        (*advParamMap)[advParamKey] = advParamData;
    }
}

void CarrierPbTranslator::faultAdToPb(const std::string aid, const uint64_t& faultBmp, const DpAdapter::CarrierFault& adFault, Chm6CarrierFault& pbFault, bool force) const
{
    hal_dataplane::CarrierFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_fault();

    uint64_t faultBmpNew = adFault.facBmp;
    uint64_t bitMask = 1ULL;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if (((faultBmp ^ faultBmpNew) & bitMask) || (true == force))
        {
            addCarrierFaultPair(vFault->faultName, vFault->direction, vFault->location, vFault->faulted, pbFault);
            if (vFault->faulted)
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FAULT: " << vFault->faultName << "-" << toString(vFault->direction) << "-" << toString(vFault->location) << " = RAISED";
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FAULT: " << vFault->faultName << "-" << toString(vFault->direction) << "-" << toString(vFault->location) << " = CLEARED";
            }
        }

        bitMask <<= 1;
    }
}

void CarrierPbTranslator::addCarrierFaultPair(std::string faultName, DataPlane::FaultDirection faultDirection, DataPlane::FaultLocation faultLocation, bool value, Chm6CarrierFault& pbFault) const
{
    hal_dataplane::CarrierFault_OperationalFault* faultHal = pbFault.mutable_hal();
    google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* faultMap = faultHal->mutable_fault()->mutable_fault();

    std::string faultKey(faultName);
    faultKey.append("-");
    faultKey.append(toString(faultDirection));
    faultKey.append("-");
    faultKey.append(toString(faultLocation));

    hal_common::FaultType_FaultDataType faultData;
    faultData.mutable_fault_name()->set_value(faultName);
    faultData.mutable_value()->set_value(value);

    wrapper::Bool faultValue = (value == true) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE;
    faultData.set_fault_value(faultValue);

    switch (faultDirection)
    {
        case FAULTDIRECTION_NA:
            faultData.set_direction(hal_common::DIRECTION_NA);
            break;
        case FAULTDIRECTION_RX:
            faultData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case FAULTDIRECTION_TX:
            faultData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (faultLocation)
    {
        case FAULTLOCATION_NA:
            faultData.set_location(hal_common::LOCATION_NA);
            break;
        case FAULTLOCATION_NEAR_END:
            faultData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case FAULTLOCATION_FAR_END:
            faultData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    (*faultMap)[faultKey] = faultData;
}

void CarrierPbTranslator::faultAdToPbSim(const uint64_t& fault, Chm6CarrierFault& pbFault) const
{
    hal_dataplane::CarrierFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_fault();

    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_BDI)                      , FAULTDIRECTION_RX, FAULTLOCATION_FAR_END , (bool)(fault & (1ULL << CAR_FAULT_BDI))                      , pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_LOF)                      , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_LOF))                      , pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_OLOS)                     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_OLOS))                     , pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_OPT_OORL)                 , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_OPT_OORL))                 , pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_OPT_OORH)                 , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_OPT_OORH))                 , pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_DGD_OORH)                 , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_DGD_OORH))                 , pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_POST_FEC_Q_SIGNAL_FAILURE), FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_POST_FEC_Q_SIGNAL_FAILURE)), pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_POST_FEQ_Q_SIGNAL_DEGRADE), FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_POST_FEQ_Q_SIGNAL_DEGRADE)), pbFault);
    addCarrierFaultPair(carrierFaultParamsToString.at(CAR_FAULT_PRE_FEQ_Q_SIGNAL_DEGRADE) , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << CAR_FAULT_PRE_FEQ_Q_SIGNAL_DEGRADE)) , pbFault);
}

void CarrierPbTranslator::faultAdToPb(std::string aid, const uint64_t& faultBmp, const DpAdapter::CarrierFault& adFault, Chm6ScgFault& pbFault, bool force) const
{
    hal_dataplane::ScgFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_fault();

    uint64_t faultBmpNew = adFault.facBmp;
    uint64_t bitMask = 1ULL;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        // Only extract Carrier OLOS and map to Scg OLOS
        if ((vFault->faultName == scgFaultParamsToString.at(SCG_FAULT_OLOS)) &&
            (vFault->direction == FAULTDIRECTION_RX) &&
            (vFault->location  == FAULTLOCATION_NEAR_END))
        {
            if (((faultBmp ^ faultBmpNew) & bitMask) || (true == force))
            {
                addScgFaultPair(vFault->faultName, vFault->direction, vFault->location, vFault->faulted, pbFault);
                if (vFault->faulted)
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FAULT: " << vFault->faultName << "-" << toString(vFault->direction) << "-" << toString(vFault->location) << " = RAISED";
                }
                else
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " FAULT: " << vFault->faultName << "-" << toString(vFault->direction) << "-" << toString(vFault->location) << " = CLEARED";
                }
            }
        }

        bitMask <<= 1;
    }
}

bool CarrierPbTranslator::isCarrierSdFault(const DpAdapter::CarrierFault& adFault) const
{
    bool isSdFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_POST_FEQ_Q_SIGNAL_DEGRADE)) &&
            (vFault->direction == FAULTDIRECTION_RX) &&
            (vFault->location  == FAULTLOCATION_NEAR_END) &&
            (vFault->faulted == true))
        {
            isSdFault = true;
            break;
        }
    }

    return isSdFault;
}

bool CarrierPbTranslator::isCarrierSfFault(const DpAdapter::CarrierFault& adFault) const
{
    bool isSfFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_POST_FEC_Q_SIGNAL_FAILURE)) &&
            (vFault->direction == FAULTDIRECTION_RX) &&
            (vFault->location  == FAULTLOCATION_NEAR_END) &&
            (vFault->faulted == true))
        {
            isSfFault = true;
            break;
        }
    }

    return isSfFault;
}

bool CarrierPbTranslator::isCarrierPreFecSdFault(const DpAdapter::CarrierFault& adFault) const
{
    bool isSfFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_PRE_FEQ_Q_SIGNAL_DEGRADE)) &&
            (vFault->direction == FAULTDIRECTION_RX) &&
            (vFault->location  == FAULTLOCATION_NEAR_END) &&
            (vFault->faulted == true))
        {
            isSfFault = true;
            break;
        }
    }

    return isSfFault;
}

bool CarrierPbTranslator::isCarrierBsdFault(const DpAdapter::CarrierFault& adFault) const
{
    bool isSfFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_BSD)) &&
            (vFault->direction == FAULTDIRECTION_RX) &&
            (vFault->location  == FAULTLOCATION_FAR_END) &&
            (vFault->faulted == true))
        {
            isSfFault = true;
            break;
        }
    }

    return isSfFault;
}

bool CarrierPbTranslator::isCarrierOlosFault(const DpAdapter::CarrierFault& adFault) const
{
    bool isSfFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_OLOS)) &&
            (vFault->direction == FAULTDIRECTION_RX) &&
            (vFault->location  == FAULTLOCATION_NEAR_END) &&
            (vFault->faulted == true))
        {
            isSfFault = true;
            break;
        }
    }

    return isSfFault;
}

void CarrierPbTranslator::addScgFaultPair(std::string faultName, FaultDirection faultDirection, FaultLocation faultLocation, bool value, Chm6ScgFault& pbFault) const
{
    hal_dataplane::ScgFault_OperationalFault* faultHal = pbFault.mutable_hal();
    google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* faultMap = faultHal->mutable_fault()->mutable_fault();

    std::string faultKey(faultName);
    faultKey.append("-");
    faultKey.append(toString(faultDirection));
    faultKey.append("-");
    faultKey.append(toString(faultLocation));

    hal_common::FaultType_FaultDataType faultData;
    faultData.mutable_fault_name()->set_value(faultName);
    faultData.mutable_value()->set_value(value);

    wrapper::Bool faultValue = (value == true) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE;
    faultData.set_fault_value(faultValue);

    switch (faultDirection)
    {
        case FAULTDIRECTION_NA:
            faultData.set_direction(hal_common::DIRECTION_NA);
            break;
        case FAULTDIRECTION_RX:
            faultData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case FAULTDIRECTION_TX:
            faultData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (faultLocation)
    {
        case FAULTLOCATION_NA:
            faultData.set_location(hal_common::LOCATION_NA);
            break;
        case FAULTLOCATION_NEAR_END:
            faultData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case FAULTLOCATION_FAR_END:
            faultData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    (*faultMap)[faultKey] = faultData;
}

void CarrierPbTranslator::faultAdToPbSim(const uint64_t& fault, Chm6ScgFault& pbFault) const
{
    hal_dataplane::ScgFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_fault();

    addScgFaultPair(scgFaultParamsToString.at(SCG_FAULT_OLOS), FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (0x10000ULL << SCG_FAULT_OLOS)), pbFault);
}

bool CarrierPbTranslator::isOfecCCFault(const DpAdapter::CarrierFault& adFault) const
{
    bool isOfecCCFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if (((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_BDI)) &&
             (vFault->faulted == true)) ||
             ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_LOF)) &&
             (vFault->faulted == true)) ||
             ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_OLOS)) &&
             (vFault->faulted == true)) ||
             ((vFault->faultName == carrierFaultParamsToString.at(CAR_FAULT_POST_FEC_Q_SIGNAL_FAILURE)) &&
             (vFault->faulted == true)))
        {
            INFN_LOG(SeverityLevel::debug) << " ofec is faulted";
            isOfecCCFault = true;
            break;
        }
    }
    INFN_LOG(SeverityLevel::debug) << "isOfecCCFault = " << isOfecCCFault;

    return isOfecCCFault;
}


void CarrierPbTranslator::addCarrierPmPair(CARRIER_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, uint64_t value, bool validity, Chm6CarrierPm& pbPm) const
{
    hal_dataplane::CarrierPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_pm()->mutable_pm();

    std::string pmKey(carrierPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(carrierPmParamsToString.at(pmParam));

    hal_common::PmType_PmDataType pmData;
    pmData.mutable_pm_data_name()->set_value(pmName);
    pmData.mutable_uint64_val()->set_value(value);

    switch (pmDirection)
    {
        case PMDIRECTION_NA:
            pmData.set_direction(hal_common::DIRECTION_NA);
            break;
        case PMDIRECTION_RX:
            pmData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case PMDIRECTION_TX:
            pmData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (pmLocation)
    {
        case PMLOCATION_NA:
            pmData.set_location(hal_common::LOCATION_NA);
            break;
        case PMLOCATION_NEAR_END:
            pmData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case PMLOCATION_FAR_END:
            pmData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    wrapper::Bool validityValue = (validity == true) ? wrapper::BOOL_TRUE: wrapper::BOOL_FALSE;
    pmData.set_validity_flag(validityValue);

    (*pmMap)[pmKey] = pmData;
}

void CarrierPbTranslator::addCarrierPmPair(CARRIER_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, double value, bool validity, Chm6CarrierPm& pbPm) const
{
    hal_dataplane::CarrierPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_pm()->mutable_pm();

    std::string pmKey(carrierPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(carrierPmParamsToString.at(pmParam));

    hal_common::PmType_PmDataType pmData;
    pmData.mutable_pm_data_name()->set_value(pmName);
    pmData.mutable_double_val()->set_value(value);

    switch (pmDirection)
    {
        case PMDIRECTION_NA:
            pmData.set_direction(hal_common::DIRECTION_NA);
            break;
        case PMDIRECTION_RX:
            pmData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case PMDIRECTION_TX:
            pmData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (pmLocation)
    {
        case PMLOCATION_NA:
            pmData.set_location(hal_common::LOCATION_NA);
            break;
        case PMLOCATION_NEAR_END:
            pmData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case PMLOCATION_FAR_END:
            pmData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    wrapper::Bool validityValue = (validity == true) ? wrapper::BOOL_TRUE: wrapper::BOOL_FALSE;
    pmData.set_validity_flag(validityValue);

    (*pmMap)[pmKey] = pmData;
}

void CarrierPbTranslator::pmAdToPb(const DataPlane::DpMsCarrierPMContainer& adPm, bool validity, Chm6CarrierPm& pbPm) const
{
    hal_dataplane::CarrierPm_OperationalPm* pmHal = pbPm.mutable_hal();
    pmHal->clear_pm();

    uint64_t undefinedField = 0;

    addCarrierPmPair(CAR_PM_LBC_EGRESS        , PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.laser_current     , validity, pbPm);
    addCarrierPmPair(CAR_PM_LBC_INGRESS       , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.laser_current     , validity, pbPm);
    addCarrierPmPair(CAR_PM_CD                , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.cd                , validity, pbPm);
    addCarrierPmPair(CAR_PM_DGD               , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.dgd               , validity, pbPm);
    addCarrierPmPair(CAR_PM_SO_DGD            , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.so_dgd            , validity, pbPm);
    addCarrierPmPair(CAR_PM_SNR_AVG_X         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.snr_avg_x         , validity, pbPm);
    addCarrierPmPair(CAR_PM_SNR_AVG_Y         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.snr_avg_y         , validity, pbPm);
    addCarrierPmPair(CAR_PM_SNR_TOTAL         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.snr               , validity, pbPm);
    addCarrierPmPair(CAR_PM_SNR_INNER         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.snr_inner         , validity, pbPm);
    addCarrierPmPair(CAR_PM_SNR_OUTER         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.snr_outer         , validity, pbPm);
    addCarrierPmPair(CAR_PM_SNR_DIFF_LEFT     , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.snr_diff_left     , validity, pbPm);
    addCarrierPmPair(CAR_PM_SNR_DIFF_RIGHT    , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.snr_diff_right    , validity, pbPm);
    addCarrierPmPair(CAR_PM_PRE_FEC_Q         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.pre_fec_q         , validity, pbPm);
    addCarrierPmPair(CAR_PM_POST_FEC_Q        , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.post_fec_q        , validity, pbPm);
    addCarrierPmPair(CAR_PM_PRE_FEC_BER       , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.pre_fec_ber       , validity, pbPm);
    addCarrierPmPair(CAR_PM_PASSING_FEC_FRAMES, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.pass_frame_count  , validity, pbPm);
    addCarrierPmPair(CAR_PM_FAILED_FEC_FRAMES , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.failed_frame_count, validity, pbPm);
    addCarrierPmPair(CAR_PM_TOTAL_FEC_FRAMES  , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.total_frame_count , validity, pbPm);
    addCarrierPmPair(CAR_PM_CORRECTED_BITS    , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.corrected_bits,     validity, pbPm);
    addCarrierPmPair(CAR_PM_PHASE_CORRECTION  , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.phase_correction  , validity, pbPm);
    addCarrierPmPair(CAR_PM_TX_CD             , PMDIRECTION_TX, PMLOCATION_FAR_END,  adPm.tx.cd                , validity, pbPm);
    addCarrierPmPair(CAR_PM_OSNR              , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.osnr              , validity, pbPm);

    double frequency;
    DpCarrierMgrSingleton::Instance()->getMspCfgHndlr()->getChm6CarrierConfigFreqFromCache(adPm.aid, frequency);
    if (0 == frequency)
    {
        addCarrierPmPair(CAR_PM_TX_FREQUENCY, PMDIRECTION_TX, PMLOCATION_NEAR_END, 0.0, false, pbPm);
        addCarrierPmPair(CAR_PM_RX_FREQUENCY, PMDIRECTION_RX, PMLOCATION_NEAR_END, 0.0, false, pbPm);
    }
    else
    {
        addCarrierPmPair(CAR_PM_TX_FREQUENCY, PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.frequency, validity, pbPm);
        addCarrierPmPair(CAR_PM_RX_FREQUENCY, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.frequency, validity, pbPm);
    }

    addCarrierPmPair(CAR_PM_SUBCARRIER2_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[2].subcarrier_snr, validity, pbPm);
    addCarrierPmPair(CAR_PM_SUBCARRIER3_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[3].subcarrier_snr, validity, pbPm);
    addCarrierPmPair(CAR_PM_SUBCARRIER4_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[4].subcarrier_snr, validity, pbPm);
    addCarrierPmPair(CAR_PM_SUBCARRIER5_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[5].subcarrier_snr, validity, pbPm);

    if (adPm.subCarrierCnt == 8)
    {
	    addCarrierPmPair(CAR_PM_SUBCARRIER0_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[0].subcarrier_snr, validity, pbPm);
	    addCarrierPmPair(CAR_PM_SUBCARRIER1_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[1].subcarrier_snr, validity, pbPm);
	    addCarrierPmPair(CAR_PM_SUBCARRIER6_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[6].subcarrier_snr, validity, pbPm);
	    addCarrierPmPair(CAR_PM_SUBCARRIER7_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[7].subcarrier_snr, validity, pbPm);
    }
    else if (adPm.subCarrierCnt == 4) //mark sub carrier 0,1,6,7 snr as invalid to show na in CLI
    {
	    addCarrierPmPair(CAR_PM_SUBCARRIER0_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[0].subcarrier_snr, false, pbPm);
	    addCarrierPmPair(CAR_PM_SUBCARRIER1_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[1].subcarrier_snr, false, pbPm);
	    addCarrierPmPair(CAR_PM_SUBCARRIER6_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[6].subcarrier_snr, false, pbPm);
	    addCarrierPmPair(CAR_PM_SUBCARRIER7_SNR   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.subcarrier_snr_list[7].subcarrier_snr, false, pbPm);
    }
}

void CarrierPbTranslator::addScgPmPair(SCG_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, double value, bool validity, Chm6ScgPm& pbPm) const
{
    hal_dataplane::ScgPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_pm()->mutable_pm();

    std::string pmKey(scgPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(scgPmParamsToString.at(pmParam));

    hal_common::PmType_PmDataType pmData;
    pmData.mutable_pm_data_name()->set_value(pmName);
    pmData.mutable_double_val()->set_value(value);

    switch (pmDirection)
    {
        case PMDIRECTION_NA:
            pmData.set_direction(hal_common::DIRECTION_NA);
            break;
        case PMDIRECTION_RX:
            pmData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case PMDIRECTION_TX:
            pmData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (pmLocation)
    {
        case PMLOCATION_NA:
            pmData.set_location(hal_common::LOCATION_NA);
            break;
        case PMLOCATION_NEAR_END:
            pmData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case PMLOCATION_FAR_END:
            pmData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    wrapper::Bool validityValue = (validity == true) ? wrapper::BOOL_TRUE: wrapper::BOOL_FALSE;
    pmData.set_validity_flag(validityValue);

    (*pmMap)[pmKey] = pmData;
}

void CarrierPbTranslator::pmAdToPb(const DataPlane::DpMsCarrierPMContainer& adPm, bool validity, Chm6ScgPm& pbPm) const
{
    hal_dataplane::ScgPm_OperationalPm* pmHal = pbPm.mutable_hal();
    pmHal->clear_pm();

    uint64_t undefinedField = 0;

    addScgPmPair(SCG_PM_OPT        , PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.opt              , validity, pbPm);
    addScgPmPair(SCG_PM_OPR        , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.opr              , validity, pbPm);
    addScgPmPair(SCG_PM_TX_EDFA_OPT, PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.edfa_output_power, validity, pbPm);
    addScgPmPair(SCG_PM_TX_EDFA_OPR, PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.edfa_input_power , validity, pbPm);
    addScgPmPair(SCG_PM_TX_EDFA_LBC, PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.edfa_pump_current, validity, pbPm);
}

void CarrierPbTranslator::pmAdDefault(DataPlane::DpMsCarrierPMContainer& adPm)
{
    memset(&adPm.rx, 0, sizeof(adPm.rx));
    memset(&adPm.tx, 0, sizeof(adPm.tx));

    adPm.tx.opt               = -55.0;
    adPm.rx.opr               = -55.0;
    adPm.tx.edfa_output_power = -55.0;
    adPm.tx.edfa_input_power  = -55.0;
}

void CarrierPbTranslator::configPbToAd(const Chm6CarrierConfig& pbCfg, DpAdapter::CardConfig& adCfg) const
{
    std::string carAid = pbCfg.base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << carAid;

    if (pbCfg.hal().has_mode())
    {
        // Update Client Mode
        if (hal_common::CLIENT_MODE_UNSPECIFIED != pbCfg.hal().mode().client())
        {
            hal_common::ClientMode client = pbCfg.hal().mode().client();
            INFN_LOG(SeverityLevel::info) << "AID: " << carAid << " Client Mode : " << toString(client);
            switch (client)
            {
                case hal_common::CLIENT_MODE_LXTP_E:
                    adCfg.mode = DataPlane::CLIENTMODE_LXTP_E;
                    break;
                case hal_common::CLIENT_MODE_LXTP_M:
                    adCfg.mode = DataPlane::CLIENTMODE_LXTP_M;
                    break;
                case hal_common::CLIENT_MODE_LXTP:
                    adCfg.mode = DataPlane::CLIENTMODE_LXTP;
                    break;
                default:
                    break;
            }
        }
    }
}

void CarrierPbTranslator::statusAdToPb(const DpAdapter::CardStatus& adStat, Chm6CarrierState& pbStat) const
{
    //TODO: Need ClientMode in DCO Adapter CardStatus structure
    // ClientMode
    hal_common::ClientMode clientMode = hal_common::CLIENT_MODE_LXTP_E;
#if 0
    switch (adStat.mode)
    {
        case DataPlane::CLIENTMODE_LXTP_E:
            clientMode = hal_common::CLIENT_MODE_LXTP_E:
            break;
        case DataPlane::CLIENTMODE_LXTP_M:
            clientMode = hal_common::CLIENT_MODE_LXTP_M:
            break;
        case DataPlane::CLIENTMODE_LXTP:
            clientMode = hal_common::CLIENT_MODE_LXTP:
            break;
        default:
            clientMode = hal_common::CLIENT_MODE_UNSPECIFIED:
            break;
    }
#endif
    pbStat.mutable_hal()->mutable_status()->set_client(clientMode);
}

bool CarrierPbTranslator::isPbChanged(const chm6_dataplane::Chm6CarrierState& pbStatPrev, const chm6_dataplane::Chm6CarrierState& pbStatCur) const
{
    if (  (pbStatPrev.hal().tx_state()                   != pbStatCur.hal().tx_state())
       || (pbStatPrev.hal().rx_state()                   != pbStatCur.hal().rx_state())
       || (pbStatPrev.hal().fec_iteration().value()      != pbStatCur.hal().fec_iteration().value())
       || (pbStatPrev.hal().fec_oh_percentage().value()  != pbStatCur.hal().fec_oh_percentage().value())
       || (pbStatPrev.hal().status().capacity().value()  != pbStatCur.hal().status().capacity().value())
       || (pbStatPrev.hal().status().client()            != pbStatCur.hal().status().client())
       || (pbStatPrev.hal().status().baud().value()      != pbStatCur.hal().status().baud().value())
       || (pbStatPrev.hal().gain_share_status_bool()     != pbStatCur.hal().gain_share_status_bool())
       || (pbStatPrev.hal().actual_tx_frequency().value()     != pbStatCur.hal().actual_tx_frequency().value())
       || (pbStatPrev.hal().actual_rx_frequency().value()     != pbStatCur.hal().actual_rx_frequency().value()) )
    {
        return true;
    }

    if (  (pbStatPrev.hal().status().application_code().value().compare(pbStatCur.hal().status().application_code().value()) != 0)
       || (pbStatPrev.hal().line_mode().value().compare(                pbStatCur.hal().line_mode().value()) != 0) )
    {
        return true;
    }

    if (  (fabs(pbStatCur.hal().tx_cd_pre_comp_status().value() - pbStatPrev.hal().tx_cd_pre_comp_status().value()) > CARRIER_CD_THRESHOLD)
       || (fabs(pbStatCur.hal().rx_cd_status().value()          - pbStatPrev.hal().rx_cd_status().value())          > CARRIER_CD_THRESHOLD) )
    {
        return true;
    }

    if(pbStatPrev.hal().current_advanced_param().advanced_param().size() != pbStatCur.hal().current_advanced_param().advanced_param().size())
    {
	    return true;
    }
    for(auto prev = pbStatPrev.hal().current_advanced_param().advanced_param().begin(); prev !=  pbStatPrev.hal().current_advanced_param().advanced_param().end(); prev++)
    {
	    if(pbStatCur.hal().current_advanced_param().advanced_param().find(prev->first) == pbStatPrev.hal().current_advanced_param().advanced_param().end())
	    {
		    return true;
	    }
        if (prev->second.param_name().value() != (pbStatCur.hal().current_advanced_param().advanced_param()).at(prev->first).param_name().value()
        || prev->second.value().value() != (pbStatCur.hal().current_advanced_param().advanced_param()).at(prev->first).value().value()
        || prev->second.default_() != (pbStatCur.hal().current_advanced_param().advanced_param()).at(prev->first).default_()
        || prev->second.default_value().value() != (pbStatCur.hal().current_advanced_param().advanced_param()).at(prev->first).default_value().value()
        || prev->second.ap_status() != (pbStatCur.hal().current_advanced_param().advanced_param()).at(prev->first).ap_status() )
	    {
		    return true;
	    }
    }
    return false;
}

bool CarrierPbTranslator::isAdChanged(const DpAdapter::CarrierFault& adFaultPrev, const DpAdapter::CarrierFault& adFaultCur)
{
    if (adFaultPrev.facBmp != adFaultCur.facBmp)
    {
        return true;
    }

    return false;
}

std::string CarrierPbTranslator::numToAidCar(uint carNum) const
{
    // Example: Carrier Aid = 1-4-L2-1

    std::string aid = getChassisSlotAid() + "-L" + to_string(carNum) + "-1";

    return (aid);
}

std::string CarrierPbTranslator::aidScgToCar(std::string aidStr) const
{
    std::string carAid = aidStr + "-1";

    return carAid;
}

std::string CarrierPbTranslator::aidCarToScg(std::string aidStr) const
{
    // Example: Carrier Aid = 1-4-L2-1
    //          Scg Aid     = 1-4-L2

    if (aidStr.empty())
    {
        return aidStr;
    }

    std::string scgAid;
    std::string delim = "-";
    std::size_t found = aidStr.rfind(delim);

    if (found != std::string::npos)
    {
        scgAid = aidStr.substr(0,found);
    }
    else
    {
        scgAid.assign(aidStr);
    }

    return scgAid;
}

bool CarrierPbTranslator::isCarrierTxStateChanged(std::string aid, const chm6_dataplane::Chm6CarrierState& pbStat, hal_dataplane::CarrierOpticalState& pbCarrTxState) const
{
    bool isChanged = false;

    if ( (pbStat.hal().tx_state() != hal_dataplane::CARRIER_OPTICAL_STATE_UNSPECIFIED) &&
         (pbStat.hal().tx_state() != pbCarrTxState) )
    {
        isChanged = true;
    }
    pbCarrTxState = pbStat.hal().tx_state();

    return isChanged;
}

void CarrierPbTranslator::addPbScgCarrierLockFault(std::string scgAid, const hal_dataplane::CarrierOpticalState& pbCarrTxState, chm6_dataplane::Chm6ScgFault& pbFault) const
{
    bool faulted = true;

    if (pbCarrTxState == hal_dataplane::CARRIER_OPTICAL_STATE_STANDBY)
    {
        faulted = true;

    }
    else if (pbCarrTxState == hal_dataplane::CARRIER_OPTICAL_STATE_ACTIVE)
    {
        faulted = false;
    }

    addScgFaultPair(scgFaultParamsToString.at(SCG_FAULT_LOCK), FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, faulted, pbFault);

    INFN_LOG(SeverityLevel::info) << "scgAid: " << scgAid
                                  << " Scg Tx state: " << hal_dataplane::CarrierOpticalState_Name(pbCarrTxState)
                                  << " LS-ACTIVE faulted:" << (int)faulted;
}

void CarrierPbTranslator::mergeObj(chm6_dataplane::Chm6CarrierConfig& toCfg, const chm6_dataplane::Chm6CarrierConfig& fromCfg)
{
    toCfg.MergeFrom(fromCfg);

    // Merge DoubleVals here due to issue handling default values in MergeFrom above

    if (fromCfg.hal().has_frequency())
    {
        toCfg.mutable_hal()->mutable_frequency()->set_value((fromCfg.hal().frequency().value()));
    }

    if (fromCfg.hal().has_frequency_offset())
    {
        toCfg.mutable_hal()->mutable_frequency_offset()->set_value((fromCfg.hal().frequency_offset().value()));
    }

    if (fromCfg.hal().has_target_power())
    {

        toCfg.mutable_hal()->mutable_target_power()->set_value((fromCfg.hal().target_power().value()));
    }

    if (fromCfg.hal().has_tx_cd_pre_comp())
    {
        toCfg.mutable_hal()->mutable_tx_cd_pre_comp()->set_value((fromCfg.hal().tx_cd_pre_comp().value()));
    }
}

}; // namespace DataPlane
