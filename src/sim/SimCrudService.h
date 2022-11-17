/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_CRUD_SERVICE_H_
#define SIM_CRUD_SERVICE_H_

#include <google/protobuf/message.h>

#include "CrudService.h"
#include "dp_common.h"

#include "SimObjMgr.h"
#include "DpEnv.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/unordered_map.hpp>
#include <iostream>


namespace DpSim
{

class SimSbGnmiClient : public ::GnmiClient::SBGnmiClient
{
public:

    SimSbGnmiClient()
        : ::GnmiClient::SBGnmiClient(GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoZynq()))
    {
        initializeSim ();
    }
    ~SimSbGnmiClient() { cleanup(); }

    static std::shared_ptr<SimSbGnmiClient> getInstance()
    {
        if (mspSingleInstance == nullptr)
        {
            mspSingleInstance = std::make_shared<SimSbGnmiClient>();
        }

        return mspSingleInstance;
    }

    void initializeSim();
    void cleanup();

    grpc::Status Create( const google::protobuf::Message* objMsg );
    grpc::Status Read  ( google::protobuf::Message*& objMsg );
    grpc::Status Update( const google::protobuf::Message* objMsg );
    grpc::Status Delete( const google::protobuf::Message* objMsg );

    void dumpObjMgrConfigCache(std::ostream& out, uint objMgrIdx, uint objId);
    void clearObjMgrCache     (std::ostream& out, uint objMgrIdx, uint objId);
    void dumpMgrNames         (std::ostream& out);
    void dumpStateParamNames  (std::ostream& out, std::string objMgrName);
    void setStateParam        (std::ostream& out, std::string objMgrName, uint objId, std::string paramName, std::string valueStr);
    void updateCardEqptFault  (std::ostream& out, std::string faultName, bool set);
    void updateCardPostFault  (std::ostream& out, std::string faultName, bool set);
    void clearAllCardEqptFault(std::ostream& out);
    void clearAllCardPostFault(std::ostream& out);

    enum SIM_OBJ_MGRS
    {
        SO_CARD    = 0,
        SO_CARRIER    ,
        SO_GIGE       ,
        SO_OTU        ,
        SO_ODU        ,
        SO_XCON       ,
        SO_GCC        ,
        NUM_SIM_OBJ_MGRS
    };

    const boost::unordered_map<SIM_OBJ_MGRS,const char*> simObjMgrsToString = boost::assign::map_list_of
        (SO_CARD,    "Card"   )
        (SO_CARRIER, "Carrier")
        (SO_GIGE,    "Gige"   )
        (SO_OTU,     "Otu"    )
        (SO_ODU,     "Odu"    )
        (SO_XCON,    "XCon"   )
        (SO_GCC,     "Gcc"    );


private:

    std::vector<SimObjMgr*>  mvSimObjMgr;
    static std::shared_ptr<SimSbGnmiClient> mspSingleInstance;
};

//typedef SingletonService<SimSbGnmiClient> SimCrudServiceSingleton;

} //namespace DpSim

#endif /* SIM_CRUD_SERVICE_H_ */
