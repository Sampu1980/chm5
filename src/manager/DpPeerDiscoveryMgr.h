/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
//Keep this singleton 
//Call from carrier manager to create and set carrier id

#ifndef DP_PEER_DISCOVERY_MGR_H
#define DP_PEER_DISCOVERY_MGR_H

#include <memory>
#include <google/protobuf/message.h>
#include <SingletonService.h>
#include "DcoSecProcVlanAdapter.h"
#include "DcoSecProcIcdpAdapter.h"
#include "DpObjectMgr.h"
#include "DpProtoDefs.h"

namespace DataPlane
{

    class DpPeerDiscoveryMgr : public DpObjectMgr
    {
    public:
        DpPeerDiscoveryMgr():DpObjectMgr("PeerDisc", ((1 << DCC_ZYNQ) | (1 << DCC_NXP)) ) {};
        virtual ~DpPeerDiscoveryMgr() {};

        bool checkAndCreate(google::protobuf::Message* objMsg) { return false; }
        bool checkAndUpdate(google::protobuf::Message* objMsg) { return false; }
        bool checkAndDelete(google::protobuf::Message* objMsg) { return false; }
        bool checkOnResync (google::protobuf::Message* objMsg) { return false; }

        void start() {}
        void initialize();

        static bool CreateVlan(std::string carrier, DataPlane::ControlChannelVlanIdx, DpAdapter::ControlChannelVlanCommon&);
        static bool DeleteVlan(std::string carrier, DataPlane::ControlChannelVlanIdx, DpAdapter::ControlChannelVlanCommon&);

        static void CreatePcpVlan(Chm6PeerDiscVlanState& vlanState);
        static void DeletePcpVlan(Chm6PeerDiscVlanState& vlanState);

        //APIs to update ICDP state to redis
        //Get called in grpc subscribe response handler context
        static bool CreateIcdpState(Chm6IcdpState& state);
        static bool UpdateIcdpState(Chm6IcdpState& state);
        static bool DeleteIcdpState(Chm6IcdpState& state);

        void DcoSecProcConnectionUpdate(bool isConnectUp);

        virtual void connectionUpdate();
        /*
        * Notify carrier create to secproc vlan adapter
        * Subscription on vlan setting can start when the carrier creation is complete
        * @param: Carrier Id
        */
        void NotifyCarrierCreate(std::string carrierId);

        /*
        * Notify carrier delete to secproc vlan adapter
        * Subscription on vlan setting can stop when the carrier deleteion is complete
        * @param: Carrier Id
        */
        void NotifyCarrierDelete(std::string carrierId);

        /*
         * Send ICDP configuration to peer discovery manager from the card manager
         * @param: Icdp Node config object [in]
         */
        void CreateIcdpNodeConfig(const IcdpNodeInfo& node);

        /*
         * Send update on icdp node config - toggling enable-disable and other parameters
         * @param: Icdp Node config object [in]
         */
        void UpdateIcdpNodeConfig(const IcdpNodeInfo& node);

        /*
         * Send delete configuration to peer discovery manager from the card manager
         * @param: Icdp Node config object [in]
         */
        void DeleteIcdpNodeConfig(const IcdpNodeInfo& node);

        /*
        * Dump active vlan subscriptions, retries
        * @param: output stream [out]
        */
        void DumpVlanSubscriptions(ostream& out);

        /*
        * Dump active icdp subscriptions, retries
        * @param: output stream [out]
        */
        void DumpIcdpSubscriptions(ostream& out);

    private:
        std::unique_ptr<DpAdapter::DcoSecProcVlanAdapter> mpVlanAdapter;
        std::unique_ptr<DpAdapter::DcoSecProcIcdpAdapter> mpIcdpAdapter;

    };

    typedef SingletonService<DpPeerDiscoveryMgr> DpPeerDiscoveryMgrSingleton;

} // namespace DataPlane

#endif /* DP_PEER_DISC_MGR_H */
