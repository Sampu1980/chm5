/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_SEC_PROC_VLAN_ADAPTER_H
#define DCO_SEC_PROC_VLAN_ADAPTER_H
#include "CrudService.h"
#include "DcoSecProcAdapterProtoDefs.h"
#include "GccControlAdapter.h"
#include "dp_common.h"
#include "DpProtoDefs.h"
#include <mutex>
#include <unordered_set>
#include "RetryHelperIntf.h"


namespace DpAdapter {

    using MgrCallback = std::function<bool(std::string, DataPlane::ControlChannelVlanIdx, ControlChannelVlanCommon&)>; 
    using MgrPcpCallback = std::function<void(Chm6PeerDiscVlanState&)>; 

    class DcoSecProcVlanSubContext;
   
    class DcoSecProcVlanAdapter: public RetryHelperIntf
    {
        public:
            DcoSecProcVlanAdapter();
            ~DcoSecProcVlanAdapter();

            /*
             * Fuction implemented from the base class to do the object resubscription
             * @param: Id of the Icdp object ( same as carrierId )
             **/
            void SubscribeObject(std::string objId);
            void UnSubscribeObject(std::string objId);

            /**
            * Subscribe to updates in DcoSecProc VlanSetting object
            * Asynchronous gnmi subscription
            * TBD : Add retry on failure, remove hardcoded value of board id ?
            * Converts DcoSecProc objects to chm6 objects and vice versa
            **/
            void SubscribeDcoSecProcVlanState(std::string& carrierId);

            void UnSubscribeDcoSecProcVlanState(std::string& carrierId);

            /*
            * Listens to vlan settings from nxp
            * Calls manager to configure DCO zync
            * @param Vlan State protobuf object
            * 
            */
            void UpdateVlanState(const Dco6SecProcVlanState &vlanState);

            /*
            * Register callback functions to be called on the vlan creation, deletion
            */
            void RegisterVlanCreateCallback(MgrCallback f) { mCreateVlanCb = f; };
            void RegisterVlanDeleteCallback(MgrCallback f) { mDeleteVlanCb = f; };

            /*
             * Register callback functions to update Peer discovery PCP state
             */
            void RegisterPcpCreateCallback(MgrPcpCallback f) { mCreatePcpCb = f; };
            void RegisterPcpDeleteCallback(MgrPcpCallback f) { mDeletePcpCb = f; };

            /*
            * Set DCO SecProc connection state
            */
            void SetConnectionState(bool isConnectUp);

            /*
             * Generate IKE Id for a givencarrier
             */
            void GenerateIkeCarrierId(std::string carrierId, std::string& ikeCarrierId);

            /*
            * Notify carrier create
            * @param : Carrier Id
            */
           void CarrierCreate(std::string& carrierId);

            /*
            * Notify carrier delete
            * @param : Carrier Id
            */
           void CarrierDelete(std::string& carrierId);

           /* Data conversion functions */
           /*
            * Convert DCO secproc data structure to PCP microservice datastructure
            * @param : Dco6SecProcVlanState (Input)
            * @param : Chm6PeerDiscVlanState (Output)
            * @return : true if conversion is successful
            */
           bool ConvertSecProcToPcpVlanState(Dco6SecProcVlanState& state,\
                   Chm6PeerDiscVlanState& pcpState);

           /*
            * Convert DCO secproc data structure to GCC datastructure
            * @param : Dco6SecProcVlanState (Input)
            * @param : ControlChannelVlanIdx (Output)
            * @param : ControlChannelVlanCommon (Output)
            * @return : true if conversion is successful
            */
           bool ConvertSecProcToGccVlanState(Dco6SecProcVlanState& state,\
                   DataPlane::ControlChannelVlanIdx& idx,\
                   ControlChannelVlanCommon& cmn);
            
            /*
            * Dump Vlan subscription data
            * @param : output stream [out]
            */
            void DumpVlanSubscriptions(ostream& out);

        private : 
            std::shared_ptr<::GnmiClient::NBGnmiClient> mspDcoSecProcCrud;

            //Callbacks to call vlan creation and deletion
            MgrCallback mCreateVlanCb;
            MgrCallback mDeleteVlanCb;

            //Callbacks for PCP MS objects
            MgrPcpCallback mCreatePcpCb;
            MgrPcpCallback mDeletePcpCb;

            std::mutex mDcoSecMutex;
            std::mutex mDcoSecStateMutex;
            std::map<std::string, Dco6SecProcVlanState*> mCurrStateMap; //carrierId as key
            std::unordered_set<std::string> mChangeList;//carrierId upate list - rename
            std::set<std::string> mCarrierSet;
            bool mIsConnectUp;
            std::thread mSubThread;
    };

    //DCO secproc vlan state callback context
    class DcoSecProcVlanSubContext: public GnmiClient::AsyncSubCallback
    {
        std::map<std::string, std::string> mCallMap;
        DcoSecProcVlanAdapter& mAdapterRef;
        std::mutex mSubCbMutex;

        public:
        DcoSecProcVlanSubContext( DcoSecProcVlanAdapter& ref ): mAdapterRef(ref) { }
        ~DcoSecProcVlanSubContext() {};
        /*
        * @todo: Handle error case where the same carrierid gets added to map twice and 
        * Function to add carrierId and subscription to callmap
        * Callmap indicates which objects are being subscribed
        * @param: carrier Id
        * @param: Unique callback id returned by async callback
        * @return: true if carrier doesnt exist and id added successfully, false if carrier id exists
        */
        bool AddCall(std::string carrier, std::string cbId);

        /**
        * Function to handle DcoSecProcVlan update from nxp and cache the response locally
        * @param google::protobuf::Message aka VlanSettings Msg 
        * @param grpc call status
        * @param callback Id
        * @return None
        */
        void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);
    };



}
#endif
