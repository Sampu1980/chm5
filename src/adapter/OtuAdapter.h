/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef OTU_ADAPTER_H
#define OTU_ADAPTER_H

#include "dp_common.h"
#include "AdapterCommon.h"
#include <string>


using namespace std;
using namespace DataPlane;

namespace DpAdapter {

struct OtuCommon
{
    vector<uint32_t> clientSerdesLane;  // for OTU4 Client
    vector<ClientSerdesCfg> serdesCfg;  // for OTU4 Client
    string name;  // for AID
    bool fecGenEnable;
    bool fecCorrEnable;
    DataPlane::LoopBackType lpbk;
    CarrierChannel channel;
    ServiceMode srvMode;
    InterfaceType intfType;
    PrbsMode monitorTx;
    PrbsMode monitorRx;
    PrbsMode generatorTx;
    PrbsMode generatorRx;
    uint8_t  ttiTx[MAX_TTI_LENGTH];
    DataPlane::MaintenanceSignal forceMsTx;
    DataPlane::MaintenanceSignal forceMsRx;
    DataPlane::MaintenanceSignal autoMsTx;
    DataPlane::MaintenanceSignal autoMsRx;
    OtuSubType type;   // Line or client
    OtuPayLoad payLoad;
    uint64_t injectTxFaultStatus;
    uint64_t injectRxFaultStatus;
    bool regen;
    uint64_t transactionId;
    OtuCommon() : name(""), fecGenEnable(false), fecCorrEnable(false),
        lpbk(LOOP_BACK_TYPE_UNSPECIFIED), channel(CARRIERCHANNEL_UNSPECIFIED),
        srvMode (SERVICEMODE_UNSPECIFIED),
        intfType(INTERFACETYPE_UNSPECIFIED),
        generatorTx(PRBSMODE_PRBS_NONE), generatorRx(PRBSMODE_PRBS_NONE),
        monitorTx(PRBSMODE_PRBS_NONE), monitorRx(PRBSMODE_PRBS_NONE),
        forceMsTx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        forceMsRx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        autoMsTx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        autoMsRx (MAINTENANCE_SIGNAL_UNSPECIFIED),
        type (OTUSUBTYPE_UNSPECIFIED), payLoad(OTUPAYLOAD_UNSPECIFIED),
        injectTxFaultStatus (0), injectRxFaultStatus (0),
        regen(false), transactionId(0)
    {
        memset(ttiTx, 0, sizeof(ttiTx));
    }
};

typedef struct
{
    OtuCommon otu;
    uint8_t  ttiRx[MAX_TTI_LENGTH];
    double rate;
    double delayMeasurement;
    uint64_t faultTx;       // Obsolete to be remove
    uint64_t faultRx;       // Obsolete to be remove
    uint64_t faultFac;
} OtuStatus;

typedef struct
{
    std::vector<FaultCapability> fac;
} OtuFaultCapability;

typedef struct
{
    std::vector<AdapterFault> fac;
    uint64_t facBmp;
} OtuFault;

// OtuAdapter Base class
class OtuAdapter
{
public:
    OtuAdapter() {}
    virtual ~OtuAdapter() {}

    virtual bool createOtuClientXpress(string aid) = 0;
    virtual bool createOtu(string aid, OtuCommon *cfg) = 0;
    virtual bool deleteOtu(string aid) = 0;
    virtual bool initializeOtu() = 0;

    virtual bool setOtuLoopBack(string aid, DataPlane::LoopBackType mode) = 0;
    virtual bool setOtuTti(string aid, uint8_t *tti) = 0;
    virtual bool setOtuAutoMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode) = 0;
    virtual bool setOtuForceMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode) = 0;
    virtual bool setOtuPrbsMon(string aid, DataPlane::Direction dir, PrbsMode mode) = 0;
    virtual bool setOtuPrbsGen(string aid, DataPlane::Direction dir, PrbsMode mode) = 0;
    virtual bool setOtuFaultStatus(string aid, DataPlane::Direction dir, uint64_t injectFaultBitmap) = 0;
    virtual bool setOtuTimBdi(string aid, bool bInject) = 0;

    virtual bool getOtuConfig(string aid, OtuCommon *cfg) = 0;
    virtual bool getOtuStatus(string aid, OtuStatus *stat) = 0;
    virtual bool getOtuFaultRx(string aid, uint64_t *fault) = 0;
    virtual bool getOtuFaultTx(string aid, uint64_t *fault) = 0;
    virtual bool getOtuFault(string aid, OtuFault *faults, bool force = false) = 0;
    virtual bool getOtuPm(string aid) = 0;
    virtual uint32_t getOtuFrameRate(string aid) = 0;
    virtual uint64_t getOtuPmErrorBlocks(string aid) = 0;
    virtual bool getOtuFecCorrEnable(string aid) = 0;

    virtual bool otuConfigInfo(ostream& out, string aid) = 0;
    virtual bool otuStatusInfo(ostream& out, string aid) = 0;
    virtual bool otuFaultInfo(ostream& out, string aid) = 0;
};

}
#endif // OTU_ADAPTER_H
