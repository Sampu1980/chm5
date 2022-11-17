/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_GIGE_CLIENT_MGR_H
#define DP_GIGE_CLIENT_MGR_H

#include <memory>
#include <google/protobuf/message.h>

#include "GearBoxAdapter.h"
#include "GearBoxAdapterSim.h"
#include "BcmPm.h"


#include "GigeClientPbTranslator.h"
#include "GigeClientConfigHandler.h"
#include "GigeClientStateCollector.h"
#include "DpObjectMgr.h"
#include "SerdesTuner.h"

#include <SingletonService.h>

namespace DataPlane
{

class DpGigeClientMgr : public DpObjectMgr
{
public:

    DpGigeClientMgr();
    virtual ~DpGigeClientMgr();

    virtual void initialize();

    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    void dumpGigeClientCacheFault(std::string gigeAid, std::ostream& os) { mspGigeClientCollector->dumpCacheFault(gigeAid, os); }
    void dumpGigeClientCachePm   (std::string gigeAid, std::ostream& os) { mspGigeClientCollector->dumpCachePm   (gigeAid, os); }
    void setGigeClientStatePollMode(bool mode, std::ostream& os) { mspGigeClientCollector->setStatePollMode(mode, os); }
    void setGigeClientFaultPollMode(bool mode, std::ostream& os) { mspGigeClientCollector->setFaultPollMode(mode, os); }
    void setGigeClientLldpPollMode (bool mode, std::ostream& os) { mspGigeClientCollector->setLldpPollMode(mode, os); }
    void setGigeClientStatePollInterval(uint32_t intvl, std::ostream& os) { mspGigeClientCollector->setStatePollInterval(intvl, os); }
    void setGigeClientFaultPollInterval(uint32_t intvl, std::ostream& os) { mspGigeClientCollector->setFaultPollInterval(intvl, os); }
    void setGigeClientLldpPollInterval(uint32_t intvl, std::ostream& os)  { mspGigeClientCollector->setLldpPollInterval(intvl, os); }
    void dumpGigeClientPollInfo(std::ostream& os) { mspGigeClientCollector->dumpPollInfo(os); }
    void dumpGigeClientLldpCounters(ostream& os) {mspGigeClientCollector->dumpLldpCounters(os);}
    void getGigeClientGearBoxPm(std::string gigeAid, gearbox::BcmPm& gigeGearBoxPm) { mspGigeClientCollector->getGigeClientGearBoxPm(gigeAid, gigeGearBoxPm); }
    void handleTomPresenceMapUpdate(lccmn_tom::TomPresenceMap* tomMsg) { mspGigeClientCfgHndlr->handleTomPresenceMapUpdate(tomMsg); }
    void handleTomTdaHoldOffMapUpdate(lccmn_tom::TomPresenceMap* tomMsg) { mspGigeClientCfgHndlr->handleTomTdaHoldOffMapUpdate(tomMsg); }
    void readTomPresenceMapFromRedis(void) { mspGigeClientCfgHndlr->readTomPresenceMapFromRedis(); }
    uint getInstalledGigeClients(void) { return mspGigeClientCollector->getInstalledGigeClients(); }
    bool checkBootUpTraffic() { return mspGigeClientCollector->checkBootUpTraffic(); }

    DpAdapter::DcoGigeClientAdapter* getAdPtr() { return mpGigeClientAd; };
    std::shared_ptr<GigeClientConfigHandler> getMspCfgHndlr() { return mspGigeClientCfgHndlr; };
    virtual bool isSubEnabled();
    virtual void updateFaultCache();

protected:

    virtual void start();

    virtual void connectionUpdate();

    virtual void reSubscribe();

    virtual ConfigStatus processConfig(google::protobuf::Message* objMsg, ConfigType cfgType);

    virtual ConfigStatus completeConfig(google::protobuf::Message* objMsg,
                                        ConfigType cfgType,
                                        ConfigStatus status,
                                        std::ostringstream& log);

private:

    // GigeClient PB Translator
    void createGigeClientPbTranslator();

    // GigeClient Config Handler
    void createGigeClientConfigHandler();

    // GigeClient State Collector
    void createGigeClientStateCollector();

    // GigeClient PM Callback registration
    void registerGigeClientPm();
    void unregisterGigeClientPm();

    std::shared_ptr<GigeClientStateCollector>  mspGigeClientCollector;
    std::shared_ptr<GigeClientConfigHandler>   mspGigeClientCfgHndlr;

    GigeClientPbTranslator*           mpPbTranslator;
    DpAdapter::DcoGigeClientAdapter*  mpGigeClientAd;
    gearbox::GearBoxAdIf*             mpGearBoxAd;
    tuning::SerdesTuner*              mpSerdesTuner;
};

typedef SingletonService<DpGigeClientMgr> DpGigeClientMgrSingleton;

} // namespace DataPlane

#endif /* DP_GIGE_CLIENT_MGR_H */
