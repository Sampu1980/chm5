/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef ODU_STATE_COLLECTOR_H
#define ODU_STATE_COLLECTOR_H

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "OduPbTranslator.h"
#include "DcoOduAdapter.h"
#include "DpProtoDefs.h"
#include "DpStateCollector.h"

#include "FpgaRegIf.h"
#include "RegIfFactory.h"

namespace DataPlane {

class OduStateCollector : public DpStateCollector
{
public:

    OduStateCollector(OduPbTranslator* pbTrans, DpAdapter::DcoOduAdapter *pAd, shared_ptr<FpgaSramIf> fpgaSramIf);

    ~OduStateCollector() {}

    void start();

    void createOdu(const chm6_dataplane::Chm6OduConfig& inCfg);
    void deleteOdu(std::string oduAid);

    void updateConfigCache(const std::string oduAid, const chm6_dataplane::Chm6OduConfig* pCfg);

    void createFaultPmPlaceHolder(std::string oduAid);

    virtual void dumpCacheState      (std::string oduAid, std::ostream& os);
    virtual void listCacheState(std::ostream& os);
    void dumpCachePm         (std::string oduAid, std::ostream& os);
    void dumpCacheFault      (std::string oduAid, std::ostream& os);
    void dumpCacheStateConfig(std::string oduAid, std::ostream& os);
    void dumpSdTracking      (std::string oduAid, std::ostream& os);

    void setTimAlarmMode (bool mode, std::ostream& os);
    void setSdAlarmMode  (bool mode, std::ostream& os);

    virtual void setIsConnectionUp(bool val);

    virtual void updateInstalledConfig();
    void updateInstalledConfig(const chm6_dataplane::Chm6OduConfig& inCfg);

    int getCacheState(std::string configId, chm6_dataplane::Chm6OduState*& pMsg);

    void updateOduIdx(std::string configId, uint32 oduIdx);

    bool checkBootUpTraffic(void);

private:

    void collectOduStatus();
    void collectOduFaults();

    void getAndUpdateOduStatus();
    void getAndUpdateOduFaults();

    void createConfigCache(const std::string oduAid);
    void createSdTracking(const std::string oduAid, FaultDirection direction);

    bool evaluateTimFault(std::string oduAid, bool isHardFault);

    bool evaluateSdFault(std::string oduAid, bool isHardFault, FaultDirection direction);
    bool getOduSdScratchpad(std::string oduAid, FaultDirection direction);
    void updateOduSdScratchpad(std::string otuAid, bool sdFault, FaultDirection direction);

    bool isValidOduId(int oduId) { return ( ((oduId < 1) || (oduId > MAX_ODU_ID)) ? false : true ); }
    bool isClientOduId(int oduId) { return ( (oduId > CLIENT_ODU_ID_OFFSET) ? true : false ); }

    std::string toString(const FaultDirection direction) const;

    void findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6OduState*& pState);

    boost::thread mThrFaults;
    boost::thread mThrState;

    std::mutex mOduDataMtx;
    typedef std::map<std::string, chm6_dataplane::Chm6OduState*> TypeMapOduState;
    TypeMapOduState mMapOduState;

    typedef std::map<std::string, chm6_dataplane::Chm6OduFault*> TypeMapOduFault;
    TypeMapOduFault mMapOduFault;

    typedef std::map<std::string, uint64_t> TypeMapOduFaultBitmap;
    TypeMapOduFaultBitmap mMapOduFacFaultBitmap;

    typedef std::map<std::string, chm6_dataplane::Chm6OduPm*> TypeMapOduPm;
    TypeMapOduPm mMapOduPm;

    OduPbTranslator*           mpPbTranslator;
    DpAdapter::DcoOduAdapter*  mpDcoAd;
    shared_ptr<FpgaSramIf>     mspFpgaSramIf;

    // Alarm Configuration
    bool mTimAlarmMode;
    bool mSdAlarmMode;

    typedef struct
    {
        uint8_t                     ttiExpected[MAX_TTI_LENGTH];
        hal_common::TtiMismatchType ttiMismatchType;
        uint                        sdThresholdRx;
        uint                        sdIntervalRx;
        uint                        sdThresholdTx;
        uint                        sdIntervalTx;
    } OduConfigCache;

    typedef std::map<std::string, OduConfigCache*> TypeMapOduConfigCache;
    TypeMapOduConfigCache mMapOduConfigCache;

    // SD Alarm
    typedef struct
    {
        bool isSdFaulted;        // Current signal degrade fault state
        uint sdRaiseSeconds;     // Consecutive seconds of signal degreade
        uint sdClearSeconds;     // Consecutive seconds of no signale degrade
    } OduSdTracking;

    typedef std::map<std::string, OduSdTracking*> TypeMapOduSdTracking;
    TypeMapOduSdTracking mMapOduSdRxTracking;
    TypeMapOduSdTracking mMapOduSdTxTracking;
};

} // namespace DataPlane

#endif /* ODU_STATE_COLLECTOR_H */
