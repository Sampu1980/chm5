/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
 
#include "OtuPbTranslator.h"
#include "InfnLogger.h"

#include <iostream>
#include <sstream>


using ::chm6_dataplane::Chm6OtuState;
using ::chm6_dataplane::Chm6OtuConfig;
using ::chm6_dataplane::Chm6OtuFault;
using ::chm6_dataplane::Chm6OtuPm;

using namespace ::std;


namespace DataPlane {

#define OTU_PM_PROP_DELAY_TOLERANCE  0.001


void OtuPbTranslator::configPbToAd(const Chm6OtuConfig& pbCfg, DpAdapter::OtuCommon& adCfg) const
{
    std::string otuAid = pbCfg.base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid;

    // ConfigSvcType - Not modifiable
    if (hal_common::BAND_WIDTH_UNSPECIFIED != pbCfg.hal().config_svc_type())
    {
        hal_common::BandWidth bandwidth = pbCfg.hal().config_svc_type();
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " ConfigSvcType: " << toString(bandwidth);
        switch (bandwidth)
        {
            case hal_common::BAND_WIDTH_OTUCNI:
                adCfg.payLoad = DataPlane::OTUPAYLOAD_OTUCNI;
                adCfg.type = DataPlane::OTUSUBTYPE_LINE;
                break;
            case hal_common::BAND_WIDTH_OTU4:
                adCfg.payLoad = DataPlane::OTUPAYLOAD_OTU4;
                adCfg.type = DataPlane::OTUSUBTYPE_CLIENT;
                break;
            default:
                break;
        }
    }

    // ServiceMode - Not modifiable
    if (hal_common::SERVICE_MODE_UNSPECIFIED != pbCfg.hal().service_mode())
    {
        hal_common::ServiceMode serviceMode = pbCfg.hal().service_mode();
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " ServiceMode: " << toString(serviceMode);
        switch (serviceMode)
        {
            case hal_common::SERVICE_MODE_NONE:
                adCfg.srvMode = DataPlane::SERVICEMODE_NONE;
                break;
            case hal_common::SERVICE_MODE_SWITCHING:
                adCfg.srvMode = DataPlane::SERVICEMODE_SWITCHING;
                break;
            case hal_common::SERVICE_MODE_ADAPTATION:
                adCfg.srvMode = DataPlane::SERVICEMODE_ADAPTATION;
                break;
            case hal_common::SERVICE_MODE_TRANSPORT:
                adCfg.srvMode = DataPlane::SERVICEMODE_TRANSPORT;
                break;
            case hal_common::SERVICE_MODE_WRAPPER:
            case hal_common::SERVICE_MODE_MUXED:
            default:
                adCfg.srvMode = DataPlane::SERVICEMODE_NONE;
                break;
        }
    }

    // OtuMsConfig
    if (pbCfg.hal().has_otu_ms_config())
    {
        // FacMs
        if (pbCfg.hal().otu_ms_config().has_fac_ms())
        {
            // ForcedMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().otu_ms_config().fac_ms().forced_ms())
            {
                hal_common::MaintenanceAction forcedMs = pbCfg.hal().otu_ms_config().fac_ms().forced_ms();
                INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TxForcedMs: " << toString(forcedMs);
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

            // AutoMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().otu_ms_config().fac_ms().auto_ms())
            {
                hal_common::MaintenanceAction autoMs = pbCfg.hal().otu_ms_config().fac_ms().auto_ms();
                INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TxAutoMs: " << toString(autoMs);
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
        }

        // TermMs
        if (pbCfg.hal().otu_ms_config().has_term_ms())
        {
            // ForcedMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().otu_ms_config().term_ms().forced_ms())
            {
                hal_common::MaintenanceAction forcedMs = pbCfg.hal().otu_ms_config().term_ms().forced_ms();
                INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " RxForcedMs: " << toString(forcedMs);
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

            // AutoMs
            if (hal_common::MAINTENANCE_ACTION_UNSPECIFIED != pbCfg.hal().otu_ms_config().term_ms().auto_ms())
            {
                hal_common::MaintenanceAction autoMs = pbCfg.hal().otu_ms_config().term_ms().auto_ms();
                INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " RxAutoMs: " << toString(autoMs);
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
    }

    // Loopback
    if (hal_common::LOOPBACK_TYPE_UNSPECIFIED != pbCfg.hal().loopback())
    {
        hal_common::LoopbackType lpbk = pbCfg.hal().loopback();
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " Loopback: " << toString(lpbk);
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

    // LoopbackBehavior
    if (hal_common::TERM_LOOPBACK_BEHAVIOR_UNSPECIFIED != pbCfg.hal().loopback_behavior())
    {
        hal_common::TermLoopbackBehavior lpbkBehavior = pbCfg.hal().loopback_behavior();
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " LoopbackBehavior: " << toString(lpbkBehavior);
        switch (lpbkBehavior)
        {
            case hal_common::TERM_LOOPBACK_BEHAVIOR_BRIDGESIGNAL:
                // ?
                break;
            case hal_common::TERM_LOOPBACK_BEHAVIOR_TURNOFFLASER:
                // ?
                break;
            default:
                break;
        }
    }

    // OtuPrbsConfig
    if (pbCfg.hal().has_otu_prbs_config())
    {
        // FacPrbsGen
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().otu_prbs_config().fac_prbs_gen_bool())
        {
            wrapper::Bool generatorTx = pbCfg.hal().otu_prbs_config().fac_prbs_gen_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " FacPrbsGen: " << toString(generatorTx);
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
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().otu_prbs_config().fac_prbs_mon_bool())
        {
            wrapper::Bool monitorTx = pbCfg.hal().otu_prbs_config().fac_prbs_mon_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " FacPrbsMon: " << toString(monitorTx);
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
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().otu_prbs_config().term_prbs_gen_bool())
        {
            wrapper::Bool generatorRx = pbCfg.hal().otu_prbs_config().term_prbs_gen_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TermPrbsGen: " << toString(generatorRx);
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
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().otu_prbs_config().term_prbs_mon_bool())
        {
            wrapper::Bool monitorRx = pbCfg.hal().otu_prbs_config().term_prbs_mon_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TermPrbsMon: " << toString(monitorRx);
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

    if (hal_common::CARRIER_CHANNEL_UNSPECIFIED != pbCfg.hal().carrier_channel())
    {
        hal_common::CarrierChannel carrierChannel = pbCfg.hal().carrier_channel();
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " CarrierChannel: " << toString(carrierChannel);
        switch (carrierChannel)
        {
            case hal_common::CARRIER_CHANNEL_ONE:
                adCfg.channel = DataPlane::CARRIERCHANNEL_ONE;
                break;
            case hal_common::CARRIER_CHANNEL_TWO:
                adCfg.channel = DataPlane::CARRIERCHANNEL_TWO;
                break;
            case hal_common::CARRIER_CHANNEL_BOTH:
                adCfg.channel = DataPlane::CARRIERCHANNEL_BOTH;
                break;
            default:
                break;
        }
    }

    if (hal_dataplane::FEC_MODE_UNSPECIFIED != pbCfg.hal().fec_gen_mode())
    {
        hal_dataplane::FecMode fecGenMode = pbCfg.hal().fec_gen_mode();
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " FecGenMode: " << toString(fecGenMode);
        if (hal_dataplane::FEC_MODE_G709 == fecGenMode)
        {
            adCfg.fecGenEnable = true;
        }
        else
        {
            adCfg.fecGenEnable = false;
        }
    }

    if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().fec_correction())
    {
        wrapper::Bool fecCorrection = pbCfg.hal().fec_correction();
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " FecCorrection: " << toString(fecCorrection);
        switch (fecCorrection)
        {
            case wrapper::BOOL_TRUE:
                adCfg.fecCorrEnable = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.fecCorrEnable = false;
                break;
            default:
                break;
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
                INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TxTtiConfig invalid index = " << it->second.value().value();
            }
        }

        std::ostringstream logSapi;
        for (uint i = 0; i < 16; i++)
        {
            logSapi << " 0x" << std::hex << (int)adCfg.ttiTx[i] << std::dec;
        }
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TxTtiSAPI =" << logSapi.str();

        std::ostringstream logDapi;
        for (uint i = 16; i < 32; i++)
        {
            logDapi << " 0x" << std::hex << (int)adCfg.ttiTx[i] << std::dec;
        }
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TxTtiDAPI =" << logDapi.str();

        std::ostringstream logOper;
        for (uint i = 32; i < MAX_TTI_LENGTH; i++)
        {
            logOper << " 0x" << std::hex << (int)adCfg.ttiTx[i] << std::dec;
        }
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TxTtiOPER =" << logOper.str();
    }

    // Alarm Threshold
    if (pbCfg.hal().has_alarm_threshold())
    {
        // Signal Degrade Interval
        if (pbCfg.hal().alarm_threshold().has_signal_degrade_interval())
        {
            uint interval = pbCfg.hal().alarm_threshold().signal_degrade_interval().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " SignalDegradeInterval: " << interval;
            // Used by Dataplane Mgr, not by DCO Adapter
        }

        // Signal Degrade Threshold
        if (pbCfg.hal().alarm_threshold().has_signal_degrade_threshold())
        {
            uint interval = pbCfg.hal().alarm_threshold().signal_degrade_threshold().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " SignalDegradeThreshold: " << interval;
            // Used by Dataplane Mgr, not by DCO Adapter
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
                    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TtiExpectedConfig invalid index = " << it->second.value().value();
                }
            }

            std::ostringstream logSapi;
            for (uint i = 0; i < 16; i++)
            {
                logSapi << " 0x" << std::hex << (int)ttiExpect[i] << std::dec;
            }
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " ExpectedTtiSAPI =" << logSapi.str();

            std::ostringstream logDapi;
            for (uint i = 16; i < 32; i++)
            {
                logDapi << " 0x" << std::hex << (int)ttiExpect[i] << std::dec;
            }
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " ExpectedTtiDAPI =" << logDapi.str();

            std::ostringstream logOper;
            for (uint i = 32; i < MAX_TTI_LENGTH; i++)
            {
                logOper << " 0x" << std::hex << (int)ttiExpect[i] << std::dec;
            }
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " ExpectedTtiOPER =" << logOper.str();
            // Used by Dataplane Mgr, not by DCO Adapter
        }

        // TTI Mismatch Type
        if (hal_common::TTI_MISMATCH_TYPE_UNSPECIFIED != pbCfg.hal().alarm_threshold().tti_mismatch_type())
        {
            hal_common::TtiMismatchType mismatchType = pbCfg.hal().alarm_threshold().tti_mismatch_type();
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " TtiMismatchType: " << toString(mismatchType);
            // Used by Dataplane Mgr, not by DCO Adapter
        }
    }
}

void OtuPbTranslator::configAdToPb(const DpAdapter::OtuCommon& adCfg, Chm6OtuState& pbStat) const
{
    std::string configId = pbStat.base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << configId;

    ::infinera::hal::dataplane::v2::OtuConfig_Config* pInstallCfg = pbStat.mutable_hal()->mutable_installed_config();

    // ConfigSvcType
    switch (adCfg.payLoad)
    {
        case DataPlane::OTUPAYLOAD_OTUCNI:
            pInstallCfg->set_config_svc_type(hal_common::BAND_WIDTH_OTUCNI);
            break;
        case DataPlane::OTUPAYLOAD_OTU4:
            pInstallCfg->set_config_svc_type(hal_common::BAND_WIDTH_OTU4);
            break;
        default:
            pInstallCfg->set_config_svc_type(hal_common::BAND_WIDTH_UNSPECIFIED);
            break;
    }

    // ServiceMode - Not modifiable
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
        default:        
            pInstallCfg->set_service_mode(hal_common::SERVICE_MODE_NONE);
            break;
    }

    // FacForcedMs
    switch (adCfg.forceMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }

    // FacAutoMs
    switch (adCfg.autoMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pInstallCfg->mutable_otu_ms_config()->mutable_fac_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }


    // TermForcedMs
    switch (adCfg.forceMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }

    // TermAutoMs
    switch (adCfg.autoMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_AIS:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_SENDAIS);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_OCI:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_SENDOCI);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LCK:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_SENDLCK);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_PRBS_X31_INV:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_PRBS_X31_INV);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pInstallCfg->mutable_otu_ms_config()->mutable_term_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
            break;
        default:
            break;
    }

    // Loopback
    switch (adCfg.lpbk)
    {
        case DataPlane::LOOP_BACK_TYPE_OFF:
            pInstallCfg->set_loopback(hal_common::LOOPBACK_TYPE_NONE);
            break;
        case DataPlane::LOOP_BACK_TYPE_TERMINAL:
            pInstallCfg->set_loopback(hal_common::LOOPBACK_TYPE_TERMINAL);
            break;
        case DataPlane::LOOP_BACK_TYPE_FACILITY:
            pInstallCfg->set_loopback(hal_common::LOOPBACK_TYPE_FACILITY);
            break;
        default:
            break;
    }

    ::infinera::hal::dataplane::v2::OtuPrbsConfig* pPrbsCfg = pInstallCfg->mutable_otu_prbs_config();

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

    // CarrierChannel
    switch (adCfg.channel)
    {
        case DataPlane::CARRIERCHANNEL_ONE: 
            pInstallCfg->set_carrier_channel(hal_common::CARRIER_CHANNEL_ONE);
            break;
        case DataPlane::CARRIERCHANNEL_TWO: 
            pInstallCfg->set_carrier_channel(hal_common::CARRIER_CHANNEL_TWO);
            break;
        case DataPlane::CARRIERCHANNEL_BOTH: 
            pInstallCfg->set_carrier_channel(hal_common::CARRIER_CHANNEL_BOTH);
            break;
        default:
            break;
    }

    // FecGenMode
    if (true == adCfg.fecGenEnable)
    {
        pInstallCfg->set_fec_gen_mode(hal_dataplane::FEC_MODE_G709);
    }
    else
    {
        pInstallCfg->set_fec_gen_mode(hal_dataplane::FEC_MODE_DISABLE_FEC);
    }

    // FecCorrection
    pInstallCfg->set_fec_correction(adCfg.fecCorrEnable ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);

    // TxTtiConfig
    pInstallCfg->mutable_tx_tti_cfg()->clear_tti();
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        hal_dataplane::OtuState_OperationalState* stateHal = pbStat.mutable_hal();
        google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>* ttiMap = stateHal->mutable_installed_config()->mutable_tx_tti_cfg()->mutable_tti();

        // build key
        uint32_t ttiKey = index;

        // build data
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(adCfg.ttiTx[index]);

        (*ttiMap)[ttiKey] = ttiData;
    }

}

void OtuPbTranslator::statusAdToPb(const DpAdapter::OtuStatus& adStat, Chm6OtuState& pbStat) const
{
    // InternalFacRcvdTti
    pbStat.mutable_hal()->clear_internal_fac_rx_tti();
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        hal_dataplane::OtuState_OperationalState* stateHal = pbStat.mutable_hal();
        google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>* ttiMap = stateHal->mutable_internal_fac_rx_tti()->mutable_tti();

        // build key
        uint32_t ttiKey = index;

        // build data
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(adStat.ttiRx[index]);

        (*ttiMap)[ttiKey] = ttiData;
    }
}

void OtuPbTranslator::statusAdToPb(const uint8_t* rxTtiPtr, Chm6OtuState& pbStat) const
{
    // InternalFacRcvdTti
    pbStat.mutable_hal()->clear_internal_fac_rx_tti();
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        hal_dataplane::OtuState_OperationalState* stateHal = pbStat.mutable_hal();
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

void OtuPbTranslator::faultAdToPb(const std::string aid, const uint64_t& faultBmp, const DpAdapter::OtuFault& adFault, Chm6OtuFault& pbFault, bool force) const
{
    hal_dataplane::OtuFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_otu_faults();

    uint64_t faultBmpNew = adFault.facBmp;
    uint64_t bitMask = 1ULL;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if (((faultBmp ^ faultBmpNew) & bitMask) || (true == force))
        {
            addOtuFaultPair(vFault->faultName, vFault->direction, vFault->location, vFault->faulted, pbFault);
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

bool OtuPbTranslator::isOtuHardFault(const DpAdapter::OtuFault& adFault, const FaultDirection direction) const
{
    bool isHardFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if (FAULTDIRECTION_RX == direction)
        {
            // 100GbE,400GbE Faults
            if ((vFault->faultName == otuFaultParamsToString.at(OTU_FAULT_LOFLOM)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true; 
                break;
            }
            if ((vFault->faultName == otuFaultParamsToString.at(OTU_FAULT_AIS)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true; 
                break;
            }

            // OTU4 Faults
            if ((vFault->faultName == otuFaultParamsToString.at(OTU_FAULT_LOF)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true; 
                break;
            }
            if ((vFault->faultName == otuFaultParamsToString.at(OTU_FAULT_LOM)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true; 
                break;
            }
            if ((vFault->faultName == otuFaultParamsToString.at(OTU_FAULT_LOFLANE)) &&
                (vFault->direction == FAULTDIRECTION_RX) &&
                (vFault->location  == FAULTLOCATION_NEAR_END) &&
                (vFault->faulted == true))
            {
                isHardFault = true; 
                break;
            }
            if ((vFault->faultName == otuFaultParamsToString.at(OTU_FAULT_LOL)) &&
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
        }
    } 

    return isHardFault;
}

bool OtuPbTranslator::isOtuIaeFault(const DpAdapter::OtuFault& adFault, const FaultDirection direction) const
{
    bool isIaeFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if ((vFault->faultName == otuFaultParamsToString.at(OTU_FAULT_IAE)) &&
            (vFault->direction == direction) &&
            (vFault->location  == FAULTLOCATION_NEAR_END) &&
            (vFault->faulted == true))
        {
            isIaeFault = true; 
            break;
        }
    }

    return isIaeFault;
}

bool OtuPbTranslator::isOtuLofFault(const DpAdapter::OtuFault& adFault, const FaultDirection direction) const
{
    bool isLofFault = false;

    for (auto i = adFault.fac.begin(); i != adFault.fac.end(); i++)
    {
        if ((i->faultName == otuFaultParamsToString.at(OTU_FAULT_LOF)) &&
            (i->direction == direction) &&
            (i->location  == FAULTLOCATION_NEAR_END) &&
            (i->faulted   == true))
        {
            isLofFault = true; 
            break;
        }
    }

    return isLofFault;
}

void OtuPbTranslator::addOtuTimFault(bool faulted, DpAdapter::OtuFault& otuFault) const
{
    uint bitPos = 0;
    for (auto i = otuFault.fac.begin(); i != otuFault.fac.end(); i++)
    {
        bitPos++;
    }

    DpAdapter::AdapterFault tmpFault;
    tmpFault.faultName = otuFaultParamsToString.at(OTU_FAULT_TIM);
    tmpFault.direction = FAULTDIRECTION_RX;
    tmpFault.location = FAULTLOCATION_NEAR_END; 
    tmpFault.faulted = faulted;
    otuFault.fac.push_back(tmpFault);

    if (faulted)
    { 
        otuFault.facBmp |= 1ULL << bitPos;
    }
    else
    {
        otuFault.facBmp &= ~(1ULL << bitPos);
    }
}

void OtuPbTranslator::addOtuSdFault(bool faulted, DpAdapter::OtuFault& otuFault) const
{
    uint bitPos = 0;

    for (auto i = otuFault.fac.begin(); i != otuFault.fac.end(); i++)
    {
        bitPos++;
    }

    DpAdapter::AdapterFault tmpFault;
    tmpFault.faultName = otuFaultParamsToString.at(OTU_FAULT_SD);
    tmpFault.direction = FAULTDIRECTION_RX;
    tmpFault.location = FAULTLOCATION_NEAR_END;
    tmpFault.faulted = faulted;
    otuFault.fac.push_back(tmpFault);

    if (faulted)
    { 
        otuFault.facBmp |= 1ULL << bitPos;
    }
    else
    {
        otuFault.facBmp &= ~(1ULL << bitPos);
    }
}

void OtuPbTranslator::addOtuFaultPair(std::string faultName, FaultDirection faultDirection, FaultLocation faultLocation, bool value, Chm6OtuFault& pbFault) const
{
    hal_dataplane::OtuFault_OperationalFault* faultHal = pbFault.mutable_hal();
    google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* faultMap = faultHal->mutable_otu_faults()->mutable_fault();

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

    if (faultName == otuFaultParamsToString.at(OTU_FAULT_TIM))
    {
        faultData.set_service_affecting(hal_common::SERVICE_AFFECTING_DEMOTE);
    }

    (*faultMap)[faultKey] = faultData;
}

void OtuPbTranslator::faultAdToPbSim(const uint64_t& fault, Chm6OtuFault& pbFault) const
{
    hal_dataplane::OtuFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_otu_faults();

    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_AIS)     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_AIS))     , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_BDI)     , FAULTDIRECTION_RX, FAULTLOCATION_FAR_END , (bool)(fault & (1ULL << OTU_FAULT_BDI))     , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_LOFLOM)  , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_LOFLOM))  , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_PARTFAIL), FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_PARTFAIL)), pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_SD)      , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_SD))      , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_TIM)     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_TIM))     , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_LOF)     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_LOF))     , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_LOM)     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_LOM))     , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_LOFLANE) , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_LOFLANE)) , pbFault);
    addOtuFaultPair(otuFaultParamsToString.at(OTU_FAULT_LOL)     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1ULL << OTU_FAULT_LOL))     , pbFault);
}

void OtuPbTranslator::addOtuPmPair(OTU_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, uint64_t value, bool validity, Chm6OtuPm& pbPm) const
{
    hal_dataplane::OtuPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_otu_fac_pm()->mutable_pm();

    std::string pmKey(otuPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(otuPmParamsToString.at(pmParam));

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

void OtuPbTranslator::addOtuPmPair(OTU_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, double value, bool validity, Chm6OtuPm& pbPm) const
{
    hal_dataplane::OtuPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_otu_fac_pm()->mutable_pm();

    std::string pmKey(otuPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(otuPmParamsToString.at(pmParam));

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

void OtuPbTranslator::pmAdToPb(const DataPlane::DpMsOtuPMSContainer& adPm, bool fecCorrEnable, bool otuTimFault, bool validity, Chm6OtuPm& pbPm) const
{
    hal_dataplane::OtuPm_OperationalPm* pmHal = pbPm.mutable_hal();
    pmHal->clear_otu_fac_pm();

    uint64_t undefinedField = 0;

    addOtuPmPair(OTU_PM_ERRORED_BLOCKS_NEAR, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.pmcommon.err_blocks         , validity, pbPm);
    addOtuPmPair(OTU_PM_ERRORED_BLOCKS_FAR , PMDIRECTION_RX, PMLOCATION_FAR_END , adPm.rx.pmcommon.far_end_err_blocks , validity, pbPm);

    bool isValid = ((fabs(adPm.rx.pmcommon.latency) < OTU_PM_PROP_DELAY_TOLERANCE) ? false : true);
    addOtuPmPair(OTU_PM_PROPAGATION_DELAY  , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.pmcommon.latency, isValid, pbPm);
    if (adPm.aid.find("-T") != std::string::npos) //client otu case
    {
        if ((true == fecCorrEnable) && (0 != adPm.rx.fecstat.uncorrected_words))
        {
            addOtuPmPair(OTU_PM_PRE_FEC_BER, PMDIRECTION_RX, PMLOCATION_NEAR_END, 0.0, false, pbPm);
        }
        else
        {
            addOtuPmPair(OTU_PM_PRE_FEC_BER, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.fecstat.pre_fec_ber, validity, pbPm);
        }

        if (true == fecCorrEnable)
        {
            addOtuPmPair(OTU_PM_UNCORRECTED_WORDS,  PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.fecstat.uncorrected_words, validity, pbPm);
            addOtuPmPair(OTU_PM_CORRECTED_BITS,     PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.fecstat.corrected_bits   , validity, pbPm);
        }
        else
        {
            addOtuPmPair(OTU_PM_UNCORRECTED_WORDS,  PMDIRECTION_RX, PMLOCATION_NEAR_END, 0.0, false, pbPm);
            addOtuPmPair(OTU_PM_CORRECTED_BITS,     PMDIRECTION_RX, PMLOCATION_NEAR_END, 0.0, false, pbPm);
        }

        if (true == otuTimFault)
        {
            uint64_t value = 0;
            addOtuPmPair(OTU_PM_IAE , PMDIRECTION_RX, PMLOCATION_NEAR_END, value, false, pbPm);
            addOtuPmPair(OTU_PM_BIAE, PMDIRECTION_RX, PMLOCATION_FAR_END , value, false, pbPm);
        }
        else
        {
            addOtuPmPair(OTU_PM_IAE  , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.rx.pmcommon.iae , validity, pbPm);
            addOtuPmPair(OTU_PM_BIAE , PMDIRECTION_RX, PMLOCATION_FAR_END , adPm.rx.pmcommon.biae, validity, pbPm);
        }
    }
}

void OtuPbTranslator::pmAdDefault(DataPlane::DpMsOtuPMSContainer& adPm)
{
    memset(&adPm.rx.pmcommon, 0, sizeof(adPm.rx.pmcommon));
    memset(&adPm.rx.fecstat,  0, sizeof(adPm.rx.fecstat));
    memset(&adPm.tx,          0, sizeof(adPm.tx));
}

bool OtuPbTranslator::isPbChanged(const chm6_dataplane::Chm6OtuState& pbStatPrev, const chm6_dataplane::Chm6OtuState& pbStatCur) const
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
            INFN_LOG(SeverityLevel::error) << "Otu RxTtti key = " << ttiKey << " not found in previous map";
            return true;
        }

        itCur  = pbStatCur.hal().internal_fac_rx_tti().tti().find(ttiKey);
        if (itCur == pbStatCur.hal().internal_fac_rx_tti().tti().end())
        {
            INFN_LOG(SeverityLevel::error) << "Otu RxTtti key = " << ttiKey << " not found in current map";
            return true;
        }

        if (itPrev->second.value().value() != itCur->second.value().value())
        {
            INFN_LOG(SeverityLevel::debug) << "Otu RxTtti changed: Byte" << index
                           << ": Prev = 0x" << std::hex << itPrev->second.value().value() << std::dec
                           << ", Cur = 0x"  << std::hex << itCur->second.value().value()  << std::dec;
            return true;
        }
    }

    return false;
}

bool OtuPbTranslator::isAdChanged(const DpAdapter::OtuFault& adFaultPrev, const DpAdapter::OtuFault& adFaultCur) const
{
    if (adFaultPrev.facBmp != adFaultCur.facBmp)
    {
        return true;
    }

    return false;
}

std::string OtuPbTranslator::tomToOtnAid(uint tomId) const
{
    // Example: aidStr = 1-4-T1 .. 1-4-T16

    std::string aid = getChassisSlotAid() + "-T" + to_string(tomId);

    return (aid);
}

uint OtuPbTranslator::otnAidToTom(std::string otnAid) const
{
    // Example: aidStr = 1-4-T1 .. 1-4-T16

    std::string aidPort = "-T";
    std::size_t found = otnAid.find(aidPort);
    if (found != std::string::npos)
    {
        std::string sId = otnAid.substr(found + aidPort.length());
        return (stoi(sId,nullptr));
    }

    return 0;
}


}; // namespace DataPlane


