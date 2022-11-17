/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include "SimObjMgr.h"
#include "DataPlaneMgrLog.h"
#include "InfnLogger.h"


using ::google::protobuf::util::MessageToJsonString;

namespace DpSim
{

void SimObjMgr::clearAllData()
{
    INFN_LOG(SeverityLevel::info) << "";

    std::lock_guard<std::mutex> lock(mMutex);

    TypeObjMsgMap::iterator it;
    for(it = mMapObjMsg.begin(); it != mMapObjMsg.end(); ++it)
    {
        INFN_LOG(SeverityLevel::info) << "Deleting " << it->first;

        delete (it->second);
    }
    mMapObjMsg.clear();
}

bool SimObjMgr::clearData(uint objId)
{
    INFN_LOG(SeverityLevel::info) << "";

    std::lock_guard<std::mutex> lock(mMutex);

    TypeObjMsgMap::iterator it = mMapObjMsg.find(objId);
    if (it == mMapObjMsg.end())
    {
        return false;
    }

    delete (it->second);
    mMapObjMsg.erase(it);

    return true;
}

bool SimObjMgr::setStateParamData(uint objId, std::string paramName, std::string valueStr)
{
    INFN_LOG(SeverityLevel::info) << "";

    std::lock_guard<std::mutex> lock(mMutex);

    TypeObjMsgMap::iterator it = mMapObjMsg.find(objId);
    if (it == mMapObjMsg.end())
    {
        return false;
    }

    setStateMsgObj(it->second, paramName, valueStr);

    return true;
}

SimObjMgr::MSG_STAT SimObjMgr::addObj(uint id, google::protobuf::Message* pObjMsg, bool isReplace)
{
    INFN_LOG(SeverityLevel::info) << "";

    TypeObjMsgMap::iterator it = mMapObjMsg.find(id);
    if (it == mMapObjMsg.end() || isReplace)
    {
        if ((isReplace) && (it != mMapObjMsg.end()))
        {
            INFN_LOG(SeverityLevel::info) << "Object already exists with id: " << id << " Replacing object with new one";
            bool delStat = delObj(id);

            INFN_LOG(SeverityLevel::info) << "Delete status: " << delStat;
        }

        google::protobuf::Message* pMsgCpy;

        createDerivedMsgObj(pObjMsg, pMsgCpy);

        createStateMsgObj(pMsgCpy);

        mMapObjMsg[id] = pMsgCpy;

        return SimObjMgr::MS_OK;
    }
    else
    {
        return SimObjMgr::MS_ALREADY_EXISTS;
    }
}

bool SimObjMgr::getObj(uint id, google::protobuf::Message*& pObjMsg)
{
    TypeObjMsgMap::iterator it = mMapObjMsg.find(id);
    if (it != mMapObjMsg.end())
    {
        pObjMsg = it->second;

        return true;
    }
    return false;
}

bool SimObjMgr::delObj(uint id)
{
    TypeObjMsgMap::iterator it = mMapObjMsg.find(id);
    if (it != mMapObjMsg.end())
    {
        delete it->second;
        mMapObjMsg.erase(it);
        return true;
    }
    return false;
}

void SimObjMgr::dumpObjConfigCache(std::ostream& out, uint objId)
{
    std::lock_guard<std::mutex> lock(mMutex);

    out << std::endl;

    if (objId == 0)
    {
        out << "*** " << mName << " Dumping Object Messages ***" << std::endl << std::endl;

        TypeObjMsgMap::iterator it;
        for(it = mMapObjMsg.begin(); it != mMapObjMsg.end(); ++it)
        {
            out << "Id: " << it->first << std::endl;
            dumpObjMsg(out, it->second);
            out << std::endl;
        }
    }
    else
    {
        TypeObjMsgMap::iterator it = mMapObjMsg.find(objId);
        if (it != mMapObjMsg.end())
        {
            out << "Id: " << it->first << std::endl;
            dumpObjMsg(out, it->second);
            out << std::endl;
        }
        else
        {
            out << " OBJECT NOT FOUND for Id" << objId << std::endl;
        }
    }

    out << "*** " << mName << " DONE ***" << std::endl << std::endl;
}

void SimObjMgr::dumpObjMsg(std::ostream& out, google::protobuf::Message* pObjMsg)
{
    std::string data;
    MessageToJsonString(*pObjMsg, &data);
    out << data << std::endl;
}

void SimObjMgr::clearCache(std::ostream& out, uint objId)
{
    if (objId != 0)
    {
        if (clearData(objId))
        {
            out << mName << " Object Data Deleted for Id " << objId << std::endl;
        }
        else
        {
            out << mName << " Object Data Not Found for Id " << objId << std::endl;
        }
    }
    else
    {
        out << mName << " Deleting all object data" << std::endl;
        clearAllData();
    }
}

void SimObjMgr::dumpStateParamNames(std::ostream& out)
{
    std::lock_guard<std::mutex> lock(mMutex);

    dumpParamNames(out);
}

void SimObjMgr::setStateParam(std::ostream& out, uint objId, std::string paramName, std::string valueStr)
{
    if (objId != 0)
    {
        if (setStateParamData(objId, paramName, valueStr))
        {
            out << mName << " Object Data Set for Id " << objId << std::endl;
        }
        else
        {
            out << mName << " Object Data Not Found for Id " << objId << std::endl;
        }
    }
}


} //namespace DpSim

