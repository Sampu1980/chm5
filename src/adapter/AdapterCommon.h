
/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef ADAPTER_COMMON_H
#define ADAPTER_COMMON_H
#include <cstring>
#include <string>
#include <vector>
#include "dp_common.h"
#include "types.h"
#include <iomanip>
#include <sstream>
#include <chrono>

using namespace std;
using namespace DataPlane;

namespace DpAdapter {


#define FAULT_BIT_POS_UNSPECIFIED  256  // Set to some our of range value.
#define DCO_TRANS_ID_TIMEOUT_MSEC 120000   // 2 minutes
#define DCO_TRANS_ID_DELAY_MSEC   100
static constexpr double DCO_TS_GRANULARITY = 25.0;  // 25Gb
static constexpr double LXTF20_E_FRAME_RATE = 2690.54936896759;
static constexpr double LXTF20_M_FRAME_RATE = 2808.575281;
static constexpr uint32_t OTU4_FRAME_RATE = 856388;

static int ebMapSize = 100;

enum FaultSeverity {
    FAULTSEVERITY_UNSPECIFIED = 0,
    FAULTSEVERITY_DEGRADED = 1,
    FAULTSEVERITY_CRITICAL = 2,
    FAULTSEVERITY_FAILURE = 3,
};

typedef enum
{
    CDI_UNSPECIFIED,
    CDI_LOCAL_DEGRADED,
    CDI_REMOTE_DEGRADED,
    CDI_NONE,
    CDI_UNKNOWN,
    CDI_LOCAL_AND_REMOTE_DEGRADED,
    CDI_FEC_SIGNAL_DEGRADED
} Cdi;

typedef struct
{
    string faultKey;
    string faultName;
    FaultSeverity severity;
    uint32_t bitPos;
    string faultDescription;
    bool serviceAffecting;
    FaultDirection direction;
    FaultLocation location;
} FaultCapability;

typedef struct
{
    string faultName;
    bool faulted;
    FaultDirection direction;
    FaultLocation location;
} AdapterFault;

typedef struct
{
    string cdiKey;
    string cdiName;
    FaultSeverity severity;
    uint32_t bitPos;
    string faultDescription;
    bool serviceAffecting;
    FaultDirection direction;
    FaultLocation location;
    Cdi cdiEnum;
} CdiCapability;

/*
   isNotified is a flag to mark each obj's fault is been updated by notification or not
   if certain object fault is already been updated by notification, this flag will be marked to true
   so that during creation, fault retrieval will be skipped(to solve get/notify timing issue)
   When dpms came up or when grpc connection is re-established, this flag will be refreshed to false
*/
struct AdapterCacheFault
{
    uint64_t faultBitmap;
    bool isNotified;
    std::time_t forceUpdateTime;
    AdapterCacheFault() : faultBitmap(0), isNotified(false), forceUpdateTime(0) {}
    void ClearFaultCache()
    {
        faultBitmap = 0;
        isNotified = false;
    }
};

struct AdapterCacheTTI
{
    uint8_t ttiRx[MAX_TTI_LENGTH];
    bool isNotified;
    uint32_t notifyCount;
    AdapterCacheTTI() : ttiRx(), isNotified(false), notifyCount(0) {}
    void ClearTTICache()
    {
        memset(ttiRx, 0, sizeof(ttiRx));
        isNotified = false;
        notifyCount = 0;
    }
};

struct AdapterCacheOpStatus
{
    DataPlane::OpStatus opStatus;
    uint64_t transactionId;
    AdapterCacheOpStatus() :opStatus(OPSTATUS_UNSPECIFIED), transactionId(0) {}
    void ClearOpStatusCache()
    {
        opStatus = OPSTATUS_UNSPECIFIED;
        transactionId = 0;
    }
    bool waitForDcoAck(uint64_t transId);
};

class IdGen
{
public:

    static std::string genLineAid(uint32 lineId);
    static std::string genOtuAid(uint32 otuId);
    static std::string genOduAid(uint32 oduId);
    static std::string genOduAid(uint32 otuId, const std::vector<uint32_t>& tS, OduPayLoad payLoad);
    static std::string genOduAid(uint32 oduId, uint32 otuId, OduPayLoad payLoad);
    static std::string genGigeAid(uint32 portId);
    static std::string genXconAid(uint32 loOduId, uint32 portId);
    static std::string genXconAid(std::string oduAid, uint32 portId, XconDirection dir);

    static const string cOdu4AidStr;
    static const string cOdu4iAidStr;
    static const string cOduFlexAidStr;
    static const uint32 cOffOdu4i_1    = 0;
    static const uint32 cOffOduFlex_1  = 24; //32;
    static const uint32 cOffOdu4i_2    = 16;
    static const uint32 cOffOduFlex_2  = 26; //34;
};

class EnumTranslate
{
public:
    static std::string toString(const bool);
    static std::string toString(const DataPlane::OtuSubType);
    static std::string toString(const DataPlane::OtuPayLoad);
    static std::string toString(const DataPlane::OduSubType);
    static std::string toString(const DataPlane::OduPayLoad);
    static std::string toString(const DataPlane::OduMsiPayLoad);
    static std::string toString(const DataPlane::LoopBackType);
    static std::string toString(const DataPlane::CarrierChannel);
    static std::string toString(const DataPlane::ServiceMode);
    static std::string toString(const DataPlane::InterfaceType);
    static std::string toString(const DataPlane::MaintenanceSignal);
    static std::string toString(const DataPlane::PrbsMode);
    static std::string toString(const DataPlane::Direction);
    static std::string toString(const DataPlane::EthernetMode);
    static std::string toString(const DataPlane::ServiceModeQualifier);
    static std::string toString(const DataPlane::PortRate);
    static std::string toString(const DataPlane::ClientStatus);
    static std::string toString(const DataPlane::CarrierState);
    static std::string toString(const DataPlane::ClientMode);
    static std::string toString(const DataPlane::CarrierAdvParmStatus);
    static std::string toString(const DataPlane::DcoState);
    static std::string toString(const DataPlane::FwUpgradeState);
    static std::string toString(const DataPlane::BootState);
    static std::string toString(const DataPlane::XconPayLoadType payload);
    static std::string toString(const DataPlane::XconPayLoadTreatment ptr);
    static std::string toString(const DataPlane::XconDirection dir);
    static std::string toString(const DataPlane::OpStatus);
};

// Serdes Lane configuration
typedef struct
{
    bool txPolarity;
    bool rxPolarity;
    int  txPre2;
    int  txPre1;
    int  txMain;
    int  txPost1;
    int  txPost2;
    int  txPost3;
} ClientSerdesCfg;

inline void getDecimal(double& val, const int fraction)
{
     std::ostringstream decout;
     decout.precision(fraction);
     decout << std::fixed << val << endl;
     val = stod(decout.str());
}

// Get time stamp in nanoseconds.
inline uint64_t getTimeNanos()
{
    uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    return ns;
}

} /* DpAdapter */
#endif // ADATER_COMMON_H
