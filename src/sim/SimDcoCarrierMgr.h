/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_DCO_CARRIER_MGR_H_
#define SIM_DCO_CARRIER_MGR_H_

#include <google/protobuf/message.h>
#include <infinera_dco_carrier/infinera_dco_carrier.pb.h>

#include "SimObjMgr.h"


namespace DpSim
{

class SimDcoCarrierMgr : public SimObjMgr
{
public:

    SimDcoCarrierMgr(std::string strName) : SimObjMgr(strName) {}
    virtual ~SimDcoCarrierMgr() { }

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

    enum SIM_CARRIER_PARAMS
    {
        SIM_CAR_TXSTATE  = 0,
        SIM_CAR_RXSTATE     ,
        SIM_CAR_FECITER     ,
        SIM_CAR_FECOHPERCENT,
        SIM_CAR_CAPACITY    ,
        SIM_CAR_BAUDRATE    ,
        SIM_CAR_APPCODE     ,
        SIM_CAR_LINEMODE    ,
        SIM_CAR_GAINSHARE   ,
        SIM_CAR_TXCDPRECOMP ,
        SIM_CAR_RXCD        ,
        SIM_CAR_FAULT       ,
        NUM_SIM_CARRIER_PARAMS
    };

    const boost::unordered_map<SIM_CARRIER_PARAMS,const char*> simCarrierParamsToString = boost::assign::map_list_of
        (SIM_CAR_TXSTATE     , "TxState"           )
        (SIM_CAR_RXSTATE     , "RxState"           )
        (SIM_CAR_FECITER     , "FecIteration"      )
        (SIM_CAR_FECOHPERCENT, "FecOhPercent"      )
        (SIM_CAR_CAPACITY    , "Capacity"          )
        (SIM_CAR_BAUDRATE    , "BaudRate"          )
        (SIM_CAR_APPCODE     , "AppCode"           )
        (SIM_CAR_LINEMODE    , "LineMode"          )
        (SIM_CAR_GAINSHARE   , "GainShareStatus"   )
        (SIM_CAR_TXCDPRECOMP , "TxCdPreCompStatus" )
        (SIM_CAR_RXCD        , "RxCdStatus"        )
        (SIM_CAR_FAULT       , "Fault"             );

private:

    void updateObj(const ::dcoyang::infinera_dco_carrier::Carriers_Carrier* pCarIn,
                         ::dcoyang::infinera_dco_carrier::Carriers_Carrier* pCarOut);
};

} //namespace DpSim

#endif /* SIM_DCO_CARRIER_MSG_H_ */
