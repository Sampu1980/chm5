/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_DCO_ODU_MGR_H_
#define SIM_DCO_ODU_MGR_H_

#include <google/protobuf/message.h>
#include <infinera_dco_odu/infinera_dco_odu.pb.h>
#include <boost/algorithm/string/predicate.hpp>

#include "SimObjMgr.h"


namespace DpSim
{

class SimDcoOduMgr : public SimObjMgr
{
public:

    SimDcoOduMgr(std::string strName) : SimObjMgr(strName) {}
    virtual ~SimDcoOduMgr() { }

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

    enum SIM_ODU_PARAMS
    {
        SIM_ODU_FAC_RCVD_TTI = 0,
        SIM_ODU_PAYLOAD_TYPE    ,
        SIM_ODU_FAULT           ,
        NUM_SIM_ODU_PARAMS
    };

    const boost::unordered_map<SIM_ODU_PARAMS,const char*> simOduParamsToString = boost::assign::map_list_of
        (SIM_ODU_FAC_RCVD_TTI, "FacRcvdTti" )
        (SIM_ODU_PAYLOAD_TYPE, "PayloadType")
        (SIM_ODU_FAULT       , "Fault"      );


private:

    void updateObj(const ::dcoyang::infinera_dco_odu::Odus_Odu* pCarIn,
                         ::dcoyang::infinera_dco_odu::Odus_Odu* pCarOut);
};

} //namespace DpSim

#endif /* SIM_DCO_ODU_MGR_H_ */
