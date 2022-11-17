#ifndef DCO_SEC_PROC_ICDP_ADAPTER_H
#define DCO_SEC_PROC_ICDP_ADAPTER_H
#include "CrudService.h"
#include "DcoSecProcAdapterProtoDefs.h"
#include "RetryHelperIntf.h"
#include "GccControlAdapter.h"
#include "dp_common.h"
#include "DpProtoDefs.h"
#include <mutex>
#include <unordered_set>

namespace DpAdapter {

    using IcdpMgrCallback = std::function<bool(Chm6IcdpState&)>;
    class DcoSecProcIcdpSubContext;
    class DcoSecProcIcdpAdapter: public RetryHelperIntf
    {
        public:
            DcoSecProcIcdpAdapter();
            ~DcoSecProcIcdpAdapter();
            /*
             * Fuction implemented from the base class to do the object resubscription
             * @param: Id of the Icdp object ( same as carrierId )
             **/
            void SubscribeObject(std::string objId);
            void UnSubscribeObject(std::string objId);

            /**
             * Subscribe to updates in DcoSecProc VlanSetting object
             * Asynchronous gnmi subscription
             * Converts DcoSecProc objects to chm6 objects and vice versa
             **/
            void SubscribeDcoSecProcIcdpState(std::string icdpId);
            void UnSubscribeDcoSecProcIcdpState(std::string icdpId);

            /*
             * Listens to vlan settings from nxp
             * Calls manager to configure DCO zync
             * @param Icdp State protobuf object
             * 
             */
            void UpdateIcdpState(Dco6SecProcIcdpState &icdpState);

            /*
             * Set DCO SecProc connection state
             * @param : Connection status up/down
             */
            void SetConnectionState(bool isConnectUp);

            /*
             *  DCO SecProc ICDP creation
             * @param : [in] IcdpNodeInfo ( Icdp Node config proto from chm6 )
             */
            void UpdateIcdpNode(const IcdpNodeInfo& icdp);

            /*
             * DCO SecProc ICDP deletion
             */
            void DeleteIcdpNode();

            /*
             *  DCO SecProc carrier creation- which will trigger Icdp creation
             * @param : [in] carrierId
             */
            void CarrierCreate(std::string carrierId);

            /*
             * DCO SecProc carrier deletion which will trigger ICDP deletion
             * @param : [in] carrierId
             */
            void CarrierDelete(std::string carrierId);

            bool GetIcdpEnable();
            /*
             * Convert chm6 object to DCo secproc config
             * @param : Icdp Id
             * @param : IcdpNodeInfo from Chm6 proto
             * @param : DcoSecProcConfig ( Data in this will be filled on translation from IcdpNodeInfo)
             * @param : isCreate ( true by default )
             */
            bool ConvertChm6ToDco6SecProcIcdpNodeConfig(std::string icdpId, 
                    IcdpNodeInfo& node, Dco6SecProcIcdpConfig* pCfg, bool isCreate = true);

            /*
             * Convert DcoSecProc Icdp state object to Chm6 Icdp state object
             * @param : DCO Secproc ICDP state[in]
             * @param : CHM6 ICDP state[out]
             */
            void ConvertDco6SecProcIcdpNodeStateToChm6(Dco6SecProcIcdpState& dcoIcdpState, Chm6IcdpState& chmIcdpState);

            /*
             * Register callback functions to be called on the vlan creation, deletion
             */
            void RegisterIcdpCreateCallback(IcdpMgrCallback f) { mCreateIcdpStCb = f; };
            void RegisterIcdpUpdateCallback(IcdpMgrCallback f) { mUpdateIcdpStCb = f; };
            void RegisterIcdpDeleteCallback(IcdpMgrCallback f) { mDeleteIcdpStCb = f; };

            /*
            * Dump ICDP subscription data
            * @param : output stream [out]
            */
            void DumpIcdpSubscriptions(ostream& out);

        private : 
            std::shared_ptr<::GnmiClient::NBGnmiClient> mspDcoSecProcCrud;

            std::mutex mDcoSecMutex;
            std::mutex mDcoSecCfgMutex;
            std::map<std::string, Dco6SecProcIcdpState*> mCurrStateMap; //carrierId as key
            std::map<std::string, bool> mIcdpConfigMap;
            bool mIsConnectUp;

            IcdpMgrCallback mCreateIcdpStCb;
            IcdpMgrCallback mUpdateIcdpStCb;
            IcdpMgrCallback mDeleteIcdpStCb;

            //Per node Icdp information
            IcdpNodeInfo mIcdpNodeInfo;
    };

    //DCO secproc icdp state callback context
    class DcoSecProcIcdpSubContext: public GnmiClient::AsyncSubCallback
    {
        std::map<std::string, std::string> mCallMap; // cbId, icdp_id
        DcoSecProcIcdpAdapter& mAdapterRef;
        std::mutex mSubCbMutex;

        public:
        DcoSecProcIcdpSubContext( DcoSecProcIcdpAdapter& ref ): mAdapterRef(ref) { }
        ~DcoSecProcIcdpSubContext() {};
        /*
         * Function to add carrierId and subscription to callmap
         * Callmap indicates which objects are being subscribed
         * @param: icdp Id ( same as carrier id )
         * @param: Unique callback id returned by async callback
         * @return: true if added to map
         */
        bool AddCall(std::string icdp, std::string cbId);

        /**
         * Function to handle DcoSecProcIcdp update from nxp and cache the response locally
         * @param google::protobuf::Message aka IcdpState Msg 
         * @param grpc call status
         * @param callback Id
         * @return None
         */
        void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);
    };



}
#endif
