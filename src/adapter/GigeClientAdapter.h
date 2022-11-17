/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef GIGE_CLIENT_ADAPTER_H
#define GIGE_CLIENT_ADAPTER_H

#include "dp_common.h"
#include "AdapterCommon.h"
#include <string>


using namespace std;
using namespace DataPlane;

namespace DpAdapter {

typedef struct
{
    bool     enable;
    double   activateThreshold;
    double   deactivateThreshold;
    uint32_t monitorPeriod;
} FecDegSerCfg;


struct GigeCommon
{
    string name;     // AID
    uint32_t clientSerdesLane[8];
    ClientSerdesCfg serdes[8];
    uint32_t mtu;
    bool lldpTxDrop;
    bool lldpRxDrop;
    bool lldpTxSnoop;
    bool lldpRxSnoop;
    bool fwdTdaTrigger;
    bool fecEnable;
    EthernetMode ethMode;
    EthernetFlexType fType;
    InterfaceType intfType;
    ServiceMode srvMode;
    ServiceModeQualifier srvQMode;
    DataPlane::LoopBackType lpbk;
    DataPlane::PortRate rate;
    DataPlane::MaintenanceSignal forceMsTx;
    DataPlane::MaintenanceSignal forceMsRx;
    DataPlane::MaintenanceSignal autoMsTx;
    DataPlane::MaintenanceSignal autoMsRx;
    FecDegSerCfg fecDegSer;
    bool generatorTx;
    bool generatorRx;
    bool monitorRx;
    bool regen;
    uint64_t transactionId;
    GigeCommon() : name(""), mtu(0), lldpTxDrop(false), lldpRxDrop(false), lldpTxSnoop(false), lldpRxSnoop(false),
            fwdTdaTrigger(false), fecEnable(false), ethMode(ETHERNETMODE_UNSPECIFIED), fType(ETHERNETFLEXTYPE_UNSPECIFIED),
            intfType(INTERFACETYPE_UNSPECIFIED), srvMode(SERVICEMODE_UNSPECIFIED), srvQMode(SERVICEMODEQUALIFIER_UNSPECIFIED),
            lpbk(LOOP_BACK_TYPE_UNSPECIFIED), rate(PORT_RATE_UNSET), forceMsTx(MAINTENANCE_SIGNAL_UNSPECIFIED),
            forceMsRx(MAINTENANCE_SIGNAL_UNSPECIFIED), autoMsTx(MAINTENANCE_SIGNAL_UNSPECIFIED),
            autoMsRx(MAINTENANCE_SIGNAL_UNSPECIFIED), generatorTx(false), generatorRx(false), monitorRx(false),
            regen(false), transactionId(0)
    {
        memset(clientSerdesLane, 0, sizeof(clientSerdesLane));
        memset(serdes, 0, sizeof(serdes));
        memset(&fecDegSer, 0, sizeof(fecDegSer));
    }
};

typedef struct
{
    GigeCommon eth;
    uint32_t maxMtu;
    ClientStatus status;
    uint64_t facFault;
} GigeStatus;

typedef struct
{
    std::vector<AdapterFault> fac;
    uint64_t facBmp;    // Faults bitmap
} GigeFault;

// GigeClient Adapter Base class
class GigeClientAdapter
{
public:
    GigeClientAdapter() {}
    virtual ~GigeClientAdapter() {}

    virtual bool createGigeClient(string aid, GigeCommon *cfg) = 0;
    virtual bool createGigeClientXpress(string aid, DataPlane::PortRate portRate) = 0;
    virtual bool deleteGigeClient(string aid) = 0;
    virtual bool initializeGigeClient() = 0;

    virtual bool setGigeMtu(string aid, int mtu) = 0;
    virtual bool setGigeFecEnable(string aid, bool enable) = 0;
    virtual bool setGigeLldpDrop(string aid, DataPlane::Direction dir, bool enable) = 0;
    virtual bool setGigeLldpSnoop(string aid, DataPlane::Direction dir, bool enable) = 0;
    virtual bool setGigeLoopBack(string aid, DataPlane::LoopBackType mode) = 0;
    virtual bool setGigeAutoMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode) = 0;
    virtual bool setGigeForceMS(string aid, DataPlane::Direction dir, DataPlane::MaintenanceSignal mode) = 0;
    virtual bool setGigeTestSignalGen(string aid, Direction dir, bool mode, DataPlane::MaintenanceSignal cacheOverrideForceMs) = 0;
    virtual bool setGigeFwdTdaTrigger(string aid, bool enable) = 0;

    virtual bool getGigeConfig(string aid, GigeCommon *cfg) = 0;
    virtual bool getGigeStatus(string aid, GigeStatus *stat) = 0;
    virtual bool getGigeMaxMtu(string aid, int *maxMtu) = 0;
    virtual bool getGigeLinkStatus(string aid, ClientStatus *linkStat) = 0;
    virtual bool getGigeLldpPkt(string aid, DataPlane::Direction dir, uint8_t *ptk) = 0;
    virtual bool getGigeFault(string aid, uint64_t *fault) = 0;
    virtual bool getGigeFault(string aid, GigeFault *faults, bool force = false) = 0;
    virtual bool getGigePm(string aid) = 0;
    virtual bool setTdaHoldOff(string aid, bool enable, bool updateDco, DataPlane::MaintenanceSignal mode) = 0;

    virtual bool gigeConfigInfo(ostream& out, string aid) = 0;
    virtual bool gigeStatusInfo(ostream& out, string aid) = 0;
    virtual bool gigeFaultInfo(ostream& out, string aid) = 0;
};

}
#endif // GIGE_CLIENT_ADAPTER_H
