/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_DCO_GIGE_MGR_H_
#define SIM_DCO_GIGE_MGR_H_

#include <google/protobuf/message.h>
#include <infinera_dco_client_gige/infinera_dco_client_gige.pb.h>
#include <boost/algorithm/string/predicate.hpp>

#include "SimObjMgr.h"


namespace DpSim
{

class SimDcoGigeMgr : public SimObjMgr
{
public:

    SimDcoGigeMgr(std::string strName) : SimObjMgr(strName) {}
    virtual ~SimDcoGigeMgr() { }

    virtual MSG_STAT checkAndCreate(const google::protobuf::Message* pObjMsg);
    virtual MSG_STAT checkAndRead  (      google::protobuf::Message* pObjMsg);
    virtual MSG_STAT checkAndUpdate(const google::protobuf::Message* pObjMsg);
    virtual MSG_STAT checkAndDelete(const google::protobuf::Message* pObjMsg);

    virtual void createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                     google::protobuf::Message*& pNewObj);

    virtual void createStateMsgObj(google::protobuf::Message* pObjMsg);

    virtual void dumpParamNames(std::ostream& out);
    virtual void setStateMsgObj(google::protobuf::Message* pObjMsg,
                                std::string                paramName,
                                std::string                valueStr);

    enum SIM_GIGE_PARAMS
    {
        SIM_GIGE_MAXMTUSIZE = 0,
        SIM_GIGE_LINKUP        ,
        SIM_GIGE_LLDPPKTTX     ,
        SIM_GIGE_LLDPPKTRX     ,
        SIM_GIGE_FAULT         ,
        NUM_SIM_GIGE_PARAMS
    };

    const boost::unordered_map<SIM_GIGE_PARAMS,const char*> simGigeParamsToString = boost::assign::map_list_of
        (SIM_GIGE_MAXMTUSIZE, "MaxMtuSize")
        (SIM_GIGE_LINKUP    , "LinkUp"    )
        (SIM_GIGE_LLDPPKTTX , "LldpPktTx" )
        (SIM_GIGE_LLDPPKTRX , "LldpPktRx" )
        (SIM_GIGE_FAULT     , "Fault"     );


private:

    void updateObj(const ::dcoyang::infinera_dco_client_gige::ClientsGige_ClientGige* pGigeIn,
                         ::dcoyang::infinera_dco_client_gige::ClientsGige_ClientGige* pGigeOut);
};

} //namespace DpSim

#endif /* SIM_DCO_GIGE_MGR_H_ */
