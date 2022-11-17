#ifndef DP_PM_HANDLER_H
#define DP_PM_HANDLER_H

#include <iostream>
#include <DpPm.h>

namespace DataPlane
{

class DpmsCardCbHandler : public DpmsCbHandler {
public:
	bool OnRcvPm(string className, DcoBasePM &basePm);
};

class DpmsCarrierCbHandler : public DpmsCbHandler {
public:
	bool OnRcvPm(string className, DcoBasePM &basePm);
};

class DpmsGigeCbHandler : public DpmsCbHandler {
public:
	bool OnRcvPm(string className, DcoBasePM &basePm);
};

class DpmsOtuCbHandler : public DpmsCbHandler {
public:
	bool OnRcvPm(string className, DcoBasePM &basePm);
};

class DpmsOduCbHandler : public DpmsCbHandler {
public:
	bool OnRcvPm(string className, DcoBasePM &basePm);
};

class DpmsGccCbHandler : public DpmsCbHandler {
public:
	bool OnRcvPm(string className, DcoBasePM &basePm);
};


} // namespace DataPlane


#endif /* DP_PM_HANDLER_H */
