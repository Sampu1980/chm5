/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DpCarrierMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DpPmHandler.h"
#include "DataPlaneMgr.h"
#include "DpPeerDiscoveryMgr.h"
#include "DcoConnectHandler.h"

using namespace ::std;
using chm6_dataplane::Chm6ScgConfig;
using chm6_dataplane::Chm6CarrierConfig;
using chm6_dataplane::Chm6CarrierState;
using google::protobuf::util::MessageToJsonString;
using namespace DpAdapter;

namespace DataPlane
{

DpCarrierMgr::DpCarrierMgr()
    : DpObjectMgr("Carrier", ((1 << DCC_ZYNQ) | (1 << DCC_NXP)))
    , mpPbTranslator(NULL)
    , mpCarrierAd(NULL)
    , mpSecProcCarrierAd(NULL)
    , mIsConfigScg(false)
    , mspFpgaSramIf(NULL)
{

}

DpCarrierMgr::~DpCarrierMgr()
{
    if (mpPbTranslator)
    {
        delete mpPbTranslator;
    }
    if (mpCarrierAd)
    {
        delete mpCarrierAd;
    }
    //Delete DCO secproc carrier adapter
    if(mpSecProcCarrierAd)
    {
        delete mpSecProcCarrierAd;
    }
    unregisterCarrierPm();
}

void DpCarrierMgr::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Creating DcoCarrierAdapter";

    mpCarrierAd = new DpAdapter::DcoCarrierAdapter();
    mpSecProcCarrierAd = new DpAdapter::DcoSecProcCarrierAdapter();
    mspFpgaSramIf = DataPlaneMgrSingleton::Instance()->getFpgaSramIf();

    createCarrierPbTranslator();
    CreateSecProcCarrierCfgHdlr();
    createCarrierConfigHandler();
    createCarrierStateCollector();

    mIsInitDone = true;
}

void DpCarrierMgr::start()
{
    INFN_LOG(SeverityLevel::info) << "Initializing DcoCarrierAdapter";

    if (mpCarrierAd->initializeCarrier() == false)
    {
        INFN_LOG(SeverityLevel::error) << "DcoCarrierAdapter Initialize Failed";

        // what todo?
    }

    registerCarrierPm();

    mspCarrierCollector->start();
}

void DpCarrierMgr::connectionUpdate()
{
    INFN_LOG(SeverityLevel::info) << "Updating collector with new connection state isConnectionUp: " << mIsConnectionUp;

    mspCarrierCollector->setIsConnectionUp(mIsConnectionUp);
}

bool DpCarrierMgr::isSubscribeConnUp(DcoCpuConnectionsType connId)
{
    bool retVal = false;

    if ((DCC_ZYNQ == connId) && (mvIsConnectionUp[DCC_ZYNQ]))
    {
        retVal = true;
    }

    return retVal;
}

void DpCarrierMgr::reSubscribe()
{
    INFN_LOG(SeverityLevel::info) << "";

    mpCarrierAd->subscribeStreams();
}

void DpCarrierMgr::registerCarrierPm()
{
    INFN_LOG(SeverityLevel::info) << "Registering Carrier PM Callback";

    DpMsCarrierPMs carrierPm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpmsCbHandler *cbHandler = new DpmsCarrierCbHandler();
    dpMsPmCallbacksSingleton->RegisterCallBack(CLASS_NAME(carrierPm), cbHandler);
}

void DpCarrierMgr::unregisterCarrierPm()
{
    INFN_LOG(SeverityLevel::info) << "Unregistering Carrier PM Callback";

    DpMsCarrierPMs carrierPm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    dpMsPmCallbacksSingleton->UnregisterCallBack(CLASS_NAME(carrierPm));
}

void DpCarrierMgr::createCarrierPbTranslator()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mpPbTranslator = new CarrierPbTranslator();
}

void DpCarrierMgr::CreateSecProcCarrierCfgHdlr()
{
    mspSecProcCarrierCfgHdlr = std::make_shared<DcoSecProcCarrierCfgHdlr>(mpSecProcCarrierAd);
    if(NULL == mspSecProcCarrierCfgHdlr)
    {
        INFN_LOG(SeverityLevel::error) << "Dco secproc carrier config handler not created";
    }
}

void DpCarrierMgr::createCarrierConfigHandler()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspCarrierCfgHndlr = std::make_shared<CarrierConfigHandler>(mpPbTranslator, mpCarrierAd, mspSecProcCarrierCfgHdlr);

    mspConfigHandler = mspCarrierCfgHndlr;
}

void DpCarrierMgr::createCarrierStateCollector()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspCarrierCollector = std::make_shared<CarrierStateCollector>(mpPbTranslator, mpCarrierAd, mspFpgaSramIf);

    mspStateCollector = mspCarrierCollector;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpCarrierMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    // Check for Scg ..............................................................................
    Chm6ScgConfig *pScgCfg = dynamic_cast<Chm6ScgConfig*>(objMsg);
    if (pScgCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Scg Config";
            return true;
        }

        ConfigStatus status = ConfigStatus::SUCCESS;

        mspCarrierCfgHndlr->sendTransactionStateToInProgress(pScgCfg);

        string data;
        MessageToJsonString(*pScgCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is ScgConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mIsConfigScg = true;
        handleConfig(objMsg, pScgCfg->base_config().config_id().value(), ConfigType::CREATE);

        return true;
    }

    // Check for Carrier ..............................................................................
    Chm6CarrierConfig *pCarCfg = dynamic_cast<Chm6CarrierConfig*>(objMsg);
    if (pCarCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Carrier Config";
            return true;
        }

        mspCarrierCfgHndlr->sendTransactionStateToInProgress(pCarCfg);

        string data;
        MessageToJsonString(*pCarCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is CarrierConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;
        
        mIsConfigScg = false;
        handleConfig(objMsg, pCarCfg->base_config().config_id().value(), ConfigType::CREATE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpCarrierMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    // Check for Scg ..............................................................................
    Chm6ScgConfig *pScgCfg = dynamic_cast<Chm6ScgConfig*>(objMsg);
    if (pScgCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Scg Config";
            return true;
        }

        STATUS status = STATUS::STATUS_SUCCESS;

        //mspCarrierCfgHndlr->sendTransactionStateToInProgress(pScgCfg);

        string data;
        MessageToJsonString(*pScgCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is ScgConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspCarrierCfgHndlr->sendTransactionStateToComplete(pScgCfg, status);

        // todo: add scg config caching

        return true;
    }

    // Check for Carrier ..............................................................................
    Chm6CarrierConfig *pCarCfg = dynamic_cast<Chm6CarrierConfig*>(objMsg);
    if (pCarCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Carrier Config";
            return true;
        }

        mspCarrierCfgHndlr->sendTransactionStateToInProgress(pCarCfg);

        string data;
        MessageToJsonString(*pCarCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is CarrierConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mIsConfigScg = false;
        handleConfig(objMsg, pCarCfg->base_config().config_id().value(), ConfigType::MODIFY);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpCarrierMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    // Check for Scg ..............................................................................
    Chm6ScgConfig *pScgCfg = dynamic_cast<Chm6ScgConfig*>(objMsg);
    if (pScgCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Scg Config";
            return true;
        }

        STATUS status = STATUS::STATUS_SUCCESS;

        //mspCarrierCfgHndlr->sendTransactionStateToInProgress(pScgCfg);

        string data;
        MessageToJsonString(*pScgCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is ScgConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspCarrierCfgHndlr->sendTransactionStateToComplete(pScgCfg, status);

        return true;
    }

    // Check for Carrier ..............................................................................
    Chm6CarrierConfig *pCarCfg = dynamic_cast<Chm6CarrierConfig*>(objMsg);
    if (pCarCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Carrier Config";
            return true;
        }

        STATUS status = STATUS::STATUS_SUCCESS;

        mspCarrierCfgHndlr->sendTransactionStateToInProgress(pCarCfg);

        string data;
        MessageToJsonString(*pCarCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is CarrierConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mIsConfigScg = false;
        handleConfig(objMsg, pCarCfg->base_config().config_id().value(), ConfigType::DELETE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpCarrierMgr::checkOnResync(google::protobuf::Message* objMsg)
{
    // Check for Carrier ..............................................................................
    Chm6CarrierConfig *pCarCfg = dynamic_cast<Chm6CarrierConfig*>(objMsg);
    if (pCarCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Carrier Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCarCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is CarrierConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspCarrierCfgHndlr->createCacheObj(*pCarCfg);

        mspCarrierCollector->createCarrier(*pCarCfg);

        return true;
    }

    return false;
}

ConfigStatus DpCarrierMgr::processConfig(google::protobuf::Message* objMsg, ConfigType cfgType)
{
    if (mIsConfigScg)
    {
        return processConfigScg(objMsg, cfgType);
    }

    Chm6CarrierConfig *pCarCfg = static_cast<Chm6CarrierConfig*>(objMsg);

    ConfigStatus status = ConfigStatus::SUCCESS;

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspCarrierCollector->createFaultPmPlaceHolder(pCarCfg->base_config().config_id().value());

            status = mspCarrierCfgHndlr->onCreate(pCarCfg);

            //Calling create to DCO security processor
            //@todo - add status tracking
            if(mspSecProcCarrierCfgHdlr)
            {
                mspSecProcCarrierCfgHdlr->onCreate(pCarCfg);
            }
            break;

        case ConfigType::MODIFY:
            status = mspCarrierCfgHndlr->onModify(pCarCfg);
            break;

        case ConfigType::DELETE:
            status = mspCarrierCfgHndlr->onDelete(pCarCfg);

            //Carrier is deleted in DCO Zynq
            //Call peer discovery manager to notify carrier creation
            DpPeerDiscoveryMgrSingleton::Instance()->NotifyCarrierDelete(pCarCfg->base_config().config_id().value());

            //Calling delete on DCO security processor
            if(mspSecProcCarrierCfgHdlr)
            {
                mspSecProcCarrierCfgHdlr->onDelete(pCarCfg);
            }
            break;
    }

    return status;
}

ConfigStatus DpCarrierMgr::completeConfig(google::protobuf::Message* objMsg,
                                 ConfigType cfgType,
                                 ConfigStatus status,
                                 std::ostringstream& log)
{
    if (mIsConfigScg)
    {
        return completeConfigScg(objMsg, cfgType, status, log);
    }

    INFN_LOG(SeverityLevel::info) << "cfgType " << toString(cfgType);

    Chm6CarrierConfig *pCarCfg = static_cast<Chm6CarrierConfig*>(objMsg);

    STATUS transactionStat = STATUS::STATUS_SUCCESS;
    if (ConfigStatus::SUCCESS != status)
    {
        transactionStat = STATUS::STATUS_FAIL;

        INFN_LOG(SeverityLevel::info) << "Fall-back failure handling: Caching Config although it has failed.";

        switch(cfgType)
        {
            case ConfigType::CREATE:
                mspCarrierCfgHndlr->createCacheObj(*pCarCfg);
                break;

            case ConfigType::MODIFY:
                mspCarrierCfgHndlr->updateCacheObj(*pCarCfg);
                break;

            case ConfigType::DELETE:
                STATUS tStat = mspCarrierCfgHndlr->deleteCarrierConfigCleanup(pCarCfg);
                if (STATUS::STATUS_FAIL == tStat)
                {
                    INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCarCfg->base_config().config_id().value();
    
                }
                break;
        }

        mspCarrierCfgHndlr->setTransactionErrorString(log.str());
        mspCarrierCfgHndlr->sendTransactionStateToComplete(pCarCfg, transactionStat);
        return ConfigStatus::SUCCESS;
    }

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspCarrierCfgHndlr->createCacheObj(*pCarCfg);

            //Carrier is created in DCO Zynq
            //Call peer discovery manager to notify carrier creation
            DpPeerDiscoveryMgrSingleton::Instance()->NotifyCarrierCreate(pCarCfg->base_config().config_id().value());

            mspCarrierCollector->createCarrier(*pCarCfg);

            break;

        case ConfigType::MODIFY:
            //@todo : Do we need to notify modifications in carrier to DCO Secproc?

            mspCarrierCfgHndlr->updateCacheObj(*pCarCfg);

            mspCarrierCollector->updateInstalledConfig(*pCarCfg);

            break;

        case ConfigType::DELETE:
            mspCarrierCfgHndlr->deleteCacheObj(*pCarCfg);

            mspCarrierCollector->deleteCarrier(pCarCfg->base_config().config_id().value());

            mspCarrierCfgHndlr->sendTransactionStateToComplete(pCarCfg, transactionStat);

            STATUS tStat = mspCarrierCfgHndlr->deleteCarrierConfigCleanup(pCarCfg);
            if (STATUS::STATUS_FAIL == tStat)
            {
                INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCarCfg->base_config().config_id().value();

                return ConfigStatus::FAIL_OTHER;
            }

            // return here; transaction response already sent above in this case
            return ConfigStatus::SUCCESS;

            break;
    }

    mspCarrierCfgHndlr->sendTransactionStateToComplete(pCarCfg, transactionStat);

    return ConfigStatus::SUCCESS;
}


ConfigStatus DpCarrierMgr::processConfigScg(google::protobuf::Message* objMsg, ConfigType cfgType)
{
    Chm6ScgConfig *pCfg = static_cast<Chm6ScgConfig*>(objMsg);

    ConfigStatus status = ConfigStatus::SUCCESS;

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspCarrierCollector->createFaultPmPlaceHolderScg(pCfg->base_config().config_id().value());

            status = mspCarrierCfgHndlr->onCreate(pCfg);

            break;

        case ConfigType::MODIFY:
            //do nothing
            break;

        case ConfigType::DELETE:
            //do nothing
            break;
    }

    return status;
}

ConfigStatus DpCarrierMgr::completeConfigScg(google::protobuf::Message* objMsg,
                                 ConfigType cfgType,
                                 ConfigStatus status,
                                 std::ostringstream& log)
{
    INFN_LOG(SeverityLevel::info) << "cfgType " << toString(cfgType);

    Chm6ScgConfig *pCarCfg = static_cast<Chm6ScgConfig*>(objMsg);

    STATUS transactionStat = STATUS::STATUS_SUCCESS;
    if (ConfigStatus::SUCCESS != status)
    {
        transactionStat = STATUS::STATUS_FAIL;

        mspCarrierCfgHndlr->setTransactionErrorString(log.str());
    }

    mspCarrierCfgHndlr->sendTransactionStateToComplete(pCarCfg, transactionStat);

    return ConfigStatus::SUCCESS;
}

bool DpCarrierMgr::isSubEnabled()
{
    return DcoCarrierAdapter::mCarrierFltSubEnable && DcoCarrierAdapter::mCarrierPmSubEnable && DcoCarrierAdapter::mCarrierStateSubEnable;
}

void DpCarrierMgr::updateFaultCache()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " update carrier adapter notify cache";
    mpCarrierAd->updateNotifyCache();
}

} // namespace DataPlane

