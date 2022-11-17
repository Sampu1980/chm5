/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/

#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <infinera_dco_carrier/infinera_dco_carrier.pb.h>

#include "SimObjMgr.h"
#include "SimCrudService.h"
#include "SimDcoCarrierMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"

using ::dcoyang::infinera_dco_carrier::Carriers;
using ::google::protobuf::util::MessageToJsonString;

using namespace dcoyang::infinera_dco_carrier;


namespace DpSim
{

// returns true if it is carrier msg; false otherwise
SimObjMgr::MSG_STAT SimDcoCarrierMgr::checkAndCreate(const google::protobuf::Message* pObjMsg)
{
    // Check for Carrier ..............................................................................
    const Carriers *pMsg = dynamic_cast<const Carriers*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Carrier Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->carrier(0).carrier_id();

        SimObjMgr::MSG_STAT stat = addObj(id, (google::protobuf::Message*)&(pMsg->carrier(0)), true);

        INFN_LOG(SeverityLevel::info) << "Create Done. Status: " << stat;

        return stat;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoCarrierMgr::checkAndRead(google::protobuf::Message* pObjMsg)
{
    // Check for Carrier ..............................................................................
    Carriers *pMsg = dynamic_cast<Carriers*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        INFN_LOG(SeverityLevel::debug) << "Determined it is Carrier Sim Msg";

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->carrier(0).carrier_id();

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        Carriers_CarrierKey* pCacheKey = (Carriers_CarrierKey*)pTempMsgCache;

        Carriers_CarrierKey* pKey = pMsg->add_carrier();

        *pKey                       = *pCacheKey;  // copy
        *(pMsg->mutable_carrier(0)) = *pCacheKey;  // copy

        INFN_LOG(SeverityLevel::debug) << "Read Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoCarrierMgr::checkAndUpdate(const google::protobuf::Message* pObjMsg)
{
    // Check for Carrier ..............................................................................
    const Carriers *pMsg = dynamic_cast<const Carriers*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Carrier Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->carrier(0).carrier_id();

        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
#define CARRIER_IS_DISABLED_INSTEAD_OF_DELETED
#ifdef CARRIER_IS_DISABLED_INSTEAD_OF_DELETED
            return SimObjMgr::MS_OK;
#else
            return SimObjMgr::MS_DOESNT_EXIST;
#endif
        }

        Carriers_CarrierKey* pCarMsg = (Carriers_CarrierKey*)pTempMsg;

        updateObj(&pMsg->carrier(0).carrier(), pCarMsg->mutable_carrier());

        INFN_LOG(SeverityLevel::info) << "Update Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoCarrierMgr::checkAndDelete(const google::protobuf::Message* pObjMsg)
{
    // Check for Carrier ..............................................................................
    const Carriers *pMsg = dynamic_cast<const Carriers*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Carrier Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->carrier(0).carrier_id();

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
void SimDcoCarrierMgr::createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                           google::protobuf::Message*& pNewObj)
{
    INFN_LOG(SeverityLevel::info) << "Create Carrier Sim config object";

    const Carriers_CarrierKey* pKeyFrom = (const Carriers_CarrierKey*)pFromObj;

    Carriers_CarrierKey* pKeyNew = new Carriers_CarrierKey(*pKeyFrom);

    pNewObj = (google::protobuf::Message*)pKeyNew;

    INFN_LOG(SeverityLevel::info) << "Done";
}

void SimDcoCarrierMgr::createStateMsgObj(google::protobuf::Message* pObjMsg)
{
    INFN_LOG(SeverityLevel::info) << "Create Carrier Sim state object";

    Carriers_CarrierKey* pKey = (Carriers_CarrierKey*)pObjMsg;

    uint id = pKey->carrier_id();
    pKey->mutable_carrier()->mutable_state()->mutable_carrier_id()->set_value(id);
    pKey->mutable_carrier()->mutable_state()->set_tx_carrier_state(::dcoyang::enums::InfineraDcoCarrierOpticalState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE);
    pKey->mutable_carrier()->mutable_state()->set_rx_carrier_state(::dcoyang::enums::InfineraDcoCarrierOpticalState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE);
    pKey->mutable_carrier()->mutable_state()->mutable_fec_iteration()->set_value(0.0);
    pKey->mutable_carrier()->mutable_state()->mutable_fec_oh_percentage()->set_value(0.0);
    pKey->mutable_carrier()->mutable_state()->mutable_capacity_reported()->set_digits(0);
    pKey->mutable_carrier()->mutable_state()->mutable_capacity_reported()->set_precision(2);
    pKey->mutable_carrier()->mutable_state()->mutable_baud_rate_reported()->set_digits(0);
    pKey->mutable_carrier()->mutable_state()->mutable_baud_rate_reported()->set_precision(2);
    pKey->mutable_carrier()->mutable_state()->mutable_app_code()->set_value("P");
    pKey->mutable_carrier()->mutable_state()->mutable_line_mode()->set_value("E");
    pKey->mutable_carrier()->mutable_state()->mutable_tx_cd_pre_comp()->set_digits(0);
    pKey->mutable_carrier()->mutable_state()->mutable_tx_cd_pre_comp()->set_precision(2);
    pKey->mutable_carrier()->mutable_state()->mutable_rx_cd_status()->set_value(0);
    pKey->mutable_carrier()->mutable_state()->mutable_equipment_fault()->set_value(0x0);
    pKey->mutable_carrier()->mutable_state()->mutable_facility_fault()->set_value(0x0);

    ::dcoyang::infinera_dco_carrier::Carriers_Carrier_State_AdvancedParameterCurrentKey* apKeyOut;
    apKeyOut = pKey->mutable_carrier()->mutable_state()->add_advanced_parameter_current();
    apKeyOut->set_ap_key("interWaveGainSharing");
    apKeyOut->mutable_advanced_parameter_current()->mutable_ap_value()->set_value("1");
    apKeyOut->mutable_advanced_parameter_current()->mutable_default_()->set_value(false);
    apKeyOut->mutable_advanced_parameter_current()->mutable_default_ap_value()->set_value("0");
    apKeyOut->mutable_advanced_parameter_current()->set_ap_status(::dcoyang::infinera_dco_carrier::Carriers_Carrier_State_AdvancedParameterCurrent::APSTATUS_SET);

    apKeyOut = pKey->mutable_carrier()->mutable_state()->add_advanced_parameter_current();
    apKeyOut->set_ap_key("FFCRAvgN");
    apKeyOut->mutable_advanced_parameter_current()->mutable_ap_value()->set_value("[47,47,47,47,47,47,47,47]");
    apKeyOut->mutable_advanced_parameter_current()->mutable_default_()->set_value(false);
    apKeyOut->mutable_advanced_parameter_current()->mutable_default_ap_value()->set_value("[31,31,31,31,31,31,31,31]");
    apKeyOut->mutable_advanced_parameter_current()->set_ap_status(::dcoyang::infinera_dco_carrier::Carriers_Carrier_State_AdvancedParameterCurrent::APSTATUS_SET);

    apKeyOut = pKey->mutable_carrier()->mutable_state()->add_advanced_parameter_current();
    apKeyOut->set_ap_key("SCAvg");
    apKeyOut->mutable_advanced_parameter_current()->mutable_ap_value()->set_value("1");
    apKeyOut->mutable_advanced_parameter_current()->mutable_default_()->set_value(false);
    apKeyOut->mutable_advanced_parameter_current()->mutable_default_ap_value()->set_value("0");
    apKeyOut->mutable_advanced_parameter_current()->set_ap_status(::dcoyang::infinera_dco_carrier::Carriers_Carrier_State_AdvancedParameterCurrent::APSTATUS_SET);
}

void SimDcoCarrierMgr::updateObj(const ::dcoyang::infinera_dco_carrier::Carriers_Carrier* pObjIn,
                                       ::dcoyang::infinera_dco_carrier::Carriers_Carrier* pObjOut)
{
    INFN_LOG(SeverityLevel::info) << "";

    // todo: verify captured all parameters??

    if (pObjIn->config().has_enable_channel())
    {
        bool enable = pObjIn->config().enable_channel().value();

        INFN_LOG(SeverityLevel::info) << "Updating enable = " << enable;

        pObjOut->mutable_config()->mutable_enable_channel()->set_value(enable);
    }

    if (pObjIn->config().has_carrier_frequency())
    {
        // todo: double check this is correct??
        double digits  = pObjIn->config().carrier_frequency().digits();
        uint precision = pObjIn->config().carrier_frequency().precision();

        INFN_LOG(SeverityLevel::info) << "Updating frequency. digits = " << digits << " precision = " << precision;

        pObjOut->mutable_config()->mutable_carrier_frequency()->set_digits(digits);
        pObjOut->mutable_config()->mutable_carrier_frequency()->set_precision(precision);
    }

    if (pObjIn->config().has_carrier_frequency_offset())
    {
        double digits  = pObjIn->config().carrier_frequency_offset().digits();
        uint precision = pObjIn->config().carrier_frequency_offset().precision();

        INFN_LOG(SeverityLevel::info) << "Updating frequency offset. digits = " << digits << " precision = " << precision;

        pObjOut->mutable_config()->mutable_carrier_frequency_offset()->set_digits(digits);
        pObjOut->mutable_config()->mutable_carrier_frequency_offset()->set_precision(precision);
    }

    if (pObjIn->config().has_optical_power_target())
    {
        // todo: double check this is correct??
        double digits  = pObjIn->config().optical_power_target().digits();
        uint precision = pObjIn->config().optical_power_target().precision();

        INFN_LOG(SeverityLevel::info) << "Updating Target Power. digits = " << digits << " precision = " << precision;

        pObjOut->mutable_config()->mutable_optical_power_target()->set_digits(digits);
        pObjOut->mutable_config()->mutable_optical_power_target()->set_precision(precision);
    }

    if (pObjIn->config().has_tx_cd_pre_comp())
    {
        double digits  = pObjIn->config().tx_cd_pre_comp().digits();
        uint precision = pObjIn->config().tx_cd_pre_comp().precision();

        INFN_LOG(SeverityLevel::info) << "Updating Tx CD Pre-Compensation. digits = " << digits << " precision = " << precision;

        pObjOut->mutable_config()->mutable_tx_cd_pre_comp()->set_digits(digits);
        pObjOut->mutable_config()->mutable_tx_cd_pre_comp()->set_precision(precision);
    }

    if (pObjIn->config().has_app_code())
    {
        std::string appCode = pObjIn->config().app_code().value();

        INFN_LOG(SeverityLevel::info) << "Updating app_code = " << appCode;

        pObjOut->mutable_config()->mutable_app_code()->set_value(appCode);
    }

    if (pObjIn->config().has_capacity())
    {
        int cap = pObjIn->config().capacity().value();

        INFN_LOG(SeverityLevel::info) << "Updating capacity = " << cap;

        pObjOut->mutable_config()->mutable_capacity()->set_value(cap);
    }

    if (pObjIn->config().has_baud_rate())
    {
        double digit = pObjIn->config().baud_rate().digits();
        uint precision = pObjIn->config().baud_rate().precision();

        INFN_LOG(SeverityLevel::info) << "Updating baud rate digits = " << digit << " precision = " << precision;

        pObjOut->mutable_config()->mutable_baud_rate()->set_digits(digit);
        pObjOut->mutable_config()->mutable_baud_rate()->set_precision(precision);
    }

    pObjOut->mutable_config()->clear_advanced_parameter();
    for (int i = 0; i < pObjIn->config().advanced_parameter_size(); i++)
    {
        const ::dcoyang::infinera_dco_carrier::Carriers_Carrier_Config_AdvancedParameterKey apKeyIn = pObjIn->config().advanced_parameter(i);

        ::dcoyang::infinera_dco_carrier::Carriers_Carrier_Config_AdvancedParameterKey* apKeyOut;
        apKeyOut = pObjOut->mutable_config()->add_advanced_parameter();
        apKeyOut->set_ap_key(apKeyIn.ap_key());
        apKeyOut->mutable_advanced_parameter()->mutable_ap_value()->set_value(apKeyIn.advanced_parameter().ap_value().value());

        INFN_LOG(SeverityLevel::info) << "Updating adv param name = " << apKeyIn.ap_key() << ", adv param value = " << apKeyIn.advanced_parameter().ap_value().value();
    }

    INFN_LOG(SeverityLevel::info) << "Done";
}


//
// CLI Commands
//
void SimDcoCarrierMgr::dumpParamNames(std::ostream& out)
{
    for (uint paramIdx = 0; paramIdx < NUM_SIM_CARRIER_PARAMS; paramIdx++)
    {
        SIM_CARRIER_PARAMS simCarParam = static_cast<SIM_CARRIER_PARAMS>(paramIdx);
        out << "[" << paramIdx << "]  " << simCarrierParamsToString.at(simCarParam) << std::endl;
    }
}

void SimDcoCarrierMgr::setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "Set Carrier Sim state param " << paramName;

    uint paramIdx;
    for (paramIdx = 0; paramIdx < NUM_SIM_CARRIER_PARAMS; paramIdx++)
    {
        SIM_CARRIER_PARAMS simCarParam = static_cast<SIM_CARRIER_PARAMS>(paramIdx);
        if (boost::iequals(paramName, simCarrierParamsToString.at(simCarParam)))
        {
            break;
        }
    }

    if (NUM_SIM_CARRIER_PARAMS <= paramIdx)
    {
        INFN_LOG(SeverityLevel::info) << "Carrier parameter '" << paramName << "' not valid";
        return;
    }

    Carriers_CarrierKey* pKey = (Carriers_CarrierKey*)pObjMsg;

    SIM_CARRIER_PARAMS simCarrierParam = static_cast<SIM_CARRIER_PARAMS>(paramIdx);
    switch (simCarrierParam)
    {
        case SIM_CAR_TXSTATE:
            {
            // CARRIEROPTICALSTATE_UNSET   = 0
            // CARRIEROPTICALSTATE_ACTIVE  = 1
            // CARRIEROPTICALSTATE_STANDBY = 2
            uint valueUL = std::stoul(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating TxState = " << valueUL;
            ::dcoyang::enums::InfineraDcoCarrierOpticalState txOpticalState;
            txOpticalState = (1 == valueUL)
                           ? ::dcoyang::enums::InfineraDcoCarrierOpticalState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE
                           : ::dcoyang::enums::InfineraDcoCarrierOpticalState::INFINERADCOCARRIEROPTICALSTATE_STANDBY;
            pKey->mutable_carrier()->mutable_state()->set_tx_carrier_state(txOpticalState);
            break;
            }

        case SIM_CAR_RXSTATE:
            {
            // CARRIEROPTICALSTATE_UNSET   = 0
            // CARRIEROPTICALSTATE_ACTIVE  = 1
            // CARRIEROPTICALSTATE_STANDBY = 2
            uint valueUL = std::stoul(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating RxState = " << valueUL;
            ::dcoyang::enums::InfineraDcoCarrierOpticalState rxOpticalState;
            rxOpticalState = (1 == valueUL)
                           ? ::dcoyang::enums::InfineraDcoCarrierOpticalState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE
                           : ::dcoyang::enums::InfineraDcoCarrierOpticalState::INFINERADCOCARRIEROPTICALSTATE_STANDBY;
            pKey->mutable_carrier()->mutable_state()->set_rx_carrier_state(rxOpticalState);
            break;
            }

        case SIM_CAR_FECITER:
            {
            uint64_t valueULL = std::stoull(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating FecIteration = " << valueULL;
            pKey->mutable_carrier()->mutable_state()->mutable_fec_iteration()->set_value(valueULL);
            break;
            }

        case SIM_CAR_FECOHPERCENT:
            {
            uint64_t valueULL = std::stoull(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating FecOhPercentage = " << valueULL;
            pKey->mutable_carrier()->mutable_state()->mutable_fec_oh_percentage()->set_value(valueULL);
            break;
            }

        case SIM_CAR_CAPACITY:
            {
            double valueDouble = std::stod(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating Capacity = " << valueDouble;
            uint precision = 2;
            valueDouble *= pow(10, precision);
            pKey->mutable_carrier()->mutable_state()->mutable_capacity_reported()->set_digits(valueDouble);
            pKey->mutable_carrier()->mutable_state()->mutable_capacity_reported()->set_precision(precision);
            break;
            }

        case SIM_CAR_BAUDRATE:
            {
            double valueDouble = std::stod(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating BaudRate = " << valueDouble;
            uint precision = 2;
            valueDouble *= pow(10, precision);
            pKey->mutable_carrier()->mutable_state()->mutable_baud_rate_reported()->set_digits(valueDouble);
            pKey->mutable_carrier()->mutable_state()->mutable_baud_rate_reported()->set_precision(precision);
            break;
            }

        case SIM_CAR_APPCODE:
            {
            INFN_LOG(SeverityLevel::info) << "Updating ApplicationCode value = " << valueStr;
            pKey->mutable_carrier()->mutable_state()->mutable_app_code()->set_value(valueStr);
            break;
            }

        case SIM_CAR_LINEMODE:
            {
            INFN_LOG(SeverityLevel::info) << "Updating LineMode value = " << valueStr;
            pKey->mutable_carrier()->mutable_state()->mutable_line_mode()->set_value(valueStr);
            break;
            }

        case SIM_CAR_TXCDPRECOMP:
            {
            double valueDouble = std::stod(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating TxCdPreComp value = " << valueDouble;
            uint precision = 2;
            valueDouble *= pow(10, precision);
            pKey->mutable_carrier()->mutable_state()->mutable_tx_cd_pre_comp()->set_digits(valueDouble);
            pKey->mutable_carrier()->mutable_state()->mutable_tx_cd_pre_comp()->set_precision(precision);
            break;
            }
        case SIM_CAR_RXCD:
            // TODO: Checking with Paul to see if it need to be a double
            {
            double valueDouble = std::stod(valueStr);
            INFN_LOG(SeverityLevel::info) << "Updating RxCd value = " << valueDouble;
            pKey->mutable_carrier()->mutable_state()->mutable_rx_cd_status()->set_value(valueDouble);
            break;
            }
        case SIM_CAR_FAULT:
            {
            uint64_t valueULL = std::stoull(valueStr, nullptr, 16);
            INFN_LOG(SeverityLevel::info) << "Updating Fault = 0x" << std::hex << valueULL << std::dec;
            pKey->mutable_carrier()->mutable_state()->mutable_facility_fault()->set_value(valueULL);
            pKey->mutable_carrier()->mutable_state()->mutable_equipment_fault()->set_value(valueULL);
            break;
            }
        default:
            break;
    }
}


} //namespace DpSim
