/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_ODU_MGR_H
#define DP_ODU_MGR_H

#include <memory>
#include <google/protobuf/message.h>

#include "FpgaRegIf.h"
#include "RegIfFactory.h"

#include "OduPbTranslator.h"
#include "OduConfigHandler.h"
#include "OduStateCollector.h"
#include "DpObjectMgr.h"

#include <SingletonService.h>

namespace DataPlane
{

class DpOduMgr : public DpObjectMgr
{
public:

    DpOduMgr();
    virtual ~DpOduMgr();

    virtual void initialize();

    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    void dumpOduCacheState      (std::string oduAid, std::ostream& os) { mspOduCollector->dumpCacheState      (oduAid, os); }
    void dumpOduCacheFault      (std::string oduAid, std::ostream& os) { mspOduCollector->dumpCacheFault      (oduAid, os); }
    void dumpOduCachePm         (std::string oduAid, std::ostream& os) { mspOduCollector->dumpCachePm         (oduAid, os); }
    void dumpOduCacheStateConfig(std::string oduAid, std::ostream& os) { mspOduCollector->dumpCacheStateConfig(oduAid, os); }
    void dumpOduSdTracking      (std::string oduAid, std::ostream& os) { mspOduCollector->dumpSdTracking      (oduAid, os); }

    void setOduTimAlarmMode (bool mode, std::ostream& os) { mspOduCollector->setTimAlarmMode(mode, os); }
    void setOduSdAlarmMode  (bool mode, std::ostream& os) { mspOduCollector->setSdAlarmMode(mode, os);  }
    void setOduStatePollMode(bool mode, std::ostream& os) { mspOduCollector->setStatePollMode(mode, os); }
    void setOduFaultPollMode(bool mode, std::ostream& os) { mspOduCollector->setFaultPollMode(mode, os); }
    void setOduStatePollInterval(uint32_t intvl, std::ostream& os) { mspOduCollector->setStatePollInterval(intvl, os); }
    void setOduFaultPollInterval(uint32_t intvl, std::ostream& os) { mspOduCollector->setFaultPollInterval(intvl, os); }
    void dumpOduPollInfo(std::ostream& os) { mspOduCollector->dumpPollInfo(os); }
    bool checkBootUpTraffic() { return mspOduCollector->checkBootUpTraffic(); }

    DpAdapter::DcoOduAdapter* getAdPtr() { return mpOduAd; };
    std::shared_ptr<OduConfigHandler> getMspCfgHndlr() { return mspOduCfgHndlr; };

protected:

    virtual void start();

    virtual void connectionUpdate();

    virtual void reSubscribe();

    virtual ConfigStatus processConfig(google::protobuf::Message* objMsg, ConfigType cfgType);

    virtual ConfigStatus completeConfig(google::protobuf::Message* objMsg,
                                        ConfigType cfgType,
                                        ConfigStatus status,
                                        std::ostringstream& log);

    virtual bool isSubEnabled();

    virtual void updateFaultCache();

private:

    // Odu PB Translator
    void createOduPbTranslator();

    // Odu Config Handler
    void createOduConfigHandler();

    // Odu State Collector
    void createOduStateCollector();

    // Odu PM Callback registration
    void registerOduPm();
    void unregisterOduPm();

    std::shared_ptr<OduStateCollector>  mspOduCollector;
    std::shared_ptr<OduConfigHandler>   mspOduCfgHndlr;

    OduPbTranslator*                    mpPbTranslator;
    DpAdapter::DcoOduAdapter*           mpOduAd;
    shared_ptr<FpgaSramIf>              mspFpgaSramIf;
};

typedef SingletonService<DpOduMgr> DpOduMgrSingleton;

} // namespace DataPlane

#endif /* DP_ODU_MGR_H */
