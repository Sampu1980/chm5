/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>

#include <infinera_dco_carrier/infinera_dco_carrier.pb.h>

#include "SimCrudService.h"
#include "CrudService.h"
#include "DataPlaneMgrLog.h"
#include "SimDcoCarrierMgr.h"
#include "SimDcoCardMgr.h"
#include "SimDcoGigeMgr.h"
#include "SimDcoOtuMgr.h"
#include "SimDcoOduMgr.h"
#include "SimDcoXconMgr.h"
#include "SimDcoGccMgr.h"
#include "InfnLogger.h"

namespace DpSim
{

std::shared_ptr<SimSbGnmiClient> SimSbGnmiClient::mspSingleInstance;

void SimSbGnmiClient::initializeSim()
{
    INFN_LOG(SeverityLevel::info) << "";

    // Size and insert sim objects at specific indexes

    mvSimObjMgr.resize(NUM_SIM_OBJ_MGRS);

    // Create/Add Card
    SimDcoCardMgr* pCardMsg = new SimDcoCardMgr("SimDcoCard");
    mvSimObjMgr[SO_CARD] = ((SimObjMgr*)pCardMsg);

    // Create/Add Carrier
    SimDcoCarrierMgr* pCarMsg = new SimDcoCarrierMgr("SimDcoCarrier");
    mvSimObjMgr[SO_CARRIER] = ((SimObjMgr*)pCarMsg);

    // Create/Add Gige
    SimDcoGigeMgr* pGigeMsg = new SimDcoGigeMgr("SimDcoGige");
    mvSimObjMgr[SO_GIGE] = ((SimObjMgr*)pGigeMsg);

    // Create/Add Otu
    SimDcoOtuMgr* pOtuMsg = new SimDcoOtuMgr("SimDcoOtu");
    mvSimObjMgr[SO_OTU] = ((SimObjMgr*)pOtuMsg);

    // Create/Add Odu
    SimDcoOduMgr* pOduMsg = new SimDcoOduMgr("SimDcoOdu");
    mvSimObjMgr[SO_ODU] = ((SimObjMgr*)pOduMsg);

    // Create/Add Odu
    SimDcoXconMgr* pXcMsg = new SimDcoXconMgr("SimDcoXcon");
    mvSimObjMgr[SO_XCON] = ((SimObjMgr*)pXcMsg);

    // Create/Add Gcc
    SimDcoGccMgr* pGccMsg = new SimDcoGccMgr("SimDcoGcc");
    mvSimObjMgr[SO_GCC] = ((SimObjMgr*)pGccMsg);

    INFN_LOG(SeverityLevel::info) << "DONE";
}

void SimSbGnmiClient::cleanup()
{
    INFN_LOG(SeverityLevel::info) << "";
    std::vector<SimObjMgr*>::iterator it;
    for(it = mvSimObjMgr.begin(); it != mvSimObjMgr.end(); ++it)
    {
        INFN_LOG(SeverityLevel::info) << "Deleting SimObj";
        delete *it;
    }
    mvSimObjMgr.clear();
}

grpc::Status SimSbGnmiClient::Create( const google::protobuf::Message* pObjMsg )
{
    INFN_LOG(SeverityLevel::info) << "";

    grpc::Status status;

    std::vector<SimObjMgr*>::iterator it;
    for(it = mvSimObjMgr.begin(); it != mvSimObjMgr.end(); ++it)
    {
        if  ((*it)->checkAndCreate(pObjMsg) == SimObjMgr::MS_OK)
        {
            return grpc::Status::OK;
        }
    }

    INFN_LOG(SeverityLevel::info) << "Object not found";

    // todo: Can we return other error codes?? Looks like this is only one??
    return grpc::Status::CANCELLED;
}

grpc::Status SimSbGnmiClient::Read( google::protobuf::Message*& pObjMsg )
{
    //INFN_LOG(SeverityLevel::info) << "";

    grpc::Status status;

    std::vector<SimObjMgr*>::iterator it;
    for(it = mvSimObjMgr.begin(); it != mvSimObjMgr.end(); ++it)
    {
        if  ((*it)->checkAndRead(pObjMsg) == SimObjMgr::MS_OK)
        {
            return grpc::Status::OK;
        }
    }

    INFN_LOG(SeverityLevel::info) << "Object not found";

    // todo: Can we return other error codes?? Looks like this is only one??
    return grpc::Status::CANCELLED;
}

grpc::Status SimSbGnmiClient::Update( const google::protobuf::Message* pObjMsg )
{
    INFN_LOG(SeverityLevel::info) << "";

    grpc::Status status;

    std::vector<SimObjMgr*>::iterator it;
    for(it = mvSimObjMgr.begin(); it != mvSimObjMgr.end(); ++it)
    {
        if  ((*it)->checkAndUpdate(pObjMsg) == SimObjMgr::MS_OK)
        {
            return grpc::Status::OK;
        }
    }

    INFN_LOG(SeverityLevel::info) << "Object not found";

    // todo: Can we return other error codes?? Looks like this is only one??
    return grpc::Status::CANCELLED;
}

grpc::Status SimSbGnmiClient::Delete( const google::protobuf::Message* pObjMsg )
{
    INFN_LOG(SeverityLevel::info) << "";

    grpc::Status status;

    std::vector<SimObjMgr*>::iterator it;
    for(it = mvSimObjMgr.begin(); it != mvSimObjMgr.end(); ++it)
    {
        SimObjMgr::MSG_STAT stat = (*it)->checkAndDelete(pObjMsg);
        if  ((stat == SimObjMgr::MS_OK) ||
             (stat == SimObjMgr::MS_DOESNT_EXIST))
        {
            return grpc::Status::OK;
        }
    }

    INFN_LOG(SeverityLevel::info) << "Object not found";

    // todo: Can we return other error codes?? Looks like this is only one??
    return grpc::Status::CANCELLED;
}

// objMgrIdx - if it is max or higher, then will dump all obj mgrs
// objId     - if it is 0 then will dump all objects in cache
//                otherwise will dump cache data of Id only
void SimSbGnmiClient::dumpObjMgrConfigCache(std::ostream& out, uint objMgrIdx, uint objId)
{
    if (objMgrIdx >= NUM_SIM_OBJ_MGRS)
    {
        for (uint i = 0; i < NUM_SIM_OBJ_MGRS; i++)
        {
            if (SO_GCC == i) continue;  // Skip for now

            mvSimObjMgr[i]->dumpObjConfigCache(out, objId);
        }
    }
    else
    {
        mvSimObjMgr[objMgrIdx]->dumpObjConfigCache(out, objId);
    }
}

// objMgrIdx - if it is max or higher, then will dump all obj mgrs
// objId     - if it is 0 then will dump all objects in cache
//                otherwise will dump cache data of Id only
void SimSbGnmiClient::clearObjMgrCache(std::ostream& out, uint objMgrIdx, uint objId)
{
    if (objMgrIdx >= NUM_SIM_OBJ_MGRS)
    {
        for (uint i = 0; i < NUM_SIM_OBJ_MGRS; i++)
        {
            mvSimObjMgr[i]->clearCache(out, objId);
        }
    }
    else
    {
        mvSimObjMgr[objMgrIdx]->clearCache(out, objId);
    }
}


void SimSbGnmiClient::dumpMgrNames(std::ostream& out)
{
    for (uint i = 0; i < NUM_SIM_OBJ_MGRS; i++)
    {
        SIM_OBJ_MGRS simObjMgr = static_cast<SIM_OBJ_MGRS>(i);
        out << "[" << i << "]  " << simObjMgrsToString.at(simObjMgr) << std::endl;
    }
}


void SimSbGnmiClient::dumpStateParamNames(std::ostream& out, std::string objMgrName)
{
    for (uint i = 0; i < NUM_SIM_OBJ_MGRS; i++)
    {
        SIM_OBJ_MGRS simObjMgr = static_cast<SIM_OBJ_MGRS>(i);
        if (boost::iequals(objMgrName, simObjMgrsToString.at(simObjMgr)))
        {
            mvSimObjMgr[i]->dumpStateParamNames(out);
            break;
        }
    }
}

void SimSbGnmiClient::setStateParam(std::ostream& out, std::string objMgrName, uint objId, std::string paramName, std::string valueStr)
{
    for (uint i = 0; i < NUM_SIM_OBJ_MGRS; i++)
    {
        SIM_OBJ_MGRS simObjMgr = static_cast<SIM_OBJ_MGRS>(i);
        if (boost::iequals(objMgrName, simObjMgrsToString.at(simObjMgr)))
        {
            mvSimObjMgr[i]->setStateParam(out, objId, paramName, valueStr);
            break;
        }
    }
}

void SimSbGnmiClient::updateCardEqptFault  (std::ostream& out, std::string faultName, bool set)
{
    // Set eqpt fault to Card
    mvSimObjMgr[SO_CARD]->updateCardEqptFault(out, faultName, set);
}

void SimSbGnmiClient::updateCardPostFault  (std::ostream& out, std::string faultName, bool set)
{
    // Set post fault to Card
    mvSimObjMgr[SO_CARD]->updateCardPostFault(out, faultName, set);
}

void SimSbGnmiClient::clearAllCardEqptFault(std::ostream& out)
{
    mvSimObjMgr[SO_CARD]->clearAllCardEqptFault(out);
}

void SimSbGnmiClient::clearAllCardPostFault(std::ostream& out)
{
    mvSimObjMgr[SO_CARD]->clearAllCardPostFault(out);
}

} //namespace DpSim

