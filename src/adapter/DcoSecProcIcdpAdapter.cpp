#include <iostream>
#include <string>
#include <chrono>
#include "DcoSecProcIcdpAdapter.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "dp_common.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include "DcoConnectHandler.h"
#include <sys/time.h>

using google::protobuf::util::MessageToJsonString;
using google::protobuf::util::MessageDifferencer;
using google::protobuf::Message;

using namespace DataPlane;

namespace DpAdapter {
    /**
     * Adapter class constructor 
     * Adapter Converts DcoSecProc vlan objects to chm6 objects and vice versa
     **/
    DcoSecProcIcdpAdapter::DcoSecProcIcdpAdapter():RetryHelperIntf(RETRY_INTERVAL, MAX_RETRY_COUNT)
    {
        //grpc server address of the form <ip>:<port> eg - 192.168.1.2:50051
        std::string dco_secproc_grpc = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoNxp());
        mspDcoSecProcCrud = std::dynamic_pointer_cast<GnmiClient::NBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_NXP));
        if (mspDcoSecProcCrud == NULL)
        {
            INFN_LOG(SeverityLevel::error) <<  __func__ << "Dco secproc interface is NULL" << '\n';
        }
    }

    DcoSecProcIcdpAdapter::~DcoSecProcIcdpAdapter()
    {
        for (const auto& kv : mCurrStateMap)
        {
            delete kv.second;
        }
        mCurrStateMap.clear();

    }

    void DcoSecProcIcdpAdapter::UpdateIcdpState(Dco6SecProcIcdpState& state)
    {
        bool isIcdpEnable = GetIcdpEnable();
        {
            std::lock_guard<std::mutex> guard(mDcoSecMutex);
            std::string icdpId = state.base_state().config_id().value();
            Chm6IcdpState chmIcdpState;

            auto iter = mCurrStateMap.find(icdpId);
            if(iter == mCurrStateMap.end()) 
            {
                if(state.base_state().mark_for_delete().value() == true)
                {
                    //duplicate delete
                    return;
                }
                if( true == isIcdpEnable )
                {
                    Dco6SecProcIcdpState* pState = new Dco6SecProcIcdpState(state);
                    mCurrStateMap.insert(std::pair<std::string, Dco6SecProcIcdpState*>(icdpId, pState));
                    ConvertDco6SecProcIcdpNodeStateToChm6(*pState, chmIcdpState);
                    mCreateIcdpStCb(chmIcdpState);
                }
            }
            else
            {
                Dco6SecProcIcdpState* pState = iter->second;
                if(false == MessageDifferencer::Equals(*pState, state))
                {
                    if(pState)
                    {
                        pState->CopyFrom(state);
                        ConvertDco6SecProcIcdpNodeStateToChm6(*pState, chmIcdpState);
                        //@todo : Check how to communicate ICDP delete
                        //Call API in the manager, Same call context as grpc
                        //Add a callback in the manager
                        if((pState->base_state().mark_for_delete().value() == true) || (isIcdpEnable == false))
                        {
                            mDeleteIcdpStCb(chmIcdpState);

                            if( false == isIcdpEnable )
                            {
                                //Delete current icdp entry
                                mCurrStateMap.erase(iter);
                            }
                            return;
                        }
                    }
                    if( true == isIcdpEnable )
                    {
                        mUpdateIcdpStCb(chmIcdpState);
                    }
                }
            }

        }
    }

    void DcoSecProcIcdpAdapter::SubscribeObject(std::string objId)
    {
        SubscribeDcoSecProcIcdpState(objId);
    }

    void DcoSecProcIcdpAdapter::UnSubscribeObject(std::string objId)
    {
        UnSubscribeDcoSecProcIcdpState(objId);
    }

    void DcoSecProcIcdpAdapter::SubscribeDcoSecProcIcdpState(std::string icdpId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " sub " << icdpId;
        Dco6SecProcIcdpState icdpState;
        icdpState.mutable_base_state()->mutable_config_id()->set_value(icdpId);
        google::protobuf::Message *msg = static_cast<google::protobuf::Message *>(&icdpState);
        
        if( false == IsActiveSubscription(icdpId) )
        {
            DcoSecProcIcdpSubContext* pCb = new DcoSecProcIcdpSubContext(*this);
            std::string cbId = mspDcoSecProcCrud->Subscribe(msg, pCb);
            if(pCb) pCb->AddCall(icdpId, cbId);
        }
    }

    void DcoSecProcIcdpAdapter::UnSubscribeDcoSecProcIcdpState(std::string icdpId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " unsubscr " << icdpId;
        try 
        {
            CleanupOnUnSubscribe(icdpId);
        }
        catch(const std::exception& e)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " exception in unsubscribe " << icdpId << " " << e.what();
        }
    }

    void DcoSecProcIcdpAdapter::SetConnectionState(bool isConnectUp)
    {  
        if( mIsConnectUp != isConnectUp )
        {
            SetConnState(isConnectUp);
            if( !isConnectUp)
            {
                for(auto iter : mIcdpConfigMap)
                {
                    UnSubscribeDcoSecProcIcdpState(iter.first);
                }
                mIcdpConfigMap.clear();
            }
        }
        mIsConnectUp =  isConnectUp;
    }

    bool DcoSecProcIcdpAdapter::ConvertChm6ToDco6SecProcIcdpNodeConfig(std::string icdpId, 
            IcdpNodeInfo& node, Dco6SecProcIcdpConfig* pCfg, bool isCreate)
    {
        bool isChanged = false;

        if(isCreate)
        {
            pCfg->mutable_base_config()->mutable_config_id()->set_value(icdpId);
            pCfg->mutable_config()->mutable_local_info()->mutable_ne_id()->set_value(node.icdp_info().ne_id().value());
            pCfg->mutable_config()->mutable_local_info()->mutable_ne_type()->set_value(node.icdp_info().ne_type().value());
            pCfg->mutable_config()->mutable_local_info()->mutable_ne_name()->set_value(node.icdp_info().ne_name().value());
            pCfg->mutable_config()->mutable_local_info()->mutable_ipv4_loopback_addr()->set_value(node.icdp_info().ipv4_loopback_addr().value());
            pCfg->mutable_config()->mutable_local_info()->mutable_ipv6_loopback_addr()->set_value(node.icdp_info().ipv6_loopback_addr().value());
            isChanged = true;
        }
        else
        {
            auto icdpNeId =  pCfg->mutable_config()->mutable_local_info()->mutable_ne_id();
            auto ipNeId = node.icdp_info().ne_id().value();
            if(icdpNeId->value() != ipNeId)
            {
                icdpNeId->set_value(ipNeId);
                isChanged = true;
            }

            auto icdpNeType = pCfg->mutable_config()->mutable_local_info()->mutable_ne_type();
            auto ipNeType = node.icdp_info().ne_type().value();
            if( icdpNeType->value() != ipNeType )
            {
                icdpNeType->set_value(ipNeType);
                isChanged = true;
            }

            auto icdpNeName = pCfg->mutable_config()->mutable_local_info()->mutable_ne_name();
            auto ipNeName = node.icdp_info().ne_name().value();
            if( icdpNeName->value() != ipNeName )
            {
                icdpNeName->set_value(ipNeName);
                isChanged = true;
            }

            auto icdpV4Lpbk = pCfg->mutable_config()->mutable_local_info()->mutable_ipv4_loopback_addr();
            auto ipV4Lpbk = node.icdp_info().ipv4_loopback_addr().value();
            if( icdpV4Lpbk->value() != ipV4Lpbk )
            {
                icdpV4Lpbk->set_value(ipV4Lpbk);
                isChanged = true;
            }

            auto icdpV6Lpbk = pCfg->mutable_config()->mutable_local_info()->mutable_ipv6_loopback_addr();
            auto ipV6Lpbk = node.icdp_info().ipv6_loopback_addr().value();
            if( icdpV6Lpbk->value() != ipV6Lpbk)
            {
                icdpV6Lpbk->set_value(ipV6Lpbk);
                isChanged = true;
            }
        }
        return isChanged;

    }

    void DcoSecProcIcdpAdapter::ConvertDco6SecProcIcdpNodeStateToChm6(Dco6SecProcIcdpState& dcoIcdpState, Chm6IcdpState& chmIcdpState)
    {
        chmIcdpState.mutable_base_state()->mutable_config_id()->set_value(dcoIcdpState.base_state().config_id().value());
        if( dcoIcdpState.base_state().mark_for_delete().value() == true )
        {
            //Deletion request
            chmIcdpState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);
            return;
        }
        chmIcdpState.mutable_icdp_state_data()->mutable_peer_carrier_id()->set_value(dcoIcdpState.state().peer_carrier_id().value());
        chmIcdpState.mutable_icdp_state_data()->mutable_peer_info()->\
            mutable_ne_id()->set_value(dcoIcdpState.state().peer_info().ne_id().value());
        chmIcdpState.mutable_icdp_state_data()->mutable_peer_info()->mutable_ne_type()->\
            set_value(dcoIcdpState.state().peer_info().ne_type().value());
        chmIcdpState.mutable_icdp_state_data()->mutable_peer_info()->mutable_ne_name()->\
            set_value(dcoIcdpState.state().peer_info().ne_name().value());
        chmIcdpState.mutable_icdp_state_data()->mutable_peer_info()->mutable_ipv4_loopback_addr()->\
            set_value(dcoIcdpState.state().peer_info().ipv4_loopback_addr().value());
        chmIcdpState.mutable_icdp_state_data()->mutable_peer_info()->mutable_ipv6_loopback_addr()->\
            set_value(dcoIcdpState.state().peer_info().ipv6_loopback_addr().value());
    }

    bool DcoSecProcIcdpAdapter::GetIcdpEnable()
    {
        std::lock_guard<std::mutex> guard(mDcoSecCfgMutex);
        bool enable = false;
        if(mIcdpNodeInfo.enable_icdp() == BoolWrapper::BOOL_TRUE)
        {
            enable = true;
        }
        return enable;
    }

/*    bool DcoSecProcIcdpAdapter::GetIcdpEnableChanged()
    {
        std::lock_guard<std::mutex> guard(mDcoSecCfgMutex);
        bool isChanged = false;
        if(mIcdpNodeInfo.enable_icdp() == BoolWrapper::BOOL_TRUE)
        {
            enableIcdp = true;
        }
        return enable;
    }*/


    void DcoSecProcIcdpAdapter::UpdateIcdpNode(const IcdpNodeInfo& node)
    {
        std::string icdpCfgStr, icdpOrig, newIcdp;
        MessageToJsonString(node, &icdpCfgStr);
        MessageToJsonString(mIcdpNodeInfo, &icdpOrig);

        INFN_LOG(SeverityLevel::debug) << __func__ << " orig: " << icdpOrig << " new " << icdpCfgStr;
        
        {
            std::lock_guard<std::mutex> guard(mDcoSecCfgMutex);
            mIcdpNodeInfo.MergeFrom(node);
           
        }

        MessageToJsonString(mIcdpNodeInfo, &newIcdp);
        INFN_LOG(SeverityLevel::info) << __func__ << " updated : " << newIcdp;

        bool enableIcdp = false;
        Dco6SecProcIcdpConfig cfg;
        if(mIcdpNodeInfo.enable_icdp() == BoolWrapper::BOOL_TRUE)
        {
            enableIcdp = true;
        }

        //Iterate through all carriers, call set for all carriers
        for(auto iter = mIcdpConfigMap.begin(); iter != mIcdpConfigMap.end(); iter++)
        {
            iter->second = enableIcdp;

            if( enableIcdp )
            {
                ConvertChm6ToDco6SecProcIcdpNodeConfig(iter->first, mIcdpNodeInfo, &cfg);
            }
            else
            {
                cfg.mutable_base_config()->mutable_config_id()->set_value(iter->first);
            }

            struct timeval tv;
            gettimeofday(&tv, NULL);

            cfg.mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
            cfg.mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
            google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(&cfg);
            //Call a gnmi set
            //@todo check if the set was successful
            
            grpc::Status callStatus = mspDcoSecProcCrud->Set(cfgMsg);

            if(grpc::StatusCode::OK != callStatus.error_code() )
            {
                INFN_LOG(SeverityLevel::info) <<" Error in Icdp set" << callStatus.error_code();
            }
        }

    }

    void DcoSecProcIcdpAdapter::DeleteIcdpNode()
    {
        INFN_LOG(SeverityLevel::info) << __func__ ;
        mIcdpNodeInfo.Clear();
    }

    void DcoSecProcIcdpAdapter::CarrierCreate(std::string carrierId)
    {
        auto iter = mIcdpConfigMap.find(carrierId);
        Dco6SecProcIcdpConfig cfg;

        if(iter == mIcdpConfigMap.end())
        {
            //New object creation
            if(mIcdpNodeInfo.enable_icdp() == BoolWrapper::BOOL_TRUE)
            {
                //Create DCOSecproc icdp object and send config to DCOSecProc
                ConvertChm6ToDco6SecProcIcdpNodeConfig(carrierId, mIcdpNodeInfo, &cfg);

                mIcdpConfigMap.insert(std::pair<std::string, bool>(carrierId, true));
            }
            else if( (mIcdpNodeInfo.enable_icdp() == BoolWrapper::BOOL_FALSE) || 
                    (mIcdpNodeInfo.enable_icdp() == BoolWrapper::BOOL_UNSPECIFIED) )
            {
                //Carrier created but Icdp is disabled
                cfg.mutable_base_config()->mutable_config_id()->set_value(carrierId);
                mIcdpConfigMap.insert(std::pair<std::string, bool>(carrierId, false));
            }
            struct timeval tv;
            gettimeofday(&tv, NULL);

            cfg.mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
            cfg.mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
            google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(&cfg);
            //Call a gnmi set
            grpc::Status callStatus = mspDcoSecProcCrud->Set(cfgMsg);

            if(grpc::StatusCode::OK != callStatus.error_code() )
            {
                INFN_LOG(SeverityLevel::info) <<" Error in icdp set" << callStatus.error_code();
            }

            //subscribe for DCO secproc state update
            SubscribeDcoSecProcIcdpState(carrierId);
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "Error: " << __func__ << " duplicate carrier creation";
        }

    }

    void DcoSecProcIcdpAdapter::CarrierDelete(std::string carrierId)
    {
        auto iter = mIcdpConfigMap.find(carrierId);
        Dco6SecProcIcdpConfig cfg;

        if(iter != mIcdpConfigMap.end())
        {
            //Send ICDP delete to NXP, unsubscribe
            mIcdpConfigMap.erase(iter);
            cfg.mutable_base_config()->mutable_config_id()->set_value(carrierId);
            cfg.mutable_base_config()->mutable_mark_for_delete()->set_value(1);
            struct timeval tv;
            gettimeofday(&tv, NULL);

            cfg.mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
            cfg.mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
            google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(&cfg);
            //Call a gnmi set
            grpc::Status callStatus = mspDcoSecProcCrud->Set(cfgMsg);
            if(grpc::StatusCode::OK != callStatus.error_code() )
            {
                INFN_LOG(SeverityLevel::info) <<" Error in icdp set" << callStatus.error_code();
            }
            //Call unsubscribe
            UnSubscribeObject(carrierId);
        }

    }

    void DcoSecProcIcdpAdapter::DumpIcdpSubscriptions(ostream& out)
    {
        out << "=====Icdp state Subscriptions=====";
        DumpSubscriptionData(out);
    }

 //-------------------------DcoSecProcIcdpSubContext APIs------------------------------------------

    bool DcoSecProcIcdpSubContext::AddCall(std::string id, std::string cbId) 
    {
        std::lock_guard<std::mutex> lock(mSubCbMutex);
        if(mCallMap.find(cbId) == mCallMap.end())
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Added cbId: " << cbId << " " << id << std::endl; 
            mCallMap.insert(std::pair<std::string, std::string>(cbId, id) );
            return true;
        }
        return false;
    }

    void DcoSecProcIcdpSubContext::HandleSubResponse(google::protobuf::Message& obj,
            grpc::Status status, std::string cbId)
    {
        std::string secprocStateStr;
        auto iter = mCallMap.find(cbId);
        if(iter == mCallMap.end())
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Error! DcoSecProc Icdp cbId not found! ";
            return;
        }

        if( status.error_code() == grpc::StatusCode::OK )
        {
            Dco6SecProcIcdpState* cp = static_cast<Dco6SecProcIcdpState*>(&obj);
            MessageToJsonString(*cp, &secprocStateStr);
            INFN_LOG(SeverityLevel::debug) << __func__ << " DcoSecProc Icdp update triggerred!!!" << " , cbId: " << cbId  <<
                " " << secprocStateStr << std::endl;
            mAdapterRef.UpdateIcdpState(*cp);

            //Remove if present is re-sub list since first successful response
            mAdapterRef.RemoveFromReSubscriptionList(iter->second, this);
        }
        else
        {
            //Error : Subscription failed case
            //Call adapter to indicate the subscription has failed
            INFN_LOG(SeverityLevel::info) << __func__ << " Error!"
                << " , cbId: " << cbId << " " << status.error_code() << std::endl;
            RetryObjWrapper icdp(iter->second);
            mAdapterRef.AddToReSubscriptionList(icdp);
            mCallMap.clear();
        }

    } 


}//end namespace
