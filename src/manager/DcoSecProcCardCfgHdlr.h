/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef DCO_SECPROC_CARD_CFG_HDLR_H
#define DCO_SECPROC_CARD_CFG_HDLR_H

#include "DcoSecProcAdapter.h"
#include "DpProtoDefs.h"

#include <google/protobuf/message.h>


namespace DataPlane {

    //Handles callbacks for the changes in CHM6 Dco card config object
    //Passes the change to DCO security processor interface
    class DcoSecProcCardCfgHdlr
    {
        public:

            DcoSecProcCardCfgHdlr();

            virtual ~DcoSecProcCardCfgHdlr() {}

            virtual void onCreate(chm6_common::Chm6DcoConfig* pMsg);

            virtual void onModify(chm6_common::Chm6DcoConfig* pMsg);

            virtual void onDelete(chm6_common::Chm6DcoConfig* pMsg);

    };

} // namespace DataPlane

#endif // CARD_CONFIG_HANDLER_H
