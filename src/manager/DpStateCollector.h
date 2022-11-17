/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_STATE_COLLECTOR_H
#define DP_STATE_COLLECTOR_H

#include "DpProtoDefs.h"

using STATUS = chm6_common::Status;

namespace DataPlane
{

class DpStateCollector
{
public:
    DpStateCollector(int stateInterval, int faultInterval)
        : mIsConnectionUp(false)
        , mIsCollectionEn(false)
        , mCollectStateInterval(stateInterval)
        , mCollectFaultInterval(faultInterval)
        , mCollectStatePollMode(true)
        , mCollectFaultPollMode(true)
    {}
    virtual ~DpStateCollector() {}

    virtual void setIsConnectionUp(bool val) { mIsConnectionUp = val; }
    virtual void setIsCollectionEn(bool val) { mIsCollectionEn = val; }
    bool getIsCollectionEn() { return mIsCollectionEn; }

    virtual void setStatePollMode(bool mode, std::ostream& os);
    virtual void setFaultPollMode(bool mode, std::ostream& os);
    virtual void setStatePollInterval(uint32_t intvl, std::ostream& os);
    virtual void setFaultPollInterval(uint32_t intvl, std::ostream& os);
    virtual void dumpPollInfo(std::ostream& os);

    virtual void updateInstalledConfig() {}

    virtual void dumpCacheState(std::string stateId, std::ostream& os) {}
    virtual void listCacheState(std::ostream& out) {}


protected:

    bool mIsConnectionUp;
    bool mIsCollectionEn;

    int mCollectStateInterval;
    int mCollectFaultInterval;

    int mCollectStatePollMode;
    int mCollectFaultPollMode;

};

} // namespace DataPlane

#endif /* DP_STATE_COLLECTOR_H */
