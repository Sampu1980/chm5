/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_SEC_PROC_CARRIER_ADAPTER_H
#define DCO_SEC_PROC_CARRIER_ADAPTER_H
#include "InfnBoostTimer.h"
#include "CrudService.h"
#include "DcoSecProcAdapterProtoDefs.h"
#include "DpProtoDefs.h"
#include <mutex>
#include <iostream>
#include <boost/system/error_code.hpp>

namespace DpAdapter {

    class OfecStatusSoakTimer : public InfnBoostTimer
    {
        public:

            OfecStatusSoakTimer(std::string aid, boost::asio::io_context& ioService, std::shared_ptr<::GnmiClient::NBGnmiClient>& crud):
                InfnBoostTimer(ioService)
        {
            mAid = aid;
            mCrud = crud;
        }
            ~OfecStatusSoakTimer() {}
            bool IsRunning() const
            {
                return (
#ifdef Z_LINUX
                        (mTimerId == 0)
#else
                        (mTimerId < 0)
#endif
                        ? false
                        : true);
            }
            virtual void TimerExpired() ;

        private:
            timer_t mTimerId;
            std::shared_ptr<::GnmiClient::NBGnmiClient> mCrud;
            std::string mAid;
    };

    class DcoSecProcCarrierAdapter
    {
        public:
            DcoSecProcCarrierAdapter();
            ~DcoSecProcCarrierAdapter();
            /*
            * Converts Chm6 carrier config to nxp carrier config
            * Calls blocking set operation to nxp.
            * @param pCfg : Chm6 carrier config
            * @param isCreate : true if is Create operation
            * 
            */
            grpc::StatusCode UpdateCarrierConfig(chm6_dataplane::Chm6CarrierConfig *pCfg, bool isDelete = false);
            grpc::StatusCode UpdateCarrierConfig(std::string carrId, bool isOfecUp,  bool isCreate = false, bool isDelete = false);
            grpc::StatusCode UpdateCarrierRxConfig(std::string carrId, bool isRxAcq);
            static void TimerInitialize();

            std::shared_ptr<::GnmiClient::NBGnmiClient>& GetDcoSecProcCrud()
            {
                return mspDcoSecProcCrud;
            }

        private: 
            std::mutex mDcoSecMutex;
            std::shared_ptr<::GnmiClient::NBGnmiClient> mspDcoSecProcCrud;
            typedef std::map<std::string, OfecStatusSoakTimer*> TypeOfecStatusTimerMap;
            TypeOfecStatusTimerMap mOfecStatusTimerList;

    };
}
#endif
