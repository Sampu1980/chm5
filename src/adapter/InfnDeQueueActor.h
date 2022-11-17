
#ifndef InfnDeQueueActor_h
#define InfnDeQueueActor_h

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <InfnDeQueue.h>
#include <iostream>
#include <ZSys.h>
#include <mutex>
#include <condition_variable>
#include "DcoSecProcEvent.h"
#include "InfnLogger.h"

using namespace std;

//template <typename T>
class InfnDeQueueActor
{
    public:
        InfnDeQueueActor(uint32 size):mQueue(size)
    {

    };

        ~InfnDeQueueActor(){};
        virtual void ImplementHandleMsg(DcoSecProcEvent *pMsg) = 0;
        virtual void Start()
        {
            boost::thread(boost::bind(&InfnDeQueueActor::Run,this)).detach();
        };

        ZStatus SubmitMsg(DcoSecProcEvent *pMsg)
        {
            INFN_LOG(SeverityLevel::debug) << "inside Submit Message";
            {
                std::unique_lock<std::recursive_mutex>  lock(mMutex);
                ZStatus isSuccess = mQueue.EnQueue(pMsg);
                if(isSuccess == ZFAIL)
                {
                    INFN_LOG(SeverityLevel::error) << "EnQueue Failed";
                    return ZFAIL;
                }
                else
                {
                    INFN_LOG(SeverityLevel::debug) << " Enqueue passed";
                }
            }
            mCond_empty.notify_one();
            return ZOK;
        };

    private:

        InfnDeQueue mQueue;
        std::recursive_mutex mMutex;
        std::condition_variable_any mCond_empty;
        void Run(void)
        {
            while(true)
            {
                DcoSecProcEvent *pMsg;
                {
                    std::unique_lock<std::recursive_mutex> lock(mMutex);
                    mCond_empty.wait(lock, [&]{return !mQueue.queue.empty();});
                    pMsg = mQueue.DeQueue();
                    if(pMsg == NULL)
                    {
                        INFN_LOG(SeverityLevel::error) << "DeQueue Failed";
                        continue;
                    }
                    else
                    {
                        INFN_LOG(SeverityLevel::debug) << "DeQueue Passed";
                    }
                }			
                ImplementHandleMsg(pMsg);

                INFN_LOG(SeverityLevel::debug) << "Free the Memory";
                if(pMsg)
                {
                    delete pMsg;
                }

            }
        };

};

#endif
