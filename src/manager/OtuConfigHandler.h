/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef OTU_CONFIG_HANDLER_H
#define OTU_CONFIG_HANDLER_H

#include "OtuPbTranslator.h"
#include "DcoOtuAdapter.h"
#include "DpProtoDefs.h"
#include "DpConfigHandler.h"
#include "GearBoxAdapter.h"
#include "GearBoxAdapterSim.h"
#include "SerdesTuner.h"

using STATUS = chm6_common::Status;


namespace DataPlane {

class OtuConfigHandler : public DpConfigHandler
{
public:

    OtuConfigHandler(OtuPbTranslator* pbTrans, DpAdapter::DcoOtuAdapter *pAd, gearbox::GearBoxAdIf* pGearBoxAd, tuning::SerdesTuner* pSerdesTuner);

    virtual ~OtuConfigHandler() {}

    virtual ConfigStatus onCreate(chm6_dataplane::Chm6OtuConfig* otuMsg);

    virtual ConfigStatus onModify(chm6_dataplane::Chm6OtuConfig* otuMsg);

    virtual ConfigStatus onDelete(chm6_dataplane::Chm6OtuConfig* otuMsg);

    STATUS deleteOtuConfigCleanup(chm6_dataplane::Chm6OtuConfig* otuCfg);

    // TomPresenceMap
    void   handleTomPresenceMapUpdate(lccmn_tom::TomPresenceMap* tomMsg);
    void   readTomPresenceMapFromRedis(void);

    void sendTransactionStateToInProgress(chm6_dataplane::Chm6OtuConfig* otuCfg);
    void sendTransactionStateToComplete  (chm6_dataplane::Chm6OtuConfig* otuCfg,
                                          STATUS status);

    void createCacheObj(chm6_dataplane::Chm6OtuConfig& otuCfg);
    void updateCacheObj(chm6_dataplane::Chm6OtuConfig& otuCfg);
    void deleteCacheObj(chm6_dataplane::Chm6OtuConfig& otuCfg);

    virtual STATUS sendConfig(google::protobuf::Message& msg);
    virtual STATUS sendDelete(google::protobuf::Message& msg);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg);

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg);

private:

    ConfigStatus createOtuConfig(chm6_dataplane::Chm6OtuConfig* otuCfg);
    ConfigStatus updateOtuConfig(chm6_dataplane::Chm6OtuConfig* otuCfg);
    ConfigStatus deleteOtuConfig(chm6_dataplane::Chm6OtuConfig* otuCfg);

    STATUS updateGearBoxOtuConfig(chm6_dataplane::Chm6OtuConfig* otuCfg, bool onCreate);

    uint32_t                   mTomPresenceMap;

    OtuPbTranslator*           mpPbTranslator;
    DpAdapter::DcoOtuAdapter*  mpDcoAd;
    gearbox::GearBoxAdIf*      mpGearBoxAd;
    tuning::SerdesTuner*       mpSerdesTuner;
};

} // namespace DataPlane
#endif /* OTU_CONFIG_HANDLER_H */
