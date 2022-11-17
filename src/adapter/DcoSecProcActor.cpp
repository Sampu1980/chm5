//
//
// DcoSecProcEventListener receives Events from CHM6 & posts for doing GNMi set to NXP
//
// Copyright(c) Infinera 2021-2022

#include <sys/types.h>
#include <thread>
#include <ZException.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
#include <iostream>
#include <google/protobuf/util/json_util.h>
#include "DpProtoDefs.h"
#include "InfnLogger.h"
#include "DcoSecProcActor.h"
#include "DpCarrierMgr.h"


using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

DcoSecProcEventListener::DcoSecProcEventListener(uint32 evtQueueSize): InfnDeQueueActor(evtQueueSize)
{
}

DcoSecProcEventListener::~DcoSecProcEventListener()
{
}


bool DcoSecProcEventListener::IsRunning() const
{
    return true;
}


void DcoSecProcEventListener::ImplementHandleMsg( DcoSecProcEvent *pDcoSecProcEvent )
{
    if(NULL == pDcoSecProcEvent)
    {
        INFN_LOG(SeverityLevel::error) << " pDcoSecProcEvent is NULL, this shouldn't have happened!!";
        return;
    }

    INFN_LOG(SeverityLevel::debug) << " Event = " << pDcoSecProcEvent->GetEventType() << " AID = " << pDcoSecProcEvent->GetAid();
    switch(pDcoSecProcEvent->GetEventType())
    {
        case DCO_SECPROC_OFEC_STATUS_CREATE:
            {
                INFN_LOG(SeverityLevel::debug) << " DCO_SECPROC_OFEC_STATUS_CREATE event, send it";
                grpc::StatusCode gnmierror = DpCarrierMgrSingleton::Instance()->GetmspSecProcCarrierCfgHdlr()->onCreate(pDcoSecProcEvent->GetAid(), pDcoSecProcEvent->GetEventValue());
                if(grpc::StatusCode::OK != gnmierror)
                {
                    INFN_LOG(SeverityLevel::error) << " GNMI set failed error code = " << gnmierror;
                    DpCarrierMgrSingleton::Instance()->GetMspStateCollector()->SetForceCarrFaultToNxp(pDcoSecProcEvent->GetAid());
                }
            }
            break;
        case DCO_SECPROC_OFEC_STATUS_UPDATE:
            {
                INFN_LOG(SeverityLevel::debug) << " DCO_SECPROC_OFEC_STATUS_UPDATE event, send it";
                grpc::StatusCode gnmierror = DpCarrierMgrSingleton::Instance()->GetmspSecProcCarrierCfgHdlr()->onModify(pDcoSecProcEvent->GetAid(), pDcoSecProcEvent->GetEventValue());
                if(grpc::StatusCode::OK != gnmierror)
                {
                    INFN_LOG(SeverityLevel::error) << " GNMI set failed error code = " << gnmierror;
                    DpCarrierMgrSingleton::Instance()->GetMspStateCollector()->SetForceCarrFaultToNxp(pDcoSecProcEvent->GetAid());
                }
            }
            break;
        case DCO_SECPROC_OFEC_STATUS_DELETE:
            {
                INFN_LOG(SeverityLevel::debug) << " DCO_SECPROC_OFEC_STATUS_DELETE event, send it";
                grpc::StatusCode gnmierror = DpCarrierMgrSingleton::Instance()->GetmspSecProcCarrierCfgHdlr()->onDelete(pDcoSecProcEvent->GetAid(), pDcoSecProcEvent->GetEventValue());
                //TODO: If Delete fails there is no way this is again tried, should we do anything about this?
                if(grpc::StatusCode::OK != gnmierror)
                {
                    INFN_LOG(SeverityLevel::error) << " GNMI set failed error code = " << gnmierror;
                    DpCarrierMgrSingleton::Instance()->GetMspStateCollector()->SetForceCarrFaultToNxp(pDcoSecProcEvent->GetAid());
                }
            }
            break;

        case DCO_SECPROC_RX_STATUS:
            {
                INFN_LOG(SeverityLevel::debug) << " DCO_SECPROC_RX_STATUS event, send it";
                grpc::StatusCode gnmierror = DpCarrierMgrSingleton::Instance()->GetmspSecProcCarrierCfgHdlr()->onModifyCarrRx(pDcoSecProcEvent->GetAid(), pDcoSecProcEvent->GetEventValue());
                if(grpc::StatusCode::OK != gnmierror)
                {
                    INFN_LOG(SeverityLevel::error) << " GNMI set failed error code = " << gnmierror;
                    DpCarrierMgrSingleton::Instance()->GetMspStateCollector()->SetForceCarrStateToNxp(pDcoSecProcEvent->GetAid());
                }
            }
            break;
        default:
            {
                INFN_LOG(SeverityLevel::error) << "'Error case, unknown event drop it event = " << pDcoSecProcEvent->GetEventType();
            }
    }

    INFN_LOG(SeverityLevel::debug) << "pid = " << std::this_thread::get_id() << " outside while";
}

ZStatus DcoSecProcEventListener::PostEvent(DcoSecProcEvent *pDcoSecProcEvent)
{
    ZStatus status = ZFAIL;
    INFN_LOG(SeverityLevel::debug) << "Posting event = " <<  pDcoSecProcEvent->GetEventType();
    status = SubmitMsg(pDcoSecProcEvent);

    return status;
}

void DcoSecProcEventListener::Dump( ostringstream & os ) const
{
    os << "--------------------------------------------------" << endl
        << " DcoSecProcEventListener:" << endl
        << "   Name                : TODO" <<  endl
        << "   Events (pending)    : ";
}


