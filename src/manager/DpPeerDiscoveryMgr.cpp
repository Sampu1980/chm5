#include "DpPeerDiscoveryMgr.h"
#include "InfnLogger.h"
#include "DataPlaneMgr.h"
#include "DpGccMgr.h"
#include <sys/time.h>

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;


namespace DataPlane
{

    bool DpPeerDiscoveryMgr::CreateVlan(std::string carrier, DataPlane::ControlChannelVlanIdx vlan, DpAdapter::ControlChannelVlanCommon& commonVlan)
    {
        bool cfgSuccess = false;
        INFN_LOG(SeverityLevel::info) << __func__ << "Create vlan called";
        //Calling DCO Gcc Adapter for configuration on DCO zynq
        DpAdapter::DcoGccControlAdapter* pGccAd = DpGccMgrSingleton::Instance()->getAdPtr();
        
        if(pGccAd)
        {
            cfgSuccess = pGccAd->createCcVlan(vlan, &(commonVlan));
        }
    
        return cfgSuccess;
    }



    bool DpPeerDiscoveryMgr::DeleteVlan(std::string carrier, DataPlane::ControlChannelVlanIdx vlan, DpAdapter::ControlChannelVlanCommon& commonVlan)
    {
        bool cfgSuccess = false;
        INFN_LOG(SeverityLevel::info) << __func__ << "Delete vlan called";
        //Calling DCO Gcc Adapter for configuration on DCO zynq
        DpAdapter::DcoGccControlAdapter* pGccAd = DpGccMgrSingleton::Instance()->getAdPtr();
        
        if(pGccAd)
        {
            cfgSuccess = pGccAd->deleteCcVlan(vlan);
        }
        return cfgSuccess;
    }

    void DpPeerDiscoveryMgr::CreatePcpVlan(Chm6PeerDiscVlanState& vlanState)
    {
        //Calling PCP MS - update vlan state object in redis for PCP to pick up

        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);

        vlanState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
	vlanState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);


        //@todo: Enable on grpc-adapter version change to latest
        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(vlanState);
    }

    void DpPeerDiscoveryMgr::DeletePcpVlan(Chm6PeerDiscVlanState& vlanState)
    {
        //Calling PCP MS - update vlan state object in redis for PCP to pick up
        std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

        vlanState.mutable_base_state()->mutable_mark_for_delete()->set_value(true);

        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);

        vlanState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
	vlanState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        vDelObjects.push_back(vlanState);

        //@todo: Enable on grpc-adapter version change to latest
        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);
    }

    void DpPeerDiscoveryMgr::initialize()
    {
        INFN_LOG(SeverityLevel::info) << __func__ << "Registering callbacks";
        mpVlanAdapter = std::make_unique<DpAdapter::DcoSecProcVlanAdapter>();

        if(mpVlanAdapter)
        {
            //Register callback function to get callback on vlan update
            mpVlanAdapter->RegisterVlanCreateCallback( &DpPeerDiscoveryMgr::CreateVlan );
            mpVlanAdapter->RegisterVlanDeleteCallback( &DpPeerDiscoveryMgr::DeleteVlan );
            mpVlanAdapter->RegisterPcpCreateCallback( &DpPeerDiscoveryMgr::CreatePcpVlan );
            mpVlanAdapter->RegisterPcpDeleteCallback( &DpPeerDiscoveryMgr::DeletePcpVlan );
        }
        
        mpIcdpAdapter = std::make_unique<DpAdapter::DcoSecProcIcdpAdapter>();
        if(mpIcdpAdapter)
        {
            //Register callback function to get callback on icdp updates
            mpIcdpAdapter->RegisterIcdpCreateCallback( &DpPeerDiscoveryMgr::CreateIcdpState );
            mpIcdpAdapter->RegisterIcdpUpdateCallback( &DpPeerDiscoveryMgr::UpdateIcdpState );
            mpIcdpAdapter->RegisterIcdpDeleteCallback( &DpPeerDiscoveryMgr::DeleteIcdpState );
        }

        mIsInitDone = true;
    }

    void DpPeerDiscoveryMgr::connectionUpdate()
    {
        INFN_LOG(SeverityLevel::info) << "Updating with new connection state isConnectionUp: " << mIsConnectionUp;
        mpVlanAdapter->SetConnectionState(mIsConnectionUp);
        mpIcdpAdapter->SetConnectionState(mIsConnectionUp);

    }

//@todo : Remove this
    void DpPeerDiscoveryMgr::DcoSecProcConnectionUpdate(bool isConnectUp)
    {
#if 0
        INFN_LOG(SeverityLevel::info) << __func__ << " New connection state : " << isConnectUp;
        mpVlanAdapter->SetConnectionState(isConnectUp);
#endif
    }

    void DpPeerDiscoveryMgr::NotifyCarrierCreate(std::string carrierId)
    {
        mpVlanAdapter->CarrierCreate(carrierId);
        mpIcdpAdapter->CarrierCreate(carrierId);
    }

    void DpPeerDiscoveryMgr::NotifyCarrierDelete(std::string carrierId)
    {
        mpIcdpAdapter->CarrierDelete(carrierId);
        mpVlanAdapter->CarrierDelete(carrierId);
    }

    void DpPeerDiscoveryMgr::CreateIcdpNodeConfig(const IcdpNodeInfo& node)
    {
        INFN_LOG(SeverityLevel::info) << __func__ ;
        mpIcdpAdapter->UpdateIcdpNode(node);
    }

    void DpPeerDiscoveryMgr::UpdateIcdpNodeConfig(const IcdpNodeInfo& node)
    {
        //@todo: Clarify if update is a difference or a complete object update
        INFN_LOG(SeverityLevel::info) << __func__ ;
        mpIcdpAdapter->UpdateIcdpNode(node);
    }

    void DpPeerDiscoveryMgr::DeleteIcdpNodeConfig(const IcdpNodeInfo& node)
    {
        INFN_LOG(SeverityLevel::info) << __func__ ;
        mpIcdpAdapter->DeleteIcdpNode();
    }

    bool DpPeerDiscoveryMgr::CreateIcdpState(Chm6IcdpState& state)
    {
        try{
            string stateDataStr;
            MessageToJsonString(state, &stateDataStr);
            INFN_LOG(SeverityLevel::info) << "ICDP state create : " << stateDataStr;

            // Retrieve timestamp
            struct timeval tv;
            gettimeofday(&tv, NULL);
            state.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
            state.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(state);
        }
        catch(const std::exception& e)
        {
            INFN_LOG(SeverityLevel::info) << "Exception ICDP state create : " << e.what();
            return false;
        }
        return true;
    }
    
    bool DpPeerDiscoveryMgr::UpdateIcdpState(Chm6IcdpState& state)
    {
        try
        {
            string stateDataStr;
            MessageToJsonString(state, &stateDataStr);
            INFN_LOG(SeverityLevel::debug) << "ICDP state update : " << stateDataStr;

            // Retrieve timestamp
            struct timeval tv;
            gettimeofday(&tv, NULL);
            state.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
            state.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

	    DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(state);
        }
        catch(const std::exception& e)
        {
            INFN_LOG(SeverityLevel::info) << "Exception ICDP state update : " << e.what();
            return false;
        }
        return true;
    }

    bool DpPeerDiscoveryMgr::DeleteIcdpState(Chm6IcdpState& state)
    {
        string stateDataStr;
        MessageToJsonString(state, &stateDataStr);
        INFN_LOG(SeverityLevel::info) << "ICDP state delete : " << stateDataStr;

        //@todo : Verify appropriate objects for deletion
        std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

        state.mutable_base_state()->mutable_mark_for_delete()->set_value(true);

	// Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);

        state.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
	state.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        vDelObjects.push_back(state);

        try
        {
            //@todo: Enable on grpc-adapter version change to latest
            DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects);
        }
        catch(const std::exception& e)
        {
            INFN_LOG(SeverityLevel::info) << "Exception ICDP state delete : " << e.what();
            return false;
        }
       return true;
    }

    void DpPeerDiscoveryMgr::DumpVlanSubscriptions(ostream& out)
    {
        if(mpVlanAdapter)
        {
            mpVlanAdapter->DumpVlanSubscriptions(out);
        }
    }

    void DpPeerDiscoveryMgr::DumpIcdpSubscriptions(ostream& out)
    {
        if(mpIcdpAdapter)
        {
            mpIcdpAdapter->DumpIcdpSubscriptions(out);
        }
    }


}//namespace DataPlane



