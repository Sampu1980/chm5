/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef XC_STATE_COLLECTOR_H
#define XC_STATE_COLLECTOR_H

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "XcPbTranslator.h"
#include "DcoXconAdapter.h"
#include "DpProtoDefs.h"
#include "DpStateCollector.h"


namespace DataPlane {

class XCStateCollector : public DpStateCollector
{
public:

    XCStateCollector(XcPbTranslator* pbTrans,
                     DpAdapter::DcoXconAdapter *pAd);

    ~XCStateCollector() {}

    //void start();

    void createXcon(const chm6_dataplane::Chm6XCConfig& inCfg);
    void deleteXcon(std::string xcAid);

    void updateConfigCache(const std::string xcAid, const chm6_dataplane::Chm6XCConfig* pCfg);

    virtual void dumpCacheState(std::string xcAid, std::ostream& os);
    virtual void listCacheState(std::ostream& os);

    virtual void updateInstalledConfig();
    void updateInstalledConfig(const chm6_dataplane::Chm6XCConfig& inCfg);

    int getCacheState(std::string configId, chm6_dataplane::Chm6XCState*& pMsg);

    virtual bool isCacheExist(std::string configId);

private:

    void findOrCreateStateObj(std::string stateId, chm6_dataplane::Chm6XCState*& pState);

    boost::thread mThrState;
    boost::thread mThrFaults;

    std::recursive_mutex mXcDataMtx;
    typedef std::map<std::string, chm6_dataplane::Chm6XCState*> TypeMapXcState;
    TypeMapXcState mMapXcState;

    XcPbTranslator            *mpPbTranslator;
    DpAdapter::DcoXconAdapter *mpDcoAd;
};


} // namespace DataPlane

#endif /* XC_STATE_COLLECTOR_H */
