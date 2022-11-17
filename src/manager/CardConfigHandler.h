/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef CARD_CONFIG_HANDLER_H
#define CARD_CONFIG_HANDLER_H

#include "CardPbTranslator.h"
#include "DcoCardAdapter.h"
#include "DpProtoDefs.h"
#include "DpConfigHandler.h"
#include "DcoSecProcCardCfgHdlr.h"

#include <google/protobuf/message.h>


namespace DataPlane {

class CardConfigHandler : public DpConfigHandler
{
public:

    CardConfigHandler(CardPbTranslator* pbTrans,
                      DpAdapter::DcoCardAdapter *pCardAd,
                      std::shared_ptr<DcoSecProcCardCfgHdlr> spSecProcCfgHndlr);

    virtual ~CardConfigHandler() {}

    virtual void onCreate(chm6_common::Chm6DcoConfig* pMsg);

    virtual void onModify(chm6_common::Chm6DcoConfig* pMsg);

    virtual void onDelete(chm6_common::Chm6DcoConfig* pMsg);

    void createCacheObj(chm6_common::Chm6DcoConfig& pMsg);
    void updateCacheObj(chm6_common::Chm6DcoConfig& pMsg);
    void deleteCacheObj(chm6_common::Chm6DcoConfig& pMsg);

    virtual STATUS sendConfig(google::protobuf::Message& msg);

    virtual bool isMarkForDelete(const google::protobuf::Message& msg) { return false; } // not needing support card delete currently }

    virtual void dumpCacheMsg(std::ostream& out, google::protobuf::Message& msg);

private:

    void UpdateCardConfig(chm6_common::Chm6DcoConfig* pMsg);

    void UpdateDcoShutdownCompleteState();

    CardPbTranslator          *mpPbTranslator;
    DpAdapter::DcoCardAdapter *mpDcoCardAd;
    std::shared_ptr<DcoSecProcCardCfgHdlr>  mspDcoSecProcCardCfgHndlr;
};

} // namespace DataPlane

#endif // CARD_CONFIG_HANDLER_H
