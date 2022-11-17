/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_XCON_ADAPTER_H
#define DCO_XCON_ADAPTER_H

#include <mutex>
#include <SimCrudService.h>

#include "dcoyang/infinera_dco_xcon/infinera_dco_xcon.pb.h"
#include "XconAdapter.h"
#include "CrudService.h"
#include "types.h"

using namespace GnmiClient;
using namespace DataPlane;
using namespace dcoyang::infinera_dco_xcon;

using Xcon = dcoyang::infinera_dco_xcon::Xcons;
using XconConfig = dcoyang::infinera_dco_xcon::Xcons_Xcon_Config;
using XconState = dcoyang::infinera_dco_xcon::Xcons_Xcon_State;

namespace DpAdapter {

typedef std::map<std::string, std::shared_ptr<XconCommon>> TypeMapXconConfig;

class DcoXconAdapter: public XconAdapter
{
public:
    DcoXconAdapter();
    virtual ~DcoXconAdapter();

    virtual bool createXcon(string aid, XconCommon *cfg);
    virtual bool createXpressXcon(string aid, XconCommon *cfg, uint32_t *srcLoOduId, uint32_t *dstLoOduId);
    virtual bool deleteXcon(string aid);
    virtual bool deleteXpressXcon(string aid, string *srcClientId, string *dstClientId, XconPayLoadType *payload, uint32_t *srcLoOduId, uint32_t *dstLoOduId);
    virtual bool initializeXcon();

    virtual bool getXconConfig(string aid, XconCommon *cfg);
    virtual bool getXconStatus(string aid, XconStatus *stat);

    virtual bool xconConfigInfo(ostream& out, string aid);
    virtual bool xconStatusInfo(ostream& out, string aid);
    virtual bool xconConfigInfo(std::ostream& out);
    virtual void subXconOpStatus();
    virtual void subscribeStreams();

    void dumpConfigInfo(std::ostream& out, const XconCommon& cfg);

    bool getXconConfig(TypeMapXconConfig& mapCfg);
    void translateConfig(const XconConfig& dcoCfg, XconCommon& adCfg);

    bool isSimModeEn() { return mIsGnmiSimModeEn; }
    void setSimModeEn(bool enable);

    void dumpAll(std::ostream& out);

    void dumpStatusInfo(std::ostream& out, const XconStatus& stat);

    void translateStatus(const XconState& dcoState, XconStatus& stat);

    static AdapterCacheOpStatus mOpStatus[MAX_XCON_ID];
    static bool mXconOpStatusSubEnable;

private:
    int aidToXconId (string aid);
    bool setKey (Xcon *xcon, int id);
    bool getXconObj( Xcon *xcon, int xconId, int *idx);
    bool getClientId (string aid, uint32_t *clientId);
    bool getOtnId (string aid, uint32_t *hoOtuId, uint32_t *loOduId);
    bool createUniDirXpressXcon(XconCommon *cfg, uint32_t xconId, uint32_t clientId, uint32_t loOduId, uint32_t otuCniId, Xcons_Xcon_Config_Direction dir);
    string encodeXpressXconAid(string xconAid, string srcClientAid, string dstClientAid);

    std::recursive_mutex  mMutex;

    bool                       mIsGnmiSimModeEn;
    std::shared_ptr<::DpSim::SimSbGnmiClient>   mspSimCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspSbCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspCrud;
};

}
#endif // DCO_XCON_ADAPTER_H
