/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "GigeClientStateCollector.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"
#include "DpXcMgr.h"
#include "DpGccMgr.h"

#include <boost/bind.hpp>
#include <string>
#include <iostream>

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;
using chm6_dataplane::Chm6GigeClientConfig;
using chm6_dataplane::Chm6GigeClientState;
using chm6_dataplane::Chm6GigeClientFault;
using chm6_dataplane::Chm6GigeClientPm;

using namespace DpAdapter;


namespace DataPlane {

#define MIN_GIGE_ID  1
#define MAX_GIGE_ID  16

#define GIG_STA_COLL_INTVL   1
#define GIG_FLT_COLL_INTVL   1

#define GIG_PM_COLL_INTVL    1

#define GIG_LLDP_COLL_INTVL  10

#define GIG_FLT_BITMAP_DEFAULT 0xFFFFFFFFFFFFFFFF

GigeClientStateCollector::GigeClientStateCollector( GigeClientPbTranslator* pbTrans, DpAdapter::DcoGigeClientAdapter *pAd, gearbox::GearBoxAdIf* pGearBoxAd)
    : DpStateCollector(GIG_STA_COLL_INTVL, GIG_FLT_COLL_INTVL)
    , mCollectPmInterval(GIG_PM_COLL_INTVL)
    , mCollectLldpInterval(GIG_LLDP_COLL_INTVL)
    , mCollectLldpPollMode(true)
    , mGigeDataMtx()
    , mpPbTranslator(pbTrans)
    , mpDcoAd(pAd)
    , mpGearBoxAd(pGearBoxAd)
    , mInstalledGigeClients(0)
{
    assert(pbTrans != NULL);
    assert(pAd  != NULL);

    mCollectStatePollMode = false;

    mMapGigeState.clear();
    mMapGigeFault.clear();
    mMapGigeGearBoxPm.clear();

    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        mCollectStatePollMode = true;
        mCollectFaultPollMode = true;
    }

    createLldpStatePlaceHolder();

    for ( int i = 0; i< MAX_GIGECLIENT_PORT; ++i )
    {
        mLldpRxCnt[i] = 0;
        mLldpTxCnt[i] = 0;
    }
}

void  GigeClientStateCollector::createGigeClient(const chm6_dataplane::Chm6GigeClientConfig& inCfg)
{
    std::string gigeAid = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapGigeState.find(gigeAid) == mMapGigeState.end())
    {
        // Create GigeClient state object
        Chm6GigeClientState* pState = new Chm6GigeClientState();
        pState->mutable_base_state()->mutable_config_id()->set_value(gigeAid);
        pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        pState->mutable_hal()->mutable_installed_config()->CopyFrom(inCfg.hal());

        mMapGigeState[gigeAid] = pState;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "GigeClient " << gigeAid << " state object already created";
    }

    if (mMapGigeFault.find(gigeAid) == mMapGigeFault.end())
    {
        // Create GigeClient fault object
        Chm6GigeClientFault* pFault = new Chm6GigeClientFault();
        pFault->mutable_base_fault()->mutable_config_id()->set_value(gigeAid);
        pFault->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pFault->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mMapGigeFacFaultBitmap[gigeAid] = GIG_FLT_BITMAP_DEFAULT;

        mMapGigeFault[gigeAid] = pFault;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "GigeClient " << gigeAid << " fault object already created";
    }


    if (mMapGigeGearBoxPm.find(gigeAid) == mMapGigeGearBoxPm.end())
    {
        gearbox::BcmPm* pPm = new gearbox::BcmPm();

        mMapGigeGearBoxPm[gigeAid] = pPm;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "GigeClient " << gigeAid << " GearBox pm object already created";
    }
}

void GigeClientStateCollector::deleteGigeClient(std::string gigeAid)
{
    INFN_LOG(SeverityLevel::info) << "AID: " << gigeAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    if (mMapGigeState.find(gigeAid) != mMapGigeState.end())
    {
        uint32 tempSize = mMapGigeState[gigeAid]->mutable_hal()->lldp_pkt_rx_size();

        delete mMapGigeState[gigeAid];
        mMapGigeState.erase(gigeAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "GigeClient " << gigeAid << " state object does not exist";
    }

    if (mMapGigeFault.find(gigeAid) != mMapGigeFault.end())
    {
        delete mMapGigeFault[gigeAid];
        mMapGigeFault.erase(gigeAid);

        mMapGigeFacFaultBitmap.erase(gigeAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "GigeClient " << gigeAid << " fault object does not exist";
    }


    if (mMapGigeGearBoxPm.find(gigeAid) != mMapGigeGearBoxPm.end())
    {
        delete mMapGigeGearBoxPm[gigeAid];
        mMapGigeGearBoxPm.erase(gigeAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "GigeClient " << gigeAid << " GearBox pm object does not exist";
    }

}


void GigeClientStateCollector::start()
{
    mThrFaults = boost::thread(boost::bind(
            &GigeClientStateCollector::collectGigeClientFaults, this
            ));

    mThrFaults.detach();

    mThrLldp = boost::thread(boost::bind(
            &GigeClientStateCollector::collectGigeClientLldp, this
            ));

    mThrLldp.detach();
}

void GigeClientStateCollector::collectGigeClientFaults()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of GigeClient Faults";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mGigeDataMtx);

            if ((true == mCollectFaultPollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateGigeClientFaults();
                }
                else
                {
                    INFN_LOG(SeverityLevel::debug) << "Connection to DCO is down. Skip collection";
                }
            }
        }

        boost::posix_time::seconds workTime(mCollectFaultInterval);

        boost::this_thread::sleep(workTime);
    }

    INFN_LOG(SeverityLevel::info) << "Finish collection of GigeClient Faults";
}


void GigeClientStateCollector::collectGigeClientPm()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of GigeClient Pm";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mGigeDataMtx);

            if (true == mIsCollectionEn)
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateGigeClientPm();
                }
                else
                {
                    INFN_LOG(SeverityLevel::debug) << "Connection to DCO is down. Skip collection";
                }
            }
        }

        boost::posix_time::seconds workTime(mCollectPmInterval);

        boost::this_thread::sleep(workTime);
    }

    INFN_LOG(SeverityLevel::info) << "Finish collection of GigeClient Pm";
}


void GigeClientStateCollector::collectGigeClientLldp()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of GigeClient Lldp";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mGigeDataMtx);

            if (true == mIsCollectionEn)
            {
                if (true == mCollectLldpPollMode)
                {
                    getAndUpdateGigeClientLldp();
                }
            }
        }

        boost::posix_time::seconds workTime(mCollectLldpInterval);

        boost::this_thread::sleep(workTime);
    }

    INFN_LOG(SeverityLevel::info) << "Finish collection of GigeClient Lldp";
}

void GigeClientStateCollector::getAndUpdateGigeClientFaults()
{
    if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
    {
        return;
    }

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    TypeMapGigeClientFault::iterator itGigeFault;
    for (itGigeFault = mMapGigeFault.begin(); itGigeFault != mMapGigeFault.end(); ++itGigeFault)
    {
        std::string aid = itGigeFault->second->base_fault().config_id().value();
        INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " Collecting fault for GigeClient: " << aid;

        itGigeFault->second->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        itGigeFault->second->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        if (false == dp_env::DpEnvSingleton::Instance()->isSimEnv())
        {
            DpAdapter::GigeFault dcoFault;
            if (mpDcoAd->getGigeFault(aid, &dcoFault))
            {
                DpAdapter::GigeFault gigeFault = dcoFault;

                DpAdapter::GigeFault gearBoxFault;
                if (mpGearBoxAd->getBcmFaults(aid, &gearBoxFault) == 0)
                {
                    INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " GigeClient GearBox Fault = 0x" << std::hex << gearBoxFault.facBmp << std::dec;
                    mpPbTranslator->faultAdGearBoxUpdate(gearBoxFault, gigeFault);
                }

                DpAdapter::GigeFault gigeFaultCurrent;
                gigeFaultCurrent.facBmp = (mMapGigeFacFaultBitmap.end() == mMapGigeFacFaultBitmap.find(aid) ? GIG_FLT_BITMAP_DEFAULT : mMapGigeFacFaultBitmap[aid]);

                bool force = (GIG_FLT_BITMAP_DEFAULT == gigeFaultCurrent.facBmp) ? true : false;

                if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
                {
                    INFN_LOG(SeverityLevel::info) << "Block unstable faults for AID: " << aid << " during DCO shutdown";
                    return;
                }

                if (mpPbTranslator->isAdChanged(gigeFaultCurrent, gigeFault) || (true == force))
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << std::hex << " GearBox Fault(" << std::hex << gearBoxFault.facBmp << ")" << std::dec;
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << std::hex << " DCO Fault(" << std::hex << dcoFault.facBmp << ")" << std::dec;
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << std::hex << " FaultBmpCur(0x" << gigeFaultCurrent.facBmp << ") FaultBmpNew(0x" << gigeFault.facBmp << ")" << std::dec;

                    mpPbTranslator->faultAdToPb(aid, gigeFaultCurrent.facBmp, gigeFault, *itGigeFault->second, force);

                    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itGigeFault->second);

                    mMapGigeFacFaultBitmap[aid] = gigeFault.facBmp;
                }
            }
        }
        else
        {
            // Get GigeClient fault
            uint64_t fault;
            if (mpDcoAd->getGigeFault(aid, &fault))
            {
                uint64_t faultCurrent = (mMapGigeFacFaultBitmap.end() == mMapGigeFacFaultBitmap.find(aid) ? GIG_FLT_BITMAP_DEFAULT : mMapGigeFacFaultBitmap[aid]);
                if (faultCurrent != fault)
                {
                    mpPbTranslator->faultAdToPbSim(fault, *itGigeFault->second);

                    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itGigeFault->second);

                    mMapGigeFacFaultBitmap[aid] = fault;
                }
            }
        }
    }
}


void GigeClientStateCollector::getAndUpdateGigeClientPm()
{
    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        return;
    }

    TypeMapGigeGearBoxPm::iterator itGigePm;
    for (itGigePm = mMapGigeGearBoxPm.begin(); itGigePm != mMapGigeGearBoxPm.end(); ++itGigePm)
    {
        std::string aid = itGigePm->first;
        INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " Collecting pm for GigeClient: " << aid;

        gearbox::BcmPm gigePm;
        if (mpGearBoxAd->getPmCounts(aid, gigePm) == 0)
        {
            itGigePm->second->fecMode = gigePm.fecMode;

            if (gigePm.fecMode == gearbox::PCSFEC)
            {
                itGigePm->second->pcs.icgCounts = gigePm.pcs.icgCounts;
                itGigePm->second->pcs.bip8Counts = gigePm.pcs.bip8Counts;

                for (uint i = 0; i < gearbox::cNumPcsLanes; i++)
                {
                    itGigePm->second->pcs.bip8CountsLane[i] = gigePm.pcs.bip8CountsLane[i];
                }
            }
            else if (gigePm.fecMode == gearbox::KR4FEC)
            {
                itGigePm->second->fec.rxUnCorrWord = gigePm.fec.rxUnCorrWord;
                itGigePm->second->fec.rxCorrWord   = gigePm.fec.rxCorrWord;
                itGigePm->second->fec.fecSymbolErr = gigePm.fec.fecSymbolErr;
            }
        }
    }
}

void GigeClientStateCollector::getGigeClientGearBoxPm(std::string gigeAid, gearbox::BcmPm& gigeGearBoxPm)
{

    memset(&gigeGearBoxPm, 0, sizeof(gigeGearBoxPm));
    if (0 != mpGearBoxAd->getPmCounts(gigeAid, gigeGearBoxPm))
    {
        INFN_LOG(SeverityLevel::debug) << "AID: " << gigeAid << " Error retrieving GearBox PM";
    }
}


void GigeClientStateCollector::getAndUpdateGigeClientLldp()
{
    INFN_LOG(SeverityLevel::debug) << "Collecting lldp packets for GigeClient";

    // Retrieve from the LLDP frame pool
    LldpFramePool framePool;
    DpGccMgrSingleton::Instance()->collectLldpFrame(framePool);

    if (0 == framePool.size())
    {
        INFN_LOG(SeverityLevel::debug) << "No lldp packets to process";
        return;
    }

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Initialize Chm6LldpState proto
    std::string chm6Aid = mpPbTranslator->getChassisSlotAid();

    chm6_dataplane::Chm6LldpState lldpState;
    lldpState.mutable_base_state()->mutable_config_id()->set_value(chm6Aid);
    lldpState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    lldpState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    lldpState.mutable_hal()->mutable_lldp_packet()->clear();

    // Fill Chm6LldpState proto with LLDP packets from frame pool
    uint indexKey = 0;
    for (LldpFramePool::iterator it = framePool.begin(); it != framePool.end(); ++it)
    {
        uint16         len;
        uint8          serdesLaneNum;
        LldpDir        direction;
        unsigned char *frame;

        // Retrieve the LLDP packet information
        frame = it->second.GetLldpFrame(len, serdesLaneNum, direction);

        // Convert serdesLane to Gige port id
        uint portId;
        if (false == mpDcoAd->getLldpPortIdFromFlowId(serdesLaneNum, &portId))
        {
            INFN_LOG(SeverityLevel::info) << "Cannot convert serdesLaneNum " << serdesLaneNum << " to portId";
            continue;
        }
        std::string tpAid = "";
        std::string gigeAid = mpPbTranslator->numToAid(portId, tpAid);

        for(auto itr = mMapGigeState.begin(); itr != mMapGigeState.end(); ++itr)
        {
             INFN_LOG(SeverityLevel::debug) << " SERLA mMapGigeState keys: " << itr->first;
        }

        if(mMapGigeState.find(tpAid) != mMapGigeState.end())
        {
            gigeAid = tpAid;
        }
        INFN_LOG(SeverityLevel::debug) << "Processing LLDP frame for AID: " << gigeAid << " len(" << len << ") serdesLane(" << (int)serdesLaneNum << ") direction(" << direction << ")";

        hal_dataplane::LldpPacket lldpPkt;

        lldpPkt.mutable_aid()->set_value(gigeAid);

        if (LLDP_DIR_RX == direction)
        {
            lldpPkt.set_direction(hal_common::DIRECTION_INGRESS);
            mLldpRxCnt[portId-1]++;
        }
        else
        {
            lldpPkt.set_direction(hal_common::DIRECTION_EGRESS);
            mLldpTxCnt[portId-1]++;
        }

        lldpPkt.clear_packet();

        for (uint i = 0; i < len; i++)
        {
            // build key
            uint32 lldpKey = i;

            // build packet data
            hal_common::LldpType_LldpDataType lldpData;
            lldpData.mutable_value()->set_value(frame[i]);

            // insert data
            (*lldpPkt.mutable_packet()->mutable_lldp())[lldpKey] = lldpData;
        }

        (*lldpState.mutable_hal()->mutable_lldp_packet())[indexKey] = lldpPkt;

        indexKey++;
    }

    if (0 != indexKey)
    {
#if 0
        string dataStr;
        MessageToJsonString(lldpState, &dataStr);
        INFN_LOG(SeverityLevel::info) << dataStr;
#endif

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(lldpState);
    }
}

void GigeClientStateCollector::createFaultPmPlaceHolder(std::string gigeAid)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    Chm6GigeClientFault gigeClientFault;
    gigeClientFault.mutable_base_fault()->mutable_config_id()->set_value(gigeAid);
    gigeClientFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    gigeClientFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    string faultDataStr;
    MessageToJsonString(gigeClientFault, &faultDataStr);
    INFN_LOG(SeverityLevel::info) << faultDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(gigeClientFault);

    Chm6GigeClientPm gigeClientPm;
    gigeClientPm.mutable_base_pm()->mutable_config_id()->set_value(gigeAid);
    gigeClientPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    gigeClientPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    DataPlane::DpMsClientGigePmContainer gigeClientAdPm;
    gearbox::BcmPm gigeClientAdGearBoxPm;
    GigeClientPbTranslator pbTranslator;
    pbTranslator.pmAdDefault(gigeClientAdPm, gigeClientAdGearBoxPm);
    pbTranslator.pmAdToPb(gigeClientAdPm, gigeClientAdGearBoxPm, false, false, false, false, gigeClientPm);

    string pmDataStr;
    MessageToJsonString(gigeClientPm, &pmDataStr);
    INFN_LOG(SeverityLevel::info) << pmDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(gigeClientPm);
}

void GigeClientStateCollector::dumpCacheFault(std::string gigeAid, std::ostream& os)
{
    os << "******* GigeClient Fault *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    std::string gigeAidUp(gigeAid);
    std::transform(gigeAidUp.begin(), gigeAidUp.end(), gigeAidUp.begin(), ::toupper);

    TypeMapGigeClientFault::iterator itGigeFault;
    for (itGigeFault = mMapGigeFault.begin(); itGigeFault != mMapGigeFault.end(); ++itGigeFault)
    {
        if ((gigeAidUp.compare("ALL") == 0) || (gigeAid.compare(itGigeFault->first) == 0))
        {
            string data;
            MessageToJsonString(*itGigeFault->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void GigeClientStateCollector::dumpCacheState(std::string gigeAid, std::ostream& os)
{
    os << "******* GigeClient State *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    std::string gigeAidUp(gigeAid);
    std::transform(gigeAidUp.begin(), gigeAidUp.end(), gigeAidUp.begin(), ::toupper);

    TypeMapGigeClientState::iterator itGigeState;
    for (itGigeState = mMapGigeState.begin(); itGigeState != mMapGigeState.end(); ++itGigeState)
    {
        if ((gigeAidUp.compare("ALL") == 0) || (gigeAid.compare(itGigeState->first) == 0))
        {
            std::string data;
            MessageToJsonString(*itGigeState->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void GigeClientStateCollector::listCacheState(std::ostream& out)
{
    lock_guard<std::mutex> lock(mGigeDataMtx);

    for(auto& it : mMapGigeState)
    {
        out << it.first << endl;
    }

    out << endl;
}

void GigeClientStateCollector::dumpCachePm(std::string gigeAid, std::ostream& os)
{
    os << "******* GigeClient GearBox Pm *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    std::string gigeAidUp(gigeAid);
    std::transform(gigeAidUp.begin(), gigeAidUp.end(), gigeAidUp.begin(), ::toupper);

    TypeMapGigeGearBoxPm::iterator itGigePm;
    for (itGigePm = mMapGigeGearBoxPm.begin(); itGigePm != mMapGigeGearBoxPm.end(); ++itGigePm)
    {
        if ((gigeAidUp.compare("ALL") == 0) || (gigeAid.compare(itGigePm->first) == 0))
        {
            os << "FecMode         = " << itGigePm->second->fecMode << std::endl;
            os << "PCS ICG Counts  = " << itGigePm->second->pcs.icgCounts << std::endl;
            os << "PCS BIP8 Counts = " << itGigePm->second->pcs.bip8Counts << std::endl;

            for (uint i = 0; i < gearbox::cNumPcsLanes; i++)
            {
                os << "PCS BIP8[" << std::setw(2) << i+1 << "] = " << itGigePm->second->pcs.bip8CountsLane[i] << std::endl;
            }

            os << "FEC Rx UnCorrected Words = " << itGigePm->second->fec.rxUnCorrWord << std::endl;
            os << "FEC Rx Corrected Words   = " << itGigePm->second->fec.rxCorrWord   << std::endl;
            os << "FEC Symbol Errors        = " << itGigePm->second->fec.fecSymbolErr << std::endl;
        }
    }

}

void GigeClientStateCollector::setLldpPollMode(bool mode, std::ostream& os)
{
    mCollectLldpPollMode = mode;
}

void GigeClientStateCollector::setLldpPollInterval(uint32_t intvl, std::ostream& os)
{
    mCollectLldpInterval = intvl;
}

void GigeClientStateCollector::dumpPollInfo(std::ostream& os)
{
    os << "State Polling : " << (true == mCollectStatePollMode ? "Enabled" : "Disabled") << endl;
    os << "State Interval: " << mCollectStateInterval << endl;

    os << "Fault Polling : " << (true == mCollectFaultPollMode ? "Enabled" : "Disabled") << endl;
    os << "Fault Interval: " << mCollectFaultInterval << endl;

    os << "Lldp Polling  : " << (true == mCollectLldpPollMode ? "Enabled" : "Disabled") << endl;
    os << "Lldp Interval : " << mCollectLldpInterval << endl;

    os << "Collection Enable: " << (true == mIsCollectionEn ? "Enabled" : "Disabled") << endl << endl;




}

void  GigeClientStateCollector::dumpLldpCounters(std::ostream& os)
{
    os << " Lldp Counters " << endl;
    for (int i=0; i<MAX_GIGECLIENT_PORT; ++i)
    {
        os << " Tom Port " << i+1 << "  Rx " << mLldpRxCnt[i] << "  Tx " << mLldpTxCnt[i] << endl;
    }
}

void GigeClientStateCollector::createLldpStatePlaceHolder()
{
    INFN_LOG(SeverityLevel::info) << "";

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Create Chm6LldpState proto
    std::string chm6Aid = mpPbTranslator->getChassisSlotAid();

    chm6_dataplane::Chm6LldpState lldpState;
    lldpState.mutable_base_state()->mutable_config_id()->set_value(chm6Aid);
    lldpState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    lldpState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(lldpState);
}

void GigeClientStateCollector::updateInstalledConfig()
{
    DpAdapter::TypeMapGigeConfig mapAdConfig;

    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    mpDcoAd->getGigeConfig(mapAdConfig);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    bool isFecEnable;

    for (auto& it : mapAdConfig)
    {
        Chm6GigeClientState* pState;

        findOrCreateStateObj(it.first, pState);

        mpGearBoxAd->getFecEnable(it.first, isFecEnable);

        mpPbTranslator->configAdToPb(*it.second, *pState, isFecEnable);

        string data;
        MessageToJsonString(*pState, &data);
        INFN_LOG(SeverityLevel::info)  << data;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

        // Cache installed clients for Tom serdes tuning
        int gigeId = mpDcoAd->aidToPortId(it.first);
        if ((0 < gigeId) && (MAX_CLIENTS >= gigeId))
        {
            mInstalledGigeClients |= 0x1 << (gigeId-1);
        }
        INFN_LOG(SeverityLevel::info) << "DCO installed Gige clients = 0x" << std::hex << mInstalledGigeClients << std::dec;
    }
}

void GigeClientStateCollector::updateInstalledConfig(const chm6_dataplane::Chm6GigeClientConfig& inCfg)
{
    std::string gigeId = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << gigeId;

    // Take Lock
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    Chm6GigeClientState* pState;

    findOrCreateStateObj(gigeId, pState);

    pState->mutable_hal()->mutable_installed_config()->MergeFrom(inCfg.hal());

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
}

void GigeClientStateCollector::findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6GigeClientState*& pState)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapGigeState.find(stateId) == mMapGigeState.end())
    {
        // Create State
        pState = new chm6_dataplane::Chm6GigeClientState();
        mMapGigeState[stateId] = pState;
    }
    else
    {
        pState = mMapGigeState[stateId];
    }

    pState->mutable_base_state()->mutable_config_id()->set_value(stateId);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
}

int GigeClientStateCollector::getCacheState(std::string stateId, chm6_dataplane::Chm6GigeClientState*& pMsg)
{
    INFN_LOG(SeverityLevel::debug) << "configId: " << stateId;

    lock_guard<std::mutex> lock(mGigeDataMtx);

    auto it = mMapGigeState.find(stateId);

    if (it == mMapGigeState.end())
    {
        INFN_LOG(SeverityLevel::info) << "stateId: " << stateId << " NOT FOUND!!";
        return -1;
    }

    pMsg = it->second;

    return 0;
}

void GigeClientStateCollector::setIsConnectionUp(bool val)
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    if (val != mIsConnectionUp)
    {
        mIsConnectionUp = val;

        if (!mIsConnectionUp)
        {
            INFN_LOG(SeverityLevel::info) << "Connection Down. Set PM validity flag to false";

            TypeMapGigeGearBoxPm::iterator itGigePm;
            for (itGigePm = mMapGigeGearBoxPm.begin(); itGigePm != mMapGigeGearBoxPm.end(); ++itGigePm)
            {
                std::string aid = itGigePm->first;
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Setting PM invalid for GigeClient";

                DataPlane::DpMsClientGigePmContainer gigeClientAdPm;
                gearbox::BcmPm gigeClientAdGearBoxPm;

                Chm6GigeClientPm gigeClientPbPm;
                gigeClientPbPm.mutable_base_pm()->mutable_config_id()->set_value(aid);

                GigeClientPbTranslator pbTranslator;
                pbTranslator.pmAdDefault(gigeClientAdPm, gigeClientAdGearBoxPm);
                pbTranslator.pmAdToPb(gigeClientAdPm, gigeClientAdGearBoxPm, false, false, false, false, gigeClientPbPm);
                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(gigeClientPbPm);

            }
        }
    }
}

bool GigeClientStateCollector::checkBootUpTraffic()
{
    std::lock_guard<std::mutex> lock(mGigeDataMtx);

    bool traffic_up = true;

    for (TypeMapGigeClientFault::iterator itGigeFault = mMapGigeFault.begin(); itGigeFault != mMapGigeFault.end(); ++itGigeFault)
    {
        std::string aid = itGigeFault->second->base_fault().config_id().value();

        if (mMapGigeFacFaultBitmap.find(aid) == mMapGigeFacFaultBitmap.end())
        {
            continue;
        }

        // evaluate only client port with xcon
        bool isXconPresent = DpXcMgrSingleton::Instance()->isXconPresent(aid, OBJ_MGR_GIGE);
        if ((true == isXconPresent) && (0 != mMapGigeFacFaultBitmap[aid]))
        {
            traffic_up = false;
            break;
        }
    }

    return traffic_up;
}

} // namespace DataPlane
