/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DpConfigHandler.h"
#include "InfnLogger.h"

namespace DataPlane
{

using namespace std;

void DpConfigHandler::syncConfig()
{
    INFN_LOG(SeverityLevel::info) << "";

    lock_guard<recursive_mutex> lock(mMutex);

    auto it = mMapCfgMsg.begin();
    while (it != mMapCfgMsg.end())
    {
        if (isMarkForDelete(*it->second))
        {
            INFN_LOG(SeverityLevel::info) << "Mark for delete detected in cache. Processing Delete for configId: " << it->first;

            STATUS stat = sendDelete(*it->second);
            if (stat != STATUS::STATUS_SUCCESS)
            {
                INFN_LOG(SeverityLevel::error) << "Delete Config Error for configId: " << it->first;
            }

            delete it->second;

            it = mMapCfgMsg.erase(it);
        }
        else
        {
            STATUS stat = sendConfig(*it->second);
            if (stat != STATUS::STATUS_SUCCESS)
            {
                INFN_LOG(SeverityLevel::error) << "Config Error for configId: " << it->first;

                // what todo on failure? retry?? report?
            }
            it++;
        }

    }
}

void DpConfigHandler::clearCache()
{
    INFN_LOG(SeverityLevel::info) << "";

    lock_guard<recursive_mutex> lock(mMutex);

    for(auto& it : mMapCfgMsg)
    {
        delete it.second;
    }

    mMapCfgMsg.clear();
}

void DpConfigHandler::clearCache(string configId)
{
    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    auto it = mMapCfgMsg.find(configId);

    if (it == mMapCfgMsg.end())
    {
        INFN_LOG(SeverityLevel::error)<< "configId " << configId << " NOT found!!";
        return;
    }

    delete it->second;

    mMapCfgMsg.erase(it);
}


bool DpConfigHandler::isCacheExist(string configId)
{
    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    auto it = mMapCfgMsg.find(configId);

    bool found = (it == mMapCfgMsg.end() ? false : true);

    return (found);
}


void DpConfigHandler::dumpCacheObj(std::ostream& out, std::string configId)
{
    lock_guard<recursive_mutex> lock(mMutex);

    auto it = mMapCfgMsg.find(configId);

    if (it == mMapCfgMsg.end())
    {
        out << "configId " << configId << " NOT found!!" << endl << endl;
        return;
    }

    INFN_LOG(SeverityLevel::debug) << "*** Dumping Cache for " << configId;
    dumpCacheMsg(out, *it->second);
}

void DpConfigHandler::dumpCache(std::ostream& out)
{
   // out << "*** Dumping Obj Config Cache All Items *** " << endl << endl;

    lock_guard<recursive_mutex> lock(mMutex);

    for(auto& it : mMapCfgMsg)
    {
        INFN_LOG(SeverityLevel::debug) << "** Dumping Cache for " << it.first;
        dumpCacheMsg(out, *it.second);
    }
}

void DpConfigHandler::listCache(std::ostream& out)
{
   // out << "*** Listing All Obj Config Cache Items ****** " << endl << endl;

    lock_guard<recursive_mutex> lock(mMutex);

    for(auto& it : mMapCfgMsg)
    {
        out << it.first << endl;
    }

    out << endl;
}

void DpConfigHandler::addCacheObj(std::string configId, google::protobuf::Message* pMsg)
{
    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    auto it = mMapCfgMsg.find(configId);

    if (it == mMapCfgMsg.end())
    {
        mMapCfgMsg[configId] = pMsg;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "configId: " << configId << " already exists in cache. Replacing old with new";

        delete it->second;

        it->second = pMsg;
    }
}

int DpConfigHandler::getCacheObj(std::string configId, google::protobuf::Message*& pMsg)
{
    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    auto it = mMapCfgMsg.find(configId);

    if (it == mMapCfgMsg.end())
    {
        INFN_LOG(SeverityLevel::debug) << "configId: " << configId << " NOT FOUND!!";
        return -1;
    }

    pMsg = it->second;

    return 0;
}

int DpConfigHandler::getCacheObjContainsId(std::string configId, google::protobuf::Message*& pMsg)
{
    //INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    for (auto& it : mMapCfgMsg)
    {
        if (it.first.find(configId) != std::string::npos)
        {
            pMsg = it.second;
            return 0;
        }
    }

    return -1;
}

std::string statToString(ConfigStatus stat)
{
    switch(stat)
    {
        case ConfigStatus::SUCCESS:
            return "SUCCESS";
            break;
        case ConfigStatus::FAIL_DATA:
            return "FAIL_DATA";
            break;
        case ConfigStatus::FAIL_CONNECT:
            return "FAIL_CONNECT";
            break;
        case ConfigStatus::FAIL_OTHER:
            return "FAIL_OTHER";
            break;
        default:
            return "UNKNOWN";
            break;
    }
}

} // namespace DataPlane



