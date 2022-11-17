/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef ODU_ADAPTER_H
#define ODU_ADAPTER_H

#include "dp_common.h"
#include "AdapterCommon.h"
#include <string>


using namespace std;
using namespace DataPlane;

namespace DpAdapter {

struct OduCommon
{
    string name;        // AID
    uint32_t hoOtuId;   // Point which otu it belong to.
    uint32_t loOduId;   // Use this loOduId instead of parsing AID for ID
    OduSubType type;   // Line or client
    OduPayLoad payLoad;
    OduMsiPayLoad msiPayLoad;
    ServiceMode srvMode;
    DataPlane::LoopBackType lpbk;
    PrbsMode monitorTx;
    PrbsMode monitorRx;
    PrbsMode generatorTx;
    PrbsMode generatorRx;
    uint8_t  ttiTx[MAX_TTI_LENGTH];
    DataPlane::MaintenanceSignal forceMsTx;
    DataPlane::MaintenanceSignal forceMsRx;
    DataPlane::MaintenanceSignal autoMsTx;
    DataPlane::MaintenanceSignal autoMsRx;
    TsGranularity tsg;   // Time slot granularity
    std::vector<uint32_t> tS;  // Time slot for this LO-ODUi
    uint32_t injectRxFaultStatus;  // To inject Rx Fault
    bool regen;  // Xpress Xcon
    uint64_t transactionId;
    OduCommon() : name(""), hoOtuId(0), loOduId(0),
        type (ODUSUBTYPE_UNSPECIFIED), payLoad(ODUPAYLOAD_UNSPECIFIED),
        msiPayLoad(ODUMSIPAYLOAD_UNALLOCATED),
        srvMode (SERVICEMODE_UNSPECIFIED), lpbk (LOOP_BACK_TYPE_UNSPECIFIED),
        monitorTx (PRBSMODE_UNSPECIFIED), monitorRx (PRBSMODE_UNSPECIFIED),
        generatorTx (PRBSMODE_UNSPECIFIED),
        generatorRx (PRBSMODE_UNSPECIFIED),
        forceMsTx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        forceMsRx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        autoMsTx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        autoMsRx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        tsg (TSGRANULARITY_UNSPECIFIED), injectRxFaultStatus (0),
        regen(false), transactionId(0)
    {
        memset(ttiTx, 0, sizeof(ttiTx));
    }
};

typedef struct
{
    OduCommon odu;
    double rate;
    uint8_t  ttiRx[MAX_TTI_LENGTH];
    uint32_t  freeTs[MAX_25G_TIME_SLOT];  // Free ts for HO-ODU
    uint64_t faultTx;
    uint64_t faultRx;
    uint8_t cdiMask;
} OduStatus;

// TODO ODU fault still being finalize they may go back to TX/RX faults
typedef struct
{
    std::vector<FaultCapability> fac;
} OduFaultCapability;

typedef struct
{
    std::vector<AdapterFault> fac;
    uint64_t facBmp;
} OduFault;

typedef struct
{
    std::vector<CdiCapability> cdiCapa;
} OduCdiCapability;


// OduAdapter Base class
class OduAdapter
{
public:
    OduAdapter() {}
    virtual ~OduAdapter() {}

    virtual bool createOduClientXpress(string aid) = 0;
    virtual bool createOdu(string aid, OduCommon *cfg) = 0;
    virtual bool deleteOdu(string aid) = 0;
    virtual bool initializeOdu() = 0;

    virtual bool setOduLoopBack(string aid, DataPlane::LoopBackType mode) = 0;
    virtual bool setOduTti(string aid, uint8_t *tti) = 0;
    virtual bool setOduAutoMS(string aid, Direction dir, DataPlane::MaintenanceSignal mode) = 0;
    virtual bool setOduForceMS(string aid, Direction dir, DataPlane::MaintenanceSignal mode) = 0;
    virtual bool setOduPrbsMon(string aid, Direction dir, PrbsMode mode) = 0;
    virtual bool setOduPrbsGen(string aid, Direction dir, PrbsMode mode) = 0;
    virtual bool setOduTimeSlot(string aid, std::vector<uint32_t> tS) = 0;
    virtual bool setOduFaultStatus(string aid, uint64_t injectFaultBitmap) = 0;
    virtual bool setOduTimBdi(string aid, bool bInject) = 0;
    virtual bool setOduRegen(string aid, bool bRegen) = 0;

    virtual bool getOduConfig(string aid, OduCommon *cfg) = 0;
    virtual bool getOduStatus(string aid, OduStatus *stat) = 0;
    virtual bool getOduFaultRx(string aid, uint64_t *fault) = 0;
    virtual bool getOduFaultTx(string aid, uint64_t *fault) = 0;
    virtual bool getOduFault(string aid, OduFault *faults, bool force = false) = 0;
    virtual bool getOduPm(string aid) = 0;
    virtual uint32_t getOduFrameRate(string aid) = 0;
    virtual uint64_t getOduPmErrorBlocks(string aid, DataPlane::FaultDirection direction) = 0;

    virtual bool oduConfigInfo(ostream& out, string aid) = 0;
    virtual bool oduStatusInfo(ostream& out, string aid) = 0;
    virtual bool oduFaultInfo(ostream& out, string aid) = 0;
    virtual bool oduCdiInfo(ostream& out, string aid) = 0;
};

}
#endif // ODU_ADAPTER_H
