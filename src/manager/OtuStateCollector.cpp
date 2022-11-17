/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "OtuStateCollector.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DpXcMgr.h"
#include "dp_common.h"

#include <boost/bind.hpp>
#include <string>
#include <iostream>

#include <InfnTracer/InfnTracer.h>

using google::protobuf::util::MessageToJsonString;
using chm6_dataplane::Chm6OtuConfig;
using chm6_dataplane::Chm6OtuState;
using chm6_dataplane::Chm6OtuFault;
using chm6_dataplane::Chm6OtuPm;

using namespace DpAdapter;


namespace DataPlane {

#define OTU_STA_COLL_INTVL 5
#define OTU_FLT_COLL_INTVL 1

#define OTU_FLT_BITMAP_DEFAULT 0xFFFFFFFFFFFFFFFF

OtuStateCollector::OtuStateCollector( OtuPbTranslator* pbTrans, DpAdapter::DcoOtuAdapter *pAd, shared_ptr<FpgaSramIf> fpgaSramIf)
    : DpStateCollector(OTU_STA_COLL_INTVL, OTU_FLT_COLL_INTVL)
    , mOtuDataMtx()
    , mpPbTranslator(pbTrans)
    , mspFpgaSramIf(fpgaSramIf)
    , mpDcoAd(pAd)
    , mTimAlarmMode(true)
    , mSdAlarmMode(true)
    , mInstalledOtuClients(0)
{
    assert(pbTrans != NULL);
    assert(pAd  != NULL);

    mMapOtuState.clear();
    mMapOtuFault.clear();
    mMapOtuPm.clear();

    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        mCollectStatePollMode = true;
        mCollectFaultPollMode = true;
    }
}

void  OtuStateCollector::createOtu(const chm6_dataplane::Chm6OtuConfig& inCfg)
{
    std::string otuAid = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapOtuState.find(otuAid) == mMapOtuState.end())
    {
        // Create Otu state object
        Chm6OtuState* pState = new Chm6OtuState();
        pState->mutable_base_state()->mutable_config_id()->set_value(otuAid);
        pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        pState->mutable_hal()->mutable_installed_config()->CopyFrom(inCfg.hal());

        mMapOtuState[otuAid] = pState;

        TRACE_STATE(Chm6OtuState, *pState, TO_REDIS(),
        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
	)
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " state object already created";
    }

    if (mMapOtuFault.find(otuAid) == mMapOtuFault.end())
    {
        // Create Otu fault object
        Chm6OtuFault* pFault = new Chm6OtuFault();
        pFault->mutable_base_fault()->mutable_config_id()->set_value(otuAid);
        pFault->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pFault->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mMapOtuFacFaultBitmap[otuAid]  = OTU_FLT_BITMAP_DEFAULT;

        mMapOtuFault[otuAid] = pFault;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " fault object already created";
    }

    if (mMapOtuPm.find(otuAid) == mMapOtuPm.end())
    {
        // Create Otu pm object
        Chm6OtuPm* pPm = new Chm6OtuPm();
        pPm->mutable_base_pm()->mutable_config_id()->set_value(otuAid);
        pPm->mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pPm->mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mMapOtuPm[otuAid] = pPm;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " pm object already created";
    }

    if (mMapOtuConfigCache.find(otuAid) == mMapOtuConfigCache.end())
    {
        // Create Otu config attribute cache
        createConfigCache(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " config attribute cache already created";
    }

    // Initialize Otu signal degrade tracking
    if (mMapOtuSdTracking.find(otuAid) == mMapOtuSdTracking.end())
    {
        // Create Otu signal degrade tracking
        createSdTracking(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " signal degrade tracking created";
    }

    if (mMapOtuTimTracking.find(otuAid) == mMapOtuTimTracking.end())
    {
        mMapOtuTimTracking[otuAid] = false;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " tim tracking created";
    }
}

void OtuStateCollector::deleteOtu(std::string otuAid)
{
    INFN_LOG(SeverityLevel::info) << "AID: " << otuAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    if (mMapOtuState.find(otuAid) != mMapOtuState.end())
    {
        delete mMapOtuState[otuAid];
        mMapOtuState.erase(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " state object does not exist";
    }

    if (mMapOtuFault.find(otuAid) != mMapOtuFault.end())
    {
        delete mMapOtuFault[otuAid];
        mMapOtuFault.erase(otuAid);

        mMapOtuFacFaultBitmap.erase(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " fault object does not exist";
    }

    if (mMapOtuPm.find(otuAid) != mMapOtuPm.end())
    {
        delete mMapOtuPm[otuAid];
        mMapOtuPm.erase(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " pm object does not exist";
    }

    if (mMapOtuConfigCache.find(otuAid) != mMapOtuConfigCache.end())
    {
        delete mMapOtuConfigCache[otuAid];
        mMapOtuConfigCache.erase(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " config attribute cache does not exist";
    }

    if (mMapOtuSdTracking.find(otuAid) != mMapOtuSdTracking.end())
    {
        updateOtuSdScratchpad(otuAid, false);
        delete mMapOtuSdTracking[otuAid];
        mMapOtuSdTracking.erase(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " signal degrade tracking does not exist";
    }

    if (mMapOtuTimTracking.find(otuAid) != mMapOtuTimTracking.end())
    {
        mMapOtuTimTracking.erase(otuAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " tim tracking does not exist";
    }
}

void OtuStateCollector::start()
{
    mThrFaults = boost::thread(boost::bind(
            &OtuStateCollector::collectOtuFaults, this
            ));

    mThrFaults.detach();

    mThrState = boost::thread(boost::bind(
            &OtuStateCollector::collectOtuStatus, this
            ));

    mThrState.detach();
}

void OtuStateCollector::collectOtuStatus()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of Otu State";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mOtuDataMtx);

            if ((true == mCollectStatePollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateOtuStatus();
                }
                else
                {
                    INFN_LOG(SeverityLevel::debug) << "Connection to DCO is down. Skip collection";
                }
            }
        }

        boost::posix_time::seconds workTime(mCollectStateInterval);

        boost::this_thread::sleep(workTime);
    }

    INFN_LOG(SeverityLevel::info) << "Finish collection of Otu State";
}

void OtuStateCollector::collectOtuFaults()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of Otu Faults";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mOtuDataMtx);

            if ((true == mCollectFaultPollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateOtuFaults();
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

    INFN_LOG(SeverityLevel::info) << "Finish collection of Otu Faults";
}

void OtuStateCollector::getAndUpdateOtuStatus()
{
    //INFN_LOG(SeverityLevel::info) << "Get Otu state data");

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    TypeMapOtuState::iterator itOtuState;
    for (itOtuState = mMapOtuState.begin(); itOtuState != mMapOtuState.end(); ++itOtuState)
    {
        std::string aid = itOtuState->second->base_state().config_id().value();
        INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " Collecting status for Otu: " << aid;

        chm6_dataplane::Chm6OtuState prevOtuState(*itOtuState->second);

        // Get Otu state
        DpAdapter::OtuStatus adOtuStat;

        uint8_t* rxTtiPtr = mpDcoAd->getOtuRxTti(aid);
        if (NULL == rxTtiPtr)
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Failed DCO Adapter get RxTTI";
            continue;
        }

        // In presence of hard fault, clear the TTI
        DpAdapter::OtuFault otuFault;
        if (mpDcoAd->getOtuFault(aid, &otuFault))
        {
            bool isHardFault = mpPbTranslator->isOtuHardFault(otuFault, FAULTDIRECTION_RX);
            if (true == isHardFault)
            {
                rxTtiPtr = NULL;
            }
        }

        mpPbTranslator->statusAdToPb(rxTtiPtr, *itOtuState->second);

        if (mpPbTranslator->isPbChanged(prevOtuState, *itOtuState->second))
        {
            INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " Otu State update";
            string stateDataStr;
            MessageToJsonString(*itOtuState->second, &stateDataStr);
            INFN_LOG(SeverityLevel::debug) << stateDataStr;

            TRACE_STATE(Chm6OtuState, *itOtuState->second, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itOtuState->second);
	    )
        }
    }
}

void OtuStateCollector::getAndUpdateOtuFaults()
{
    if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
    {
        return;
    }

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    TypeMapOtuFault::iterator itOtuFault;
    for (itOtuFault = mMapOtuFault.begin(); itOtuFault != mMapOtuFault.end(); ++itOtuFault)
    {
        std::string aid = itOtuFault->second->base_fault().config_id().value();
        INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " Collecting fault for Otu: " << aid;

        itOtuFault->second->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        itOtuFault->second->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        if (false == dp_env::DpEnvSingleton::Instance()->isSimEnv())
        {
            DpAdapter::OtuFault otuFault;
            if (mpDcoAd->getOtuFault(aid, &otuFault))
            {
                DpAdapter::OtuFault otuFaultCurrent;
                otuFaultCurrent.facBmp = (mMapOtuFacFaultBitmap.end() == mMapOtuFacFaultBitmap.find(aid) ? OTU_FLT_BITMAP_DEFAULT : mMapOtuFacFaultBitmap[aid]);

                bool isRxHardFault = mpPbTranslator->isOtuHardFault(otuFault, FAULTDIRECTION_RX);
                bool isIaeFault = mpPbTranslator->isOtuIaeFault(otuFault, FAULTDIRECTION_RX);

                if (true == mTimAlarmMode)
                {
                    bool timFault = evaluateTimFault(aid, isRxHardFault);

                    INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " TIM Fault = " << timFault;

                    // We inject OTU BDI on TIM alarm
                    if (mpDcoAd->setOtuTimBdi(aid, timFault) == false)
                    {
                        INFN_LOG(SeverityLevel::error) << "AID: " << aid << " Failed to set/clear Otu BDI: " << aid;
                    }

                    mpPbTranslator->addOtuTimFault(timFault, otuFault);
                }

                if (true == mSdAlarmMode)
                {
                    bool sdFault = evaluateSdFault(aid, (isRxHardFault || isIaeFault));

                    updateOtuSdScratchpad(aid, sdFault);

                    mpPbTranslator->addOtuSdFault(sdFault, otuFault);
                }

                bool force = (OTU_FLT_BITMAP_DEFAULT == otuFaultCurrent.facBmp) ? true : false;

                if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
                {
                    INFN_LOG(SeverityLevel::info) << "Block unstable faults for AID: " << aid << " during DCO shutdown";
                    return;
                }

                if ((mpPbTranslator->isAdChanged(otuFaultCurrent, otuFault)) || (true == force))
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << std::hex << " FaultBmpCur(0x" << otuFaultCurrent.facBmp << ") FaultBmpNew(0x" << otuFault.facBmp << ")" << std::dec;

                    mpPbTranslator->faultAdToPb(aid, otuFaultCurrent.facBmp, otuFault, *itOtuFault->second, force);

                    TRACE_FAULT(Chm6OtuFault, *itOtuFault->second, TO_REDIS(),
                    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itOtuFault->second);
		    )

                    mMapOtuFacFaultBitmap[aid] = otuFault.facBmp;
                }
            }
        }
        else
        {
            // Get Otu fault
            uint64_t faultRx;
            if (mpDcoAd->getOtuFaultRx(aid, &faultRx))
            {
                uint64_t faultCurrent = (mMapOtuFacFaultBitmap.end() == mMapOtuFacFaultBitmap.find(aid) ? OTU_FLT_BITMAP_DEFAULT : mMapOtuFacFaultBitmap[aid]);
                if (faultCurrent != faultRx)
                {
                    LOG_FLT() << "AID: " << aid << " Otu Fault update: Current(0x" << std::hex << faultCurrent << std::dec << ") New(0x" << std::hex << faultRx << std::dec << ")";

                    mpPbTranslator->faultAdToPbSim(faultRx, *itOtuFault->second);

                    TRACE_FAULT(Chm6OtuFault, *itOtuFault->second, TO_REDIS(),
                    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itOtuFault->second);
		    )

                    mMapOtuFacFaultBitmap[aid] = faultRx;
                }
            }

            uint64_t faultTx;
            if (mpDcoAd->getOtuFaultTx(aid, &faultTx))
            {
                //
            }
        }
    }
}

void OtuStateCollector::createConfigCache(const std::string otuAid)
{
    INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " Create Otu State Collector config cache for TTI & SD parameters";

    OtuConfigCache* pCfgCache = new OtuConfigCache;

    // TIM TTI
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        pCfgCache->ttiExpected[index] = 0;
    }
    pCfgCache->ttiMismatchType = hal_common::TTI_MISMATCH_TYPE_DISABLED;

    // Signal Degrade
    pCfgCache->sdThreshold = 0;
    pCfgCache->sdInterval = 0;

    mMapOtuConfigCache[otuAid] = pCfgCache;
}

void OtuStateCollector::updateConfigCache(const std::string otuAid, const chm6_dataplane::Chm6OtuConfig* pCfg)
{
    INFN_LOG(SeverityLevel::info) << "Otu " << otuAid << " Update Otu State Collector config cache for TTI & SD parameters";

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    if (mMapOtuConfigCache.find(otuAid) == mMapOtuConfigCache.end())
    {
        INFN_LOG(SeverityLevel::error) << "Otu " << otuAid << " config attribute cache does not exist";
        return;
    }

    if (pCfg->has_hal() == false)
    {
        INFN_LOG(SeverityLevel::error) << "Otu " << otuAid << " - NO hal()";
        return;
    }

    if (pCfg->hal().has_alarm_threshold() == false)
    {
        INFN_LOG(SeverityLevel::error) << "Otu " << otuAid << " - NO hal().alarm_threshold()";
        return;
    }

    // TIM TTI Expected
    if (0 < pCfg->hal().alarm_threshold().tti_expected().tti_size())
    {
        for (google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>::const_iterator
             it  = pCfg->hal().alarm_threshold().tti_expected().tti().begin();
             it != pCfg->hal().alarm_threshold().tti_expected().tti().end(); ++it)
        {
            uint index = it->first;
            if (MAX_TTI_LENGTH > index)
            {
                mMapOtuConfigCache[otuAid]->ttiExpected[index] = it->second.value().value();
            }
        }
    }

    // TIM TTI Mismatch Type
    if (hal_common::TTI_MISMATCH_TYPE_UNSPECIFIED != pCfg->hal().alarm_threshold().tti_mismatch_type())
    {
        mMapOtuConfigCache[otuAid]->ttiMismatchType = pCfg->hal().alarm_threshold().tti_mismatch_type();
    }

    // Signal Degrade Threshold
    if (pCfg->hal().alarm_threshold().has_signal_degrade_threshold())
    {
        mMapOtuConfigCache[otuAid]->sdThreshold = pCfg->hal().alarm_threshold().signal_degrade_threshold().value();
    }

    // Signal Degrade Interval
    if (pCfg->hal().alarm_threshold().has_signal_degrade_interval())
    {
        mMapOtuConfigCache[otuAid]->sdInterval = pCfg->hal().alarm_threshold().signal_degrade_interval().value();
    }
}

bool OtuStateCollector::evaluateTimFault(std::string otuAid, bool isHardFault)
{
    uint indexBeginA, indexEndA;
    uint indexBeginB, indexEndB;

    if (mMapOtuConfigCache.find(otuAid) == mMapOtuConfigCache.end())
    {
        INFN_LOG(SeverityLevel::error) << "Otu " << otuAid << " config attribute cache does not exist";
        mMapOtuTimTracking[otuAid] = false;
        return false;
    }

    if (true == isHardFault)
    {
        mMapOtuTimTracking[otuAid] = false;
        return false;
    }

    // Get Otu state
    uint8_t* rxTtiPtr;
    rxTtiPtr = mpDcoAd->getOtuRxTti(otuAid);
    if (NULL == rxTtiPtr)
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " Failed DCO Adapter get RxTTI";
        mMapOtuTimTracking[otuAid] = false;
        return false;
    }

    switch (mMapOtuConfigCache[otuAid]->ttiMismatchType)
    {
        case hal_common::TTI_MISMATCH_TYPE_FULL_64BYTES:
        case hal_common::TTI_MISMATCH_TYPE_SAPI_DAPI_OPER:
             indexBeginA = 0;  indexEndA = 63;
             indexBeginB = 0;  indexEndB = 0;
             break;
        case hal_common::TTI_MISMATCH_TYPE_SAPI:
             indexBeginA = 0;  indexEndA = 15;
             indexBeginB = 0;  indexEndB = 0;
             break;
        case hal_common::TTI_MISMATCH_TYPE_DAPI:
             indexBeginA = 16; indexEndA = 31;
             indexBeginB = 0;  indexEndB = 0;
             break;
        case hal_common::TTI_MISMATCH_TYPE_OPER:
             indexBeginA = 32; indexEndA = 63;
             indexBeginB = 0;  indexEndB = 0;
             break;
        case hal_common::TTI_MISMATCH_TYPE_SAPI_DAPI:
             indexBeginA = 0;  indexEndA = 31;
             indexBeginB = 0;  indexEndB = 0;
             break;
        case hal_common::TTI_MISMATCH_TYPE_SAPI_OPER:
             indexBeginA = 0;  indexEndA = 15;
             indexBeginB = 32; indexEndB = 63;
             break;
        case hal_common::TTI_MISMATCH_TYPE_DAPI_OPER:
             indexBeginA = 16; indexEndA = 63;
             indexBeginB = 0;  indexEndB = 0;
             break;
        default:
             INFN_LOG(SeverityLevel::debug) << "Otu " << otuAid << "Disabled or Unknown TTI match type - CLEAR";
             mMapOtuTimTracking[otuAid] = false;
             return false;
    }

    for (uint index = indexBeginA; index <= indexEndA; index++)
    {
        if (mMapOtuConfigCache[otuAid]->ttiExpected[index] != rxTtiPtr[index])
        {
            INFN_LOG(SeverityLevel::debug) << "Otu " << otuAid << "indexBeginA(" << indexBeginA << ") indexEndA(" << indexEndA << ") index(" << index << ") - RAISE";
            mMapOtuTimTracking[otuAid] = true;
            return true;
        }
    }

    if ((0 == indexBeginB) && (0 == indexEndB))
    {
        // Done with check
        INFN_LOG(SeverityLevel::debug) << "Otu " << otuAid << "Expected TTI match Received TTI - CLEAR";
        mMapOtuTimTracking[otuAid] = false;
        return false;
    }

    for (uint index = indexBeginB; index <= indexEndB; index++)
    {
        if (mMapOtuConfigCache[otuAid]->ttiExpected[index] != rxTtiPtr[index])
        {
            INFN_LOG(SeverityLevel::debug) << "Otu " << otuAid << "indexBeginB(" << indexBeginB << ") indexEndB(" << indexEndB << ") index(" << index << ") - RAISE";
            mMapOtuTimTracking[otuAid] = true;
            return true;
        }
    }

    INFN_LOG(SeverityLevel::debug) << "Otu " << otuAid << "Expected TTI match Received TTI - CLEAR";
    mMapOtuTimTracking[otuAid] = false;
    return false;
}

bool OtuStateCollector::isTimFault(std::string otuAid)
{
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    if (mMapOtuTimTracking.find(otuAid) == mMapOtuTimTracking.end())
    {
        // Turn off log since Xpress XCON creates internal OTU4
        // Clients would cause rolling logs.
        INFN_LOG(SeverityLevel::debug) << "Otu " << otuAid << " tim tracking does not exist";
        return false;
    }

    return (mMapOtuTimTracking[otuAid]);
}

void OtuStateCollector::createSdTracking(const std::string otuAid)
{
    // Create Otu signal degrade tracking
    OtuSdTracking* pSdTracking = new OtuSdTracking;
    pSdTracking->isSdFaulted = false;
    pSdTracking->sdRaiseSeconds = 0;
    pSdTracking->sdClearSeconds = 0;

    mMapOtuSdTracking[otuAid] = pSdTracking;
}

bool OtuStateCollector::evaluateSdFault(std::string otuAid, bool isHardFault)
{
    bool fault = false;

    if (mMapOtuConfigCache.find(otuAid) == mMapOtuConfigCache.end())
    {
        INFN_LOG(SeverityLevel::error) << "Otu " << otuAid << " config attribute cache does not exist";
        return false;
    }

    if (mMapOtuSdTracking.find(otuAid) == mMapOtuSdTracking.end())
    {
        INFN_LOG(SeverityLevel::error) << "Otu " << otuAid << " signal degrade tracking does not exist";
        return false;
    }

    uint sdThreshold = mMapOtuConfigCache[otuAid]->sdThreshold;
    uint sdInterval  = mMapOtuConfigCache[otuAid]->sdInterval;

    bool &isSdFaulted    = mMapOtuSdTracking[otuAid]->isSdFaulted;
    uint &sdRaiseSeconds = mMapOtuSdTracking[otuAid]->sdRaiseSeconds;
    uint &sdClearSeconds = mMapOtuSdTracking[otuAid]->sdClearSeconds;

    // Do not evaluate bogus error blocks on hard fault
    if (true == isHardFault)
    {
        INFN_LOG(SeverityLevel::debug) << "AID: " << otuAid << " Ignore error blocks pm on hard fault";
        sdClearSeconds = 0;
        sdRaiseSeconds = 0;
        isSdFaulted = false;
        return false;
    }

    // Convert theshold to allowewEB
    uint32_t frameRate = mpDcoAd->getOtuFrameRate(otuAid);

    uint64_t allowedEb = ((uint64_t)frameRate * sdThreshold) / 100 ;

    INFN_LOG(SeverityLevel::debug) << "AID: " << otuAid << " allowedEb = " << (uint64_t)allowedEb;

    if (0 >= allowedEb)
    {
        INFN_LOG(SeverityLevel::debug) << "AID: " << otuAid << " Allowed Error Block " << allowedEb << " is invalid";
        return false;
    }

    uint64_t errorBlocks = mpDcoAd->getOtuPmErrorBlocks(otuAid);

    INFN_LOG(SeverityLevel::debug) << "AID: " << otuAid << " errorBlocks = " << (uint64_t)errorBlocks;

    if (errorBlocks >= allowedEb)
    {
        sdClearSeconds = 0;
        //INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " errorBlocks " << errorBlocks << " EXCEED allowedEb";

        // If configured threshold is reset to 0, clear fault
        if (0 >= sdInterval)
        {
            sdRaiseSeconds = 0;
            return false;
        }

        // Raise fault after signal degrade hits configured threshold
        if ((true == isSdFaulted) || (sdRaiseSeconds >= sdInterval))
        {
            fault = true;
            //INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " sdRaiseSeconds = " << sdRaiseSeconds << " - RAISE signal degrade fault";
        }
        else
        {
            sdRaiseSeconds++;
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " errorBlocks(" << errorBlocks  << ") allowedBlocks(" << allowedEb << ") raiseSec(" << sdRaiseSeconds << ")";
        }
    }
    else
    {
        sdRaiseSeconds = 0;
        //INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " errorBlocks " << errorBlocks << " LESS THAN allowedEb";

        if ((true == isSdFaulted) && (sdClearSeconds < sdInterval))
        {
            fault = true;
            sdClearSeconds++;
            INFN_LOG(SeverityLevel::info) << "AID: " << otuAid << " errorBlocks(" << errorBlocks  << ") allowedBlocks(" << allowedEb << ") clearSec(" << sdClearSeconds << ")";
        }
    }

    isSdFaulted = fault;

    return fault;
}

bool OtuStateCollector::getOtuSdScratchpad(std::string otuAid)
{
    if (nullptr == mspFpgaSramIf)
    {
        return false;
    }

    int otuId = mpDcoAd->aidToOtuId(otuAid);

    uint32 offset;
    if (false == isValidOtuId(otuId))
    {
        return false;
    }
    else if (true == isClientOtuId(otuId))
    {
        offset = cFpgaSram_Otu4SdRxRegOffset;
    }
    else
    {
        otuId = otuId - HO_OTU_ID_OFFSET;
        offset = cFpgaSram_HoOtuSdRxRegOffset;
    }

    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (otuId - 1))) == 0 ? false : true;

    return scratchFault;
}

void OtuStateCollector::updateOtuSdScratchpad(std::string otuAid, bool sdFault)
{
    if (nullptr == mspFpgaSramIf)
    {
        return;
    }

    int otuId = mpDcoAd->aidToOtuId(otuAid);

    uint32 offset;
    if (false == isValidOtuId(otuId))
    {
        return;
    }
    else if (true == isClientOtuId(otuId))
    {
        offset = cFpgaSram_Otu4SdRxRegOffset;
    }
    else
    {
        otuId = otuId - HO_OTU_ID_OFFSET;
        offset = cFpgaSram_HoOtuSdRxRegOffset;
    }

    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (otuId - 1))) == 0 ? false : true;

    if (scratchFault != sdFault)
    {
        INFN_LOG(SeverityLevel::info) << " Updating OTU SD INGRESS Scratchpad(0x" << std::hex << offset << ") = 0x" << regVal << std::dec << " for AID: " << otuAid;

        if (sdFault)
        {
            regVal |= (0x1 << (otuId - 1));
            INFN_LOG(SeverityLevel::info) << " OTU SD INGRESS Scratchpad set for AID: " << otuAid;
        }
        else
        {
            regVal &= ~(0x1 << (otuId - 1));
            INFN_LOG(SeverityLevel::info) << " OTU SD INGRESS Scratchpad clear for AID: " << otuAid;
        }
        mspFpgaSramIf->Write32(offset, regVal);

        regVal = mspFpgaSramIf->Read32(offset);
        INFN_LOG(SeverityLevel::info) << " New OTU SD INGRESS Scratchpad(0x" << std::hex << offset << ") = 0x" << regVal << std::dec << " for AID: " << otuAid;
    }
}

void OtuStateCollector::createFaultPmPlaceHolder(std::string otuAid)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    Chm6OtuFault otuFault;
    otuFault.mutable_base_fault()->mutable_config_id()->set_value(otuAid);
    otuFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    otuFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    string faultDataStr;
    MessageToJsonString(otuFault, &faultDataStr);
    INFN_LOG(SeverityLevel::info) << faultDataStr;

    TRACE_FAULT(Chm6OtuFault, otuFault, TO_REDIS(),
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(otuFault);
    )

    Chm6OtuPm otuPm;
    otuPm.mutable_base_pm()->mutable_config_id()->set_value(otuAid);
    otuPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    otuPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    DataPlane::DpMsOtuPMSContainer otuAdPm;
    otuAdPm.aid.assign(otuAid);

    OtuPbTranslator pbTranslator;
    pbTranslator.pmAdDefault(otuAdPm);
    pbTranslator.pmAdToPb(otuAdPm, false, false, false, otuPm);

    string pmDataStr;
    MessageToJsonString(otuPm, &pmDataStr);
    INFN_LOG(SeverityLevel::info) << pmDataStr;

    TRACE_PM(Chm6OtuPm, otuPm, TO_REDIS(),
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(otuPm);
    )
}

void OtuStateCollector::dumpCacheState(std::string otuAid, std::ostream& os)
{
    os << "******* Otu State *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    std::string otuAidUp(otuAid);
    std::transform(otuAidUp.begin(), otuAidUp.end(), otuAidUp.begin(), ::toupper);

    TypeMapOtuState::iterator itOtuState;
    for (itOtuState = mMapOtuState.begin(); itOtuState != mMapOtuState.end(); ++itOtuState)
    {
        if ((otuAidUp.compare("ALL") == 0) || (otuAid.compare(itOtuState->first) == 0))
        {
            string data;
            MessageToJsonString(*itOtuState->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void OtuStateCollector::listCacheState(std::ostream& out)
{
    lock_guard<std::mutex> lock(mOtuDataMtx);

    for(auto& it : mMapOtuState)
    {
        out << it.first << endl;
    }

    out << endl;
}


void OtuStateCollector::dumpCacheFault(std::string otuAid, std::ostream& os)
{
    os << "******* Otu Fault *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    std::string otuAidUp(otuAid);
    std::transform(otuAidUp.begin(), otuAidUp.end(), otuAidUp.begin(), ::toupper);

    TypeMapOtuFault::iterator itOtuFault;
    for (itOtuFault = mMapOtuFault.begin(); itOtuFault != mMapOtuFault.end(); ++itOtuFault)
    {
        if ((otuAidUp.compare("ALL") == 0) || (otuAid.compare(itOtuFault->first) == 0))
        {
            string data;
            MessageToJsonString(*itOtuFault->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void OtuStateCollector::dumpCachePm(std::string otuAid, std::ostream& os)
{
    os << "******* Otu Pm *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    std::string otuAidUp(otuAid);
    std::transform(otuAidUp.begin(), otuAidUp.end(), otuAidUp.begin(), ::toupper);

    TypeMapOtuPm::iterator itOtuPm;
    for (itOtuPm = mMapOtuPm.begin(); itOtuPm != mMapOtuPm.end(); ++itOtuPm)
    {
        if ((otuAidUp.compare("ALL") == 0) || (otuAid.compare(itOtuPm->first) == 0))
        {
            string data;
            MessageToJsonString(*itOtuPm->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void OtuStateCollector::dumpCacheStateConfig(std::string otuAid, std::ostream& os)
{
    os << "******* Otu State Config *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    std::string otuAidUp(otuAid);
    std::transform(otuAidUp.begin(), otuAidUp.end(), otuAidUp.begin(), ::toupper);

    TypeMapOtuConfigCache::iterator itOtuConfig;
    for (itOtuConfig = mMapOtuConfigCache.begin(); itOtuConfig != mMapOtuConfigCache.end(); ++itOtuConfig)
    {
        if ((otuAidUp.compare("ALL") == 0) || (otuAid.compare(itOtuConfig->first) == 0))
        {
            os << "AID: " << itOtuConfig->first << std::endl;

            // TTI Expected
            std::ostringstream logSapi;
            for (uint i = 0; i < 16; i++)
            {
                logSapi << " 0x" << std::hex << (int)itOtuConfig->second->ttiExpected[i] << std::dec;
            }
            os << "  ExpectedTtiSAPI =" << logSapi.str() << std::endl;

            std::ostringstream logDapi;
            for (uint i = 16; i < 32; i++)
            {
                logDapi << " 0x" << std::hex << (int)itOtuConfig->second->ttiExpected[i] << std::dec;
            }
            os << "  ExpectedTtiDAPI =" << logDapi.str() << std::endl;

            std::ostringstream logOper;
            for (uint i = 32; i < MAX_TTI_LENGTH; i++)
            {
                logOper << " 0x" << std::hex << (int)itOtuConfig->second->ttiExpected[i] << std::dec;
            }
            os << "  ExpectedTtiOPER =" << logOper.str() << std::endl;

            // TTI Mismatch Type
            if (hal_common::TTI_MISMATCH_TYPE_UNSPECIFIED != itOtuConfig->second->ttiMismatchType)
            {
                hal_common::TtiMismatchType mismatchType = itOtuConfig->second->ttiMismatchType;
                os << "  TtiMismatchType: " << mpPbTranslator->toString(mismatchType) << std::endl;
            }

            // Signal Degrade Threshold
            os << "  Signal Degrade Threshold: " << itOtuConfig->second->sdThreshold << std::endl;

            // Signal Degrade Interval
            os << "  Signal Degrade Interval: " << itOtuConfig->second->sdInterval << std::endl;

            os << std::endl;
        }
    }
}

void OtuStateCollector::dumpSdTracking(std::string otuAid, std::ostream& os)
{
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    TypeMapOtuSdTracking::iterator itOtuSdTracking;
    for (itOtuSdTracking = mMapOtuSdTracking.begin(); itOtuSdTracking != mMapOtuSdTracking.end(); ++itOtuSdTracking)
    {
        os << " AID: "            << itOtuSdTracking->first
           << ", Faulted = "      << itOtuSdTracking->second->isSdFaulted
           << ", RaiseSeconds = " << itOtuSdTracking->second->sdRaiseSeconds
           << ", ClearSeconds = " << itOtuSdTracking->second->sdClearSeconds
           << std::endl;
    }
}

void OtuStateCollector::dumpTimTracking(std::string otuAid, std::ostream& os)
{
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    TypeMapOtuTimTracking::iterator itOtuTimTracking;
    for (itOtuTimTracking = mMapOtuTimTracking.begin(); itOtuTimTracking != mMapOtuTimTracking.end(); ++itOtuTimTracking)
    {
        os << " AID: "            << itOtuTimTracking->first
           << ", Faulted = "      << itOtuTimTracking->second
           << std::endl;
    }
}

void OtuStateCollector::setTimAlarmMode(bool mode, std::ostream& os)
{
    mTimAlarmMode = mode;
}

void OtuStateCollector::setSdAlarmMode(bool mode, std::ostream& os)
{
    mSdAlarmMode = mode;
}

void OtuStateCollector::updateInstalledConfig()
{
    DpAdapter::TypeMapOtuConfig mapAdConfig;

    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    mpDcoAd->getOtuConfig(mapAdConfig);

    for (auto& it : mapAdConfig)
    {
        Chm6OtuState* pState;

        findOrCreateStateObj(it.first, pState);

        mpPbTranslator->configAdToPb(*it.second, *pState);

        string data;
        MessageToJsonString(*pState, &data);
        INFN_LOG(SeverityLevel::info)  << data;

        TRACE_STATE(Chm6OtuState, *pState, TO_REDIS(),
        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
	)

        // Cache installed clients for Tom serdes tuning
        uint otuNum = mpPbTranslator->otnAidToTom(it.first);
        if ((0 < otuNum) && (MAX_CLIENTS >= otuNum))
        {
            mInstalledOtuClients |= 0x1 << (otuNum-1);
        }
        INFN_LOG(SeverityLevel::info) << "DCO installed OTU4 clients = 0x" << std::hex << mInstalledOtuClients << std::dec;
    }
}

void OtuStateCollector::updateInstalledConfig(const chm6_dataplane::Chm6OtuConfig& inCfg)
{
    std::string otuId = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << otuId;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    Chm6OtuState* pState;

    findOrCreateStateObj(otuId, pState);

    pState->mutable_hal()->mutable_installed_config()->MergeFrom(inCfg.hal());

    TRACE_STATE(Chm6OtuState, *pState, TO_REDIS(),
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    )
}

int OtuStateCollector::getCacheState(std::string stateId, chm6_dataplane::Chm6OtuState*& pMsg)
{
    INFN_LOG(SeverityLevel::debug) << "configId: " << stateId;

    lock_guard<std::mutex> lock(mOtuDataMtx);

    auto it = mMapOtuState.find(stateId);

    if (it == mMapOtuState.end())
    {
        INFN_LOG(SeverityLevel::info) << "stateId: " << stateId << " NOT FOUND!!";
        return -1;
    }

    pMsg = it->second;

    return 0;
}

void OtuStateCollector::findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6OtuState*& pState)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapOtuState.find(stateId) == mMapOtuState.end())
    {
        // Create State
        pState = new chm6_dataplane::Chm6OtuState();
        mMapOtuState[stateId] = pState;
    }
    else
    {
        pState = mMapOtuState[stateId];
    }

    pState->mutable_base_state()->mutable_config_id()->set_value(stateId);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
}

void OtuStateCollector::setIsConnectionUp(bool val)
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    if (val != mIsConnectionUp)
    {
        mIsConnectionUp = val;

        if (!mIsConnectionUp)
        {
            INFN_LOG(SeverityLevel::info) << "Connection Down. Set PM validity flag to false";

            TypeMapOtuPm::iterator itOtuPm;
            for (itOtuPm = mMapOtuPm.begin(); itOtuPm != mMapOtuPm.end(); ++itOtuPm)
            {
                std::string aid = itOtuPm->second->base_pm().config_id().value();
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Setting PM invalid for Otu";

                DataPlane::DpMsOtuPMSContainer otuAdPm;
                otuAdPm.aid.assign(aid);

                Chm6OtuPm otuPbPm;
                otuPbPm.mutable_base_pm()->mutable_config_id()->set_value(aid);

                OtuPbTranslator pbTranslator;
                pbTranslator.pmAdDefault(otuAdPm);
                bool fecCorrEnable = mpDcoAd->getOtuFecCorrEnable(aid);
                pbTranslator.pmAdToPb(otuAdPm, fecCorrEnable, false, false, otuPbPm);

                TRACE_PM(Chm6OtuPm, otuPbPm, TO_REDIS(),
                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(otuPbPm);
		)
            }
        }
    }
}

bool OtuStateCollector::checkBootUpTraffic()
{
    std::lock_guard<std::mutex> lock(mOtuDataMtx);

    bool traffic_up = true;

    for (TypeMapOtuFault::iterator itOtuFault = mMapOtuFault.begin(); itOtuFault != mMapOtuFault.end(); ++itOtuFault)
    {
        std::string aid = itOtuFault->second->base_fault().config_id().value();

        if (mMapOtuFacFaultBitmap.find(aid) == mMapOtuFacFaultBitmap.end())
        {
            continue;
        }

        int otuId = mpDcoAd->aidToOtuId(aid);
        if (true == isClientOtuId(otuId))
        {
            // evaluate only client port with xcon
            bool isXconPresent = DpXcMgrSingleton::Instance()->isXconPresent(aid, OBJ_MGR_OTU);
            if ((true == isXconPresent) && (0 != mMapOtuFacFaultBitmap[aid]))
            {
                traffic_up = false;
                break;
            }
        }
        else
        {
            if (0 != mMapOtuFacFaultBitmap[aid])
            {
                traffic_up = false;
                break;
            }
        }
    }

    return traffic_up;
}

} // namespace DataPlane
