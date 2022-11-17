/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef OTU_STATE_COLLECTOR_H
#define OTU_STATE_COLLECTOR_H

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "OtuPbTranslator.h"
#include "DcoOtuAdapter.h"
#include "DpProtoDefs.h"
#include "DpStateCollector.h"

#include "FpgaRegIf.h"
#include "RegIfFactory.h"


namespace DataPlane {

class OtuStateCollector : public DpStateCollector
{
public:

    OtuStateCollector(OtuPbTranslator* pbTrans, DpAdapter::DcoOtuAdapter *pAd, shared_ptr<FpgaSramIf> fpgaSramIf);

    ~OtuStateCollector() {}

    void start();

    void createOtu(const chm6_dataplane::Chm6OtuConfig& inCfg);
    void deleteOtu(std::string otuAid);

    void updateConfigCache(const std::string otuAid, const chm6_dataplane::Chm6OtuConfig* pCfg);

    void createFaultPmPlaceHolder(std::string otuAid);

    virtual void dumpCacheState(std::string otuAid, std::ostream& os);
    virtual void listCacheState(std::ostream& os);
    void dumpCachePm         (std::string otuAid, std::ostream& os);
    void dumpCacheFault      (std::string otuAid, std::ostream& os);
    void dumpCacheStateConfig(std::string otuAid, std::ostream& os);
    void dumpSdTracking      (std::string otuAid, std::ostream& os);
    void dumpTimTracking     (std::string otuAid, std::ostream& os);

    bool isTimFault      (std::string otuAid);
    void setTimAlarmMode (bool mode, std::ostream& os);
    void setSdAlarmMode  (bool mode, std::ostream& os);

    virtual void setIsConnectionUp(bool val);

    virtual void updateInstalledConfig();
    void updateInstalledConfig(const chm6_dataplane::Chm6OtuConfig& inCfg);

    int getCacheState(std::string configId, chm6_dataplane::Chm6OtuState*& pMsg);
    uint getInstalledOtuClients(void) { return mInstalledOtuClients; }

    bool checkBootUpTraffic(void);

private:

    void collectOtuStatus();
    void collectOtuFaults();

    void getAndUpdateOtuStatus();
    void getAndUpdateOtuFaults();

    void createConfigCache(const std::string otuAid);
    void createSdTracking(const std::string otuAid);

    bool evaluateTimFault(std::string otuAid, bool isHardFault);

    bool evaluateSdFault(std::string otuAid, bool isHardFault);
    bool getOtuSdScratchpad(std::string otuAid);
    void updateOtuSdScratchpad(std::string otuAid, bool sdFault);

    bool isValidOtuId(int otuId) { return ( ((otuId < 1) || (otuId > MAX_OTU_ID)) ? false : true ); }
    bool isClientOtuId(int otuId) { return ( (otuId <= MAX_CLIENTS) ? true : false ); }

    void findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6OtuState*& pState);

    boost::thread mThrState;
    boost::thread mThrFaults;

    std::mutex mOtuDataMtx;
    typedef std::map<std::string, chm6_dataplane::Chm6OtuState*> TypeMapOtuState;
    TypeMapOtuState mMapOtuState;

    typedef std::map<std::string, chm6_dataplane::Chm6OtuFault*> TypeMapOtuFault;
    TypeMapOtuFault mMapOtuFault;

    typedef std::map<std::string, uint64_t> TypeMapOtuFaultBitmap;
    TypeMapOtuFaultBitmap mMapOtuFacFaultBitmap;

    typedef std::map<std::string, chm6_dataplane::Chm6OtuPm*> TypeMapOtuPm;
    TypeMapOtuPm mMapOtuPm;

    OtuPbTranslator          *mpPbTranslator;
    DpAdapter::DcoOtuAdapter *mpDcoAd;
    shared_ptr<FpgaSramIf>   mspFpgaSramIf;

    // Alarm Configuration
    bool mTimAlarmMode;
    bool mSdAlarmMode;

    typedef struct
    {
        uint8_t                     ttiExpected[MAX_TTI_LENGTH];
        hal_common::TtiMismatchType ttiMismatchType;
        uint                        sdThreshold;
        uint                        sdInterval;
    } OtuConfigCache;

    typedef std::map<std::string, OtuConfigCache*> TypeMapOtuConfigCache;
    TypeMapOtuConfigCache mMapOtuConfigCache;

    typedef std::map<std::string, bool> TypeMapOtuTimTracking;
    TypeMapOtuTimTracking mMapOtuTimTracking;

    // SD Alarm
    typedef struct
    {
        bool isSdFaulted;        // Current signal degrade fault state
        uint sdRaiseSeconds;     // Consecutive seconds of signal degreade
        uint sdClearSeconds;     // Consecutive seconds of no signale degrade
    } OtuSdTracking;

    typedef std::map<std::string, OtuSdTracking*> TypeMapOtuSdTracking;
    TypeMapOtuSdTracking mMapOtuSdTracking;

    // TOM Serdes
    uint mInstalledOtuClients;
};


} // namespace DataPlane

#endif /* OTU_STATE_COLLECTOR_H */
