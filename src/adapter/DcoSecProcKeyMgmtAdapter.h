#ifndef DCO_SEC_PROC_KEY_MGMT_ADAPTER_H
#define DCO_SEC_PROC_KEY_MGMT_ADAPTER_H

#include "CrudService.h"
#include <boost/thread.hpp>
#include "DcoSecProcAdapterProtoDefs.h"
#include "DpProtoDefs.h"
#include <mutex>
#include "RetryHelperIntf.h"
#include "CardAdapter.h"
#include "KeyMgmtConfigData.h"

// #define MAX_RETRY_COUNT 100 
#define RETRY_INTERVAL 5

namespace DpAdapter 
{
    using KeyMgrCallback = std::function<bool(Chm6KeyMgmtState&)>;

    class DcoSecProcKeyMgmtAdapter : public RetryHelperIntf
    {
         public:
            
            DcoSecProcKeyMgmtAdapter();

	        ~DcoSecProcKeyMgmtAdapter();

            void UpdateKeyMgmtConfig(Chm6KeyMgmtConfig* keyConfig);

            void ConvertDco6SecProcKeyMgmtStateToChm6(Dco6SecProcKeyMgmtState& stateObj,Chm6KeyMgmtState& keyState);

	        //void UpdateKeyMgmtData(chm6_common::Chm6CmnDcoSecKeyMgmtState &stateObj);

            void UpdateKeyMgmtState(Dco6SecProcKeyMgmtState &stateObj);
            
            void SubscribeObject(std::string objId);
	    			
            void UnSubscribeObject(std::string objId);
       	    
            void Initialize();

            void SetConnectionState(bool isConnectUp);

            std::string GetChm6StateObjConfigId();

            void SyncFromCache();

            void RegisterStateCreateCallback(KeyMgrCallback f) { mCreateKeyStCb = f; };
            void RegisterStateUpdateCallback(KeyMgrCallback f) { mUpdateKeyStCb = f; };
            void RegisterStateDeleteCallback(KeyMgrCallback f) { mDeleteKeyStCb = f; };

	     private:
            std::shared_ptr<::GnmiClient::NBGnmiClient> mspDcoSecProcCrud;
            std::string mKeyMgmtObjId;
            std::mutex mDcoSecMutex;
            bool mIsCardSigned;
            
            Dco6SecProcKeyMgmtState mCurrState;
            bool mIsConnectUp;

            std::map<std::string, KeyMgmtConfigData*> mDcoSecProcKeyMgmtConfigMap;

            KeyMgrCallback mCreateKeyStCb;
            KeyMgrCallback mUpdateKeyStCb;
            KeyMgrCallback mDeleteKeyStCb;

    };

    //DCO secproc KeyMgmt state callback context
    class DcoSecProcKeyMgmtSubContext: public GnmiClient::AsyncSubCallback
    {
        std::map<std::string, std::string> mCallMap; // cbId, KeyMgmt_id
        DcoSecProcKeyMgmtAdapter& mAdapterRef;
        std::mutex mSubCbMutex;

        public:
        DcoSecProcKeyMgmtSubContext( DcoSecProcKeyMgmtAdapter& ref ): mAdapterRef(ref) { }
        ~DcoSecProcKeyMgmtSubContext() {};
        /*
         * Function to add carrierId and subscription to callmap
         * Callmap indicates which objects are being subscribed
         * @param: KeyMgmt Id ( same as carrier id )
         * @param: Unique callback id returned by async callback
         * @return: true if added to map
         */
        bool AddCall(std::string KeyMgmt, std::string cbId);

        /**
         * Function to handle DcoSecProcKeyMgmt update from nxp and cache the response locally
         * @param google::protobuf::Message aka KeyMgmtState Msg 
         * @param grpc call status
         * @param callback Id
         * @return None
         */
        void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);
    };
 
}

#endif

