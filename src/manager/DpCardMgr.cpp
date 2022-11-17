/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include <sstream>

#include "DpCardMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DcoSecProcCardCfgHdlr.h"
#include "DcoConnectHandler.h"
#include "DpPeerDiscoveryMgr.h"
#include "DataPlaneMgr.h"

using namespace ::std;
using google::protobuf::util::MessageToJsonString;
using namespace DpAdapter;

#define ILOG INFN_LOG(SeverityLevel::info)

namespace DataPlane
{

DpCardMgr::DpCardMgr()
    : DpObjectMgr("Card", ((1 << DCC_ZYNQ) | (1 << DCC_NXP)))
    , mpPbTranslator(NULL)
    , mpCardAd(NULL)
    , mpDcoSecProcAd(NULL)
    , mPowerMode(chm6_common::POWER_MODE_UNSPECIFIED)
{

}

DpCardMgr::~DpCardMgr()
{
    if (mpPbTranslator)
    {
        delete mpPbTranslator;
    }

    if (mpCardAd)
    {
        delete mpCardAd;
    }

    unregisterCardPm();
    unregisterGccPm();
}

void DpCardMgr::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Creating DcoCardAdapter";

    mpCardAd = new DpAdapter::DcoCardAdapter();

    createCardPbTranslator();
    createCardConfigHandler();
    createCardStateCollector();

    mIsInitDone = true;
}

void DpCardMgr::start()
{
    INFN_LOG(SeverityLevel::info) << "Initializing DcoCardAdapter";

    if (mPowerMode == chm6_common::POWER_MODE_LOW)
    {
        ILOG << "Power Mode is low. Starting with limited functionality";

        mspCardCollector->startStatus(false);

        return;
    }

    if (mpCardAd->initializeCard() == false)
    {
        INFN_LOG(SeverityLevel::error) << "DcoCardAdapter Initialize Failed";

        // what todo?
    }

    //TODO : When to initialize the DCO card adapter
    INFN_LOG(SeverityLevel::info) << __func__ << " Starting secproc adapter" << std::endl;
    (DpAdapter::DcoSecProcAdapter::GetInstance())->Start();

    // Do NOT report card pm ("dspTemperature", "moduleCaseTemperature", "picTemperature")
    // Report card pm ("dspTemperature", "moduleCaseTemperature", "picTemperature")
    registerCardPm();
    registerGccPm();

    mspCardCollector->startStatus(true);
}

void DpCardMgr::startFaultCollector()
{
    if (mPowerMode == chm6_common::POWER_MODE_LOW)
    {
        ILOG << "Power Mode is low. No Fault Collection. Aborting";

        return;
    }

    mspCardCollector->startFault();
}

void DpCardMgr::reSubscribe()
{
    if (mPowerMode == chm6_common::POWER_MODE_LOW)
    {
        ILOG << "Power Mode is low. No subscriptions. Aborting.";

        return;
    }

    INFN_LOG(SeverityLevel::info) << "";

    mpCardAd->subscribeStreams();
}


void DpCardMgr::connectionUpdate()
{
    INFN_LOG(SeverityLevel::info) << "Updating collector with new connection state isConnectionUp: " << mIsConnectionUp;

    mspCardCollector->setIsConnectionUp(mIsConnectionUp);

    if (mPowerMode != chm6_common::POWER_MODE_LOW)
    {
        (DpAdapter::DcoSecProcAdapter::GetInstance())->SetConnectionState(mIsConnectionUp);
    }

    if (mpCardAd)
    {
        mpCardAd->SetConnectionState(mIsConnectionUp);
    }
}

bool DpCardMgr::isSubscribeConnUp(DcoCpuConnectionsType connId)
{
    bool retVal = false;

    if ((DCC_ZYNQ == connId) && (mvIsConnectionUp[DCC_ZYNQ]))
    {
        retVal = true;
    }

    return retVal;
}

//@todo : Remove this fn
void DpCardMgr::DcoSecProcConnectionUpdate(bool isConnectUp)
{
#if 0
    INFN_LOG(SeverityLevel::info) << __func__ << " New connection state : " << isConnectUp;
    (DpAdapter::DcoSecProcAdapter::GetInstance())->SetConnectionState(isConnectUp);
#endif
}

void DpCardMgr::registerCardPm()
{
    INFN_LOG(SeverityLevel::info) << "Registering Card PM Callback";

	DpMsCardPm cardPm;
	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	DpmsCbHandler *cbHandler = new DpmsCardCbHandler();
	dpMsPmCallbacksSingleton->RegisterCallBack(CLASS_NAME(cardPm), cbHandler);
}

void DpCardMgr::unregisterCardPm()
{
    INFN_LOG(SeverityLevel::info) << "Unregistering Card PM Callback";

    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpMsCardPm cardPm;
    dpMsPmCallbacksSingleton->UnregisterCallBack(CLASS_NAME(cardPm));
}

void DpCardMgr::registerGccPm()
{
//TODO
#if 0
    INFN_LOG(SeverityLevel::info) << "Registering GCC Control PM Callback";

	DpMsCardPm cardPm;
	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	DpmsCbHandler *cbHandler = new DpmsCardCbHandler();
	dpMsPmCallbacksSingleton->RegisterCallBack(CLASS_NAME(cardPm), cbHandler);
#endif
}

void DpCardMgr::unregisterGccPm()
{
//TODO
#if 0
    INFN_LOG(SeverityLevel::info) << "Unregistering Card PM Callback";

    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpMsCardPm cardPm;
    dpMsPmCallbacksSingleton->UnregisterCallBack(CLASS_NAME(cardPm));
#endif
}

void DpCardMgr::createCardPbTranslator()
{
    INFN_LOG(SeverityLevel::info) << "";

    mpPbTranslator = new CardPbTranslator();
}

void DpCardMgr::createCardConfigHandler()
{
    INFN_LOG(SeverityLevel::info) << "";

    mspDcoSecProcCardCfgHndlr = std::unique_ptr<DcoSecProcCardCfgHdlr>(new DcoSecProcCardCfgHdlr());

    mspCardCfgHndlr = std::make_shared<CardConfigHandler>(mpPbTranslator, mpCardAd, mspDcoSecProcCardCfgHndlr);

    mspConfigHandler = mspCardCfgHndlr;
}

void DpCardMgr::createCardStateCollector()
{
    INFN_LOG(SeverityLevel::info) << "";

    mspCardCollector = std::make_shared<CardStateCollector>(mpPbTranslator, mpCardAd);

    mspStateCollector = mspCardCollector;
}

void
DpCardMgr::handleBCMSwitchFaultStateUpdate(
    chm6_pcp::Chm6BcmSwitchFault* pBCMSwitchFault)
{
    string stateData;
    MessageToJsonString(*pBCMSwitchFault, &stateData);
    ILOG << stateData;

    string str = pBCMSwitchFault->base_fault().config_id().value();
    auto found = str.find_last_of("t");
    const string strPort = str.substr(found + 1);

    if( "9" == strPort || "10" == strPort || "11" == strPort )
    {
        auto& m = pBCMSwitchFault->hal();

        const string keyLD("BCM_FAULT_LINK_DOWN");
        const string keyCRC("BCM_FAULT_CRC_ERROR");

        if( m.find(keyLD) != m.end() && m.find(keyCRC) != m.end() )
        {
            auto ldf = m.at(keyLD).fault_value();
            auto crcf = m.at(keyCRC).fault_value();
            const bool bldf = ( 1 == ldf ? true : false );
            const bool bcrc = ( 1 == crcf ? true : false );

            ILOG << "FaultId = " << strPort
                 << ", " << keyLD << " = " << bldf
                 << ", " << keyCRC << " = " << bcrc;

            // Translate and forward to CardStateCollector.
            //
            mspCardCollector->setLinkFaultCache( strPort,
                                                 keyLD, bldf,
                                                 keyCRC, bcrc);
        }
        else
        {
          ILOG << "Error: Invalid BCMSwitchFault Protobuf structure.";
        }
    }
    // Else Ignore
}


// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpCardMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    // Check for Card ..............................................................................
    chm6_common::Chm6DcoConfig *pCardCfg = dynamic_cast<chm6_common::Chm6DcoConfig*>(objMsg);
    if (pCardCfg)
    {
        if (!mIsInitDone || !DataPlaneMgrSingleton::Instance()->getIsInitDone())
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Card Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCardCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Chm6DcoConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        if(mspCardCfgHndlr)
            mspCardCfgHndlr->onCreate(pCardCfg);

        //Dco security processor card object create
        if(mspDcoSecProcCardCfgHndlr)
            mspDcoSecProcCardCfgHndlr->onCreate(pCardCfg);

        //Call peer discovery manager to store the icdp node info
        if(pCardCfg->has_icdp_node_info())
        {
            DpPeerDiscoveryMgrSingleton::Instance()->CreateIcdpNodeConfig(pCardCfg->icdp_node_info());
        }

        if(mspCardCollector)
            mspCardCollector->createCard(pCardCfg->base_state().config_id().value());

        mspCardCfgHndlr->createCacheObj(*pCardCfg);

        return true;
    }

    // Filter for Chm6BcmSwitchFault
    //
    auto* pBCMSwitchFault = dynamic_cast<chm6_pcp::Chm6BcmSwitchFault*>(objMsg);
    if (pBCMSwitchFault)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Bcm Config";
            return true;
        }

        ILOG << "Determined it is BcmSwitchFault Msg";
        handleBCMSwitchFaultStateUpdate(pBCMSwitchFault);
        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpCardMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    // Check for Card ..............................................................................
    chm6_common::Chm6DcoConfig *pCardCfg = dynamic_cast<chm6_common::Chm6DcoConfig*>(objMsg);
    if (pCardCfg)
    {
        if (!mIsInitDone || !DataPlaneMgrSingleton::Instance()->getIsInitDone())
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Card Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCardCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Chm6DcoConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspCardCfgHndlr->onModify(pCardCfg);
        
        //Dco security processor call on DCO card object onModify
        mspDcoSecProcCardCfgHndlr->onModify(pCardCfg);
        
        if(pCardCfg->has_icdp_node_info())
        {
            DpPeerDiscoveryMgrSingleton::Instance()->UpdateIcdpNodeConfig(pCardCfg->icdp_node_info());
        }

        mspCardCfgHndlr->updateCacheObj(*pCardCfg);

        return true;
    }

    // Filter for Chm6BcmSwitchFault
    //
    auto* pBCMSwitchFault = dynamic_cast<chm6_pcp::Chm6BcmSwitchFault*>(objMsg);
    if (pBCMSwitchFault)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Bcm Config";
            return true;
        }

        ILOG << "Determined it is BcmSwitchFault Msg";
        handleBCMSwitchFaultStateUpdate(pBCMSwitchFault);
        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpCardMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    // Check for Card ..............................................................................
    chm6_common::Chm6DcoConfig *pCardCfg = dynamic_cast<chm6_common::Chm6DcoConfig*>(objMsg);
    if (pCardCfg)
    {
        if (!mIsInitDone || !DataPlaneMgrSingleton::Instance()->getIsInitDone())
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Card Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCardCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Chm6DcoConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspCardCfgHndlr->onDelete(pCardCfg);
        
        //Dco security processor call on DCO card object onDelete
        mspDcoSecProcCardCfgHndlr->onDelete(pCardCfg);

        //Call peer discovery manager to store the icdp node info
        if(pCardCfg->has_icdp_node_info())
        {
            DpPeerDiscoveryMgrSingleton::Instance()->DeleteIcdpNodeConfig(pCardCfg->icdp_node_info());
        }

        mspCardCfgHndlr->deleteCacheObj(*pCardCfg);

       // mspCardCollector->deleteCard(pCardCfg->config().aid().value());
        return true;
    }
    return false;
}

bool DpCardMgr::checkOnResync(google::protobuf::Message* objMsg)
{
    // Check for Card ..............................................................................
    chm6_common::Chm6DcoConfig *pCardCfg = dynamic_cast<chm6_common::Chm6DcoConfig*>(objMsg);
    if (pCardCfg)
    {
        if (!mIsInitDone || !DataPlaneMgrSingleton::Instance()->getIsInitDone())
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Card Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCardCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is Chm6DcoConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspCardCfgHndlr->createCacheObj(*pCardCfg);

        mspCardCollector->createCard(pCardCfg->base_state().config_id().value());

        return true;
    }

    // Filter for Chm6BcmSwitchFault
    //
    auto* pBCMSwitchFault = dynamic_cast<chm6_pcp::Chm6BcmSwitchFault*>(objMsg);
    if (pBCMSwitchFault)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Bcm Config";
            return true;
        }

        ILOG << "Determined it is BcmSwitchFault Msg";
        handleBCMSwitchFaultStateUpdate(pBCMSwitchFault);
        return true;
    }

    return false;
}

bool DpCardMgr::isDcoSyncReady()
{
    return mspCardCollector->isDcoSyncReady();
}

bool DpCardMgr::isConnectFaultLatch()
{
    return mspCardCollector->isConnectFaultLatch();
}

void DpCardMgr::setCardEnable(bool isEnable)
{
    INFN_LOG(SeverityLevel::info) << "isEnable: " << isEnable;

#define EN_MAX_TRY 5

    uint32 count = 0;
    while (count++ < EN_MAX_TRY)
    {
        if (mpCardAd->setCardEnabled(isEnable) == false)
        {
            INFN_LOG(SeverityLevel::error)
                    << "setCardEnabled Failed. Count: " << count << " Max: " << EN_MAX_TRY;
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "setCardEnabled Success!";

            break;
        }

        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
}

void DpCardMgr::setCardStateSyncReady(bool isReady)
{
    INFN_LOG(SeverityLevel::info) << "isReady: " << isReady;

    mspCardCollector->setCardStateSyncReady(isReady);
}

bool DpCardMgr::isSubEnabled()
{
    return DcoCardAdapter::mCardFltSubEnable
      && DcoCardAdapter::mCardStateSubEnable
      && DcoCardAdapter::mCardIskSubEnable
      && DcoCardAdapter::mCardPmSubEnable;
}

void DpCardMgr::updateFaultCache()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " update card adapter notify cache";
    mpCardAd->updateNotifyCache();
}

void DpCardMgr::setLocalPowerMode(chm6_common::PowerMode pwrMode)
{
    ILOG << "pwrMode: " << chm6_common::PowerMode_Name(pwrMode);

    mPowerMode = pwrMode;
}

void DpCardMgr::updateDcoPowerMode(bool isHighPwr)
{
    INFN_LOG(SeverityLevel::info) << "isHighPower: " << isHighPwr;

    setCardEnable(isHighPwr);
}


} // namespace DataPlane

