/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_SEC_PROC_ADAPTER_H
#define DCO_SEC_PROC_ADAPTER_H
#include "CrudService.h"
#include <boost/thread.hpp>
#include "DcoSecProcAdapterProtoDefs.h"
#include "DpProtoDefs.h"
#include <mutex>
#include "RetryHelperIntf.h"

namespace DpAdapter {

    class DcoSecProcAdapter: public RetryHelperIntf
    {
        public:
            static DcoSecProcAdapter* GetInstance();
            ~DcoSecProcAdapter();

            /*
             * Fuction implemented from the base class to do the object resubscription
             * @param: Id of the card object
             **/
            void SubscribeObject(std::string objId);
            void UnSubscribeObject(std::string objId);

            void SubscribeDcoSecProcCardState(std::string boardId);
            void UnSubscribeDcoSecProcCardState(std::string boardId);

            void SubscribeDcoSecProcCardFault(std::string boardFaultObjId);
            void UnSubscribeDcoSecProcCardFault(std::string boardFaultObjId);
            
            void GetDcoSecProcCardState();
            Dco6SecprocBoardState GetCurrentCardState() { std::lock_guard<std::mutex> guard(mDcoSecMutex);
                return mCurrState; };
            void UpdateUpgradableDeviceList(chm6_common::Chm6DcoCardState &stateObj);
            void SetBoardConfig(Dco6SecprocBoardConfig *pCfg);
            void Start();
            void UpdateBoardState(const Dco6SecprocBoardState& state);
            void UpdateCardConfig(chm6_common::Chm6DcoConfig* pMsg);
            void SetConnectionState(bool isConnectUp);
            hal_common::FaultType GetSecprocBoardFaults() { std::lock_guard<std::mutex> guard(mDcoSecMutex);
                return mCurrFault.secproc_fault(); };

            std::string GetCardId(){ return mCardId; };
            std::string GetCardFaultObjId(){ return std::string(mCardId+"_fault"); };

            /*
            * Dump board subscription data
            * @param : output stream [out]
            */
            void DumpBoardSubscriptions(std::ostream& out);

            //Function to update board faults
            void UpdateBoardFault(const Dco6SecprocFault& fault);

        private:
            std::string mCardId;
            static DcoSecProcAdapter* mInstance;
            DcoSecProcAdapter();
            std::mutex mDcoSecMutex;
            Dco6SecprocBoardState mCurrState;
            Dco6SecprocFault mCurrFault;
            std::shared_ptr<::GnmiClient::NBGnmiClient> mspDcoSecProcCrud;
            bool mIsConnectUp;

    };

    //DCO secproc state callback context
    class DcoSecProcStateSubContext: public GnmiClient::AsyncSubCallback
    {
        std::string mCallId;
        Dco6SecprocBoardState mCurrState;
        DcoSecProcAdapter& mAdapterRef;
        std::mutex mSubCbMutex;

        public:
        DcoSecProcStateSubContext(DcoSecProcAdapter& ref): mCallId(""), mAdapterRef(ref) { }
        ~DcoSecProcStateSubContext() {}
        void AddCall(std::string cbId) { mCallId = cbId; }

        void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

    };

    //DCO secproc fault callback context
    class DcoSecProcFaultSubContext: public GnmiClient::AsyncSubCallback
    {
        std::string mCallId;
        Dco6SecprocFault mCurrFault;
        DcoSecProcAdapter& mAdapterRef;
        std::mutex mSubCbMutex;

        public:
        DcoSecProcFaultSubContext(DcoSecProcAdapter& ref): mCallId(""), mAdapterRef(ref) { }
        ~DcoSecProcFaultSubContext() {}
        void AddCall(std::string cbId) { mCallId = cbId; }

        void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);
    };


}
#endif
