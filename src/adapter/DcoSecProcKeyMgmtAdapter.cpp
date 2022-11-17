#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <boost/thread.hpp>
#include "DcoSecProcKeyMgmtAdapter.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "dp_common.h"
#include "DcoConnectHandler.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include "DataPlaneMgr.h"

using google::protobuf::util::MessageToJsonString;
using google::protobuf::util::MessageDifferencer;
using google::protobuf::Message;

using namespace DataPlane;

namespace DpAdapter 
{

    DcoSecProcKeyMgmtAdapter::DcoSecProcKeyMgmtAdapter():RetryHelperIntf(RETRY_INTERVAL, MAX_RETRY_COUNT)
    {
        //grpc server address of the form <ip>:<port> eg - 192.168.1.2:50051
        std::string dco_secproc_grpc = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoNxp());
        mspDcoSecProcCrud = std::dynamic_pointer_cast<GnmiClient::NBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_NXP));
        if (mspDcoSecProcCrud == NULL)
        {
            INFN_LOG(SeverityLevel::error) <<  __func__ << "Dco secproc interface is NULL" << '\n';
        }
        mKeyMgmtObjId = "DCOSECPROC";
        mIsConnectUp = true;
        mIsCardSigned = false;
    }

    DcoSecProcKeyMgmtAdapter::~DcoSecProcKeyMgmtAdapter()
    {
        for(auto iter:mDcoSecProcKeyMgmtConfigMap)
        {
            if(iter.second)
            {
                delete iter.second;
            }
        }

    }

    void DcoSecProcKeyMgmtAdapter::Initialize()
    {
        //Assumption: if chm6 is not signed secproc will not be signed as well
        mIsCardSigned = dp_env::DpEnvSingleton::Instance()->isCardSigned();
        if(true == mIsCardSigned)
        {
            SubscribeObject(mKeyMgmtObjId);
        }
    }

    std::string DcoSecProcKeyMgmtAdapter::GetChm6StateObjConfigId()
    { 
        return ( DataPlaneMgrSingleton::Instance()->getChassisIdStr() + "-" + dp_env::DpEnvSingleton::Instance()->getSlotNum() + "-" + "NXP");
    }

    void DcoSecProcKeyMgmtAdapter::UpdateKeyMgmtConfig( Chm6KeyMgmtConfig* pConfig )
    {
        std::string configStr;
        MessageToJsonString(*pConfig, &configStr);
        INFN_LOG(SeverityLevel::debug) << __func__ << " secproc KeyMgmt config " << 
                " " << configStr << std::endl;

        if(pConfig->has_keymgmt_config())
        {
            hal_keymgmt::KeyMgmtOperation operation = pConfig->keymgmt_config().operation();

            if (operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY ||
                    operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY)
            {
                hal_keymgmt::ImageSigningKey isk = pConfig->keymgmt_config().isk();

                {   //Taking secproc mutex
                    
                    std::lock_guard<std::mutex> guard(mDcoSecMutex);
                    //Check if key exists in the local cache
                    auto iter = mDcoSecProcKeyMgmtConfigMap.find(isk.key_name().value());
                    if(iter != mDcoSecProcKeyMgmtConfigMap.end())
                    {
                        KeyMgmtConfigData* pData = iter->second;
                        if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY) 
                        {
                            if(pData && pData->IsKeyDeleted())
                            {
                                INFN_LOG(SeverityLevel::info) << __func__ << " Duplicate DELETE request, dropping " << isk.key_name().value();
                                //Duplicate delete config - drop and return
                                return;
                            }

                            if(pData)
                            {                       
                                pData->ClearKey();
                            }

                        }
                        if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY)
                        {
                            if( pData && (false == pData->IsKeyDeleted()) )
                            {
                                INFN_LOG(SeverityLevel::info) << __func__ << " Duplicate add request, dropping " << isk.key_name().value();
                                //Duplicate add operation - drop request
                                return;
                            }

                            if( pData )
                            {
                                //add key
                                pData->SetKey(isk);
                            }
                        }
                    }
                    else
                    {
                        //Key not found in cache
                        bool isDeleted = false;
                        if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY)
                        {
                            isDeleted = true;
                        }
                        else if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY)
                        {
                            isDeleted = false;
                        }

                        KeyMgmtConfigData* pNewData = new KeyMgmtConfigData(isk, isDeleted);
                        mDcoSecProcKeyMgmtConfigMap[isk.key_name().value()] = pNewData;
                        INFN_LOG(SeverityLevel::info) << __func__ << " Added to cache " << isk.key_name().value();
                    }
                }//release mutex
            }
        }

    	//Convert Chm6KeyMgmtConfig to DcoSecKeyMgmtConfig
        Dco6SecProcKeyMgmtConfig dcoSecKeyCfg;
        
        dcoSecKeyCfg.mutable_base_config()->mutable_config_id()->set_value(mKeyMgmtObjId);
        dcoSecKeyCfg.mutable_config()->set_operation(pConfig->keymgmt_config().operation());
        hal_keymgmt::ImageSigningKey* pIsk=dcoSecKeyCfg.mutable_config()->mutable_isk();
        pIsk->CopyFrom( pConfig->keymgmt_config().isk() );

        struct timeval tv;
        gettimeofday(&tv, NULL);

        dcoSecKeyCfg.mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
        dcoSecKeyCfg.mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
        google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(&dcoSecKeyCfg);
        
        //Call a gnmi set
        //@todo check if the set was successful
        mspDcoSecProcCrud->Set(cfgMsg);
    }
    

    void DcoSecProcKeyMgmtAdapter::ConvertDco6SecProcKeyMgmtStateToChm6(Dco6SecProcKeyMgmtState& stateObj,Chm6KeyMgmtState& keyState)
    {
        keyState.mutable_keymgmt_state()->mutable_krk()->CopyFrom(stateObj.state().krk());
        keyState.mutable_keymgmt_state()->mutable_idevid()->CopyFrom(stateObj.state().idevid());
        auto pIskMap = keyState.mutable_keymgmt_state()->mutable_isk_map();
        if(pIskMap)
        {
            pIskMap->insert(stateObj.state().isk_map().begin(), stateObj.state().isk_map().end());
        }
    }
    
    void DcoSecProcKeyMgmtAdapter::UpdateKeyMgmtState(Dco6SecProcKeyMgmtState &stateObj) 
    {
        std::lock_guard<std::mutex> guard(mDcoSecMutex);
        std::string keyMgmtId = mCurrState.base_state().config_id().value();
        Chm6KeyMgmtState keyState;
        
        keyState.mutable_base_state()->mutable_config_id()->set_value(GetChm6StateObjConfigId());

        if(keyMgmtId == "")
        {
            //New creation
            mCurrState.CopyFrom(stateObj);
            ConvertDco6SecProcKeyMgmtStateToChm6(stateObj, keyState);
            mCreateKeyStCb(keyState);
        }
        else
        {
            const Dco6SecProcKeyMgmtState::State& stateData = mCurrState.state();
            if(false == MessageDifferencer::Equals(stateObj.state(), stateData))
            {
                mCurrState.CopyFrom(stateObj);
                ConvertDco6SecProcKeyMgmtStateToChm6(stateObj, keyState);
                //Call API in the manager, Same call context as grpc
                //Add a callback in the manager
                if((stateObj.base_state().mark_for_delete().value() == true))
                {
                    mDeleteKeyStCb(keyState);
                    mCurrState.Clear();
                    return;
                }
            
                mUpdateKeyStCb(keyState);
            }
        }
    }

    void DcoSecProcKeyMgmtAdapter::SubscribeObject(std::string objId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " sub " << objId;
        //@todo: Change variable name from icdp to keymgmt
        Dco6SecProcKeyMgmtState icdpState;
        icdpState.mutable_base_state()->mutable_config_id()->set_value(objId);
        google::protobuf::Message *msg = static_cast<google::protobuf::Message *>(&icdpState);
        
        if( false == IsActiveSubscription(objId) )
        {
            DcoSecProcKeyMgmtSubContext* pCb = new DcoSecProcKeyMgmtSubContext(*this);
            std::string cbId = mspDcoSecProcCrud->Subscribe(msg, pCb);
            if(pCb) pCb->AddCall(objId, cbId);
        }

    }

    void DcoSecProcKeyMgmtAdapter::UnSubscribeObject(std::string objId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " unsubscr " << objId;
        try 
        {
            CleanupOnUnSubscribe(objId);
        }
        catch(const std::exception& e)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " exception in unsubscribe " << objId << " " << e.what();
        }
    }

   void DcoSecProcKeyMgmtAdapter::SetConnectionState(bool isConnectUp)
    {  
        if( mIsConnectUp != isConnectUp )
        {
            SetConnState(isConnectUp);
            if( !isConnectUp)
            {
                if(mIsCardSigned)
                {
                    UnSubscribeObject(mKeyMgmtObjId);
                }
            }
            else
            {
                if(mIsCardSigned)
                {
                    SubscribeObject(mKeyMgmtObjId);
                }
                SyncFromCache();
            }
        }
        mIsConnectUp =  isConnectUp;
    }

   //Subscription to dcsosecproc

    bool DcoSecProcKeyMgmtSubContext::AddCall(std::string id, std::string cbId) 
    {
        std::lock_guard<std::mutex> lock(mSubCbMutex);
        if(mCallMap.find(cbId) == mCallMap.end())
        {
            INFN_LOG(SeverityLevel::debug) << __func__ << " Added cbId: " << cbId << " " << id << std::endl; 
            mCallMap.insert(std::pair<std::string, std::string>(cbId, id) );
            return true;
        }
        return false;
    }

    void DcoSecProcKeyMgmtSubContext::HandleSubResponse(google::protobuf::Message& obj,
            grpc::Status status, std::string cbId)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Update triggered !\n";
        std::string secprocStateStr;
        std::map<std::string, std::string>::iterator iter;
        {
            std::lock_guard<std::mutex> lock(mSubCbMutex);
            iter = mCallMap.find(cbId);
            if(iter == mCallMap.end())
            {
                INFN_LOG(SeverityLevel::debug) << __func__ << " Error! DcoSecProc KeyMgmt cbId not found! ";
                return;
            }
        }

        if( status.error_code() == grpc::StatusCode::OK )
        {
            try
            {
                Dco6SecProcKeyMgmtState* cp = static_cast<Dco6SecProcKeyMgmtState*>(&obj);
                MessageToJsonString(*cp, &secprocStateStr);
                INFN_LOG(SeverityLevel::debug) << __func__ << " DcoSecProc KeyMgmt update triggerred!!!" << " , cbId: " << cbId  <<
                    " " << secprocStateStr << std::endl;
                mAdapterRef.UpdateKeyMgmtState(*cp);

                //Remove if present is re-sub list since first successful response
                mAdapterRef.RemoveFromReSubscriptionList(iter->second, this);
            }
            catch(const std::exception& e)
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " exception " << e.what();               
            }
        }
        else
        {
            //Error : Subscription failed case
            //Call adapter to indicate the subscription has failed
            INFN_LOG(SeverityLevel::debug) << __func__ << " Error!"
                << " , cbId: " << cbId << " " << status.error_code() << std::endl;
            RetryObjWrapper KeyMgmt(iter->second);
            mAdapterRef.AddToReSubscriptionList(KeyMgmt);

            {
                std::lock_guard<std::mutex> lock(mSubCbMutex);
                mCallMap.clear();
            }
        }

    }

    void DcoSecProcKeyMgmtAdapter::SyncFromCache()
    {
        std::lock_guard<std::mutex> guard(mDcoSecMutex);
        std::map<std::string, KeyMgmtConfigData*>::iterator iter;
        for(iter = mDcoSecProcKeyMgmtConfigMap.begin(); iter != mDcoSecProcKeyMgmtConfigMap.end(); iter++)
        {
            KeyMgmtConfigData* pData = iter->second;
            std::string keyName(iter->first);
            if(pData)
            {
                hal_keymgmt::ImageSigningKey isk = pData->GetKey();

                //Convert Chm6KeyMgmtConfig to DcoSecKeyMgmtConfig
                Dco6SecProcKeyMgmtConfig dcoSecKeyCfg;
                dcoSecKeyCfg.mutable_base_config()->mutable_config_id()->set_value(mKeyMgmtObjId);
                hal_keymgmt::ImageSigningKey* pIsk=dcoSecKeyCfg.mutable_config()->mutable_isk();
                pIsk->CopyFrom( isk );

                struct timeval tv;
                gettimeofday(&tv, NULL);

                dcoSecKeyCfg.mutable_base_config()->mutable_timestamp()->set_seconds(tv.tv_sec);
                dcoSecKeyCfg.mutable_base_config()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);
                google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(&dcoSecKeyCfg);

                if( true ==  pData->IsKeyDeleted() )
                {
                    //Call delete key operation
                    pIsk->mutable_key_name()->set_value(keyName);
                    dcoSecKeyCfg.mutable_config()->set_operation(hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY);
                }
                else
                {
                    //Key getting added
                    dcoSecKeyCfg.mutable_config()->set_operation(hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY);

                }
                
                mspDcoSecProcCrud->Set(cfgMsg);
            }//end if
        }//end for
    }

}

