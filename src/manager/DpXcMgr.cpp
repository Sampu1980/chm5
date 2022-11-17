/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include <iostream>
#include <string>
#include <sstream>

#include "DpXcMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DataPlaneMgr.h"
#include "DcoConnectHandler.h"

using namespace ::std;
using chm6_dataplane::Chm6XCConfig;
using google::protobuf::util::MessageToJsonString;


namespace DataPlane
{

DpXcMgr::DpXcMgr()
    : DpObjectMgr("Xcon", (1 << DCC_ZYNQ))
    , mpPbTranslator(NULL)
    , mpXcAd(NULL)
    , mspFpgaSacIf(NULL)
{

}

DpXcMgr::~DpXcMgr()
{
    if (mpPbTranslator)
    {
        delete mpPbTranslator;
    }
    if (mpXcAd)
    {
        delete mpXcAd;
    }
}

void DpXcMgr::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Creating DcoXcAdapter";

    mspFpgaSacIf = DataPlaneMgrSingleton::Instance()->getFpgaSacIf();

    mpXcAd = new DpAdapter::DcoXconAdapter();

    mpPbTranslator = new XcPbTranslator();

    createXcStateCollector();

    createXcConfigHandler();

    mIsInitDone = true;
}

void DpXcMgr::start()
{
    INFN_LOG(SeverityLevel::info) << "Initializing DcoXcAdapter";

    if (mpXcAd->initializeXcon() == false)
    {
        INFN_LOG(SeverityLevel::error) << "DcoXconAdapter Initialize Failed";

        // what todo?
    }
}

void DpXcMgr::reSubscribe()
{
    INFN_LOG(SeverityLevel::info) << "reSubscribe XCON Streams";

    mpXcAd->subscribeStreams();
}

void DpXcMgr::createXcConfigHandler()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspXcCfgHndlr = std::make_shared<XCConfigHandler>(mpPbTranslator, mpXcAd, mspXcCollector, mspFpgaSacIf);

    mspConfigHandler = mspXcCfgHndlr;
}

void DpXcMgr::createXcStateCollector()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspXcCollector = std::make_shared<XCStateCollector>(mpPbTranslator, mpXcAd);

    mspStateCollector = mspXcCollector;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpXcMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    // Check for Xc ..............................................................................
    Chm6XCConfig *pCfg = dynamic_cast<Chm6XCConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for XC Config";
            return true;
        }

        mspXcCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is XcConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::CREATE);

	return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpXcMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    // Check for Xc ..............................................................................
    Chm6XCConfig *pCfg = dynamic_cast<Chm6XCConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for XC Config";
            return true;
        }

        mspXcCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is XcConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::MODIFY);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpXcMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    // Check for Xc ..............................................................................
    Chm6XCConfig *pCfg = dynamic_cast<Chm6XCConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for XC Config";
            return true;
        }

        mspXcCfgHndlr->sendTransactionStateToInProgress(pCfg);

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is XcConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        handleConfig(objMsg, pCfg->base_config().config_id().value(), ConfigType::DELETE);

        return true;
    }

    return false;
}

// Returns true if the dynamic_cast succeeds and message is processed
//         false otherwise
bool DpXcMgr::checkOnResync(google::protobuf::Message* objMsg)
{
    // Check for Xc ..............................................................................
    Chm6XCConfig *pCfg = dynamic_cast<Chm6XCConfig*>(objMsg);
    if (pCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for XC Config";
            return true;
        }

        string data;
        MessageToJsonString(*pCfg, &data);
        INFN_LOG(SeverityLevel::info) << "Determined it is XcConfig Msg";
        INFN_LOG(SeverityLevel::info) << data;

        mspXcCfgHndlr->createCacheObj(*pCfg);

        mspXcCollector->createXcon(*pCfg);

        return true;
    }

    return false;
}

ConfigStatus DpXcMgr::processConfig(google::protobuf::Message* objMsg, ConfigType cfgType)
{
    // Check for Carrier ..............................................................................
    Chm6XCConfig *pCfg = static_cast<Chm6XCConfig*>(objMsg);

    ConfigStatus status = ConfigStatus::SUCCESS;

    switch(cfgType)
    {
        case ConfigType::CREATE:
            status = mspXcCfgHndlr->onCreate(pCfg);
            break;

        case ConfigType::MODIFY:
            status = mspXcCfgHndlr->onModify(pCfg);
            break;

        case ConfigType::DELETE:
            status = mspXcCfgHndlr->onDelete(pCfg);
            break;
    }

    return status;
}

ConfigStatus DpXcMgr::completeConfig(google::protobuf::Message* objMsg,
                                 ConfigType cfgType,
                                 ConfigStatus status,
                                 std::ostringstream& log)
{
    INFN_LOG(SeverityLevel::info) << "cfgType " << toString(cfgType);

    Chm6XCConfig *pCfg = static_cast<Chm6XCConfig*>(objMsg);

    STATUS transactionStat = STATUS::STATUS_SUCCESS;
    if (ConfigStatus::SUCCESS != status)
    {
        transactionStat = STATUS::STATUS_FAIL;

        INFN_LOG(SeverityLevel::info) << "Fall-back failure handling: Caching Config although it has failed.";

        switch(cfgType)
        {
            case ConfigType::CREATE:
                mspXcCfgHndlr->createCacheObj(*pCfg);
                break;

            case ConfigType::MODIFY:
                mspXcCfgHndlr->updateCacheObj(*pCfg);
                break;

            case ConfigType::DELETE:
                STATUS tStat = mspXcCfgHndlr->deleteXCConfigCleanup(pCfg);
                if (STATUS::STATUS_FAIL == tStat)
                {
                    INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();
                }
                break;
        }

        mspXcCfgHndlr->setTransactionErrorString(log.str());
        mspXcCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);
        return ConfigStatus::SUCCESS;
    }

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspXcCfgHndlr->createCacheObj(*pCfg);

            mspXcCollector->createXcon(*pCfg);

            break;

        case ConfigType::MODIFY:
            //@todo : Do we need to notify modifications in carrier to DCO Secproc?

            mspXcCfgHndlr->updateCacheObj(*pCfg);

            mspXcCollector->updateInstalledConfig(*pCfg);

            break;

        case ConfigType::DELETE:
            mspXcCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

            STATUS tStat = mspXcCfgHndlr->deleteXCConfigCleanup(pCfg);
            if (STATUS::STATUS_FAIL == tStat)
            {
                INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();

                return ConfigStatus::FAIL_OTHER;
            }

            mspXcCfgHndlr->deleteCacheObj(*pCfg);

            mspXcCollector->deleteXcon(pCfg->base_config().config_id().value());

            // return here; transaction response already sent above in this case
            return ConfigStatus::SUCCESS;

            break;
    }

    mspXcCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

    return ConfigStatus::SUCCESS;
}


} // namespace DataPlane
