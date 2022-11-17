/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_GIGE_CLIENT_ADAPTER_H
#define DCO_GIGE_CLIENT_ADAPTER_H

#include "dcoyang/infinera_dco_client_gige/infinera_dco_client_gige.pb.h"
#include "dcoyang/infinera_dco_client_gige_fault/infinera_dco_client_gige_fault.pb.h"
#include "GigeClientAdapter.h"
#include "CrudService.h"
#include <SimCrudService.h>
#include "DpPm.h"
#include "AtlanticSerdesTable.h"
#include "types.h"

using namespace DataPlane;

using gigeClient = dcoyang::infinera_dco_client_gige::ClientsGige;
using gigeClientConfig = dcoyang::infinera_dco_client_gige::ClientsGige_ClientGige_Config;
using gigeClientState  = dcoyang::infinera_dco_client_gige::ClientsGige_ClientGige_State;

namespace DpAdapter {

typedef std::map<std::string, std::shared_ptr<GigeCommon>> TypeMapGigeConfig;

typedef struct
{
    int                 fpPort;     // Front pannel port
    DataPlane::PortRate pRate;
    int                 clientLane;
}clientPortEntry;


// GigeClient Adapter Base class
class DcoGigeClientAdapter : public GigeClientAdapter
{
public:
    DcoGigeClientAdapter();
    virtual ~DcoGigeClientAdapter();

    virtual bool createGigeClient(string aid, GigeCommon *cfg);
    virtual bool createGigeClientXpress(string aid, DataPlane::PortRate portRate);
    virtual bool deleteGigeClient(string aid);
    virtual bool initializeGigeClient();

    virtual bool setGigeMtu(string aid, int mtu);
    virtual bool setGigeFecEnable(string aid, bool enable);
    virtual bool setGigeLldpDrop(string aid, DataPlane::Direction dir, bool enable);
    virtual bool setGigeLldpSnoop(string aid, DataPlane::Direction dir, bool enable);
    virtual bool setGigeLoopBack(string aid, DataPlane::LoopBackType mode);
    virtual bool setGigeAutoMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode);
    virtual bool setGigeForceMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode);
    virtual bool setGigeTestSignalGen(string aid, Direction dir, bool mode, DataPlane::MaintenanceSignal cacheOverrideForceMs);
    virtual bool setGigeTestSignalMon(string aid, bool mode);
    virtual bool setGigeFwdTdaTrigger(string aid, bool enable);
    virtual bool setGigeFecDegSerEn(string aid, bool enable);
    virtual bool setGigeFecDegSerActThresh(string aid, double actThresh);
    virtual bool setGigeFecDegSerDeactThresh(string aid, double deactThresh);
    virtual bool setGigeFecDegSerMonPeriod(string aid, uint32_t monPer);

    virtual bool getGigeConfig(string aid, GigeCommon *cfg);
    virtual bool getGigeStatus(string aid, GigeStatus *stat);
    virtual bool getGigeMaxMtu(string aid, int *maxMtu);
    virtual bool getGigeLinkStatus(string aid, ClientStatus *linkStat);
    virtual bool getGigeLldpPkt(string aid, DataPlane::Direction dir, uint8_t *ptk);
    virtual bool getGigeFault(string aid, uint64_t *fault);
    virtual bool getGigeFault(string aid, GigeFault *faults, bool force = false);
    virtual bool getGigePm(string aid);
    virtual void subGigePm();
    virtual void subGigeFault();
    virtual void subGigeOpStatus();
    virtual bool getLldpPortIdFromFlowId(uint32_t flowId,  uint32_t *portId);
    virtual void subscribeStreams();
    virtual void clearFltNotifyFlag();
    virtual bool setTdaHoldOff(string aid, bool enable, bool updateDco, DataPlane::MaintenanceSignal mode);

    virtual bool gigeConfigInfo(ostream& out, string aid);
    virtual bool gigeStatusInfo(ostream& out, string aid);
    virtual bool gigePmInfo(std::ostream& out, int portId);
    virtual bool gigePmAccumInfo(std::ostream& out, int portId);
    virtual bool gigePmAccumClear(std::ostream& out, int portId);
    virtual bool gigePmAccumEnable(std::ostream& out, int portId, bool enable);
    virtual bool gigeFaultInfo(ostream& out, string aid);
    virtual void gigePmTimeInfo(ostream& out);
    virtual bool gigeConfigInfo(std::ostream& out);

    void dumpConfigInfo(std::ostream& out, const GigeCommon& cfg);
    void dumpStatusInfo(std::ostream& out, const GigeStatus& stat);

    void dumpAll(ostream& out);

    void translateStatus(const gigeClientState& gigeState, GigeStatus& stat);

    bool isSimModeEn() { return mIsGnmiSimModeEn; }
    void setSimModeEn(bool enable);
    void setSimPmData();

    int aidToPortId (string aid);

    bool updateNotifyCache();

    // Serdes
    void updateSerdesCfgTbl(string aid, DataPlane::PortRate portRate, const vector<tuning::AtlBcmSerdesTxCfg> & atlBcmTbl);

    bool getGigeConfig(TypeMapGigeConfig& mapCfg);
    void translateConfig(const gigeClientConfig& dcoCfg, GigeCommon& adCfg);


    static DataPlane::DpMsClientGigePms mGigePms;
    static AdapterCacheFault mGigeFault[MAX_CLIENTS];
    static AdapterCacheOpStatus mOpStatus[MAX_CLIENTS];

    static high_resolution_clock::time_point mStart;
    static high_resolution_clock::time_point mEnd;
    static int64_t mStartSpan;
    static int64_t mEndSpan;
    static int64_t mDelta;
    static deque<int64_t> mStartSpanArray;
    static deque<int64_t> mEndSpanArray;
    static deque<int64_t>  mDeltaArray;
    static bool mGigePmSubEnable;
    static bool mGigeFltSubEnable;
    static bool mGigeOpStatusSubEnable;




private:
    bool setKey (gigeClient *gige, int id);

    std::string toString(const DataPlane::Direction) const;

    const clientPortEntry *getFpPortEntry( int portId, DataPlane::PortRate rate);
    bool getGigeObj( gigeClient *gige, int portId, int *idx);
    bool getGigeFaultCapa();
    unsigned int getDcoLanesPerPort(DataPlane::PortRate portRate);
    std::string printMaintenanceSignal(MaintenanceSignal ms);
    string printLoopBackType(LoopBackType lb);
    string printPortRate(PortRate rate);

    static const clientPortEntry mChm6_PortMap[];
    PortRate mPortRate[MAX_CLIENTS+1];
    bool mTdaHoldOff[MAX_CLIENTS+1];

    unsigned int getAtlBcmSerdesTxOffset(int portId, DataPlane::PortRate portRate, unsigned int lane);


    std::recursive_mutex       mCrudPtrMtx;
    bool                       mIsGnmiSimModeEn;
    std::shared_ptr<::DpSim::SimSbGnmiClient>   mspSimCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspSbCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspCrud;
    std::vector<FaultCapability> mFaultCapa;
    std::thread			mSimGigePmThread;

    static const unsigned int DCO_FEC_DEG_TH_PRECISION = 12;
};

}
#endif // DCO_GIGE_CLIENT_ADAPTER_H
