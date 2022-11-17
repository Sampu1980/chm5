/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_DCO_OTU_MGR_H_
#define SIM_DCO_OTU_MGR_H_

#include <google/protobuf/message.h>
#include <infinera_dco_otu/infinera_dco_otu.pb.h>
#include <boost/algorithm/string/predicate.hpp>

#include "SimObjMgr.h"


namespace DpSim
{

class SimDcoOtuMgr : public SimObjMgr
{
public:

    SimDcoOtuMgr(std::string strName) : SimObjMgr(strName) {}
    virtual ~SimDcoOtuMgr() { }

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

    enum SIM_OTU_PARAMS
    {
        SIM_OTU_FAC_RCVD_TTI = 0,
        SIM_OTU_FAULT           ,
        NUM_SIM_OTU_PARAMS
    };

    const boost::unordered_map<SIM_OTU_PARAMS,const char*> simOtuParamsToString = boost::assign::map_list_of
        (SIM_OTU_FAC_RCVD_TTI, "FacRcvdTti" )
        (SIM_OTU_FAULT       , "Fault"      );

private:

    void updateObj(const ::dcoyang::infinera_dco_otu::Otus_Otu* pCarIn,
                         ::dcoyang::infinera_dco_otu::Otus_Otu* pCarOut);
};

} //namespace DpSim

#endif /* SIM_DCO_OTU_MGR_H_ */
