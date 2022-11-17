#include "InfnLogger.h"
#include "CardPbTranslator.h"
#include "CarrierPbTranslator.h"
#include "GigeClientPbTranslator.h"
#include "OtuPbTranslator.h"
#include "OduPbTranslator.h"
#include "GccPbTranslator.h"
#include "DataPlaneMgr.h"
#include "DpPmHandler.h"
#include "dp_common.h"
#include "DpGigeClientMgr.h"
#include "DpOtuMgr.h"

#include <InfnTracer/InfnTracer.h>

using chm6_common::Chm6DcoCardPm;
using chm6_common::Chm6DcoCardState;
using chm6_dataplane::Chm6CarrierPm;
using chm6_dataplane::Chm6GigeClientPm;
using chm6_dataplane::Chm6OtuPm;
using chm6_dataplane::Chm6OduPm;
using chm6_dataplane::Chm6ScgPm;
using chm6_dataplane::Chm6GccPm;

using google::protobuf::util::MessageToJsonString;

using namespace std;


namespace DataPlane
{

bool DpmsCardCbHandler::OnRcvPm(string className, DcoBasePM &basePm)
{
    INFN_LOG(SeverityLevel::debug) << "Card PM Stream Callback triggered";

    DpMsCardPm* cardAdPm = static_cast<DpMsCardPm*>(&basePm);

    Chm6DcoCardState dcoCardPbState;
    dcoCardPbState.mutable_base_state()->mutable_config_id()->set_value("Chm6Internal");

    CardPbTranslator pbTranslator;
    pbTranslator.pmAdToPb(*cardAdPm, dcoCardPbState);

    if (dp_env::DpEnvSingleton::Instance()->isPmDebug())
    {
	    string pmDataStr;
	    MessageToJsonString(dcoCardPbState, &pmDataStr);
	    INFN_LOG(SeverityLevel::debug) << pmDataStr;
    }

    TRACE_STATE(Chm6DcoCardState, dcoCardPbState, TO_REDIS(), 
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(dcoCardPbState);
    )

#if 0  //For streaming PM to Board MS
    Chm6DcoCardPm dcoCardPbPm;
    dcoCardPbPm.mutable_base_pm()->mutable_config_id()->set_value("Chm6Internal");

    CardPbTranslator pbTranslator;
    pbTranslator.pmAdToPb(*cardAdPm, dcoCardPbPm);

    string pmDataStr;
    MessageToJsonString(dcoCardPbPm, &pmDataStr);
    INFN_LOG(SeverityLevel::debug) << pmDataStr;

    //TODO - need Redis and gRPC update
    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(dcoCardPbPm);
#endif

    return true;
}

bool DpmsCarrierCbHandler::OnRcvPm(string className, DcoBasePM &basePm)
{
    INFN_LOG(SeverityLevel::debug) << "Carrier PM Stream Callback triggered";

    DpMsCarrierPMs* carrierPmList = static_cast< DpMsCarrierPMs *>(&basePm);

    for (uint index = 1; index <= DataPlane::MAX_CARRIERS; index++)
    {
        if (false == carrierPmList->HasPmData(index))
        {
            continue;
        }

        INFN_LOG(SeverityLevel::debug) << "Carrier PM for CarrierId = " << index;

        DataPlane::DpMsCarrierPMContainer carrierAdPm = carrierPmList->GetPmData(index);

        //check pm data has valid aid, redis client would throw exception if aid is empty
        if (!carrierPmList->HasPmData(index) || carrierAdPm.aid.empty())
        {
            INFN_LOG(SeverityLevel::error) << " Invalid aid for CarrierId: " << index;
            continue;
        }

        Chm6CarrierPm carrierPbPm;
        carrierPbPm.mutable_base_pm()->mutable_config_id()->set_value(carrierAdPm.aid);

        CarrierPbTranslator pbTranslator;
        pbTranslator.pmAdToPb(carrierAdPm, true, carrierPbPm);

        if (dp_env::DpEnvSingleton::Instance()->isPmDebug())
        {
            string pmDataStr;
            MessageToJsonString(carrierPbPm, &pmDataStr);
            INFN_LOG(SeverityLevel::debug) << pmDataStr;
        }

        try
        {
            TRACE_PM(Chm6CarrierPm, carrierPbPm, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(carrierPbPm);
	    )
        }
        catch(std::exception const &excp)
        {
            INFN_LOG(SeverityLevel::info) << "Exception " << excp.what() << " caught in DpMgrRedisObjectStream carrierPbPm " << index;
        }
        catch(...)
        {
            INFN_LOG(SeverityLevel::info) << "Exception caught in DpMgrRedisObjectStream carrierPbPm " << index;
        }

        //Scg
        INFN_LOG(SeverityLevel::debug) << "Scg PM for ScgId = " << index;

        std::string scgAid;
        scgAid = pbTranslator.aidCarToScg(carrierAdPm.aid);

        Chm6ScgPm scgPbPm;
        scgPbPm.mutable_base_pm()->mutable_config_id()->set_value(scgAid);
        pbTranslator.pmAdToPb(carrierAdPm, true, scgPbPm);

        try
        {
            TRACE_PM(Chm6ScgPm, scgPbPm, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(scgPbPm);
	    )
        }
        catch(std::exception const &excp)
        {
            INFN_LOG(SeverityLevel::info) << "Exception " << excp.what() << " caught in DpMgrRedisObjectStream scgPbPm " << index;
        }
        catch(...)
        {
            INFN_LOG(SeverityLevel::info) << "Exception caught in DpMgrRedisObjectStream scgPbPm " << index;
        }
    }

	return true;
}

bool DpmsGigeCbHandler::OnRcvPm(string className, DcoBasePM &basePm)
{
    INFN_LOG(SeverityLevel::debug) << "GigeClient PM Stream Callback triggered";

    DpMsClientGigePms* gigeClientPmList = static_cast< DpMsClientGigePms *>(&basePm);

    for (uint index = 1; index <= DataPlane::MAX_CLIENTS; index++)
    {
        if (false == gigeClientPmList->HasPmData(index))
        {
            continue;
        }

        INFN_LOG(SeverityLevel::debug) << "GigeClient PM for GigeClientId = " << index;

        DataPlane::DpMsClientGigePmContainer gigeClientAdPm = gigeClientPmList->GetPmData(index);

        //check pm data has valid aid, redis client would throw exception if aid is empty
        if (!gigeClientPmList->HasPmData(index) || gigeClientAdPm.aid.empty())
        {
            INFN_LOG(SeverityLevel::error) << " Invalid aid for gigeId: " << index;
            continue;
        }

        Chm6GigeClientPm gigeClientPbPm;
        gigeClientPbPm.mutable_base_pm()->mutable_config_id()->set_value(gigeClientAdPm.aid);

        gearbox::BcmPm gigeClientAdGearBoxPm;
        DpGigeClientMgrSingleton::Instance()->getGigeClientGearBoxPm(gigeClientAdPm.aid, gigeClientAdGearBoxPm);

        GigeClientPbTranslator pbTranslator;

        bool isHardFaultRx, isHardFaultTx;

        DpAdapter::GigeFault adFault;
        if (DpGigeClientMgrSingleton::Instance()->getAdPtr()->getGigeFault(gigeClientAdPm.aid, &adFault, false))
        {
            isHardFaultRx = pbTranslator.isHardFault(adFault, FAULTDIRECTION_RX);
            isHardFaultTx = pbTranslator.isHardFault(adFault, FAULTDIRECTION_TX);
        }

        bool testSignalMon;
        DpGigeClientMgrSingleton::Instance()->getMspCfgHndlr()->getChm6GigeClientConfigTestSigMonFromCache(gigeClientAdPm.aid, testSignalMon);

        pbTranslator.pmAdToPb(gigeClientAdPm, gigeClientAdGearBoxPm, testSignalMon, isHardFaultRx, isHardFaultTx, true, gigeClientPbPm);

        if (dp_env::DpEnvSingleton::Instance()->isPmDebug())
        {
            string pmDataStr;
            MessageToJsonString(gigeClientPbPm, &pmDataStr);
            INFN_LOG(SeverityLevel::debug) << pmDataStr;
        }

        try
        {
            TRACE_PM(Chm6GigeClientPm, gigeClientPbPm, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(gigeClientPbPm);
	    )
        }
        catch(std::exception const &excp)
        {
            INFN_LOG(SeverityLevel::info) << "Exception " << excp.what() << " caught in DpMgrRedisObjectStream gigePm " << index;
        }
        catch(...)
        {
            INFN_LOG(SeverityLevel::info) << "Exception caught in DpMgrRedisObjectStream gigePm " << index;
        }
    }

    return true;
}

bool DpmsOtuCbHandler::OnRcvPm(string className, DcoBasePM &basePm)
{
    INFN_LOG(SeverityLevel::debug) << "Otu PM Stream Callback triggered";

    DpMsOTUPm* otuPmList = static_cast< DpMsOTUPm *>(&basePm);

    for (uint index = 1; index <= DataPlane::MAX_OTU_ID; index++)
    {
        if (false == otuPmList->IsPmValid(index))
        {
            continue;
        }

        INFN_LOG(SeverityLevel::debug) << "Otu PM for OtuId = " << index;

        DataPlane::DpMsOtuPMSContainer otuAdPm = otuPmList->GetPmData(index);

        //check pm data has valid aid, redis client would throw exception if aid is empty
        if (!otuPmList->IsPmValid(index) || otuAdPm.aid.empty())
        {
            INFN_LOG(SeverityLevel::error) << " Invalid aid for OtuId: " << index;
            continue;
        }

        bool fecCorrEnable = DpOtuMgrSingleton::Instance()->getAdPtr()->getOtuFecCorrEnable(otuAdPm.aid);

        Chm6OtuPm otuPbPm;
        otuPbPm.mutable_base_pm()->mutable_config_id()->set_value(otuAdPm.aid);

        bool isTimFault;
        isTimFault = DpOtuMgrSingleton::Instance()->isOtuTimFault(otuAdPm.aid);

        OtuPbTranslator pbTranslator;
        pbTranslator.pmAdToPb(otuAdPm, fecCorrEnable, isTimFault, true, otuPbPm);

        if (dp_env::DpEnvSingleton::Instance()->isPmDebug())
        {
            string pmDataStr;
            MessageToJsonString(otuPbPm, &pmDataStr);
            INFN_LOG(SeverityLevel::debug) << pmDataStr;
        }

        try
        {
            TRACE_PM(Chm6OtuPm, otuPbPm, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(otuPbPm);
	    )
        }
        catch(std::exception const &excp)
        {
            INFN_LOG(SeverityLevel::info) << "Exception " << excp.what() << " caught in DpMgrRedisObjectStream otuPbPm " << index;
        }
        catch(...)
        {
            INFN_LOG(SeverityLevel::info) << "Exception caught in DpMgrRedisObjectStream otuPbPm " << index;
        }
        otuPmList->UpdatePmValid(index, false); //mark this entry as invalid after transaction finished
    }

    return true;
}

bool DpmsOduCbHandler::OnRcvPm(string className, DcoBasePM &basePm)
{
    INFN_LOG(SeverityLevel::debug) << "Odu PM Stream Callback triggered";

    DpMsODUPm* oduPmList = static_cast< DpMsODUPm *>(&basePm);

    for (uint index = 1; index <= DataPlane::MAX_ODU_ID; index++)
    {
        if (false == oduPmList->IsPmValid(index))
        {
            continue;
        }

        INFN_LOG(SeverityLevel::debug) << "Odu PM for OduId = " << index;

        DataPlane::DpMsOduPMContainer oduAdPm = oduPmList->GetPmData(index);

        //check pm data has valid aid, redis client would throw exception if aid is empty
        if (!oduPmList->IsPmValid(index) || oduAdPm.aid.empty())
        {
            INFN_LOG(SeverityLevel::error) << " Invalid aid for OtuId: " << index;
            continue;
        }

        Chm6OduPm oduPbPm;
        oduPbPm.mutable_base_pm()->mutable_config_id()->set_value(oduAdPm.aid);

        bool lof_alarm = false;
        OtuPbTranslator pbOtuTranslator;
        DpAdapter::OtuFault adFault;
        if (DpOtuMgrSingleton::Instance()->getAdPtr()->getOtuFault(oduAdPm.aid, &adFault, false))
        {
            lof_alarm = pbOtuTranslator.isOtuLofFault(adFault, FAULTDIRECTION_RX);
        }

        OduPbTranslator pbTranslator;
        pbTranslator.pmAdToPb(oduAdPm, lof_alarm, true, oduPbPm);
        if (dp_env::DpEnvSingleton::Instance()->isPmDebug())
        {
            string pmDataStr;
            MessageToJsonString(oduPbPm, &pmDataStr);
            INFN_LOG(SeverityLevel::debug) << pmDataStr;
        }

        try
        {
            TRACE_PM(Chm6OduPm, oduPbPm, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(oduPbPm);
	    )
        }
        catch(std::exception const &excp)
        {
            INFN_LOG(SeverityLevel::info) << "Exception " << excp.what() << " caught in DpMgrRedisObjectStream oduPbPm " << index;
        }
        catch(...)
        {
            INFN_LOG(SeverityLevel::info) << "Exception caught in DpMgrRedisObjectStream oduPbPm " << index;
        }
        oduPmList->UpdatePmValid(index, false); //mark this entry as invalid after transaction finished
    }

    return true;
}

bool DpmsGccCbHandler::OnRcvPm(string className, DcoBasePM &basePm)
{
    INFN_LOG(SeverityLevel::debug) << " Gcc PM Stream Callback triggered";

    DpMsGCCPm* gccPmList = static_cast< DpMsGCCPm *>(&basePm);

    for (int index = 1; index <= DataPlane::MAX_CARRIERS; index++)
    {
        if (false == gccPmList->HasPmData(index))
        {
            continue;
        }

        INFN_LOG(SeverityLevel::debug) << " Gcc PM for CarrierId = " << index;

        DataPlane::DpMsGccPMContainer gccAdPm = gccPmList->GetPmData(index);

        //check pm data has valid aid, redis client would throw exception if aid is empty
        if (!gccPmList->HasPmData(index) || gccAdPm.aid.empty())
        {
            INFN_LOG(SeverityLevel::error) << " Invalid aid for carrierId: " << index;
            continue;
        }

        Chm6GccPm gccPbPm;
        gccPbPm.mutable_base_pm()->mutable_config_id()->set_value(gccAdPm.aid);

        GccPbTranslator pbTranslator;
        pbTranslator.pmAdToPb(gccAdPm, true, gccPbPm);

        if (dp_env::DpEnvSingleton::Instance()->isPmDebug())
        {
            string pmDataStr;
            MessageToJsonString(gccPbPm, &pmDataStr);
            INFN_LOG(SeverityLevel::debug) << pmDataStr;
        }

        try
        {
            TRACE_PM(Chm6GccPm, gccPbPm, TO_REDIS(),
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectStream(gccPbPm);
	    )
        }
        catch(std::exception const &excp)
        {
            INFN_LOG(SeverityLevel::info) << "Exception " << excp.what() << " caught in DpMgrRedisObjectStream gccPbPm " << index;
        }
        catch(...)
        {
            INFN_LOG(SeverityLevel::info) << "Exception caught in DpMgrRedisObjectStream gccPbPm " << index;
        }
    }

    return true;
}


} // namespace DataPlane
