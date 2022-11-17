/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include <ostream>

#include "DpStateCollector.h"

using namespace std;

namespace DataPlane
{

void DpStateCollector::setStatePollMode(bool mode, std::ostream& os)
{
    mCollectStatePollMode = mode;
}

void DpStateCollector::setFaultPollMode(bool mode, std::ostream& os)
{
    mCollectFaultPollMode = mode;
}

void DpStateCollector::setStatePollInterval(uint32_t intvl, std::ostream& os)
{
    mCollectStateInterval = intvl;
}

void DpStateCollector::setFaultPollInterval(uint32_t intvl, std::ostream& os)
{
    mCollectFaultInterval = intvl;
}

void DpStateCollector::dumpPollInfo(std::ostream& os)
{
    os << "State Polling    : " << (true == mCollectStatePollMode ? "Enabled" : "Disabled") << endl;
    os << "State Interval   : " << mCollectStateInterval << endl;

    os << "Fault Polling    : " << (true == mCollectFaultPollMode ? "Enabled" : "Disabled") << endl;
    os << "Fault Interval   : " << mCollectFaultInterval << endl;

    os << "Collection Enable: " << (true == mIsCollectionEn ? "Enabled" : "Disabled") << endl << endl;
}

}// namespace dataplane
