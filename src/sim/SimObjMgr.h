/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef SIM_OBJ_MGR_H_
#define SIM_OBJ_MGR_H_

#include <google/protobuf/message.h>

namespace DpSim
{

class SimObjMgr
{
public:

    SimObjMgr(std::string strName) : mName(strName) {}
    virtual ~SimObjMgr() { clearAllData(); }

    virtual void initialize() {};

    virtual void clearCache(std::ostream& out, uint objId);
    virtual void setStateParam(std::ostream& out, uint objId, std::string paramName, std::string valueStr);

    enum MSG_STAT
    {
        MS_OK         = 0,
        MS_WRONG_TYPE    ,
        MS_ALREADY_EXISTS,
        MS_DOESNT_EXIST  ,
        MS_INVALID_DATA
    };
    virtual MSG_STAT checkAndCreate(const google::protobuf::Message* pObjMsg) = 0;
    virtual MSG_STAT checkAndRead  (      google::protobuf::Message* pObjMsg) = 0;
    virtual MSG_STAT checkAndUpdate(const google::protobuf::Message* pObjMsg) = 0;
    virtual MSG_STAT checkAndDelete(const google::protobuf::Message* pObjMsg) = 0;

    virtual void createDerivedMsgObj(google::protobuf::Message*  pFromObj,
                                     google::protobuf::Message*& pNewObj) = 0;

    virtual void dumpParamNames(std::ostream& out) = 0;
    virtual void createStateMsgObj(google::protobuf::Message* pObjMsg) = 0;
    virtual void setStateMsgObj(google::protobuf::Message* pObjMsg, std::string paramName, std::string valueStr) = 0;

    MSG_STAT addObj(uint id, google::protobuf::Message* pObjMsg, bool isReplace=false);
    bool     getObj(uint id, google::protobuf::Message*& pObjMsg);
    bool     delObj(uint id);

    // Output Dumps....
    virtual void dumpObjConfigCache(std::ostream& out, uint objId);
    virtual void dumpObjMsg(std::ostream& out, google::protobuf::Message* pObjMsg);
    virtual void dumpStateParamNames(std::ostream& out);

    // Card fault test
    virtual void updateCardEqptFault  (std::ostream& out, std::string faultName, bool set)
    {
        out << "Fault name is " << faultName << std::endl;
    }

    virtual void updateCardPostFault  (std::ostream& out, std::string faultName, bool set)
    {
        out << "Fault name is " << faultName << std::endl;
    }

    virtual void clearAllCardEqptFault(std::ostream& out) {}

    virtual void clearAllCardPostFault(std::ostream& out) {}

protected:

    virtual void clearAllData();
    virtual bool clearData(uint objId);
    virtual bool setStateParamData(uint objId, std::string paramName, std::string valueStr);

    std::mutex mMutex;
    typedef std::map<uint, google::protobuf::Message*> TypeObjMsgMap;
    TypeObjMsgMap mMapObjMsg;
    std::string mName;
};

} //namespace DpSim

#endif /* SIM_OBJ_MSG_H_ */
