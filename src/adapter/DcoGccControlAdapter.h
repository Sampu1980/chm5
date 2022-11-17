
/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef DCO_GCC_CONTROL_ADAPTER_H
#define DCO_GCC_CONTROL_ADAPTER_H

#include <mutex>
#include <SimCrudService.h>

#include "dcoyang/infinera_dco_gcc_control/infinera_dco_gcc_control.pb.h"
#include "dcoyang/infinera_dco_gcc_control_fault/infinera_dco_gcc_control_fault.pb.h"
#include "GccControlAdapter.h"
#include "CrudService.h"
#include "DpPm.h"

using namespace GnmiClient;
//using namespace DataPlane;

using gccControl = dcoyang::infinera_dco_gcc_control::DcoGccControl;
using GccVlanState = dcoyang::infinera_dco_gcc_control::DcoGccControl_State_VlanTable;
using GccVlanConfig = dcoyang::infinera_dco_gcc_control::DcoGccControl_Config_VlanTable;
using GccState = dcoyang::infinera_dco_gcc_control::DcoGccControl_State;

namespace DpAdapter {

//Gcc Control Devired Class
class DcoGccControlAdapter : public GccControlAdapter
{
public:
	DcoGccControlAdapter();
	virtual ~DcoGccControlAdapter();

    virtual bool initializeCcVlan();
    virtual bool createCcVlan(ControlChannelVlanIdx vIdx, ControlChannelVlanCommon *cfg);
    virtual bool deleteCcVlan(ControlChannelVlanIdx vIdx);
    virtual bool enableCcVlan(ControlChannelVlanIdx vIdx, bool enable);

    virtual bool deleteCcVlanTxVlan(ControlChannelVlanIdx vIdx);
    virtual bool deleteCcVlanTxVlanTag(ControlChannelVlanIdx vIdx);
    virtual bool deleteCcVlanTxVlanReTag(ControlChannelVlanIdx vIdx);

    virtual bool getCcVlanConfig(ControlChannelVlanIdx vIdx, ControlChannelVlanCommon *cfg);
    virtual bool getCcVlanStatus(ControlChannelVlanIdx vIdx, ControlChannelVlanStatus *stat);
    virtual bool getCcVlanFault(ControlChannelVlanIdx vIdx, ControlChannelFault *faults, bool force = false);

    virtual bool ccVlanConfigInfo(ostream& out, ControlChannelVlanIdx vIdx);
    virtual bool ccVlanStatusInfo(ostream& out, ControlChannelVlanIdx vIdx);

	virtual bool setLldpVlan();
	virtual void subGccPm();

    virtual bool gccPmInfo(std::ostream& out, int vIdx);
    virtual bool gccPmAccumInfo(std::ostream& out, int vIdx);
    virtual bool gccPmAccumClear(std::ostream& out, int vIdx);
    virtual void gccPmTimeInfo(ostream& out);

	bool getLldp(gccControl* gcc);
	bool getLldpVlanConfig(ostream& out);
	bool getLldpVlanState(ostream& out);

	void setSimModeEn(bool enable);
	void setSimPmData();

	void dumpAll(std::ostream& out);
	void dumpConfig(std::ostream& out, ControlChannelVlanCommon cmn);
	void dumpStatus(std::ostream& out, ControlChannelVlanStatus stat);

	void translateConfig(const GccVlanConfig& dcoCfg, ControlChannelVlanCommon& cmn);
	void translateStatus(const GccState& dcoStat, int idx, ControlChannelVlanStatus& stat);

	static DataPlane::DpMsGCCPm mGccPms;
	static high_resolution_clock::time_point mStart;
	static high_resolution_clock::time_point mEnd;
	static int64_t mStartSpan;
	static int64_t mEndSpan;
	static int64_t mDelta;
	static deque<int64_t> mStartSpanArray;
	static deque<int64_t> mEndSpanArray;
	static deque<int64_t>  mDeltaArray;

private:

    bool setKey (gccControl *gcc, ControlChannelVlanIdx vIdx);
    bool getCcVlanObj( gccControl *gcc, ControlChannelVlanIdx vIdx, int *tIdx, bool configObj );
	uint64_t getLldpDstMacAddr();

    std::recursive_mutex       mCrudPtrMtx;
    bool                       mIsGnmiSimModeEn;
    std::thread                mSimGccPmThread;
    std::shared_ptr<::DpSim::SimSbGnmiClient>   mspSimCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspSbCrud;
    std::shared_ptr<::GnmiClient::SBGnmiClient> mspCrud;

};

}
#endif // DCO_GCC_CONTROL_ADAPTER_H
