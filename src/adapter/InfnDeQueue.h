#ifndef InfnDeQueue_h
#define InfnDeQueue_h

#include <iostream>
#include <boost/thread/mutex.hpp>
#include <deque>
#include <ZSys.h>
#include "DcoSecProcEvent.h"
#include "InfnLogger.h"

using namespace std;

//template <typename T>
class InfnDeQueue
{

public:
    InfnDeQueue(uint32 evtQueueSize) : queue()
    {
        mMaxSize = evtQueueSize;
    };

    ~InfnDeQueue(){

    };

    /* Special Enqueue */
    /*
         * First check if similar type of event for same carrier is present or not. 
         * If present delete the old event & add the new one at end of Queue 
         */
    ZStatus EnQueue(DcoSecProcEvent *pEvent)
    {
        ZStatus status = ZOK;

        if (NULL == pEvent)
        {
            //TODO: Should we handle this differently?
            INFN_LOG(SeverityLevel::error) << " Tried to PUSH a NULL structure on queue, can't push return error";
            status = ZFAIL;
        }
        else
        {
            mtx.lock();

            INFN_LOG(SeverityLevel::debug) << " Current size = " << queue.size() << " max size = " << mMaxSize;
            if (queue.size() >= mMaxSize)
            {
                INFN_LOG(SeverityLevel::error) << " Max size of queue reached, drop event, max size  =  " << mMaxSize << " Current size  = " << queue.size();
                INFN_LOG(SeverityLevel::error) << " Dropping event id = " << pEvent->GetEventType() << " Aid = "<< pEvent->GetAid();
                delete pEvent;
                INFN_LOG(SeverityLevel::debug) << "freed memory of event";
                status = ZFAIL;
            }
            
            else
            {
                DcoSecProcEventType_t eventType = pEvent->GetEventType();
                size_t size = queue.size();
                std::string aid = pEvent->GetAid();
                INFN_LOG(SeverityLevel::debug) << "Pushing data on stack, old size  = " << queue.size() << " Eventtype = " << eventType;
                if ((eventType == DCO_SECPROC_INIT) || (eventType == DCO_SECPROC_MAX))
                {
                   INFN_LOG(SeverityLevel::debug) << "No need to push event to queue EventType = " << eventType;
                    delete pEvent;
                    mtx.unlock();
                   return status;
                }

                //First check if this event-type already exists, if it exists then delete the old event in queue & add the new event at the end of queue.
                for (unsigned int i = 0; i < size;)
                {
                    DcoSecProcEventType_t queueEventType = queue[i]->GetEventType();
                    std::string queueAid = queue[i]->GetAid();
                    INFN_LOG(SeverityLevel::debug) << " i = " << i << " queue event = " << queueEventType << " queueaid = " << queueAid;
                    INFN_LOG(SeverityLevel::debug) << " event = " << eventType << " aid = " << aid;
                    switch (queueEventType)
                    {
                        case DCO_SECPROC_OFEC_STATUS_CREATE:
                        case DCO_SECPROC_OFEC_STATUS_UPDATE:
                        case DCO_SECPROC_OFEC_STATUS_DELETE:
                        {
                            INFN_LOG(SeverityLevel::debug) << "inside OFEC event";
                            if ((eventType == DCO_SECPROC_OFEC_STATUS_CREATE) || (eventType == DCO_SECPROC_OFEC_STATUS_UPDATE) || (eventType == DCO_SECPROC_OFEC_STATUS_DELETE))
                            {
                            INFN_LOG(SeverityLevel::debug) << "inside OFEC event queue";
                                if (aid.compare(queueAid) == 0)
                                {
                                    INFN_LOG(SeverityLevel::debug) << "inside OFEC event, aid match found";
                                    INFN_LOG(SeverityLevel::debug) << "old size = " << queue.size();
                                    DcoSecProcEvent *ptemp = queue[i];
                                    queue.erase(queue.begin() + i);
                                 INFN_LOG(SeverityLevel::debug) << "new size = " << queue.size();
                                   delete ptemp;
                                   INFN_LOG(SeverityLevel::debug) << "freed memory of event";
                                size = queue.size();
                                   continue;
                                }
                            }
                        }
                        break;
                        
                        case DCO_SECPROC_RX_STATUS:
                        {
                            INFN_LOG(SeverityLevel::debug) << "inside Rx Status event";
                            if (eventType == DCO_SECPROC_RX_STATUS)
                            {
                                INFN_LOG(SeverityLevel::debug) << "inside Rx event queue, replace if aid matches";
                                if (aid.compare(queueAid) == 0)
                                {
                                    INFN_LOG(SeverityLevel::debug) << "inside Rx event, aid match found";
                                    INFN_LOG(SeverityLevel::debug) << "old size = " << queue.size();
                                    DcoSecProcEvent *ptemp = queue[i];
                                    queue.erase(queue.begin() + i);
                                    INFN_LOG(SeverityLevel::debug) << "new size = " << queue.size();
                                    delete ptemp;
                                    INFN_LOG(SeverityLevel::debug) << "freed memory of event";
                                    size = queue.size();
                                    continue;
                                }
                            }
                        }
                        break;

                        default:
                            INFN_LOG(SeverityLevel::error) << "Error case, unknown event = " << queueEventType;
                            break;
                    }
                    i++;
                }
                INFN_LOG(SeverityLevel::debug) << "Done Pushing data on stack, new size  = " << queue.size();
                queue.push_back(pEvent);
                INFN_LOG(SeverityLevel::debug) << "Done Pushing data on stack, new size  = " << queue.size();
                status = ZOK;
            }
            mtx.unlock();
        }
    
        return status;
    };

    DcoSecProcEvent *DeQueue()
    {
        DcoSecProcEvent *pEvent = NULL;
        mtx.lock();
        INFN_LOG(SeverityLevel::debug) << __FUNCTION__;
        if (true == queue.empty())
        {
            INFN_LOG(SeverityLevel::debug) << __func__ << "Queue is empty, cannot pop";
        }
        else
        {
            INFN_LOG(SeverityLevel::debug) << "popping queue before size = " << queue.size();
            pEvent = queue.front();
            queue.pop_front();
            INFN_LOG(SeverityLevel::debug) << " post popping queue after size = " << queue.size();
        }
        mtx.unlock();
        return pEvent;
    };

    std::deque<DcoSecProcEvent *> queue;
    boost::mutex mtx;
    size_t mMaxSize;
};

#endif
