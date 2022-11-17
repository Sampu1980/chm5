/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <boost/thread.hpp>
#include "DcoSecProcAdapter.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "dp_common.h"
#include <google/protobuf/util/json_util.h>
#include "DcoConnectHandler.h"

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace DataPlane;

namespace DpAdapter {

    DcoSecProcAdapter* DcoSecProcAdapter::mInstance = NULL;
    
    /**
     * Adapter singleton class constructor 
     * Converts DcoSecProc objects to chm6 objects and vice versa
     **/
    DcoSecProcAdapter::DcoSecProcAdapter():RetryHelperIntf(RETRY_INTERVAL, MAX_RETRY_COUNT)
    {
        mCardId = "board-1";
        std::string dco_secproc_grpc = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoNxp());
        INFN_LOG(SeverityLevel::info) << __func__ << "NXP server name:" << dco_secproc_grpc;
        mspDcoSecProcCrud = std::dynamic_pointer_cast<GnmiClient::NBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_NXP));

        if (mspDcoSecProcCrud == NULL)
        {
            INFN_LOG(SeverityLevel::error) <<  __func__ << "Dco secproc interface is NULL" << '\n';
        }
        mIsConnectUp = false;
        GetDcoSecProcCardState();
    }

    DcoSecProcAdapter::~DcoSecProcAdapter()
    {

    }

    DcoSecProcAdapter* DcoSecProcAdapter::GetInstance()
    {
        if(NULL == mInstance){
            mInstance =  new DcoSecProcAdapter();
        }
        return mInstance;

    }
    
    /**
     * Subscribe to updates in DcoSecProcBoardState object
     * Asynchronous gnmi subscription
     * TBD : Add retry on failure, remove hardcoded value of board id ?
     * Converts DcoSecProc objects to chm6 objects and vice versa
     **/
    void DcoSecProcAdapter::SubscribeDcoSecProcCardState(std::string boardId)
    {
        if( true == IsActiveSubscription(mCardId) )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " subscription already active, returning" << std::endl;
            return;
        }
        
        try
        {
            Dco6SecprocBoardState *pBoardState = new Dco6SecprocBoardState();
            if (!mIsConnectUp)
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "Connection is not up, subscription might fail" << mCardId;
            }
            pBoardState->mutable_base_state()->mutable_config_id()->set_value(mCardId);
            google::protobuf::Message *msg = static_cast<google::protobuf::Message *>(pBoardState);
            ::GnmiClient::AsyncSubCallback *cb = new DcoSecProcStateSubContext(*this); //cleaned up in grpc client 
            std::string cbId = mspDcoSecProcCrud->Subscribe(msg, cb);
            if(cb) { static_cast<DcoSecProcStateSubContext *>(cb)->AddCall(cbId);}
        }
        catch (std::exception e)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Exception occured .. " << e.what() << std::endl;
            //std::cout << " Retrying after sleep " << retryCount;
        }
    }

    void DcoSecProcAdapter::SubscribeObject(std::string objId)
    {
        std::size_t found = objId.find("_fault");
        if(found != std::string::npos)
        {
            SubscribeDcoSecProcCardFault(GetCardFaultObjId());
        }
        else
        {
            SubscribeDcoSecProcCardState(objId);
        }
    }

    void DcoSecProcAdapter::UnSubscribeObject(std::string objId)
    {
        std::size_t found = objId.find("_fault");
        if(found != std::string::npos)
        {
            UnSubscribeDcoSecProcCardFault(GetCardFaultObjId());
        }
        else
        {
            UnSubscribeDcoSecProcCardState(objId);
        }
    }

    void DcoSecProcAdapter::UnSubscribeDcoSecProcCardState(std::string boardId)
    {

        INFN_LOG(SeverityLevel::info) << __func__ << " unsubscr " << boardId;
        CleanupOnUnSubscribe(boardId);

    }

    void DcoSecProcAdapter::Start()
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " get " << boost::this_thread::get_id() << std::endl;
        SubscribeDcoSecProcCardState(mCardId);
    }

    /**
     * TODO: Store object/return value to caller
     * Function to get DcoSecProcCardState
     * Calls blocking gnmi get 
     * @param None 
     * @return NA
     */
    void DcoSecProcAdapter::GetDcoSecProcCardState()
    {
        Dco6SecprocBoardState* stateObj = new Dco6SecprocBoardState();
        stateObj->mutable_base_state()->mutable_config_id()->set_value(mCardId);
        google::protobuf::Message* stateMsg = static_cast<google::protobuf::Message*>(stateObj);
        
        //Calling synchronous get
        mspDcoSecProcCrud->Get(stateMsg);
        if(stateMsg)
        {
            delete stateMsg;
        }
    }
 
    /**
     * Function to set DcoSecProcCardState
     * Calls blocking gnmi set to dco secproc grpc server
     * @param Dco6ProcBoardConfig* 
     * @return None
     */
    void DcoSecProcAdapter::SetBoardConfig(Dco6SecprocBoardConfig* cfg)
    {
        google::protobuf::Message* cfgMsg = static_cast<google::protobuf::Message*>(cfg);
        mspDcoSecProcCrud->Set(cfgMsg);
    }
    
    /**
     * Function to update cached DcoSecProcCardState
     * @param  const Dco6SecprocBoardState& 
     * @return None
     */
    void DcoSecProcAdapter::UpdateBoardState(const Dco6SecprocBoardState& state)
    {
        INFN_LOG(SeverityLevel::debug) << " \n Updating board state\n";
        std::lock_guard<std::mutex> guard(mDcoSecMutex);
        mCurrState.CopyFrom(state);
    }

    /**
     * Function to update UpgradableDeviceList in chm6 dco board object from cached DcoSecProcCardState
     * @param  chm6_common::Chm6DcoCardState &stateObj 
     * @return None
     */
    void DcoSecProcAdapter::UpdateUpgradableDeviceList(chm6_common::Chm6DcoCardState &stateObj) 
    { 
        //Convert Dco6SecprocBoardState to Chm6DcoCardState upgradable devices
        hal_common::UpgradableDeviceType* chmDev = (stateObj.mutable_upgradable_devices());
        auto chmDevMap = (chmDev->mutable_upgradable_devices());

        std::lock_guard<std::mutex> guard(mDcoSecMutex);
        const Dco6SecprocBoardState::OperationalState& state = mCurrState.state();
        const Dco6SecprocBoardState::UpgradableDeviceType& devType = state.upgradable_devices();
        google::protobuf::Map<std::string,Dco6SecprocBoardState::UpgradableDeviceType::UpgradableDevice> devMap = devType.upgradable_devices();

        for(auto& iter: devMap)
        {
            hal_common::UpgradableDeviceType::UpgradableDevice device;
            (device.mutable_device_name())->set_value(((iter.second).device_name()).value());
            (device.mutable_hw_version())->set_value( ((iter.second).hw_version()).value() );
            (device.mutable_sw_version())->set_value( ((iter.second).sw_version()).value() );
            (device.mutable_fw_update_state())->set_value( ((iter.second).fw_update_state()).value() );
            (device.mutable_file_location())->set_value( ((iter.second).file_location()).value() );
            //device.mutable_fw_update_state(iter->second().device_name());
            (*chmDevMap)[iter.first] = device;
        }

    }

    /**
     * Function to update DcoSecProcBoardConfig on change in the CHM6 DCO board config
     * @param  chm6_common::Chm6DcoConfig* dcoCfg
     * @return None
     */
    void DcoSecProcAdapter::UpdateCardConfig(chm6_common::Chm6DcoConfig* dcoCfg)
    {
        //If icdp only info is updated; do not call the set on nxp
        if( (dcoCfg->dco_card_action() == hal_common::BOARD_ACTION_UNSPECIFIED) )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Return, BOARD_ACTION_UNSPECIFIED. No nxp card update needed";
            return;        
        }
        //@todo - store dco card config board action  - check if different only then call secproc

        Dco6SecprocBoardConfig* pConfig = new Dco6SecprocBoardConfig();

        //Convert from DcoCardConfig to DcoSecProcBoardConfig
        pConfig->mutable_base_config()->mutable_config_id()->set_value(dcoCfg->base_state().config_id().value());

        Dco6SecprocBoardConfig::BoardAction dcoAction; 
        switch(dcoCfg->dco_card_action())
        {
            case hal_common::BOARD_ACTION_UNSPECIFIED:
                dcoAction = Dco6SecprocBoardConfig::BOARD_ACTION_UNSPECIFIED;
                break;
            case hal_common::BOARD_ACTION_GRACEFUL_SHUTDOWN:
            case hal_common::BOARD_ACTION_RESTART_COLD:
                dcoAction = Dco6SecprocBoardConfig::BOARD_ACTION_GRACEFUL_SHUTDOWN;
                break;
            case hal_common::BOARD_ACTION_RESTART_WARM:
                dcoAction = Dco6SecprocBoardConfig::BOARD_ACTION_RESTART_WARM;
                break;
            case hal_common::BOARD_ACTION_UPDATE_FW:
                dcoAction = Dco6SecprocBoardConfig::BOARD_ACTION_UPDATE_FW;
                break;  
            default:
                INFN_LOG(SeverityLevel::info) << "Invalid action, ignoring for secproc";

        };
        INFN_LOG(SeverityLevel::info) << __func__ <<" Dco card action " << dcoAction;

        pConfig->mutable_config()->set_action(dcoAction);

        SetBoardConfig(pConfig);
        delete pConfig;
    }

    void DcoSecProcAdapter::DumpBoardSubscriptions(std::ostream& out)
    {
        out << "=====Secproc Board Subscriptions=====";
        DumpSubscriptionData(out);
    }

    //=================================================================================================
    /**
     * Function to handle DcoSecProcCardState update from nxp and cache the response locally
     * @param google::protobuf::Message aka DcoSecProcCardState Msg 
     * @param grpc call status
     * @param callback Id
     * @return None
     */
    void DcoSecProcStateSubContext::HandleSubResponse(google::protobuf::Message& obj,
            grpc::Status status, std::string cbId)
    {
        if(cbId != mCallId)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Should not see! Invalid response id!! , cbId: " << cbId  <<  std::endl;
            return;
        }	
        std::string secprocStateStr;
        //INFN_LOG(SeverityLevel::info) << __func__ << " DcoSecProcCardState update triggerred!!!" << " , cbId: " << cbId  <<  std::endl;

        if( status.error_code() == grpc::StatusCode::UNKNOWN )
        {
            //Object not available for subscription
            INFN_LOG(SeverityLevel::info) << __func__ << " Object does not exist in the server yet! ,cbId: " << cbId  <<  std::endl;

        }
        else if( status.error_code() == grpc::StatusCode::UNAVAILABLE)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Grpc server unreachable ,cbId: " << cbId  <<  std::endl;
        }
        else if ( status.error_code() == grpc::StatusCode::INTERNAL )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Connection interrupted!, cbId: " << cbId  <<  std::endl;
        }
        else
        {
            Dco6SecprocBoardState* cp = static_cast<Dco6SecprocBoardState*>(&obj);
            MessageToJsonString(*cp, &secprocStateStr);
            (DpAdapter::DcoSecProcAdapter::GetInstance())->UpdateBoardState(*cp);
            INFN_LOG(SeverityLevel::debug) << " Updated Object " << secprocStateStr << std::endl;



            //Remove if present is re-sub list since first successful response
            mAdapterRef.RemoveFromReSubscriptionList(mAdapterRef.GetCardId(), this);
        }

        if( status.error_code() != grpc::StatusCode::OK )
        {
            RetryObjWrapper board(mAdapterRef.GetCardId());
            mAdapterRef.AddToReSubscriptionList(board);
        }

    }

    void DcoSecProcAdapter::SetConnectionState(bool isConnectUp)
    {  
        if( mIsConnectUp != isConnectUp )
        {
            SetConnState(isConnectUp);
            if(isConnectUp)
            {
                SubscribeDcoSecProcCardState(mCardId);
                SubscribeDcoSecProcCardFault(GetCardFaultObjId());
            }
            else
            {
                UnSubscribeDcoSecProcCardState(mCardId);
                UnSubscribeDcoSecProcCardFault(GetCardFaultObjId());
            }

        }
        mIsConnectUp =  isConnectUp;
    } 


    //=================================================================================================
    /**
     * Function to handle DcoSecProcCardFault update from nxp and cache the response locally
     * @param google::protobuf::Message aka DcoSecProcCardFault  Msg 
     * @param grpc call status
     * @param callback Id
     * @return None
     */
    void DcoSecProcFaultSubContext::HandleSubResponse(google::protobuf::Message& obj,
            grpc::Status status, std::string cbId)
    {
        if(cbId != mCallId)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Should not see! Invalid response id!! , cbId: " << cbId  <<  std::endl;
            return;
        }	
        std::string secprocFaultStr;
        
        if( status.error_code() == grpc::StatusCode::UNKNOWN )
        {
            //Object not available for subscription
            INFN_LOG(SeverityLevel::info) << __func__ << " Object does not exist in the server yet! ,cbId: " << cbId  <<  std::endl;

        }
        else if( status.error_code() == grpc::StatusCode::UNAVAILABLE)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Grpc server unreachable ,cbId: " << cbId  <<  std::endl;
        }
        else if ( status.error_code() == grpc::StatusCode::INTERNAL )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Connection interrupted!, cbId: " << cbId  <<  std::endl;
        }
        else
        {
            Dco6SecprocFault* cp = static_cast<Dco6SecprocFault*>(&obj);
            MessageToJsonString(*cp, &secprocFaultStr);
            (DpAdapter::DcoSecProcAdapter::GetInstance())->UpdateBoardFault(*cp);
            INFN_LOG(SeverityLevel::debug) << " Updated Object " << secprocFaultStr << std::endl;

            //Remove if present is re-sub list since first successful response
            mAdapterRef.RemoveFromReSubscriptionList(mAdapterRef.GetCardFaultObjId(), this);
        }

        if( status.error_code() != grpc::StatusCode::OK )
        {
            RetryObjWrapper boardFault(mAdapterRef.GetCardFaultObjId());
            mAdapterRef.AddToReSubscriptionList(boardFault);
        }

    }

    /**
     * Function to update cached DcoSecProcCardFault
     * @param  const Dco6SecprocFault& 
     * @return None
     */
    void DcoSecProcAdapter::UpdateBoardFault(const Dco6SecprocFault& fault)
    {
        INFN_LOG(SeverityLevel::debug) << " \n Updating board fault\n";
        std::lock_guard<std::mutex> guard(mDcoSecMutex);
        mCurrFault.CopyFrom(fault);
    }

    void DcoSecProcAdapter::SubscribeDcoSecProcCardFault(std::string boardFaultObj)
    {
        if( true == IsActiveSubscription(boardFaultObj) )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " subscription already active, returning" << std::endl;
            return;
        }
        
        try
        {
            Dco6SecprocFault *pBoardFault = new Dco6SecprocFault();
            if (!mIsConnectUp)
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "Connection is not up, subscription might fail " << mCardId;
            }
            pBoardFault->mutable_base_fault()->mutable_config_id()->set_value(mCardId);
            google::protobuf::Message *msg = static_cast<google::protobuf::Message *>(pBoardFault);
            ::GnmiClient::AsyncSubCallback *cb = new DcoSecProcFaultSubContext(*this); //cleaned up in grpc client 
            std::string cbId = mspDcoSecProcCrud->Subscribe(msg, cb);
            if(cb) { static_cast<DcoSecProcFaultSubContext *>(cb)->AddCall(cbId);}
        }
        catch (std::exception e)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Exception occured .. " << e.what() << std::endl;
        }
    }

    void DcoSecProcAdapter::UnSubscribeDcoSecProcCardFault(std::string boardFaultObj)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " unsubscr " << boardFaultObj;
        CleanupOnUnSubscribe(boardFaultObj);
    }
    
}
