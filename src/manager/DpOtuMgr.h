/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_OTU_MGR_H
#define DP_OTU_MGR_H

#include <memory>
#include <google/protobuf/message.h>

#include "GearBoxAdapter.h"
#include "GearBoxAdapterSim.h"
#include "BcmPm.h"

#include "FpgaRegIf.h"
#include "RegIfFactory.h"

#include "OtuPbTranslator.h"
#include "OtuConfigHandler.h"
#include "OtuStateCollector.h"
#include "DpObjectMgr.h"

#include <SingletonService.h>

namespace DataPlane
{

class DpOtuMgr : public DpObjectMgr
{
public:

    DpOtuMgr();
    virtual ~DpOtuMgr();

    virtual void initialize();

    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    void dumpOtuCacheState      (std::string otuAid, std::ostream& os) { mspOtuCollector->dumpCacheState      (otuAid, os); }
    void dumpOtuCacheFault      (std::string otuAid, std::ostream& os) { mspOtuCollector->dumpCacheFault      (otuAid, os); }
    void dumpOtuCachePm         (std::string otuAid, std::ostream& os) { mspOtuCollector->dumpCachePm         (otuAid, os); }
    void dumpOtuCacheStateConfig(std::string otuAid, std::ostream& os) { mspOtuCollector->dumpCacheStateConfig(otuAid, os); }
    void dumpOtuSdTracking      (std::string otuAid, std::ostream& os) { mspOtuCollector->dumpSdTracking      (otuAid, os); }
    void dumpOtuTimTracking     (std::string otuAid, std::ostream& os) { mspOtuCollector->dumpTimTracking     (otuAid, os); }

    void setOtuTimAlarmMode (bool mode, std::ostream& os) { mspOtuCollector->setTimAlarmMode(mode, os); }
    void setOtuSdAlarmMode  (bool mode, std::ostream& os) { mspOtuCollector->setSdAlarmMode(mode, os);  }
    void setOtuStatePollMode(bool mode, std::ostream& os) { mspOtuCollector->setStatePollMode(mode, os); }
    void setOtuFaultPollMode(bool mode, std::ostream& os) { mspOtuCollector->setFaultPollMode(mode, os); }
    void setOtuStatePollInterval(uint32_t intvl, std::ostream& os) { mspOtuCollector->setStatePollInterval(intvl, os); }
    void setOtuFaultPollInterval(uint32_t intvl, std::ostream& os) { mspOtuCollector->setFaultPollInterval(intvl, os); }
    void dumpOtuPollInfo(std::ostream& os) { mspOtuCollector->dumpPollInfo(os); }
    void handleTomPresenceMapUpdate(lccmn_tom::TomPresenceMap* tomMsg) { mspOtuCfgHndlr->handleTomPresenceMapUpdate(tomMsg); }
    void readTomPresenceMapFromRedis(void) { mspOtuCfgHndlr->readTomPresenceMapFromRedis(); }
    uint getInstalledOtuClients(void) { return mspOtuCollector->getInstalledOtuClients(); }
    bool checkBootUpTraffic() { return mspOtuCollector->checkBootUpTraffic(); }
    bool isOtuTimFault(std::string otuAid) { return mspOtuCollector->isTimFault(otuAid); }

    DpAdapter::DcoOtuAdapter* getAdPtr() { return mpOtuAd; };

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

    // Otu PB Translator
    void createOtuPbTranslator();

    // Otu Config Handler
    void createOtuConfigHandler();

    // Otu State Collector
    void createOtuStateCollector();

    // Otu PM Callback registration
    void registerOtuPm();
    void unregisterOtuPm();

    std::shared_ptr<OtuStateCollector>  mspOtuCollector;
    std::shared_ptr<OtuConfigHandler>   mspOtuCfgHndlr;

    OtuPbTranslator*            mpPbTranslator;
    DpAdapter::DcoOtuAdapter*   mpOtuAd;
    gearbox::GearBoxAdIf*       mpGearBoxAd;
    tuning::SerdesTuner*        mpSerdesTuner;
    shared_ptr<FpgaSramIf>      mspFpgaSramIf;
};

typedef SingletonService<DpOtuMgr> DpOtuMgrSingleton;

} // namespace DataPlane

#endif /* DP_OTU_MGR_H */
