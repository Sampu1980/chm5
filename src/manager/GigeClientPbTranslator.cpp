/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "GigeClientPbTranslator.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"

#include <iostream>
#include <sstream>


using ::chm6_dataplane::Chm6GigeClientState;
using ::chm6_dataplane::Chm6GigeClientConfig;
using ::chm6_dataplane::Chm6GigeClientFault;
using ::chm6_dataplane::Chm6GigeClientPm;

using namespace ::std;


namespace DataPlane {

std::map<string, GigeClientPbTranslator::FecSerCache> GigeClientPbTranslator::mFecSerCache;

void GigeClientPbTranslator::configPbToAd(const Chm6GigeClientConfig& pbCfg, DpAdapter::GigeCommon& adCfg) const
{
    std::string gigeAid = pbCfg.base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid;

    // Port Rate
    if (hal_dataplane::PORT_RATE_UNSPECIFIED != pbCfg.hal().rate())
    {
        hal_dataplane::PortRate portRate = pbCfg.hal().rate();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " PortRate: " << toString(portRate);
        switch (portRate)
        {
            case hal_dataplane::PORT_RATE_ETH100G:
                adCfg.rate = DataPlane::PORT_RATE_ETH100G;
                break;
            case hal_dataplane::PORT_RATE_ETH400G:
                adCfg.rate = DataPlane::PORT_RATE_ETH400G;
                break;
            default:
                break;
        }
    }

    // LoopBack
    if (hal_dataplane::LOOP_BACK_TYPE_UNSPECIFIED != pbCfg.hal().loopback())
    {
        hal_dataplane::LoopBackType loopBack = pbCfg.hal().loopback();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " Loopback: " << toString(loopBack);
        switch (loopBack)
        {
            case hal_dataplane::LOOP_BACK_TYPE_OFF:
                adCfg.lpbk = DataPlane::LOOP_BACK_TYPE_OFF;
                break;
            case hal_dataplane::LOOP_BACK_TYPE_FACILITY:
                adCfg.lpbk = DataPlane::LOOP_BACK_TYPE_FACILITY;
                break;
            case hal_dataplane::LOOP_BACK_TYPE_TERMINAL:
                adCfg.lpbk = DataPlane::LOOP_BACK_TYPE_TERMINAL;
                break;
            default:
                break;
        }
    }

    // FEC Enable
    //     100GE - Apply on GearBox
    //     400GE - Apply on DCO
    if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().fec_enable_bool())
    {
        wrapper::Bool fecEnable = pbCfg.hal().fec_enable_bool();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " FecEnable: " << toString(fecEnable);
        switch (fecEnable)
        {
            case wrapper::BOOL_TRUE:
                adCfg.fecEnable = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.fecEnable = false;
                break;
            default:
                break;
        }

        //Always set fecEnable = true in DCO
        adCfg.fecEnable = true;
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " Always set FecEnable = true in DCO";
    }

    // MTU
    if (pbCfg.hal().has_mtu())
    {
        uint mtu = pbCfg.hal().mtu().value();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " Mtu: " << mtu;
        adCfg.mtu = mtu;
    }

    // Rx/TX Snoop/Drop Enable
    if (pbCfg.hal().has_mode())
    {
        // RX Snoop Enable
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().mode().rx_snoop_enable_bool())
        {
            wrapper::Bool enable = pbCfg.hal().mode().rx_snoop_enable_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " RxSnoopEnable: " << toString(enable);
            switch (enable)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.lldpRxSnoop = true;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.lldpRxSnoop = false;
                    break;
                default:
                    break;
            }
        }

        // TX Snoop Enable
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().mode().tx_snoop_enable_bool())
        {
            wrapper::Bool enable = pbCfg.hal().mode().tx_snoop_enable_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " TxSnoopEnable: " << toString(enable);
            switch (enable)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.lldpTxSnoop = true;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.lldpTxSnoop = false;
                    break;
                default:
                    break;
            }
        }

        // RX Drop Enable
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().mode().rx_drop_enable_bool())
        {
            wrapper::Bool enable = pbCfg.hal().mode().rx_drop_enable_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " RxDropEnable: " << toString(enable);
            switch (enable)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.lldpRxDrop = true;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.lldpRxDrop = false;
                    break;
                default:
                    break;
            }
        }

        // TX Drop Enable
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().mode().tx_drop_enable_bool())
        {
            wrapper::Bool enable = pbCfg.hal().mode().tx_drop_enable_bool();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " TxDropEnable: " << toString(enable);
            switch (enable)
            {
                case wrapper::BOOL_TRUE:
                    adCfg.lldpTxDrop = true;
                    break;
                case wrapper::BOOL_FALSE:
                    adCfg.lldpTxDrop = false;
                    break;
                default:
                    break;
            }
        }
    }

    // Auto TX
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != pbCfg.hal().auto_tx())
    {
        hal_dataplane::MaintenanceSignal autoTx = pbCfg.hal().auto_tx();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " AutoTx: " << toString(autoTx);
        switch (autoTx)
        {
            case hal_dataplane::MAINTENANCE_SIGNAL_IDLE:
                adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_IDLE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LF:
                adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_LF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_RF:
                adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_RF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE:
                adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF:
                adCfg.autoMsTx = DataPlane::MAINTENANCE_SIGNAL_LASEROFF;
                break;
            default:
                break;
        }
    }

    // Force TX
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != pbCfg.hal().force_tx())
    {
        hal_dataplane::MaintenanceSignal forceTx = pbCfg.hal().force_tx();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " ForceTx: " << toString(forceTx);
        switch (forceTx)
        {
            case hal_dataplane::MAINTENANCE_SIGNAL_IDLE:
                adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_IDLE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LF:
                adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_LF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_RF:
                adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_RF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE:
                adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF:
                adCfg.forceMsTx = DataPlane::MAINTENANCE_SIGNAL_LASEROFF;
                break;
            default:
                break;
        }
    }

    // Auto Rx
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != pbCfg.hal().auto_rx())
    {
        hal_dataplane::MaintenanceSignal autoRx = pbCfg.hal().auto_rx();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " AutoRx: " << toString(autoRx);
        switch (autoRx)
        {
            case hal_dataplane::MAINTENANCE_SIGNAL_IDLE:
                adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_IDLE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LF:
                adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_LF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_RF:
                adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_RF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE:
                adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF:
                adCfg.autoMsRx = DataPlane::MAINTENANCE_SIGNAL_LASEROFF;
                break;
            default:
                break;
        }
    }

    // Force RX
    if (hal_dataplane::MAINTENANCE_SIGNAL_UNSPECIFIED != pbCfg.hal().force_rx())
    {
        hal_dataplane::MaintenanceSignal forceRx = pbCfg.hal().force_rx();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " ForceRx: " << toString(forceRx);
        switch (forceRx)
        {
            case hal_dataplane::MAINTENANCE_SIGNAL_IDLE:
                adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_IDLE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LF:
                adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_LF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_RF:
                adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_RF;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE:
                adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_NOREPLACE;
                break;
            case hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF:
                adCfg.forceMsRx = DataPlane::MAINTENANCE_SIGNAL_LASEROFF;
                break;
            default:
                break;
        }
    }

    if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().fwd_defect_tda())
    {
        wrapper::Bool fwdDefectTda = pbCfg.hal().fwd_defect_tda();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " FwdDefectTda: " << toString(fwdDefectTda);
        switch (fwdDefectTda)
        {
            case wrapper::BOOL_TRUE:
                adCfg.fwdTdaTrigger = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.fwdTdaTrigger = false;
                break;
            default:
                break;
        }
    }

    if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().test_signal_gen())
    {
        wrapper::Bool testSigGen = pbCfg.hal().test_signal_gen();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " TestSignalGenFac: " << toString(testSigGen);
        switch (testSigGen)
        {
            case wrapper::BOOL_TRUE:
                adCfg.generatorTx = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.generatorTx = false;
                break;
            default:
                break;
        }
    }
    else
    {
        adCfg.generatorTx = false;
    }

    if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().test_signal_gen_term())
    {
        wrapper::Bool testSigGen = pbCfg.hal().test_signal_gen_term();
        INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " TestSignalGenTerm: " << toString(testSigGen);
        switch (testSigGen)
        {
            case wrapper::BOOL_TRUE:
                adCfg.generatorRx = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.generatorRx = false;
                break;
            default:
                break;
        }
    }
    else
    {
        adCfg.generatorRx = false;
    }

     if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().test_signal_mon())
     {
         wrapper::Bool testSigMon = pbCfg.hal().test_signal_mon();
         INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " TestSignalMon: " << toString(testSigMon);
         switch (testSigMon)
         {
             case wrapper::BOOL_TRUE:
                 adCfg.monitorRx = true;
                 break;
             case wrapper::BOOL_FALSE:
                 adCfg.monitorRx = false;
                 break;
             default:
                 break;
         }
     }
     else
     {
         adCfg.monitorRx = false;
     }

    if (pbCfg.hal().has_alarm_threshold())
    {
        if (wrapper::BOOL_UNSPECIFIED != pbCfg.hal().alarm_threshold().fec_deg_ser_en())
        {
            wrapper::Bool enable = pbCfg.hal().alarm_threshold().fec_deg_ser_en();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " FEC Deg SER Enable: " << toString(enable);
            switch(enable)
            {
            case wrapper::BOOL_TRUE:
                adCfg.fecDegSer.enable = true;
                break;
            case wrapper::BOOL_FALSE:
                adCfg.fecDegSer.enable = false;
                break;
            default:
                break;
            }
        }

        if (pbCfg.hal().alarm_threshold().has_fec_deg_ser_act_thresh())
        {
            double actThresh = pbCfg.hal().alarm_threshold().fec_deg_ser_act_thresh().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " FEC Deg SER Activate Threshold: " << actThresh;
            adCfg.fecDegSer.activateThreshold = actThresh;
        }

        if (pbCfg.hal().alarm_threshold().has_fec_deg_ser_deact_thresh())
        {
            double deactThresh = pbCfg.hal().alarm_threshold().fec_deg_ser_deact_thresh().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " FEC Deg SER Deactivate Threshold: " << deactThresh;
            adCfg.fecDegSer.deactivateThreshold = deactThresh;
        }

        if (pbCfg.hal().alarm_threshold().has_fec_deg_ser_mon_per())
        {
            uint monPer = pbCfg.hal().alarm_threshold().fec_deg_ser_mon_per().value();
            INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid << " FEC Deg SER Monitor Period: " << monPer;
            adCfg.fecDegSer.monitorPeriod = monPer;
            GigeClientPbTranslator::cacheFecDegSerMonPeriod(gigeAid, monPer);
        }
    }
}



void GigeClientPbTranslator::configAdToPb(const DpAdapter::GigeCommon& adCfg, Chm6GigeClientState& pbStat, bool isGbFecEnable) const
{
    std::string gigeAid = pbStat.base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << gigeAid;

    // Port Rate and FEC Enable
    switch (adCfg.rate)
    {
        case DataPlane::PORT_RATE_ETH100G:
            pbStat.mutable_hal()->mutable_installed_config()->set_rate(hal_dataplane::PORT_RATE_ETH100G);
            pbStat.mutable_hal()->mutable_installed_config()->set_fec_enable_bool(isGbFecEnable ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);
            break;
        case DataPlane::PORT_RATE_ETH400G:
            pbStat.mutable_hal()->mutable_installed_config()->set_rate(hal_dataplane::PORT_RATE_ETH400G);
            pbStat.mutable_hal()->mutable_installed_config()->set_fec_enable_bool(wrapper::BOOL_TRUE);
            break;
        default:
            break;
    }

    // LoopBack
    switch (adCfg.lpbk)
    {
        case DataPlane::LOOP_BACK_TYPE_OFF:
            pbStat.mutable_hal()->mutable_installed_config()->set_loopback(hal_dataplane::LOOP_BACK_TYPE_OFF);
            break;
        case DataPlane::LOOP_BACK_TYPE_FACILITY:
            pbStat.mutable_hal()->mutable_installed_config()->set_loopback(hal_dataplane::LOOP_BACK_TYPE_FACILITY);
            break;
        case  DataPlane::LOOP_BACK_TYPE_TERMINAL:
            pbStat.mutable_hal()->mutable_installed_config()->set_loopback(hal_dataplane::LOOP_BACK_TYPE_TERMINAL);
            break;
        default:
            break;
    }

    // MTU
    pbStat.mutable_hal()->mutable_installed_config()->mutable_mtu()->set_value(adCfg.mtu);

    // Snoop,Drop
    pbStat.mutable_hal()->mutable_installed_config()->mutable_mode()
            ->set_rx_snoop_enable_bool(adCfg.lldpRxSnoop ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);
    pbStat.mutable_hal()->mutable_installed_config()->mutable_mode()
            ->set_tx_snoop_enable_bool(adCfg.lldpTxSnoop ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);
    pbStat.mutable_hal()->mutable_installed_config()->mutable_mode()
            ->set_rx_drop_enable_bool(adCfg.lldpRxDrop ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);
    pbStat.mutable_hal()->mutable_installed_config()->mutable_mode()
            ->set_tx_drop_enable_bool(adCfg.lldpTxDrop ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);


    // Auto TX
    switch (adCfg.autoMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_IDLE:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_tx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LF:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_tx(hal_dataplane::MAINTENANCE_SIGNAL_LF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_RF:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_tx(hal_dataplane::MAINTENANCE_SIGNAL_RF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_tx(hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LASEROFF:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_tx(hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF);
            break;
        default:
            break;
    }

    // Force TX
    switch (adCfg.forceMsTx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_IDLE:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_tx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LF:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_tx(hal_dataplane::MAINTENANCE_SIGNAL_LF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_RF:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_tx(hal_dataplane::MAINTENANCE_SIGNAL_RF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_tx(hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LASEROFF:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_tx(hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF);
            break;
        default:
            break;
    }

    // Auto RX
    switch (adCfg.autoMsRx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_IDLE:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_rx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LF:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_rx(hal_dataplane::MAINTENANCE_SIGNAL_LF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_RF:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_rx(hal_dataplane::MAINTENANCE_SIGNAL_RF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_rx(hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LASEROFF:
            pbStat.mutable_hal()->mutable_installed_config()->set_auto_rx(hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF);
            break;
        default:
            break;
    }

    // Force RX
    switch (adCfg.forceMsRx)
    {
        case DataPlane::MAINTENANCE_SIGNAL_IDLE:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_rx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LF:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_rx(hal_dataplane::MAINTENANCE_SIGNAL_LF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_RF:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_rx(hal_dataplane::MAINTENANCE_SIGNAL_RF);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_NOREPLACE:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_rx(hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE);
            break;
        case DataPlane::MAINTENANCE_SIGNAL_LASEROFF:
            pbStat.mutable_hal()->mutable_installed_config()->set_force_rx(hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF);
            break;
        default:
            break;
    }

    pbStat.mutable_hal()->mutable_installed_config()
            ->set_fwd_defect_tda(adCfg.fwdTdaTrigger ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);
}

void GigeClientPbTranslator::statusAdToPb(const DpAdapter::GigeStatus& adStat, chm6_dataplane::Chm6GigeClientState& pbStat) const
{
    // Max Mtu
    pbStat.mutable_hal()->mutable_max_mtu_size()->set_value(adStat.maxMtu);

    // LinkUp
    wrapper::Bool linkUpState = (DataPlane::CLIENTSTATUS_UP == adStat.status) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE;
    pbStat.mutable_hal()->set_link_up_bool(linkUpState);
}

void GigeClientPbTranslator::faultAdToPb(const std::string aid, const uint64_t& faultBmp, const DpAdapter::GigeFault& adFault, Chm6GigeClientFault& pbFault, bool force) const
{
    hal_dataplane::GigeClientFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_fault();

    uint64_t faultBmpNew = adFault.facBmp;
    uint64_t bitMask = 1ULL;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if (((faultBmp ^ faultBmpNew) & bitMask) || (true == force))
        {
            addGigeClientFaultPair(vFault->faultName, vFault->direction, vFault->location, vFault->faulted, pbFault);
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

bool GigeClientPbTranslator::isHardFault(const DpAdapter::GigeFault& adFault, DataPlane::FaultDirection direction) const
{
    bool isHardFault = false;

    for (auto vFault = adFault.fac.begin(); vFault != adFault.fac.end(); vFault++)
    {
        if ((vFault->faultName == gigeFaultParamsToString.at(GIGE_FAULT_LF_INGRESS)) &&
            (vFault->direction == direction) &&
            (vFault->location  == FAULTLOCATION_NEAR_END) &&
            (vFault->faulted == true))
        {
            isHardFault = true;
            break;
        }

        if ((vFault->faultName == gigeFaultParamsToString.at(GIGE_FAULT_LF_EGRESS)) &&
            (vFault->direction == direction) &&
            (vFault->location  == FAULTLOCATION_NEAR_END) &&
            (vFault->faulted == true))
        {
            isHardFault = true;
            break;
        }

        if ((vFault->faultName == gigeFaultParamsToString.at(GIGE_FAULT_RF_INGRESS)) &&
            (vFault->direction == direction) &&
            (vFault->location  == FAULTLOCATION_FAR_END) &&
            (vFault->faulted == true))
        {
            isHardFault = true;
            break;
        }

        if ((vFault->faultName == gigeFaultParamsToString.at(GIGE_FAULT_RF_EGRESS)) &&
            (vFault->direction == direction) &&
            (vFault->location  == FAULTLOCATION_FAR_END) &&
            (vFault->faulted == true))
        {
            isHardFault = true;
            break;
        }
    }

    return isHardFault;
}

void GigeClientPbTranslator::faultAdGearBoxUpdate(const DpAdapter::GigeFault& adGearBoxFault, DpAdapter::GigeFault& adFault) const
{
    for (auto i = adGearBoxFault.fac.begin(); i != adGearBoxFault.fac.end(); i++)
    {
        uint bitPos = 0;

        for (auto j = adFault.fac.begin(); j != adFault.fac.end(); j++)
        {
           if ((j->faultName == i->faultName) &&
               (j->direction == i->direction) &&
               (j->location  == i->location))
           {
                INFN_LOG(SeverityLevel::debug) << "Overwrite DCO with GearBox fault " << i->faultName << "-" << toString(i->direction) << "-" << toString(i->location) << " = " << i->faulted;
                j->faulted = i->faulted;

                if (true == j->faulted)
                {
                     adFault.facBmp |= 1ULL << bitPos;
                }
                else
                {
                     adFault.facBmp &= ~(1ULL << bitPos);
                }
            }

            bitPos++;
        }
    }

    INFN_LOG(SeverityLevel::debug) << "Merged GearBox and DCO fault(0x" << std::hex << adFault.facBmp  << std::dec << ")";
}


void GigeClientPbTranslator::addGigeClientFaultPair(std::string faultName, FaultDirection faultDirection, FaultLocation faultLocation, bool value, Chm6GigeClientFault& pbFault) const
{
    hal_dataplane::GigeClientFault_OperationalFault* faultHal = pbFault.mutable_hal();
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

void GigeClientPbTranslator::faultAdToPbSim(const uint64_t& fault, Chm6GigeClientFault& pbFault) const
{
    hal_dataplane::GigeClientFault_OperationalFault* faultHal = pbFault.mutable_hal();
    faultHal->clear_fault();

    bool undefinedBit = false;

    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LF_EGRESS)       , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LF_EGRESS))       , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LF_INGRESS)      , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LF_INGRESS))      , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LINE_TESTSIG_GEN), FAULTDIRECTION_NA, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LINE_TESTSIG_GEN)), pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LINE_TESTSIG_OOS), FAULTDIRECTION_NA, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LINE_TESTSIG_OOS)), pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LOA_EGRESS)      , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LOA_EGRESS))      , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LOA_INGRESS)     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LOA_INGRESS))     , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LOLM_EGRESS)     , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LOLM_EGRESS))     , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LOLM_INGRESS)    , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LOLM_INGRESS))    , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_HIBER_EGRESS)    , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_HIBER_EGRESS))    , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_HIBER_INGRESS)   , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_HIBER_INGRESS))   , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LOSYNC_EGRESS)   , FAULTDIRECTION_TX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LOSYNC_EGRESS))   , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LOSYNC_INGRESS)  , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LOSYNC_INGRESS))  , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_OOS_AU)          , FAULTDIRECTION_NA, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_OOS_AU))          , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_RF_EGRESS)       , FAULTDIRECTION_TX, FAULTLOCATION_FAR_END , (bool)(fault & (1UL << GIGE_FAULT_RF_EGRESS))       , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_RF_INGRESS)      , FAULTDIRECTION_RX, FAULTLOCATION_FAR_END , (bool)(fault & (1UL << GIGE_FAULT_RF_INGRESS))      , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_TRIB_TESTSIG_GEN)      , FAULTDIRECTION_NA, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_TRIB_TESTSIG_GEN))      , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_TRIB_TESTSIG_OOS)      , FAULTDIRECTION_NA, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_TRIB_TESTSIG_OOS))      , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_LOCAL_DEGRADED)  , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_LOCAL_DEGRADED))  , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_REMOTE_DEGRADED) , FAULTDIRECTION_RX, FAULTLOCATION_FAR_END,  (bool)(fault & (1UL << GIGE_FAULT_REMOTE_DEGRADED)) , pbFault);
    addGigeClientFaultPair(gigeFaultParamsToString.at(GIGE_FAULT_FEC_DEG_SER)     , FAULTDIRECTION_RX, FAULTLOCATION_NEAR_END, (bool)(fault & (1UL << GIGE_FAULT_FEC_DEG_SER))     , pbFault);

}

void GigeClientPbTranslator::addGigeClientPmPair(GIGE_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, uint64_t value, bool validity, Chm6GigeClientPm& pbPm) const
{
    hal_dataplane::GigeClientPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_pm()->mutable_pm();

    std::string pmKey(gigePmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(gigePmParamsToString.at(pmParam));

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

void GigeClientPbTranslator::addGigeClientPmPair(GIGE_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, double value, bool validity, Chm6GigeClientPm& pbPm) const
{
    hal_dataplane::GigeClientPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_pm()->mutable_pm();

    std::string pmKey(gigePmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(gigePmParamsToString.at(pmParam));

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

void GigeClientPbTranslator::pmAdToPb(const DataPlane::DpMsClientGigePmContainer& adPm, const gearbox::BcmPm& adGearBoxPm, bool testSignalMon, bool isHardFaultRx, bool isHardFaultTx, bool validity, Chm6GigeClientPm& pbPm)
{
    hal_dataplane::GigeClientPm_OperationalPm* pmHal = pbPm.mutable_hal();
    pmHal->clear_pm();

    uint64_t undefinedField = 0;

    if (true == isHardFaultRx)
    {
        addGigeClientPmPair(GIGE_PM_OCTETS_INGRESS, PMDIRECTION_RX, PMLOCATION_NEAR_END, undefinedField, validity, pbPm);
    }
    else
    {
        addGigeClientPmPair(GIGE_PM_OCTETS_INGRESS, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.octets, validity, pbPm);
    }

    if (true == isHardFaultTx)
    {
        addGigeClientPmPair(GIGE_PM_OCTETS_EGRESS, PMDIRECTION_TX, PMLOCATION_NEAR_END, undefinedField, validity, pbPm);
    }
    else
    {
        addGigeClientPmPair(GIGE_PM_OCTETS_EGRESS, PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.octets, validity, pbPm);
    }

    addGigeClientPmPair(GIGE_PM_PACKETS_INGRESS           , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.packets           , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_ERR_PACKETS_INGRESS       , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.error_packets     , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_ERR_OCTETS_INGRESS        , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.error_octets      , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_JABBERS_INGRESS           , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.jabber_frames     , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_FRAGMENTS_INGRESS         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.fragmented_frames , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_CRC_ALIGNED_ERR_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.crc_aligned_error , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_UNDERSIZED_INGRESS        , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.under_sized       , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_OVERSIZED_INGRESS         , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.over_sized        , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_BROADCAST_PKTS_INGRESS    , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.broadcast_frames  , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_MULTICAST_PKTS_INGRESS    , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.multicast_frames  , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_IN_PAUSE_FRAMES_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.pause_frames      , validity, pbPm);

    addGigeClientPmPair(GIGE_PM_PACKETS_EGRESS            , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.packets          , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_ERR_PACKETS_EGRESS        , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.error_packets    , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_ERR_OCTETS_EGRESS         , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.error_octets     , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_JABBERS_EGRESS            , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.jabber_frames    , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_FRAGMENTS_EGRESS          , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.fragmented_frames, validity, pbPm);
    addGigeClientPmPair(GIGE_PM_CRC_ALIGNED_ERR_EGRESS    , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.crc_aligned_error, validity, pbPm);
    addGigeClientPmPair(GIGE_PM_UNDERSIZED_EGRESS         , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.under_sized      , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_OVERSIZED_EGRESS          , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.over_sized       , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_BROADCAST_PKTS_EGRESS     , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.broadcast_frames , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_MULTICAST_PKTS_EGRESS     , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.multicast_frames , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_OUT_PAUSE_FRAMES_EGRESS   , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.pause_frames     , validity, pbPm);

    addGigeClientPmPair(GIGE_PM_SIZE_64_INGRESS           , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.size_64           , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_65_TO_127_INGRESS    , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.size_65_to_127    , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_128_TO_255_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.size_128_to_255   , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_256_TO_511_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.size_256_to_511   , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_512_TO_1023_INGRESS  , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.size_512_to_1023  , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_1024_TO_1518_INGRESS , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.size_1024_to_1518 , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_1519_TO_JUMBO_INGRESS, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacRx.jumbo_frames      , validity, pbPm);

    addGigeClientPmPair(GIGE_PM_SIZE_64_EGRESS            , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.size_64          , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_65_TO_127_EGRESS     , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.size_65_to_127   , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_128_TO_255_EGRESS    , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.size_128_to_255  , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_256_TO_511_EGRESS    , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.size_256_to_511  , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_512_TO_1023_EGRESS   , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.size_512_to_1023 , validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_1024_TO_1518_EGRESS  , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.size_1024_to_1518, validity, pbPm);
    addGigeClientPmPair(GIGE_PM_SIZE_1519_TO_JUMBO_EGRESS , PMDIRECTION_TX , PMLOCATION_NEAR_END, adPm.DpMsClientGigePmMacTx.jumbo_frames     , validity, pbPm);

    if (gearbox::PCSFEC == adGearBoxPm.fecMode)
    {
        addGigeClientPmPair(GIGE_PM_PCS_ICG_INGRESS       , PMDIRECTION_RX, PMLOCATION_NEAR_END, adGearBoxPm.pcs.icgCounts                   , validity, pbPm);
    }
    else
    {
        addGigeClientPmPair(GIGE_PM_PCS_ICG_INGRESS       , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPcsRx.errored_blocks   , validity, pbPm);
    }

    addGigeClientPmPair(GIGE_PM_PCS_ICG_EGRESS            , PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPcsTx.errored_blocks   , validity, pbPm);

    if (gearbox::KR4FEC == adGearBoxPm.fecMode)
    {
        addGigeClientPmPair(GIGE_PM_CORR_WORD_INGRESS       , PMDIRECTION_RX, PMLOCATION_NEAR_END, adGearBoxPm.fec.rxCorrWord  , validity, pbPm);
        addGigeClientPmPair(GIGE_PM_UN_CORR_WORD_INGRESS    , PMDIRECTION_RX, PMLOCATION_NEAR_END, adGearBoxPm.fec.rxUnCorrWord, validity, pbPm);
        addGigeClientPmPair(GIGE_PM_CORRECTED_BIT_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adGearBoxPm.fec.rxCorrBits  , validity, pbPm);
        addGigeClientPmPair(GIGE_PM_FEC_SYMBOL_ERROR_INGRESS, PMDIRECTION_RX, PMLOCATION_NEAR_END, adGearBoxPm.fec.fecSymbolErr, validity, pbPm);
    }
    else if (gearbox::NoFEC == adGearBoxPm.fecMode) //400G only, NOT TRIGGERED BY 100G KR4FEC or PCSFEC
    {
        addGigeClientPmPair(GIGE_PM_CORR_WORD_INGRESS       , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPhyFecRx.fec_corrected        , validity, pbPm);
        addGigeClientPmPair(GIGE_PM_UN_CORR_WORD_INGRESS    , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPhyFecRx.fec_un_corrected     , validity, pbPm);
        addGigeClientPmPair(GIGE_PM_CORRECTED_BIT_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPhyFecRx.bit_error            , validity, pbPm);
        addGigeClientPmPair(GIGE_PM_FEC_SYMBOL_ERROR_INGRESS, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPhyFecRx.fec_symbol_error     , validity, pbPm);
        addGigeClientPmPair(GIGE_PM_PRE_FEC_BER_INGRESS     , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPhyFecRx.pre_fec_ber          , validity, pbPm);
        addGigeClientPmPair(GIGE_PM_FSE_RATE_INGRESS        , PMDIRECTION_RX, PMLOCATION_NEAR_END, calcFecSER(adPm.aid, adPm.DpMsClientGigePmPhyFecRx.fec_symbol_error), validity, pbPm);
    }

    if (gearbox::PCSFEC == adGearBoxPm.fecMode)
    {
        addGigeClientPmPair(GIGE_PM_PCS_BIP_TOTAL_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adGearBoxPm.pcs.bip8Counts          , validity, pbPm);
    }
    else
    {
        addGigeClientPmPair(GIGE_PM_PCS_BIP_TOTAL_INGRESS   , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPcsRx.bip_total, validity, pbPm);
    }

    if (true == testSignalMon)
    {
        addGigeClientPmPair(GIGE_PM_TRIB_TEST_SIG_ERR, PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.DpMsClientGigePmPcsRx.test_pattern_error, validity, pbPm);
    }
    else
    {
        uint64_t value = 0;
        addGigeClientPmPair(GIGE_PM_TRIB_TEST_SIG_ERR, PMDIRECTION_RX, PMLOCATION_NEAR_END, value, false, pbPm);
    }
}

double GigeClientPbTranslator::calcFecSER(string aid, uint64_t fec_symbol_error)
{
    const double cBitRatePreFec  = 400000000000;
    double bitRatePostFec = ((double)257*cBitRatePreFec/(double)256)*((double)544/(double)514);
    double symbolRate     = bitRatePostFec / (double)10;    // AF0 todo - 10 bits per symbol, this is dependent on modulation, need to fix for other modulations
    double fecSymErrRate = 0.0;
    uint64_t totalFecSymErrs = 0;

    auto elem = GigeClientPbTranslator::mFecSerCache.find(aid);

    if (elem->second.cachedMonPer > GigeClientPbTranslator::cMaxMonPer)
    {
        INFN_LOG(SeverityLevel::error) << "SW BUG!! cachedMonPer is not initialized or invalid! cachedMonPer=" << elem->second.cachedMonPer;
        elem->second.cachedMonPer %= GigeClientPbTranslator::cMaxMonPer;
    }
    else if (elem->second.cachedMonPer == 0)
    {
        INFN_LOG(SeverityLevel::error) << "SW BUG!! cachedMonPer is not initialized or invalid! cachedMonPer=" << elem->second.cachedMonPer;
        elem->second.cachedMonPer = 1;
    }

    if (elem != GigeClientPbTranslator::mFecSerCache.end())
    {
        // found
        // make sure we are within array boundaries
        elem->second.monPerCounter %= elem->second.cachedMonPer;

        if (elem->second.fecSymErrs.size() < elem->second.cachedMonPer)
        {
            elem->second.fecSymErrs.push_back(fec_symbol_error);
            elem->second.monPerCounter++;
        }
        else if (elem->second.fecSymErrs.size() > elem->second.cachedMonPer)
        {
            // User reduced monitoring period, reduce vector size down
            unsigned int sizeReduce = elem->second.fecSymErrs.size() - elem->second.cachedMonPer;


            elem->second.fecSymErrs.erase(elem->second.fecSymErrs.begin(), elem->second.fecSymErrs.begin() + sizeReduce);

            // add fec sym err to vector
            elem->second.fecSymErrs[elem->second.monPerCounter++] = fec_symbol_error;
        }
        else
        {
            // we have enough fec sym err data to run calcs

            elem->second.fecSymErrs[elem->second.monPerCounter++] = fec_symbol_error;


            for (unsigned int i = 0; i < elem->second.fecSymErrs.size(); i++)
            {
                totalFecSymErrs += elem->second.fecSymErrs[i];
            }
            fecSymErrRate = (double)totalFecSymErrs / (symbolRate * (double)elem->second.cachedMonPer);

        }

    }


    return fecSymErrRate;
}

void GigeClientPbTranslator::cacheFecDegSerMonPeriod(string aid, uint64_t monPer)
{
    // cache the monitoring period
    auto elem = GigeClientPbTranslator::mFecSerCache.find(aid);
    if (elem != GigeClientPbTranslator::mFecSerCache.end())
    {
        elem->second.cachedMonPer = monPer;
    }
    else
    {
        FecSerCache fecSer;
        fecSer.cachedMonPer = monPer;
        GigeClientPbTranslator::mFecSerCache.insert(make_pair(aid, fecSer));
    }
    INFN_LOG(SeverityLevel::info) << " AID=" << aid << " Cached FEC Deg SER Monitor Period: " << monPer;
}

void GigeClientPbTranslator::pmAdDefault(DataPlane::DpMsClientGigePmContainer& adPm, gearbox::BcmPm& adGearBoxPm)
{
    memset(&adPm.DpMsClientGigePmMacRx,    0, sizeof(adPm.DpMsClientGigePmMacRx));
    memset(&adPm.DpMsClientGigePmMacTx,    0, sizeof(adPm.DpMsClientGigePmMacTx));
    memset(&adPm.DpMsClientGigePmPcsRx,    0, sizeof(adPm.DpMsClientGigePmPcsRx));
    memset(&adPm.DpMsClientGigePmPcsTx,    0, sizeof(adPm.DpMsClientGigePmPcsTx));
    memset(&adPm.DpMsClientGigePmPhyFecRx, 0, sizeof(adPm.DpMsClientGigePmPhyFecRx));

    memset(&adGearBoxPm.pcs,               0, sizeof(adGearBoxPm.pcs));
    memset(&adGearBoxPm.pcsLanes,          0, sizeof(adGearBoxPm.pcsLanes));
    memset(&adGearBoxPm.pcsAccum,          0, sizeof(adGearBoxPm.pcsAccum));
    memset(&adGearBoxPm.pcsLanesAccum,     0, sizeof(adGearBoxPm.pcsLanesAccum));
    memset(&adGearBoxPm.fec,               0, sizeof(adGearBoxPm.fec));
    memset(&adGearBoxPm.fecLanes,          0, sizeof(adGearBoxPm.fecLanes));
    memset(&adGearBoxPm.fecLanesPrev,      0, sizeof(adGearBoxPm.fecLanesPrev));
    memset(&adGearBoxPm.fecAccum,          0, sizeof(adGearBoxPm.fecAccum));
    memset(&adGearBoxPm.fecLanesAccum,     0, sizeof(adGearBoxPm.fecLanesAccum));
}

bool GigeClientPbTranslator::isPbChanged(const chm6_dataplane::Chm6GigeClientState& pbStatPrev, const chm6_dataplane::Chm6GigeClientState& pbStatCur)
{
    if (  (pbStatPrev.hal().max_mtu_size().value() != pbStatCur.hal().max_mtu_size().value())
       || (pbStatPrev.hal().link_up_bool()         != pbStatCur.hal().link_up_bool()) )
    {
        return true;
    }

    return false;
}

bool GigeClientPbTranslator::isAdChanged(const DpAdapter::GigeFault& adFaultPrev, const DpAdapter::GigeFault& adFaultCur)
{
    if (adFaultPrev.facBmp != adFaultCur.facBmp)
    {
        return true;
    }

    return false;
}

std::string GigeClientPbTranslator::numToAid(uint gigeNum, std::string &tpAid) const
{
    // Example: aidStr = 1-4-T1 .. 1-4-T16

    std::string aid = getChassisSlotAid() + "-T" + to_string(gigeNum);

    if(mTomIdMap.find(gigeNum) != mTomIdMap.end())
    {
        tpAid = getChassisSlotAid() + "-T" + mTomIdMap.at(gigeNum) ;
    }

    return (aid);
}


}; // namespace DataPlane
