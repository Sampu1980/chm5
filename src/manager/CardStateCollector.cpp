/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include <boost/bind.hpp>
#include <string>
#include <iostream>

#include "CardStateCollector.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "dp_common.h"
#include "DcoSecProcAdapter.h"
#include "DpProtoDefs.h"
#include <google/protobuf/util/message_differencer.h>
#include "RegIfException.h"
#include "ShmMutexGuard.h"

using google::protobuf::util::MessageToJsonString;

using namespace std;

using google::protobuf::util::MessageDifferencer;

namespace DataPlane {

#define CARD_STA_COLL_INTVL 5
#define CARD_FLT_COLL_INTVL 1

#define COMM_FAIL_SOAK_TIME_SET_INIT    600 // sec
#define COMM_FAIL_SOAK_TIME_SET         120 // sec
#define COMM_FAIL_SOAK_TIME_CLR          10 // sec

#define ILOG INFN_LOG(SeverityLevel::info)


CardStateCollector::CardStateCollector(CardPbTranslator* pbTrans, DpAdapter::DcoCardAdapter *pCarAd)
    : DpStateCollector(CARD_STA_COLL_INTVL, CARD_FLT_COLL_INTVL)
    , mCardEqptFaultBitmap(0xFFFFFFFF)
    , mCardDacEqptFaultBitmap(0xFFFFFFFF)
    , mCardPsEqptFaultBitmap(0xFFFFFFFF)
    , mCardPostFaultBitmap(0xFFFFFFFF)
    , mCardDacPostFaultBitmap(0xFFFFFFFF)
    , mCardPsPostFaultBitmap(0xFFFFFFFF)
    , mDcoCardRebootState(DpAdapter::DCO_CARD_REBOOT_NONE)
    , mCardDataMtx()
    , mpCardState(NULL)
    , mpCardFault(NULL)
    , mpDcoCapa(NULL)
    , mpOecKeyMgmtState(NULL)
    , mpPbTranslator(pbTrans)
    , mpDcoCardAd(pCarAd)
    , mDcoState(DCOSTATE_UNSPECIFIED)
    , mForceDcoState(DCOSTATE_UNSPECIFIED)
    , mForceTempHiFanIncrease(wrapper::BOOL_UNSPECIFIED)
    , mForceTempStableFanDecrease(wrapper::BOOL_UNSPECIFIED)
    , mConnectFault(false)
    , mConnectFaultLatch(false)
    , mIsFirstFaultCheck(true)
    , mPCPModified(false)
    , mIsDcoFaultReportEn(true)
    , mIsChm6HighPowerMode(true)
    , mpShmMtxFcp(NULL)
{
    INFN_LOG(SeverityLevel::info) << "";

    assert(pbTrans != NULL);
    assert(pCarAd  != NULL);

    mCollectFaultPollMode = true;
    mIsCollectionEn = true;

    if (true == dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        mCollectStatePollMode = true;
        mCollectFaultPollMode = true;
    }

    mIsKeyMgmtStateChanged = false;

    initShmMutex();
}

// todo: is configId needed?
void  CardStateCollector::createCard(std::string aid)
{
    INFN_LOG(SeverityLevel::info) << "Called for Aid " << aid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (NULL == mpCardState)
    {
        // Create State
        mpCardState = new chm6_common::Chm6DcoCardState();
        mpCardState->mutable_base_state()->mutable_config_id()->set_value("Chm6Internal");
        mpCardState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        mpCardState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        //Get state from DCO security processor
        (DpAdapter::DcoSecProcAdapter::GetInstance())->UpdateUpgradableDeviceList((*mpCardState));


        // Call singleton to gearbox and populate/add to map using mpCardState
        gearbox::GearBoxAdIf* adapter = gearbox::GearBoxAdapterSingleton::Instance();

        adapter->UpdateUpgradableDeviceList((*mpCardState));

        string stateData;
        MessageToJsonString(*mpCardState, &stateData);
        INFN_LOG(SeverityLevel::info) << stateData;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpCardState);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << aid << " state object already created";
    }

    if (NULL == mpOecKeyMgmtState)
    {
        mpOecKeyMgmtState = new infinera::chm6::keymgmt::v2::KeyMgmtState();
        mpOecKeyMgmtState->mutable_base_state()->mutable_config_id()->set_value(genKeyMgmtConfigId());
        mpOecKeyMgmtState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        mpOecKeyMgmtState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpOecKeyMgmtState);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << aid << " OEC key management object already created";
    }

    if (NULL == mpCardFault)
    {
        // Create Fault
        mpCardFault = new chm6_common::Chm6DcoCardFault();
        mpCardFault->mutable_base_fault()->mutable_config_id()->set_value(aid);
        mpCardFault->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
        mpCardFault->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        string faultData;
        MessageToJsonString(*mpCardFault, &faultData);
        INFN_LOG(SeverityLevel::info) << faultData;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpCardFault);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << aid << " fault object already created";
    }

    if (NULL == mpDcoCapa)
    {
        // Create DCO capabilities state
        std::string chm6Aid = mpPbTranslator->getChassisSlotAid();

        mpDcoCapa = new chm6_dataplane::DcoCapabilityState();
        mpDcoCapa->mutable_base_state()->mutable_config_id()->set_value(chm6Aid);
        mpDcoCapa->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        mpDcoCapa->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        string dcoCapaData;
        MessageToJsonString(*mpDcoCapa, &dcoCapaData);
        INFN_LOG(SeverityLevel::info) << dcoCapaData;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpDcoCapa);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << aid << " DCO capabilities object already created";
    }
}

void CardStateCollector::deleteCard(std::string boardAid)
{
    INFN_LOG(SeverityLevel::info) << "Called for AID: " << boardAid;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    if (NULL != mpCardState)
    {
        delete mpCardState;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << boardAid << " state object does not exist";
    }

    if (NULL != mpOecKeyMgmtState)
    {
        delete mpOecKeyMgmtState;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << boardAid << " state object does not exist";
    }

    if (NULL != mpCardFault)
    {
        delete mpCardFault;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << boardAid << " fault object does not exist";
    }

    if (NULL != mpDcoCapa)
    {
        delete mpDcoCapa;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Card " << boardAid << " DCO capabilities object does not exist";
    }
}

void CardStateCollector::startStatus(bool isHighPowerMode)
{
    mIsChm6HighPowerMode = isHighPowerMode;

    mThrState = boost::thread(boost::bind(
            &CardStateCollector::collectCardStatus, this
            ));

    mThrState.detach();

}

void CardStateCollector::startFault()
{
    mThrFaults = boost::thread(boost::bind(
            &CardStateCollector::collectCardFaults, this
            ));

    mThrFaults.detach();
}

void CardStateCollector::collectCardFaults()
{
    INFN_LOG(SeverityLevel::info) << "Start collection of Card Faults";

    bool isFirstConnect = true;
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mCardDataMtx);

            if (mpCardFault == NULL)
            {
                createCardFault();
                continue;   // wait for next iteration
            }

            bool isUpdate = false, isSecprocUpdate = false;

            DpAdapter::CardFault cardFault;
            hal_common::FaultType secprocFault;

            if ((true == mCollectFaultPollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)
                {
                    isUpdate = getCardFaults(cardFault);
                    //Check secproc faults - mIsConnectionUp ( conn up for DCO and secproc both)
                    isSecprocUpdate = checkSecprocCardFaults(secprocFault);
                }
                else
                {
                    INFN_LOG(SeverityLevel::debug) << "Connection to DCO is down. Skip collection";
                }
            }

            if (isFirstConnect && mIsConnectionUp)
            {
                isFirstConnect = false;
                isUpdate = true;    // make sure comm failure fault clears on first connect
            }

            bool isConnectFault;
            if (checkConnectionFaults(isFirstConnect, isConnectFault)
                || isUpdate || isSecprocUpdate)
            {
                updateCardFaults(cardFault, isConnectFault);
            }

            if( isPCPModified() )
            {
                updatePCPFaults();
                mPCPModified = false;
            }

        }

        boost::posix_time::seconds workTime(mCollectFaultInterval);

        boost::this_thread::sleep(workTime);
    }

    INFN_LOG(SeverityLevel::info) << "Finish collection of Card Faults";
}

void CardStateCollector::collectCardPm()
{
#if 0
    INFN_LOG(SeverityLevel::info) << "CardStateCollector::collectCardPm() started ...";
    boost::posix_time::seconds workTime(1);

    while (true)
    {
        getAndUpdateCardPm();

        boost::this_thread::sleep(workTime);
    }

    INFN_LOG(SeverityLevel::info) << "CardStateCollector::collectCardPm() finished ...";
#endif
}

void CardStateCollector::collectCardStatus()
{
    INFN_LOG(SeverityLevel::info) << "CardStateCollector::collectCardStatus started ...";

    bool enPrint = true;
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mCardDataMtx);

            if ((true == mCollectStatePollMode) &&
                (true == mIsCollectionEn))
            {
                if (mIsConnectionUp)    // if both dco connections up
                {
                    if (mIsChm6HighPowerMode)
                    {
                        getAndUpdateCardStatus();
                    }
                    else
                    {
                        checkCardStatusLowPower(enPrint);
                    }
                }
                else
                {
                    INFN_LOG(SeverityLevel::debug) << "Connection to DCO is down. Skip collection";

                    if (mIsChm6HighPowerMode)
                    {
                        mpDcoCapa->Clear();
                    }

                    enPrint = true;
                }
            }
        }

        boost::posix_time::seconds workTime(mCollectStateInterval);

        boost::this_thread::sleep(workTime);
    }

    INFN_LOG(SeverityLevel::info) << "CardStateCollector::collectCardStatus() finished ...";
}

void CardStateCollector::getAndUpdateCardStatus()
{
    // Get DCO Capabilities state
    if (mpDcoCapa== NULL)
    {
        INFN_LOG(SeverityLevel::info) << "Creating Card DCO capabilities state here";

        std::string chm6Aid = mpPbTranslator->getChassisSlotAid();

        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);
        mpDcoCapa = new chm6_dataplane::DcoCapabilityState();
        mpDcoCapa->mutable_base_state()->mutable_config_id()->set_value(chm6Aid);
        mpDcoCapa->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        mpDcoCapa->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        //Get DCO Capabilities
        DpAdapter::CardCapability cardCapa;
        if (mpDcoCardAd->getCardCapabilities(&cardCapa))
        {
            mpPbTranslator->capaAdToPb(cardCapa, *mpDcoCapa);
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "Error reading DCO card capabilities";
        }

        string dcoCapaData;
        MessageToJsonString(*mpDcoCapa, &dcoCapaData);
        INFN_LOG(SeverityLevel::info) << dcoCapaData;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpDcoCapa);
    }

    else
    {
        if (!(mpDcoCapa->has_hal()))
        {
            //Get DCO Capabilities
            DpAdapter::CardCapability cardCapa;
            if (mpDcoCardAd->getCardCapabilities(&cardCapa))
            {
                mpPbTranslator->capaAdToPb(cardCapa, *mpDcoCapa);
            }
            else
            {
                INFN_LOG(SeverityLevel::error) << "Error reading DCO card capabilities";
            }

            if (mpDcoCapa->has_hal())
            {
                // Retrieve timestamp
                struct timeval tv;
                gettimeofday(&tv, NULL);
                std::string chm6Aid = mpPbTranslator->getChassisSlotAid();
                mpDcoCapa->mutable_base_state()->mutable_config_id()->set_value(chm6Aid);
                mpDcoCapa->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
                mpDcoCapa->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

                string dcoCapaData;
                MessageToJsonString(*mpDcoCapa, &dcoCapaData);
                INFN_LOG(SeverityLevel::info) << dcoCapaData;

                DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*mpDcoCapa);
            }
        }
    }

    gearbox::GearBoxAdIf* adapter = gearbox::GearBoxAdapterSingleton::Instance();

    if (mpCardState == NULL)
    {
        // For stand alone ms
        INFN_LOG(SeverityLevel::info) << "Card Config not created yet. Creating Card State here";

        // todo: workaround - Do we need to wait for card creation first????

        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);
        mpCardState = new chm6_common::Chm6DcoCardState();
        mpCardState->mutable_base_state()->mutable_config_id()->set_value("Chm6Internal");
        mpCardState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        mpCardState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        //Get state from DcoSecProc
        (DpAdapter::DcoSecProcAdapter::GetInstance())->UpdateUpgradableDeviceList(*mpCardState);

        // Call singleton to gearbox and populate/add to map using mpCardState

        adapter->UpdateUpgradableDeviceList((*mpCardState));

        string stateData;
        MessageToJsonString(*mpCardState, &stateData);
        INFN_LOG(SeverityLevel::info) << stateData;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpCardState);

        return;
    }

    INFN_LOG(SeverityLevel::debug) << "Collecting state for Card";

    chm6_common::Chm6DcoCardState prevCardState(*mpCardState);

    DpAdapter::CardStatus cardStat;
    //Get state from DcoSecProc
    (DpAdapter::DcoSecProcAdapter::GetInstance())->UpdateUpgradableDeviceList(*mpCardState);

        if (mpDcoCardAd->getCardStatus(&cardStat))
        {
            if (mDcoState != cardStat.state)
            {
                INFN_LOG(SeverityLevel::info) << "DCO state changed from " << toString(mDcoState) << " to " << toString(cardStat.state);
                mDcoState = cardStat.state;
            }

            mpPbTranslator->statusAdToPb(cardStat, *mpCardState);

            if (NULL != mpOecKeyMgmtState)
            {

                Chm6KeyMgmtState prevOecKeyMgmtState(*mpOecKeyMgmtState);

                // Retrieve timestamp
                struct timeval tv;
                gettimeofday(&tv, NULL);

                mpOecKeyMgmtState->mutable_base_state()->mutable_config_id()->set_value(genKeyMgmtConfigId());
                mpOecKeyMgmtState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
                mpOecKeyMgmtState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

                if(mpOecKeyMgmtState->mutable_keymgmt_state()->mutable_isk_map())
                {
                    mpOecKeyMgmtState->mutable_keymgmt_state()->mutable_isk_map()->clear();
                }

                /* Update the ISK state info */
                for (auto& iter:cardStat.isk)
                {
                    infinera::hal::keymgmt::v2::ImageSigningKey isk;

                    /* Attributes commented out are currently not updated by DCO */

                    // isk.mutable_shelf_id()->set_value((iter.second).shelf_id);
                    // isk.mutable_slot_id()->set_value((iter.second).slot_id);
                    // isk.set_cpu(static_cast<::infinera::hal::keymgmt::v2::IskCpu>((iter.second).cpu));

                    isk.mutable_key_name()->set_value((iter.second).key_name);
                    isk.mutable_key_serial_number()->set_value((iter.second).key_serial_number);
                    isk.mutable_issuer_name()->set_value((iter.second).issuer_name);
                    isk.mutable_key_length()->set_value((iter.second).key_length);
                    isk.mutable_key_payload()->set_value((iter.second).key_payload);

                    // isk.mutable_krk_name()->set_value((iter.second).krk_name);

                    isk.set_is_key_in_use(static_cast<::infinera::wrappers::v1::Bool>((iter.second).is_key_in_use));
                    isk.set_is_key_verified(static_cast<::infinera::wrappers::v1::Bool>((iter.second).is_key_verified));

                    // isk.set_being_deleted(static_cast<::infinera::wrappers::v1::Bool>((iter.second).being_deleted));

                    isk.mutable_signature_payload()->set_value((iter.second).signature_payload);
                    isk.mutable_signature_hash_scheme()->set_value((iter.second).signature_hash_scheme);
                    isk.mutable_signature_gen_time()->set_value((iter.second).signature_gen_time);
                    isk.mutable_krk_name()->set_value((iter.second).signature_key_id);

                    // isk.mutable_signature_algorithm()->set_value((iter.second).signature_algorithm);

                    (*mpOecKeyMgmtState->mutable_keymgmt_state()->mutable_isk_map())[iter.first] = isk;
                }

                infinera::hal::keymgmt::v2::IdevIdCertificate idevid;
                idevid.mutable_cert()->set_value(cardStat.idevidCert);
                *mpOecKeyMgmtState->mutable_keymgmt_state()->mutable_idevid() = idevid;


                std::vector<std::string> keysToDelete;

                if(false == MessageDifferencer::Equals(prevOecKeyMgmtState.keymgmt_state(), *mpOecKeyMgmtState->mutable_keymgmt_state() ))
                {
                    if (  prevOecKeyMgmtState.keymgmt_state().isk_map().size() !=  mpOecKeyMgmtState->keymgmt_state().isk_map().size() )
                    {
                        for(auto const& entry: prevOecKeyMgmtState.keymgmt_state().isk_map())
                        {
                            auto new_isk_map =  mpOecKeyMgmtState->mutable_keymgmt_state()->mutable_isk_map();
                            if(new_isk_map)
                            {
                                if( new_isk_map->find( entry.first ) == new_isk_map->end() )
                                {
                                    //Entry deleted from map
                                    keysToDelete.push_back( entry.first );
                                }
                            }
                        }
                    }

                    INFN_LOG(SeverityLevel::info) << "New Oec KeyMgmt state update received. Sending to Redis";

                    for( auto key:keysToDelete )
                    {
                        INFN_LOG(SeverityLevel::info) << " Removing key : " << key << std::endl;

                        infinera::hal::keymgmt::v2::ImageSigningKey isk;

                        //Fill up NA data and send update
                        isk.set_cpu(hal_keymgmt::IskCpu::ISK_CPU_DCO);
                        isk.mutable_key_name()->set_value(key);
                        isk.mutable_key_serial_number()->set_value("NA");
                        isk.mutable_issuer_name()->set_value("NA");
                        isk.mutable_key_length()->set_value(1);
                        isk.mutable_key_payload()->set_value("NA");
                        isk.mutable_signature_hash_scheme()->set_value("NA");
                        isk.mutable_signature_algorithm()->set_value("NA");
                        isk.mutable_krk_name()->set_value("NA");
                        isk.mutable_signature_payload()->set_value("NA");
                        isk.mutable_signature_gen_time()->set_value("NA");
                        isk.set_being_deleted(BoolWrapper::BOOL_TRUE);
                        isk.set_is_key_in_use(BoolWrapper::BOOL_FALSE);
                        isk.set_is_key_verified(BoolWrapper::BOOL_FALSE);

                        (*mpOecKeyMgmtState->mutable_keymgmt_state()->mutable_isk_map())[key] = isk;
                    }

                    /* Update key management state to Redis */
                    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*mpOecKeyMgmtState);

                    //Remove the deleted key from mpOecKeyMgmtState
                    auto oecIskMap = (mpOecKeyMgmtState->mutable_keymgmt_state()->mutable_isk_map());
                    for( auto key:keysToDelete )
                    {
                        auto keyIter = oecIskMap->find(key);

                        if(keyIter != oecIskMap->end())
                        {
                            //Remove Key with NA values
                            oecIskMap->erase(keyIter);
                        }
                    }

                }//end message differencer diff

            }

        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "Error reading DCO card state";

            // Could be DCO connection failure
            // Set fan control flags to the below values in this case
            mpCardState->set_alt_temp_oorh(wrapper::BOOL_TRUE);
            mpCardState->set_alt_temp_oorl(wrapper::BOOL_FALSE);
        }


        // Force fan temperature flags through CLI
        if (wrapper::BOOL_UNSPECIFIED != mForceTempHiFanIncrease)
        {
            mpCardState->set_alt_temp_oorh(mForceTempHiFanIncrease);
        }

        if (wrapper::BOOL_UNSPECIFIED != mForceTempStableFanDecrease)
        {
            mpCardState->set_alt_temp_oorl(mForceTempStableFanDecrease);
        }

        if (mpPbTranslator->isPbChanged(prevCardState, *mpCardState))
        {
            string stateDataStr;
            MessageToJsonString(*mpCardState, &stateDataStr);
            INFN_LOG(SeverityLevel::info) << stateDataStr;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*mpCardState);
    }
}

void CardStateCollector::getAndUpdateCardPm()
{
    INFN_LOG(SeverityLevel::info) << "ToDo";
}

void CardStateCollector::createCardFault()
{
    INFN_LOG(SeverityLevel::info) << "Card Config not created yet. Creating Card Fault here";

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    mpCardFault = new chm6_common::Chm6DcoCardFault();
    mpCardFault->mutable_base_fault()->mutable_config_id()->set_value("Chm6Internal");
    mpCardFault->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    mpCardFault->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    string faultData;
    MessageToJsonString(*mpCardFault, &faultData);
    INFN_LOG(SeverityLevel::info) << faultData;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpCardFault);
    return;
}

// returns true if card faults have changed; false otherwise
bool CardStateCollector::getCardFaults(DpAdapter::CardFault& cardFault)
{
    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    std::string aid = mpCardFault->base_fault().config_id().value();
    INFN_LOG(SeverityLevel::debug) << "Collecting fault for Card: " << aid;

    mpCardFault->mutable_base_fault()->mutable_timestamp()->set_seconds(tv.tv_sec);
    mpCardFault->mutable_base_fault()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    // Get Card faults
    if (mpDcoCardAd->getCardFault(&cardFault))
    {
        if (mIsDcoFaultReportEn == false)
        {
            // Clear all DCO card faults
            cardFault.eqptBmp    = 0ULL;
            cardFault.dacEqptBmp = 0ULL;
            cardFault.psEqptBmp  = 0ULL;
            cardFault.postBmp    = 0ULL;
            cardFault.dacPostBmp = 0ULL;
            cardFault.psPostBmp  = 0ULL;
        }

        DpAdapter::CardFault cardFaultCurrent;
        cardFaultCurrent.eqptBmp = mCardEqptFaultBitmap;
        cardFaultCurrent.dacEqptBmp = mCardDacEqptFaultBitmap;
        cardFaultCurrent.psEqptBmp = mCardPsEqptFaultBitmap;
        cardFaultCurrent.postBmp = mCardPostFaultBitmap;
        cardFaultCurrent.dacPostBmp = mCardDacPostFaultBitmap;
        cardFaultCurrent.psPostBmp = mCardPsPostFaultBitmap;
        cardFaultCurrent.rebootState = mDcoCardRebootState;

        if (mpPbTranslator->isAdChanged(cardFaultCurrent, cardFault))
        {
            if (mIsDcoFaultReportEn == false)
            {
                // Clear all DCO card faults
                std::vector<DpAdapter::AdapterFault>& eqptFault = cardFault.eqpt;
                for ( auto vFault = eqptFault.begin();  vFault != eqptFault.end(); vFault++ )
                {
                    vFault->faulted = false;
                }

                std::vector<DpAdapter::AdapterFault>& dacEqptFault = cardFault.dacEqpt;
                for ( auto vFault = dacEqptFault.begin();  vFault != dacEqptFault.end(); vFault++ )
                {
                    vFault->faulted = false;
                }

                std::vector<DpAdapter::AdapterFault>& psEqptFault = cardFault.psEqpt;
                for ( auto vFault = psEqptFault.begin();  vFault != psEqptFault.end(); vFault++ )
                {
                    vFault->faulted = false;
                }

                std::vector<DpAdapter::AdapterFault>& postFault = cardFault.post;
                for ( auto vFault = postFault.begin();  vFault != postFault.end(); vFault++ )
                {
                    vFault->faulted = false;
                }

                std::vector<DpAdapter::AdapterFault>& dacPostFault = cardFault.dacPost;
                for ( auto vFault = dacPostFault.begin();  vFault != dacPostFault.end(); vFault++ )
                {
                    vFault->faulted = false;
                }

                std::vector<DpAdapter::AdapterFault>& psPostFault = cardFault.psPost;
                for ( auto vFault = psPostFault.begin();  vFault != psPostFault.end(); vFault++ )
                {
                    vFault->faulted = false;
                }

                INFN_LOG(SeverityLevel::info) << "mIsDcoFaultReportEn is false - disable DCO fault report and clear all DCO faults. ";
            }

            mCardEqptFaultBitmap = cardFault.eqptBmp;
            mCardDacEqptFaultBitmap = cardFault.dacEqptBmp;
            mCardPsEqptFaultBitmap = cardFault.psEqptBmp;
            mCardPostFaultBitmap = cardFault.postBmp;
            mCardDacPostFaultBitmap = cardFault.dacPostBmp;
            mCardPsPostFaultBitmap = cardFault.psPostBmp;
            mDcoCardRebootState  = cardFault.rebootState;

            INFN_LOG(SeverityLevel::info) << "Detected DCO card Faults Change: " << std::endl
                                << "mCardEqptFaultBitmap = 0x" << std::hex << mCardEqptFaultBitmap << std::endl
                                << "mCardDacEqptFaultBitmap = 0x" << mCardDacEqptFaultBitmap << std::endl
                                << "mCardPsEqptFaultBitmap = 0x" << mCardPsEqptFaultBitmap << std::endl
                                << "mCardPostFaultBitmap = 0x" << mCardPostFaultBitmap << std::endl
                                << "mCardDacPostFaultBitmap = 0x" << mCardDacPostFaultBitmap << std::endl
                                << "mCardPsPostFaultBitmap = 0x" << mCardPsPostFaultBitmap << std::dec << std::endl
                                << "mDcoCardRebootState = " << mDcoCardRebootState;

            return true;
        }
    }

    return false;
}

//Update local secproc fault cache
//Return true if secproc fault is changed
bool CardStateCollector::checkSecprocCardFaults(hal_common::FaultType& tmpFault)
{
    bool isSecprocUpdate = false;
    tmpFault = (DpAdapter::DcoSecProcAdapter::GetInstance())->GetSecprocBoardFaults();
    if(false == MessageDifferencer::Equals(tmpFault, mSecprocFault))
    {
        isSecprocUpdate = true;
        {
            mSecprocFault.CopyFrom(tmpFault);
            mIsSecprocFaultChanged = true;
        }
    }

    return isSecprocUpdate;

}
// Translates faults to proto defs and updates redis
void
CardStateCollector::updateCardFaults( DpAdapter::CardFault& cardFault,
                                      bool isConnectFault)
{
    mpPbTranslator->faultAdToPb( cardFault,
                                 *mpCardFault,
                                 isConnectFault );

    if(mIsSecprocFaultChanged)
    {
        google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* faultMap = mpCardFault->mutable_hal()->mutable_fault();

        for ( auto itFault = mSecprocFault.fault().begin();
          itFault != mSecprocFault.fault().end();
          itFault++ )
        {
            (*faultMap)[itFault->first] = itFault->second;
        }
        //Add faults to mpCardFault
        mIsSecprocFaultChanged = false;
    }

    string faultDataStr;
    MessageToJsonString(*mpCardFault, &faultDataStr);
    INFN_LOG(SeverityLevel::debug) << faultDataStr;

    DataPlaneMgrSingleton::Instance()
        ->DpMgrRedisObjectUpdate(*mpCardFault);
}

void
CardStateCollector::updatePCPFaults()
{
    mpPbTranslator->faultPCPAdToPb( *mpCardFault, mFaultCache );

    INFN_LOG(SeverityLevel::debug) << "PCP Fault update";

    string faultDataStr;
    MessageToJsonString(*mpCardFault, &faultDataStr);
    INFN_LOG(SeverityLevel::debug) << faultDataStr;

    DataPlaneMgrSingleton::Instance()
        ->DpMgrRedisObjectUpdate(*mpCardFault);
}


// returns true if fault state has changed; isFaultSet contains the fault state
bool CardStateCollector::checkConnectionFaults(bool isFirstConnect, bool& isFaultSet)
{
    bool newFaultState = !mIsConnectionUp;
    bool isFaultChg = false;

    if (mIsFirstFaultCheck || (mConnectFault != newFaultState))
    {
        // Raw Fault State Change
        mConnectFltChgTime = std::chrono::steady_clock::now();

        mConnectFault = newFaultState;

        if (!mIsFirstFaultCheck)
        {
            INFN_LOG(SeverityLevel::info) << "Raw Connection Fault State Change. isRawFaultSet: " << mConnectFault;
        }

        mIsFirstFaultCheck = false;
    }
    else if (mConnectFault != mConnectFaultLatch)
    {
        // Soaking Condition
        auto nowTime = std::chrono::steady_clock::now();

        auto elapSecsDur =
                std::chrono::duration_cast<chrono::seconds>(nowTime - mConnectFltChgTime);

        uint32 elapSecs = elapSecsDur.count();
        bool doPrint = ((elapSecs % 30) == 0);  // print every 30

        uint32 soakTimeClr = COMM_FAIL_SOAK_TIME_CLR;
        uint32 soakTimeSet;
        if (isFirstConnect)
        {
            soakTimeSet = COMM_FAIL_SOAK_TIME_SET_INIT;
        }
        else
        {
            soakTimeSet = COMM_FAIL_SOAK_TIME_SET;
        }

        if (mConnectFault)
        {
            // soaking set condition
            if (doPrint)
            {
                INFN_LOG(SeverityLevel::info) << "Soaking Connection Fault, Set Condition." " Soak Threshold (secs): "
                               << soakTimeSet << " Elapsed Secs: " << elapSecs;
            }

            if (elapSecs > soakTimeSet)
            {
                INFN_LOG(SeverityLevel::info) << "Connection Fault Soaking Done. Fault Set";

                mConnectFaultLatch = mConnectFault;
                isFaultChg = true;
            }
        }
        else
        {
            // soaking clear condition
            if (doPrint)
            {
                INFN_LOG(SeverityLevel::info) << "Soaking Connection Fault, Clear Condition." " Soak Threshold (secs): "
                               << soakTimeClr << " Elapsed Secs: " << elapSecs;
            }

            if (elapSecs > soakTimeClr)
            {
                INFN_LOG(SeverityLevel::info) << "Connection Fault Soaking Done. Fault Clear";

                mConnectFaultLatch = mConnectFault;
                isFaultChg = true;
            }
        }
    }

    isFaultSet = mConnectFaultLatch;
    return isFaultChg;
}

void CardStateCollector::dumpCacheState(std::ostream& os)
{
    os << "******* Card State *******" << endl << endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    if (mpCardState)
    {
        std::string data;
        MessageToJsonString(*mpCardState, &data);
        mpPbTranslator->dumpJsonFormat(data, os);
    }
    else
    {
        os << " Card not created yet" << endl;
    }

    os << endl << endl;
}

void CardStateCollector::dumpCacheDcoCapabilities(std::ostream& os)
{
    os << "******* Card DCO Capabilities State *******" << endl << endl;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    if (mpDcoCapa)
    {
        std::string data;
        MessageToJsonString(*mpDcoCapa, &data);
        mpPbTranslator->dumpJsonFormat(data, os);
    }
    else
    {
        os << " Card not created yet" << endl;
    }
}

void CardStateCollector::dumpCacheFault(std::ostream& os)
{
    os << "******* Card Fault *******" << endl << endl;

    if (mpCardFault)
    {
        std::string data;
        MessageToJsonString(*mpCardFault, &data);
        mpPbTranslator->dumpJsonFormat(data, os);
    }
    else
    {
        os << " Card not created yet" << endl;
    }
}

void CardStateCollector::dumpDcoState(std::ostream& os)
{
    os << "DCO State      : " << toString(mDcoState) << endl;
    os << "DCO State FORCE: " << toString(mForceDcoState) << endl;
}

void CardStateCollector::dumpPollInfo(std::ostream& os)
{
    os << "State Polling    : " << (true == mCollectStatePollMode ? "Enabled" : "Disabled") << endl;
    os << "State Interval   : " << mCollectStateInterval << endl;

    os << "Fault Polling    : " << (true == mCollectFaultPollMode ? "Enabled" : "Disabled") << endl;
    os << "Fault Interval   : " << mCollectFaultInterval << endl;

    os << "Collection Enable: " << (true == mIsCollectionEn ? "Enabled" : "Disabled") << endl << endl;

    os << "DCO Faults Report Enable: " << (true == mIsDcoFaultReportEn ? "Enabled" : "Disabled") << endl << endl;
}

void CardStateCollector::setIsDcoFaultReportEn(std::ostream& os, bool val)
{
    mIsDcoFaultReportEn = val;

    os << "DCO Faults Report Enable: " << (true == mIsDcoFaultReportEn ? "Enabled" : "Disabled") << endl << endl;
}

void CardStateCollector::forceFanTempFlags(uint32_t tempHi, uint32_t tempStable)
{
    switch(tempHi)
    {
        case 1:
            mForceTempHiFanIncrease = wrapper::BOOL_TRUE;
            break;
        case 2:
            mForceTempHiFanIncrease = wrapper::BOOL_FALSE;
            break;
        default:
            mForceTempHiFanIncrease = wrapper::BOOL_UNSPECIFIED;
            break;
    }

    switch(tempStable)
    {
        case 1:
            mForceTempStableFanDecrease = wrapper::BOOL_TRUE;
            break;
        case 2:
            mForceTempStableFanDecrease = wrapper::BOOL_FALSE;
            break;
        default:
            mForceTempStableFanDecrease = wrapper::BOOL_UNSPECIFIED;
            break;
    }

    INFN_LOG(SeverityLevel::info) << "CLI mForceTempHiFanIncrease set to: " << wrapper::Bool_Name(mForceTempHiFanIncrease)
                   << " mForceTempStableFanDecrease set to: " << wrapper::Bool_Name(mForceTempStableFanDecrease);
}

void CardStateCollector::dumpFanTempFlags(std::ostream& os)
{
    if (mpCardState)
    {
        os << "tempHiFanIncrease: " << wrapper::Bool_Name(mpCardState->alt_temp_oorh())
           << "\ttempStableFanDecrease: " << wrapper::Bool_Name(mpCardState->alt_temp_oorl()) << std::endl;
    }
    else
    {
        os << "mCardState not created yet" << std::endl;
    }

    os << "mForceTempHiFanIncrease: " << wrapper::Bool_Name(mForceTempHiFanIncrease)
       << "\tmForceTempStableFanDecrease: " <<  wrapper::Bool_Name(mForceTempStableFanDecrease) << std::endl;
}

std::string CardStateCollector::genKeyMgmtConfigId()
{
    return DataPlaneMgrSingleton::Instance()->getChassisIdStr() + "-"
            + dp_env::DpEnvSingleton::Instance()->getSlotNum() + "-"
            + "DCO";
}

std::string CardStateCollector::toString(const DcoState docState) const
{
    switch (docState)
    {
        case DCOSTATE_UNSPECIFIED: return ("UNSPECIFIED");
        case DCOSTATE_BRD_INIT:    return ("BRD_INIT");
        case DCOSTATE_DSP_CONFIG:  return ("DSP_CONFIG");
        case DCOSTATE_LOW_POWER:   return ("LOW_POWER");
        case DCOSTATE_POWER_UP:    return ("POWER_UP");
        case DCOSTATE_POWER_DOWN:  return ("POWER_DOWN");
        case DCOSTATE_FAULTED:     return ("FAULTED");
        default:                   return ("UNKNOWN");
    }
}

void CardStateCollector::setCardStateSyncReady(bool isSyncReady)
{
    INFN_LOG(SeverityLevel::info) << "isSyncReady: " << (uint32)isSyncReady;

    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    if (mpCardState)
    {
        //update state in case polling update is not running due to connectionup flag not been updated
        if(isSyncReady)
        {
            DpAdapter::CardStatus cardStat;
            if (mpDcoCardAd->getCardStatus(&cardStat, true))
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " force update state once";
                mpPbTranslator->statusAdToPb(cardStat, *mpCardState);
            }
        }

        mpCardState->set_sync_ready(isSyncReady ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);

        if (::dp_env::DpEnvSingleton::Instance()->isBlockSyncReady())
        {
            INFN_LOG(SeverityLevel::info) << "Blocking Sync Ready update to Redis due to Env Variable";

            return;
        }

        struct timeval tv;
        gettimeofday(&tv, NULL);

        mpCardState->mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        mpCardState->mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        string stateData;
        MessageToJsonString(*mpCardState, &stateData);
        INFN_LOG(SeverityLevel::info) << stateData;

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(*mpCardState);
    }
    else
    {
        // should only happen in test scenario where ForceSyncReady is used

        INFN_LOG(SeverityLevel::info) << "mpCardState is NULL. Taking no action";
    }
}

bool CardStateCollector::isDcoSyncReady()
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    DcoState dcoState = mDcoState;

    // Force through CLI backdoor
    if (DCOSTATE_UNSPECIFIED != mForceDcoState)
    {
        dcoState = mForceDcoState;
    }

    return mpPbTranslator->isDcoStateReady(dcoState);
}

bool CardStateCollector::isConnectFaultLatch()
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    return mConnectFaultLatch;
}

void CardStateCollector::setIsConnectionUp(bool val)
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    if (val != mIsConnectionUp)
    {
        mIsConnectionUp = val;
        INFN_LOG(SeverityLevel::info) << __func__ << " mIsConnectionUp is changed to: " << mIsConnectionUp;

        if (!mIsConnectionUp)
        {
            INFN_LOG(SeverityLevel::info) << "Connection Down. DcoState to DCOSTATE_UNSPECIFIED";

            mDcoState = DCOSTATE_UNSPECIFIED;
        }
    }
}

void
CardStateCollector::setLinkFaultCache( const std::string strLinkPort,
                                       const std::string strFaultLD,
                                       const bool bFaultValueLD,
                                       const std::string strFaultCRC,
                                       const bool bFaultValueCRC )
{
    std::lock_guard<std::mutex> lock(mCardDataMtx);

    // if key strLinkPort exists in map
    if( mFaultCache.find(strLinkPort) != mFaultCache.end() )
    {
        // Set bFaultValue for strFault in innermap
        auto& inmap = mFaultCache.at(strLinkPort);
        inmap[strFaultLD] = bFaultValueLD;
        inmap[strFaultCRC] = bFaultValueCRC;
    }
    else
    {
        // Create an innermap keyed by strFault with bFaultValue
        std::map<std::string, bool> innermap;
        innermap.insert({strFaultLD, bFaultValueLD});
        innermap.insert({strFaultCRC, bFaultValueCRC});

        // Insert innermap keyed by strLinkPort
        mFaultCache[strLinkPort] = innermap;
    }

    setPCPModified();
}

bool CardStateCollector::isPCPModified() const
{
    return mPCPModified;
}

void CardStateCollector::setPCPModified()
{
    mPCPModified = true;
}

void CardStateCollector::checkCardStatusLowPower(bool& enPrint)
{
    shared_ptr<FpgaMiscIf> spMiscIf = DataPlaneMgrSingleton::Instance()->getFpgaMiscIf();

    if (spMiscIf == nullptr)
    {
        INFN_LOG(SeverityLevel::error) << "MiscIf Pointer is Null. Cannot update FPGA Status Response";

        return;
    }

    try
    {
        uint32 regVal = spMiscIf->Read32(cFpgaFruStatus);
        if (regVal & cFpgaFruStatus_sled_pwr_stat_mask)
        {
            // Wait for DCO to reach Low Power State
            if (checkIsDcoLowPower())
            {
                uint32 prevVal;
                {
#ifndef ARCH_x86
                    ShmMutexGuard guard(mpShmMtxFcp);
#endif

                    prevVal = spMiscIf->Read32(cFpgaFruStatus);
                    regVal = prevVal & (~cFpgaFruStatus_sled_pwr_stat_mask);

                    spMiscIf->Write32(cFpgaFruStatus, regVal);
                }

                INFN_LOG(SeverityLevel::info) << "Clearing SLED_POWER_STATUS bit. Prev Val: 0x" <<
                        std::hex << prevVal << " New Val: 0x" << regVal;
            }
            else
            {
                if (enPrint)
                {
                    INFN_LOG(SeverityLevel::info) << "Dco has not yet reached low power state";
                }

                enPrint = false;
            }
        }
        else
        {
            if (enPrint)
            {
                INFN_LOG(SeverityLevel::info) << "FRU_STATUS reg bit SLED_POWER_STATUS is already cleared. No need to update it";
            }

            enPrint = false;
        }
    }
    catch (regIf::RegIfException &ex)
    {
        INFN_LOG(SeverityLevel::error) << "Error: Fpga Access Failed. Ex: " << ex.GetError();
    }
    catch(exception& ex)
    {
        INFN_LOG(SeverityLevel::error) << "Caught Exception: " << ex.what();
    }
    catch( ... )
    {
        INFN_LOG(SeverityLevel::error) << "Caught Exception";
    }
}

bool CardStateCollector::checkIsDcoLowPower()
{
    bool isLp = false;

    DpAdapter::CardStatus cardStat;
    if (mpDcoCardAd->getCardStatus(&cardStat, true))
    {
        if (mpPbTranslator->isDcoStateLowPower(cardStat.state))
        {
            INFN_LOG(SeverityLevel::info) << "Dco is Low Power";

            isLp = true;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "Dco card state: " << toString(cardStat.state);
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "Error reading DCO card state";
    }

    return isLp;
}

void CardStateCollector::initShmMutex()
{
#ifndef ARCH_x86
    if (0 != ShmMutex::Initialize())
    {
        INFN_LOG(SeverityLevel::error) << "Shared memory mutex init failed. Continuing without shm mutex protection";
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Shared memory mutex init successful!";

        mpShmMtxFcp = ShmMutex::Get((char *)"fcp");
        if (NULL == mpShmMtxFcp)
        {
            INFN_LOG(SeverityLevel::error) << "Shared memory mutex for FCP not found. Continuing without shm mutex protection";
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "Shared memory mutex for FCP created successfully";

        }
    }
#endif
}

} // namespace DataPlane
