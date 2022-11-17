/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_OTU_ADAPTER_H
#define DCO_OTU_ADAPTER_H

#include <mutex>
#include <SimCrudService.h>
#include "dcoyang/infinera_dco_otu/infinera_dco_otu.pb.h"
#include "dcoyang/infinera_dco_otu_fault/infinera_dco_otu_fault.pb.h"
#include "OtuAdapter.h"
#include "CrudService.h"
#include "DpPm.h"
#include "AtlanticSerdesTable.h"
#include "types.h"

using namespace GnmiClient;
using namespace DataPlane;

using Otu = dcoyang::infinera_dco_otu::Otus;
using OtuConfig = dcoyang::infinera_dco_otu::Otus_Otu_Config;
using OtuStat = dcoyang::infinera_dco_otu::Otus_Otu_State;

namespace DpAdapter {

typedef std::map<std::string, std::shared_ptr<OtuCommon>> TypeMapOtuConfig;

class DcoOtuAdapter: public OtuAdapter
{
public:
    DcoOtuAdapter();
    virtual ~DcoOtuAdapter();

    virtual bool createOtuClientXpress(string aid);
    virtual bool createOtu(string aid, OtuCommon *cfg);
    virtual bool deleteOtu(string aid);
    virtual bool initializeOtu();

    virtual bool setOtuLoopBack(string aid, DataPlane::LoopBackType mode);
    virtual bool setOtuTti(string aid, uint8_t *tti);
    virtual bool setOtuAutoMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode);
    virtual bool setOtuForceMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode);
    virtual bool setOtuPrbsMon(string aid, DataPlane::Direction dir, PrbsMode mode);
    virtual bool setOtuPrbsGen(string aid, DataPlane::Direction dir, PrbsMode mode);
    virtual bool setOtuFecGenEnable(string aid, bool enable);
    virtual bool setOtuFecCorrEnable(string aid, bool enable);
    virtual bool setOtuFaultStatus(string aid, DataPlane::Direction dir, uint64_t injectFaultBitmap);
    virtual bool setOtuTimBdi(string aid, bool bInject);

    virtual bool getOtuConfig(string aid, OtuCommon *cfg);
    virtual bool getOtuStatus(string aid, OtuStatus *stat);
    virtual bool getOtuFaultRx(string aid, uint64_t *fault);
    virtual bool getOtuFaultTx(string aid, uint64_t *fault);
    virtual bool getOtuFault(string aid, OtuFault *faults, bool force = false);
    virtual bool getOtuPm(string aid);
    virtual uint32_t getOtuFrameRate(string aid);
    virtual uint64_t getOtuPmErrorBlocks(string aid);
    virtual bool getOtuFecCorrEnable(string aid);

    virtual void subOtuPm();
    virtual void subOtuFault();
    virtual void subOtuState();
    virtual void subOtuOpStatus();
    virtual void subscribeStreams();
    virtual void clearNotifyFlag();

    virtual bool otuConfigInfo(ostream& out, string aid);
    virtual bool otuStatusInfo(ostream& out, string aid);
    virtual bool otuPmInfo(std::ostream& out, int otuId);
    virtual bool otuPmAccumInfo(std::ostream& out, int otuId);
    virtual bool otuPmAccumClear(std::ostream& out, int otuId);
    virtual bool otuPmAccumEnable(std::ostream& out, int otuId, bool enable);
    virtual bool otuFaultInfo(ostream& out, string aid);
    virtual void otuNotifyState(std::ostream& out, int otuId);
    virtual void otuPmTimeInfo(ostream& out);
    virtual bool otuConfigInfo(std::ostream& out);
    void dumpConfigInfo(std::ostream& out, const OtuCommon& cfg);
    void dumpStatusInfo(std::ostream& out, const OtuStatus& cfg);

    void dumpAll(std::ostream& out);

    void translateStatus(const OtuStat& dcoStat, OtuStatus& adCfg, bool updateCache = false);

    bool isSimModeEn() { return mIsGnmiSimModeEn; }
    void setSimModeEn(bool enable);
    void setSimPmData();
    uint8_t* getOtuRxTti(const string aid);

    int aidToOtuId (string aid);
    bool updateNotifyCache();

    //Serdes
    void updateSerdesCfgTbl(string aid, const vector<tuning::AtlBcmSerdesTxCfg> & atlBcmTbl);
    bool getOtuConfig(TypeMapOtuConfig& mapCfg);
    void translateConfig(const OtuConfig& dcoCfg, OtuCommon& adCfg);
    void configFir(int serdesLane, OtuConfig* dcoCfg);

    static DataPlane::DpMsOTUPm mOtuPms;
    static AdapterCacheFault mOtuFault[MAX_OTU_ID];
    static AdapterCacheTTI mTtiRx[MAX_OTU_ID];
    static AdapterCacheOpStatus mOpStatus[MAX_OTU_ID];

    static high_resolution_clock::time_point mStart;
    static high_resolution_clock::time_point mEnd;
    static int64_t mStartSpan;
    static int64_t mEndSpan;
    static int64_t mDelta;
    static deque<int64_t> mStartSpanArray;
    static deque<int64_t> mEndSpanArray;
    static deque<int64_t>  mDeltaArray;
    static bool mOtuStateSubEnable;
    static bool mOtuFltSubEnable;
    static bool mOtuPmSubEnable;
    static bool mOtuOpStatusSubEnable;

    //TODO: remove before release
    static unordered_map<string, vector<std::time_t>> ebOverflowMap;

    static void updateEbOverflowMap(string key)
    {
        auto nowTime = std::chrono::system_clock::now();
        std::time_t curTime = std::chrono::system_clock::to_time_t(nowTime);
        vector<std::time_t> timeStamps = ebOverflowMap[key];
        if (timeStamps.size() == ebMapSize)
        {
            timeStamps.erase(timeStamps.begin());
        }
        timeStamps.push_back(curTime);
    }

private:
    bool setKey (Otu *otu, int id);
    bool getOtuObj( Otu *otu, int otuId, int *idx);
    bool getOtuFaultCapa();

// Hardcode from DCO Yang Definitions since we lose the fraction during the Yang to proto conversion
    static const int DCO_RATE_PRECISION = 2;
    static const int DCO_DELAY_MEASUERMENT_PRECISION = 3;
    static constexpr double DCO_OTU_RATE_MIN = 100.0;
    static constexpr double DCO_OTU_RATE_MAX = 800.0;
    static const std::string DCO_OTU_SAPI_MISM_NAME_STR;

    static const std::array<uint8_t, MAX_CLIENTS + 1> mChm6_PortMap;

    std::recursive_mutex  mMutex;
    std::shared_ptr<::DpSim::SimSbGnmiClient>   mspSimCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspSbCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspCrud;

    bool                        mIsGnmiSimModeEn;
    OtuFaultCapability          mFaultCapa;
    uint32_t                    mOtuRate[MAX_OTU_ID+1];  // Cache rate
    uint64_t                    mRxSapiMismBitmap;  // Get from faul capa
    bool                        mbBdiInject[MAX_OTU_ID+1];
    bool                        mbFecCorrEnable[MAX_OTU_ID+1];
    std::thread                 mSimOtuPmThread;
};

}
#endif // DCO_OTU_ADAPTER_H
