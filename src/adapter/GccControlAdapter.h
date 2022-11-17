/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef GIGE_GCC_CONTROL_H
#define GIGE_GCC_CONTROL_H

#include "dp_common.h"
#include "AdapterCommon.h"
#include <string>


using namespace std;
using namespace DataPlane;

namespace DpAdapter {

typedef struct
{
	uint64_t srcMac;
	uint64_t dstMac;
	uint64_t ethType;
	uint32_t vlanTag;
}LldpVlanInfo;

typedef struct
{
    ControlChannelCarrier carrier;
    ControlChannelType    type;
    bool                  enable;
    uint32_t              rxVlanTag;
    uint32_t              rxVlanReTag;
    uint32_t              txVlanTag;
    uint32_t              txVlanReTag;
    bool                  deleteTxTags;
} ControlChannelVlanCommon;

typedef struct
{
    ControlChannelVlanCommon    cc;
    uint32_t                    fpgaVersion;
    uint64_t                    facFault;
} ControlChannelVlanStatus;

typedef struct
{
    std::vector<AdapterFault> fac;
    uint64_t facBmp;    // Faults bitmap
} ControlChannelFault;

//Gcc Control Devired Class
class GccControlAdapter
{
public:
	GccControlAdapter() {}
	virtual ~GccControlAdapter() {}

    virtual bool initializeCcVlan() = 0;
    virtual bool createCcVlan(ControlChannelVlanIdx vIdx, ControlChannelVlanCommon *cfg) = 0;
    virtual bool deleteCcVlan(ControlChannelVlanIdx vIdx) = 0;
    virtual bool enableCcVlan(ControlChannelVlanIdx vIdx, bool enable) = 0;

    virtual bool deleteCcVlanTxVlan(ControlChannelVlanIdx vIdx) = 0;
    virtual bool deleteCcVlanTxVlanTag(ControlChannelVlanIdx vIdx) = 0;
    virtual bool deleteCcVlanTxVlanReTag(ControlChannelVlanIdx vIdx) = 0;

    virtual bool getCcVlanConfig(ControlChannelVlanIdx vIdx, ControlChannelVlanCommon *cfg) = 0;
    virtual bool getCcVlanStatus(ControlChannelVlanIdx vIdx, ControlChannelVlanStatus *stat) = 0;
    virtual bool getCcVlanFault(ControlChannelVlanIdx vIdx, ControlChannelFault *faults, bool force = false) = 0;

    virtual bool ccVlanConfigInfo(ostream& out, ControlChannelVlanIdx vIdx) = 0;
    virtual bool ccVlanStatusInfo(ostream& out, ControlChannelVlanIdx vIdx) = 0;

	virtual bool setLldpVlan() = 0;
	virtual bool getLldpVlanConfig(ostream& out) = 0;
	virtual bool getLldpVlanState(ostream& out) = 0;

};

}
#endif // GIGE_GCC_CONTROL_H
