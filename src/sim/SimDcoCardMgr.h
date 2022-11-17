/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_DCO_CARD_MGR_H_
#define SIM_DCO_CARD_MGR_H_

#include <google/protobuf/message.h>
#include <infinera_dco_card/infinera_dco_card.pb.h>
#include <boost/unordered_map.hpp>
#include <boost/assign.hpp>

#include "SimObjMgr.h"


namespace DpSim
{

class SimDcoCardMgr : public SimObjMgr
{
public:

    SimDcoCardMgr(std::string strName) : SimObjMgr(strName) {}
    virtual ~SimDcoCardMgr() { }

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

    void updateCardEqptFault(std::ostream& out, std::string faultName, bool set);
    void updateCardPostFault(std::ostream& out, std::string faultName, bool set);
    void clearAllCardEqptFault(std::ostream& out);
    void clearAllCardPostFault(std::ostream& out);

    enum SIM_CARD_PARAMS
    {
        SIM_CARD_ZYNQ_FW_VER = 0,
        SIM_CARD_DCO_CAP        ,
        SIM_CARD_STATE          ,
        NUM_SIM_CARD_PARAMS
    };

    const boost::unordered_map<SIM_CARD_PARAMS,const char*> simCardParamsToString = boost::assign::map_list_of
        (SIM_CARD_ZYNQ_FW_VER , "ZynqFwVer"       )
        (SIM_CARD_DCO_CAP     , "DcoCapabilities" )
        (SIM_CARD_STATE       , "DcoCardState" );
private:

    void updateObj(const ::dcoyang::infinera_dco_card::DcoCard* pCarIn,
                         ::dcoyang::infinera_dco_card::DcoCard* pCarOut);
};

} //namespace DpSim

#endif /* SIM_DCO_CARD_MGR_H_ */
