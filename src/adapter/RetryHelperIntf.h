/************************************************************************
  Copyright (c) 2020 Infinera
 ************************************************************************/
#ifndef RETRY_HELPER_INTF_H
#define RETRY_HELPER_INTF_H

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include "CrudService.h"

#define MAX_RETRY_COUNT 100
#define RETRY_INTERVAL 5

namespace DpAdapter
{
    class RetryObjWrapper
    {
        public:
            RetryObjWrapper():mRetryId(""), mRetryCount(0) {}
            RetryObjWrapper(std::string id, int maxRetryCnt = MAX_RETRY_COUNT):mRetryId(id), mRetryCount(0), mMaxRetries(maxRetryCnt) {}
            ~RetryObjWrapper() {}
            bool IsMaxRetryDone() { return (mRetryCount >= mMaxRetries? true:false); }
            void IncrRetryCount() { mRetryCount++; }
            int GetRetryCount() { return mRetryCount; }
            std::string GetId() { return mRetryId; }
            void SetMaxRetries(int retryCount) { mMaxRetries = retryCount; };
        private:
            int mMaxRetries;
            std::string mRetryId;
            int mRetryCount;
    };

    class RetryHelperIntf
    {
        public:
            RetryHelperIntf(uint32_t interval = RETRY_INTERVAL, int maxRetries = MAX_RETRY_COUNT);
            virtual ~RetryHelperIntf();

            /*
             * If there is a failure in the subscription - insert into the list
             * for re-subscription
             * @param : retry wrapper object
             */
            virtual void AddToReSubscriptionList(RetryObjWrapper& obj);
            /*
             * On the first successful response remove from resubscription list
             * @param : retry wrapper object
             * @param : Asynchronous callback object pointer
             */
            virtual void RemoveFromReSubscriptionList(const std::string& objId, GnmiClient::AsyncSubCallback*);
            /*
             * Subscribe function which resubscibes in case of subscription failure
             * Thread function, executes at the time interval provided by user
             */
            void ReSubscribeLoop();
            /*
             * Function must implemented by derived class to have resubscription request sent.
             */
            virtual void SubscribeObject(std::string objId) = 0;

            /*
             * Function must implemented by derived class to cancel subscription request sent.
             */
            virtual void UnSubscribeObject(std::string objId) = 0;
            virtual void CleanupOnUnSubscribe(const std::string& objId);

            GnmiClient::AsyncSubCallback* GetActiveSubCallbackObj(std::string& objId);

            //check if subscription is active for an id
            bool IsActiveSubscription( std::string subId );

            /*
            * Dump active subscriptions and retries
            */
            void DumpSubscriptionData(std::ostream& out);

            void SetConnState(bool isConnectUp);

            bool IsConnectionUp() { std::lock_guard<std::mutex> lock(mConnMutex); return mIsConnectionUp; }

        private:
            bool mIsConnectionUp;
            int mMaxRetries;
            uint32_t mInterval; //Retry interval in seconds
            std::map<std::string, RetryObjWrapper*> mRetryList;
            std::map<std::string, GnmiClient::AsyncSubCallback*> mActiveSubList; 
            std::thread mSubThread;
            std::mutex mRetListMutex;
            std::mutex mConnMutex;
    };


}//end namespace

#endif
