/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "CarrierStateCollector.h"
#include "DataPlaneMgr.h"
#include "DpCardMgr.h"
#include "DpCarrierMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"
#include "DcoSecProcActor.h"
#include "CarrierPbTranslator.h"

#include <boost/bind.hpp>
#include <string>
#include <iostream>

using google::protobuf::util::MessageToJsonString;

using chm6_dataplane::Chm6CarrierConfig;
using chm6_dataplane::Chm6CarrierState;
using chm6_dataplane::Chm6CarrierFault;
using chm6_dataplane::Chm6CarrierPm;
using chm6_dataplane::Chm6ScgFault;
using chm6_dataplane::Chm6ScgPm;

using namespace std;


namespace DataPlane {

#define CAR_STA_COLL_INTVL 10
#define CAR_FLT_COLL_INTVL 1

#define CAR_FLT_BITMAP_DEFAULT 0xFFFFFFFFFFFFFFFF

CarrierStateCollector::CarrierStateCollector(CarrierPbTranslator* pbTrans, DpAdapter::DcoCarrierAdapter *pCarAd, shared_ptr<FpgaSramIf> fpgaSramIf)
    : DpStateCollector(CAR_STA_COLL_INTVL, CAR_FLT_COLL_INTVL)
    , mCarDataMtx()
    , mpPbTranslator(pbTrans)
    , mpDcoCarrierAd(pCarAd)
    , mspFpgaSramIf(fpgaSramIf)
{
    assert(pbTrans != NULL);
    assert(pCarAd  != NULL);

    mCollectStatePollMode = true;
    mIsCollectionEn = true;

    mMapCarState.clear();
    mMapCarFault.clear();
    mMapCarPm.clear();

    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        mCollectStatePollMode = true;
        mCollectFaultPollMode = true;
    }
}

void  CarrierStateCollector::createCarrier(const chm6_dataplane::Chm6CarrierConfig& inCfg)
{
    std::string carrierAid = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << carrierAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCarDataMtx);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapCarState.find(carrierAid) == mMapCarState.end())
    {
        // Create State
        Chm6CarrierState* pState = new Chm6CarrierState();
        pState->mutable_base_state()->mutable_config_id()->set_value(carrierAid);
        pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        // ... createInstalledConfig (UpdateInstalledConfig)
        pState->mutable_hal()->mutable_installed_config()->CopyFrom(inCfg.hal());

        mMapCarState[carrierAid] = pState;
        mCarrTxState[carrierAid] = hal_dataplane::CARRIER_OPTICAL_STATE_UNSPECIFIED;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Carrier " << carrierAid << " state object already created";
    }

    if (mMapCarFault.find(carrierAid) == mMapCarFault.end())
    {
        // Create Fault
        Chm6CarrierFault* pFault = new Chm6CarrierFault();
        pFault->mutable_base_fault()->mutable_config_id()->set_value(carrierAid);
        pFault->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pFault->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mMapCarFacFaultBitmap[carrierAid]  = CAR_FLT_BITMAP_DEFAULT;

        mMapCarFault[carrierAid] = pFault;

    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Carrier " << carrierAid << " fault object already created";
    }

    if (mMapCarPm.find(carrierAid) == mMapCarPm.end())
    {
        // Create Pm
        Chm6CarrierPm* pPm = new Chm6CarrierPm();
        pPm->mutable_base_pm()->mutable_config_id()->set_value(carrierAid);
        pPm->mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pPm->mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        mMapCarPm[carrierAid] = pPm;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Carrier " << carrierAid << " pm object already created";
    }

    mForceCarrFaultToNxp[carrierAid] = true;
    mForceCarrStateToNxp[carrierAid] = true;
}

void CarrierStateCollector::deleteCarrier(std::string carrierAid)
{
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << carrierAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCarDataMtx);

    if (mMapCarState.find(carrierAid) != mMapCarState.end())
    {
        delete mMapCarState[carrierAid];
        mMapCarState.erase(carrierAid);
        mCarrTxState.erase(carrierAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Carrier " << carrierAid << " state object does not exist";
    }

    if (mMapCarFault.find(carrierAid) != mMapCarFault.end())
    {
        INFN_LOG(SeverityLevel::debug)  << " Creating & Posting Event DCO_SECPROC_OFEC_STATUS_DELETE for carrier " << carrierAid << endl;
        DcoSecProcEvent* faultEvent = new DcoSecProcEvent(DcoSecProcEventType::DCO_SECPROC_OFEC_STATUS_DELETE, false, carrierAid);
        //Memory is freed inside the actor framework
        DcoSecProcEventListenerService::Instance()->PostEvent(faultEvent);

        delete mMapCarFault[carrierAid];
        mMapCarFault.erase(carrierAid);

        mMapCarFacFaultBitmap[carrierAid]  = CAR_FLT_BITMAP_DEFAULT;

    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Carrier " << carrierAid << " fault object does not exist";
    }

    mForceCarrFaultToNxp[carrierAid] = false;
    mForceCarrStateToNxp[carrierAid] = false;

    // Keep PM entry for continuous notifications
#if 0
    if (mMapCarPm.find(carrierAid) != mMapCarPm.end())
    {
         delete mMapCarPm[carrierAid];
         mMapCarPm.erase(carrierAid);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Carrier " << carrierAid << " pm object does not exist";
    }
#endif
}

void CarrierStateCollector::start()
{
    mThrFaults = boost::thread(boost::bind(
            &CarrierStateCollector::collectCarrierFaults, this
            ));

    mThrFaults.detach();

    mThrStats = boost::thread(boost::bind(
            &CarrierStateCollector::collectCarrierState, this
            ));

    mThrStats.detach();
}

void CarrierStateCollector::collectCarrierState()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of Carrier State";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mCarDataMtx);

            if ((true == mCollectStatePollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateCarrierState();
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

    INFN_LOG(SeverityLevel::info) << "Finish collection of Carrier States";
}

void CarrierStateCollector::collectCarrierFaults()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of Carrier Faults";

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mCarDataMtx);

            if ((true == mCollectFaultPollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    getAndUpdateCarrierFaults();
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

    INFN_LOG(SeverityLevel::info) << "Finish collection of Carrier Faults";
}

void CarrierStateCollector::getAndUpdateCarrierState()
{

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    TypeMapCarrierState::iterator itCarState;
    for (itCarState = mMapCarState.begin(); itCarState != mMapCarState.end(); ++itCarState)
    {
        DpAdapter::CarrierStatus state;
        chm6_dataplane::Chm6CarrierState prevCarState(*itCarState->second);
        std::string aid = itCarState->second->base_state().config_id().value();
        INFN_LOG(SeverityLevel::debug) << "AID: " <<  aid << " Collecting status for Carrier: " << aid;

		chm6_dataplane::Chm6CarrierState previCarState(*itCarState->second);
        bool isPbChanged = false;

        // Carier status notify only have small subset of update data.
        // the reset is initialized to 0
        bool ret = mpDcoCarrierAd->getCarrierStatusNotify(aid, &state);
        if (false == ret)
        {
            INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Failed DCO Adapter get State Notify";
            continue;
        }
        mpPbTranslator->statusAdToPb(state, *itCarState->second);
        isPbChanged = mpPbTranslator->isPbChanged(prevCarState, *itCarState->second);
        if (isPbChanged || (true == mForceCarrStateToNxp[aid]))
        {
            INFN_LOG(SeverityLevel::info) << "AID: " <<  aid << " Carrier State update";
            string stateDataStr;
            MessageToJsonString(*itCarState->second, &stateDataStr);
            INFN_LOG(SeverityLevel::info) << stateDataStr;

            if(isPbChanged)
            {
                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itCarState->second);
                INFN_LOG(SeverityLevel::info) << "AID: " <<  aid
                                              << " Prev Tx State = " << hal_dataplane::CarrierOpticalState_Name(prevCarState.mutable_hal()->tx_state())
                                              << " Current Tx State = " << hal_dataplane::CarrierOpticalState_Name(itCarState->second->mutable_hal()->tx_state())
                                              << " Scg Tx state = " << hal_dataplane::CarrierOpticalState_Name(mCarrTxState[aid]);
            }
            INFN_LOG(SeverityLevel::debug) << "AID: " <<  aid << " Prev Rx State = " << prevCarState.mutable_hal()->rx_state() << " Current Rx State = " << itCarState->second->mutable_hal()->rx_state();
            INFN_LOG(SeverityLevel::info) << "Carrier " << aid << " state force = " << mForceCarrStateToNxp[aid];
            //Update NXP only if there is change in the prev & current value of Rx state
            if((true == mForceCarrStateToNxp[aid]) || (prevCarState.mutable_hal()->rx_state() != itCarState->second->mutable_hal()->rx_state()))
            {
                bool isRxAcquired = false;
                if(hal_dataplane::CARRIER_OPTICAL_STATE_ACTIVE == itCarState->second->mutable_hal()->rx_state())
                {
                        isRxAcquired = true;
                }
                INFN_LOG(SeverityLevel::debug)  << " Creating & Posting DCO_SECPROC_RX_STATUS Event for carrier " << aid << endl;
                DcoSecProcEvent* faultEvent = new DcoSecProcEvent(DcoSecProcEventType::DCO_SECPROC_RX_STATUS, isRxAcquired, aid);
                //Memory is freed inside the actor framework
                DcoSecProcEventListenerService::Instance()->PostEvent(faultEvent);
            }
            if(true == mForceCarrStateToNxp[aid])
            {
              INFN_LOG(SeverityLevel::info) << "Carrier " << aid << " setting state proto to false";
               mForceCarrStateToNxp[aid] = false;
            }
            
        }
    }
}

void CarrierStateCollector::getAndUpdateCarrierFaults()
{
    if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
    {
        return;
    }

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (uint carNum = 1; carNum <= DataPlane::MAX_CARRIERS; carNum++)
    {
        std::string aid = mpPbTranslator->numToAidCar(carNum);
        INFN_LOG(SeverityLevel::debug) << "AID: " << aid << " Collecting fault for Carrier: " << carNum;

        // Create scgFault data in case there are faults for Scg
        std::string scgAid = mpPbTranslator->aidCarToScg(aid);

        Chm6ScgFault scgFault;
        scgFault.mutable_base_fault()->mutable_config_id()->set_value(scgAid);
        scgFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        scgFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        bool updateScgFault = false;

        if (false == dp_env::DpEnvSingleton::Instance()->isSimEnv())
        {
            DpAdapter::CarrierFault adCarFault;
            if (mpDcoCarrierAd->getCarrierFault(aid, &adCarFault))
            {
                DpAdapter::CarrierFault carFaultCurrent;
                carFaultCurrent.facBmp = (mMapCarFacFaultBitmap.end() == mMapCarFacFaultBitmap.find(aid) ? CAR_FLT_BITMAP_DEFAULT : mMapCarFacFaultBitmap[aid]);

                bool sdFault = mpPbTranslator->isCarrierSdFault(adCarFault);
                updateCarrierSdScratchpad(aid, sdFault);

                bool sfFault = mpPbTranslator->isCarrierSfFault(adCarFault);
                updateCarrierSfScratchpad(aid, sfFault);

                bool preFecSdFault = mpPbTranslator->isCarrierPreFecSdFault(adCarFault);
                updateCarrierPreFecSdScratchpad(aid, preFecSdFault);

                bool bsdFault = mpPbTranslator->isCarrierBsdFault(adCarFault);
                updateCarrierBsdScratchpad(aid, bsdFault);

                bool olosFault = mpPbTranslator->isCarrierOlosFault(adCarFault);
                updateCarrierOlosScratchpad(aid, olosFault);

                bool force = (CAR_FLT_BITMAP_DEFAULT == carFaultCurrent.facBmp) ? true : false;
                bool isFaultChanged = mpPbTranslator->isAdChanged(carFaultCurrent, adCarFault);

                if ((true == isFaultChanged) || (true == force) || (true == mForceCarrFaultToNxp[aid]))
                {
                    TypeMapCarrierFault::iterator itCarFault;
                    itCarFault = mMapCarFault.find(aid);

                    // Carrier object created (i.e. SCH created)
                    if (itCarFault != mMapCarFault.end())
                    {
                        itCarFault->second->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
                        itCarFault->second->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

                        if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
                        {
                            INFN_LOG(SeverityLevel::info) << "Block unstable faults for AID: " << aid << " during DCO shutdown";
                            return;
                        }

                        INFN_LOG(SeverityLevel::info) << "AID: " << aid << std::hex << " FaultBmpCur(0x" << carFaultCurrent.facBmp << ") FaultBmpNew(0x" << adCarFault.facBmp << ")" << std::dec;

                        mpPbTranslator->faultAdToPb(aid, carFaultCurrent.facBmp, adCarFault, *itCarFault->second, force);

                        if((true == isFaultChanged) || (true == force))
                        {
                            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itCarFault->second);
                        }
                        //Extract the OLOS & LOF value from the Carrier Fault map
                        auto ofecCc_FaultCurrent = mpPbTranslator->isOfecCCFault(adCarFault);
                        INFN_LOG(SeverityLevel::debug) << " Carrier " << aid << " Force fault = " << mForceCarrFaultToNxp[aid] << " Current = " << ofecCc_FaultCurrent << " isFaultChanged = " << isFaultChanged << endl;
                        if((true == mForceCarrFaultToNxp[aid]) || (true == isFaultChanged) || (true == force))
                        {
                           auto isOfecUp = !ofecCc_FaultCurrent;

                           INFN_LOG(SeverityLevel::debug)  << " Creating & Posting Event DCO_SECPROC_OFEC_STATUS_UPDATE carrier " << aid << " ofecUp = " << isOfecUp << endl;
                           DcoSecProcEvent* faultEvent = new DcoSecProcEvent(DcoSecProcEventType::DCO_SECPROC_OFEC_STATUS_UPDATE, isOfecUp, aid);
                           DcoSecProcEventListenerService::Instance()->PostEvent(faultEvent);
                        }

                        if(true == mForceCarrFaultToNxp[aid])
                        {
                            INFN_LOG(SeverityLevel::info) << "Carrier " << aid << " setting fault to false";
                            mForceCarrFaultToNxp[aid] = false;
                        }
                    }

                    TypeMapCarrierFaultBitmap::iterator itCarFacFaultBitmap;
                    itCarFacFaultBitmap = mMapCarFacFaultBitmap.find(aid);

                    // Scg object created or SCH created
                    if (itCarFacFaultBitmap != mMapCarFacFaultBitmap.end())
                    {
                        INFN_LOG(SeverityLevel::info) << "AID: " << scgAid << std::hex << " FaultBmpCur(0x" << carFaultCurrent.facBmp << ") FaultBmpNew(0x" << adCarFault.facBmp << ")" << std::dec;

                        mpPbTranslator->faultAdToPb(scgAid, carFaultCurrent.facBmp, adCarFault, scgFault, force);

                        updateScgFault = true;

                        mMapCarFacFaultBitmap[aid] = adCarFault.facBmp;

                        if (true == mForceCarrFaultToNxp[aid])
                        {
                            INFN_LOG(SeverityLevel::info) << "Carrier " << aid << " setting NXP force fault update to false";
                            mForceCarrFaultToNxp[aid] = false;
                        }
                    }
                }
            } // if (mpDcoCarrierAd->getCarrierFault(aid, &adCarFault))

            // Check if update Carrier TX not active fault
            auto itr = mMapCarState.find(aid);
            if (itr != mMapCarState.end())
            {
                const chm6_dataplane::Chm6CarrierState& pbCarrState = *(itr->second);
                bool isCarrTxStateChanged = mpPbTranslator->isCarrierTxStateChanged(aid, pbCarrState, mCarrTxState[aid]);

                if (isCarrTxStateChanged == true)
                {
                    // Handle carrier lock/unlock fault based on carrier TX's state
                    mpPbTranslator->addPbScgCarrierLockFault(scgAid, mCarrTxState[aid], scgFault);

                    updateScgFault = true;
                }
            }

            if (updateScgFault == true)
            {
                if (true == DataPlaneMgrSingleton::Instance()->getDcoShutdownInProgress())
                {
                    INFN_LOG(SeverityLevel::info) << "Block unstable faults for SCG AID: " << aid << " during DCO shutdown";
                    return;
                }
                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(scgFault);
            }
        }
        else
        {
            //Keeping this as it is for SIM, currently this is not supported on SIM
            auto ofecCcFaultMask = ((1 << CarrierPbTranslator::CAR_FAULT_BDI) | (1 << CarrierPbTranslator::CAR_FAULT_LOF) | ( 1 << CarrierPbTranslator::CAR_FAULT_OLOS) | (1 << CarrierPbTranslator::CAR_FAULT_POST_FEC_Q_SIGNAL_FAILURE));
            TypeMapCarrierFault::iterator itCarFault;
            itCarFault = mMapCarFault.find(aid);

            if (itCarFault != mMapCarFault.end())
            {
                // Get Carrier faults
                uint64_t fault;
                if (mpDcoCarrierAd->getCarrierFault(aid, &fault))
                {
                    uint64_t faultCurrent = (mMapCarFacFaultBitmap.end() == mMapCarFacFaultBitmap.find(aid) ? CAR_FLT_BITMAP_DEFAULT : mMapCarFacFaultBitmap[aid]);
                    if ((faultCurrent != fault) || (true == mForceCarrFaultToNxp[aid]))
                    {
                        mpPbTranslator->faultAdToPbSim(fault, *itCarFault->second);

                        if(faultCurrent != fault)
                        {
                            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*itCarFault->second);
                        }
                        auto ofecCc_Fault = fault & ofecCcFaultMask;
                        auto ofecCc_FaultCurrent = faultCurrent & ofecCcFaultMask;
                        INFN_LOG(SeverityLevel::info) << "Carrier " << aid << " Force value = " << mForceCarrFaultToNxp[aid];
                        if((true == mForceCarrFaultToNxp[aid]) || (ofecCc_Fault != ofecCc_FaultCurrent))
                        {
                            auto isOfecUp = false;
                            if(0 == ofecCc_FaultCurrent)
                            {
                                isOfecUp = true;
                            }
                            INFN_LOG(SeverityLevel::debug)  << " Creating & Posting Event DCO_SECPROC_OFEC_STATUS_UPDATE carrier " << aid << endl;
                            DcoSecProcEvent* faultEvent = new DcoSecProcEvent(DcoSecProcEventType::DCO_SECPROC_OFEC_STATUS_UPDATE, isOfecUp, aid);
                            DcoSecProcEventListenerService::Instance()->PostEvent(faultEvent);
                        }

                        if(true == mForceCarrFaultToNxp[aid])
                        {
                            INFN_LOG(SeverityLevel::info) << "Carrier " << aid << "Setting state to false";
                            mForceCarrFaultToNxp[aid] = false;
                        }


                        std::string scgAid = mpPbTranslator->aidCarToScg(aid);

                        Chm6ScgFault scgFault;
                        scgFault.mutable_base_fault()->mutable_config_id()->set_value(scgAid);
                        scgFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
                        scgFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

                        mpPbTranslator->faultAdToPbSim(fault, scgFault);

                        string faultScgDataStr;
                        MessageToJsonString(scgFault, &faultScgDataStr);
                        INFN_LOG(SeverityLevel::info) << faultScgDataStr;

                        if (faultCurrent != fault)
                        {
                            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(scgFault);
                        }

                        mMapCarFacFaultBitmap[aid] = fault;
                    }
                }
            }
        }
    }
}

void CarrierStateCollector::updateCarrierSdScratchpad(std::string carAid, bool sdFault)
{
    if (nullptr == mspFpgaSramIf)
    {
        return;
    }

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);

    if ((carId < 1) || (carId > MAX_CARRIERS))
    {
        return;
    }

    uint32 offset = cFpgaSram_CarrierSdRegOffset;
    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (carId - 1))) == 0 ? false : true;

    if (scratchFault != sdFault)
    {
        INFN_LOG(SeverityLevel::info) << " Updating Carrier SD Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;

        if (sdFault)
        {
            regVal |= (0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier SD Scratchpad set for AID: " << carAid;
        }
        else
        {
            regVal &= ~(0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier SD Scratchpad clear for AID: " << carAid;
        }
        mspFpgaSramIf->Write32(offset, regVal);

        regVal = mspFpgaSramIf->Read32(offset);
        INFN_LOG(SeverityLevel::info) << " New Carrier SD Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;
    }
}

void CarrierStateCollector::updateCarrierSfScratchpad(std::string carAid, bool sfFault)
{
    if (nullptr == mspFpgaSramIf)
    {
        return;
    }

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);

    if ((carId < 1) || (carId > MAX_CARRIERS))
    {
        return;
    }

    uint32 offset = cFpgaSram_CarrierSfRegOffset;
    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (carId - 1))) == 0 ? false : true;

    if (scratchFault != sfFault)
    {
        INFN_LOG(SeverityLevel::info) << " Updating Carrier SF Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;

        if (sfFault)
        {
            regVal |= (0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier SF Scratchpad set for AID: " << carAid;
        }
        else
        {
            regVal &= ~(0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier SF Scratchpad clear for AID: " << carAid;
        }
        mspFpgaSramIf->Write32(offset, regVal);

        regVal = mspFpgaSramIf->Read32(offset);
        INFN_LOG(SeverityLevel::info) << " New Carrier SF Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;
    }
}

void CarrierStateCollector::updateCarrierPreFecSdScratchpad(std::string carAid, bool preFecsdFault)
{
    if (nullptr == mspFpgaSramIf)
    {
        return;
    }

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);

    if ((carId < 1) || (carId > MAX_CARRIERS))
    {
        return;
    }

    uint32 offset = cFpgaSram_CarrierPreFecSdRegOffset;
    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (carId - 1))) == 0 ? false : true;

    if (scratchFault != preFecsdFault)
    {
        INFN_LOG(SeverityLevel::info) << " Updating Carrier Pre FEC SD Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;

        if (preFecsdFault)
        {
            regVal |= (0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier Pre FEC SD Scratchpad set for AID: " << carAid;
        }
        else
        {
            regVal &= ~(0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier Pre FEC SD Scratchpad clear for AID: " << carAid;
        }
        mspFpgaSramIf->Write32(offset, regVal);

        regVal = mspFpgaSramIf->Read32(offset);
        INFN_LOG(SeverityLevel::info) << " New Carrier Pre FEC SD Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;
    }
}

void CarrierStateCollector::updateCarrierBsdScratchpad(std::string carAid, bool bsdFault)
{
    if (nullptr == mspFpgaSramIf)
    {
        return;
    }

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);

    if ((carId < 1) || (carId > MAX_CARRIERS))
    {
        return;
    }

    uint32 offset = cFpgaSram_CarrierBsdRegOffset;
    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (carId - 1))) == 0 ? false : true;

    if (scratchFault != bsdFault)
    {
        INFN_LOG(SeverityLevel::info) << " Updating Carrier BSD Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;

        if (bsdFault)
        {
            regVal |= (0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier BSD Scratchpad set for AID: " << carAid;
        }
        else
        {
            regVal &= ~(0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier BSD Scratchpad clear for AID: " << carAid;
        }
        mspFpgaSramIf->Write32(offset, regVal);

        regVal = mspFpgaSramIf->Read32(offset);
        INFN_LOG(SeverityLevel::info) << " New Carrier BSD Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;
    }
}

void CarrierStateCollector::updateCarrierOlosScratchpad(std::string carAid, bool isOlosFault)
{
    if (nullptr == mspFpgaSramIf)
    {
        return;
    }

    int carId = mpDcoCarrierAd->aidToCarrierId(carAid);

    if ((carId < 1) || (carId > MAX_CARRIERS))
    {
        return;
    }

    uint32 offset = cFpgaSram_CarrierOlosRegOffset;
    uint32 regVal = mspFpgaSramIf->Read32(offset);
    bool scratchFault = (regVal & (0x1 << (carId - 1))) == 0 ? false : true;

    if (scratchFault != isOlosFault)
    {
        INFN_LOG(SeverityLevel::info) << " Updating Carrier OLOS Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;

        if (isOlosFault)
        {
            regVal |= (0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier OLOS Scratchpad set for AID: " << carAid;
        }
        else
        {
            regVal &= ~(0x1 << (carId - 1));
            INFN_LOG(SeverityLevel::info) << " Carrier OLOS Scratchpad clear for AID: " << carAid;
        }
        mspFpgaSramIf->Write32(offset, regVal);

        regVal = mspFpgaSramIf->Read32(offset);
        INFN_LOG(SeverityLevel::info) << " New Carrier OLOS Scratchpad = 0x" << std::hex << regVal << std::dec << " for AID: " << carAid;
    }
}

void CarrierStateCollector::createFaultPmPlaceHolderScg(std::string scgAid)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    Chm6ScgFault scgFault;
    scgFault.mutable_base_fault()->mutable_config_id()->set_value(scgAid);
    scgFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    scgFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    string scgFaultDataStr;
    MessageToJsonString(scgFault, &scgFaultDataStr);
    INFN_LOG(SeverityLevel::info) << scgFaultDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(scgFault);

    Chm6ScgPm scgPm;
    scgPm.mutable_base_pm()->mutable_config_id()->set_value(scgAid);
    scgPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    scgPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    DataPlane::DpMsCarrierPMContainer carrierAdPm;
    CarrierPbTranslator pbTranslator;
    pbTranslator.pmAdDefault(carrierAdPm);
    pbTranslator.pmAdToPb(carrierAdPm, false, scgPm);

    string scgPmDataStr;
    MessageToJsonString(scgPm, &scgPmDataStr);
    INFN_LOG(SeverityLevel::info) << scgPmDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(scgPm);

    std::string carAid = mpPbTranslator->aidScgToCar(scgAid);
    mMapCarFacFaultBitmap [carAid] = CAR_FLT_BITMAP_DEFAULT;
}

void CarrierStateCollector::createFaultPmPlaceHolder(std::string carrierAid)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Carrier
    Chm6CarrierFault carrierFault;
    carrierFault.mutable_base_fault()->mutable_config_id()->set_value(carrierAid);
    carrierFault.mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    carrierFault.mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    string faultDataStr;
    MessageToJsonString(carrierFault, &faultDataStr);
    INFN_LOG(SeverityLevel::info) << faultDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(carrierFault);

    Chm6CarrierPm carrierPm;
    carrierPm.mutable_base_pm()->mutable_config_id()->set_value(carrierAid);
    carrierPm.mutable_base_pm()->mutable_timestamp()->set_seconds(tv.tv_sec);
    carrierPm.mutable_base_pm()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    DataPlane::DpMsCarrierPMContainer carrierAdPm;
    CarrierPbTranslator pbTranslator;
    pbTranslator.pmAdDefault(carrierAdPm);
    pbTranslator.pmAdToPb(carrierAdPm, false, carrierPm);

    string pmDataStr;
    MessageToJsonString(carrierPm, &pmDataStr);
    INFN_LOG(SeverityLevel::info) << pmDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(carrierPm);
}

void CarrierStateCollector::dumpCacheState(std::string carAid, std::ostream& os)
{
    os << "******* Carrier State *******" << endl << endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCarDataMtx);

    std::string carAidUp(carAid);
    std::transform(carAidUp.begin(), carAidUp.end(), carAidUp.begin(), ::toupper);

    TypeMapCarrierState::iterator itCarState;
    for (itCarState = mMapCarState.begin(); itCarState != mMapCarState.end(); ++itCarState)
    {
        if ((carAidUp.compare("ALL") == 0) || (carAid.compare(itCarState->first) == 0) )
        {
            string data;
            MessageToJsonString(*itCarState->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }

    os << endl << endl;
}

void CarrierStateCollector::listCacheState(std::ostream& out)
{
    lock_guard<std::mutex> lock(mCarDataMtx);

    for(auto& it : mMapCarState)
    {
        out << it.first << endl;
    }

    out << endl;
}

void CarrierStateCollector::dumpCacheFault(std::string carAid, std::ostream& os)
{
    os << "******* Carrier Fault *******" << endl << endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCarDataMtx);

    std::string carAidUp(carAid);
    std::transform(carAidUp.begin(), carAidUp.end(), carAidUp.begin(), ::toupper);

    TypeMapCarrierFault::iterator itCarFault;
    for (itCarFault = mMapCarFault.begin(); itCarFault != mMapCarFault.end(); ++itCarFault)
    {
        if ((carAidUp.compare("ALL") == 0) || (carAid.compare(itCarFault->first) == 0) )
        {
            string data;
            MessageToJsonString(*itCarFault->second, &data);
            mpPbTranslator->dumpJsonFormat(data, os);
        }
    }
}

void CarrierStateCollector::updateInstalledConfig()
{
    DpAdapter::TypeMapCarrierConfig mapAdConfig;
    DpAdapter::CardConfig cardCfg;

    DpAdapter::DcoCardAdapter* pCardAd = DpCardMgrSingleton::Instance()->getAdPtr();

    std::lock_guard<std::mutex> lock(mCarDataMtx);

    if (mpDcoCarrierAd->getCarrierConfig(mapAdConfig) == false)
    {
       INFN_LOG(SeverityLevel::error) << "Failed to retrieve Carrier Config from DCO";

       return;
    }

    if (pCardAd->getCardConfig(&cardCfg) == false)
    {
        INFN_LOG(SeverityLevel::error) << "Failed to retrieve Card Config from DCO";
       // what todo? Assert?
        return;
    }

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (auto& it : mapAdConfig)
    {
        Chm6CarrierState* pState;

        if (false == it.second->provisioned)
        {
            INFN_LOG(SeverityLevel::info) << "Carrier AID: " << it.first << " not provisioned, no update to Redis";
            continue;
        }

        findOrCreateStateObj(it.first, pState);

        mpPbTranslator->configAdToPb(*it.second, cardCfg, *pState);

        string data;
        MessageToJsonString(*pState, &data);
        INFN_LOG(SeverityLevel::info)  << data;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
    }
}

void CarrierStateCollector::updateInstalledConfig(const chm6_dataplane::Chm6CarrierConfig& inCfg)
{
    std::string carrierAid = inCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "AID: " << carrierAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCarDataMtx);

    chm6_dataplane::Chm6CarrierState *pState;

    findOrCreateStateObj(carrierAid, pState);

    pState->mutable_hal()->mutable_installed_config()->MergeFrom(inCfg.hal());

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);
}

void CarrierStateCollector::findOrCreateStateObj(std::string carId, chm6_dataplane::Chm6CarrierState*& pState)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (mMapCarState.find(carId) == mMapCarState.end())
    {
        INFN_LOG(SeverityLevel::info) << "Carrier State not present for ID: " << carId << " Creating new";

        pState = new Chm6CarrierState();
        mMapCarState[carId] = pState;

        mCarrTxState[carId] = hal_dataplane::CARRIER_OPTICAL_STATE_UNSPECIFIED;
    }
    else
    {
        pState = mMapCarState[carId];
    }

    pState->mutable_base_state()->mutable_config_id()->set_value(carId);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
}

void CarrierStateCollector::setIsConnectionUp(bool val)
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mCarDataMtx);
    INFN_LOG(SeverityLevel::info) << " Entering Function " << __FUNCTION__ << " Is Up = " << val;

    if (val != mIsConnectionUp)
    {

        INFN_LOG(SeverityLevel::info) << " Change is state of connection old state = " << mIsConnectionUp << " new state = " << val;
        mIsConnectionUp = val;

        if (!mIsConnectionUp)
        {
            INFN_LOG(SeverityLevel::info) << "Connection Down. Set PM validity flag to false";

            TypeMapCarrierPm::iterator itCarPm;
            for (itCarPm = mMapCarPm.begin(); itCarPm != mMapCarPm.end(); ++itCarPm)
            {
                std::string aid = itCarPm->second->base_pm().config_id().value();
                INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Setting PM invalid for Carrier";

                DataPlane::DpMsCarrierPMContainer carrierAdPm;

                Chm6CarrierPm carrierPbPm;
                carrierPbPm.mutable_base_pm()->mutable_config_id()->set_value(aid);

                CarrierPbTranslator pbTranslator;
                pbTranslator.pmAdDefault(carrierAdPm);
                pbTranslator.pmAdToPb(carrierAdPm, false, carrierPbPm);
                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(carrierPbPm);

                std::string scgAid = pbTranslator.aidCarToScg(aid);
                INFN_LOG(SeverityLevel::info) << "AID: " << scgAid << " Setting PM invalid for Scg";

                Chm6ScgPm scgPbPm;
                scgPbPm.mutable_base_pm()->mutable_config_id()->set_value(scgAid);

                pbTranslator.pmAdToPb(carrierAdPm, false, scgPbPm);
                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(scgPbPm);
            }
        }
        else
        {
            TypeMapCarrierPm::iterator itCarPm;
            INFN_LOG(SeverityLevel::info) << " Connection is up now ";
            for (itCarPm = mMapCarPm.begin(); itCarPm != mMapCarPm.end(); ++itCarPm)
            {
                std::string aid = itCarPm->second->base_pm().config_id().value();
                INFN_LOG(SeverityLevel::info) << " Connection is up now " << "aid = " << aid;
                mForceCarrStateToNxp[aid] = true;
                mForceCarrFaultToNxp[aid] = true;
            }
        }
    }
}

int CarrierStateCollector::getCacheState(std::string stateId, chm6_dataplane::Chm6CarrierState*& pMsg)
{
    INFN_LOG(SeverityLevel::debug) << "configId: " << stateId;

    lock_guard<std::mutex> lock(mCarDataMtx);

    auto it = mMapCarState.find(stateId);

    if (it == mMapCarState.end())
    {
        INFN_LOG(SeverityLevel::info) << "stateId: " << stateId << " NOT FOUND!!";
        return -1;
    }

    pMsg = it->second;

    return 0;
}

bool CarrierStateCollector::checkBootUpTraffic()
{
    std::lock_guard<std::mutex> lock(mCarDataMtx);

    if (mMapCarFault.begin() == mMapCarFault.end())
    {
        return false;
    }

    bool traffic_up = true;

    for (TypeMapCarrierFault::iterator itCarFault = mMapCarFault.begin(); itCarFault != mMapCarFault.end(); ++itCarFault)
    {
        std::string aid = itCarFault->second->base_fault().config_id().value();

        if (mMapCarFacFaultBitmap.find(aid) == mMapCarFacFaultBitmap.end())
        {
            continue;
        }

        if (0 != mMapCarFacFaultBitmap[aid])
        {
            traffic_up = false;
            break;
        }
    }

    return traffic_up;
}

} // namespace DataPlane
