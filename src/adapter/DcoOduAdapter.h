/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_ODU_ADAPTER_H
#define DCO_ODU_ADAPTER_H

#include <mutex>
#include <ctime>
#include <SimCrudService.h>

#include "dcoyang/infinera_dco_odu/infinera_dco_odu.pb.h"
#include "dcoyang/infinera_dco_odu_fault/infinera_dco_odu_fault.pb.h"
#include "OduAdapter.h"
#include "CrudService.h"
#include "DpPm.h"
#include "types.h"

using namespace GnmiClient;
using namespace DataPlane;

using Odu = dcoyang::infinera_dco_odu::Odus;
using OduConfig = dcoyang::infinera_dco_odu::Odus_Odu_Config;
using DcoOduState = dcoyang::infinera_dco_odu::Odus_Odu_State;

namespace DpAdapter {

typedef std::map<std::string, std::shared_ptr<OduCommon>> TypeMapOduConfig;
typedef std::map<uint32     , std::shared_ptr<OduStatus>> TypeMapOduStatus;
typedef std::map<uint32     , std::shared_ptr<OduFault >> TypeMapOduFault;


class DcoOduAdapter: public OduAdapter
{
public:
    DcoOduAdapter();
    virtual ~DcoOduAdapter();

    virtual bool createOduClientXpress(string aid);
    virtual bool createOdu(string aid, OduCommon *cfg);
    virtual bool deleteOdu(string aid);
    virtual bool initializeOdu();

    virtual bool setOduLoopBack(string aid, DataPlane::LoopBackType mode);
    virtual bool setOduTti(string aid, uint8_t *tti);
    virtual bool setOduAutoMS(string aid, Direction dir, DataPlane::MaintenanceSignal mode);
    virtual bool setOduForceMS(string aid, Direction dir, DataPlane::MaintenanceSignal mode);
    virtual bool setOduPrbsMon(string aid, Direction dir, PrbsMode mode);
    virtual bool setOduPrbsGen(string aid, Direction dir, PrbsMode mode);
    virtual bool setOduTimeSlot(string aid, std::vector<uint32_t> tS);
    virtual bool setOduFaultStatus(string aid, uint64_t injectFaultBitmap);
    virtual bool setOduTimBdi(string aid, bool bInject);
    virtual bool setOduRegen(string aid, bool bRegen);

    virtual bool getOduConfig(string aid, OduCommon *cfg);
    virtual bool getOduStatus(string aid, OduStatus *stat);
    virtual bool getOduFaultRx(string aid, uint64_t *fault);
    virtual bool getOduFaultTx(string aid, uint64_t *fault);
    virtual bool getOduFault(string aid, OduFault *faults, bool force = false);
    virtual bool getOduPm(string aid);
    virtual uint32_t getOduFrameRate(string aid);
    virtual uint64_t getOduPmErrorBlocks(string aid, DataPlane::FaultDirection direction);

    virtual void subOduPm();
    virtual void subOduFault();
    virtual void subOduState();
    virtual void subOduOpStatus();
    static  int  getCacheOduId(string aid);     // To call in XCON adapter
    static  std::string getCacheOduAid(uint32 oduId);
    virtual void subscribeStreams();
    virtual void clearNotifyFlag();

    virtual bool oduConfigInfo(ostream& out, string aid);
    virtual bool oduStatusInfo(ostream& out, string aid);
    virtual bool oduStatusInfoAll(ostream& out);
    virtual bool oduFaultInfo(ostream& out, string aid);
    virtual bool oduFaultInfoAll(ostream& out);
    virtual bool oduCdiInfo(ostream& out, string aid);
    virtual bool oduPmInfo(std::ostream& out, int oduId);
    virtual bool oduPmInfoAll(std::ostream& out);
    virtual void oduNotifyState(std::ostream& out, int oduId);
    virtual void oduNotifyStateAll(std::ostream& out);
    virtual bool oduPmAccumInfo(std::ostream& out, int oduId);
    virtual bool oduPmAccumClear(std::ostream& out, int oduId);
    virtual bool oduPmAccumEnable(std::ostream& out, int oduId, bool enable);
    virtual void oduPmTimeInfo(ostream& out);
    virtual bool oduConfigInfo(std::ostream& out);
    void dumpConfigInfo(std::ostream& out, const OduCommon& cfg);

    void dumpAll(std::ostream& out);

    bool isSimModeEn() { return mIsGnmiSimModeEn; }
    void setSimModeEn(bool enable);
    void setSimPmData();
    uint8_t* getOduRxTti(const string aid);
    Cdi getOduCdi(const string aid);

    bool getOduConfig(TypeMapOduConfig& mapCfg);
    void translateConfig(const OduConfig& dcoCfg, OduCommon& adCfg);

    bool getOduStatus(TypeMapOduStatus& mapStat);
    void translateStatus(const DcoOduState& dcoStat, OduStatus& adStat, bool updateCache = false);

    bool getOduFault(TypeMapOduFault& mapCfg);
    void translateFault(uint64 dcoFltBmp, OduFault& adFlt);

    int aidToOduId (string aid);
    bool updateNotifyCache();

    static DataPlane::DpMsODUPm mOduPms;
    static AdapterCacheFault mOduFault[MAX_ODU_ID];
    static const std::vector<uint32_t> mClientOduTimeSlot;
    static AdapterCacheTTI mTtiRx[MAX_ODU_ID];
    static AdapterCacheOpStatus mOpStatus[MAX_ODU_ID];

    static high_resolution_clock::time_point mStart;
    static high_resolution_clock::time_point mEnd;
    static int64_t mStartSpan;
    static int64_t mEndSpan;
    static int64_t mDelta;
    static deque<int64_t> mStartSpanArray;
    static deque<int64_t> mEndSpanArray;
    static deque<int64_t>  mDeltaArray;

    static bool mOduStateSubEnable;
    static bool mOduFltSubEnable;
    static bool mOduPmSubEnable;
    static bool mOduOpStatusSubEnable;

    static uint8_t mCdi[MAX_ODU_ID];

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
    int aidToHoOtuId (string aid);
    bool setKey (Odu *odu, int id);
    bool getOduObj( Odu *odu, int oduId, int *idx);
    bool getOduFaultCapa();
    bool cacheOduAid(string aid, OduCommon *cfg);
    bool cacheOduAid(string aid, uint32 oduId);

    void dumpOduStatusInfo(std::ostream& out, OduStatus& oduStat, uint32 loOduId);
    void dumpOduFaultInfo (std::ostream& out, OduFault& oduFault);
    void dumpOduPmInfo    (std::ostream& out, DpMsOduPMContainer& oduPm);

// Hardcode from DCO Yang Definitions since we lose the fraction during the Yang to proto conversion
    static const int ODU_RATE_PRECISION = 2;
    // Lo-ODU id is from 1-32.  33-34 is for HO-ODUCni.
    static const int HO_ODU_ID_OFFSET = 32;
    static const int ODU_FORCE_UPDATE_TIME_SEC = 2;
    static const std::string DCO_ODU_SAPI_MISM_NAME_STR;

    static std::recursive_mutex  mMutex;

    bool                       mIsGnmiSimModeEn;
    std::shared_ptr<::DpSim::SimSbGnmiClient>   mspSimCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspSbCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspCrud;
    OduFaultCapability          mFaultCapa;
    OduCdiCapability            mCdiCapa;
    static string               mOduIdToAid[MAX_ODU_ID+1];
    OduPayLoad                  mOduPayLoad[MAX_ODU_ID+1];
    uint64_t                    mRxSapiMismBitmap;  // Get from faul capa
    bool                        mbBdiInject[MAX_ODU_ID+1];

    std::thread			mSimOduPmThread;

};

}
#endif // DCO_ODU_ADAPTER_H
