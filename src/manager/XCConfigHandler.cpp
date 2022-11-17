/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "XCConfigHandler.h"
#include "DataPlaneMgr.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "dp_common.h"
#include "DpGigeClientMgr.h"
#include "DpOtuMgr.h"
#include "DpOduMgr.h"

#include <iostream>
#include <string>
#include <sstream>


using chm6_dataplane::Chm6XCConfig;
using chm6_dataplane::Chm6XCState;

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace DpAdapter;


namespace DataPlane {

XCConfigHandler::XCConfigHandler(XcPbTranslator* pbTrans,
                                 DpAdapter::DcoXconAdapter *pAd,
                                 std::shared_ptr<XCStateCollector> spXcCollector,
                                 shared_ptr<FpgaSacIf> fpgaSacIf)
    : mpPbTranslator(pbTrans)
    , mpDcoAd(pAd)
    , mspXcCollector(spXcCollector)
    , mspFpgaSacIf(fpgaSacIf)
{
    INFN_LOG(SeverityLevel::info) << "Constructor";

    assert(pbTrans != NULL);
    assert(pAd  != NULL);
}

ConfigStatus XCConfigHandler::onCreate(Chm6XCConfig* xconCfg)
{
    std::string xconAid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << xconAid;

    std::string cfgDataStr;
    MessageToJsonString(*xconCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    ConfigStatus status = createXCConfig(xconCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Xcon create on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << xconAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus XCConfigHandler::onModify(Chm6XCConfig* xconCfg)
{
    std::string xconAid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << xconAid;

    std::string cfgDataStr;
    MessageToJsonString(*xconCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    ConfigStatus status = updateXCConfig(xconCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Xcon update on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << xconAid << " status = " << statToString(status);

    return (status);
}


ConfigStatus XCConfigHandler::onDelete(Chm6XCConfig* xconCfg)
{
    std::string xconAid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << xconAid;

    std::string cfgDataStr;
    MessageToJsonString(*xconCfg, &cfgDataStr);
    INFN_LOG(SeverityLevel::info) << cfgDataStr;

    ConfigStatus status = deleteXCConfig(xconCfg);
    if (ConfigStatus::SUCCESS != status)
    {
        std::ostringstream log;
        log << "Xcon delete on DCO failed";
        setTransactionErrorString(log.str());
    }

    INFN_LOG(SeverityLevel::info) << "AID: " << xconAid << " status = " << statToString(status);

    return (status);
}

ConfigStatus XCConfigHandler::createXCConfig(Chm6XCConfig* xconCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;
    bool ret = false;

    std::string aid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " create";

    DpAdapter::XconCommon adXconCfg;
    mpPbTranslator->configPbToAd(*xconCfg, adXconCfg);

    // Xpress Xcon
    if (adXconCfg.payloadTreatment == XCONPAYLOADTREATMENT_REGEN ||
        adXconCfg.payloadTreatment == XCONPAYLOADTREATMENT_REGEN_SWITCHING  )
    {
        switch (adXconCfg.payload)
        {
            case XCONPAYLOADTYPE_100G: // ODU Switching.
                // 1. Create OTU4 client with term LoopBack
                ret = DpOtuMgrSingleton::Instance()->getAdPtr()->createOtuClientXpress(adXconCfg.sourceClient);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createClientXpress OTU4 on DCO failed " << adXconCfg.sourceClient;
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpOtuMgrSingleton::Instance()->getAdPtr()->createOtuClientXpress(adXconCfg.destinationClient);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createClientXpress OTU4 on DCO failed " << adXconCfg.destinationClient;
                    return (ConfigStatus::FAIL_CONNECT);
                }
                // 2. create ODU4 client.
                ret = DpOduMgrSingleton::Instance()->getAdPtr()->createOduClientXpress(adXconCfg.sourceClient);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createXpressXcon ODU4 on DCO failed " << adXconCfg.sourceClient;
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpOduMgrSingleton::Instance()->getAdPtr()->createOduClientXpress(adXconCfg.destinationClient);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createXpressXcon ODU4 on DCO failed " << adXconCfg.sourceClient;
                    return (ConfigStatus::FAIL_CONNECT);
                }
                break;
            case XCONPAYLOADTYPE_100GBE:
                ret = DpGigeClientMgrSingleton::Instance()->getAdPtr()->createGigeClientXpress(adXconCfg.sourceClient, PORT_RATE_ETH100G);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createXpressXcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpGigeClientMgrSingleton::Instance()->getAdPtr()->createGigeClientXpress(adXconCfg.destinationClient, PORT_RATE_ETH100G);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createXpressXcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                break;
            case XCONPAYLOADTYPE_400GBE:
                ret = DpGigeClientMgrSingleton::Instance()->getAdPtr()->createGigeClientXpress(adXconCfg.sourceClient, PORT_RATE_ETH400G);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createXpressXcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpGigeClientMgrSingleton::Instance()->getAdPtr()->createGigeClientXpress(adXconCfg.destinationClient, PORT_RATE_ETH400G);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "createXpressXcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                break;
            case XCONPAYLOADTYPE_OTU4:  // Not in R6.0
                default:
                INFN_LOG(SeverityLevel::error) << "createXpressXcon not support payload " << adXconCfg.payload;
                return ( ConfigStatus::FAIL_DATA );
        }

        uint32_t srcLoOduId, dstLoOduId;  // get id to configure OH FPGA
        // Create Xpress Xcon
         ret = mpDcoAd->createXpressXcon(aid, &adXconCfg, &srcLoOduId, &dstLoOduId);
        // Turn on LoOdu OH forwarding
        configFpgaLoOduOhForwarding(srcLoOduId, dstLoOduId, true);
    }
    else
    {
        // Update DCO Adapter
        ret = mpDcoAd->createXcon(aid, &adXconCfg);
    }
    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "createXcon on DCO failed";
    }

    return (status);
}

ConfigStatus XCConfigHandler::updateXCConfig(Chm6XCConfig* xconCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " modify";

    bool retValue = true;
    bool ret;

    DpAdapter::XconCommon adXconCfg;
    mpPbTranslator->configPbToAd(*xconCfg, adXconCfg);

    // Source
    if (xconCfg->hal().has_source())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Source: ?";
        ret = false;  // ?
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setSource() failed";
        }
    }

    // Destination
    if (xconCfg->hal().has_destination())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " Destination: ?";
        ret = false;  // ?
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setDestination() failed";
        }
    }

    // XCDirection
    if (hal_dataplane::DIRECTION_UNSPECIFIED != xconCfg->hal().xc_direction())
    {
        INFN_LOG(SeverityLevel::info) << "AID: " << aid << " XCDirection: ?";
        ret = false;  // ?
        if (false == ret)
        {
            retValue = false;
            INFN_LOG(SeverityLevel::error) << "setXCDirection() failed";
        }
    }

    if (false == retValue)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "modifyXcon on DCO failed";
    }

    return (status);
}

ConfigStatus XCConfigHandler::deleteXCConfig(Chm6XCConfig* xconCfg)
{
    ConfigStatus status = ConfigStatus::SUCCESS;

    std::string aid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " delete";

    // Xpress XCON: Delete 4 unidir XCON and 2 clients
    // Xpress XCON AID: 1-5-L1-1-ODU4i-1,1-5-L2-ODU4i-1"
    string aidClient = "-T";
    std::size_t tpos = aid.find(aidClient);
    bool ret = false;
    // No Trib AID, this is the Xpress XCON case
    if (tpos == std::string::npos)
    {
        string srcClientAid, dstClientAid;
        uint32_t srcLoOduId, dstLoOduId;
        XconPayLoadType payload;
        ret = mpDcoAd->deleteXpressXcon(aid, &srcClientAid, &dstClientAid, &payload, &srcLoOduId, &dstLoOduId);
        if (false == ret)
        {
            INFN_LOG(SeverityLevel::error) << "delete Xpress Xcon on DCO failed";
            return (ConfigStatus::FAIL_CONNECT);
        }
        // Turn off LoOdu OH forwarding
        configFpgaLoOduOhForwarding(srcLoOduId, dstLoOduId, false);
        switch (payload)
        {
            case XCONPAYLOADTYPE_100G:  // ODU Switching
                // Deleting OTU4 and ODU4 clients
                ret = DpOduMgrSingleton::Instance()->getAdPtr()->deleteOdu(srcClientAid);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "Delete Xpress Xcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpOduMgrSingleton::Instance()->getAdPtr()->deleteOdu(dstClientAid);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "Delete Xpress Xcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpOtuMgrSingleton::Instance()->getAdPtr()->deleteOtu(srcClientAid);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "Delete Xpress Xcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpOtuMgrSingleton::Instance()->getAdPtr()->deleteOtu(dstClientAid);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "Delete Xpress Xcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                break;
            case XCONPAYLOADTYPE_100GBE:
            case XCONPAYLOADTYPE_400GBE:
                ret = DpGigeClientMgrSingleton::Instance()->getAdPtr()->deleteGigeClient(srcClientAid);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "Delete Xpress Xcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                ret = DpGigeClientMgrSingleton::Instance()->getAdPtr()->deleteGigeClient(dstClientAid);
                if (false == ret)
                {
                    INFN_LOG(SeverityLevel::error) << "Delete Xpress Xcon on DCO failed";
                    return (ConfigStatus::FAIL_CONNECT);
                }
                break;
            case XCONPAYLOADTYPE_OTU4:  // Not in R6.0
            default:
                INFN_LOG(SeverityLevel::error) << "Delete Xpress Xcon not support payload: " << payload;
                return ( ConfigStatus::FAIL_DATA );
        }
    }
    else
    {
        // Update DCO Adapter
        ret = mpDcoAd->deleteXcon(aid);
    }

    if (false == ret)
    {
        status = ConfigStatus::FAIL_CONNECT;
        INFN_LOG(SeverityLevel::error) << "deleteXcon on DCO failed";
    }

    return (status);
}

STATUS XCConfigHandler::deleteXCConfigCleanup(Chm6XCConfig* xconCfg)
{
    STATUS status = STATUS::STATUS_SUCCESS;

    std::string aid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info)"AID: " << aid << " delete";

    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    Chm6XCState xcState;
    xcState.mutable_base_state()->mutable_config_id()->set_value(aid);
    xcState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    xcState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
    xcState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(xcState);


    xconCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
    vDelObjects.push_back(*xconCfg);

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);

    return (status);
}

void XCConfigHandler::sendTransactionStateToInProgress(Chm6XCConfig* xconCfg)
{
    std::string aid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction in-progress state";

    Chm6XCState* pState = new Chm6XCState();

    pState->mutable_base_state()->mutable_config_id()->set_value(aid);
    pState->mutable_base_state()->mutable_timestamp()->set_seconds(xconCfg->base_config().timestamp().seconds());
    pState->mutable_base_state()->mutable_timestamp()->set_nanos(xconCfg->base_config().timestamp().nanos());

    pState->mutable_base_state()->set_transaction_state(chm6_common::STATE_INPROGRESS);

    string stateDataStr;
    MessageToJsonString(*pState, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(*pState);

    delete pState;
}

void XCConfigHandler::sendTransactionStateToComplete(Chm6XCConfig* xconCfg,
                                                     STATUS status)
{
    std::string aid = xconCfg->base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << aid << " send transaction complete state " << toString(status);

    Chm6XCState stateObj;

    stateObj.mutable_base_state()->mutable_config_id()->set_value(aid);
    stateObj.mutable_base_state()->mutable_timestamp()->set_seconds(xconCfg->base_config().timestamp().seconds());
    stateObj.mutable_base_state()->mutable_timestamp()->set_nanos(xconCfg->base_config().timestamp().nanos());

    stateObj.mutable_base_state()->set_transaction_state(chm6_common::STATE_COMPLETE);
    stateObj.mutable_base_state()->set_transaction_status(status);

    stateObj.mutable_base_state()->mutable_transaction_info()->set_value(getTransactionErrorString());

    string stateDataStr;
    MessageToJsonString(stateObj, &stateDataStr);
    INFN_LOG(SeverityLevel::info) << stateDataStr;

    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(stateObj);

    setTransactionErrorString("");
}

void XCConfigHandler::createCacheObj(chm6_dataplane::Chm6XCConfig& objCfg)
{
    chm6_dataplane::Chm6XCConfig* pObjCfg = new chm6_dataplane::Chm6XCConfig(objCfg);

    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    addCacheObj(configId, pObjCfg);
}

void XCConfigHandler::updateCacheObj(chm6_dataplane::Chm6XCConfig& objCfg)
{
    chm6_dataplane::Chm6XCConfig* pObjCfg;
    google::protobuf::Message* pMsg;

    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    lock_guard<recursive_mutex> lock(mMutex);

    if (getCacheObj(configId, pMsg))
    {
        INFN_LOG(SeverityLevel::error) << "Cached Object not found for configId: " << configId;

        // todo: what to do here??? This should not happen
        createCacheObj(objCfg);
    }
    else
    {
        if (pMsg)
        {
            pObjCfg = (chm6_dataplane::Chm6XCConfig*)pMsg;

            pObjCfg->MergeFrom(objCfg);
        }
    }
}

void XCConfigHandler::deleteCacheObj(chm6_dataplane::Chm6XCConfig& objCfg)
{
    std::string configId = objCfg.base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    clearCache(configId);
}

STATUS XCConfigHandler::sendConfig(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6XCConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6XCConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    // todo: is it ok to always call create here??
    return (createXCConfig(pCfgMsg)  == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);
}

STATUS XCConfigHandler::sendDelete(google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6XCConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6XCConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::info) << "configId: " << configId;

    return (deleteXCConfig(pCfgMsg)  == ConfigStatus::SUCCESS ? STATUS::STATUS_SUCCESS : STATUS::STATUS_FAIL);
}

bool XCConfigHandler::isMarkForDelete(const google::protobuf::Message& msg)
{
    const chm6_dataplane::Chm6XCConfig* pMsg = static_cast<const chm6_dataplane::Chm6XCConfig*>(&msg);

    std::string configId = pMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    bool retVal = false;

    if (pMsg->base_config().has_mark_for_delete())
    {
        if (pMsg->base_config().mark_for_delete().value())
        {
            retVal = true;
        }
    }

    return retVal;
}

void XCConfigHandler::dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg)
{
    chm6_dataplane::Chm6XCConfig* pCfgMsg = static_cast<chm6_dataplane::Chm6XCConfig*>(&msg);

    std::string configId = pCfgMsg->base_config().config_id().value();

    INFN_LOG(SeverityLevel::debug) << "configId: " << configId;

    string jsonStr;
    MessageToJsonString(*pCfgMsg, &jsonStr);

    out << "** Dumping Xcon ConfigId: " << configId << endl;
    mpPbTranslator->dumpJsonFormat(jsonStr, out);
    out << endl << endl;
}

bool XCConfigHandler::isXconPresent(std::string configId, ObjectMgrs objId)
{
    bool retVal = false;

    google::protobuf::Message* pMsg;

    if (getCacheObjContainsId(configId, pMsg))
    {
        INFN_LOG(SeverityLevel::debug) << "Cached Object not found for configId: " << configId;
        return retVal;
    }
    else
    {
        chm6_dataplane::Chm6XCConfig* pObjCfg;
        pObjCfg = (chm6_dataplane::Chm6XCConfig*)pMsg;

        std::string xconAid = pObjCfg->base_config().config_id().value();

        if (OBJ_MGR_GIGE == objId)
        {
            if ((xconAid.find("ODU4i") != std::string::npos) || (xconAid.find("ODUflexi") != std::string::npos))
            {
                retVal =  true;
            }
        }
        else if ((OBJ_MGR_OTU == objId) || (OBJ_MGR_ODU == objId))
        {
            if (xconAid.find("ODU4") != std::string::npos)
            {
                retVal =  true;
            }
        }
    }

    return retVal;
}

// When we have Xpress XCON of LO-ODUs, the faults would need to be forwarded
// from one LoOdu to the other LoOdu.  This function is perform in CHM6 FPGA
void XCConfigHandler::configFpgaLoOduOhForwarding(uint32_t srcLoOduId, uint32_t dstLoOduId, bool bEnableForwarding)
{
    if (nullptr == mspFpgaSacIf)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " ERROR!! mspFpgaSacIf is NULL " << endl;
        return;
    }

    // LoOdu instance is always 1-32 for SDK/FPGA compability
    if ( srcLoOduId > CLIENT_ODU_ID_OFFSET || dstLoOduId > CLIENT_ODU_ID_OFFSET || srcLoOduId == dstLoOduId || srcLoOduId == 0 || dstLoOduId == 0 )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " ERROR!! Bad LoOdu ID src: " << srcLoOduId << " dst: " << dstLoOduId << endl;
        return;
    }

    uint32_t offset = cFpgaSac_OduRegenConfigRegOffset + ((srcLoOduId-1) * sizeof (uint32_t)) ;

    uint32_t regVal;

    if ( bEnableForwarding == true )
    {
        regVal = ( ((dstLoOduId-1) << cFpgaSac_OduRegenConfigRemapOffset) | ( (int) bEnableForwarding << cFpgaSac_OduRegenConfigEnableOffset) );
    }
    else
    {
        // Turn off OH forwarding
        regVal = 0;
    }
    // Src OH forward to Dst ODU
    mspFpgaSacIf->Write32(offset, regVal);

    offset = cFpgaSac_OduRegenConfigRegOffset + ((dstLoOduId-1) * sizeof (uint32_t)) ;

    if ( bEnableForwarding == true )
    {
        regVal = ( ((srcLoOduId-1) << cFpgaSac_OduRegenConfigRemapOffset) | ( (int) bEnableForwarding << cFpgaSac_OduRegenConfigEnableOffset) );
    }
    else
    {
        // Turn off OH forwarding
        regVal = 0;
    }
    // dst OH forward to src ODU
    mspFpgaSacIf->Write32(offset, regVal);
}

void XCConfigHandler::dumpXpressXconOhFpga(std::ostream& os)
{
    if (nullptr == mspFpgaSacIf)
    {
        os << " ERROR!! mspFpgaSacIf is NULL " << endl;
        return;
    }
os << " FPGA LO-ODU index is 0-based" << endl << endl;

    for (int idx = 0; idx < CLIENT_ODU_ID_OFFSET; idx++)
    {
        uint32_t offset = cFpgaSac_OduRegenConfigRegOffset + (idx * sizeof (uint32_t)) ;
        uint32 regVal = mspFpgaSacIf->Read32(offset);

        os << "addr: 0x" << hex << offset << dec << " LoOdu HO: " << idx << " = 0x" << hex << regVal << dec << " enable: " << (int) (regVal & cFpgaSac_OduRegenConfigEnableMask) << " DST ODU: " << (int)((regVal & cFpgaSac_OduRegenConfigRemapMask) >> cFpgaSac_OduRegenConfigRemapOffset) << endl;
    }
    os << endl; 
}

} // namespace DataPlane
