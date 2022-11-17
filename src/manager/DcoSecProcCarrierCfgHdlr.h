/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef DCO_SECPROC_CARRIER_CFG_HDLR_H
#define DCO_SECPROC_CARRIER_CFG_HDLR_H

#include "DcoSecProcCarrierAdapter.h"
#include "DpConfigHandler.h"
#include "DpProtoDefs.h"

#include <google/protobuf/message.h>


namespace DataPlane {

    //Handles callbacks for the changes in CHM6 Dco Carrier config object
    //Passes the change to DCO security processor interface
    class DcoSecProcCarrierCfgHdlr
    {
        public:

            DcoSecProcCarrierCfgHdlr(DpAdapter::DcoSecProcCarrierAdapter* pAd);

            virtual ~DcoSecProcCarrierCfgHdlr() {}

            /*
            * Callback function called on create of carrier object in chm6 redis
            * @param : Carrier protobuf message 
            */
            virtual ConfigStatus onCreate(chm6_dataplane::Chm6CarrierConfig* pMsg);

            /*
            * Callback function called on update of carrier object in chm6 redis
            * @param : Carrier protobuf message 
            */
            virtual ConfigStatus onModify(chm6_dataplane::Chm6CarrierConfig* pMsg);

            /*
             * Callback function called on delete of carrier object in chm6 redis
             * @param : Carrier protobuf message 
             */
            virtual ConfigStatus onDelete(chm6_dataplane::Chm6CarrierConfig* pMsg);
            /*
             * Callback function called on create of carrier object in chm6 redis
             * @param : OFEC CC status 
             */
            virtual grpc::StatusCode onCreate(std::string carrId, bool isOfecUp);

            /*
             * Callback function called on update of carrier Fault object in chm6 redis
             * @param : OFEC CC status 
             */
            virtual grpc::StatusCode onModify(std::string carrId, bool isOfecUp);

            /*
             * Callback function called on Delete of carrier object in chm6 redis
             * @param : OFEC CC Status 
             */
            virtual grpc::StatusCode onDelete(std::string carrId, bool isOfecUp);

            /*
             * Callback function called on update of carrier state object in chm6 redis
             * @param : Carrier Rx acquisition state 
             */
            virtual grpc::StatusCode onModifyCarrRx(std::string carrId, bool isRxAcq);



        private:
            DpAdapter::DcoSecProcCarrierAdapter* mpCarrierAdapter;

    };

} // namespace DataPlane

#endif // CARRIER_CONFIG_HANDLER_H

