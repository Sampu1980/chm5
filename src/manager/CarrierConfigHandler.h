/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef CARRIER_CONFIG_HANDLER_H
#define CARRIER_CONFIG_HANDLER_H

#include "DpConfigHandler.h"
#include "CarrierPbTranslator.h"
#include "DcoCarrierAdapter.h"
#include "DpProtoDefs.h"
#include "DcoSecProcCarrierCfgHdlr.h"


namespace DataPlane {

class CarrierConfigHandler : public DpConfigHandler //: public ICallbackHandler
{
public:

    CarrierConfigHandler(CarrierPbTranslator* pbTrans,
                         DpAdapter::DcoCarrierAdapter* pCarAd,
                         std::shared_ptr<DcoSecProcCarrierCfgHdlr> spSecProcHndlr);

    virtual ~CarrierConfigHandler() {}

    virtual ConfigStatus onCreate(chm6_dataplane::Chm6CarrierConfig* carrierMsg);

    virtual ConfigStatus onModify(chm6_dataplane::Chm6CarrierConfig* carrierMsg);

    virtual ConfigStatus onDelete(chm6_dataplane::Chm6CarrierConfig* carrierMsg);

    virtual ConfigStatus onCreate(chm6_dataplane::Chm6ScgConfig* scgMsg);

    STATUS deleteCarrierConfigCleanup(chm6_dataplane::Chm6CarrierConfig* carrierCfg);

    void sendTransactionStateToInProgress(chm6_dataplane::Chm6CarrierConfig* carrierCfg);
    void sendTransactionStateToComplete  (chm6_dataplane::Chm6CarrierConfig* carrierCfg,
                                          STATUS status);
    void sendTransactionStateToInProgress(chm6_dataplane::Chm6ScgConfig* scgCfg);
    void sendTransactionStateToComplete  (chm6_dataplane::Chm6ScgConfig* scgCfg, STATUS status);

    void getChm6CarrierConfigFreqFromCache(std::string aid, double& frequency);

    bool checkBootUpTraffic(void);

    virtual void createCacheObj(chm6_dataplane::Chm6CarrierConfig& carrierMsg);
    virtual void updateCacheObj(chm6_dataplane::Chm6CarrierConfig& carrierMsg);
    virtual void deleteCacheObj(chm6_dataplane::Chm6CarrierConfig& carrierMsg);

    virtual STATUS sendConfig(google::protobuf::Message& msg);
    virtual STATUS sendDelete(google::protobuf::Message& msg);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg);

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg);

private:

    ConfigStatus createCarrierConfig(chm6_dataplane::Chm6CarrierConfig* carrierCfg);
    ConfigStatus updateCarrierConfig(chm6_dataplane::Chm6CarrierConfig* carrierCfg);
    ConfigStatus deleteCarrierConfig(chm6_dataplane::Chm6CarrierConfig* carrierCfg);

    void toggleSyncReadyInSimMode();

    CarrierPbTranslator*           mpPbTranslator;
    DpAdapter::DcoCarrierAdapter*  mpDcoCarrierAd;
    std::shared_ptr<DcoSecProcCarrierCfgHdlr> mspSecProcCfgHndlr;
};

} // namespace DataPlane
#endif
