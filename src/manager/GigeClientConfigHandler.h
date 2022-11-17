/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef GIGE_CLIENT_CONFIG_HANDLER_H
#define GIGE_CLIENT_CONFIG_HANDLER_H

#include "GigeClientPbTranslator.h"
#include "DcoGigeClientAdapter.h"
#include "GearBoxAdapter.h"
#include "GearBoxAdapterSim.h"
#include "DpProtoDefs.h"
#include "SerdesTuner.h"
#include "DpConfigHandler.h"

using STATUS = chm6_common::Status;


namespace DataPlane {

class GigeClientConfigHandler : public DpConfigHandler
{
public:

    GigeClientConfigHandler(GigeClientPbTranslator* pbTrans, DpAdapter::DcoGigeClientAdapter* pDcoAd, gearbox::GearBoxAdIf* pGearBoxAd,
                            tuning::SerdesTuner* pSerdesTuner);

    virtual ~GigeClientConfigHandler() {}

    virtual ConfigStatus onCreate(chm6_dataplane::Chm6GigeClientConfig* gigeMsg);

    virtual ConfigStatus onModify(chm6_dataplane::Chm6GigeClientConfig* gigeMsg);

    virtual ConfigStatus onDelete(chm6_dataplane::Chm6GigeClientConfig* gigeMsg);

    STATUS deleteGigeClientConfigCleanup(chm6_dataplane::Chm6GigeClientConfig* gigeCfg);

    // TomPresenceMap
    void   handleTomPresenceMapUpdate(lccmn_tom::TomPresenceMap* tomMsg);
    void   handleTomTdaHoldOffMapUpdate(lccmn_tom::TomPresenceMap* tomMsg);
    void   readTomPresenceMapFromRedis(void);

    void sendTransactionStateToInProgress(chm6_dataplane::Chm6GigeClientConfig* gigeClientCfg);
    void sendTransactionStateToComplete  (chm6_dataplane::Chm6GigeClientConfig* gigeClientCfg,
                                          STATUS status);

    virtual void createCacheObj(chm6_dataplane::Chm6GigeClientConfig& gigeMsg);
    virtual void updateCacheObj(chm6_dataplane::Chm6GigeClientConfig& gigeMsg);
    virtual void deleteCacheObj(chm6_dataplane::Chm6GigeClientConfig& gigeMsg);

    void getChm6GigeClientConfigForceMsFromCache(std::string aid, Direction dir, MaintenanceSignal& msMode);
    void getChm6GigeClientConfigAutoMsFromCache(std::string aid, Direction dir, MaintenanceSignal& msMode);
    void getChm6GigeClientConfigTestSigGenFromCache(std::string aid, Direction dir, bool& testSigGen);
    void getChm6GigeClientConfigTestSigMonFromCache(std::string aid, bool& testSigMon);

    virtual STATUS sendConfig(google::protobuf::Message& msg);
    virtual STATUS sendDelete(google::protobuf::Message& msg);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg);

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg);

private:

    ConfigStatus createGigeClientConfig(chm6_dataplane::Chm6GigeClientConfig* gigeCfg);
    ConfigStatus updateGigeClientConfig(chm6_dataplane::Chm6GigeClientConfig* gigeCfg);
    ConfigStatus deleteGigeClientConfig(chm6_dataplane::Chm6GigeClientConfig* gigeCfg);

    STATUS updateGearBoxGigeClientConfig(chm6_dataplane::Chm6GigeClientConfig* gigeCfg, bool onCreate);

    uint32_t                          mTomPresenceMap;

    GigeClientPbTranslator*           mpPbTranslator;
    DpAdapter::DcoGigeClientAdapter*  mpDcoAd;
    gearbox::GearBoxAdIf*             mpGearBoxAd;
    tuning::SerdesTuner*              mpSerdesTuner;
};

} // namespace DataPlane
#endif /* GIGE_CLIENT_CONFIG_HANDLER_H */
