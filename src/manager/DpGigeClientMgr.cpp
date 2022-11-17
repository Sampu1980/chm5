/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DpGigeClientMgr.h"
#include "GearBoxCreator.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DpPmHandler.h"
#include "DataPlaneMgr.h"
#include "SerdesTunerCreator.h"
#include "DcoConnectHandler.h"

using namespace ::std;
using chm6_dataplane::Chm6GigeClientConfig;
using chm6_dataplane::Chm6GigeClientState;
using namespace DpAdapter;

using google::protobuf::util::MessageToJsonString;


namespace DataPlane
{

DpGigeClientMgr::DpGigeClientMgr()
    : DpObjectMgr("GigeClient", (1 << DCC_ZYNQ))
    , mpPbTranslator(NULL)
    , mpGigeClientAd(NULL)
    , mpGearBoxAd(NULL)
    , mpSerdesTuner(NULL)
{

}

DpGigeClientMgr::~DpGigeClientMgr()
{
    if (mpPbTranslator)
    {
        delete mpPbTranslator;
    }
    if (mpGigeClientAd)
    {
        delete mpGigeClientAd;
    }

    unregisterGigeClientPm();
}

void DpGigeClientMgr::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Initialize GearBox and Serdes Tuner pointers";

    mpGearBoxAd = DataPlaneMgrSingleton::Instance()->getGearBoxAd();
    mpSerdesTuner = DataPlaneMgrSingleton::Instance()->getSerdesTuner();

    INFN_LOG(SeverityLevel::info) << "Creating DcoGigeClientAdapter";

    mpGigeClientAd = new DpAdapter::DcoGigeClientAdapter();

    createGigeClientPbTranslator();
    createGigeClientConfigHandler();
    createGigeClientStateCollector();

    mIsInitDone = true;
}

void DpGigeClientMgr::start()
{
    INFN_LOG(SeverityLevel::info) << "Initializing DcoGigeClientAdapter";

    if (mpGigeClientAd->initializeGigeClient() == false)
    {
        INFN_LOG(SeverityLevel::error) << "DcoGigeClientAdapter Initialize Failed";

        // what todo?
    }

    registerGigeClientPm();

    mspGigeClientCollector->start();
}

void DpGigeClientMgr::reSubscribe()
{
    INFN_LOG(SeverityLevel::info) << "";

    mpGigeClientAd->subscribeStreams();
}

void DpGigeClientMgr::connectionUpdate()
{
    INFN_LOG(SeverityLevel::info) << "Updating collector with new connection state isConnectionUp: " << mIsConnectionUp;

    mspGigeClientCollector->setIsConnectionUp(mIsConnectionUp);
}


void DpGigeClientMgr::registerGigeClientPm()
{
    INFN_LOG(SeverityLevel::info) << "Registering GigeClient PM Callback";

    DpMsClientGigePms gigePm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpmsCbHandler *cbHandler = new DpmsGigeCbHandler();
    dpMsPmCallbacksSingleton->RegisterCallBack(CLASS_NAME(gigePm), cbHandler);
}

void DpGigeClientMgr::unregisterGigeClientPm()
{
    INFN_LOG(SeverityLevel::info) << "Unregistering GigeClient PM Callback";

    DpMsClientGigePms gigePm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    dpMsPmCallbacksSingleton->UnregisterCallBack(CLASS_NAME(gigePm));
}

void DpGigeClientMgr::createGigeClientPbTranslator()
{
    INFN_LOG(SeverityLevel::info) << "";

    mpPbTranslator = new GigeClientPbTranslator();
}

void DpGigeClientMgr::createGigeClientConfigHandler()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspGigeClientCfgHndlr = std::make_shared<GigeClientConfigHandler>(mpPbTranslator, mpGigeClientAd, mpGearBoxAd, mpSerdesTuner);

    mspConfigHandler = mspGigeClientCfgHndlr;
}

void DpGigeClientMgr::createGigeClientStateCollector()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspGigeClientCollector = std::make_shared<GigeClientStateCollector>(mpPbTranslator, mpGigeClientAd, mpGearBoxAd);

    mspStateCollector = mspGigeClientCollector;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpGigeClientMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    // Check for GigeClient ..............................................................................
    Chm6GigeClientConfig *pCfg = dynamic_cast<Chm6GigeClientConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for GigeClient Config";
            return true;
        }

        mspGigeClientCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is GigeClientConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::CREATE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpGigeClientMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    // Check for GigeClient ..............................................................................
    Chm6GigeClientConfig *pCfg = dynamic_cast<Chm6GigeClientConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for GigeClient Config";
            return true;
        }

        mspGigeClientCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is GigeClientConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::MODIFY);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpGigeClientMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    // Check for GigeClient ..............................................................................
    Chm6GigeClientConfig *pCfg = dynamic_cast<Chm6GigeClientConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for GigeClient Config";
            return true;
        }

        mspGigeClientCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is GigeClientConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::DELETE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpGigeClientMgr::checkOnResync(google::protobuf::Message* objMsg)
{
    // Check for GigeClient ..............................................................................
    Chm6GigeClientConfig *pCfg = dynamic_cast<Chm6GigeClientConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for GigeClient Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is GigeClientConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspGigeClientCfgHndlr->createCacheObj(*pCfg);

        mspGigeClientCollector->createGigeClient(*pCfg);

        return true;
    }

    return false;
}

ConfigStatus DpGigeClientMgr::processConfig(google::protobuf::Message* objMsg, ConfigType cfgType)
{
    // Check for Carrier ..............................................................................
    Chm6GigeClientConfig *pCfg = static_cast<Chm6GigeClientConfig*>(objMsg);

    ConfigStatus status = ConfigStatus::SUCCESS;

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspGigeClientCollector->createFaultPmPlaceHolder(pCfg->base_config().config_id().value());

            status = mspGigeClientCfgHndlr->onCreate(pCfg);

            break;

        case ConfigType::MODIFY:
            status = mspGigeClientCfgHndlr->onModify(pCfg);
            break;

        case ConfigType::DELETE:
            status = mspGigeClientCfgHndlr->onDelete(pCfg);
            break;
    }

    return status;
}

ConfigStatus DpGigeClientMgr::completeConfig(google::protobuf::Message* objMsg,
                                 ConfigType cfgType,
                                 ConfigStatus status,
                                 std::ostringstream& log)
{
    INFN_LOG(SeverityLevel::info) << "cfgType " << toString(cfgType);

    Chm6GigeClientConfig *pCfg = static_cast<Chm6GigeClientConfig*>(objMsg);

    STATUS transactionStat = STATUS::STATUS_SUCCESS;
    if (ConfigStatus::SUCCESS != status)
    {
        transactionStat = STATUS::STATUS_FAIL;

        INFN_LOG(SeverityLevel::info) << "Fall-back failure handling: Caching Config although it has failed.";

        switch(cfgType)
        {
            case ConfigType::CREATE:
                mspGigeClientCfgHndlr->createCacheObj(*pCfg);
                break;

            case ConfigType::MODIFY:
                mspGigeClientCfgHndlr->updateCacheObj(*pCfg);
                break;

            case ConfigType::DELETE:
                STATUS tStat = mspGigeClientCfgHndlr->deleteGigeClientConfigCleanup(pCfg);
                if (STATUS::STATUS_FAIL == tStat)
                {
                    INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();
                }
                break;
        }

        mspGigeClientCfgHndlr->setTransactionErrorString(log.str());
        mspGigeClientCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);
        return ConfigStatus::SUCCESS;
    }

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspGigeClientCfgHndlr->createCacheObj(*pCfg);

            mspGigeClientCollector->createGigeClient(*pCfg);

            break;

        case ConfigType::MODIFY:
            //@todo : Do we need to notify modifications in carrier to DCO Secproc?

            mspGigeClientCfgHndlr->updateCacheObj(*pCfg);

            mspGigeClientCollector->updateInstalledConfig(*pCfg);

            break;

        case ConfigType::DELETE:
            mspGigeClientCollector->deleteGigeClient(pCfg->base_config().config_id().value());

            mspGigeClientCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

            STATUS tStat = mspGigeClientCfgHndlr->deleteGigeClientConfigCleanup(pCfg);
            if (STATUS::STATUS_FAIL == tStat)
            {
                INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();

                return ConfigStatus::FAIL_OTHER;
            }

            mspGigeClientCfgHndlr->deleteCacheObj(*pCfg);

            // return here; transaction response already sent above in this case
            return ConfigStatus::SUCCESS;

            break;
    }

    mspGigeClientCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

    return ConfigStatus::SUCCESS;
}

bool DpGigeClientMgr::isSubEnabled()
{
    return DcoGigeClientAdapter::mGigePmSubEnable && DcoGigeClientAdapter::mGigeFltSubEnable;
}

void DpGigeClientMgr::updateFaultCache()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " update gige adapter notify cache";
    mpGigeClientAd->updateNotifyCache();
}

} // namespace DataPlane
