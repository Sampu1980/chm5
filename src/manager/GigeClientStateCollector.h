/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef GIGE_CLIENT_STATE_COLLECTOR_H
#define GIGE_CLIENT_STATE_COLLECTOR_H

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "GigeClientPbTranslator.h"
#include "DcoGigeClientAdapter.h"

#include "GearBoxAdapter.h"
#include "GearBoxAdapterSim.h"
#include "BcmPm.h"

#include "DpProtoDefs.h"
#include "DpStateCollector.h"


namespace DataPlane {

class GigeClientStateCollector : public DpStateCollector
{
public:

    GigeClientStateCollector(GigeClientPbTranslator* pbTrans, DpAdapter::DcoGigeClientAdapter* pAd, gearbox::GearBoxAdIf* pGearBoxAd);
    ~GigeClientStateCollector() {}

    void start();

    void createGigeClient(const chm6_dataplane::Chm6GigeClientConfig& inCfg);
    void deleteGigeClient(std::string gigeClientAid);

    void createFaultPmPlaceHolder(std::string gigeClientAid);

    virtual void dumpCacheState(std::string gigeAid, std::ostream& os);
    virtual void listCacheState(std::ostream& os);
    void dumpCachePm   (std::string gigeAid, std::ostream& os);
    void dumpCacheFault(std::string gigeAid, std::ostream& os);
    void setLldpPollMode (bool mode, std::ostream& os);
    void setLldpPollInterval (uint32_t intvl, std::ostream& os);
    virtual void dumpPollInfo(std::ostream& os);
    void dumpLldpCounters(std::ostream& os);
    void getGigeClientGearBoxPm(std::string gigeAid, gearbox::BcmPm& gigeGearBoxPm);

    virtual void setIsConnectionUp(bool val);

    virtual void updateInstalledConfig();
    void updateInstalledConfig(const chm6_dataplane::Chm6GigeClientConfig& inCfg);

    int getCacheState(std::string configId, chm6_dataplane::Chm6GigeClientState*& pMsg);

    uint getInstalledGigeClients(void) { return mInstalledGigeClients; }

    bool checkBootUpTraffic(void);

private:

    void collectGigeClientFaults();
    void collectGigeClientPm();
    void collectGigeClientLldp();

    void getAndUpdateGigeClientFaults();
    void getAndUpdateGigeClientPm();
    void getAndUpdateGigeClientLldp();

    void findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6GigeClientState*& pState);

    boost::thread mThrFaults;
    boost::thread mThrPm;
    boost::thread mThrLldp;

    int mCollectPmInterval;
    int mCollectLldpInterval;

    int mCollectLldpPollMode;

    std::mutex mGigeDataMtx;

    typedef std::map<std::string, chm6_dataplane::Chm6GigeClientState*> TypeMapGigeClientState;
    TypeMapGigeClientState mMapGigeState;

    typedef std::map<std::string, chm6_dataplane::Chm6GigeClientFault*> TypeMapGigeClientFault;
    TypeMapGigeClientFault mMapGigeFault;

    typedef std::map<std::string, uint64_t> TypeMapGigeClientFaultBitmap;
    TypeMapGigeClientFaultBitmap mMapGigeFacFaultBitmap;

    typedef std::map<std::string, gearbox::BcmPm*> TypeMapGigeGearBoxPm;
    TypeMapGigeGearBoxPm mMapGigeGearBoxPm;

    GigeClientPbTranslator*          mpPbTranslator;
    DpAdapter::DcoGigeClientAdapter* mpDcoAd;
    gearbox::GearBoxAdIf*            mpGearBoxAd;

    void createLldpStatePlaceHolder();

    static const int MAX_GIGECLIENT_PORT = 16;
    uint32_t mLldpRxCnt[MAX_GIGECLIENT_PORT];
    uint32_t mLldpTxCnt[MAX_GIGECLIENT_PORT];

    // TOM Serdes
    uint32_t mInstalledGigeClients;
};


} // namespace DataPlane

#endif /* GIGE_CLIENT_STATE_COLLECTOR_H */
