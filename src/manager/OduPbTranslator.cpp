/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "OduPbTranslator.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"

#include <iostream>
#include <sstream>

using ::chm6_dataplane::Chm6OduState;
using ::chm6_dataplane::Chm6OduConfig;
using ::chm6_dataplane::Chm6OduFault;
using ::chm6_dataplane::Chm6OduPm;

using namespace ::std;


namespace DataPlane {

void OduPbTranslator::configPbToAd(const Chm6OduConfig& pbCfg, DpAdapter::OduCommon& adCfg) const
{
    std::string oduAid = pbCfg.base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid;

    // OduId
    if (pbCfg.hal().has_odu_index())
    {
        uint oduId = pbCfg.hal().odu_index().value();
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " OduId: " << oduId;
        adCfg.loOduId = oduId;
    }

    // ServiceMode
    hal_common::ServiceMode serviceMode = hal_common::SERVICE_MODE_NONE;
    if (hal_common::SERVICE_MODE_UNSPECIFIED != pbCfg.hal().service_mode())
    {
        serviceMode = pbCfg.hal().service_mode();
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " ServiceMode: " << toString(serviceMode);
        switch (serviceMode)
        {
            case hal_common::SERVICE_MODE_NONE:
                adCfg.srvMode = DataPlane::SERVICEMODE_NONE;
                break;
            case hal_common::SERVICE_MODE_SWITCHING:
                adCfg.srvMode = DataPlane::SERVICEMODE_SWITCHING;
                break;
            case hal_common::SERVICE_MODE_WRAPPER:
            case hal_common::SERVICE_MODE_MUXED:
            case hal_common::SERVICE_MODE_ADAPTATION:
            default:
                adCfg.srvMode = DataPlane::SERVICEMODE_NONE;
                break;
        }
    }

    // ConfigSvcType
    if (hal_common::BAND_WIDTH_UNSPECIFIED != pbCfg.hal().config_svc_type())
    {
        hal_common::BandWidth bandwidth = pbCfg.hal().config_svc_type();
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " ConfigSvcType: " << toString(bandwidth);
        switch (bandwidth)
        {
            case hal_common::BAND_WIDTH_100GB_ELAN:
            case hal_common::BAND_WIDTH_LO_ODU4I:
                // In R5.x ODU Switching have LO-ODU4 Service Mode set
                // to  SWITCHING.  We checked Service Mode to set the
                // payload
                //  In R6.0 Xpress XCON case, LO-ODU4I can also have
                //  Service Mode set to SWITCHING.
                //  Checking Service Mode won't work anymore. We need to
                //  check the AID to make sure we don't set the wrong payload
                {
                    string Odu4i = "ODU4i";
                    std::size_t tpos = oduAid.find(Odu4i);

                    // AID is not LO-ODU4I, it is for LO-ODU4 Switching
                    if (tpos == std::string::npos)
                    {
                        adCfg.payLoad = DataPlane::ODUPAYLOAD_LO_ODU4;
                    }
                    else
                    {
                        adCfg.payLoad = DataPlane::ODUPAYLOAD_LO_ODU4I;
                    }
                }
                adCfg.type = DataPlane::ODUSUBTYPE_LINE;
                break;
            case hal_common::BAND_WIDTH_400GB_ELAN:
            case hal_common::BAND_WIDTH_LO_FLEXI:
                adCfg.payLoad = DataPlane::ODUPAYLOAD_LO_FLEXI;
                adCfg.type = DataPlane::ODUSUBTYPE_LINE;
                break;
            case hal_common::BAND_WIDTH_ODU4:
                adCfg.payLoad = DataPlane::ODUPAYLOAD_ODU4;
                adCfg.type = DataPlane::ODUSUBTYPE_CLIENT;
                break;
            default:
                break;
        }
    }

    // OduPrbsConfig
    if (pbCfg.hal().has_odu_prbs_config())
    {
        // FacPrbsGen
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().odu_prbs_config().fac_prbs_gen_bool())
        {
            wrapper::Bool generatorTx = pbCfg.hal().odu_prbs_config().fac_prbs_gen_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " FacPrbsGen: " << toString(generatorTx);
            switch (generatorTx)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.generatorTx = PRBSMODE_PRBS_X31;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.generatorTx = PRBSMODE_PRBS_NONE;
                    break;
                default:
                    break;
            }
        }

        // FacPrbsMon
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().odu_prbs_config().fac_prbs_mon_bool())
        {
            wrapper::Bool monitorTx = pbCfg.hal().odu_prbs_config().fac_prbs_mon_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " FacPrbsMon: " << toString(monitorTx);
            switch (monitorTx)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.monitorTx = PRBSMODE_PRBS_X31;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.monitorTx = PRBSMODE_PRBS_NONE;
                    break;
                default:
                    break;
            }
        }

        // TermPrbsGen
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().odu_prbs_config().term_prbs_gen_bool())
        {
            wrapper::Bool generatorRx = pbCfg.hal().odu_prbs_config().term_prbs_gen_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TermPrbsGen: " << toString(generatorRx);
            switch (generatorRx)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.generatorRx = PRBSMODE_PRBS_X31;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.generatorRx = PRBSMODE_PRBS_NONE;
                    break;
                default:
                    break;
            }
        }

        // TermPrbsMon
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().odu_prbs_config().term_prbs_mon_bool())
        {
            wrapper::Bool monitorRx = pbCfg.hal().odu_prbs_config().term_prbs_mon_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TermPrbsMon: " << toString(monitorRx);
            switch (monitorRx)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.monitorRx = PRBSMODE_PRBS_X31;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.monitorRx = PRBSMODE_PRBS_NONE;
                    break;
                default:
                    break;
            }
        }
    }

    // OduMsConfig
    if (pbCfg.hal().has_odu_ms_config())
    {
        // FacForceAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().odu_ms_config().fac_forceaction())
        {
            hal_common::MaintenanceAction forcedMs = pbCfg.hal().odu_ms_config().fac_forceaction();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " FacForceAction: " << toString(forcedMs);
            switch (forcedMs)
            {
                case hal_common::MAINTENANCE_ACTION_SENDAIS:
                    adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_AIS;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDOCI:
                    adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_OCI;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDLCK:
                    adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_LCK;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31:
                    adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31_INV:
                    adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV;
                    break;
                case hal_common::MAINTENANCE_ACTION_NOREPLACE:
                    adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                    break;
                default:
                    break;
            }
        }

        // FacAutoAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().odu_ms_config().fac_autoaction())
        {
            hal_common::MaintenanceAction autoMs = pbCfg.hal().odu_ms_config().fac_autoaction();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " FacAutoAction: " << toString(autoMs);
            switch (autoMs)
            {
                case hal_common::MAINTENANCE_ACTION_SENDAIS:
                    adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_AIS;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDOCI:
                    adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_OCI;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDLCK:
                    adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_LCK;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31:
                    adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31_INV:
                    adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV;
                    break;
                case hal_common::MAINTENANCE_ACTION_NOREPLACE:
                    adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                    break;
                default:
                    break;
            }
        }

        // TermForceAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().odu_ms_config().term_forceaction())
        {
            hal_common::MaintenanceAction forcedMs = pbCfg.hal().odu_ms_config().term_forceaction();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TermForceAction: " << toString(forcedMs);
            switch (forcedMs)
            {
                case hal_common::MAINTENANCE_ACTION_SENDAIS:
                    adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_AIS;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDOCI:
                    adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_OCI;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDLCK:
                    adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_LCK;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31:
                    adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31_INV:
                    adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV;
                    break;
                case hal_common::MAINTENANCE_ACTION_NOREPLACE:
                    adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                    break;
                default:
                    break;
            }
        }

        // TermAutoAction
        if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().odu_ms_config().term_autoaction())
        {
            hal_common::MaintenanceAction autoMs = pbCfg.hal().odu_ms_config().term_autoaction();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TermAutoAction: " << toString(autoMs);
            switch (autoMs)
            {
                case hal_common::MAINTENANCE_ACTION_SENDAIS:
                    adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_AIS;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDOCI:
                    adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_OCI;
                    break;
                case hal_common::MAINTENANCE_ACTION_SENDLCK:
                    adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_LCK;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31:
                    adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31;
                    break;
                case hal_common::MAINTENANCE_ACTION_PRBS_X31_INV:
                    adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV;
                    break;
                case hal_common::MAINTENANCE_ACTION_NOREPLACE:
                    adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                    break;
                default:
                    break;
            }
        }
    }

    // TxTtiConfig - TODO: Deprecate when new TTI map is implemented on Agent
    if (pbCfg.hal().tx_tti_config().transmit_tti_size() != 0)
    {
        for (uint index = 0; index < pbCfg.hal().tx_tti_config().transmit_tti_size(); index++)
        {
            uint txTtiConfig = pbCfg.hal().tx_tti_config().transmit_tti(index).value();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TxTtiConfig: " << txTtiConfig;
            adCfg.ttiTx[index] = static_cast<uint8_t>(txTtiConfig);
        }
        for (uint index = pbCfg.hal().tx_tti_config().transmit_tti_size(); index < MAX_TTI_LENGTH; index++)
        {
            adCfg.ttiTx[index] = 0;
        }
    }

    // TxTtiConfig
    if (pbCfg.hal().tx_tti_cfg().tti_size() != 0)
    {
        for (google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>::const_iterator
             it = pbCfg.hal().tx_tti_cfg().tti().begin();
             it != pbCfg.hal().tx_tti_cfg().tti().end(); ++it)
        {
            uint index = it->first;
            if (MAX_TTI_LENGTH > index)
            {
                adCfg.ttiTx[index] = it->second.value().value();
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TxTtiConfig invalid index = " << it->second.value().value();
            }
        }

        std::ostringstream logSapi;
        for (uint i = 0; i < 16; i++)
        {
            logSapi << " 0x" << std::hex << (int)adCfg.ttiTx[i] << std::dec;
        }
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TxTtiSAPI =" << logSapi.str();

        std::ostringstream logDapi;
        for (uint i = 16; i < 32; i++)
        {
            logDapi << " 0x" << std::hex << (int)adCfg.ttiTx[i] << std::dec;
        }
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TxTtiDAPI =" << logDapi.str();

        std::ostringstream logOper;
        for (uint i = 32; i < MAX_TTI_LENGTH; i++)
        {
            logOper << " 0x" << std::hex << (int)adCfg.ttiTx[i] << std::dec;
        }
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TxTtiOPER =" << logOper.str();
    }

    // Ts
    if (pbCfg.hal().timeslot().ts_size() != 0)
    {
        uint32_t timeslot[MAX_25G_TIME_SLOT];
        memset(timeslot, 0, sizeof(timeslot));

        uint numTimeslots = 0;
        for (google::protobuf::Map<uint32_t, hal_common::TsType_TsDataType>::const_iterator
             it = pbCfg.hal().timeslot().ts().begin();
             it != pbCfg.hal().timeslot().ts().end(); ++it)
        {
            uint index = it->first;
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " Timeslot[" << index << "]: " << it->second.value().value();
            if (MAX_25G_TIME_SLOT > index)
            {
                timeslot[index] = it->second.value().value();
            }
            else
            {
                INFN_LOG(SeverityLevel::error) << "AID: " << oduAid << " timeslot invalid index = " << index;
            }
            numTimeslots++;
        }
        for (uint i = 0; i < numTimeslots; i++)
        {
            adCfg.tS.push_back(timeslot[i]);
        }
    }

    // Loopback
    if (hal_common::LOOPBACK_TYPE_UNSPECIFIED != pbCfg.hal().loopback())
    {
        hal_common::LoopbackType lpbk = pbCfg.hal().loopback();
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " Loopback: " << toString(lpbk);
        switch (lpbk)
        {
            case hal_common::LOOPBACK_TYPE_NONE:
                adCfg.lpbk = DataPlane::LOOP_BACK_TYPE_OFF;
                break;
            case hal_common::LOOPBACK_TYPE_TERMINAL:
                adCfg.lpbk = DataPlane::LOOP_BACK_TYPE_TERMINAL;
                break;
            case hal_common::LOOPBACK_TYPE_FACILITY:
                adCfg.lpbk = DataPlane::LOOP_BACK_TYPE_FACILITY;
                break;
            default:
                break;
        }
    }

    // TsGranularity
    if (hal_dataplane::TS_GRANULARITY_UNSPECIFIED != pbCfg.hal().ts_granularity())
    {
        hal_dataplane::TSGranularity tsGranularity = pbCfg.hal().ts_granularity();
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TsGranularity: " << toString(tsGranularity);
        switch (tsGranularity)
        {
            case hal_dataplane::TS_GRANULARITY_25G:
                adCfg.tsg = DataPlane::TSGRANULARITY_25G;
                break;
            default:
                break;
        }
    }

    // Alarm Threshold Ingress
    if (pbCfg.hal().has_alarm_threshold())
    {
        // Signal Degrade Interval - Ingress
        if (pbCfg.hal().alarm_threshold().has_signal_degrade_interval())
        {
            uint interval = pbCfg.hal().alarm_threshold().signal_degrade_interval().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " Signal Degrade Interval Ingress: " << interval;
            // Used by DataplaneMgr, not by DCO Adapter
        }

        // Signal Degrade Threshold - Ingress
        if (pbCfg.hal().alarm_threshold().has_signal_degrade_threshold())
        {
            uint interval = pbCfg.hal().alarm_threshold().signal_degrade_threshold().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " Signal Degrade Threshold Ingress: " << interval;
            // Used by DataplaneMgr, not by DCO Adapter
        }

        // Expected TTI
        if (pbCfg.hal().alarm_threshold().tti_expected().tti_size() != 0)
        {
            uint8_t ttiExpect[MAX_TTI_LENGTH];

            for (google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>::const_iterator
                 it = pbCfg.hal().alarm_threshold().tti_expected().tti().begin();
                 it != pbCfg.hal().alarm_threshold().tti_expected().tti().end(); ++it)
            {
                uint index = it->first;
                if (MAX_TTI_LENGTH > index)
                {
                    ttiExpect[index] = it->second.value().value();
                }
                else
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TtiExpectedConfig invalid index = " << it->second.value().value();
                }

            }

            std::ostringstream logSapi;
            for (uint i = 0; i < 16; i++)
            {
                logSapi << " 0x" << std::hex << (int)ttiExpect[i] << std::dec;
            }
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " ExpectedTtiSAPI =" << logSapi.str();

            std::ostringstream logDapi;
            for (uint i = 16; i < 32; i++)
            {
                logDapi << " 0x" << std::hex << (int)ttiExpect[i] << std::dec;
            }
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " ExpectedTtiDAPI =" << logDapi.str();

            std::ostringstream logOper;
            for (uint i = 32; i < MAX_TTI_LENGTH; i++)
            {
                logOper << " 0x" << std::hex << (int)ttiExpect[i] << std::dec;
            }
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " ExpectedTtiOPER =" << logOper.str();
            // Used by DataplaneMgr, not by DCO Adapter
        }

        // TTI Mismatch Type
        if (hal_common::TTI_MISMATCH_TYPE_UNSPECIFIED != pbCfg.hal().alarm_threshold().tti_mismatch_type())
        {
            hal_common::TtiMismatchType mismatchType = pbCfg.hal().alarm_threshold().tti_mismatch_type();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " TtiMismatchType: " << toString(mismatchType);
            // Used by Dataplane Mgr, not by DCO Adapter
        }
    }

    // Alarm Threshold Egress
    if (pbCfg.hal().has_alarm_threshold_egress())
    {
        // Signal Degrade Interval - Egress
        if (pbCfg.hal().alarm_threshold_egress().has_signal_degrade_interval())
        {
            uint interval = pbCfg.hal().alarm_threshold_egress().signal_degrade_interval().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " Signal Degrade Interval Egress: " << interval;
            // Used by DataplaneMgr, not by DCO Adapter
        }

        // Signal Degrade Threshold - Egress
        if (pbCfg.hal().alarm_threshold_egress().has_signal_degrade_threshold())
        {
            uint interval = pbCfg.hal().alarm_threshold_egress().signal_degrade_threshold().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " Signal Degrade Threshold Egress: " << interval;
            // Used by DataplaneMgr, not by DCO Adapter
        }
    }
    // Regen flag for Xpress XCON.
    if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().regen_enable())
    {
        wrapper::Bool regen = pbCfg.hal().regen_enable();
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " regen: " << toString(regen);
        switch (regen)
        {
            case wrapper::BOOL_TRUE:
                adCfg.regen = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.regen = false;
                break;
            default:
                adCfg.regen = false;
            break;
        }
    }
}

void OduPbTranslator::configAdToPb(const DpAdapter::OduCommon& adCfg, Chm6OduState& pbStat) const
{
    std::string configId = pbStat.base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << configId;

    ::infinera::hal::dataplane::v2::OduConfig_Config* pInstallCfg = pbStat.mutable_hal()->mutable_installed_config();

    // OduId
    pbStat.mutable_hal()->mutable_installed_config()->mutable_odu_index()->set_value(adCfg.loOduId);

    // Regen flag for Xpress Xcon
    pInstallCfg->set_regen_enable(adCfg.regen ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);

    // ConfigSvcType
    switch(adCfg.payLoad)
    {
        case DataPlane::ODUPAYLOAD_LO_ODU4I:
        case DataPlane::ODUPAYLOAD_LO_ODU4:
            pInstallCfg->set_config_svc_type(hal_common::BAND_WIDTH_100GB_ELAN);
            break;
        case DataPlane::ODUPAYLOAD_LO_FLEXI:
            pInstallCfg->set_config_svc_type(hal_common::BAND_WIDTH_400GB_ELAN);
            break;
        case DataPlane::ODUPAYLOAD_ODU4:
            pInstallCfg->set_config_svc_type(hal_common::BAND_WIDTH_100GB_ELAN);
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Unknown payload: " << adCfg.payLoad;
            break;
    }

    ::infinera::hal::dataplane::v2::OduPrbsConfig* pPrbsCfg = pInstallCfg->mutable_odu_prbs_config();

    // FacPrbsGen
    switch(adCfg.generatorTx)
    {
        case PRBSMODE_PRBS_X31:
            pPrbsCfg->set_fac_prbs_gen_bool(wrapper::BOOL_TRUE);
            break;
        case PRBSMODE_PRBS_NONE:
            pPrbsCfg->set_fac_prbs_gen_bool(wrapper::BOOL_FALSE);
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Unknown Fac Prbs Gen: " << adCfg.generatorTx;
            break;
    }

    // FacPrbsMon
    switch(adCfg.monitorTx)
    {
        case PRBSMODE_PRBS_X31:
            pPrbsCfg->set_fac_prbs_mon_bool(wrapper::BOOL_TRUE);
            break;
        case PRBSMODE_PRBS_NONE:
            pPrbsCfg->set_fac_prbs_mon_bool(wrapper::BOOL_FALSE);
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Unknown Fac Prbs Mon: " << adCfg.monitorTx;
            break;
    }

    // TermPrbsGen
    switch(adCfg.generatorRx)
    {
        case PRBSMODE_PRBS_X31:
            pPrbsCfg->set_term_prbs_gen_bool(wrapper::BOOL_TRUE);
            break;
        case PRBSMODE_PRBS_NONE:
            pPrbsCfg->set_term_prbs_gen_bool(wrapper::BOOL_FALSE);
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Unknown Term Prbs Gen: " << adCfg.generatorRx;
            break;
    }

    // TermPrbsMon
    switch(adCfg.monitorRx)
    {
        case PRBSMODE_PRBS_X31:
            pPrbsCfg->set_term_prbs_mon_bool(wrapper::BOOL_TRUE);
            break;
        case PRBSMODE_PRBS_NONE:
            pPrbsCfg->set_term_prbs_mon_bool(wrapper::BOOL_FALSE);
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Unknown Term Prbs Mon: " << adCfg.monitorRx;
            break;
    }

    // ServiceMode
    switch (adCfg.srvMode)
    {
        case DataPlane::SERVICEMODE_NONE:
            pInstallCfg->set_service_mode(hal_common::SERVICE_MODE_NONE);
            break;
        case DataPlane::SERVICEMODE_SWITCHING:
            pInstallCfg->set_service_mode(hal_common::SERVICE_MODE_SWITCHING);
            break;
        case DataPlane::SERVICEMODE_ADAPTATION:
            pInstallCfg->set_service_mode(hal_common::SERVICE_MODE_ADAPTATION);
            break;
        case PRBSMODE_UNSPECIFIED:
            break;
        default:
            pInstallCfg->set_service_mode(hal_common::SERVICE_MODE_NONE);
            break;
    }

    ::infinera::hal::dataplane::v2::OduMsConfig* pOduMsCfg = pInstallCfg->mutable_odu_ms_config();

    // FacForcedMs
    switch (adCfg.forceMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pOduMsCfg->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pOduMsCfg->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pOduMsCfg->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pOduMsCfg->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pOduMsCfg->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pOduMsCfg->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }

    // FacAutoMs
    switch (adCfg.autoMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pOduMsCfg->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pOduMsCfg->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pOduMsCfg->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pOduMsCfg->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pOduMsCfg->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pOduMsCfg->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }

    // TermForcedMs
    switch (adCfg.forceMsRx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pOduMsCfg->set_term_forceaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pOduMsCfg->set_term_forceaction(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pOduMsCfg->set_term_forceaction(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pOduMsCfg->set_term_forceaction(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pOduMsCfg->set_term_forceaction(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pOduMsCfg->set_term_forceaction(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }

    // TermAutoMs
    switch (adCfg.autoMsRx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pOduMsCfg->set_term_autoaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pOduMsCfg->set_term_autoaction(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pOduMsCfg->set_term_autoaction(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pOduMsCfg->set_term_autoaction(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pOduMsCfg->set_term_autoaction(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pOduMsCfg->set_term_autoaction(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }

    // TxTtiConfig
    pInstallCfg->mutable_tx_tti_cfg()->clear_tti();
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        hal_dataplane::OduState_OperationalState* stateHal = pbStat.mutable_hal();
        google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>* ttiMap = stateHal->mutable_installed_config()->mutable_tx_tti_cfg()->mutable_tti();

        // build key
        uint32_t ttiKey = index;

        // build data
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(adCfg.ttiTx[index]);

        (*ttiMap)[ttiKey] = ttiData;
    }

    // Ts
    pInstallCfg->mutable_timeslot()->Clear();
    google::protobuf::Map<uint32_t, hal_common::TsType_TsDataType >* tsMap = pInstallCfg->mutable_timeslot()->mutable_ts();
    for(uint32 idx = 0; idx < adCfg.tS.size(); idx++)
    {
        hal_common::TsType_TsDataType tsData;
        tsData.mutable_value()->set_value(adCfg.tS[idx]);
        (*tsMap)[idx] = tsData;
    }

    // TsGranularity
    switch(adCfg.tsg)
    {
        case DataPlane::TSGRANULARITY_25G:
            pInstallCfg->set_ts_granularity(hal_dataplane::TS_GRANULARITY_25G);
            break;
        default:
            break;
    }

}

void OduPbTranslator::statusAdToPb(const DpAdapter::OduStatus& adStat, Chm6OduState& pbStat) const
{
    // InternalFacRcvdTti
    pbStat.mutable_hal()->clear_internal_fac_rx_tti();
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        hal_dataplane::OduState_OperationalState* stateHal = pbStat.mutable_hal();
        google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>* ttiMap = stateHal->mutable_internal_fac_rx_tti()->mutable_tti();

        // build key
        uint32_t ttiKey = index;

        // build data
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(adStat.ttiRx[index]);
        (*ttiMap)[ttiKey] = ttiData;
    }

    //PayLoadType
    switch (adStat.odu.payLoad)
    {
        case DataPlane::ODUPAYLOAD_ODU4:
            pbStat.mutable_hal()->mutable_detected_payloadtype()->set_value(adStat.odu.payLoad);
            break;
        case DataPlane::ODUPAYLOAD_LO_ODU4I:
            pbStat.mutable_hal()->mutable_detected_payloadtype()->set_value(adStat.odu.payLoad);
            break;
        case DataPlane::ODUPAYLOAD_LO_FLEXI:
            pbStat.mutable_hal()->mutable_detected_payloadtype()->set_value(adStat.odu.payLoad);
            break;
        case DataPlane::ODUPAYLOAD_HO_ODUCNI:
            pbStat.mutable_hal()->mutable_detected_payloadtype()->set_value(adStat.odu.payLoad);
            break;
    }
}


void OduPbTranslator::statusAdToPb(const uint8_t* rxTtiPtr, Chm6OduState& pbStat) const
{
    // InternalFacRcvdTti
    pbStat.mutable_hal()->clear_internal_fac_rx_tti();
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        hal_dataplane::OduState_OperationalState* stateHal = pbStat.mutable_hal();
        google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>* ttiMap = stateHal->mutable_internal_fac_rx_tti()->mutable_tti();

        // build key
        uint32_t ttiKey = index;

        // build data
        hal_common::TtiType_TtiDataType ttiData;
        if (NULL == rxTtiPtr)
        {
            ttiData.mutable_value()->set_value(0);
        }
        else
        {
            ttiData.mutable_value()->set_value(rxTtiPtr[index]);
        }

        (*ttiMap)[ttiKey] = ttiData;
    }
}

// This goes to redis -> L1
void OduPbTranslator::statusAdToPb(const DpAdapter::Cdi cdi, Chm6OduState& pbStat) const
{
    hal_common::Cdi cdiValue;
    switch (cdi)
    {
    case DpAdapter::CDI_UNSPECIFIED:
        cdiValue = hal_common::CDI_UNSPECIFIED;
        break;
    case DpAdapter::CDI_LOCAL_DEGRADED:
        cdiValue = hal_common::CDI_LOCAL_DEGRADED;
        break;
    case DpAdapter::CDI_REMOTE_DEGRADED:
        cdiValue = hal_common::CDI_REMOTE_DEGRADED;
        break;
    case DpAdapter::CDI_NONE:
        cdiValue = hal_common::CDI_NONE;
        break;
    case DpAdapter::CDI_UNKNOWN:
        cdiValue = hal_common::CDI_UNKNOWN;
        break;
    case DpAdapter::CDI_LOCAL_AND_REMOTE_DEGRADED:
        cdiValue = hal_common::CDI_LOCAL_AND_REMOTE_DEGRADED;
        break;
    default:
        cdiValue = hal_common::CDI_UNKNOWN;
        break;
    }
    pbStat.mutable_hal()->set_cdi(cdiValue);

}


void OduPbTranslator::faultAdToPb(const std::string aid, const uint64_t& faultBmp, const DpAdapter::OduFault& adFault, Chm6OduFault& pbFault, bool force) const
{
    hal_dataplane::OduFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_odu_faults();

    uint64_t faultBmpNew = adFault.facBmp;
    uint64_t bitMask = 1ULL;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if (((faultBmp ^ faultBmpNew) & bitMask) || (true == force))
        {
            addOduFaultPair(vFault->faultName, vFault->direction, vFault->location, vFault->faulted, pbFault);
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

bool OduPbTranslator::isOduHardFault(const DpAdapter::OduFault& adFault, const FaultDirection direction) const
{
    bool isHardFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if (FAULTDIRECTION_RX == direction)
        {
            if ((vFault->faultName == oduFaultParamsToString.at(ODU_FAULT_LOFLOM_INGRESS)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true;
                break;
            }
            if ((vFault->faultName == oduFaultParamsToString.at(ODU_FAULT_AIS)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true;
                break;
            }
            if ((vFault->faultName == oduFaultParamsToString.at(ODU_FAULT_OCI)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true;
                break;
            }
            if ((vFault->faultName == oduFaultParamsToString.at(ODU_FAULT_LCK)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true;
                break;
            }
        }
        else if (FAULTDIRECTION_TX == direction)
        {
            if ((vFault->faultName == oduFaultParamsToString.at(ODU_FAULT_LOFLOM_EGRESS)) &&
                (vFault->direction == FAULTDIRECTION_TX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true;
                break;
            }
        }
    }

    return isHardFault;
}

void OduPbTranslator::addOduTimFault(bool faulted, DpAdapter::OduFault& oduFault) const
{
    uint bitPos = 0;
    for (auto i = oduFault.fac.begin(); i != oduFault.fac.end(); i++)
    {
        bitPos++;
    }

    DpAdapter::AdapterFault tmpFault;
    tmpFault.faultName = oduFaultParamsToString.at(ODU_FAULT_TIM);
    tmpFault.direction = FAULTDIRECTION_RX;
    tmpFault.location = FAULTLOCATION_NEAR_END;
    tmpFault.faulted = faulted;
    oduFault.fac.push_back(tmpFault);

    if (faulted)
    {
        oduFault.facBmp |= 1ULL << bitPos;
    }
    else
    {
        oduFault.facBmp &= ~(1ULL << bitPos);
    }
}

void OduPbTranslator::addOduSdFault(bool faulted, DpAdapter::OduFault& oduFault, FaultDirection direction) const
{
    uint bitPos = 0;
    bool found = false;

    DpAdapter::AdapterFault tmpFault;
    tmpFault.faultName = oduFaultParamsToString.at(ODU_FAULT_SD);
    tmpFault.direction = direction;
    tmpFault.location = FAULTLOCATION_NEAR_END;
    tmpFault.faulted = faulted;

    for (auto i = oduFault.fac.begin(); i != oduFault.fac.end(); i++)
    {
        // WORKAROUND:
        // DCO reporting SD-EGRESS-NEAR when CHM6
        // is suppose to report instead. Overwrite
        // DCO reported fault for now.
        if ((i->faultName == tmpFault.faultName) &&
            (i->direction == tmpFault.direction) &&
            (i->location  == tmpFault.location))
        {
            found = true;
            break;
        }

        bitPos++;
    }

    if (true == found)
    {
        oduFault.fac[bitPos].faulted = faulted;
    }
    else
    {
        oduFault.fac.push_back(tmpFault);
    }

    if (faulted)
    {
        oduFault.facBmp |= 1ULL << bitPos;
    }
    else
    {
        oduFault.facBmp &= ~(1ULL << bitPos);
    }
}

void OduPbTranslator::addOduAisOnLoflomMsim(DpAdapter::OduFault& oduFault) const
{
    // Check if LOFLOM or MSIM reported
    bool isLoflomMsim = false;
    for (auto i = oduFault.fac.begin(); i != oduFault.fac.end(); i++)
    {
        if ((i->faultName == oduFaultParamsToString.at(ODU_FAULT_MSIM)) &&
            (i->direction == FAULTDIRECTION_RX) &&
            (i->location  == FAULTLOCATION_NEAR_END) &&
            (i->faulted   == true))
        {
            isLoflomMsim = true;
            break;
        }

        if ((i->faultName == oduFaultParamsToString.at(ODU_FAULT_LOFLOM_INGRESS)) &&
            (i->direction == FAULTDIRECTION_RX) &&
            (i->location  == FAULTLOCATION_NEAR_END) &&
            (i->faulted   == true))
        {
            isLoflomMsim = true;
            break;
        }
    }

    if (true == isLoflomMsim)
    {
        DpAdapter::AdapterFault tmpFault;
        tmpFault.faultName = oduFaultParamsToString.at(ODU_FAULT_AIS);
        tmpFault.direction = FAULTDIRECTION_RX;
        tmpFault.location = FAULTLOCATION_NEAR_END;
        tmpFault.faulted = true;

        uint bitPos = 0;
        bool found = false;

        for (auto i = oduFault.fac.begin(); i != oduFault.fac.end(); i++)
        {
            // Overwrite DCO reported AIS
            if ((i->faultName == tmpFault.faultName) &&
                (i->direction == tmpFault.direction) &&
                (i->location  == tmpFault.location))
            {
                found = true;
                break;
            }

            bitPos++;
        }

        if (true == found)
        {
            oduFault.fac[bitPos].faulted = true;
        }
        else
        {
            oduFault.fac.push_back(tmpFault);
        }

        oduFault.facBmp |= 1ULL << bitPos;
    }
}

void OduPbTranslator::addOduFaultPair(std::string faultName, FaultDirection faultDirection, FaultLocation faultLocation, bool value, Chm6OduFault& pbFault) const
{
    hal_dataplane::OduFault_OperationalFault* faultHal = pbFault.mutable_hal();
    google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* faultMap = faultHal->mutable_odu_faults()->mutable_fault();

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

    if (faultName == oduFaultParamsToString.at(ODU_FAULT_TIM))
    {
        faultData.set_service_affecting(hal_common::SERVICE_AFFECTING_DEMOTE);
    }

    (*faultMap)[faultKey] = faultData;
}

void OduPbTranslator::faultAdToPbSim(const uint64_t& fault, Chm6OduFault& pbFault) const
{
    hal_dataplane::OduFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_odu_faults();

    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_AIS)           , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_AIS))           , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_BDI)           , FAULTDIRECTION_RX, FAULTLOCATION_FAR_END , (bool)(fault & (1UL << ODU_FAULT_BDI))           , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_CSF)           , FAULTDIRECTION_RX, FAULTLOCATION_FAR_END , (bool)(fault & (1UL << ODU_FAULT_CSF))           , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_LCK)           , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_LCK))           , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_LOFLOM_INGRESS), FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_LOFLOM_INGRESS)), pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_MSIM)          , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_MSIM))          , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_OCI)           , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_OCI))           , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_PARTFAIL)      , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_PARTFAIL))      , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_PLM)           , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_PLM))           , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_SD)            , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_SD))            , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_TIM)           , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_TIM))           , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_LOFLOM_EGRESS) , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_LOFLOM_EGRESS)) , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_PRBS_OOS_EGRESS    ) , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_PRBS_OOS_EGRESS    )) , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_PRBS_OOS_INGRESS   ) , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_PRBS_OOS_INGRESS   )) , pbFault);
    addOduFaultPair(oduFaultParamsToString.at(ODU_FAULT_SSF          ) , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << ODU_FAULT_SSF          )) , pbFault);
}

void OduPbTranslator::addOduPmPair(ODU_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, uint64_t value, bool validity, Chm6OduPm& pbPm) const
{
    hal_dataplane::OduPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_odu_fac_pm()->mutable_pm();

    std::string pmKey(oduPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(oduPmParamsToString.at(pmParam));

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

void OduPbTranslator::pmAdToPb(const DataPlane::DpMsOduPMContainer& adPm, bool isOtuLof, bool validity, Chm6OduPm& pbPm) const
{
    hal_dataplane::OduPm_OperationalPm* pmHal = pbPm.mutable_hal();
    pmHal->clear_odu_fac_pm();

    uint64_t undefinedField = 0;

    addOduPmPair(ODU_PM_ERRORED_BLOCKS_NEAR, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.err_blocks        , validity, pbPm);
    addOduPmPair(ODU_PM_ERRORED_BLOCKS_FAR , PMDIRECTION_RX, PMLOCATION_FAR_END , adPm.rx.far_end_err_blocks, validity, pbPm);
    if (adPm.aid.find("-T") != std::string::npos) //client odu case
    {
	    addOduPmPair(ODU_PM_ERRORED_BLOCKS_NEAR,       PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.err_blocks         , validity, pbPm);
	    addOduPmPair(ODU_PM_ERRORED_BLOCKS_FAR ,       PMDIRECTION_TX, PMLOCATION_FAR_END , adPm.tx.far_end_err_blocks , validity, pbPm);
	    addOduPmPair(ODU_PM_PRBS_ERRORS_EGRESS ,       PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.prbs_err_count     , validity, pbPm);
	    addOduPmPair(ODU_PM_PRBS_SYNC_ERRORS_EGRESS  , PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.tx.prbs_sync_err_count, validity, pbPm);

            if (true == isOtuLof)
            {
                uint64_t value = 0;
                addOduPmPair(ODU_PM_PRBS_ERRORS_INGRESS,       PMDIRECTION_RX, PMLOCATION_NEAR_END, value, false, pbPm);
                addOduPmPair(ODU_PM_PRBS_SYNC_ERRORS_INGRESS , PMDIRECTION_RX, PMLOCATION_NEAR_END, value, false, pbPm);
            }
            else
            {
                addOduPmPair(ODU_PM_PRBS_ERRORS_INGRESS,       PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.prbs_err_count,      validity, pbPm);
                addOduPmPair(ODU_PM_PRBS_SYNC_ERRORS_INGRESS , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.prbs_sync_err_count, validity, pbPm);
            }
    }
}

void OduPbTranslator::pmAdDefault(DataPlane::DpMsOduPMContainer& adPm)
{
    memset(&adPm.rx, 0, sizeof(adPm.rx));
    memset(&adPm.tx, 0, sizeof(adPm.tx));
}

bool OduPbTranslator::isPbChanged(const chm6_dataplane::Chm6OduState& pbStatPrev, const chm6_dataplane::Chm6OduState& pbStatCur) const
{
    if (pbStatPrev.hal().internal_fac_rx_tti().tti_size() != pbStatCur.hal().internal_fac_rx_tti().tti_size())
    {
        return true;
    }

    for (uint index = 0; index < pbStatPrev.hal().internal_fac_rx_tti().tti_size(); index++)
    {
        google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>::const_iterator itPrev;
        google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>::const_iterator itCur;

        uint32_t ttiKey = index;

        itPrev = pbStatPrev.hal().internal_fac_rx_tti().tti().find(ttiKey);
        if (itPrev == pbStatPrev.hal().internal_fac_rx_tti().tti().end())
        {
            INFN_LOG(SeverityLevel::error) << "Odu RxTtti key = " << ttiKey << " not found in previous map";
            return true;
        }

        itCur  = pbStatCur.hal().internal_fac_rx_tti().tti().find(ttiKey);
        if (itCur == pbStatCur.hal().internal_fac_rx_tti().tti().end())
        {
            INFN_LOG(SeverityLevel::error) << "Odu RxTtti key = " << ttiKey << " not found in current map";
            return true;
        }

        if (itPrev->second.value().value() != itCur->second.value().value())
        {
            INFN_LOG(SeverityLevel::debug) << "Odu RxTtti changed: Byte" << index
                           << ": Prev = 0x" << std::hex << itPrev->second.value().value() << std::dec
                           << ", Cur = 0x"  << std::hex << itCur->second.value().value()  << std::dec;
            return true;
        }
    }

    // Compare new/old CDI enums
    if (pbStatPrev.hal().cdi() != pbStatCur.hal().cdi())
    {
        INFN_LOG(SeverityLevel::debug) << "CDI changed prev:" << toString(pbStatPrev.hal().cdi()) <<
                                         " cur:" << toString(pbStatCur.hal().cdi());
        return true;
    }

    if (pbStatPrev.hal().detected_payloadtype().value() != pbStatCur.hal().detected_payloadtype().value())
    {
        return true;
    }

    return false;
}

bool OduPbTranslator::isAdChanged(const DpAdapter::OduFault& adFaultPrev, const DpAdapter::OduFault& adFaultCur) const
{
    if (adFaultPrev.facBmp != adFaultCur.facBmp)
    {
        return true;
    }

    return false;
}

int OduPbTranslator::aidToOduId(std::string aid, chm6_dataplane::Chm6OduConfig& oduCfg) const
{
    // Based on DCO aid to odu id conversion without dependency on cached information

    int aidId = -1;

    // Example: aid = 1-4-L1-1 or 1-4-L2-1  High-order Odu
    //                1-4-L1-1-ODU4i-X      Low-order Odu: X=1..16

    std::string cliOdu = "Odu ";
    std::size_t found = aid.find(cliOdu);

    if (found != std::string::npos)
    {
        std::string sId = aid.substr(found+cliOdu.length());
        return (stoi(sId,nullptr));
    }

    if (false == oduCfg.hal().has_odu_index())
    {
        INFN_LOG(SeverityLevel::error) << "Missing Odu Index";
        return aidId;
    }

    uint loOduId = oduCfg.hal().odu_index().value();
    if (loOduId < 1 || loOduId > MAX_ODU_ID)
    {
        INFN_LOG(SeverityLevel::error) << "Bad Odu Id: " << loOduId;
        return aidId;
    }

    // As agreed with L1 agent we use the LoOduId in config instead of AID
    // Cached it to use in update and delete operation
    // AID based on 1.25G time slot need to cached AID as we don't keep
    // track of 1.25 TS
    // AID 1-4-L1-1-ODUflexi-81
    // AID 1-5-L1-1-ODU4i-81

    aidId = loOduId;

    return (aidId);
}


}; // namespace DataPlane
