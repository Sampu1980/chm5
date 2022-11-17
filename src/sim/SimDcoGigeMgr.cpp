/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/

#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <infinera_dco_client_gige/infinera_dco_client_gige.pb.h>

#include "SimObjMgr.h"
#include "SimCrudService.h"
#include "SimDcoGigeMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"

using ::dcoyang::infinera_dco_client_gige::ClientsGige;
using ::google::protobuf::util::MessageToJsonString;

using namespace dcoyang::infinera_dco_client_gige;


namespace DpSim
{

// returns true if it is client_gige msg; false otherwise
SimObjMgr::MSG_STAT SimDcoGigeMgr::checkAndCreate(const google::protobuf::Message* pObjMsg)
{
    // Check for ClientGige ..............................................................................
    const ClientsGige *pMsg = dynamic_cast<const ClientsGige*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is GigeClient Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->client_gige(0).client_port_id();

        SimObjMgr::MSG_STAT stat = addObj(id, (google::protobuf::Message*)&(pMsg->client_gige(0)), true);

        INFN_LOG(SeverityLevel::info) << "Create Done. Status: " << stat;

        return stat;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoGigeMgr::checkAndRead(google::protobuf::Message* pObjMsg)
{
    // Check for ClientGige ..............................................................................
    ClientsGige *pMsg = dynamic_cast<ClientsGige*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        INFN_LOG(SeverityLevel::debug) << "Determined it is GigeClient Sim Msg";

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->client_gige(0).client_port_id();

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        ClientsGige_ClientGigeKey* pCacheKey = (ClientsGige_ClientGigeKey*)pTempMsgCache;

        ClientsGige_ClientGigeKey* pKey = pMsg->add_client_gige();

        *pKey                           = *pCacheKey;  // copy
        *(pMsg->mutable_client_gige(0)) = *pCacheKey;  // copy

        INFN_LOG(SeverityLevel::debug) << "Read Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoGigeMgr::checkAndUpdate(const google::protobuf::Message* pObjMsg)
{
    // Check for ClientGige ..............................................................................
    const ClientsGige *pMsg = dynamic_cast<const ClientsGige*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is GigeClient Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->client_gige(0).client_port_id();

        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        ClientsGige_ClientGigeKey* pCacheKey = (ClientsGige_ClientGigeKey*)pTempMsg;

        updateObj(&pMsg->client_gige(0).client_gige(), pCacheKey->mutable_client_gige());

        INFN_LOG(SeverityLevel::info) << "Update Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoGigeMgr::checkAndDelete(const google::protobuf::Message* pObjMsg)
{
    // Check for ClientGige ..............................................................................
    const ClientsGige *pMsg = dynamic_cast<const ClientsGige*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is GigeClient Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->client_gige(0).client_port_id();

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
void SimDcoGigeMgr::createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                        google::protobuf::Message*& pNewObj)
{
    INFN_LOG(SeverityLevel::info) << "Create GigeClient Sim config object";

    const ClientsGige_ClientGigeKey* pKeyFrom = (const ClientsGige_ClientGigeKey*)pFromObj;

    ClientsGige_ClientGigeKey* pKeyNew = new ClientsGige_ClientGigeKey(*pKeyFrom);

    pNewObj = (google::protobuf::Message*)pKeyNew;

    INFN_LOG(SeverityLevel::info) << "Done";
}

void SimDcoGigeMgr::createStateMsgObj(google::protobuf::Message* pObjMsg)
{
    INFN_LOG(SeverityLevel::info) << "Create GigeClient Sim state object";

    ClientsGige_ClientGigeKey* pKey = (ClientsGige_ClientGigeKey *)pObjMsg;

    uint id = pKey->client_port_id();
    pKey->mutable_client_gige()->mutable_state()->mutable_client_port_id()->set_value(id);
    pKey->mutable_client_gige()->mutable_state()->set_client_status(ClientsGige_ClientGige_State::CLIENTSTATUS_DOWN);
    pKey->mutable_client_gige()->mutable_state()->mutable_max_mtu_size()->set_value(1514);
    pKey->mutable_client_gige()->mutable_state()->mutable_facility_fault()->set_value(0x0);

    INFN_LOG(SeverityLevel::info) << "Done";
}

void SimDcoGigeMgr::updateObj(const ::dcoyang::infinera_dco_client_gige::ClientsGige_ClientGige* pObjIn,
                                    ::dcoyang::infinera_dco_client_gige::ClientsGige_ClientGige* pObjOut)
{
    INFN_LOG(SeverityLevel::info) << "Update GigeClient Sim config";

    if (pObjIn->config().loopback_type() != ClientsGige_ClientGige_Config::LOOPBACKTYPE_UNSET)
    {
        ClientsGige_ClientGige_Config_LoopbackType lpbkType;

        switch (pObjIn->config().loopback_type())
        {
            case ClientsGige_ClientGige_Config::LOOPBACKTYPE_NONE:
                lpbkType = ClientsGige_ClientGige_Config::LOOPBACKTYPE_NONE;
                break;
            case ClientsGige_ClientGige_Config::LOOPBACKTYPE_FACILITY:
                lpbkType = ClientsGige_ClientGige_Config::LOOPBACKTYPE_FACILITY;
                break;
            case ClientsGige_ClientGige_Config::LOOPBACKTYPE_TERMINAL:
                lpbkType = ClientsGige_ClientGige_Config::LOOPBACKTYPE_TERMINAL;
                break;
            default:
                lpbkType = ClientsGige_ClientGige_Config::LOOPBACKTYPE_UNSET;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Loopback Type = " << lpbkType;

        pObjOut->mutable_config()->set_loopback_type(lpbkType);
    }

    if (pObjIn->config().has_fec_enable())
    {
        bool fecEn = pObjIn->config().fec_enable().value();

        INFN_LOG(SeverityLevel::info) << "Updating Fec Enable = " << fecEn;

        pObjOut->mutable_config()->mutable_fec_enable()->set_value(fecEn);
    }

    if (pObjIn->config().has_mtu())
    {
        uint mtu = pObjIn->config().mtu().value();

        INFN_LOG(SeverityLevel::info) << "Updating Mtu = " << mtu;

        pObjOut->mutable_config()->mutable_mtu()->set_value(mtu);
    }

    if (pObjIn->config().has_lldp_rx_snooping_enable())
    {
        bool lldpRxSnoop = pObjIn->config().lldp_rx_snooping_enable().value();

        INFN_LOG(SeverityLevel::info) << "Updating LLDP Rx Snoop = " << lldpRxSnoop;

        pObjOut->mutable_config()->mutable_lldp_rx_snooping_enable()->set_value(lldpRxSnoop);
    }

    if (pObjIn->config().has_lldp_tx_snooping_enable())
    {
        bool lldpTxSnoop = pObjIn->config().lldp_tx_snooping_enable().value();

        INFN_LOG(SeverityLevel::info) << "Updating LLDP Tx Snoop = " << lldpTxSnoop;

        pObjOut->mutable_config()->mutable_lldp_tx_snooping_enable()->set_value(lldpTxSnoop);
    }

    if (pObjIn->config().has_lldp_rx_drop())
    {
        bool lldpRxDrop = pObjIn->config().lldp_rx_drop().value();

        INFN_LOG(SeverityLevel::info) << "Updating LLDP Rx Drop = " << lldpRxDrop;

        pObjOut->mutable_config()->mutable_lldp_rx_drop()->set_value(lldpRxDrop);
    }

    if (pObjIn->config().has_lldp_tx_drop())
    {
        bool lldpTxDrop = pObjIn->config().lldp_tx_drop().value();

        INFN_LOG(SeverityLevel::info) << "Updating LLDP Tx Drop = " << lldpTxDrop;

        pObjOut->mutable_config()->mutable_lldp_tx_drop()->set_value(lldpTxDrop);
    }

    if (pObjIn->config().ms_auto_signal_tx())
    {
        ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal msAutoTx;

        switch (pObjIn->config().ms_auto_signal_tx())
        {
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
                msAutoTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
                msAutoTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
                msAutoTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
                break;
            default:
                msAutoTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Auto Signal Tx = " << msAutoTx;

        pObjOut->mutable_config()->set_ms_auto_signal_tx(msAutoTx);
    }

    if (pObjIn->config().ms_force_signal_tx())
    {
        ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal msForceTx;

        switch (pObjIn->config().ms_force_signal_tx())
        {
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
                msForceTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
                msForceTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
                msForceTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
                break;
            default:
                msForceTx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Force Signal Tx = " << msForceTx;

        pObjOut->mutable_config()->set_ms_force_signal_tx(msForceTx);
    }

    if (pObjIn->config().ms_auto_signal_rx())
    {
        ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal msAutoRx;

        switch (pObjIn->config().ms_auto_signal_rx())
        {
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
                msAutoRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
                msAutoRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
                msAutoRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
                break;
            default:
                msAutoRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Auto Signal Rx = " << msAutoRx;

        pObjOut->mutable_config()->set_ms_auto_signal_rx(msAutoRx);
    }

    if (pObjIn->config().ms_force_signal_rx())
    {
        ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal msForceRx;

        switch (pObjIn->config().ms_force_signal_rx())
        {
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF:
                msForceRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_LF;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE:
                msForceRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_GIGE_IDLE;
                break;
            case ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF:
                msForceRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_RF;
                break;
            default:
                msForceRx = ::dcoyang::enums::InfineraDcoClientGigeMaintainenceSignal::INFINERADCOCLIENTGIGEMAINTAINENCESIGNAL_NOREPLACE;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Updating Force Signal Rx = " << msForceRx;

        pObjOut->mutable_config()->set_ms_force_signal_rx(msForceRx);
    }

    // Not in protobuf
    if (pObjIn->config().ethernet_mode())
    {
    }

    // Not in protobuf
    if (pObjIn->config().service_mode())
    {
    }

    // Not in protobuf
    if (pObjIn->config().payload_type())
    {
    }

    // Not in protobuf
    if (pObjIn->config().has_fwd_tda_trigger())
    {
    }

    // Not in protobuf
    if (pObjIn->config().interface_type())
    {
    }

    // Not in protobuf
    if (pObjIn->config().ethernet_flex_type())
    {
    }

    // Not in protobuf
    if (pObjIn->config().service_mode_qualifier())
    {
    }

    INFN_LOG(SeverityLevel::info) << "Done";
}


//
// CLI Commands
//
void SimDcoGigeMgr::dumpParamNames(std::ostream& out)
{
    for (uint paramIdx = 0; paramIdx < NUM_SIM_GIGE_PARAMS; paramIdx++)
    {
        SIM_GIGE_PARAMS simGigeParam = static_cast<SIM_GIGE_PARAMS>(paramIdx);
        out << "[" << paramIdx << "]  " << simGigeParamsToString.at(simGigeParam) << std::endl;
    }
}

void SimDcoGigeMgr::setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "Set GigeClient Sim state param " << paramName;

    uint paramIdx;
    for (paramIdx = 0; paramIdx < NUM_SIM_GIGE_PARAMS; paramIdx++)
    {
        SIM_GIGE_PARAMS simGigeParam = static_cast<SIM_GIGE_PARAMS>(paramIdx);
        if (boost::iequals(paramName, simGigeParamsToString.at(simGigeParam)))
        {
            break;
        }
    }

    if (NUM_SIM_GIGE_PARAMS <= paramIdx)
    {
        INFN_LOG(SeverityLevel::info) << "Gige parameter '" << paramName << "' not valid";
        return;
    }

    ClientsGige_ClientGigeKey* pKey = (ClientsGige_ClientGigeKey *)pObjMsg;

    SIM_GIGE_PARAMS simGigeParam = static_cast<SIM_GIGE_PARAMS>(paramIdx);
    switch (simGigeParam)
    {
        case SIM_GIGE_MAXMTUSIZE:
            {
            uint valueUL = std::stoul(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating MaxMtuSize = " << valueUL;
            pKey->mutable_client_gige()->mutable_state()->mutable_max_mtu_size()->set_value(valueUL);
            break;
            }
        case SIM_GIGE_LINKUP:
            {
            // CLIENTSTATUS_UNSET = 0
            // CLIENTSTATUS_UP = 1
            // CLIENTSTATUS_DOWN = 2
            // CLIENTSTATUS_FAULTED = 3
            bool valueBool = boost::iequals(valueStr, "true") ? true : false;
            INFN_LOG(SeverityLevel::info) << "Updating LinkUp = " << valueBool;
            ClientsGige_ClientGige_State_ClientStatus clientStatus;
            clientStatus = (true == valueBool) ? ClientsGige_ClientGige_State::CLIENTSTATUS_UP
                                               : ClientsGige_ClientGige_State::CLIENTSTATUS_DOWN;
            pKey->mutable_client_gige()->mutable_state()->set_client_status(clientStatus);
            break;
            }
        case SIM_GIGE_FAULT:
            {
            uint64_t valueULL = std::stoull(valueStr, nullptr, 16);
            INFN_LOG(SeverityLevel::info) << "Updating Fault = 0x" << std::hex << valueULL << std::dec;
            pKey->mutable_client_gige()->mutable_state()->mutable_facility_fault()->set_value(valueULL);
            break;
            }
        default:
            break;
    }
}


} //namespace DpSim
