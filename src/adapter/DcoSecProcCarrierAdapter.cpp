/************************************************************************
  Copyright (c) 2020 Infinera
 ************************************************************************/
#include "DpProtoDefs.h"
#include "DcoSecProcCarrierAdapter.h"
#include <iostream>
#include <string>
#include <chrono>
#include "DpEnv.h"
#include "InfnLogger.h"
#include <google/protobuf/util/json_util.h>
#include "dp_common.h"
#include "DcoConnectHandler.h"
#include <sys/time.h>
#include <infinera/wrappers/v1/infn_datatypes.pb.h>
#include "DpCarrierMgr.h"

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;
using namespace DataPlane;
namespace hal_common  = infinera::hal::common::v2;
boost::asio::io_context dcoSecProcIos;
#define OFEC_SOAK_TIME 5 * 1000

namespace DpAdapter {

    DcoSecProcCarrierAdapter::DcoSecProcCarrierAdapter()
    {
        std::string dco_secproc_grpc = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoNxp()); 
        INFN_LOG(SeverityLevel::info) << __func__ << "NXP server name:" << dco_secproc_grpc;
        mspDcoSecProcCrud = std::dynamic_pointer_cast<GnmiClient::NBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_NXP));
        if (mspDcoSecProcCrud == NULL)
        {
            INFN_LOG(SeverityLevel::error) <<  __func__ << "Dco secproc interface is NULL" << '\n';
        }
    } 

    DcoSecProcCarrierAdapter::~DcoSecProcCarrierAdapter()
    {

    }

    void DcoSecProcCarrierAdapter::TimerInitialize()
    {
        dcoSecProcIos.restart();
        dcoSecProcIos.run();
    }

    grpc::StatusCode DcoSecProcCarrierAdapter::UpdateCarrierConfig(chm6_dataplane::Chm6CarrierConfig *pCfg, bool isDelete)
    {
        //Convert attributes needed DCO carrier config to nxp carrier config
        std::string cfgData;
        MessageToJsonString(*pCfg, &cfgData);

        Dco6SecCarrierConfig* pCarrierCfg =  new Dco6SecCarrierConfig();
        pCarrierCfg->mutable_base_config()->mutable_config_id()->set_value(pCfg->base_config().config_id().value());
        pCarrierCfg->mutable_config()->set_is_encryption_feature_supported(wrapper::BOOL_TRUE);

        if(isDelete)
        {
            pCarrierCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
        }
        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);

        pCarrierCfg->mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pCarrierCfg->mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(pCarrierCfg);
        grpc::Status callStatus = mspDcoSecProcCrud->Set(cfgMsg);
        {
            std::string cfgData;
            MessageToJsonString(*pCarrierCfg, &cfgData);
            INFN_LOG(SeverityLevel::debug) <<" Dco6SecCarrierConfig  proto" <<  cfgData;
        }

        if(pCarrierCfg)
        {
            delete pCarrierCfg;
        }
        
        if(grpc::StatusCode::OK != callStatus.error_code() )
        {
            INFN_LOG(SeverityLevel::info) <<" Error in carrier set" << callStatus.error_code();
        }
        return callStatus.error_code();
    };

    void OfecStatusSoakTimer::TimerExpired()
    {
        grpc::Status gnmiSetStatus(grpc::StatusCode::OK, "Status OK");
        setTimerIsRunning(false);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        Dco6SecProcOfecConfig* pOfecCfg =  new Dco6SecProcOfecConfig();
        pOfecCfg->mutable_base_config()->mutable_config_id()->set_value(mAid);
        pOfecCfg->mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pOfecCfg->mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
        pOfecCfg->mutable_ofec_state()->set_is_ofec_up(wrapper::BOOL_TRUE);
        {
            std::string cfgData;
            MessageToJsonString(*pOfecCfg, &cfgData);
            INFN_LOG(SeverityLevel::debug) <<" TimerExpired  proto" <<  cfgData;
        }

        {
            google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(pOfecCfg);
            INFN_LOG(SeverityLevel::debug) << " Ofec timer is expired so setting redis for carrier " << mAid;
            gnmiSetStatus = mCrud->Set(cfgMsg);
            if(grpc::StatusCode::OK == gnmiSetStatus.error_code())
            {
                INFN_LOG(SeverityLevel::debug) << " GNMi SET passed";
            }
            else
            {
                INFN_LOG(SeverityLevel::error) << " GNMi SET Failed, need to resend information. Error = " << gnmiSetStatus.error_message();
                DpCarrierMgrSingleton::Instance()->GetMspStateCollector()->SetForceCarrFaultToNxp(mAid);
            }
        }

        if(pOfecCfg)
        {
            delete pOfecCfg;
        }
    }

    grpc::StatusCode DcoSecProcCarrierAdapter::UpdateCarrierConfig(std::string carrId, bool isOfecUp, bool isCreate, bool isDelete)
    {
        grpc::Status gnmiSetStatus(grpc::StatusCode::OK, "Status OK");
        static bool isFirst = true;
        Dco6SecProcOfecConfig* pOfecCfg =  new Dco6SecProcOfecConfig();
        struct timeval tv;
        gettimeofday(&tv, NULL);
        INFN_LOG(SeverityLevel::debug) << " AID = " << carrId << " OFEC CC Status = " << isOfecUp << " Create = " << isCreate << " Delete = " << isDelete;
        pOfecCfg->mutable_base_config()->mutable_config_id()->set_value(carrId);
        pOfecCfg->mutable_ofec_state()->set_is_ofec_up(wrapper::BOOL_FALSE);

        pOfecCfg->mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
        pOfecCfg->mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        if(isDelete)
        {
            pOfecCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);
        }

        if((false == isCreate) /*&& (false == isDelete)*/)
        {
            //OFEC is UP hence soak the data
            if((isOfecUp) && (false == isDelete))
            {
                pOfecCfg->mutable_ofec_state()->set_is_ofec_up(wrapper::BOOL_TRUE);
                TypeOfecStatusTimerMap::iterator itr;
                itr = mOfecStatusTimerList.find(carrId);
                if(itr == mOfecStatusTimerList.end())
                {
                    INFN_LOG(SeverityLevel::debug) << " Creating & starting new timer for aid " << carrId << " dcoSecProcIos = " << std::hex << &dcoSecProcIos << " OFEC_SOAK_TIME = " << OFEC_SOAK_TIME;
                    OfecStatusSoakTimer *ofectimer = new OfecStatusSoakTimer(carrId, dcoSecProcIos, this->GetDcoSecProcCrud());
                    mOfecStatusTimerList[carrId] = ofectimer;
                    itr = mOfecStatusTimerList.find(carrId);
                    itr->second->Start(OFEC_SOAK_TIME, false);
                }
                INFN_LOG(SeverityLevel::debug)  << " Timer is Running = " << itr->second->getTimerIsRunning() << " itr->Second = " << std::hex << itr->second;
                if(itr->second->getTimerIsRunning() == false)
                {
                    //start new timer
                    INFN_LOG(SeverityLevel::debug) << "starting timer for aid = " << carrId;
                    itr->second->Start(OFEC_SOAK_TIME, false);
                }
                INFN_LOG(SeverityLevel::debug) << " dcoSecProcIos.stopped = " << dcoSecProcIos.stopped() << " IsFirst = " << isFirst;

                //Start dcoSecProcIos thread to listen to timer expiry
                if(dcoSecProcIos.stopped() ||  isFirst)//((itr != mOfecStatusTimerList.end()) && (itr->second) && (true == itr->second->getTimerIsRunning()))
                {
                    boost::thread(boost::bind(&TimerInitialize)).detach();
                    isFirst = false;
                }
            }
            else
            {
                //cancel any timer that is running for this carrier
                INFN_LOG(SeverityLevel::debug) << " setting OFEC down";
                TypeOfecStatusTimerMap::iterator itr = mOfecStatusTimerList.find(carrId);
                if(itr != mOfecStatusTimerList.end())
                {
                    if((itr->second->getTimerIsRunning() == true) && (false == itr->second->IsInCallback()))
                    {
                        itr->second->Cancel();
                        INFN_LOG(SeverityLevel::debug) << " Cancelling timer for aid " << carrId;
                    }
                    else
                    {
                        INFN_LOG(SeverityLevel::debug) << " Timer is not running for aid = " << carrId << " timer running = " << itr->second->getTimerIsRunning() <<" Incallback =  " << itr->second->IsInCallback();
                    }
                }
                else
                {
                    INFN_LOG(SeverityLevel::debug) << " No entry present in iterator for aid " << carrId;
                }
                pOfecCfg->mutable_ofec_state()->set_is_ofec_up(wrapper::BOOL_FALSE);
            }
        }

        if(false == isOfecUp)
        {
            INFN_LOG(SeverityLevel::debug) << " Setting OFEC protobuf in GRPC/ Redis without soak";
            google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(pOfecCfg);
            gnmiSetStatus = mspDcoSecProcCrud->Set(cfgMsg);
            if(grpc::StatusCode::OK == gnmiSetStatus.error_code())
            {
                INFN_LOG(SeverityLevel::debug) << " GNMi SET passed";
            }
            else
            {
                INFN_LOG(SeverityLevel::error) << " GNMi SET Failed, need to resend information. Error = " << gnmiSetStatus.error_message();
            }
        }
        if(pOfecCfg)
        {
            delete pOfecCfg;
        }
        return gnmiSetStatus.error_code();
    }

    grpc::StatusCode DcoSecProcCarrierAdapter::UpdateCarrierRxConfig(std::string carrId, bool isRxAcq)
    {
        grpc::Status gnmiSetStatus(grpc::StatusCode::OK, "Status OK");
        Dco6SecCarrierConfig* pCarrierCfg =  new Dco6SecCarrierConfig();
        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);

        if(NULL != pCarrierCfg)
        {
            pCarrierCfg->mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
            pCarrierCfg->mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

            pCarrierCfg->mutable_base_config()->mutable_config_id()->set_value(carrId);
            pCarrierCfg->mutable_config()->set_is_encryption_feature_supported(wrapper::BOOL_TRUE);

            if(true == isRxAcq)
            {
                pCarrierCfg->mutable_config()->set_is_rx_acq_comp(wrapper::BOOL_TRUE);
            }
            else
            {
                pCarrierCfg->mutable_config()->set_is_rx_acq_comp(wrapper::BOOL_FALSE);
            }

            google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(pCarrierCfg);

            gnmiSetStatus = mspDcoSecProcCrud->Set(cfgMsg);
            if(grpc::StatusCode::OK == gnmiSetStatus.error_code())
            {
                INFN_LOG(SeverityLevel::debug) << " GNMi SET passed";
            }
            else
            {
                INFN_LOG(SeverityLevel::error) << " GNMi SET Failed, need to resend information. Error = " << gnmiSetStatus.error_message();
            }
            {
                std::string cfgData;
                MessageToJsonString(*pCarrierCfg, &cfgData);
                INFN_LOG(SeverityLevel::debug) <<" Dco6SecCarrierConfig  proto" <<  cfgData;
            }
            delete pCarrierCfg;
        }
        return gnmiSetStatus.error_code();

    }

}
