/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef CARRIER_ADAPTER_H
#define CARRIER_ADAPTER_H

#include "dp_common.h"
#include "AdapterCommon.h"
#include <string>


//using namespace std;
//using namespace DataPlane;

namespace DpAdapter {
typedef struct
{
    std::string apName;
    std::string apValue;
    bool        bDefault;  // True if default and have not change by user
    std::string defaultValue;  // default value of ap
    DataPlane::CarrierAdvParmStatus  apStatus;  // For state object only
} AdvancedParameter;

typedef struct
{
    std::string   name;      // AID
    bool     enable;
    double   frequency;
    double   freqOffset;
    std::string   appCode;
    uint32_t capacity;      // Data rate
    double   baudRate;
    double   targetPower;
    double   adTargetPower;  // Auto-discovery target power
    bool     adEnable;
    uint8_t  adId[8];       // Auto-discovery ID
    bool     regenEnable;
    double   txCdPreComp;
    bool     listenOnlyEnable;
    std::vector<AdvancedParameter> ap;
    double   dgdOorhThresh;
    double   postFecQSdThresh;
    double   postFecQSdHysteresis;
    double   preFecQSdThresh;
    double   preFecQSdHysteresis;
    bool     provisioned;    // Indicate where we have super-channel created.
} CarrierCommon;

typedef struct
{
    uint64_t fecIteration;
    uint64_t fecOhPercentage;
    std::string   appCode;
    double capacity;      // Data rate
    double baudRate;
    double refClockConfig;
    DataPlane::CarrierState txState;
    DataPlane::CarrierState rxState;
    std::string lineMode;     // 800E90P etc.
    uint64_t eqptFault;
    uint64_t facFault;
    double txCdPreComp;
    double rxCd;
    double targetPower;
    double frequency;
    double  freqOffset;
    std::vector<AdvancedParameter> currentAp;
    double   dgdOorhThresh;
    double   postFecQSdThresh;
    double   postFecQSdHysteresis;
    double   preFecQSdThresh;
    double   preFecQSdHysteresis;
    bool operationState;
    double rxAcqTime;
    uint64_t rxAcqCountLc;  // Acq count since last clear
    double txActualFrequency;
    double rxActualFrequency;
} CarrierStatus;

typedef struct
{
    std::vector<FaultCapability> eqpt;
    std::vector<FaultCapability> fac;
} CarrierFaultCapability;

typedef struct
{
    std::vector<AdapterFault> eqpt;
    std::vector<AdapterFault> fac;
    uint64_t eqptBmp;    // Faults bitmap
    uint64_t facBmp;
} CarrierFault;


// Carrier Adapter Base class
class CarrierAdapter
{
public:
    CarrierAdapter() {}
    virtual ~CarrierAdapter() {}

    virtual bool createCarrier(std::string aid, CarrierCommon *cfg) = 0;
    virtual bool deleteCarrier(std::string aid) = 0;
    virtual bool initializeCarrier() = 0;

    virtual bool setCarrierCapacity(std::string aid, uint32_t dataRate) = 0;
    virtual bool setCarrierBaudRate(std::string aid, double baudRate) = 0;
    virtual bool setCarrierAppCode(std::string aid, std::string appCode) = 0;
    virtual bool setCarrierEnable(std::string aid, bool enable) = 0;
    virtual bool setCarrierFrequency(std::string aid, double frequency) = 0;
    virtual bool setCarrierFreqOffset(std::string aid, double offset) = 0;
    virtual bool setCarrierTargetPower(std::string aid, double power) = 0;
    virtual bool setCarrierAdvParam(std::string aid, std::vector<AdvancedParameter> ap) = 0;
    virtual bool deleteCarrierAdvParamAll(std::string aid) = 0;
    virtual bool deleteCarrierConfigAttributes(std::string aid) = 0;
    virtual bool setCarrierDgdOorhThresh(std::string aid, double threshold) = 0;
    virtual bool setCarrierPostFecQSdThresh(std::string aid, double threshold) = 0;
    virtual bool setCarrierPostFecQSdHyst(std::string aid, double hysteresis) = 0;
    virtual bool setCarrierPreFecQSdThresh(std::string aid, double threshold) = 0;
    virtual bool setCarrierPreFecQSdHyst(std::string aid, double hysteresis) = 0;
    virtual bool setCarrierProvisioned(std::string aid, bool bProvisioned) = 0;

    virtual bool getCarrierConfig(std::string aid, CarrierCommon *cfg) = 0;
    virtual bool getCarrierStatus(std::string aid, CarrierStatus *stat) = 0;
    virtual bool getCarrierStatusNotify(std::string aid, CarrierStatus *stat) = 0;
    virtual bool getCarrierFault(std::string aid, uint64_t *fault) = 0;
    virtual bool getCarrierFault(std::string aid, CarrierFault *faults, bool force = false) = 0;
    virtual bool getCarrierPm(std::string aid) = 0;

    virtual bool carrierConfigInfo(std::ostream& out, std::string aid) = 0;
    virtual bool carrierStatusInfo(std::ostream& out, std::string aid) = 0;
    virtual bool carrierFaultInfo(std::ostream& out, std::string aid) = 0;
};

}
#endif // CARRIER_ADAPTER_H
