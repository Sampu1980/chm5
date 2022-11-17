/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef XC_CONFIG_HANDLER_H
#define XC_CONFIG_HANDLER_H

#include "DataPlaneMgr.h"
#include "XcPbTranslator.h"
#include "DcoXconAdapter.h"
#include "DpProtoDefs.h"
#include "DpConfigHandler.h"
#include "XCStateCollector.h"

#include "FpgaRegIf.h"
#include "RegIfFactory.h"

using STATUS = chm6_common::Status;


namespace DataPlane {

class XCConfigHandler : public DpConfigHandler
{
public:

    XCConfigHandler(XcPbTranslator* pbTrans,
                    DpAdapter::DcoXconAdapter *pAd,
                    std::shared_ptr<XCStateCollector> spXcCollector,
                    std::shared_ptr<FpgaSacIf> fpgaSacIf);

    virtual ~XCConfigHandler() {}

    virtual ConfigStatus onCreate(chm6_dataplane::Chm6XCConfig* xconMsg);

    virtual ConfigStatus onModify(chm6_dataplane::Chm6XCConfig* xconMsg);

    virtual ConfigStatus onDelete(chm6_dataplane::Chm6XCConfig* xconMsg);

    STATUS deleteXCConfigCleanup(chm6_dataplane::Chm6XCConfig* xconCfg);

    void sendTransactionStateToInProgress(chm6_dataplane::Chm6XCConfig* xconCfg);
    void sendTransactionStateToComplete  (chm6_dataplane::Chm6XCConfig* xconCfg,
                                          STATUS status);

    void createCacheObj(chm6_dataplane::Chm6XCConfig& xconCfg);
    void updateCacheObj(chm6_dataplane::Chm6XCConfig& xconCfg);
    void deleteCacheObj(chm6_dataplane::Chm6XCConfig& xconCfg);

    virtual STATUS sendConfig(google::protobuf::Message& msg);
    virtual STATUS sendDelete(google::protobuf::Message& msg);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg);

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg);

    void dumpXpressXconOhFpga(std::ostream& os);

    bool isXconPresent(std::string configId, ObjectMgrs objId);

private:

    ConfigStatus createXCConfig(chm6_dataplane::Chm6XCConfig* xconCfg);
    ConfigStatus updateXCConfig(chm6_dataplane::Chm6XCConfig* xconCfg);
    ConfigStatus deleteXCConfig(chm6_dataplane::Chm6XCConfig* xconCfg);
    void configFpgaLoOduOhForwarding(uint32_t srcLoOduId, uint32_t dstLoOduId, bool bEnableForwarding);

    XcPbTranslator*             mpPbTranslator;
    DpAdapter::DcoXconAdapter*  mpDcoAd;
    std::shared_ptr<XCStateCollector>  mspXcCollector;
    shared_ptr<FpgaSacIf>              mspFpgaSacIf;
};

} // namespace DataPlane
#endif /* XC_CONFIG_HANDLER_H */
