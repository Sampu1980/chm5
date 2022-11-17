/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_CARRIER_MGR_H
#define DP_CARRIER_MGR_H

#include <memory>
#include <google/protobuf/message.h>

#include "FpgaRegIf.h"
#include "RegIfFactory.h"

#include "CarrierConfigHandler.h"
#include "DcoSecProcCarrierCfgHdlr.h"
#include "CarrierStateCollector.h"
#include "DpObjectMgr.h"

#include <SingletonService.h>

namespace DataPlane
{

class DpCarrierMgr : public DpObjectMgr
{
public:

    DpCarrierMgr();
    virtual ~DpCarrierMgr();

    virtual void initialize();

    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    void dumpCarrierCacheFault(std::string carAid, std::ostream& os) { mspCarrierCollector->dumpCacheFault(carAid, os); }
    void setCarrierStatePollMode(bool mode, std::ostream& os) { mspCarrierCollector->setStatePollMode(mode, os); }
    void setCarrierFaultPollMode(bool mode, std::ostream& os) { mspCarrierCollector->setFaultPollMode(mode, os); }
    void setCarrierStatePollInterval(uint32_t intvl, std::ostream& os) { mspCarrierCollector->setStatePollInterval(intvl, os); }
    void setCarrierFaultPollInterval(uint32_t intvl, std::ostream& os) { mspCarrierCollector->setFaultPollInterval(intvl, os); }
    void dumpCarrierPollInfo(std::ostream& os) { mspCarrierCollector->dumpPollInfo(os); }
    bool checkBootUpTraffic() { return mspCarrierCollector->checkBootUpTraffic(); }

    DpAdapter::DcoCarrierAdapter* getAdPtr() { return mpCarrierAd; };
    std::shared_ptr<CarrierConfigHandler> getMspCfgHndlr() { return mspCarrierCfgHndlr; };
    std::shared_ptr<DcoSecProcCarrierCfgHdlr>  GetmspSecProcCarrierCfgHdlr()
    {
      return mspSecProcCarrierCfgHdlr;
    }
    std::shared_ptr<CarrierStateCollector> GetMspStateCollector()
    {
        return mspCarrierCollector;
    }

    virtual bool isSubscribeConnUp(DcoCpuConnectionsType connId);

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

    virtual ConfigStatus processConfigScg(google::protobuf::Message* objMsg, ConfigType cfgType);

    virtual ConfigStatus completeConfigScg(google::protobuf::Message* objMsg,
                                           ConfigType cfgType,
                                           ConfigStatus status,
                                           std::ostringstream& log);

private:

    // Carrier PB Translator
    void createCarrierPbTranslator();

    // Carrier Config Handler
    void createCarrierConfigHandler();

    // Carrier State Collector
    void createCarrierStateCollector();

    //Handling create of Dco Secproc carrier config handler
    void CreateSecProcCarrierCfgHdlr();

    // Carrier PM Callback registration
    void registerCarrierPm();
    void unregisterCarrierPm();

    std::shared_ptr<CarrierStateCollector>  mspCarrierCollector;
    std::shared_ptr<CarrierConfigHandler>   mspCarrierCfgHndlr;

    CarrierPbTranslator                       *mpPbTranslator;
    DpAdapter::DcoCarrierAdapter              *mpCarrierAd;
    DpAdapter::DcoSecProcCarrierAdapter       *mpSecProcCarrierAd;
    std::shared_ptr<DcoSecProcCarrierCfgHdlr>  mspSecProcCarrierCfgHdlr;
    shared_ptr<FpgaSramIf>                     mspFpgaSramIf;

    bool mIsConfigScg;

};

typedef SingletonService<DpCarrierMgr> DpCarrierMgrSingleton;

} // namespace DataPlane

#endif /* DP_CARRIER_MGR_H */
