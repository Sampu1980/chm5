/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DpOduMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DpPmHandler.h"
#include "DataPlaneMgr.h"
#include "DcoConnectHandler.h"

using namespace ::std;
using chm6_dataplane::Chm6OduConfig;
using chm6_dataplane::Chm6OduState;
using google::protobuf::util::MessageToJsonString;
using namespace DpAdapter;

namespace DataPlane
{

DpOduMgr::DpOduMgr()
    : DpObjectMgr("Odu", (1 << DCC_ZYNQ))
    , mpPbTranslator(NULL)
    , mpOduAd(NULL)
    , mspFpgaSramIf(NULL)
{

}

DpOduMgr::~DpOduMgr()
{
    if (mpPbTranslator)
    {
        delete mpPbTranslator;
    }
    if (mpOduAd)
    {
        delete mpOduAd;
    }

    unregisterOduPm();
}

void DpOduMgr::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Creating DcoOduAdapter";

    mpOduAd = new DpAdapter::DcoOduAdapter();

    mspFpgaSramIf = DataPlaneMgrSingleton::Instance()->getFpgaSramIf();

    createOduPbTranslator();
    createOduStateCollector();
    createOduConfigHandler();

    mIsInitDone = true;
}

void DpOduMgr::start()
{
    INFN_LOG(SeverityLevel::info) << "Initializing DcoOduAdapter";

    if (mpOduAd->initializeOdu() == false)
    {
        INFN_LOG(SeverityLevel::error) << "DcoOduAdapter Initialize Failed";

        // what todo?
    }

    registerOduPm();

    mspOduCollector->start();
}

void DpOduMgr::reSubscribe()
{
    INFN_LOG(SeverityLevel::info) << "";

    mpOduAd->subscribeStreams();
}

void DpOduMgr::connectionUpdate()
{
    INFN_LOG(SeverityLevel::info) << "Updating collector with new connection state isConnectionUp: " << mIsConnectionUp;

    mspOduCollector->setIsConnectionUp(mIsConnectionUp);
}

void DpOduMgr::registerOduPm()
{
    INFN_LOG(SeverityLevel::info) << "Registering Odu PM Callback";

    DpMsODUPm oduPm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpmsCbHandler *cbHandler = new DpmsOduCbHandler();
    dpMsPmCallbacksSingleton->RegisterCallBack(CLASS_NAME(oduPm), cbHandler);
}

void DpOduMgr::unregisterOduPm()
{
    INFN_LOG(SeverityLevel::info) << "Unregistering Odu PM Callback";

    DpMsODUPm oduPm;
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    dpMsPmCallbacksSingleton->UnregisterCallBack(CLASS_NAME(oduPm));
}

void DpOduMgr::createOduPbTranslator()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mpPbTranslator = new OduPbTranslator();
}

void DpOduMgr::createOduConfigHandler()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspOduCfgHndlr = std::make_shared<OduConfigHandler>(mpPbTranslator, mpOduAd, mspOduCollector);

    mspConfigHandler = mspOduCfgHndlr;
}

void DpOduMgr::createOduStateCollector()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspOduCollector = std::make_shared<OduStateCollector>(mpPbTranslator, mpOduAd, mspFpgaSramIf);

    mspStateCollector = mspOduCollector;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOduMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    // Check for Odu ..............................................................................
    Chm6OduConfig *pCfg = dynamic_cast<Chm6OduConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Odu Config";
            return true;
        }

        mspOduCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OduConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::CREATE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOduMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    // Check for Odu ..............................................................................
    Chm6OduConfig *pCfg = dynamic_cast<Chm6OduConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Odu Config";
            return true;
        }

        mspOduCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OduConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::MODIFY);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOduMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    // Check for Odu ..............................................................................
    Chm6OduConfig *pCfg = dynamic_cast<Chm6OduConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Odu Config";
            return true;
        }

        mspOduCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OduConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::DELETE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpOduMgr::checkOnResync(google::protobuf::Message* objMsg)
{
    // Check for Odu ..............................................................................
    Chm6OduConfig *pCfg = dynamic_cast<Chm6OduConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for Odu Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is OduConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspOduCfgHndlr->createCacheObj(*pCfg);

        mspOduCollector->createOdu(*pCfg);

        mspOduCollector->updateConfigCache(pCfg->base_config().config_id().value(), pCfg);

        return true;
    }

    return false;
}

ConfigStatus DpOduMgr::processConfig(google::protobuf::Message* objMsg, ConfigType cfgType)
{
    // Check for Carrier ..............................................................................
    Chm6OduConfig *pCfg = static_cast<Chm6OduConfig*>(objMsg);

    ConfigStatus status = ConfigStatus::SUCCESS;

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspOduCollector->createFaultPmPlaceHolder(pCfg->base_config().config_id().value());

            status = mspOduCfgHndlr->onCreate(pCfg);

            break;

        case ConfigType::MODIFY:
            status = mspOduCfgHndlr->onModify(pCfg);
            break;

        case ConfigType::DELETE:
            status = mspOduCfgHndlr->onDelete(pCfg);
            break;
    }

    return status;
}

ConfigStatus DpOduMgr::completeConfig(google::protobuf::Message* objMsg,
                                 ConfigType cfgType,
                                 ConfigStatus status,
                                 std::ostringstream& log)
{
    INFN_LOG(SeverityLevel::info) << "cfgType " << toString(cfgType);

    Chm6OduConfig *pCfg = static_cast<Chm6OduConfig*>(objMsg);

    STATUS transactionStat = STATUS::STATUS_SUCCESS;
    if (ConfigStatus::SUCCESS != status)
    {
        transactionStat = STATUS::STATUS_FAIL;

        INFN_LOG(SeverityLevel::info) << "Fall-back failure handling: Caching Config although it has failed.";

        switch(cfgType)
        {
            case ConfigType::CREATE:
                mspOduCfgHndlr->createCacheObj(*pCfg);
                break;

            case ConfigType::MODIFY:
                mspOduCfgHndlr->updateCacheObj(*pCfg);
                break;

            case ConfigType::DELETE:
                STATUS tStat = mspOduCfgHndlr->deleteOduConfigCleanup(pCfg);
                if (STATUS::STATUS_FAIL == tStat)
                {
                    INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();
                }
                break;
        }

        mspOduCfgHndlr->setTransactionErrorString(log.str());
        mspOduCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);
        return ConfigStatus::SUCCESS;
    }

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspOduCfgHndlr->createCacheObj(*pCfg);

            mspOduCollector->createOdu(*pCfg);

            mspOduCollector->updateConfigCache(pCfg->base_config().config_id().value(), pCfg);

            break;

        case ConfigType::MODIFY:
            //@todo : Do we need to notify modifications in carrier to DCO Secproc?

            mspOduCfgHndlr->updateCacheObj(*pCfg);

            mspOduCollector->updateConfigCache(pCfg->base_config().config_id().value(), pCfg);

            mspOduCollector->updateInstalledConfig(*pCfg);

            break;

        case ConfigType::DELETE:
            mspOduCollector->deleteOdu(pCfg->base_config().config_id().value());

            mspOduCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

            STATUS tStat = mspOduCfgHndlr->deleteOduConfigCleanup(pCfg);
            if (STATUS::STATUS_FAIL == tStat)
            {
                INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();

                return ConfigStatus::FAIL_OTHER;
            }

            mspOduCfgHndlr->deleteCacheObj(*pCfg);

            // return here; transaction response already sent above in this case
            return ConfigStatus::SUCCESS;

            break;
    }

    mspOduCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

    return ConfigStatus::SUCCESS;
}

bool DpOduMgr::isSubEnabled()
{
    return DcoOduAdapter::mOduStateSubEnable && DcoOduAdapter::mOduFltSubEnable && DcoOduAdapter::mOduPmSubEnable;
}

void DpOduMgr::updateFaultCache()
{
    INFN_LOG(SeverityLevel::info) << __func__ << " update odu adapter notify cache";
    mpOduAd->updateNotifyCache();
}

} // namespace DataPlane
