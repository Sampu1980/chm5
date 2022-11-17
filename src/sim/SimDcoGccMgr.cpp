/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/

#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include "SimObjMgr.h"
#include "SimDcoGccMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"

using ::dcoyang::infinera_dco_gcc_control::DcoGccControl;
using ::google::protobuf::util::MessageToJsonString;

using namespace dcoyang::infinera_dco_gcc_control;


namespace DpSim
{

// returns true if it is gcc msg; false otherwise
SimObjMgr::MSG_STAT SimDcoGccMgr::checkAndCreate(const google::protobuf::Message* pObjMsg)
{
    // Check for Gcc ..............................................................................
    const DcoGccControl *pMsg = dynamic_cast<const DcoGccControl*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Gcc Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = 1;

        SimObjMgr::MSG_STAT stat = addObj(id, (google::protobuf::Message*)pMsg, true);

        INFN_LOG(SeverityLevel::info) << "Create Done. Status: " << stat;

        return stat;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoGccMgr::checkAndRead(google::protobuf::Message* pObjMsg)
{
    // Check for Gcc ..............................................................................
    DcoGccControl *pMsg = dynamic_cast<DcoGccControl*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        INFN_LOG(SeverityLevel::debug) << "Determined it is Gcc Sim Msg";

        // todo: will this always be zero or need to handle multi object messages?
        uint id = 1;

        google::protobuf::Message* pTempMsgCache;
        if (!getObj(id, pTempMsgCache))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        DcoGccControl* pCacheKey = (DcoGccControl*)pTempMsgCache;

        *pMsg = *pCacheKey;  // copy

        INFN_LOG(SeverityLevel::debug) << "Read Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoGccMgr::checkAndUpdate(const google::protobuf::Message* pObjMsg)
{
    // Check for Gcc ..............................................................................
    const DcoGccControl *pMsg = dynamic_cast<const DcoGccControl*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Gcc Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = 1;

        google::protobuf::Message* pTempMsg;
        if (!getObj(id, pTempMsg))
        {
            return SimObjMgr::MS_DOESNT_EXIST;
        }

        DcoGccControl* pCacheKey = (DcoGccControl*)pTempMsg;

        updateObj(pMsg, pCacheKey);

        INFN_LOG(SeverityLevel::info) << "Update Done";

        return SimObjMgr::MS_OK;
    }

    return SimObjMgr::MS_WRONG_TYPE;
}

SimObjMgr::MSG_STAT SimDcoGccMgr::checkAndDelete(const google::protobuf::Message* pObjMsg)
{
    // Check for Gcc ..............................................................................
    const DcoGccControl *pMsg = dynamic_cast<const DcoGccControl*>(pObjMsg);
    if (pMsg)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        std::string data;
        MessageToJsonString(*pMsg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Gcc Sim Msg";
        INFN_LOG(SeverityLevel::info) << data;

        // todo: will this always be zero or need to handle multi object messages?
        uint id = 1;

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
void SimDcoGccMgr::createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                        google::protobuf::Message*& pNewObj)
{
    INFN_LOG(SeverityLevel::info) << "Create Gcc Sim config object";

    const DcoGccControl* pKeyFrom = (const DcoGccControl*)pFromObj;

    DcoGccControl* pKeyNew = new DcoGccControl(*pKeyFrom);

    //pKeyNew->mutable_config()->mutable_vlan_table(0)->set_application_id(1);

    pNewObj = (google::protobuf::Message*)pKeyNew;

    INFN_LOG(SeverityLevel::info) << "Done";
}

void SimDcoGccMgr::createStateMsgObj(google::protobuf::Message* pObjMsg)
{
    INFN_LOG(SeverityLevel::info) << "";
}

//
// CLI Commands
//
void SimDcoGccMgr::dumpParamNames(std::ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";
}

void SimDcoGccMgr::setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "";
}

void SimDcoGccMgr::updateObj(const ::dcoyang::infinera_dco_gcc_control::DcoGccControl* pObjIn,
                                    ::dcoyang::infinera_dco_gcc_control::DcoGccControl* pObjOut)
{
}


} //namespace DpSim

