#include <iostream>
#include <string>
#include "RetryHelperIntf.h"
#include "InfnLogger.h"
#include "dp_common.h"

#define DEFAULT_INTERVAL 5

namespace DpAdapter {


  RetryHelperIntf::RetryHelperIntf(uint32_t interval, int maxRetries):mInterval(interval),mMaxRetries(maxRetries)        {
        mIsConnectionUp = true;
            mSubThread = std::thread(&RetryHelperIntf::ReSubscribeLoop, this);
   }

    RetryHelperIntf::~RetryHelperIntf()
    {
        for(const auto& kv : mRetryList)
        {
            if(kv.second)
                delete kv.second;
        }
        mRetryList.clear();
        //Active sub list holds the object id as well as pointer to callback context object
        //Callback context object gets deleted by grpc_adpater by design
        //Will not be calling explicit delete on erase/clear of the callback context
        mActiveSubList.clear();
        mSubThread.join();
    }

    void RetryHelperIntf::AddToReSubscriptionList(RetryObjWrapper& obj)
    {
        std::lock_guard<std::mutex> lock(mRetListMutex);
        if( (mRetryList.find(obj.GetId()) == mRetryList.end()) && (true == IsConnectionUp()) )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " adding to retry " << obj.GetId();
            obj.SetMaxRetries(mMaxRetries);
            //New object Id being added    
            mRetryList.insert(std::pair<std::string, RetryObjWrapper*>( obj.GetId(), new RetryObjWrapper(obj))); 
        }
        auto iter = mActiveSubList.find(obj.GetId());
        if(iter != mActiveSubList.end() )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " remove from active " << obj.GetId();
            mActiveSubList.erase(iter);
        }
    }

    void RetryHelperIntf::CleanupOnUnSubscribe(const std::string& objId)
    {
        std::lock_guard<std::mutex> lock(mRetListMutex);
        auto iter =  mRetryList.find(objId);
        if( iter != mRetryList.end() )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " removing from retry " << objId;
            //objId Id being removed
            delete (iter->second);
            mRetryList.erase(iter); 
        }


        auto actIter = mActiveSubList.find(objId);
        if( actIter != mActiveSubList.end() )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " keeping in active sublist " << objId;
        }
    }

    void RetryHelperIntf::RemoveFromReSubscriptionList(const std::string& objId, GnmiClient::AsyncSubCallback* pCb)
    {
        std::lock_guard<std::mutex> lock(mRetListMutex);
        auto iter =  mRetryList.find(objId);
        if( iter != mRetryList.end() )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " removing " << objId;
            //objId Id being removed
            delete (iter->second);
            mRetryList.erase(iter); 

            //On successful response, add an object to active subscription list
            mActiveSubList.insert(std::pair<std::string, GnmiClient::AsyncSubCallback*>( objId, pCb)); 
        }
        auto actIter = mActiveSubList.find(objId);
        if( actIter == mActiveSubList.end() ) 
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Adding to active subscription " << objId;
            mActiveSubList.insert(std::pair<std::string, GnmiClient::AsyncSubCallback*>( objId, pCb));
        }
    }

    GnmiClient::AsyncSubCallback* RetryHelperIntf::GetActiveSubCallbackObj(std::string& objId)
    {
        auto iter = mActiveSubList.find(objId);
        if(iter != mActiveSubList.end() )
        {
            return (iter->second);
        }
        return NULL;
    }

    void RetryHelperIntf::ReSubscribeLoop()
    {
        std::thread::id this_id = std::this_thread::get_id();
        INFN_LOG(SeverityLevel::info)  << __func__ << " sub thread " << this_id;

        while(true)
        {
            {  
                //INFN_LOG(SeverityLevel::info)  << __func__ << " Run " << mRetryList.size();

                std::set<std::string> tmpRetryList;
                {
                    std::lock_guard<std::mutex> lock(mRetListMutex);
                    //INFN_LOG(SeverityLevel::info)  << __func__ << " Run 1 after ";
                    for(const auto& kv : mRetryList)
                    {
                        RetryObjWrapper* pObj = kv.second;
                        std::string objId = kv.first;
                        //INFN_LOG(SeverityLevel::info)  << __func__ << " carrier "  <<objId;
                        if( pObj && !pObj->IsMaxRetryDone() )
                        {
                            tmpRetryList.insert(objId);
                            INFN_LOG(SeverityLevel::info) << __func__ << " Re sub for " << objId;
                            pObj->IncrRetryCount();
                        }
                        else
                        {
                            //INFN_LOG(SeverityLevel::info)  << " Max retries done for " << objId;

                            //Raise exception?
                            //break;
                        }
                    }
                }//Release mutex mRetListMutex

                for(const auto& id : tmpRetryList)
                {
                    SubscribeObject(id);
                }

            }
            //INFN_LOG(SeverityLevel::info)  << __func__ << " Sleep ";
            std::this_thread::sleep_for(std::chrono::seconds(mInterval));
        }
    }


    void RetryHelperIntf::DumpSubscriptionData(std::ostream& out)
    {
        {
            std::lock_guard<std::mutex> lock(mRetListMutex);
            out << "\nRetry list: ";
            for(const auto& kv : mRetryList)
            {
                RetryObjWrapper* pObj = kv.second;
                std::string objId = kv.first;
                if( pObj )
                {
                    out << "\n Id: " << pObj->GetId() << " count: " << pObj->GetRetryCount();
                }
            }

            out << "\nActive subscription list: ";
            for(const auto& kv : mActiveSubList)
            {
                std::string objId = kv.first;
                out << "\n Id: " << objId << " cbId: " << kv.second << std::endl;
            }

            out << "\n\n";
        }

    }

    bool RetryHelperIntf::IsActiveSubscription( std::string objId )
    {
        bool result = true;
        auto actIter = mActiveSubList.find(objId);
        if( actIter == mActiveSubList.end() )
        {
            result = false;
        }
        return result;
    }

    void RetryHelperIntf::SetConnState(bool isConnectUp) 
    { 
        INFN_LOG(SeverityLevel::info)  << __func__ << " update conn ";
        std::lock_guard<std::mutex> lock(mConnMutex); 
        mIsConnectionUp = isConnectUp; 
        INFN_LOG(SeverityLevel::info)  << __func__ << " updated state ";
    }



}//namespace
