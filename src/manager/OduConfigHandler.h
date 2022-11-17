/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef ODU_CONFIG_HANDLER_H
#define ODU_CONFIG_HANDLER_H

#include "OduPbTranslator.h"
#include "DcoOduAdapter.h"
#include "DpProtoDefs.h"
#include "DpConfigHandler.h"
#include "OduStateCollector.h"

using STATUS = chm6_common::Status;


namespace DataPlane {

class OduConfigHandler : public DpConfigHandler
{
public:

    OduConfigHandler(OduPbTranslator* pbTrans, DpAdapter::DcoOduAdapter *pAd, std::shared_ptr<OduStateCollector> spStateCollector);

    virtual ~OduConfigHandler() {}

    virtual ConfigStatus onCreate(chm6_dataplane::Chm6OduConfig* oduMsg);

    virtual ConfigStatus onModify(chm6_dataplane::Chm6OduConfig* oduMsg);

    virtual ConfigStatus onDelete(chm6_dataplane::Chm6OduConfig* oduMsg);

    STATUS deleteOduConfigCleanup(chm6_dataplane::Chm6OduConfig* oduCfg);

    void sendTransactionStateToInProgress(chm6_dataplane::Chm6OduConfig* oduCfg);
    void sendTransactionStateToComplete  (chm6_dataplane::Chm6OduConfig* oduCfg,
                                          STATUS status);

    void createCacheObj(chm6_dataplane::Chm6OduConfig& oduCfg);
    void updateCacheObj(chm6_dataplane::Chm6OduConfig& oduCfg);
    void deleteCacheObj(chm6_dataplane::Chm6OduConfig& oduCfg);

    void getChm6OduConfigForceMsFromCache(std::string aid, Direction dir, MaintenanceSignal& msMode);
    void getChm6OduConfigPrbsGenFromCache(std::string aid, Direction dir, PrbsMode& prbsMode);

    virtual STATUS sendConfig(google::protobuf::Message& msg);
    virtual STATUS sendDelete(google::protobuf::Message& msg);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg);

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg);

private:

    ConfigStatus createOduConfig(chm6_dataplane::Chm6OduConfig* oduCfg);
    ConfigStatus updateOduConfig(chm6_dataplane::Chm6OduConfig* oduCfg);
    ConfigStatus deleteOduConfig(chm6_dataplane::Chm6OduConfig* oduCfg);

    OduPbTranslator*           mpPbTranslator;
    DpAdapter::DcoOduAdapter*  mpDcoAd;
    std::shared_ptr<OduStateCollector>  mspOduCollector;
};

} // namespace DataPlane
#endif /* ODU_CONFIG_HANDLER_H */
