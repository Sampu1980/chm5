/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/

#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <infinera_dco_odu/infinera_dco_odu.pb.h>

#include "SimObjMgr.h"
#include "SimCrudService.h"
#include "SimDcoOduMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"
#include "dp_common.h"

using ::dcoyang::infinera_dco_odu::Odus;
using ::google::protobuf::util::MessageToJsonString;

using namespace dcoyang::infinera_dco_odu;


namespace DpSim
{

// returns true if it is odu msg; false otherwise
SimObjMgr::MSG_STAT SimDcoOduMgr::checkAndCreate(const google::protobuf::Message* pObjMsg)
{
    // Check for Odu ..............................................................................
    const Odus *pMsg = dynamic_cast<const Odus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Odu Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->odu(0).odu_id();

        SimObjMgr::MSG_STAT stat = addObj(id, (google::protobuf::Message*)&(pMsg->odu(0)), true);

        INFN_LOG(SeverityLevel::info) << "Create Done. Status: " << stat;

        return stat;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoOduMgr::checkAndRead(google::protobuf::Message* pObjMsg)
{
    // Check for Odu ..............................................................................
    Odus *pMsg = dynamic_cast<Odus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        INFN_LOG(SeverityLevel::debug) << "Determined it is Odu Sim Msg";

#if 1
        if (mMapObjMsg.size() == 0)
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        uint idx = 0;
        for(auto& it : mMapObjMsg)
        {
            Odus_OduKey* pCacheKey = (Odus_OduKey*)it.second;

            Odus_OduKey* pKey = pMsg->add_odu();

            *pKey                     = *pCacheKey;  // copy
            *(pMsg->mutable_odu(idx)) = *pCacheKey;  // copy

            idx++;
        }

#else
        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->odu(0).odu_id();

        pMsg->add_odu()

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        Odus_OduKey* pCacheKey = (Odus_OduKey*)pTempMsgCache;

        Odus_OduKey* pKey = pMsg->add_odu();

        *pKey                       = *pCacheKey;  // copy
        *(pMsg->mutable_odu(0)) = *pCacheKey;  // copy

#endif
        INFN_LOG(SeverityLevel::debug) << "Read Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoOduMgr::checkAndUpdate(const google::protobuf::Message* pObjMsg)
{
    // Check for Odu ..............................................................................
    const Odus *pMsg = dynamic_cast<const Odus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Odu Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->odu(0).odu_id();

        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        Odus_OduKey* pCacheKey = (Odus_OduKey*)pTempMsg;

        updateObj(&pMsg->odu(0).odu(), pCacheKey->mutable_odu());

        INFN_LOG(SeverityLevel::info) << "Update Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoOduMgr::checkAndDelete(const google::protobuf::Message* pObjMsg)
{
    // Check for Odu ..............................................................................
    const Odus *pMsg = dynamic_cast<const Odus*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Odu Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->odu(0).odu_id();

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
void SimDcoOduMgr::createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                       google::protobuf::Message*& pNewObj)
{
    INFN_LOG(SeverityLevel::info) << "Create Odu Sim config object";

    const Odus_OduKey* pKeyFrom = (const Odus_OduKey*)pFromObj;

    Odus_OduKey* pKeyNew = new Odus_OduKey(*pKeyFrom);

    pNewObj = (google::protobuf::Message*)pKeyNew;

    INFN_LOG(SeverityLevel::info) << "Done";
}


void SimDcoOduMgr::createStateMsgObj(google::protobuf::Message* pObjMsg)
{
    INFN_LOG(SeverityLevel::info) << "Create Odu Sim state object";

    Odus_OduKey* pKey = (Odus_OduKey *)pObjMsg;

    uint id = pKey->odu_id();
    pKey->mutable_odu()->mutable_state()->mutable_odu_id()->set_value(id);
    pKey->mutable_odu()->mutable_state()->mutable_facility_fault()->set_value(0x0);

    pKey->mutable_odu()->mutable_state()->clear_rx_tti();
    for (size_t i = 0; i < DataPlane::MAX_TTI_LENGTH; i++)
    {
        //pKey->mutable_odu()->mutable_state()->add_rx_tti()->set_value(0);
        pKey->mutable_odu()->mutable_state()->add_rx_tti()->set_tti_byte_index(i + 1);
        //pKey->mutable_odu()->mutable_state()->mutable_rx_tti(i)->set_tti_byte_index(i + 1);
        pKey->mutable_odu()->mutable_state()->mutable_rx_tti(i)->mutable_rx_tti()->mutable_tti()->set_value(0x0);
    }
}

void SimDcoOduMgr::updateObj(const ::dcoyang::infinera_dco_odu::Odus_Odu* pObjIn,
                                   ::dcoyang::infinera_dco_odu::Odus_Odu* pObjOut)
{
    INFN_LOG(SeverityLevel::info) << "Update Odu Sim config";

    if (pObjIn->config().service_mode() != Odus_Odu_Config::SERVICEMODE_MODE_NONE)
    {
        Odus_Odu_Config_ServiceMode srvMode;
        switch (pObjIn->config().service_mode())
        {
            case Odus_Odu_Config::SERVICEMODE_MODE_SWITCHING:
                srvMode = Odus_Odu_Config::SERVICEMODE_MODE_SWITCHING;
                break;
            case Odus_Odu_Config::SERVICEMODE_MODE_TRANSPORT:
                srvMode = Odus_Odu_Config::SERVICEMODE_MODE_TRANSPORT;
                break;
            case Odus_Odu_Config::SERVICEMODE_MODE_NONE:
                srvMode = Odus_Odu_Config::SERVICEMODE_MODE_NONE;
                break;
            default:
                srvMode = Odus_Odu_Config::SERVICEMODE_MODE_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Service Mode = " << srvMode;

        pObjOut->mutable_config()->set_service_mode(srvMode);
    }

    if (pObjIn->config().prbs_gen_tx() != ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduPrbsMode prbsGenTx;

        switch (pObjIn->config().prbs_gen_tx())
        {
            case ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE:
                prbsGenTx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
            default:
                prbsGenTx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Generator Tx = " << prbsGenTx;

        pObjOut->mutable_config()->set_prbs_gen_tx(prbsGenTx);
    }

    if (pObjIn->config().prbs_gen_rx() != ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduPrbsMode prbsGenRx;

        switch (pObjIn->config().prbs_gen_rx())
        {
            case ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE:
                prbsGenRx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
            default:
                prbsGenRx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Generator Rx = " << prbsGenRx;

        pObjOut->mutable_config()->set_prbs_gen_rx(prbsGenRx);
    }

    if (pObjIn->config().prbs_mon_tx() != ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduPrbsMode prbsMonTx;

        switch (pObjIn->config().prbs_mon_tx())
        {
            case ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE:
                prbsMonTx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
            default:
                prbsMonTx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Monitor Tx = " << prbsMonTx;

        pObjOut->mutable_config()->set_prbs_mon_tx(prbsMonTx);
    }

    if (pObjIn->config().prbs_mon_rx() != ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduPrbsMode prbsMonRx;

        switch (pObjIn->config().prbs_mon_rx())
        {
            case ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE:
                prbsMonRx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
            default:
                prbsMonRx = ::dcoyang::enums::InfineraDcoOduPrbsMode::INFINERADCOODUPRBSMODE_PRBS_NONE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Prbs Monitor Rx = " << prbsMonRx;

        pObjOut->mutable_config()->set_prbs_mon_rx(prbsMonRx);
    }

    if (pObjIn->config().ms_force_signal_tx() != ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduMaintainenceSignal msForceTx;

        switch (pObjIn->config().ms_force_signal_tx())
        {
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
                msForceTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
                msForceTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msForceTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
                msForceTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
                msForceTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
                msForceTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
                break;
            default:
                msForceTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Force Signal Tx = " << msForceTx;

        pObjOut->mutable_config()->set_ms_force_signal_tx(msForceTx);
    }

    if (pObjIn->config().ms_auto_signal_tx() != ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduMaintainenceSignal msAutoTx;

        switch (pObjIn->config().ms_auto_signal_tx())
        {
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
                msAutoTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
                msAutoTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msAutoTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
                msAutoTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
                msAutoTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
                msAutoTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
                break;
            default:
                msAutoTx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Auto Signal Tx = " << msAutoTx;

        pObjOut->mutable_config()->set_ms_auto_signal_tx(msAutoTx);
    }

    if (pObjIn->config().ms_force_signal_rx() != ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduMaintainenceSignal msForceRx;

        switch (pObjIn->config().ms_force_signal_rx())
        {
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
                msForceRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
                msForceRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msForceRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
                msForceRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
                msForceRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
                msForceRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
                break;
            default:
                msForceRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Force Signal Rx = " << msForceRx;

        pObjOut->mutable_config()->set_ms_force_signal_rx(msForceRx);
    }

    if (pObjIn->config().ms_auto_signal_rx() != ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_UNSET)
    {
        ::dcoyang::enums::InfineraDcoOduMaintainenceSignal msAutoRx;

        switch (pObjIn->config().ms_auto_signal_rx())
        {
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF:
                msAutoRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_LASEROFF;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31:
                msAutoRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV:
                msAutoRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_PRBS_X31_INV;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS:
                msAutoRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_AIS;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI:
                msAutoRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_OCI;
                break;
            case ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK:
                msAutoRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_ODU_LCK;
                break;
            default:
                msAutoRx = ::dcoyang::enums::InfineraDcoOduMaintainenceSignal::INFINERADCOODUMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Auto Signal Rx = " << msAutoRx;

        pObjOut->mutable_config()->set_ms_auto_signal_rx(msAutoRx);
    }

    for (int i = 0; i < pObjIn->config().time_slot_id_size(); i++)
    {
        INFN_LOG(SeverityLevel::info) << "Updating TimeSlot[" << i << "] = " << pObjIn->config().time_slot_id(i).value();
        pObjOut->mutable_config()->add_time_slot_id()->set_value(pObjIn->config().time_slot_id(i).value());
    }

    if (pObjIn->config().time_slot_granularity() != Odus_Odu_Config::TIMESLOTGRANULARITY_UNSET)
    {
        Odus_Odu_Config_TimeSlotGranularity tsGranularity;

        switch (pObjIn->config().time_slot_granularity())
        {
            case Odus_Odu_Config::TIMESLOTGRANULARITY_25G:
                tsGranularity = Odus_Odu_Config::TIMESLOTGRANULARITY_25G;
                break;
            default:
                tsGranularity = Odus_Odu_Config::TIMESLOTGRANULARITY_25G;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating TS Granularity = " << tsGranularity;

        pObjOut->mutable_config()->set_time_slot_granularity(tsGranularity);
    }

    if (pObjIn->config().loopback_type() != Odus_Odu_Config::LOOPBACKTYPE_UNSET)
    {
        Odus_Odu_Config_LoopbackType lpbkType;

        switch (pObjIn->config().loopback_type())
        {
            case Odus_Odu_Config::LOOPBACKTYPE_NONE:
                lpbkType = Odus_Odu_Config::LOOPBACKTYPE_NONE;
                break;
            case Odus_Odu_Config::LOOPBACKTYPE_FACILITY:
                lpbkType = Odus_Odu_Config::LOOPBACKTYPE_FACILITY;
                break;
            case Odus_Odu_Config::LOOPBACKTYPE_TERMINAL:
                lpbkType = Odus_Odu_Config::LOOPBACKTYPE_TERMINAL;
                break;
            default:
                lpbkType = Odus_Odu_Config::LOOPBACKTYPE_UNSET;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Loopback Type = " << lpbkType;

        pObjOut->mutable_config()->set_loopback_type(lpbkType);
    }

    if (pObjIn->config().payload_type() != Odus_Odu_Config::PAYLOADTYPE_UNSET)
    {
        INFN_LOG(SeverityLevel::info) << "Updating Payload = " << pObjIn->config().payload_type();

        pObjOut->mutable_config()->set_payload_type(pObjIn->config().payload_type());
    }

    INFN_LOG(SeverityLevel::info) << "Done";
}


//
// CLI Commands
//
void SimDcoOduMgr::dumpParamNames(std::ostream& out)
{
    for (uint paramIdx = 0; paramIdx < NUM_SIM_ODU_PARAMS; paramIdx++)
    {
        SIM_ODU_PARAMS simOduParam = static_cast<SIM_ODU_PARAMS>(paramIdx);
        out << "[" << paramIdx << "]  " << simOduParamsToString.at(simOduParam) << std::endl;
    }
}

void SimDcoOduMgr::setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "Set Odu Sim state param " << paramName;

    uint paramIdx;
    for (paramIdx = 0; paramIdx < NUM_SIM_ODU_PARAMS; paramIdx++)
    {
        SIM_ODU_PARAMS simOduParam = static_cast<SIM_ODU_PARAMS>(paramIdx);
        if (boost::iequals(paramName, simOduParamsToString.at(simOduParam)))
        {
            break;
        }
    }

    if (NUM_SIM_ODU_PARAMS <= paramIdx)
    {
        INFN_LOG(SeverityLevel::info) << "Odu parameter '" << paramName << "' not valid";
        return;
    }

    Odus_OduKey* pKey = (Odus_OduKey*)pObjMsg;

    SIM_ODU_PARAMS simOduParam = static_cast<SIM_ODU_PARAMS>(paramIdx);
    switch (simOduParam)
    {
        case SIM_ODU_FAC_RCVD_TTI:
            {
            std::vector<std::string> v;
            std::stringstream ss(valueStr);
            while (ss.good())
            {
                std::string subStr;
                getline(ss, subStr, ',');
                v.push_back(subStr);
            }

            pKey->mutable_odu()->mutable_state()->clear_rx_tti();
            for (size_t i = 0; i < v.size(); i++)
            {
                uint8_t valueByte;
                valueByte = std::stoull(v[i], nullptr, 16);
                INFN_LOG(SeverityLevel::info) << "Updating Fac Rcvd Tti: byte" << i << " = 0x" << std::hex << (uint)valueByte << std::dec;
                //pKey->mutable_odu()->mutable_state()->add_rx_tti()->set_value(valueByte);
		pKey->mutable_odu()->mutable_state()->add_rx_tti()->set_tti_byte_index(i + 1);
		//pKey->mutable_odu()->mutable_state()->mutable_rx_tti(i)->set_tti_byte_index(i);
		pKey->mutable_odu()->mutable_state()->mutable_rx_tti(i)->mutable_rx_tti()->mutable_tti()->set_value(valueByte);
            }
            for (size_t i = v.size(); i < DataPlane::MAX_TTI_LENGTH; i++)
            {
                //pKey->mutable_odu()->mutable_state()->add_rx_tti()->set_value(0);
		pKey->mutable_odu()->mutable_state()->add_rx_tti()->set_tti_byte_index(i + 1);
		//pKey->mutable_odu()->mutable_state()->mutable_rx_tti(i)->set_tti_byte_index(i);
		pKey->mutable_odu()->mutable_state()->mutable_rx_tti(i)->mutable_rx_tti()->mutable_tti()->set_value(0x0);
            }
            break;
            }
        case SIM_ODU_PAYLOAD_TYPE:
            {
            // PAYLOADTYPE_UNSET    = 0
            // PAYLOADTYPE_ODU4     = 1
            // PAYLOADTYPE_LO_ODU4I = 2
            // PAYLOADTYPE_LO_FLEXI = 3
            // PAYLOADTYPE_ODUCNI   = 4
            uint valueUL = std::stoul(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating PayloadType = " << valueUL;
            Odus_Odu_State_PayloadType payLoad;
            switch (valueUL)
            {
                case 1:
                    payLoad = Odus_Odu_State::PAYLOADTYPE_ODU4;
                    break;
                case 2:
                    payLoad = Odus_Odu_State::PAYLOADTYPE_LO_ODU4I;
                    break;
                case 3:
                    payLoad = Odus_Odu_State::PAYLOADTYPE_LO_FLEXI;
                    break;
                case 4:
                    payLoad = Odus_Odu_State::PAYLOADTYPE_ODUCNI;
                    break;
                default:
                    payLoad = Odus_Odu_State::PAYLOADTYPE_UNSET;
                    break;
            }
            pKey->mutable_odu()->mutable_state()->set_payload_type(payLoad);
            break;
            }
        case SIM_ODU_FAULT:
            {
            uint64_t valueULL = std::stoull(valueStr, nullptr, 16);
            INFN_LOG(SeverityLevel::info) << "Updating Rx Fault = 0x" << std::hex << valueULL << std::dec;
            pKey->mutable_odu()->mutable_state()->mutable_facility_fault()->set_value(valueULL);
            break;
            }
        default:
            break;
    }
}


} //namespace DpSim
