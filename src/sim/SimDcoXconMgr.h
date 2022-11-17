/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_DCO_XCON_MGR_H_
#define SIM_DCO_XCON_MGR_H_

#include <google/protobuf/message.h>
#include <infinera_dco_xcon/infinera_dco_xcon.pb.h>

#include "SimObjMgr.h"


namespace DpSim
{

class SimDcoXconMgr : public SimObjMgr
{
public:

    SimDcoXconMgr(std::string strName) : SimObjMgr(strName) {}
    virtual ~SimDcoXconMgr() { }

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

private:

    void updateObj(const ::dcoyang::infinera_dco_xcon::Xcons_Xcon* pObjIn,
                         ::dcoyang::infinera_dco_xcon::Xcons_Xcon* pObjOut);
};

} //namespace DpSim

#endif /* SIM_DCO_XCON_MGR_H_ */
