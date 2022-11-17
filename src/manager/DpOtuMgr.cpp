/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DpOtuMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DpPmHandler.h"
#include "DataPlaneMgr.h"
#include "DcoConnectHandler.h"

using namespace ::std;
using chm6_dataplane::Chm6OtuConfig;
using chm6_dataplane::Chm6OtuState;
using google::protobuf::util::MessageToJsonString;
using namespace DpAdapter;

namespace DataPlane
{

DpOtuMgr::DpOtuMgr()
    : DpObjectMgr("Otu", (1 << DCC_ZYNQ))
    , mpPbTranslator(NULL)
    , mpOtuAd(NULL)
    , mpGearBoxAd(NULL)
    , mpSerdesTuner(NULL)
    , mspFpgaSramIf(NULL)
{

}

DpOtuMgr::~DpOtuMgr()
{
    if (mpPbTranslator)
    {
        delete mpPbTranslator;
    }
    if (mpOtuAd)
    {
        delete mpOtuAd;
    }

    unregisterOtuPm();
}

void DpOtuMgr::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Initialize GearBox and Serdes Tuner pointers";

    mpGearBoxAd = DataPlaneMgrSingleton::Instance()->getGearBoxAd();
    mpSerdesTuner = DataPlaneMgrSingleton::Instance()->getSerdesTuner();
    mspFpgaSramIf = DataPlaneMgrSingleton::Instance()->getFpgaSramIf();

    INFN_LOG(SeverityLevel::info) << "Creating DcoOtuAdapter";

    mpOtuAd = new DpAdapter::DcoOtuAdapter();

    createOtuPbTranslator();
    createOtuConfigHandler();
    createOtuStateCollector();

    mIsInitDone = true;
}

void DpOtuMgr::start()
{
    INFN_LOG(SeverityLevel::info) << "Initialize DcoOtuAdapter";

    if (mpOtuAd->initializeOtu() == false)
    {
        INFN_LOG(SeverityLevel::error) << "DcoOtuAdapter Initialize Failed";

        // what todo?
    }

    registerOtuPm();

    mspOtuCollector->start();
}

void DpOtuMgr::reSubscribe()
{
    INFN_LOG(SeverityLevel::info) << "";

    mpOtuAd->subscribeStreams();
}


void DpOtuMgr::connectionUpdate()
{
    INFN_LOG(SeverityLevel::info) << "Updating collector with new connection state isConnectionUp: " << mIsConnectionUp;

    mspOtuCollector->setIsConnectionUp(mIsConnectionUp);
}

void DpOtuMgr::registerOtuPm()
{
    INFN_LOG(SeverityLevel::info) << "Registering Otu PM Callback";

    DpMsOTUPm otuPm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpmsCbHandler *cbHandler = new DpmsOtuCbHandler();
    dpMsPmCallbacksSingleton->RegisterCallBack(CLASS_NAME(otuPm), cbHandler);
}

void DpOtuMgr::unregisterOtuPm()
{
    INFN_LOG(SeverityLevel::info) << "Unregistering Otu PM Callback";

    DpMsOTUPm otuPm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    dpMsPmCallbacksSingleton->UnregisterCallBack(CLASS_NAME(otuPm));
}

void DpOtuMgr::createOtuPbTranslator()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mpPbTranslator = new OtuPbTranslator();
}

void DpOtuMgr::createOtuConfigHandler()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspOtuCfgHndlr = std::make_shared<OtuConfigHandler>(mpPbTranslator, mpOtuAd, mpGearBoxAd, mpSerdesTuner);

    mspConfigHandler = mspOtuCfgHndlr;
}

void DpOtuMgr::createOtuStateCollector()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspOtuCollector = std::make_shared<OtuStateCollector>(mpPbTranslator, mpOtuAd, mspFpgaSramIf);

    mspStateCollector = mspOtuCollector;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOtuMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    // Check for Otu ..............................................................................
    Chm6OtuConfig *pCfg = dynamic_cast<Chm6OtuConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Otu Config";
            return true;
        }

        mspOtuCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OtuConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::CREATE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOtuMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    // Check for Otu ..............................................................................
    Chm6OtuConfig *pCfg = dynamic_cast<Chm6OtuConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Otu Config";
            return true;
        }

        mspOtuCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OtuConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::MODIFY);


        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOtuMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    // Check for Otu ..............................................................................
    Chm6OtuConfig *pCfg = dynamic_cast<Chm6OtuConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Otu Config";
            return true;
        }

        mspOtuCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OtuConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::DELETE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOtuMgr::checkOnResync(google::protobuf::Message* objMsg)
{
    // Check for Otu ..............................................................................
    Chm6OtuConfig *pCfg = dynamic_cast<Chm6OtuConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Otu Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OtuConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspOtuCfgHndlr->createCacheObj(*pCfg);

        mspOtuCollector->createOtu(*pCfg);

        mspOtuCollector->updateConfigCache(pCfg->base_config().config_id().value(), pCfg);

        return true;
    }

    return false;
}

ConfigStatus DpOtuMgr::processConfig(google::protobuf::Message* objMsg, ConfigType cfgType)
{
    // Check for Carrier ..............................................................................
    Chm6OtuConfig *pCfg = static_cast<Chm6OtuConfig*>(objMsg);

    ConfigStatus status = ConfigStatus::SUCCESS;

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspOtuCollector->createFaultPmPlaceHolder(pCfg->base_config().config_id().value());

            status = mspOtuCfgHndlr->onCreate(pCfg);

            break;

        case ConfigType::MODIFY:
            status = mspOtuCfgHndlr->onModify(pCfg);
            break;

        case ConfigType::DELETE:
            status = mspOtuCfgHndlr->onDelete(pCfg);
            break;
    }

    return status;
}

ConfigStatus DpOtuMgr::completeConfig(google::protobuf::Message* objMsg,
                                 ConfigType cfgType,
                                 ConfigStatus status,
                                 std::ostringstream& log)
{
    INFN_LOG(SeverityLevel::info) << "cfgType " << toString(cfgType);

    Chm6OtuConfig *pCfg = static_cast<Chm6OtuConfig*>(objMsg);

    STATUS transactionStat = STATUS::STATUS_SUCCESS;
    if (ConfigStatus::SUCCESS != status)
    {
        transactionStat = STATUS::STATUS_FAIL;

        INFN_LOG(SeverityLevel::info) << "Fall-back failure handling: Caching Config although it has failed.";

        switch(cfgType)
        {
            case ConfigType::CREATE:
                mspOtuCfgHndlr->createCacheObj(*pCfg);
                break;

            case ConfigType::MODIFY:
                mspOtuCfgHndlr->updateCacheObj(*pCfg);
                break;

            case ConfigType::DELETE:
                STATUS tStat = mspOtuCfgHndlr->deleteOtuConfigCleanup(pCfg);
                if (STATUS::STATUS_FAIL == tStat)
                {
                    INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();
                }
                break;
        }

        mspOtuCfgHndlr->setTransactionErrorString(log.str());
        mspOtuCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);
        return ConfigStatus::SUCCESS;
    }

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspOtuCfgHndlr->createCacheObj(*pCfg);

            mspOtuCollector->createOtu(*pCfg);

            mspOtuCollector->updateConfigCache(pCfg->base_config().config_id().value(), pCfg);

            break;

        case ConfigType::MODIFY:
            //@todo : Do we need to notify modifications in carrier to DCO Secproc?

            mspOtuCfgHndlr->updateCacheObj(*pCfg);

            mspOtuCollector->updateConfigCache(pCfg->base_config().config_id().value(), pCfg);

            mspOtuCollector->updateInstalledConfig(*pCfg);

            break;

        case ConfigType::DELETE:
            mspOtuCollector->deleteOtu(pCfg->base_config().config_id().value());

            mspOtuCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

            STATUS tStat = mspOtuCfgHndlr->deleteOtuConfigCleanup(pCfg);
            if (STATUS::STATUS_FAIL == tStat)
            {
                INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();

                return ConfigStatus::FAIL_OTHER;
            }

            mspOtuCfgHndlr->deleteCacheObj(*pCfg);

            // return here; transaction response already sent above in this case
            return ConfigStatus::SUCCESS;

            break;
    }

    mspOtuCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

    return ConfigStatus::SUCCESS;
}

bool DpOtuMgr::isSubEnabled()
{
    return DcoOtuAdapter::mOtuStateSubEnable && DcoOtuAdapter::mOtuFltSubEnable && DcoOtuAdapter::mOtuPmSubEnable;
}

void DpOtuMgr::updateFaultCache()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " update otu adapter notify cache";
    mpOtuAd->updateNotifyCache();
}

} // namespace DataPlane
