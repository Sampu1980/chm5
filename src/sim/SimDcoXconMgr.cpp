/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/

#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <infinera_dco_xcon/infinera_dco_xcon.pb.h>

#include "SimObjMgr.h"
#include "SimDcoXconMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"

using ::dcoyang::infinera_dco_xcon::Xcons;
using ::google::protobuf::util::MessageToJsonString;

using namespace dcoyang::infinera_dco_xcon;


namespace DpSim
{

// returns true if it is xcon msg; false otherwise
SimObjMgr::MSG_STAT SimDcoXconMgr::checkAndCreate(const google::protobuf::Message* pObjMsg)
{
    // Check for Xcon ..............................................................................
    const Xcons *pMsg = dynamic_cast<const Xcons*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Xcon Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->xcon(0).xcon_id();

        SimObjMgr::MSG_STAT stat = addObj(id, (google::protobuf::Message*)&(pMsg->xcon(0)), true);

        INFN_LOG(SeverityLevel::info) << "Create Done. Status: " << stat;

        return stat;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoXconMgr::checkAndRead(google::protobuf::Message* pObjMsg)
{
    // Check for Xcon ..............................................................................
    Xcons *pMsg = dynamic_cast<Xcons*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        INFN_LOG(SeverityLevel::debug) << "Determined it is Xcon Sim Msg";

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->xcon(0).xcon_id();

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        Xcons_XconKey* pCacheKey = (Xcons_XconKey*)pTempMsgCache;

        Xcons_XconKey* pKey = pMsg->add_xcon();

        *pKey                       = *pCacheKey;  // copy
        *(pMsg->mutable_xcon(0)) = *pCacheKey;  // copy

        INFN_LOG(SeverityLevel::debug) << "Read Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoXconMgr::checkAndUpdate(const google::protobuf::Message* pObjMsg)
{
    // Check for Xcon ..............................................................................
    const Xcons *pMsg = dynamic_cast<const Xcons*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Xcon Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->xcon(0).xcon_id();

        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        Xcons_XconKey* pCacheKey = (Xcons_XconKey*)pTempMsg;

        updateObj(&pMsg->xcon(0).xcon(), pCacheKey->mutable_xcon());

        INFN_LOG(SeverityLevel::info) << "Update Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoXconMgr::checkAndDelete(const google::protobuf::Message* pObjMsg)
{
    // Check for Xcon ..............................................................................
    const Xcons *pMsg = dynamic_cast<const Xcons*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Xcon Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = pMsg->xcon(0).xcon_id();

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
void SimDcoXconMgr::createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                        google::protobuf::Message*& pNewObj)
{
    INFN_LOG(SeverityLevel::info) << "Create Xcon Sim config object";

    const Xcons_XconKey* pKeyFrom = (const Xcons_XconKey*)pFromObj;

    Xcons_XconKey* pKeyNew = new Xcons_XconKey(*pKeyFrom);

    pNewObj = (google::protobuf::Message*)pKeyNew;

    INFN_LOG(SeverityLevel::info) << "Done";
}

void SimDcoXconMgr::createStateMsgObj(google::protobuf::Message* pObjMsg)
{
    INFN_LOG(SeverityLevel::info) << "";
}

void SimDcoXconMgr::updateObj(const ::dcoyang::infinera_dco_xcon::Xcons_Xcon* pObjIn,
                                    ::dcoyang::infinera_dco_xcon::Xcons_Xcon* pObjOut)
{
    INFN_LOG(SeverityLevel::info) << "Update Xcon Sim config";

    Xcons_Xcon_Config_Direction dir;

    if (pObjIn->config().direction() != Xcons_Xcon_Config::DIRECTION_UNSET)
    {
        Xcons_Xcon_Config_Direction dir;

        switch (pObjIn->config().direction())
        {
            case Xcons_Xcon_Config::DIRECTION_INGRESS:
                dir = Xcons_Xcon_Config::DIRECTION_INGRESS;
                break;
            case Xcons_Xcon_Config::DIRECTION_EGRESS:
                dir = Xcons_Xcon_Config::DIRECTION_EGRESS;
                break;
            case Xcons_Xcon_Config::DIRECTION_BIDIRECTIONAL:
                dir = Xcons_Xcon_Config::DIRECTION_BIDIRECTIONAL;
                break;
            default:
                dir = Xcons_Xcon_Config::DIRECTION_UNSET;
                break;
        }

        INFN_LOG(SeverityLevel::info) << "Update Direction = " << dir;

        pObjOut->mutable_config()->set_direction(dir);
    }

    INFN_LOG(SeverityLevel::info) << "Done";
}


//
// CLI Commands
//
void SimDcoXconMgr::dumpParamNames(std::ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";
}

void SimDcoXconMgr::setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "";
}


} //namespace DpSim

