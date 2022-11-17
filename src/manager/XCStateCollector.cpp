/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "XCStateCollector.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"

#include <boost/bind.hpp>
#include <string>
#include <iostream>


using google::protobuf::util::MessageToJsonString;
using chm6_dataplane::Chm6XCConfig;
using chm6_dataplane::Chm6XCState;

using namespace DpAdapter;


namespace DataPlane {

XCStateCollector::XCStateCollector( XcPbTranslator* pbTrans,
                                    DpAdapter::DcoXconAdapter *pAd)
    : DpStateCollector(1, 1)
    , mXcDataMtx()
    , mpPbTranslator(pbTrans)
    , mpDcoAd(pAd)
{
    assert(pbTrans != NULL);
    assert(pAd  != NULL);

    mCollectStatePollMode = false;

    mMapXcState.clear();
}

void  XCStateCollector::createXcon(const chm6_dataplane::Chm6XCConfig& inCfg)
{
    std::string xcAid = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << xcAid;

    // Take Lock
    std::lock_guard<std::recursive_mutex> lock(mXcDataMtx);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapXcState.find(xcAid) == mMapXcState.end())
    {
        // Create Otu state object
        Chm6XCState* pState = new Chm6XCState();
        pState->mutable_base_state()->mutable_config_id()->set_value(xcAid);
        pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        pState->mutable_hal()->mutable_installed_config()->CopyFrom(inCfg.hal());

        mMapXcState[xcAid] = pState;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Xcon " << xcAid << " state object already created";
    }
}

void XCStateCollector::deleteXcon(std::string xcAid)
{
    INFN_LOG(SeverityLevel::info) << "AID: " << xcAid;

    // Take Lock
    std::lock_guard<std::recursive_mutex> lock(mXcDataMtx);

    if (mMapXcState.find(xcAid) != mMapXcState.end())
    {
        delete mMapXcState[xcAid];
        mMapXcState.erase(xcAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Xcon " << xcAid << " state object does not exist";
    }
}

void XCStateCollector::dumpCacheState(std::string xcAid, std::ostream& os)
{
    os << "******* Xcon State *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::recursive_mutex> lock(mXcDataMtx);

    TypeMapXcState::iterator itXcState;
    for (itXcState = mMapXcState.begin(); itXcState != mMapXcState.end(); ++itXcState)
    {
        if ((xcAid.compare("ALL") == 0) || (xcAid.compare(itXcState->first) == 0))
        {
            string data;
            MessageToJsonString(*itXcState->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void XCStateCollector::listCacheState(std::ostream& out)
{
    lock_guard<std::recursive_mutex> lock(mXcDataMtx);

    for(auto& it : mMapXcState)
    {
        out << it.first << endl;
    }

    out << endl;
}

void XCStateCollector::updateInstalledConfig()
{
    DpAdapter::TypeMapXconConfig mapAdConfig;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    std::lock_guard<std::recursive_mutex> lock(mXcDataMtx);

    mpDcoAd->getXconConfig(mapAdConfig);

    for (auto& it : mapAdConfig)
    {
        std::string strId1 = it.first;
        Chm6XCState* pState = new chm6_dataplane::Chm6XCState();

        pState->mutable_base_state()->mutable_config_id()->set_value(strId1);
        pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mpPbTranslator->configAdToPb(*it.second, *pState);

        mMapXcState[pState->base_state().config_id().value()] = pState;

        string data;
        MessageToJsonString(*pState, &data);
        INFN_LOG(SeverityLevel::info)  << data;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    }
}

void XCStateCollector::updateInstalledConfig(const chm6_dataplane::Chm6XCConfig& inCfg)
{
    std::string xcId = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << xcId;

    // Take Lock
    std::lock_guard<std::recursive_mutex> lock(mXcDataMtx);

    Chm6XCState* pState;

    findOrCreateStateObj(xcId, pState);

    pState->mutable_hal()->mutable_installed_config()->MergeFrom(inCfg.hal());

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
}

int XCStateCollector::getCacheState(std::string stateId, chm6_dataplane::Chm6XCState*& pMsg)
{
    INFN_LOG(SeverityLevel::debug) << "stateId: " << stateId;

    lock_guard<std::recursive_mutex> lock(mXcDataMtx);

    auto it = mMapXcState.find(stateId);

    if (it == mMapXcState.end())
    {
        INFN_LOG(SeverityLevel::info) << "stateId: " << stateId << " NOT FOUND!!";
        return -1;
    }

    pMsg = it->second;

    return 0;
}

void XCStateCollector::findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6XCState*& pState)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapXcState.find(stateId) == mMapXcState.end())
    {
        // Create State
        pState = new chm6_dataplane::Chm6XCState();
        mMapXcState[stateId] = pState;
    }
    else
    {
        pState = mMapXcState[stateId];
    }

    pState->mutable_base_state()->mutable_config_id()->set_value(stateId);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
}

bool XCStateCollector::isCacheExist(std::string configId)
{
    lock_guard<recursive_mutex> lock(mXcDataMtx);

    auto it = mMapXcState.find(configId);

    bool found = (it == mMapXcState.end() ? false : true);

    return (found);
}

} // namespace DataPlane
