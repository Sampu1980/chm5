#include <iostream>
#include <string>
#include <chrono>
#include <boost/thread.hpp>
#include "DcoSecProcVlanAdapter.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "dp_common.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include "DcoConnectHandler.h"

using google::protobuf::util::MessageDifferencer;
using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace DataPlane;

namespace DpAdapter {
    /**
     * Adapter class constructor 
     * Adapter Converts DcoSecProc vlan objects to chm6 objects and vice versa
     **/
    DcoSecProcVlanAdapter::DcoSecProcVlanAdapter():RetryHelperIntf(RETRY_INTERVAL, MAX_RETRY_COUNT)
    {
        //grpc server address of the form <ip>:<port> eg - 192.168.1.2:50051
        std::string dco_secproc_grpc = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoNxp());
        mspDcoSecProcCrud = std::dynamic_pointer_cast<GnmiClient::NBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_NXP));
        if (mspDcoSecProcCrud == NULL)
        {
            INFN_LOG(SeverityLevel::error) <<  __func__ << "Dco secproc interface is NULL" << '\n';
        }
    
    }

    DcoSecProcVlanAdapter::~DcoSecProcVlanAdapter()
    {
        {
            std::lock_guard<std::mutex> guard(mDcoSecStateMutex);
            for (const auto& kv : mCurrStateMap)
            {
                delete kv.second;
            }
            mCurrStateMap.clear();
        }
        mChangeList.clear();

        mSubThread.join();

    }

    //----------------Data conversion APIs-------------------------------------------
    bool DcoSecProcVlanAdapter::ConvertSecProcToPcpVlanState(Dco6SecProcVlanState& state,\
            Chm6PeerDiscVlanState& pcpState)
    {
        (pcpState.mutable_base_state())->mutable_config_id()->set_value(state.base_state().config_id().value());
        if((state.vlan_settings().carrier_vlan_map().size()) > 1)
        {
            INFN_LOG(SeverityLevel::info) <<  __func__ << "Warning! Dco secproc more than one vlan entry" << '\n';
        }
        auto iter = state.vlan_settings().carrier_vlan_map().begin();
        if( iter != state.vlan_settings().carrier_vlan_map().end() )
        {
            auto vlanData = iter->second;
            (pcpState.mutable_state())->mutable_vlanid()->set_value(iter->first);

            PcpAppType pcptype;
            switch(vlanData.apptype())
            {
                case Dco6SecProcVlanState::VlanSettings::APP_TYPE_ICDP:
                    pcptype = PcpAppType::APP_TYPE_OFEC_CC_PEERDISC;
                    INFN_LOG(SeverityLevel::debug) <<  __func__ << " setting pcp type to APP_TYPE_OFEC_CC_PEERDISC";
                    break;
                case Dco6SecProcVlanState::VlanSettings::APP_TYPE_IKE:
                    pcptype = PcpAppType::APP_TYPE_OFEC_CC_IKE;
                    INFN_LOG(SeverityLevel::debug) <<  __func__ << " setting pcp type to APP_TYPE_OFEC_CC_IKE";
                    break;
                default:
                    pcptype = PcpAppType::APP_TYPE_UNSPECIFIED;
            } 
            (pcpState.mutable_state())->set_apptype(pcptype);
        }
        return true;
    }

    bool DcoSecProcVlanAdapter::ConvertSecProcToGccVlanState(Dco6SecProcVlanState& state,\
            DataPlane::ControlChannelVlanIdx& idx,\
            ControlChannelVlanCommon& cmn)
    {
        std::string carrierId = state.base_state().config_id().value();
        std::string basecarrierId = carrierId;
        std::size_t found = carrierId.find_last_of("L");
        if(found != std::string::npos)
        {
            carrierId = carrierId.substr(found+1,1);
            if (carrierId == "1")
            {
                if ( string::npos != basecarrierId.find("IKE"))
                {
                    idx = DataPlane::CONTROLCHANNELVLANIDX_IKE_CARRIER_1;
                }
                else
                {
                    idx = DataPlane::CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1;
                }
                cmn.carrier = DataPlane::CONTROLCHANNELCARRIER_1;
            }
            else if (carrierId == "2")
            {
                if ( string::npos != basecarrierId.find("IKE"))
                {
                    idx = DataPlane::CONTROLCHANNELVLANIDX_IKE_CARRIER_2;
                }
                else
                {
                    idx = DataPlane::CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_2;
                }
                cmn.carrier = DataPlane::CONTROLCHANNELCARRIER_2;
            }
            else
            {
                return false;
            }
        }
        cmn.type = DataPlane::CONTROLCHANNELTYPE_OFEC;
        auto iter = state.vlan_settings().carrier_vlan_map().begin();
        if( iter != state.vlan_settings().carrier_vlan_map().end() )
        {
            auto vlanData = iter->second;
            cmn.enable = true;
            cmn.rxVlanTag = vlanData.rx_normalized_vlanid().value();
            cmn.rxVlanReTag = iter->first;
            cmn.txVlanTag = iter->first;
            cmn.txVlanReTag = vlanData.tx_normalized_vlanid().value();
            if(Dco6SecProcVlanState::VlanSettings::OPERATION_TYPE_DELETE_TX_VLAN == vlanData.operationtype())
            {
                cmn.deleteTxTags = true;
            }
            else
            {
                cmn.deleteTxTags = false;
            }
        }

        //rxvlantag  = rx normalized 40
        //rxvlanretag = map key //dynamic vlan tag 3001
        //txvlantag = map key// dyn
        //txvlanretag = tx normalized 40
        return true;
    }
    //----------------Data conversion APIs end-------------------------------------------    

    void DcoSecProcVlanAdapter::UpdateVlanState(const Dco6SecProcVlanState& state)
    {
        {
            std::lock_guard<std::mutex> guard(mDcoSecStateMutex);
            std::string carrierId = state.base_state().config_id().value();

            Chm6PeerDiscVlanState pcpVlanState;
            DataPlane::ControlChannelVlanIdx idx;
            ControlChannelVlanCommon cmn = {};

            auto iter = mCurrStateMap.find(carrierId);
            if(iter == mCurrStateMap.end())
            {
                if(state.base_state().mark_for_delete().value() == true)
                {
                    //Duplicate delete? 
                    return;
                }
                Dco6SecProcVlanState* pState = new Dco6SecProcVlanState(state);
                mCurrStateMap.insert(std::pair<std::string, Dco6SecProcVlanState*>(carrierId, pState));

                ConvertSecProcToPcpVlanState(*pState, pcpVlanState);
                ConvertSecProcToGccVlanState( *pState, idx, cmn);

                //Call API in the manager, Same call context as grpc
                mCreateVlanCb(carrierId, idx, cmn);
                mCreatePcpCb(pcpVlanState);
            }
            else
            {
                Dco6SecProcVlanState* pState = iter->second;
                if(pState)
                {
                    if(state.base_state().mark_for_delete().value() == true)
                    {
                        pState->CopyFrom(state);
                        ConvertSecProcToPcpVlanState(*pState, pcpVlanState);
                        ConvertSecProcToGccVlanState(*pState, idx, cmn);
                        mDeleteVlanCb(carrierId, idx, cmn);
                        mDeletePcpCb(pcpVlanState);
                        mCurrStateMap.erase(iter);
                        return;
                    }

                    if(false == MessageDifferencer::Equals(*pState, state))
                    {
                        pState->CopyFrom(state);
                        ConvertSecProcToPcpVlanState(*pState, pcpVlanState);
                        ConvertSecProcToGccVlanState( *pState, idx, cmn);
                        
                        std::string secprocVlanStateStr;
                        MessageToJsonString(*pState, &secprocVlanStateStr);
                        INFN_LOG(SeverityLevel::info) << __func__ << " DcoSecProc Vlan update triggerred!!!" <<
                            " " << secprocVlanStateStr << std::endl;

                        //API to update?
                        //Call API in the manager, Same call context as grpc
                        mCreateVlanCb(carrierId, idx, cmn);
                        mCreatePcpCb(pcpVlanState);
                    }
                }

           }

        }
    }

    void DcoSecProcVlanAdapter::SubscribeObject(std::string objId)
    {
        SubscribeDcoSecProcVlanState(objId);
    }

    void DcoSecProcVlanAdapter::UnSubscribeObject(std::string objId)
    {
        UnSubscribeDcoSecProcVlanState(objId);
    }

    void DcoSecProcVlanAdapter::SubscribeDcoSecProcVlanState(std::string& carrierId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " sub " << carrierId;
        Dco6SecProcVlanState vlanState;
        vlanState.mutable_base_state()->mutable_config_id()->set_value(carrierId);
        google::protobuf::Message *msg = static_cast<google::protobuf::Message *>(&vlanState);
        
        //Check if an object is already in active subscription
        if( false == IsActiveSubscription(carrierId) )
        {
            DcoSecProcVlanSubContext* pCb = new DcoSecProcVlanSubContext(*this);
            std::string cbId = mspDcoSecProcCrud->Subscribe(msg, pCb);
            INFN_LOG(SeverityLevel::info) << __func__ << " sub after";
            if(pCb) pCb->AddCall(carrierId, cbId);
        }
   }


    void DcoSecProcVlanAdapter::UnSubscribeDcoSecProcVlanState(std::string& carrierId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " unsubscr " << carrierId;
        try{
            CleanupOnUnSubscribe(carrierId);
        }
        catch(const std::exception& e)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " exception in unsubscribe " << carrierId << " " << e.what();
        }
    
    }

    void DcoSecProcVlanAdapter::SetConnectionState(bool isConnectUp)
    {  
        if( mIsConnectUp != isConnectUp )
        {
            SetConnState(isConnectUp);

            if(!isConnectUp)
            {
                //If connection is going down- unsubscribe
                for(auto iter : mCarrierSet)
                {
                    std::string ikeCarrId;
                    UnSubscribeDcoSecProcVlanState(iter);
                    GenerateIkeCarrierId(iter, ikeCarrId);
                    UnSubscribeDcoSecProcVlanState(ikeCarrId);
                }
                mCarrierSet.clear();
            }

        }
        mIsConnectUp =  isConnectUp;
    }

    void DcoSecProcVlanAdapter::GenerateIkeCarrierId(std::string carrierId, std::string& ikeCarrierId)
    {
        ikeCarrierId = "IKE-";
        ikeCarrierId.append(carrierId);
    }

    void DcoSecProcVlanAdapter::CarrierCreate(std::string& carrierId)
    {
        std::string ikeCarrId;
        mCarrierSet.emplace(carrierId);
        SubscribeDcoSecProcVlanState(carrierId);
        GenerateIkeCarrierId(carrierId, ikeCarrId);
        SubscribeDcoSecProcVlanState(ikeCarrId);
    }

    void DcoSecProcVlanAdapter::CarrierDelete(std::string& carrierId)
    {
        auto iter = mCarrierSet.find(carrierId);
        if(iter != mCarrierSet.end())
        {
            std:string ikeCarrId;
            mCarrierSet.erase(iter);
            
            //Unsubscribe vlan state
            UnSubscribeObject(carrierId);
            GenerateIkeCarrierId(carrierId, ikeCarrId);
            UnSubscribeObject(ikeCarrId);

        }
        //@todo : Error handling when carrier Id not found
    }

    void DcoSecProcVlanAdapter::DumpVlanSubscriptions(ostream& out)
    {
        out << "=====Vlan state Subscriptions=====";
        DumpSubscriptionData(out);
    }

//-------------------------DcoSecProcVlanSubContext APIs------------------------------------------

    bool DcoSecProcVlanSubContext::AddCall(std::string carrier, std::string cbId) 
    {
        std::lock_guard<std::mutex> lock(mSubCbMutex);
        if(mCallMap.find(cbId) == mCallMap.end())
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Added cbId: " << cbId << " " << carrier << std::endl; 
            mCallMap.insert(std::pair<std::string, std::string>(cbId, carrier) );
            return true;
        }
        return false;
    }

    void DcoSecProcVlanSubContext::HandleSubResponse(google::protobuf::Message& obj,
            grpc::Status status, std::string cbId)
    {
        std::string secprocStateStr;

        auto iter = mCallMap.find(cbId);
        if( status.error_code() == grpc::StatusCode::OK )
        {
            Dco6SecProcVlanState* cp = static_cast<Dco6SecProcVlanState*>(&obj);
            MessageToJsonString(*cp, &secprocStateStr);
            /*INFN_LOG(SeverityLevel::info) << __func__ << " DcoSecProc Vlan update triggerred!!!" << " , cbId: " << cbId  <<
                " " << secprocStateStr << std::endl;*/
            mAdapterRef.UpdateVlanState(*cp);

            //Remove if present is re-sub list since first successful response
            mAdapterRef.RemoveFromReSubscriptionList(iter->second, this);
        }
        else
        {
            //Error : Subscription failed case
            //Call adapter to indicate the subscription has failed
            //Add retry logic
            if(iter != mCallMap.end())
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " Error!" << " , cbId: " << cbId << " " << status.error_code() << std::endl; 
                RetryObjWrapper carrier(iter->second);
                mAdapterRef.AddToReSubscriptionList(carrier);
                mCallMap.erase(iter);
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " callback not found " << std::endl;
            }
            
        }

    }

}//namespace DpAdapter
