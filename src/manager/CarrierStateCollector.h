/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef CARRIER_STATE_COLLECTOR_H
#define CARRIER_STATE_COLLECTOR_H

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "CarrierPbTranslator.h"
#include "DcoCarrierAdapter.h"
#include "DpStateCollector.h"
#include "InfnLogger.h"

#include "FpgaRegIf.h"
#include "RegIfFactory.h"


namespace DataPlane {

class CarrierStateCollector : public DpStateCollector
{
public:

    CarrierStateCollector(CarrierPbTranslator* pbTrans, DpAdapter::DcoCarrierAdapter *pCarAd, shared_ptr<FpgaSramIf> fpgaSramIf);

    ~CarrierStateCollector() {}

    void start();

    void createCarrier(const chm6_dataplane::Chm6CarrierConfig& inCfg);
    void deleteCarrier(std::string carrierAid);

    void updateCarrierSdScratchpad(std::string carAid, bool sdFault);
    void updateCarrierSfScratchpad(std::string carAid, bool sfFault);
    void updateCarrierPreFecSdScratchpad(std::string carAid, bool preFecsdFault);
    void updateCarrierBsdScratchpad(std::string carAid, bool preFecsdFault);
    void updateCarrierOlosScratchpad(std::string carAid, bool isOlosFault);

    void createFaultPmPlaceHolderScg(std::string scgAid);
    void createFaultPmPlaceHolder(std::string carrierAid);

    virtual void dumpCacheState(std::string carAid, std::ostream& os);
    virtual void listCacheState(std::ostream& os);
    void dumpCacheFault(std::string carAid, std::ostream& os);

    virtual void updateInstalledConfig();
    void updateInstalledConfig(const chm6_dataplane::Chm6CarrierConfig& inCfg);

    virtual void setIsConnectionUp(bool val);

    int getCacheState(std::string configId, chm6_dataplane::Chm6CarrierState*& pMsg);
    bool checkBootUpTraffic(void);

    void SetForceCarrFaultToNxp(std::string carrId)
    {
        mForceCarrFaultToNxp[carrId] = true;
    };
    void SetForceCarrStateToNxp(std::string carrId)
    {
        mForceCarrFaultToNxp[carrId] = true;
    };

private:

    void collectCarrierFaults();
    void collectCarrierState();

    void getAndUpdateCarrierFaults();
    void getAndUpdateCarrierState();

    void findOrCreateStateObj(std::string carId, chm6_dataplane::Chm6CarrierState* &pState);

    uint carrierAidtoNum(std::string Aid);

    boost::thread mThrFaults;
    boost::thread mThrStats;

    std::mutex mCarDataMtx;

    typedef std::map<std::string, chm6_dataplane::Chm6CarrierState*> TypeMapCarrierState;
    TypeMapCarrierState mMapCarState;

    typedef std::map<std::string, chm6_dataplane::Chm6CarrierFault*> TypeMapCarrierFault;
    TypeMapCarrierFault mMapCarFault;

    //Variable tracking the force sending of carrier fault data to NXP (is set to TRUE if GNMi SET fails or incase of NXP GRPC connection is reset)
    typedef std::map<std::string, bool> TypeForceCarrierFaultToDco;
    TypeForceCarrierFaultToDco mForceCarrFaultToNxp;

    //Variable tracking the force sending of carrier state data to NXP (is set to TRUE if GNMi SET fails or incase of NXP GRPC connection is reset)
    typedef std::map<std::string, bool> TypeForceCarrierStateToDco;
    TypeForceCarrierStateToDco mForceCarrStateToNxp;

    typedef std::map<std::string, chm6_dataplane::Chm6CarrierPm*> TypeMapCarrierPm;
    TypeMapCarrierPm mMapCarPm;

    typedef std::map<std::string, uint64_t> TypeMapCarrierFaultBitmap;
    TypeMapCarrierFaultBitmap mMapCarFacFaultBitmap;

    CarrierPbTranslator*          mpPbTranslator;
    DpAdapter::DcoCarrierAdapter* mpDcoCarrierAd;
    shared_ptr<FpgaSramIf>        mspFpgaSramIf;

    typedef std::map<std::string, hal_dataplane::CarrierOpticalState> TypeMapCarrTxState;
    TypeMapCarrTxState mCarrTxState;
};

} // namespace DataPlane

#endif /* CARRIER_STATE_COLLECTOR_H */
