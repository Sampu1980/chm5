/**************************************************************************
   Copyright (c) 2021 Infinera
**************************************************************************/

#ifndef GCC_CONFIG_HANDLER_H
#define GCC_CONFIG_HANDLER_H

#include "DpConfigHandler.h"
#include "DcoGccControlAdapter.h" 
#include "DpProtoDefs.h"



namespace DataPlane {

class GccConfigHandler : public DpConfigHandler //: public ICallbackHandler
{
public:

    GccConfigHandler(DpAdapter::DcoGccControlAdapter* pGccAd);

    virtual ~GccConfigHandler() {}

    virtual ConfigStatus onCreate(VlanConfig* vlanCfg);

    virtual ConfigStatus onModify(VlanConfig* vlanCfg);

    virtual ConfigStatus onDelete(VlanConfig* vlanCfg);

    STATUS deleteGccConfigCleanup(VlanConfig* vlanCfg);

    void sendTransactionStateToInProgress(VlanConfig* vlanCfg);
    void sendTransactionStateToComplete  (VlanConfig* vlanCfg , STATUS status);

    virtual void createCacheObj(VlanConfig& vlanCfg);
    virtual void updateCacheObj(VlanConfig& vlanCfg);
    virtual void deleteCacheObj(VlanConfig& vlanCfg);

    virtual STATUS sendConfig(google::protobuf::Message& msg);
    virtual STATUS sendDelete(google::protobuf::Message& msg);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg);

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg);

    void createPcpVlanConfig(VlanConfig* vlanCfg);
    bool createCcVlanConfig(VlanConfig* vlanCfg);
    void deletePcpVlanConfig(VlanConfig* vlanCfg);
    bool deleteCcVlanConfig(VlanConfig* vlanCfg);
private:

    int  aidToCarrierId(std::string vlanAid);

    DpAdapter::DcoGccControlAdapter* mpGccAd;
};

} // namespace DataPlane
#endif // GCC_CONFIG_HANDLER_H
