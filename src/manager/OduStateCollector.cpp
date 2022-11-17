/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "OduStateCollector.h"
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
using chm6_dataplane::Chm6OduConfig;
using chm6_dataplane::Chm6OduState;
using chm6_dataplane::Chm6OduFault;
using chm6_dataplane::Chm6OduPm;

using namespace DpAdapter;


namespace DataPlane {

#define ODU_STA_COLL_INTVL 5
#define ODU_FLT_COLL_INTVL 1

#define ODU_FLT_BITMAP_DEFAULT 0xFFFFFFFFFFFFFFFF

OduStateCollector::OduStateCollector(OduPbTranslator* pbTrans, DpAdapter::DcoOduAdapter *pAd, shared_ptr<FpgaSramIf> fpgaSramIf)
    : DpStateCollector(ODU_STA_COLL_INTVL, ODU_FLT_COLL_INTVL)
    , mOduDataMtx()
    , mpPbTranslator(pbTrans)
    , mpDcoAd(pAd)
    , mTimAlarmMode(true)
    , mSdAlarmMode(true)
    , mspFpgaSramIf(fpgaSramIf)
{
    assert(pbTrans != NULL);
    assert(pAd  != NULL);

    mMapOduState.clear();
    mMapOduFault.clear();
    mMapOduPm.clear();

    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        mCollectStatePollMode = true;
        mCollectFaultPollMode = true;
    }
}

void  OduStateCollector::createOdu(const chm6_dataplane::Chm6OduConfig& inCfg)
{
    TRACE(
    std::string oduAid = inCfg.base_config().config_id().value();

    TRACE_TAG("message.aid", oduAid);

    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapOduState.find(oduAid) == mMapOduState.end())
    {
        // Create Odu state object
        Chm6OduState* pState = new Chm6OduState();
        pState->mutable_base_state()->mutable_config_id()->set_value(oduAid);
        pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        pState->mutable_hal()->mutable_installed_config()->CopyFrom(inCfg.hal());

        int oduIndex = mpDcoAd->getCacheOduId(oduAid);
        if (oduIndex < 0)
        {
            // Note: this failure is expected in the onResync use case
            INFN_LOG(SeverityLevel::debug) << "Failed to get OduId from oduAid " << oduAid;
        }
        else
        {
            pState->mutable_hal()->mutable_installed_config()->mutable_odu_index()->set_value(oduIndex);
        }

        mMapOduState[oduAid] = pState;

        TRACE_STATE(Chm6OduState, *pState, TO_REDIS(),
        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
	)
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " state object already created";
    }

    if (mMapOduFault.find(oduAid) == mMapOduFault.end())
    {
        // Create Odu fault object
        Chm6OduFault* pFault = new Chm6OduFault();
        pFault->mutable_base_fault()->mutable_config_id()->set_value(oduAid);
        pFault->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pFault->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mMapOduFacFaultBitmap[oduAid]  = ODU_FLT_BITMAP_DEFAULT;

        mMapOduFault[oduAid] = pFault;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " fault object already created";
    }

    if (mMapOduPm.find(oduAid) == mMapOduPm.end())
    {
        // Create Odu pm object
        Chm6OduPm* pPm = new Chm6OduPm();
        pPm->mutable_base_pm()->mutable_config_id()->set_value(oduAid);
        pPm->mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pPm->mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mMapOduPm[oduAid] = pPm;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " pm object already created";
    }

    if (mMapOduConfigCache.find(oduAid) == mMapOduConfigCache.end())
    {
        // Create Odu config attribute cache
        createConfigCache(oduAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " config attribute cache already created";
    }

    // Initialize Odu signal degrade tracking
    if (mMapOduSdRxTracking.find(oduAid) == mMapOduSdRxTracking.end())
    {
        // Create Odu signal degrade tracking
        createSdTracking(oduAid, FAULTDIRECTION_RX);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " signal degrade rx tracking created";
    }
    if (mMapOduSdTxTracking.find(oduAid) == mMapOduSdTxTracking.end())
    {
        // Create Odu signal degrade tracking
        createSdTracking(oduAid, FAULTDIRECTION_TX);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " signal degrade tx tracking created";
    }

    )
}

void OduStateCollector::deleteOdu(std::string oduAid)
{
    INFN_LOG(SeverityLevel::info) << "AID: " << oduAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    if (mMapOduState.find(oduAid) != mMapOduState.end())
    {
        delete mMapOduState[oduAid];
        mMapOduState.erase(oduAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " state object does not exist";
    }

    if (mMapOduFault.find(oduAid) != mMapOduFault.end())
    {
        updateOduSdScratchpad(oduAid, false, FAULTDIRECTION_RX);
        updateOduSdScratchpad(oduAid, false, FAULTDIRECTION_TX);
        delete mMapOduFault[oduAid];
        mMapOduFault.erase(oduAid);

        mMapOduFacFaultBitmap.erase(oduAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " fault object does not exist";
    }

    if (mMapOduPm.find(oduAid) != mMapOduPm.end())
    {
        delete mMapOduPm[oduAid];
        mMapOduPm.erase(oduAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " pm object does not exist";
    }

    if (mMapOduConfigCache.find(oduAid) != mMapOduConfigCache.end())
    {
        delete mMapOduConfigCache[oduAid];
        mMapOduConfigCache.erase(oduAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " config attribute cache does not exist";
    }

    if (mMapOduSdRxTracking.find(oduAid) != mMapOduSdRxTracking.end())
    {
        delete mMapOduSdRxTracking[oduAid];
        mMapOduSdRxTracking.erase(oduAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " signal degrade rx tracking does not exist";
    }
    if (mMapOduSdTxTracking.find(oduAid) != mMapOduSdTxTracking.end())
    {
        delete mMapOduSdTxTracking[oduAid];
        mMapOduSdTxTracking.erase(oduAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " signal degrade tx tracking does not exist";
    }
}


void OduStateCollector::start()
{
    mThrFaults = boost::thread(boost::bind(
            &OduStateCollector::collectOduFaults, this
            ));

    mThrFaults.detach();

    mThrState = boost::thread(boost::bind(
            &OduStateCollector::collectOduStatus, this
            ));

    mThrState.detach();
}

void OduStateCollector::collectOduStatus()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of Odu State";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mOduDataMtx);

            if ((true == mCollectStatePollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateOduStatus();
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

    INFN_LOG(SeverityLevel::info) << "Finish collection of Odu State";
}

void OduStateCollector::collectOduFaults()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of Odu Faults";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mOduDataMtx);

            if ((true == mCollectFaultPollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateOduFaults();
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

    INFN_LOG(SeverityLevel::info) << "Finish collection of Odu Faults";
}

void OduStateCollector::getAndUpdateOduStatus()
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    TypeMapOduState::iterator itOduState;
    for (itOduState = mMapOduState.begin(); itOduState != mMapOduState.end(); ++itOduState)
    {
        TRACE(

        std::string aid = itOduState->second->base_state().config_id().value();
        INFN_LOG(SeverityLevel::debug) << "AID: " <<  aid << " Collecting status for Odu: " << aid;

        TRACE_TAG("message.aid", aid);
        TRACE_TAG("message.name", itOduState->second->GetDescriptor()->full_name());

        chm6_dataplane::Chm6OduState prevOduState(*itOduState->second);

        // Get Odu state
        DpAdapter::OduStatus adOduStat;

        uint8_t* rxTtiPtr = mpDcoAd->getOduRxTti(aid);
	TRACE(TRACE_TAG("rxTtiPtr", rxTtiPtr))
        if (NULL == rxTtiPtr)
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Failed DCO Adapter get RxTTI";
            continue;
        }



        // In presence of hard fault, clear the TTI
        DpAdapter::OduFault oduFault;
        if (mpDcoAd->getOduFault(aid, &oduFault))
        {
            bool isHardFault = mpPbTranslator->isOduHardFault(oduFault, FAULTDIRECTION_RX);
            if (true == isHardFault)
            {
                rxTtiPtr = NULL;
            }
        }

        mpPbTranslator->statusAdToPb(rxTtiPtr, *itOduState->second);


        // Add mpDcoAd->getOduCdi()
        DpAdapter::Cdi cdi = mpDcoAd->getOduCdi(aid);
        mpPbTranslator->statusAdToPb(cdi, *itOduState->second);

        if (mpPbTranslator->isPbChanged(prevOduState, *itOduState->second))
        {
            INFN_LOG(SeverityLevel::debug) << "AID: " <<  aid << " Odu State update";
            string stateDataStr;
            MessageToJsonString(*itOduState->second, &stateDataStr);
            INFN_LOG(SeverityLevel::debug) << stateDataStr;

            TRACE_STATE(Chm6OduState, *itOduState->second, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itOduState->second);
	    )
        }

	)
    }


}

void OduStateCollector::getAndUpdateOduFaults()
{
    if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
    {
        return;
    }

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    TypeMapOduFault::iterator itOduFault;
    for (itOduFault = mMapOduFault.begin(); itOduFault != mMapOduFault.end(); ++itOduFault)
    {
        TRACE (

        std::string aid = itOduFault->second->base_fault().config_id().value();
        INFN_LOG(SeverityLevel::debug) << "AID: " <<  aid << " Collecting fault for Odu: " << aid;

        TRACE_TAG("message.aid", aid);
        TRACE_TAG("message.name", itOduFault->second->GetDescriptor()->full_name());

        itOduFault->second->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        itOduFault->second->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        if (false == dp_env::DpEnvSingleton::Instance()->isSimEnv())
        {
            DpAdapter::OduFault oduFault;
            if (mpDcoAd->getOduFault(aid, &oduFault))
            {
                DpAdapter::OduFault oduFaultCurrent;
                oduFaultCurrent.facBmp = (mMapOduFacFaultBitmap.end() == mMapOduFacFaultBitmap.find(aid) ? ODU_FLT_BITMAP_DEFAULT : mMapOduFacFaultBitmap[aid]);

                bool isRxHardFault = mpPbTranslator->isOduHardFault(oduFault, FAULTDIRECTION_RX);
                bool isTxHardFault = mpPbTranslator->isOduHardFault(oduFault, FAULTDIRECTION_TX);

                if (true == mTimAlarmMode)
                {
                    bool timFault = evaluateTimFault(aid, isRxHardFault);

                    INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " TIM Fault = " << timFault;

                    // We inject ODU BDI on TIM alarm
                    if (mpDcoAd->setOduTimBdi(aid, timFault) == false)
                    {
                        INFN_LOG(SeverityLevel::error) << "AID: " << aid << " Failed to set/clear Odu BDI: " << aid;
                    }

                    mpPbTranslator->addOduTimFault(timFault, oduFault);
                }

                int oduId = mpDcoAd->aidToOduId(aid);

                if (true == mSdAlarmMode)
                {
                    bool sdFault = evaluateSdFault(aid, isRxHardFault, FAULTDIRECTION_RX);

                    updateOduSdScratchpad(aid, sdFault, FAULTDIRECTION_RX);

                    mpPbTranslator->addOduSdFault(sdFault, oduFault, FAULTDIRECTION_RX);

                    if (isValidOduId(oduId) && isClientOduId(oduId))
                    {
                        sdFault = evaluateSdFault(aid, isTxHardFault, FAULTDIRECTION_TX);

                        updateOduSdScratchpad(aid, sdFault, FAULTDIRECTION_TX);

                        mpPbTranslator->addOduSdFault(sdFault, oduFault, FAULTDIRECTION_TX);
                    }
                }

                if (isValidOduId(oduId) && !isClientOduId(oduId))
                {
                    mpPbTranslator->addOduAisOnLoflomMsim(oduFault);
                }

                bool force = (ODU_FLT_BITMAP_DEFAULT == oduFaultCurrent.facBmp) ? true : false;

                if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
                {
                    INFN_LOG(SeverityLevel::info) << "Block unstable faults for AID: " << aid << " during DCO shutdown";
                    return;
                }

                if ((mpPbTranslator->isAdChanged(oduFaultCurrent, oduFault)) || (true == force))
                {
                    INFN_LOG(SeverityLevel::info) << "AID: " << aid << std::hex << " FaultBmpCur(0x" << oduFaultCurrent.facBmp << ") FaultBmpNew(0x" << oduFault.facBmp << ")" << std::dec;

                    mpPbTranslator->faultAdToPb(aid, oduFaultCurrent.facBmp, oduFault, *itOduFault->second, force);

                    TRACE_FAULT(Chm6OduFault, *itOduFault->second, TO_REDIS(),
                    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itOduFault->second);
		    )

                    mMapOduFacFaultBitmap[aid] = oduFault.facBmp;
                }
            }
        }
        else
        {
            // Get Odu fault
            uint64_t faultRx;
            if (mpDcoAd->getOduFaultRx(aid, &faultRx))
            {
                uint64_t faultCurrent = (mMapOduFacFaultBitmap.end() == mMapOduFacFaultBitmap.find(aid) ? ODU_FLT_BITMAP_DEFAULT : mMapOduFacFaultBitmap[aid]);
                if (faultCurrent != faultRx)
                {
                    LOG_FLT() << "AID: " << aid << " Odu Fault update: Current(0x" << std::hex << faultCurrent << std::dec << ") New(0x" << std::hex << faultRx << std::dec << ")";

                    mpPbTranslator->faultAdToPbSim(faultRx, *itOduFault->second);

                    TRACE_FAULT(Chm6OduFault, *itOduFault->second, TO_REDIS(),
                    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itOduFault->second);
		    )

                    mMapOduFacFaultBitmap[aid] = faultRx;
                }
            }

            uint64_t faultTx;
            if (mpDcoAd->getOduFaultTx(aid, &faultTx))
            {
                //
            }
        }

	)
    }
}

void OduStateCollector::createConfigCache(const std::string oduAid)
{
    INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " Create Odu State Collector config cache for TTI & SD parameters";

    OduConfigCache* pCfgCache = new OduConfigCache;

    // TIM TTI
    for (uint index = 0; index < MAX_TTI_LENGTH; index++)
    {
        pCfgCache->ttiExpected[index] = 0;
    }
    pCfgCache->ttiMismatchType = hal_common::TTI_MISMATCH_TYPE_DISABLED;

    // Signal Degrade
    pCfgCache->sdThresholdRx = 0;
    pCfgCache->sdIntervalRx = 0;
    pCfgCache->sdThresholdTx = 0;
    pCfgCache->sdIntervalTx = 0;

    mMapOduConfigCache[oduAid] = pCfgCache;
}

void OduStateCollector::updateConfigCache(const std::string oduAid, const chm6_dataplane::Chm6OduConfig* pCfg)
{
    INFN_LOG(SeverityLevel::info) << "Odu " << oduAid << " Update Odu State Collector config cache for TTI & SD parameters";

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    if (mMapOduConfigCache.find(oduAid) == mMapOduConfigCache.end())
    {
        INFN_LOG(SeverityLevel::error) << "Odu " << oduAid << " config attribute cache does not exist";
        return;
    }

    if (pCfg->has_hal() == false)
    {
        INFN_LOG(SeverityLevel::error) << "Odu " << oduAid << " - NO hal()";
        return;
    }

    if (pCfg->hal().has_alarm_threshold() == true)
    {
        // TTI Expected
        if (0 < pCfg->hal().alarm_threshold().tti_expected().tti_size())
        {
            for (google::protobuf::Map<uint32_t, hal_common::TtiType_TtiDataType>::const_iterator
                 it  = pCfg->hal().alarm_threshold().tti_expected().tti().begin();
                 it != pCfg->hal().alarm_threshold().tti_expected().tti().end(); ++it)
            {
                uint index = it->first;
                if (MAX_TTI_LENGTH > index)
                {
                    mMapOduConfigCache[oduAid]->ttiExpected[index] = it->second.value().value();
                }
            }
        }

        // TTI Mismatch Type
        if (hal_common::TTI_MISMATCH_TYPE_UNSPECIFIED != pCfg->hal().alarm_threshold().tti_mismatch_type())
        {
            mMapOduConfigCache[oduAid]->ttiMismatchType = pCfg->hal().alarm_threshold().tti_mismatch_type();
        }

        // Signal Degrade Threshold - Ingress
        if (pCfg->hal().alarm_threshold().has_signal_degrade_threshold())
        {
            mMapOduConfigCache[oduAid]->sdThresholdRx = pCfg->hal().alarm_threshold().signal_degrade_threshold().value();
        }

        // Signal Degrade Interval - Ingress
        if (pCfg->hal().alarm_threshold().has_signal_degrade_interval())
        {
            mMapOduConfigCache[oduAid]->sdIntervalRx = pCfg->hal().alarm_threshold().signal_degrade_interval().value();
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "Odu " << oduAid << " - NO hal().alarm_threshold()";
    }

    if (pCfg->hal().has_alarm_threshold_egress() == true)
    {
        // Signal Degrade Threshold - Egress
        if (pCfg->hal().alarm_threshold_egress().has_signal_degrade_threshold())
        {
            mMapOduConfigCache[oduAid]->sdThresholdTx = pCfg->hal().alarm_threshold_egress().signal_degrade_threshold().value();
        }

        // Signal Degrade Interval - Egress
        if (pCfg->hal().alarm_threshold_egress().has_signal_degrade_interval())
        {
            mMapOduConfigCache[oduAid]->sdIntervalTx = pCfg->hal().alarm_threshold_egress().signal_degrade_interval().value();
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "Odu " << oduAid << " - NO hal().alarm_threshold_egress()";
    }
}

bool OduStateCollector::evaluateTimFault(std::string oduAid, bool isHardFault)
{
    uint indexBeginA, indexEndA;
    uint indexBeginB, indexEndB;

    if (mMapOduConfigCache.find(oduAid) == mMapOduConfigCache.end())
    {
        INFN_LOG(SeverityLevel::error) << "Odu " << oduAid << " config attribute cache does not exist";
        return false;
    }

    if (true == isHardFault)
    {
        return false;
    }

    // Get Odu state
    uint8_t* rxTtiPtr;
    rxTtiPtr = mpDcoAd->getOduRxTti(oduAid);
    if (NULL == rxTtiPtr)
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " Failed DCO Adapter get RxTTI";
        return false;
    }

    switch (mMapOduConfigCache[oduAid]->ttiMismatchType)
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
             INFN_LOG(SeverityLevel::debug) << "Odu " << oduAid << "Disabled or Unknown TTI match type - CLEAR";
             return false;
    }

    for (uint index = indexBeginA; index <= indexEndA; index++)
    {
        if (mMapOduConfigCache[oduAid]->ttiExpected[index] != rxTtiPtr[index])
        {
            INFN_LOG(SeverityLevel::debug) << "Odu " << oduAid << "indexBeginA(" << indexBeginA << ") indexEndA(" << indexEndA << ") index(" << index << ") - RAISE";
            return true;
        }
    }

    if ((0 == indexBeginB) && (0 == indexEndB))
    {
        // Done with check
        INFN_LOG(SeverityLevel::debug) << "Odu " << oduAid << "Expected TTI match Received TTI - CLEAR";
        return false;
    }

    for (uint index = indexBeginB; index <= indexEndB; index++)
    {
        if (mMapOduConfigCache[oduAid]->ttiExpected[index] != rxTtiPtr[index])
        {
            INFN_LOG(SeverityLevel::debug) << "Odu " << oduAid << "indexBeginB(" << indexBeginB << ") indexEndB(" << indexEndB << ") index(" << index << ") - RAISE";
            return true;
        }
    }

    INFN_LOG(SeverityLevel::debug) << "Odu " << oduAid << "Expected TTI match Received TTI - CLEAR";

    return false;
}

void OduStateCollector::createSdTracking(const std::string oduAid, FaultDirection direction)
{
    // Create Odu signal degrade tracking
    OduSdTracking* pSdTracking = new OduSdTracking;
    pSdTracking->isSdFaulted = false;
    pSdTracking->sdRaiseSeconds = 0;
    pSdTracking->sdClearSeconds = 0;

    if (FAULTDIRECTION_RX == direction)
    {
        mMapOduSdRxTracking[oduAid] = pSdTracking;
    }
    else
    {
        mMapOduSdTxTracking[oduAid] = pSdTracking;
    }
}

bool OduStateCollector::evaluateSdFault(std::string oduAid, bool isHardFault, FaultDirection direction)
{
    bool fault = false;

    TypeMapOduSdTracking pMapOduSdTracking;
    pMapOduSdTracking = ((FAULTDIRECTION_RX == direction) ? mMapOduSdRxTracking : mMapOduSdTxTracking);

    if (mMapOduConfigCache.find(oduAid) == mMapOduConfigCache.end())
    {
        INFN_LOG(SeverityLevel::error) << "Odu " << oduAid << " " << toString(direction) << " config attribute cache does not exist";
        return false;
    }

    if (pMapOduSdTracking.find(oduAid) == pMapOduSdTracking.end())
    {
        INFN_LOG(SeverityLevel::error) << "Odu " << oduAid << " " << toString(direction) << " signal degrade tracking does not exist";
        return false;
    }

    uint sdThreshold;
    uint sdInterval;

    if (FAULTDIRECTION_RX == direction)
    {
        sdThreshold = mMapOduConfigCache[oduAid]->sdThresholdRx;
        sdInterval  = mMapOduConfigCache[oduAid]->sdIntervalRx;
    }
    else
    {
        sdThreshold = mMapOduConfigCache[oduAid]->sdThresholdTx;
        sdInterval  = mMapOduConfigCache[oduAid]->sdIntervalTx;
    }

    bool &isSdFaulted    = pMapOduSdTracking[oduAid]->isSdFaulted;
    uint &sdRaiseSeconds = pMapOduSdTracking[oduAid]->sdRaiseSeconds;
    uint &sdClearSeconds = pMapOduSdTracking[oduAid]->sdClearSeconds;

    // Do not evaluate bogus error blocks on hard fault
    if (true == isHardFault)
    {
        INFN_LOG(SeverityLevel::debug) << "AID: " << oduAid << " " << toString(direction) << " Ignore error blocks pm on hard fault";
        sdClearSeconds = 0;
        sdRaiseSeconds = 0;
        isSdFaulted = false;
        return false;
    }

    // Convert theshold to allowewEB
    uint32_t frameRate = mpDcoAd->getOduFrameRate(oduAid);

    uint64_t allowedEb = ((uint64_t)frameRate * sdThreshold) / 100 ;

    INFN_LOG(SeverityLevel::debug) << "AID: " << oduAid << " " << toString(direction) << " allowedEb = " << (uint64_t)allowedEb;

    if (0 >= allowedEb)
    {
        INFN_LOG(SeverityLevel::debug) << "AID: " << oduAid << " " << toString(direction) << " Allowed Error Block " << allowedEb << " is invalid";
        return false;
    }

    uint64_t errorBlocks = mpDcoAd->getOduPmErrorBlocks(oduAid, direction);

    INFN_LOG(SeverityLevel::debug) << "AID: " << oduAid << " " << toString(direction) << " errorBlocks = " << (uint64_t)errorBlocks;

    if (errorBlocks >= allowedEb)
    {
        sdClearSeconds = 0;
        //INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " " << toString(direction) << " errorBlocks " << errorBlocks << " EXCEED allowedEb";

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
            //INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " " << toString(direction) << " sdRaiseSeconds = " << sdRaiseSeconds << " - RAISE signal degrade fault";
        }
        else
        {
            sdRaiseSeconds++;
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " " << toString(direction) << " errorBlocks(" << errorBlocks  << ") allowedBlocks(" << allowedEb << ") raiseSec(" << sdRaiseSeconds << ")";
        }
    }
    else
    {
        sdRaiseSeconds = 0;
        //INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " " << toString(direction) << " errorBlocks " << errorBlocks << " LESS THAN allowedEb";

        if ((true == isSdFaulted) && (sdClearSeconds < sdInterval))
        {
            fault = true;
            sdClearSeconds++;
            INFN_LOG(SeverityLevel::info) << "AID: " << oduAid << " " << toString(direction) << " errorBlocks(" << errorBlocks  << ") allowedBlocks(" << allowedEb << ") clearSec(" << sdClearSeconds << ")";
        }
    }

    isSdFaulted = fault;

    return fault;
}

bool OduStateCollector::getOduSdScratchpad(std::string oduAid, FaultDirection direction)
{
    if (nullptr == mspFpgaSramIf)
    {
        return false;
    }

    int oduId = mpDcoAd->aidToOduId(oduAid);

    uint32 offset;
    if (false == isValidOduId(oduId))
    {
        return false;
    }
    else if (true == isClientOduId(oduId))
    {
        oduId = oduId - CLIENT_ODU_ID_OFFSET;
        offset = (FAULTDIRECTION_RX == direction) ? cFpgaSram_Odu4SdRxRegOffset : cFpgaSram_Odu4SdTxRegOffset;
    }
    else
    {
        offset = cFpgaSram_LoOduSdRxRegOffset;
    }

    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (oduId - 1))) == 0 ? false : true;

    return scratchFault;
}

void OduStateCollector::updateOduSdScratchpad(std::string oduAid, bool sdFault, FaultDirection direction)
{
    if (nullptr == mspFpgaSramIf)
    {
        return;
    }

    int oduId = mpDcoAd->aidToOduId(oduAid);

    uint32 offset;
    if (false == isValidOduId(oduId))
    {
        return;
    }
    else if (true == isClientOduId(oduId))
    {
        oduId = oduId - CLIENT_ODU_ID_OFFSET;
        offset = (FAULTDIRECTION_RX == direction) ? cFpgaSram_Odu4SdRxRegOffset : cFpgaSram_Odu4SdTxRegOffset;
    }
    else
    {
        offset = cFpgaSram_LoOduSdRxRegOffset;
    }

    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (oduId - 1))) == 0 ? false : true;

    if (scratchFault != sdFault)
    {
        INFN_LOG(SeverityLevel::info) << " Updating ODU SD " << toString(direction) << " Scratchpad(0x" << std::hex << offset << ") = 0x" << regVal << std::dec << " for AID: " << oduAid;

        if (sdFault)
        {
            regVal |= (0x1 << (oduId - 1));
            INFN_LOG(SeverityLevel::info) << " ODU SD " << toString(direction) << " Scratchpad set for AID: " << oduAid;
        }
        else
        {
            regVal &= ~(0x1 << (oduId - 1));
            INFN_LOG(SeverityLevel::info) << " ODU SD " << toString(direction) << " Scratchpad clear for AID: " << oduAid;
        }
        mspFpgaSramIf->Write32(offset, regVal);

        regVal = mspFpgaSramIf->Read32(offset);
        INFN_LOG(SeverityLevel::info) << " New ODU SD " << toString(direction) << " Scratchpad(0x" << std::hex << offset << ") = 0x" << regVal << std::dec << " for AID: " << oduAid;
    }
}

void OduStateCollector::createFaultPmPlaceHolder(std::string oduAid)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    Chm6OduFault oduFault;
    oduFault.mutable_base_fault()->mutable_config_id()->set_value(oduAid);
    oduFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    oduFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    string faultDataStr;
    MessageToJsonString(oduFault, &faultDataStr);
    INFN_LOG(SeverityLevel::info) << faultDataStr;

    TRACE_FAULT(Chm6OduFault, oduFault, TO_REDIS(),
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(oduFault);
    )

    Chm6OduPm oduPm;
    oduPm.mutable_base_pm()->mutable_config_id()->set_value(oduAid);
    oduPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    oduPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    DataPlane::DpMsOduPMContainer oduAdPm;
    oduAdPm.aid.assign(oduAid);

    OduPbTranslator pbTranslator;
    pbTranslator.pmAdDefault(oduAdPm);
    pbTranslator.pmAdToPb(oduAdPm, false, false, oduPm);

    string pmDataStr;
    MessageToJsonString(oduPm, &pmDataStr);
    INFN_LOG(SeverityLevel::info) << pmDataStr;

    TRACE_PM(Chm6OduPm, oduPm, TO_REDIS(),
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(oduPm);
    )
}

void OduStateCollector::dumpCacheState(std::string oduAid, std::ostream& os)
{
    os << "******* Odu State *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    std::string oduAidUp(oduAid);
    std::transform(oduAidUp.begin(), oduAidUp.end(), oduAidUp.begin(), ::toupper);

    TypeMapOduState::iterator itOduState;
    for (itOduState = mMapOduState.begin(); itOduState != mMapOduState.end(); ++itOduState)
    {
        if ((oduAidUp.compare("ALL") == 0) || (oduAid.compare(itOduState->first) == 0))
        {
            string data;
            MessageToJsonString(*itOduState->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void OduStateCollector::listCacheState(std::ostream& out)
{
    lock_guard<std::mutex> lock(mOduDataMtx);

    for(auto& it : mMapOduState)
    {
        out << it.first << endl;
    }

    out << endl;
}

void OduStateCollector::dumpCacheFault(std::string oduAid, std::ostream& os)
{
    os << "******* Odu Fault *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    std::string oduAidUp(oduAid);
    std::transform(oduAidUp.begin(), oduAidUp.end(), oduAidUp.begin(), ::toupper);

    TypeMapOduFault::iterator itOduFault;
    for (itOduFault = mMapOduFault.begin(); itOduFault != mMapOduFault.end(); ++itOduFault)
    {
        if ((oduAidUp.compare("ALL") == 0) || (oduAid.compare(itOduFault->first) == 0))
        {
            string data;
            MessageToJsonString(*itOduFault->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void OduStateCollector::dumpCachePm(std::string oduAid, std::ostream& os)
{
    os << "******* Odu Pm *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    std::string oduAidUp(oduAid);
    std::transform(oduAidUp.begin(), oduAidUp.end(), oduAidUp.begin(), ::toupper);

    TypeMapOduPm::iterator itOduPm;
    for (itOduPm = mMapOduPm.begin(); itOduPm != mMapOduPm.end(); ++itOduPm)
    {
        if ((oduAidUp.compare("ALL") == 0) || (oduAid.compare(itOduPm->first) == 0))
        {
            string data;
            MessageToJsonString(*itOduPm->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

std::string OduStateCollector::toString(const FaultDirection direction) const
{
    switch (direction)
    {
        case FAULTDIRECTION_RX: return ("RX");
        case FAULTDIRECTION_TX: return ("TX");
        default:                return ("UNKNOWN");
    }
}

void OduStateCollector::dumpCacheStateConfig(std::string oduAid, std::ostream& os)
{
    os << "******* Odu State Config *******" << std::endl << std::endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    std::string oduAidUp(oduAid);
    std::transform(oduAidUp.begin(), oduAidUp.end(), oduAidUp.begin(), ::toupper);

    TypeMapOduConfigCache::iterator itOduConfig;
    for (itOduConfig = mMapOduConfigCache.begin(); itOduConfig != mMapOduConfigCache.end(); ++itOduConfig)
    {
        if ((oduAidUp.compare("ALL") == 0) || (oduAid.compare(itOduConfig->first) == 0))
        {
            os << "AID: " << itOduConfig->first;

            // TTI Expected
            std::ostringstream logSapi;
            for (uint i = 0; i < 16; i++)
            {
                logSapi << " 0x" << std::hex << (int)itOduConfig->second->ttiExpected[i] << std::dec;
            }
            os << "  ExpectedTtiSAPI =" << logSapi.str() << std::endl;

            std::ostringstream logDapi;
            for (uint i = 16; i < 32; i++)
            {
                logDapi << " 0x" << std::hex << (int)itOduConfig->second->ttiExpected[i] << std::dec;
            }
            os << "  ExpectedTtiDAPI =" << logDapi.str() << std::endl;

            std::ostringstream logOper;
            for (uint i = 32; i < MAX_TTI_LENGTH; i++)
            {
                logOper << " 0x" << std::hex << (int)itOduConfig->second->ttiExpected[i] << std::dec;
            }
            os << "  ExpectedTtiOPER =" << logOper.str() << std::endl;

            // TTI Mismatch Type
            if (hal_common::TTI_MISMATCH_TYPE_UNSPECIFIED != itOduConfig->second->ttiMismatchType)
            {
                hal_common::TtiMismatchType mismatchType = itOduConfig->second->ttiMismatchType;
                os << "  TtiMismatchType: " << mpPbTranslator->toString(mismatchType) << std::endl;
            }

            // Signal Degrade Threshold & Interval
            os << "  Signal Degrade Threshold Rx: " << itOduConfig->second->sdThresholdRx << std::endl;
            os << "  Signal Degrade Interval  Rx: " << itOduConfig->second->sdIntervalRx << std::endl;
            os << "  Signal Degrade Threshold Tx: " << itOduConfig->second->sdThresholdTx << std::endl;
            os << "  Signal Degrade Interval  Tx: " << itOduConfig->second->sdIntervalTx << std::endl;

            os << std::endl;
        }
    }
}

void OduStateCollector::dumpSdTracking(std::string oduAid, std::ostream& os)
{
    TypeMapOduSdTracking::iterator itOduSdTracking;
    for (itOduSdTracking = mMapOduSdRxTracking.begin(); itOduSdTracking != mMapOduSdRxTracking.end(); ++itOduSdTracking)
    {
        os << " AID: "            << itOduSdTracking->first << "  (ingress)"
           << ", Faulted = "      << itOduSdTracking->second->isSdFaulted
           << ", RaiseSeconds = " << itOduSdTracking->second->sdRaiseSeconds
           << ", ClearSeconds = " << itOduSdTracking->second->sdClearSeconds
           << std::endl;
    }
    os << std::endl;

    for (itOduSdTracking = mMapOduSdTxTracking.begin(); itOduSdTracking != mMapOduSdTxTracking.end(); ++itOduSdTracking)
    {
        os << " AID: "            << itOduSdTracking->first << "  (egress)"
           << ", Faulted = "      << itOduSdTracking->second->isSdFaulted
           << ", RaiseSeconds = " << itOduSdTracking->second->sdRaiseSeconds
           << ", ClearSeconds = " << itOduSdTracking->second->sdClearSeconds
           << std::endl;
    }
    os << std::endl;
}

void OduStateCollector::setTimAlarmMode(bool mode, std::ostream& os)
{
    mTimAlarmMode = mode;
}

void OduStateCollector::setSdAlarmMode(bool mode, std::ostream& os)
{
    mSdAlarmMode = mode;
}

void OduStateCollector::updateInstalledConfig()
{
    DpAdapter::TypeMapOduConfig mapAdConfig;

    std::lock_guard<std::mutex> lock(mOduDataMtx);

    mpDcoAd->getOduConfig(mapAdConfig);

    for (auto& it : mapAdConfig)
    {
        Chm6OduState* pState;

        findOrCreateStateObj(it.first, pState);

        mpPbTranslator->configAdToPb(*it.second, *pState);

        string data;
        MessageToJsonString(*pState, &data);
        INFN_LOG(SeverityLevel::info)  << data;

        TRACE_STATE(Chm6OduState, *pState, TO_REDIS(),
        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
	)
    }
}

void OduStateCollector::updateInstalledConfig(const chm6_dataplane::Chm6OduConfig& inCfg)
{
    std::string oduId = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << oduId;

    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    Chm6OduState* pState;

    findOrCreateStateObj(oduId, pState);

    pState->mutable_hal()->mutable_installed_config()->MergeFrom(inCfg.hal());

    TRACE_STATE(Chm6OduState, *pState, TO_REDIS(), 
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    )

}

int OduStateCollector::getCacheState(std::string stateId, chm6_dataplane::Chm6OduState*& pMsg)
{
    INFN_LOG(SeverityLevel::debug) << "configId: " << stateId;

    lock_guard<std::mutex> lock(mOduDataMtx);

    auto it = mMapOduState.find(stateId);

    if (it == mMapOduState.end())
    {
        INFN_LOG(SeverityLevel::info) << "stateId: " << stateId << " NOT FOUND!!";
        return -1;
    }

    pMsg = it->second;

    return 0;
}

void OduStateCollector::updateOduIdx(std::string configId, uint32 oduIdx)
{
    INFN_LOG(SeverityLevel::info) << "configId: " << configId << " OduIdx: " << oduIdx;

    lock_guard<std::mutex> lock(mOduDataMtx);

    auto it = mMapOduState.find(configId);

    if (it == mMapOduState.end())
    {
        INFN_LOG(SeverityLevel::info) << "stateId: " << configId << " NOT FOUND!!";
        return;
    }

    chm6_dataplane::Chm6OduState* pState = it->second;
    pState->mutable_hal()->mutable_installed_config()->mutable_odu_index()->set_value(oduIdx);

    TRACE_STATE(Chm6OduState, *pState, TO_REDIS(),
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    )

}

void OduStateCollector::findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6OduState*& pState)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapOduState.find(stateId) == mMapOduState.end())
    {
        // Create State
        pState = new chm6_dataplane::Chm6OduState();
        mMapOduState[stateId] = pState;
    }
    else
    {
        pState = mMapOduState[stateId];
    }

    pState->mutable_base_state()->mutable_config_id()->set_value(stateId);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
}

void OduStateCollector::setIsConnectionUp(bool val)
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    if (val != mIsConnectionUp)
    {
        mIsConnectionUp = val;

        if (!mIsConnectionUp)
        {
            INFN_LOG(SeverityLevel::info) << "Connection Down. Set PM validity flag to false";

            TypeMapOduPm::iterator itOduPm;
            for (itOduPm = mMapOduPm.begin(); itOduPm != mMapOduPm.end(); ++itOduPm)
            {
                std::string aid = itOduPm->second->base_pm().config_id().value();
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Setting PM invalid for Odu";

                DataPlane::DpMsOduPMContainer oduAdPm;
                oduAdPm.aid.assign(aid);

                Chm6OduPm oduPbPm;
                oduPbPm.mutable_base_pm()->mutable_config_id()->set_value(aid);

                OduPbTranslator pbTranslator;
                pbTranslator.pmAdDefault(oduAdPm);
                pbTranslator.pmAdToPb(oduAdPm, false, false, oduPbPm);

                TRACE_PM(Chm6OduPm, oduPbPm, TO_REDIS(),
                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(oduPbPm);
		)
            }
        }
    }
}

bool OduStateCollector::checkBootUpTraffic()
{
    std::lock_guard<std::mutex> lock(mOduDataMtx);

    bool traffic_up = true;

    for (TypeMapOduFault::iterator itOduFault = mMapOduFault.begin(); itOduFault != mMapOduFault.end(); ++itOduFault)
    {
        std::string aid = itOduFault->second->base_fault().config_id().value();

        if (mMapOduFacFaultBitmap.find(aid) == mMapOduFacFaultBitmap.end())
        {
            continue;
        }

        bool isXconPresent = DpXcMgrSingleton::Instance()->isXconPresent(aid, OBJ_MGR_ODU);
        if ((true == isXconPresent) && (0 != mMapOduFacFaultBitmap[aid]))
        {
            traffic_up = false;
            break;
        }
    }

    return traffic_up;
}

} // namespace DataPlane
