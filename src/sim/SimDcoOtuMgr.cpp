/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/

#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <infinera_dco_otu/infinera_dco_otu.pb.h>

#include "SimObjMgr.h"
#include "SimCrudService.h"
#include "SimDcoOtuMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"
#include "dp_common.h"

using ::dcoyang::infinera_dco_otu::Otus;
using ::google::protobuf::util::MessageToJsonString;

using namespace dcoyang::infinera_dco_otu;


namespace DpSim
{

// returns true if it is otu msg; false otherwise
SimObjMgr::MSG_STAT SimDcoOtuMgr::checkAndCreate(const google::protobuf::Message* pObjMsg)
{
    // Check for Otu ..............................................................................
    const Otus *pMsg = dynamic_cast<const Otus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Otu Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->otu(0).otu_id();

        SimObjMgr::MSG_STAT stat = addObj(id, (google::protobuf::Message*)&(pMsg->otu(0)), true);

        INFN_LOG(SeverityLevel::info) << "Create Done. Status: " << stat;

        return stat;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoOtuMgr::checkAndRead(google::protobuf::Message* pObjMsg)
{
    // Check for Otu ..............................................................................
    Otus *pMsg = dynamic_cast<Otus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        INFN_LOG(SeverityLevel::debug) << "Determined it is Otu Sim Msg";

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->otu(0).otu_id();

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        Otus_OtuKey* pCacheKey = (Otus_OtuKey*)pTempMsgCache;

        Otus_OtuKey* pKey = pMsg->add_otu();

        *pKey                       = *pCacheKey;  // copy
        *(pMsg->mutable_otu(0)) = *pCacheKey;  // copy

        INFN_LOG(SeverityLevel::debug) << "Read Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoOtuMgr::checkAndUpdate(const google::protobuf::Message* pObjMsg)
{
    // Check for Otu ..............................................................................
    const Otus *pMsg = dynamic_cast<const Otus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Otu Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->otu(0).otu_id();

        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        Otus_OtuKey* pCacheKey = (Otus_OtuKey*)pTempMsg;

        updateObj(&pMsg->otu(0).otu(), pCacheKey->mutable_otu());

        INFN_LOG(SeverityLevel::info) << "Update Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoOtuMgr::checkAndDelete(const google::protobuf::Message* pObjMsg)
{
    // Check for Otu ..............................................................................
    const Otus *pMsg = dynamic_cast<const Otus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Otu Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->otu(0).otu_id();

        if (!delObj(id))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        INFN_LOG(SeverityLevel::info) << "Delete Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

// Create the memory for derived type which will be help by base pointer in base class
void SimDcoOtuMgr::createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                       google::protobuf::Message*& pNewObj)
{
    INFN_LOG(SeverityLevel::info) << "Create Otu Sim config object";
    const Otus_OtuKey* pKeyFrom = (const Otus_OtuKey*)pFromObj;

    Otus_OtuKey* pKeyNew = new Otus_OtuKey(*pKeyFrom);

    pNewObj = (google::protobuf::Message*)pKeyNew;

    INFN_LOG(SeverityLevel::info) << "Done";
}

void SimDcoOtuMgr::createStateMsgObj(google::protobuf::Message* pObjMsg)
{
    INFN_LOG(SeverityLevel::info) << "Create Otu Sim state object";

    Otus_OtuKey* pKey = (Otus_OtuKey *)pObjMsg;

    uint id = pKey->otu_id();
    pKey->mutable_otu()->mutable_state()->mutable_otu_id()->set_value(id);
    pKey->mutable_otu()->mutable_state()->mutable_rx_fault_status()->set_value(0x0);
    pKey->mutable_otu()->mutable_state()->mutable_tx_fault_status()->set_value(0x0);

    pKey->mutable_otu()->mutable_state()->clear_rx_tti();
    for (size_t i = 0; i < DataPlane::MAX_TTI_LENGTH; i++)
    {
        pKey->mutable_otu()->mutable_state()->add_rx_tti()->set_tti_byte_index(i + 1);
        //pKey->mutable_otu()->mutable_state()->mutable_rx_tti(i)->set_tti_byte_index(i);
        pKey->mutable_otu()->mutable_state()->mutable_rx_tti(i)->mutable_rx_tti()->mutable_tti()->set_value(0x0);
    }

    INFN_LOG(SeverityLevel::info) << "Done";
}

void SimDcoOtuMgr::updateObj(const ::dcoyang::infinera_dco_otu::Otus_Otu* pObjIn,
                                   ::dcoyang::infinera_dco_otu::Otus_Otu* pObjOut)
{
    INFN_LOG(SeverityLevel::info) << "Update Otu Sim config";

    if (pObjIn->config().service_mode() != Otus_Otu_Config::SERVICEMODE_MODE_NONE)
    {
        Otus_Otu_Config_ServiceMode srvMode;
        switch (pObjIn->config().service_mode())
        {
            case Otus_Otu_Config::SERVICEMODE_MODE_SWITCHING:
                srvMode = Otus_Otu_Config::SERVICEMODE_MODE_SWITCHING;
                break;
            case Otus_Otu_Config::SERVICEMODE_MODE_TRANSPORT:
                srvMode = Otus_Otu_Config::SERVICEMODE_MODE_TRANSPORT;
                break;
            case Otus_Otu_Config::SERVICEMODE_MODE_NONE:
                srvMode = Otus_Otu_Config::SERVICEMODE_MODE_NONE;
                break;
            default:
                srvMode = Otus_Otu_Config::SERVICEMODE_MODE_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Service Mode = " << srvMode;

        pObjOut->mutable_config()->set_service_mode(srvMode);
    }

    if (pObjIn->config().ms_force_signal_tx() != ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal msForceTx;

        switch (pObjIn->config().ms_force_signal_tx())
        {
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
                msForceTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
                msForceTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msForceTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
                msForceTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
                msForceTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
                msForceTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
                break;
            default:
                msForceTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Force Signal Tx = " << msForceTx;

        pObjOut->mutable_config()->set_ms_force_signal_tx(msForceTx);
    }

    if (pObjIn->config().ms_auto_signal_tx() != ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal msAutoTx;

        switch (pObjIn->config().ms_auto_signal_tx())
        {
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
                msAutoTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
                msAutoTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msAutoTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
                msAutoTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
                msAutoTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
                msAutoTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
                break;
            default:
                msAutoTx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Auto Signal Tx = " << msAutoTx;

        pObjOut->mutable_config()->set_ms_auto_signal_tx(msAutoTx);
    }

    if (pObjIn->config().ms_force_signal_rx() != ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal msForceRx;

        switch (pObjIn->config().ms_force_signal_rx())
        {
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
                msForceRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
                msForceRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msForceRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
                msForceRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
                msForceRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
                msForceRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
                break;
            default:
                msForceRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Force Signal Rx = " << msForceRx;

        pObjOut->mutable_config()->set_ms_force_signal_rx(msForceRx);
    }

    if (pObjIn->config().ms_auto_signal_rx() != ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal msAutoRx;

        switch (pObjIn->config().ms_auto_signal_rx())
        {
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF:
                msAutoRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31:
                msAutoRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msAutoRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS:
                msAutoRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI:
                msAutoRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK:
                msAutoRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_XTF_LCK;
                break;
            default:
                msAutoRx = ::dcoyang::enums::InfineraDcoOtuMaintainenceSignal::INFINERADCOOTUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Auto Signal Rx = " << msAutoRx;

        pObjOut->mutable_config()->set_ms_auto_signal_rx(msAutoRx);
    }

#if 0
    if (pObjIn->config().has_fec_gen_enable())
    {
        bool fecGenEn = pObjIn->config().fec_gen_enable().value();

        INFN_LOG(SeverityLevel::info) << "Updating Fec Gen Enable = " << fecGenEn);

        pObjOut->mutable_config()->mutable_fec_gen_enable()->set_value(fecGenEn);
    }

    if (pObjIn->config().has_fec_correction_enable())
    {
        bool fecCorEn = pObjIn->config().fec_correction_enable().value();

        INFN_LOG(SeverityLevel::info) << "Updating Fec Correction Enable = " << fecCorEn);

        pObjOut->mutable_config()->mutable_fec_correction_enable()->set_value(fecCorEn);
    }
#endif

    if (pObjIn->config().loopback_type() != Otus_Otu_Config::LOOPBACKTYPE_UNSET)
    {
        Otus_Otu_Config_LoopbackType lpbkType;

        switch (pObjIn->config().loopback_type())
        {
            case Otus_Otu_Config::LOOPBACKTYPE_NONE:
                lpbkType = Otus_Otu_Config::LOOPBACKTYPE_NONE;
                break;
            case Otus_Otu_Config::LOOPBACKTYPE_FACILITY:
                lpbkType = Otus_Otu_Config::LOOPBACKTYPE_FACILITY;
                break;
            case Otus_Otu_Config::LOOPBACKTYPE_TERMINAL:
                lpbkType = Otus_Otu_Config::LOOPBACKTYPE_TERMINAL;
                break;
            default:
                lpbkType = Otus_Otu_Config::LOOPBACKTYPE_UNSET;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Loopback Type = " << lpbkType;

        pObjOut->mutable_config()->set_loopback_type(lpbkType);
    }

#if 0
    if (pObjIn->config().prbs_monitoring_mode_rx() != ::dcoyang::infinera_dco_otu::Otus_Otu_Config::PRBSMONITORINGMODERX_UNSET)
    {
        ::dcoyang::infinera_dco_otu::Otus_Otu_Config_PrbsMonitoringModeRx prbsMonRx;

        switch (pObjIn->config().prbs_monitoring_mode_rx())
        {
            case ::dcoyang::infinera_dco_otu::Otus_Otu_Config::PRBSMONITORINGMODERX_PRBS_X31:
                prbsMonRx = ::dcoyang::infinera_dco_otu::Otus_Otu_Config::PRBSMONITORINGMODERX_PRBS_X31;
                break;
            case ::dcoyang::infinera_dco_otu::Otus_Otu_Config::PRBSMONITORINGMODERX_PRBS_X31_INV:
                prbsMonRx = ::dcoyang::infinera_dco_otu::Otus_Otu_Config::PRBSMONITORINGMODERX_PRBS_X31_INV;
                break;
            default:
                prbsMonRx = ::dcoyang::infinera_dco_otu::Otus_Otu_Config::PRBSMONITORINGMODERX_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Monitor Rx = " << prbsMonRx);

        pObjOut->mutable_config()->set_prbs_monitoring_mode_rx(prbsMonRx);
    }
#endif

    if (pObjIn->config().prbs_gen_rx() != ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuPrbsMode prbsGenRx;

        switch (pObjIn->config().prbs_gen_rx())
        {
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31:
                prbsGenRx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV:
                prbsGenRx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
                break;
            default:
                prbsGenRx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Generator Rx = " << prbsGenRx;

        pObjOut->mutable_config()->set_prbs_gen_rx(prbsGenRx);
    }

    if (pObjIn->config().prbs_gen_tx() != ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuPrbsMode prbsGenTx;

        switch (pObjIn->config().prbs_gen_tx())
        {
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31:
                prbsGenTx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV:
                prbsGenTx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
                break;
            default:
                prbsGenTx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Generator Tx = " << prbsGenTx;

        pObjOut->mutable_config()->set_prbs_gen_tx(prbsGenTx);
    }

    if (pObjIn->config().prbs_mon_rx() != ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuPrbsMode prbsMonRx;

        switch (pObjIn->config().prbs_mon_rx())
        {
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31:
                prbsMonRx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV:
                prbsMonRx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
                break;
            default:
                prbsMonRx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Monitor Rx = " << prbsMonRx;

        pObjOut->mutable_config()->set_prbs_mon_rx(prbsMonRx);
    }

    if (pObjIn->config().prbs_mon_tx() != ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOtuPrbsMode prbsMonTx;

        switch (pObjIn->config().prbs_mon_tx())
        {
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31:
                prbsMonTx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV:
                prbsMonTx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_X31_INV;
                break;
            default:
                prbsMonTx = ::dcoyang::enums::InfineraDcoOtuPrbsMode::INFINERADCOOTUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Monitor Tx = " << prbsMonTx;

        pObjOut->mutable_config()->set_prbs_mon_tx(prbsMonTx);
    }

    if (pObjIn->config().carrier_channel() != Otus_Otu_Config::CARRIERCHANNEL_UNSET)
    {
        Otus_Otu_Config_CarrierChannel channel;
        switch (pObjIn->config().carrier_channel())
        {
            case Otus_Otu_Config::CARRIERCHANNEL_ONE:
                channel = Otus_Otu_Config::CARRIERCHANNEL_ONE;
                break;
            case Otus_Otu_Config::CARRIERCHANNEL_TWO:
                channel = Otus_Otu_Config::CARRIERCHANNEL_TWO;
                break;
            case Otus_Otu_Config::CARRIERCHANNEL_BOTH:
                channel = Otus_Otu_Config::CARRIERCHANNEL_BOTH;
                break;
            default:
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Carrier Channel = " << channel;

        pObjOut->mutable_config()->set_carrier_channel(channel);
    }

    INFN_LOG(SeverityLevel::info) << "Done";
}


//
// CLI Commands
//
void SimDcoOtuMgr::dumpParamNames(std::ostream& out)
{
    for (uint paramIdx = 0; paramIdx < NUM_SIM_OTU_PARAMS; paramIdx++)
    {
        SIM_OTU_PARAMS simOtuParam = static_cast<SIM_OTU_PARAMS>(paramIdx);
        out << "[" << paramIdx << "]  " << simOtuParamsToString.at(simOtuParam) << std::endl;
    }
}

void SimDcoOtuMgr::setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "Set Otu Sim state param " << paramName;

    uint paramIdx;
    for (paramIdx = 0; paramIdx < NUM_SIM_OTU_PARAMS; paramIdx++)
    {
        SIM_OTU_PARAMS simOtuParam = static_cast<SIM_OTU_PARAMS>(paramIdx);
        if (boost::iequals(paramName, simOtuParamsToString.at(simOtuParam)))
        {
            break;
        }
    }

    if (NUM_SIM_OTU_PARAMS <= paramIdx)
    {
        INFN_LOG(SeverityLevel::info) << "Otu parameter '" << paramName << "' not valid";
        return;
    }

    Otus_OtuKey* pKey = (Otus_OtuKey*)pObjMsg;

    SIM_OTU_PARAMS simOtuParam = static_cast<SIM_OTU_PARAMS>(paramIdx);
    switch (simOtuParam)
    {
        case SIM_OTU_FAC_RCVD_TTI:
            {
            std::vector<std::string> v;
            std::stringstream ss(valueStr);
            while (ss.good())
            {
                std::string subStr;
                getline(ss, subStr, ',');
                v.push_back(subStr);
            }

            pKey->mutable_otu()->mutable_state()->clear_rx_tti();
            for (size_t i = 0; i < v.size(); i++)
            {
                uint8_t valueByte;
                valueByte = std::stoull(v[i], nullptr, 16);
                INFN_LOG(SeverityLevel::info) << "Updating Fac Rcvd Tti: byte" << i << " = 0x" << std::hex << (uint)valueByte << std::dec;
                //pKey->mutable_otu()->mutable_state()->add_rx_tti()->set_value(valueByte);

		pKey->mutable_otu()->mutable_state()->add_rx_tti()->set_tti_byte_index(i + 1);
		//pKey->mutable_otu()->mutable_state()->mutable_rx_tti(i)->set_tti_byte_index(i);
		pKey->mutable_otu()->mutable_state()->mutable_rx_tti(i)->mutable_rx_tti()->mutable_tti()->set_value(valueByte);
            }
            for (size_t i = v.size(); i < DataPlane::MAX_TTI_LENGTH; i++)
            {
		    pKey->mutable_otu()->mutable_state()->add_rx_tti()->set_tti_byte_index(i + 1);
		    //pKey->mutable_otu()->mutable_state()->mutable_rx_tti(i)->set_tti_byte_index(i);
		    pKey->mutable_otu()->mutable_state()->mutable_rx_tti(i)->mutable_rx_tti()->mutable_tti()->set_value(0x0);
                //pKey->mutable_otu()->mutable_state()->add_rx_tti()->set_value(0);
            }
            break;
            }
        case SIM_OTU_FAULT:
            {
            uint64_t valueULL = std::stoull(valueStr, nullptr, 16);
            INFN_LOG(SeverityLevel::info) << "Updating Rx Fault = 0x" << std::hex << valueULL << std::dec;
            pKey->mutable_otu()->mutable_state()->mutable_rx_fault_status()->set_value(valueULL);
            break;
            }
#if 0
        // May need to add this base on latest Yang changes.
        case SIM_OTU_TX_FAULT:
            {
            uint64_t valueULL = std::stoull(valueStr, nullptr, 16);
            INFN_LOG(SeverityLevel::info) << "Updating Tx Fault = 0x" << std::hex << valueULL << std::dec;
            pKey->mutable_otu()->mutable_state()->mutable_tx_fault_status()->set_value(valueULL);
            break;
            }
#endif
        default:
            break;
    }
}


} //namespace DpSim
