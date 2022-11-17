/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_XC_MGR_H
#define DP_XC_MGR_H

#include <memory>
#include <google/protobuf/message.h>

#include "FpgaRegIf.h"
#include "RegIfFactory.h"

#include "XcPbTranslator.h"
#include "XCConfigHandler.h"
#include "XCStateCollector.h"
#include "DpObjectMgr.h"

#include <SingletonService.h>

namespace DataPlane
{

class DpXcMgr : public DpObjectMgr
{
public:

    DpXcMgr();
    virtual ~DpXcMgr();

    virtual void initialize();

    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    bool isXconPresent(std::string configId, ObjectMgrs objId) { return mspXcCfgHndlr->isXconPresent(configId, objId); }

    DpAdapter::DcoXconAdapter* getAdPtr() { return mpXcAd; };

    void dumpXpressXconOhFpga(std::ostream& os) {
        mspXcCfgHndlr->dumpXpressXconOhFpga (os); }

protected:

    virtual void start();

    virtual void reSubscribe();
    
    virtual ConfigStatus processConfig(google::protobuf::Message* objMsg, ConfigType cfgType);

    virtual ConfigStatus completeConfig(google::protobuf::Message* objMsg,
                                        ConfigType cfgType,
                                        ConfigStatus status,
                                        std::ostringstream& log);

private:

    // Xc Config Handler and State Collector Create
    void createXcConfigHandler();
    void createXcStateCollector();


    std::shared_ptr<XCConfigHandler>   mspXcCfgHndlr;
    std::shared_ptr<XCStateCollector>  mspXcCollector;

    XcPbTranslator*                    mpPbTranslator;
    DpAdapter::DcoXconAdapter*         mpXcAd;
    shared_ptr<FpgaSacIf>              mspFpgaSacIf;
};

typedef SingletonService<DpXcMgr> DpXcMgrSingleton;

} // namespace DataPlane

#endif /* DP_XC_MGR_H */
