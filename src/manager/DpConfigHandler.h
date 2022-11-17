/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_CONFIG_HANDLER_H
#define DP_CONFIG_HANDLER_H

#include <google/protobuf/message.h>

#include "DpProtoDefs.h"

using STATUS = chm6_common::Status;

namespace DataPlane
{

enum class ConfigStatus
{
    SUCCESS,
    FAIL_DATA,
    FAIL_CONNECT,
    FAIL_OTHER
};

extern std::string statToString(ConfigStatus stat);

class DpConfigHandler
{
public:
    DpConfigHandler()
    {}
    virtual ~DpConfigHandler() {}

    // Transaction Error Reporting
    void        setTransactionErrorString(std::string errorString) { mTransactionErrorString.assign(errorString); }
    std::string getTransactionErrorString() { return mTransactionErrorString; }

    virtual void createCacheObj(google::protobuf::Message& msg) {}
    virtual void updateCacheObj(google::protobuf::Message& msg) {}
    virtual void deleteCacheObj(google::protobuf::Message& msg) {}

    virtual void clearCache();
    virtual void clearCache(std::string configId);

    virtual bool isCacheExist(std::string configId);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg) = 0;

    virtual STATUS sendConfig(google::protobuf::Message& msg) { return STATUS::STATUS_UNSPECIFIED; }
    virtual STATUS sendDelete(google::protobuf::Message& msg) { return STATUS::STATUS_UNSPECIFIED; }

    virtual void syncConfig();

    void dumpCacheObj(std::ostream& out, std::string configId);
    void dumpCache(std::ostream& out);
    void listCache(std::ostream& out);

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg) {}

protected:

    void addCacheObj(std::string configId, google::protobuf::Message*  pMsg);
    int  getCacheObj(std::string configId, google::protobuf::Message*& pMsg);
    int  getCacheObjContainsId(std::string configId, google::protobuf::Message*& pMsg);

    std::string toString(STATUS status) { std::string statusStr = (STATUS::STATUS_SUCCESS == status) ? "SUCCESS" : "FAIL"; return statusStr; };

    std::string mTransactionErrorString;

    std::recursive_mutex mMutex;
    typedef std::map<std::string, google::protobuf::Message*> TypeCfgMsgMap;
    TypeCfgMsgMap mMapCfgMsg;

};

} // namespace DataPlane

#endif /* DP_CONFIG_HANDLER_H */
