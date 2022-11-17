/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/

#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <infinera_dco_card/infinera_dco_card.pb.h>
#include <boost/algorithm/string/predicate.hpp>

#include "SimObjMgr.h"
#include "SimDcoCardMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"
#include "SimDcoCardCapabilities.h"
#include "SimDcoCardFaults.h"

using ::dcoyang::infinera_dco_card::DcoCard;
using DcoState = ::dcoyang::infinera_dco_card::DcoCard_State;
using DcoCard_Capabilities = ::dcoyang::infinera_dco_card::DcoCard_Capabilities;
using ::google::protobuf::util::MessageToJsonString;
using namespace dcoyang::infinera_dco_card;

namespace DpSim
{

// returns true if it is card msg; false otherwise
SimObjMgr::MSG_STAT SimDcoCardMgr::checkAndCreate(const google::protobuf::Message* pObjMsg)
{
    // Check for Card ..............................................................................
    const DcoCard *pMsg = dynamic_cast<const DcoCard*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Cards Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // For card, hard coding id as 1. Assuming only 1 card for now until key is known
        uint id = 1;

        SimObjMgr::MSG_STAT stat = addObj(id, (google::protobuf::Message*)pMsg, true);

        INFN_LOG(SeverityLevel::info) << "Create Done. Status: " << stat;

        return stat;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoCardMgr::checkAndRead(google::protobuf::Message* pObjMsg)
{
    // Check for Card ..............................................................................
    DcoCard *pMsg = dynamic_cast<DcoCard*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsgCache;

        *pMsg = *pCacheKey;  // copy

        return SimObjMgr::MS_OK;
    }

    // Check for State ..............................................................................
    DcoState *pStateMsg = dynamic_cast<DcoState*>(pObjMsg);
    if (pStateMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsgCache;

        *pStateMsg = pCacheKey->state();  // copy

        return SimObjMgr::MS_OK;
    }

    // Check for Fault Capabilities ..............................................................................
    DcoCardFault *pFaultMsg = dynamic_cast<DcoCardFault*>(pObjMsg);
    if (pFaultMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        for (auto entry : eqptFaultCapa)
        {
            DcoCardFault_Capabilities_EquipmentFaultsKey* key = pFaultMsg->mutable_capabilities()->add_equipment_faults();

            key->set_fault(entry.second.key);

            DcoCardFault_Capabilities_EquipmentFaults* eqptFault = key->mutable_equipment_faults();

            eqptFault->mutable_fault_name()->set_value(entry.second.name);
            eqptFault->mutable_service_affecting()->set_value(entry.second.serviceAffecting);
            eqptFault->mutable_bit_pos()->set_value(entry.second.bitPos);
            eqptFault->mutable_fault_description()->set_value("n/a");
            eqptFault->set_direction(entry.second.direction);
            eqptFault->set_location(entry.second.location);
            eqptFault->set_severity(entry.second.severity);
        }

        for (auto entry : dacEqptFaultCapa)
        {
            DcoCardFault_Capabilities_DacEquipmentFaultsKey* key = pFaultMsg->mutable_capabilities()->add_dac_equipment_faults();

            key->set_fault(entry.second.key);

            DcoCardFault_Capabilities_DacEquipmentFaults* eqptFault = key->mutable_dac_equipment_faults();

            eqptFault->mutable_fault_name()->set_value(entry.second.name);
            eqptFault->mutable_service_affecting()->set_value(entry.second.serviceAffecting);
            eqptFault->mutable_bit_pos()->set_value(entry.second.bitPos);
            eqptFault->mutable_fault_description()->set_value("n/a");
            eqptFault->set_direction(entry.second.direction);
            eqptFault->set_location(entry.second.location);
            eqptFault->set_severity(entry.second.severity);
        }

        for (auto entry : psEqptFaultCapa)
        {
            DcoCardFault_Capabilities_PsEquipmentFaultsKey* key = pFaultMsg->mutable_capabilities()->add_ps_equipment_faults();

            key->set_fault(entry.second.key);

            DcoCardFault_Capabilities_PsEquipmentFaults* eqptFault = key->mutable_ps_equipment_faults();

            eqptFault->mutable_fault_name()->set_value(entry.second.name);
            eqptFault->mutable_service_affecting()->set_value(entry.second.serviceAffecting);
            eqptFault->mutable_bit_pos()->set_value(entry.second.bitPos);
            eqptFault->mutable_fault_description()->set_value("n/a");
            eqptFault->set_direction(entry.second.direction);
            eqptFault->set_location(entry.second.location);
            eqptFault->set_severity(entry.second.severity);
        }

        for (auto entry : postFaultCapa)
        {
            DcoCardFault_Capabilities_PostFaultsKey* key = pFaultMsg->mutable_capabilities()->add_post_faults();

            key->set_fault(entry.second.key);

            DcoCardFault_Capabilities_PostFaults* postFault = key->mutable_post_faults();

            postFault->mutable_fault_name()->set_value(entry.second.name);
            postFault->mutable_bit_pos()->set_value(entry.second.bitPos);
            postFault->mutable_fault_description()->set_value("n/a");
            postFault->set_direction(entry.second.direction);
            postFault->set_location(entry.second.location);
            postFault->set_severity(entry.second.severity);
        }

        for (auto entry : dacPostFaultCapa)
        {
            DcoCardFault_Capabilities_DacPostFaultsKey* key = pFaultMsg->mutable_capabilities()->add_dac_post_faults();

            key->set_fault(entry.second.key);

            DcoCardFault_Capabilities_DacPostFaults* postFault = key->mutable_dac_post_faults();

            postFault->mutable_fault_name()->set_value(entry.second.name);
            postFault->mutable_bit_pos()->set_value(entry.second.bitPos);
            postFault->mutable_fault_description()->set_value("n/a");
            postFault->set_direction(entry.second.direction);
            postFault->set_location(entry.second.location);
            postFault->set_severity(entry.second.severity);
        }

        for (auto entry : psPostFaultCapa)
        {
            DcoCardFault_Capabilities_PsPostFaultsKey* key = pFaultMsg->mutable_capabilities()->add_ps_post_faults();

            key->set_fault(entry.second.key);

            DcoCardFault_Capabilities_PsPostFaults* postFault = key->mutable_ps_post_faults();

            postFault->mutable_fault_name()->set_value(entry.second.name);
            postFault->mutable_bit_pos()->set_value(entry.second.bitPos);
            postFault->mutable_fault_description()->set_value("n/a");
            postFault->set_direction(entry.second.direction);
            postFault->set_location(entry.second.location);
            postFault->set_severity(entry.second.severity);
        }

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoCardMgr::checkAndUpdate(const google::protobuf::Message* pObjMsg)
{
    // Check for Card ..............................................................................
    const DcoCard *pMsg = dynamic_cast<const DcoCard*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Cards Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;

        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsg;

        updateObj(pMsg, pCacheKey);

        INFN_LOG(SeverityLevel::info) << "Update Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoCardMgr::checkAndDelete(const google::protobuf::Message* pObjMsg)
{
    // Check for Card ..............................................................................
    const DcoCard *pMsg = dynamic_cast<const DcoCard*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Cards Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;

        if (!delObj(id))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        INFN_LOG(SeverityLevel::info) << "DELETE DONE";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

// Create the memory for derived type which will be help by base pointer in base class
void SimDcoCardMgr::createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                        google::protobuf::Message*& pNewObj)
{
    INFN_LOG(SeverityLevel::info) << "";
    const DcoCard* pKeyFrom = (const DcoCard*)pFromObj;

    DcoCard* pKeyNew = new DcoCard(*pKeyFrom);

    pKeyNew->mutable_config()->mutable_name()->set_value("DcoCardSim");

    pNewObj = (google::protobuf::Message*)pKeyNew;

    INFN_LOG(SeverityLevel::info) << "DONE";
}

void SimDcoCardMgr::createStateMsgObj(google::protobuf::Message* pObjMsg)
{
    INFN_LOG(SeverityLevel::info) << "Create Card Sim state object";

    DcoCard* pCard = (DcoCard*)pObjMsg;

    pCard->mutable_state()->mutable_firmware_version()->set_value("1.94.18");

    pCard->mutable_state()->mutable_fpga_version()->set_value(246);

    pCard->mutable_state()->mutable_mcu_version()->set_value(357);

    pCard->mutable_state()->mutable_bootloader_version()->set_value("1.2");

    pCard->mutable_state()->set_board_state(DcoState::BOARDSTATE_DSP_CONFIG);
    pCard->mutable_state()->set_post_status(DcoState::POSTSTATUS_POST_SUCCEDED);

    pCard->mutable_state()->mutable_temp_high_fan_increase()->set_value(false);
    pCard->mutable_state()->mutable_temp_stable_fan_decrease()->set_value(false);
    pCard->mutable_state()->mutable_max_packet_length()->set_value(1518);

    // DCO capabilities
    DcoCard_Capabilities* pCardCapa = pCard->mutable_capabilities();

    for (auto& en : supportedClientVal)
    {
        pCardCapa->add_supported_clients(en);
    }

    for (auto& en: supportLineVal)
    {
        DcoCard_Capabilities_SupportedLineModesKey* pLmKey = pCardCapa->add_supported_line_modes();
        pLmKey->set_carrier_mode_designation(en.lineMode);

        DcoCard_Capabilities_SupportedLineModes* pLm = pLmKey->mutable_supported_line_modes();

        pLm->mutable_capacity()->set_value(en.dataRate);
        pLm->set_client_mode(en.clientMode);
        pLm->mutable_baud_rate()->set_digits(en.baudRateDigits);
        pLm->mutable_baud_rate()->set_precision(en.baudRatePrecision);
        pLm->mutable_primary_app_code()->set_value(en.appCode);
        pLm->mutable_compatability_id()->set_value(en.compId);
        pLm->set_carrier_mode_status(en.carrierModeStatus);
        pLm->mutable_max_dco_power()->set_value(en.maxPower);
    }

    for (auto& en: supportAdvParmsVal)
    {
        DcoCard_Capabilities_SupportedAdvancedParameterKey* pAdvKey = pCardCapa->add_supported_advanced_parameter();

        pAdvKey->set_ap_key(en.name);

        DcoCard_Capabilities_SupportedAdvancedParameter* pAdv = pAdvKey->mutable_supported_advanced_parameter();

        pAdv->mutable_ap_type()->set_value(en.dateType);
        pAdv->mutable_supported_values()->set_value(en.value);
        pAdv->set_ap_direction(en.direction);
        pAdv->mutable_ap_multiplicity()->set_value(en.multiplicity);
        pAdv->set_ap_configuration_impact(en.cfgImpact);
        pAdv->set_ap_service_impact(en.servImpact);
        pAdv->mutable_ap_description()->set_value(en.description);
    }

    // DCO card faults
    pCard->mutable_state()->mutable_equipment_fault()->set_value(0);
    pCard->mutable_state()->mutable_dac_equipment_fault()->set_value(0);
    pCard->mutable_state()->mutable_ps_equipment_fault()->set_value(0);

    pCard->mutable_state()->mutable_post_fault()->set_value(0);
    pCard->mutable_state()->mutable_dac_post_fault()->set_value(0);
    pCard->mutable_state()->mutable_ps_post_fault()->set_value(0);
}

void SimDcoCardMgr::dumpParamNames(std::ostream& out)
{
    for (uint paramIdx = 0; paramIdx < NUM_SIM_CARD_PARAMS; paramIdx++)
    {
        SIM_CARD_PARAMS simCardParam = static_cast<SIM_CARD_PARAMS>(paramIdx);
        out << "[" << paramIdx << "]  " << simCardParamsToString.at(simCardParam) << std::endl;
    }
}

void SimDcoCardMgr::setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "Set Card Sim state param " << paramName;

    uint paramIdx;
    for (paramIdx = 0; paramIdx < NUM_SIM_CARD_PARAMS; paramIdx++)
    {
        SIM_CARD_PARAMS simCardParam = static_cast<SIM_CARD_PARAMS>(paramIdx);
        if (boost::iequals(paramName, simCardParamsToString.at(simCardParam)))
        {
            break;
        }
    }

    if (NUM_SIM_CARD_PARAMS <= paramIdx)
    {
        INFN_LOG(SeverityLevel::info) << "Card parameter '" << paramName << "' not valid";
        return;
    }

    DcoCard* pCard = (DcoCard*)pObjMsg;
    SIM_CARD_PARAMS simCardParam = static_cast<SIM_CARD_PARAMS>(paramIdx);
    switch (simCardParam)
    {
        case SIM_CARD_ZYNQ_FW_VER:
        {
            pCard->mutable_state()->mutable_firmware_version()->set_value(valueStr);
            break;
        }

        case SIM_CARD_DCO_CAP:
        {
            INFN_LOG(SeverityLevel::info) << "NOT YET IMPLEMENTED";
            break;
        }

        case SIM_CARD_STATE:
        {
            bool isNum;
            uint cardStateInt;
            try
            {
                cardStateInt = std::stoi(valueStr);
                isNum = true;
            }
            catch(...)
            {
                isNum = false;
            }

            if (!isNum || (cardStateInt >= DcoState::BoardState_MAX))
            {
                INFN_LOG(SeverityLevel::error) << "cardState must be integer less than " << DcoState::BoardState_MAX;
                return;
            }

            ::dcoyang::infinera_dco_card::DcoCard_State_BoardState cardStateVal =
                    (::dcoyang::infinera_dco_card::DcoCard_State_BoardState)cardStateInt;

            pCard->mutable_state()->set_board_state(cardStateVal);
            break;
        }

        default:
            INFN_LOG(SeverityLevel::info) << "Unsupported simCardParam: " << simCardParam;
            break;
    }

}

void SimDcoCardMgr::updateCardEqptFault(std::ostream& out, std::string faultName, bool set)
{
    auto it = eqptFaultCapa.find(faultName);
    if (it != eqptFaultCapa.end())
    {
        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;
        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            out << "DCO state message doesn't exist. " << std::endl;
            return;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsg;

        // Read current DCO card faults
        uint64_t eqptFaultBitmap = pCacheKey->mutable_state()->mutable_equipment_fault()->value();

        // Update DCO card eqpt faults
        if (set == true)
        {
            eqptFaultBitmap |= 1ULL << (*it).second.bitPos;
        }
        else
        {
            eqptFaultBitmap &= ~(1ULL << (*it).second.bitPos);
        }

        out << "Fault bitpos : " << (*it).second.bitPos << std::endl;

        pCacheKey->mutable_state()->mutable_equipment_fault()->set_value(eqptFaultBitmap);

        out << "Updated DCO card eqpt fault bitmap to: 0x" << std::hex << (uint64_t)eqptFaultBitmap << std::dec << std::endl;
        return;
    }

    auto itd = dacEqptFaultCapa.find(faultName);
    if (itd != dacEqptFaultCapa.end())
    {
        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;
        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            out << "DCO state message doesn't exist. " << std::endl;
            return;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsg;

        // Read current DCO card faults
        uint64_t dacEqptFaultBitmap = pCacheKey->mutable_state()->mutable_dac_equipment_fault()->value();

        // Update DCO card eqpt faults
        if (set == true)
        {
            dacEqptFaultBitmap |= 1ULL << (*itd).second.bitPos;
        }
        else
        {
            dacEqptFaultBitmap &= ~(1ULL << (*itd).second.bitPos);
        }

        out << "Fault bitpos : " << (*itd).second.bitPos << std::endl;

        pCacheKey->mutable_state()->mutable_dac_equipment_fault()->set_value(dacEqptFaultBitmap);

        out << "Updated DCO card dac_eqpt fault bitmap to: 0x" << std::hex << (uint64_t)dacEqptFaultBitmap << std::dec << std::endl;
        return;
    }

    auto itp = psEqptFaultCapa.find(faultName);
    if (itp != psEqptFaultCapa.end())
    {
        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;
        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            out << "DCO state message doesn't exist. " << std::endl;
            return;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsg;

        // Read current DCO card faults
        uint64_t psEqptFaultBitmap = pCacheKey->mutable_state()->mutable_ps_equipment_fault()->value();

        // Update DCO card eqpt faults
        if (set == true)
        {
            psEqptFaultBitmap |= 1ULL << (*itp).second.bitPos;
        }
        else
        {
            psEqptFaultBitmap &= ~(1ULL << (*itp).second.bitPos);
        }

        out << "Fault bitpos : " << (*itp).second.bitPos << std::endl;

        pCacheKey->mutable_state()->mutable_ps_equipment_fault()->set_value(psEqptFaultBitmap);

        out << "Updated DCO card ps_eqpt fault bitmap to: 0x" << std::hex << (uint64_t)psEqptFaultBitmap << std::dec << std::endl;
        return;
    }

    out << "DCO eqpt fault \"" << faultName << "\" cannot be found in Capability table." << std::endl;
    return;
}

void SimDcoCardMgr::updateCardPostFault(std::ostream& out, std::string faultName, bool set)
{
    auto it = postFaultCapa.find(faultName);
    if (it != postFaultCapa.end())
    {
        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;
        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            out << "DCO state message doesn't exist. " << std::endl;
            return;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsg;

        // Read current DCO card faults
        uint64_t postFaultBitmap = pCacheKey->mutable_state()->mutable_post_fault()->value();

        // Update DCO card eqpt faults
        if (set == true)
        {
            postFaultBitmap |= 1ULL << (*it).second.bitPos;
        }
        else
        {
            postFaultBitmap &= ~(1ULL << (*it).second.bitPos);
        }
        out << "Fault bitpos : " << (*it).second.bitPos << std::endl;

        pCacheKey->mutable_state()->mutable_post_fault()->set_value(postFaultBitmap);

        out << "Updated DCO card post fault bitmap to: 0x" << std::hex << postFaultBitmap << std::dec << std::endl;
        return;
    }

    auto itd = dacPostFaultCapa.find(faultName);
    if (itd != dacPostFaultCapa.end())
    {
        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;
        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            out << "DCO state message doesn't exist. " << std::endl;
            return;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsg;

        // Read current DCO card faults
        uint64_t dacPostFaultBitmap = pCacheKey->mutable_state()->mutable_dac_post_fault()->value();

        // Update DCO card eqpt faults
        if (set == true)
        {
            dacPostFaultBitmap |= 1ULL << (*itd).second.bitPos;
        }
        else
        {
            dacPostFaultBitmap &= ~(1ULL << (*itd).second.bitPos);
        }
        out << "Fault bitpos : " << (*itd).second.bitPos << std::endl;

        pCacheKey->mutable_state()->mutable_dac_post_fault()->set_value(dacPostFaultBitmap);

        out << "Updated DCO card dac_post fault bitmap to: 0x" << std::hex << dacPostFaultBitmap << std::dec << std::endl;
        return;
    }

    auto itp = psPostFaultCapa.find(faultName);
    if (itp != psPostFaultCapa.end())
    {
        // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
        uint id = 1;
        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            out << "DCO state message doesn't exist. " << std::endl;
            return;
        }

        DcoCard* pCacheKey = (DcoCard*)pTempMsg;

        // Read current DCO card faults
        uint64_t psPostFaultBitmap = pCacheKey->mutable_state()->mutable_ps_post_fault()->value();

        // Update DCO card eqpt faults
        if (set == true)
        {
            psPostFaultBitmap |= 1ULL << (*itp).second.bitPos;
        }
        else
        {
            psPostFaultBitmap &= ~(1ULL << (*itp).second.bitPos);
        }
        out << "Fault bitpos : " << (*itp).second.bitPos << std::endl;

        pCacheKey->mutable_state()->mutable_ps_post_fault()->set_value(psPostFaultBitmap);

        out << "Updated DCO card ps_post fault bitmap to: 0x" << std::hex << psPostFaultBitmap << std::dec << std::endl;
        return;
    }

    out << "DCO post fault " << faultName << " cannot be found in Capability table." << std::endl;
    return;
}

void SimDcoCardMgr::clearAllCardEqptFault(std::ostream& out)
{
    // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
    uint id = 1;
    google::protobuf::Message* pTempMsg;
    if (!getObj(id, pTempMsg))
    {
        out << "DCO state message doesn't exist. ";
        return;
    }

    DcoCard* pCacheKey = (DcoCard*)pTempMsg;

    pCacheKey->mutable_state()->mutable_equipment_fault()->set_value(0ULL);
    pCacheKey->mutable_state()->mutable_dac_equipment_fault()->set_value(0ULL);
    pCacheKey->mutable_state()->mutable_ps_equipment_fault()->set_value(0ULL);

    out << "Cleared DCO card all eqpt fault bitmap = 0"  << std::endl;
}

void SimDcoCardMgr::clearAllCardPostFault(std::ostream& out)
{
    // For card, hard coding id as 1. Assuming only 1 card. May need to modify this if more than 1 supported
    uint id = 1;
    google::protobuf::Message* pTempMsg;
    if (!getObj(id, pTempMsg))
    {
        out << "DCO state message doesn't exist. ";
        return;
    }

    DcoCard* pCacheKey = (DcoCard*)pTempMsg;

    pCacheKey->mutable_state()->mutable_post_fault()->set_value(0ULL);
    pCacheKey->mutable_state()->mutable_dac_post_fault()->set_value(0ULL);
    pCacheKey->mutable_state()->mutable_ps_post_fault()->set_value(0ULL);

    out << "Cleared DCO card all post fault bitmap = 0"  << std::endl;
}

void SimDcoCardMgr::updateObj(const ::dcoyang::infinera_dco_card::DcoCard* pObjIn,
                                    ::dcoyang::infinera_dco_card::DcoCard* pObjOut)
{
    if (pObjIn->config().client_mode() != dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_UNSET)
    {
        dcoyang::infinera_dco_card::DcoCard_Config_ClientMode clientMode;

        switch (pObjIn->config().client_mode())
        {
            case ::dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_LXTP_E:
                clientMode = ::dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_LXTP_E;
                break;
            case ::dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_LXTP_M:
                clientMode = ::dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_LXTP_M;
                break;
            case ::dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_LXTP:
                clientMode = ::dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_LXTP;
                break;
            default:
                clientMode = ::dcoyang::infinera_dco_card::DcoCard_Config::CLIENTMODE_UNSET;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Client Mode = " << clientMode;

        pObjOut->mutable_config()->set_client_mode(clientMode);
    }

    if (pObjIn->config().has_max_packet_length())
    {
        uint32_t maxPacketLength = pObjIn->config().max_packet_length().value();

        INFN_LOG(SeverityLevel::info) << "Update max packet length = " << maxPacketLength;

        pObjOut->mutable_config()->mutable_max_packet_length()->set_value(maxPacketLength);
    }
}

} //namespace DpSim
