#ifndef DP_KEYMGMT_MGR_H
#define DP_KEYMGMT_MGR_H

#include "DpObjectMgr.h"
#include "DpProtoDefs.h"
#include <SingletonService.h>
#include "DcoSecProcKeyMgmtAdapter.h"
#include "DcoZynqKeyMgmtAdapter.h"
#include "HostKeyMgmtAdapter.h"

namespace DataPlane
{

class DpKeyMgmtMgr : public DpObjectMgr
{
public:

    //DpKeyMgmtMgr(DcoCpuConnectionsType connId);
    DpKeyMgmtMgr();
    virtual ~DpKeyMgmtMgr();

    virtual void initialize();

    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    //APIs to update key managememnt state to redis
    static bool CreateKeyMgmtState(Chm6KeyMgmtState& keyState);
    static bool UpdateKeyMgmtState(Chm6KeyMgmtState& keyState);
    static bool DeleteKeyMgmtState(Chm6KeyMgmtState& keyState);

protected:

    virtual void start();
    // virtual void reSubscribe();
    virtual void connectionUpdate();
    // virtual void DcoSecProcConnectionUpdate(bool isConnectUp);

private:

    //void CreateOrUpdateForZynq(chm6_common::Chm6DcoKeyMgmtConfig *pKeyMgmtCfg);
    //bool CreateISKRing(chm6_common::Chm6DcoKeyMgmtConfig *pKeyMgmtCfg);

    std::unique_ptr<DpAdapter::DcoSecProcKeyMgmtAdapter> mpDcoSecProcKeyMgmtAdapter;
    std::unique_ptr<DpAdapter::HostKeyMgmtAdapter> mpHostKeyMgmtAdapter;
    std::unique_ptr<DpAdapter::DcoZynqKeyMgmtAdapter> mpDcoZynqKeyMgmtAdapter;

};

typedef SingletonService<DpKeyMgmtMgr> DpKeyMgmtMgrSingleton;

} // namespace DataPlane

#endif /* DP_KEYMGMT_MGR_H */
