/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_CARRIER_ADAPTER_H
#define DCO_CARRIER_ADAPTER_H

#include <mutex>
#include <SimCrudService.h>

#include "dcoyang/infinera_dco_carrier/infinera_dco_carrier.pb.h"
#include "dcoyang/infinera_dco_carrier_fault/infinera_dco_carrier_fault.pb.h"
#include "CarrierAdapter.h"
#include "CrudService.h"
#include "DpPm.h"
#include "types.h"
#include "DcoGccControlAdapter.h"

using namespace GnmiClient;
//using namespace DataPlane;

using Carrier = dcoyang::infinera_dco_carrier::Carriers;
using CarrierConfig = dcoyang::infinera_dco_carrier::Carriers_Carrier_Config;
using DcoCarrierState = dcoyang::infinera_dco_carrier::Carriers_Carrier_State;

namespace DpAdapter {

typedef  std::map<std::string, std::shared_ptr<CarrierCommon>> TypeMapCarrierConfig;

// Carrier Adapter Base class
class DcoCarrierAdapter : public CarrierAdapter
{
public:
    DcoCarrierAdapter();
    virtual ~DcoCarrierAdapter();

    virtual bool createCarrier(std::string aid, CarrierCommon *cfg);
    virtual bool deleteCarrier(std::string aid);
    virtual bool initializeCarrier();

    virtual bool setCarrierStatus(std::string aid);
    virtual bool deleteCarrierName(string aid);
    virtual bool setCarrierCapacity(std::string aid, uint32_t dataRate);
    virtual bool deleteCarrierCapacity(string aid);
    virtual bool setCarrierBaudRate(std::string aid, double baudRate);
    virtual bool deleteCarrierBaudRate(string aid);
    virtual bool setCarrierAppCode(std::string aid, std::string appCode);
    virtual bool deleteCarrierAppCode(string aid);
    virtual bool setCarrierEnable(std::string aid, bool enable);
    virtual bool deleteCarrierEnable(string aid);
    virtual bool setCarrierFrequency(std::string aid, double frequency);
    virtual bool deleteCarrierFrequency(string aid);
    virtual bool setCarrierFreqOffset(std::string aid, double offset);
    virtual bool deleteCarrierFreqOffset(string aid);
    virtual bool setCarrierTargetPower(std::string aid, double power);
    virtual bool deleteCarrierTargetPower(string aid);
    virtual bool setCarrierTxCdPreComp(std::string aid, double txCd);
    virtual bool deleteCarrierTxCdPreComp(string aid);
    virtual bool setCarrierAdvParam(std::string aid, std::vector<AdvancedParameter> ap);
    virtual bool deleteCarrierAdvParam(std::string aid, std::string key);
    virtual bool deleteCarrierAdvParamAll(std::string aid);
    virtual bool setCarrierDgdOorhThresh(std::string aid, double threshold);
    virtual bool deleteCarrierDgdOorhThresh(string aid);
    virtual bool setCarrierPostFecQSdThresh(std::string aid, double threshold);
    virtual bool deleteCarrierPostFecQSdThresh(string aid);
    virtual bool setCarrierPostFecQSdHyst(std::string aid, double hysteresis);
    virtual bool deleteCarrierPostFecQSdHyst(string aid);
    virtual bool setCarrierPreFecQSdThresh(std::string aid, double threshold);
    virtual bool deleteCarrierPreFecQSdThresh(string aid);
    virtual bool setCarrierPreFecQSdHyst(std::string aid, double hysteresis);
    virtual bool deleteCarrierPreFecQSdHyst(string aid);
    virtual bool deleteCarrierConfigAttributes(string aid);
    virtual bool setCarrierProvisioned(std::string aid, bool bProvisioned);

    virtual bool getCarrierConfig(std::string aid, CarrierCommon *cfg);
    virtual bool getCarrierStatus(std::string aid, CarrierStatus *stat);
    virtual bool getCarrierStatusNotify(std::string aid, CarrierStatus *stat);
    virtual bool getCarrierFault(std::string aid, uint64_t *fault);
    virtual bool getCarrierFault(std::string aid, CarrierFault *faults, bool force = false);
    virtual bool getCarrierPm(std::string aid);
    virtual void subCarrierPm();
    virtual void subCarrierFault();
    virtual void subCarrierState();
    virtual void subscribeStreams();
    virtual void clearFltNotifyFlag();

    static  double  getCacheCarrierDataRate(int carrierId); // To call in OTU adapter

    virtual bool carrierConfigInfo(std::ostream& out, std::string aid);
    virtual bool carrierStatusInfo(std::ostream& out, std::string aid);
    virtual bool carrierFaultInfo(std::ostream& out, std::string aid);
    virtual bool carrierPmInfo(std::ostream& out, int carrierId);
    virtual void carrierNotifyState(std::ostream& out, int carrierId);
    virtual bool carrierPmAccumInfo(std::ostream& out, int carrierId);
    virtual bool carrierPmAccumClear(std::ostream& out, int carrierId);
    virtual bool carrierPmAccumEnable(std::ostream& out, int carrierId, bool enable);
    virtual void carrierPmTimeInfo(ostream& out);

    virtual bool carrierConfigInfo(std::ostream& out);
    void dumpConfigInfo(std::ostream& out, const CarrierCommon& cfg);
    void dumpStatusInfo(std::ostream& out, const CarrierStatus& stat);

    void dumpAll(std::ostream& out);

    bool isSimModeEn() { return mIsGnmiSimModeEn; }
    void setSimModeEn(bool enable);
    void SetSimPmData();

    int aidToCarrierId (std::string aid);

    bool getCarrierConfig(TypeMapCarrierConfig& mapCfg);

    void translateConfig(const CarrierConfig& dcoCfg, CarrierCommon& adCfg);
    void translateStatus(const DcoCarrierState& dcoState, CarrierStatus& stat);

    bool updateNotifyCache();

    static DataPlane::DpMsCarrierPMs mCarrierPms;
    static AdapterCacheFault mCarrierEqptFault[MAX_CARRIERS];
    static AdapterCacheFault mCarrierFacFault[MAX_CARRIERS];
    static DataPlane::CarrierState mTxState[MAX_CARRIERS];
    static DataPlane::CarrierState mRxState[MAX_CARRIERS];
    static vector<AdvancedParameter> mAdvParam[MAX_CARRIERS];
    static double mTxActualFrequency[MAX_CARRIERS];
    static double mRxActualFrequency[MAX_CARRIERS];

    static high_resolution_clock::time_point mStart;
    static high_resolution_clock::time_point mEnd;
    static int64_t mStartSpan;
    static int64_t mEndSpan;
    static int64_t mDelta;
    static deque<int64_t> mStartSpanArray;
    static deque<int64_t> mEndSpanArray;
    static deque<int64_t>  mDeltaArray;
    static bool mCarrierFltSubEnable;
    static bool mCarrierPmSubEnable;
    static bool mCarrierStateSubEnable;

private:
    bool setKey (Carrier *carrier, int id);
    bool setCarrierConfig(std::string aid, CarrierCommon *cfg);
    bool getCarrierObj( Carrier *carrier, int carrierId, int *idx);
    bool getCarrierFaultCapa();
    bool updateCarrierStatusNotify( std::string aid, CarrierStatus *stat);
    bool badApjWorkAround(string aid, std::vector<AdvancedParameter> ap);

// Hardcode from DCO Yang Definitions since we lose the fraction during the Yang to proto conversion
    static const int DCO_FREQ_PRECISION = 2;
    static const int DCO_FREQ_OFFSET_PRECISION = 2;
    static const int DCO_POWER_PRECISION = 2;
    static const int DCO_TX_CD_PRECISION = 2;
    static const int DCO_BAUD_RATE_PRECISION = 7;
    static const int DCO_DGD_TH_PRECISION = 1;
    static const int DCO_FEC_TH_PRECISION = 2;
    static constexpr double DCO_DGD_OORH_TH_MIN = 180.0;
    static constexpr double DCO_DGD_OORH_TH_MAX = 350.0;
    static constexpr double DCO_POST_FEC_SD_TH_MIN = 12.50;
    static constexpr double DCO_POST_FEC_SD_TH_MAX = 30.00;
    static constexpr double DCO_POST_FEC_SD_HYS_MIN = 0.10;
    static constexpr double DCO_POST_FEC_SD_HYS_MAX = 3.00;
    static constexpr double DCO_PRE_FEC_SD_TH_MIN = 5.60;
    static constexpr double DCO_PRE_FEC_SD_TH_MAX = 7.00;
    static constexpr double DCO_PRE_FEC_SD_HYS_MIN = 0.10;
    static constexpr double DCO_PRE_FEC_SD_HYS_MAX = 1.00;

    static std::recursive_mutex  mMutex;
    CarrierCommon   mCfg[DataPlane::MAX_CARRIERS];
    CarrierStatus   mStat[DataPlane::MAX_CARRIERS];

    bool                       mIsGnmiSimModeEn;
    std::shared_ptr<::DpSim::SimSbGnmiClient>   mspSimCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspSbCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspCrud;
    CarrierFaultCapability      mFaultCapa;
    static double               mDataRate[MAX_CARRIERS+1]; // one based

    std::thread			mSimCarrierPmThread;
};

}
#endif // DCO_CARRIER_ADAPTER_H
