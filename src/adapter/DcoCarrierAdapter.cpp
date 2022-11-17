
/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>      // std::setprecision

#include <google/protobuf/text_format.h>
#include "DcoCarrierAdapter.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "DcoPmSBAdapter.h"
#include "DcoConnectHandler.h"

using namespace std;
using std::make_shared;
using std::make_unique;

using namespace chrono;
using namespace google::protobuf;
using namespace GnmiClient;
using namespace DataPlane;
using namespace dcoyang::infinera_dco_carrier;
using namespace dcoyang::infinera_dco_carrier_fault;

using os = ::dcoyang::enums::InfineraDcoCarrierOpticalState;
using carrierState = dcoyang::infinera_dco_carrier::Carriers_Carrier_State;

namespace DpAdapter {

#define  BAD_APJ_WA     1;

std::recursive_mutex DcoCarrierAdapter::mMutex;
DpMsCarrierPMs DcoCarrierAdapter::mCarrierPms;
AdapterCacheFault DcoCarrierAdapter::mCarrierEqptFault[MAX_CARRIERS];
AdapterCacheFault DcoCarrierAdapter::mCarrierFacFault[MAX_CARRIERS];
DataPlane::CarrierState DcoCarrierAdapter::mTxState[MAX_CARRIERS];
DataPlane::CarrierState DcoCarrierAdapter::mRxState[MAX_CARRIERS];
double    DcoCarrierAdapter::mDataRate[MAX_CARRIERS+1]; // one based
double    DcoCarrierAdapter::mTxActualFrequency[MAX_CARRIERS];
double    DcoCarrierAdapter::mRxActualFrequency[MAX_CARRIERS];

high_resolution_clock::time_point DcoCarrierAdapter::mStart;
high_resolution_clock::time_point DcoCarrierAdapter::mEnd;
int64_t DcoCarrierAdapter::mStartSpan;
int64_t DcoCarrierAdapter::mEndSpan;
int64_t DcoCarrierAdapter::mDelta;
std::deque<int64_t> DcoCarrierAdapter::mStartSpanArray(spanTime);
std::deque<int64_t> DcoCarrierAdapter::mEndSpanArray(spanTime);
std::deque<int64_t> DcoCarrierAdapter::mDeltaArray(spanTime);
bool DcoCarrierAdapter::mCarrierFltSubEnable   = false;
bool DcoCarrierAdapter::mCarrierPmSubEnable    = false;
bool DcoCarrierAdapter::mCarrierStateSubEnable = false;

vector<AdvancedParameter> DcoCarrierAdapter::mAdvParam[MAX_CARRIERS] = {};

DcoCarrierAdapter::DcoCarrierAdapter()
{
    mspSimCrud = DpSim::SimSbGnmiClient::getInstance();

    std::string serverAddrAndPort = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoZynq());
    INFN_LOG(SeverityLevel::info) << "Gnmi Server Address = " << serverAddrAndPort;
    mspSbCrud = std::dynamic_pointer_cast<GnmiClient::SBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_ZYNQ));

    setSimModeEn(dp_env::DpEnvSingleton::Instance()->isSimEnv());

    if (mspCrud == NULL)
    {
        INFN_LOG(SeverityLevel::error) <<  __func__ << "Crud is NULL" << '\n';
    }

    memset(&mCfg, 0, sizeof(mCfg));
    memset(&mStat, 0, sizeof(mStat));
}

DcoCarrierAdapter::~DcoCarrierAdapter()
 {

 }

bool DcoCarrierAdapter::createCarrier(string aid, CarrierCommon *cfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Port Id: " << carrierId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " create Carrier Id: " << carrierId << endl;
    memset(&mCfg[carrierId-1], 0, sizeof(CarrierCommon));
    memset(&mStat[carrierId-1], 0, sizeof(CarrierStatus));

    Carrier carrier;
    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_name()->set_value(aid);
    // Send down all configuration on create
    double frequency = cfg->frequency;
    frequency *= pow(10, DCO_FREQ_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency()->set_digits(frequency);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency()->set_precision(DCO_FREQ_PRECISION);

    double offset = cfg->freqOffset;
    offset *= pow(10, DCO_FREQ_OFFSET_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency_offset()->set_digits(offset);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency_offset()->set_precision(DCO_FREQ_OFFSET_PRECISION);

    double power = cfg->targetPower;
    power *= pow(10, DCO_POWER_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_optical_power_target()->set_digits(power);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_optical_power_target()->set_precision(DCO_POWER_PRECISION);

    mDataRate[carrierId] = cfg->capacity;  // cache
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_capacity()->set_value(cfg->capacity);

    int64_t baud = (int64_t)std::round(cfg->baudRate * pow(10, DCO_BAUD_RATE_PRECISION));
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_baud_rate()->set_digits(baud);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_baud_rate()->set_precision(DCO_BAUD_RATE_PRECISION);

    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_app_code()->set_value(cfg->appCode);

    double txCd = cfg->txCdPreComp;
    txCd *= pow(10, DCO_TX_CD_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_tx_cd_pre_comp()->set_digits(txCd);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_tx_cd_pre_comp()->set_precision(DCO_TX_CD_PRECISION);

    std::vector<AdvancedParameter> ap = cfg->ap;
    for (auto i = ap.begin(); i != ap.end(); i++ )
    {
        ::dcoyang::infinera_dco_carrier::Carriers_Carrier_Config_AdvancedParameterKey *apKey;
        apKey = carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->add_advanced_parameter();
        apKey->set_ap_key(i->apName);
        apKey->mutable_advanced_parameter()->mutable_ap_value()->set_value(i->apValue);
    }

    if (cfg->dgdOorhThresh >= DCO_DGD_OORH_TH_MIN && cfg->dgdOorhThresh <= DCO_DGD_OORH_TH_MAX)
    {
        double th = cfg->dgdOorhThresh;
        th *= pow(10, DCO_DGD_TH_PRECISION);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_dgd_oorh_thresh()->set_digits(th);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_dgd_oorh_thresh()->set_precision(DCO_DGD_TH_PRECISION);
    }

    if (cfg->postFecQSdThresh >= DCO_POST_FEC_SD_TH_MIN && cfg->postFecQSdThresh <= DCO_POST_FEC_SD_TH_MAX)
    {
        double th = cfg->postFecQSdThresh;
        th *= pow(10, DCO_FEC_TH_PRECISION);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_thresh()->set_digits(th);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_thresh()->set_precision(DCO_FEC_TH_PRECISION);
    }

    if (cfg->postFecQSdHysteresis >= DCO_POST_FEC_SD_HYS_MIN && cfg->postFecQSdHysteresis <= DCO_POST_FEC_SD_HYS_MAX)
    {
        double th = cfg->postFecQSdHysteresis;
        th *= pow(10, DCO_FEC_TH_PRECISION);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_hysteresis()->set_digits(th);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_hysteresis()->set_precision(DCO_FEC_TH_PRECISION);
    }

    if (cfg->preFecQSdThresh >= DCO_PRE_FEC_SD_TH_MIN && cfg->preFecQSdThresh <= DCO_PRE_FEC_SD_TH_MAX)
    {
        double th = cfg->preFecQSdThresh;
        th *= pow(10, DCO_FEC_TH_PRECISION);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_thresh()->set_digits(th);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_thresh()->set_precision(DCO_FEC_TH_PRECISION);
    }

    if (cfg->preFecQSdHysteresis >= DCO_PRE_FEC_SD_HYS_MIN && cfg->preFecQSdHysteresis <= DCO_PRE_FEC_SD_HYS_MAX)
    {
        double th = cfg->preFecQSdHysteresis;
        th *= pow(10, DCO_FEC_TH_PRECISION);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_hysteresis()->set_digits(th);
        carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_hysteresis()->set_precision(DCO_FEC_TH_PRECISION);
    }

    // Since we do not delete carrier once it is created.  Set the provisioned flag as way to indicate to DCO if the SuperChannel has been created = true or deleted = false.
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_provisioned()->set_value(cfg->provisioned);

    // Clear Fault cache
    mCarrierEqptFault[carrierId - 1].ClearFaultCache();
    mCarrierFacFault[carrierId - 1].ClearFaultCache();

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Create( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Create FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        return false;
    }

    // Now send down the enable on a seperate request
    status = setCarrierEnable(aid, cfg->enable);
    //setCarrierStatus(aid);
    mCarrierPms.AddPmData(aid, carrierId);

    //FW send gcc pm based on carrier id
    //pmAgent is mapping all the GCC counters into “comm-channel” object which has same AID format as carrier
    //Please note if in future comm-channel AID has different format as carrier, need to generate new AID for comm-channel
    DcoGccControlAdapter::mGccPms.AddPmData(aid, carrierId);

    CarrierStatus stat;
    status = getCarrierStatus( aid, &stat);
    if (status == true)
    {
        status = updateCarrierStatusNotify( aid, &stat); // initialize notify cache
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrier(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Delete Carrier Id: " << carrierId << endl;

    // Disable carrier before delete it.
    if ( setCarrierEnable(aid, false) == false )
    {
        // Log error but allow deletion to go
        INFN_LOG(SeverityLevel::error) << __func__ << " Disable Error Carrier Id: " << carrierId << '\n';
    }

    // set provisioned = fase before delete it.
    if ( setCarrierProvisioned(aid, false) == false )
    {
        // Log error but allow deletion to go
        INFN_LOG(SeverityLevel::error) << __func__ << " Disable Error Carrier Id: " << carrierId << '\n';
    }

    Carrier carrier;
    //only set id in Carrier level, do not set id in Carrier config level for carrier obj deletion
    carrier.add_carrier()->set_carrier_id(carrierId);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }

    memset(&mCfg[carrierId-1], 0, sizeof(CarrierCommon));
    memset(&mStat[carrierId-1], 0, sizeof(CarrierStatus));
    mCarrierPms.RemovePmData(carrierId);
    DcoGccControlAdapter::mGccPms.RemovePmData(carrierId);

    // Clear baud rate cache
    mDataRate[carrierId] = 0.0;
    // Clear Fault cache
    mCarrierEqptFault[carrierId - 1].ClearFaultCache();
    mCarrierFacFault[carrierId - 1].ClearFaultCache();
    return status;
}

bool DcoCarrierAdapter::initializeCarrier()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    bool status = false;

    for ( int i=0; i < MAX_CARRIERS+1; i++)
    {
        mDataRate[i] = 0.0;
    }

    // Get Carrier Fault Capabitites
    status = getCarrierFaultCapa();

    subscribeStreams();

    return status;
}

void DcoCarrierAdapter::subscribeStreams()
{
    clearFltNotifyFlag();

    //subscribe to carrier pm
    if(!mCarrierPmSubEnable)
    {
        subCarrierPm();
        mCarrierPmSubEnable = true;
    }

    //subscribe to carrier fault
    if(!mCarrierFltSubEnable)
    {
        subCarrierFault();
        mCarrierFltSubEnable = true;
    }

    //subscribe to carrier state
    if(!mCarrierStateSubEnable)
    {
        subCarrierState();
        mCarrierStateSubEnable = true;
    }
}

bool DcoCarrierAdapter::setCarrierStatus(string aid)
{
	INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
	bool status = false;
	int carrierId = aidToCarrierId(aid);
	if (carrierId < 1 || carrierId > 2)
	{
		INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
		return false;
	}
	Carrier carrier;
	if (setKey(&carrier, carrierId) == false)
	{
		return false;
	}
	cout << "set carrier status for aid: " << aid << endl;
	carrier.mutable_carrier(0)->mutable_carrier()->mutable_state()->set_tx_carrier_state(os::INFINERADCOCARRIEROPTICALSTATE_STANDBY);
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
	return true;
}

bool DcoCarrierAdapter::setCarrierCapacity(string aid, uint32_t dataRate)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    // BAO hack to test dual carrier
#if 0
    if (dataRate > 800)
    {
        dataRate /= 2;
    }
#endif
    mDataRate[carrierId] = dataRate;  // cache

    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_capacity()->set_value(dataRate);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }

    return status;
}

bool DcoCarrierAdapter::deleteCarrierCapacity(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_capacity();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierName(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_name();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierBaudRate(string aid, double baudRate)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    int64_t baud = (int64_t)std::round(baudRate * pow(10, DCO_BAUD_RATE_PRECISION));
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_baud_rate()->set_digits(baud);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_baud_rate()->set_precision(DCO_BAUD_RATE_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierBaudRate(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_baud_rate();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}


bool DcoCarrierAdapter::setCarrierAppCode(string aid, string appCode)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_app_code()->set_value(appCode);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierAppCode(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_app_code();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierEnable(string aid, bool enable)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << "Enable: " << (int) enable << " Carrier: " << carrierId << endl;

    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_enable_channel()->set_value(enable);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierEnable(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_enable_channel();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierFrequency(string aid, double frequency)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    frequency *= pow(10, DCO_FREQ_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency()->set_digits(frequency);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency()->set_precision(DCO_FREQ_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierFrequency(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierFreqOffset(string aid, double offset)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    offset *= pow(10, DCO_FREQ_OFFSET_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency_offset()->set_digits(offset);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency_offset()->set_precision(DCO_FREQ_OFFSET_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierFreqOffset(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_carrier_frequency_offset();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierTargetPower(string aid, double power)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    power *= pow(10, DCO_POWER_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_optical_power_target()->set_digits(power);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_optical_power_target()->set_precision(DCO_POWER_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierTargetPower(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_optical_power_target();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierTxCdPreComp(string aid, double txCd)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    txCd*= pow(10, DCO_TX_CD_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_tx_cd_pre_comp()->set_digits(txCd);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_tx_cd_pre_comp()->set_precision(DCO_TX_CD_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierTxCdPreComp(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_tx_cd_pre_comp();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierAdvParam(string aid, std::vector<AdvancedParameter> ap)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    // Send List of APJ
    for (auto i = ap.begin(); i != ap.end(); i++ )
    {
        ::dcoyang::infinera_dco_carrier::Carriers_Carrier_Config_AdvancedParameterKey *apKey;
        apKey = carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->add_advanced_parameter();
        apKey->set_ap_key(i->apName);
        apKey->mutable_advanced_parameter()->mutable_ap_value()->set_value(i->apValue);

        INFN_LOG(SeverityLevel::info) << __func__ << "Sending List of APJ: " << i->apName << endl;
    }

#ifdef BAD_APJ_WA
    status = badApjWorkAround(aid, ap);
    if (status == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad APJ work around failed! Carrier: " << carrierId << endl;
    }
#endif

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;

    }
    return status;
}

// Work-around for Bad APJ.
// Since we cannot remove AP from Redis, if user add and remove a bad APJ, we won't be able to remove it. DCO would always reject full APJ list if we send it a bad APJ. To work-around this issue.
// Read AP list from DCO carrier config. Delete the AP in sysrepo if it is not in the list that L1Agent send to us.  This WA would only work when L1Agent always send the full APJ list down on every AP set (config)

bool DcoCarrierAdapter::badApjWorkAround(string aid, std::vector<AdvancedParameter> ap)
{
    bool status = false;
    CarrierCommon cfg;

    INFN_LOG(SeverityLevel::info) << __func__ << " Run Bad Apj Work Around Check... Carrier: " << aid << '\n';

    if ( getCarrierConfig(aid, &cfg) == false )
    {
        return status;
    }

    for (auto i = cfg.ap.begin(); i != cfg.ap.end(); i++)
    {
        bool found = false;
        for (auto j = ap.begin(); j != ap.end(); j++ )
        {
            if (i->apName == j->apName)
            {
                found = true;
                break;
            }
        }
        // DCO has AP that is not found in current AP list, delete it.
        if (found == false)
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " Trigger Bad Apj Work Around Carrier: " << aid << " AP " << i->apName << '\n';
            if ( deleteCarrierAdvParam(aid, i->apName) == false)
            {
                return status;
            }
        }
    }

    return true;
}

bool DcoCarrierAdapter::deleteCarrierAdvParam(std::string aid, std::string key)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << "Carrier: " << carrierId << " Deleting AP " << key << '\n';

    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);

    ::dcoyang::infinera_dco_carrier::Carriers_Carrier_Config_AdvancedParameterKey *apKey;
    apKey = carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->add_advanced_parameter();
    apKey->set_ap_key(key);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);

    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
	    status = true;
    }
    else
    {
	    INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
	    status = false;
    }
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->clear_advanced_parameter();
    return status;

}

bool DcoCarrierAdapter::deleteCarrierAdvParamAll(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << "Delete all APJ Carrier: " << carrierId << '\n';

    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->add_advanced_parameter();

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);

    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
	    status = true;
    }
    else
    {
	    INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
	    status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierDgdOorhThresh(std::string aid, double threshold)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    threshold *= pow(10, DCO_DGD_TH_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_dgd_oorh_thresh()->set_digits(threshold);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_dgd_oorh_thresh()->set_precision(DCO_DGD_TH_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierDgdOorhThresh(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_dgd_oorh_thresh();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierPostFecQSdThresh(std::string aid, double threshold)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    threshold *= pow(10, DCO_FEC_TH_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_thresh()->set_digits(threshold);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_thresh()->set_precision(DCO_FEC_TH_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierPostFecQSdThresh(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_thresh();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierPostFecQSdHyst(std::string aid, double hysteresis)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    hysteresis *= pow(10, DCO_FEC_TH_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_hysteresis()->set_digits(hysteresis);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_hysteresis()->set_precision(DCO_FEC_TH_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierPostFecQSdHyst(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_post_fec_q_sd_hysteresis();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierPreFecQSdThresh(std::string aid, double threshold)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    threshold *= pow(10, DCO_FEC_TH_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_thresh()->set_digits(threshold);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_thresh()->set_precision(DCO_FEC_TH_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierPreFecQSdThresh(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_thresh();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}


bool DcoCarrierAdapter::setCarrierPreFecQSdHyst(std::string aid, double hysteresis)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    hysteresis *= pow(10, DCO_FEC_TH_PRECISION);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_hysteresis()->set_digits(hysteresis);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_hysteresis()->set_precision(DCO_FEC_TH_PRECISION);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierPreFecQSdHyst(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }

    //since we need to specify exact leaf xpath, only set key in container level
    //keyId in config level is redundant anyway..
    Carrier carrier;
    carrier.add_carrier()->set_carrier_id(carrierId);
    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_pre_fec_q_sd_hysteresis();

    INFN_LOG(SeverityLevel::info) << __func__ << ", aid: " << aid << '\n';

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::setCarrierProvisioned(string aid, bool bProvisioned)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << "Provisioned: " << (int) bProvisioned << " Carrier: " << carrierId << endl;

    carrier.mutable_carrier(0)->mutable_carrier()->mutable_config()->mutable_provisioned()->set_value(bProvisioned);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " update FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

bool DcoCarrierAdapter::deleteCarrierConfigAttributes(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    bool status = true;

    if ( deleteCarrierName(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierCapacity(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierBaudRate(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierAppCode(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierEnable(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierFrequency(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierFreqOffset(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierTargetPower(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierTxCdPreComp(aid) == false )
    {
        status = false;
    }
    // These attribute may return failure if we don't set them in create.
    if ( deleteCarrierDgdOorhThresh(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierPostFecQSdThresh(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierPostFecQSdHyst(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierPreFecQSdThresh(aid) == false )
    {
        status = false;
    }
    if ( deleteCarrierPreFecQSdHyst(aid) == false )
    {
        status = false;
    }
    return status;
}

//
//  Get/Read methods
//

void DcoCarrierAdapter::dumpAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    Carrier carrierMsg;

    uint32 cfgId = 1;
    if (setKey(&carrierMsg, cfgId) == false)
    {
        out << "Failed to set key" << endl;

        return;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrierMsg);

    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);

        grpc::Status gStatus = mspCrud->Read( msg );
        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message();

            out << "DCO Read FAIL. Grpc Error Code: " << gStatus.error_code() << ": " << gStatus.error_message() << endl;

            return;
        }
    }

    for (int idx = 0; idx < carrierMsg.carrier_size(); idx++)
    {
        string carrierAid;

        uint32 objId = (uint32)carrierMsg.carrier(idx).carrier_id();

        if (!carrierMsg.carrier(idx).carrier().has_config())
        {
            out << "*** Carrier Config not present for idx: " << idx << " Skipping!!" << endl;

            continue;
        }

        if (carrierMsg.carrier(idx).carrier().config().has_name())
        {
            carrierAid = carrierMsg.carrier(idx).carrier().config().name().value();

            INFN_LOG(SeverityLevel::debug) << "Carrier has name: " << carrierAid;
        }
        else
        {
            carrierAid = IdGen::genLineAid(objId);

            INFN_LOG(SeverityLevel::debug) << "Carrier has NO NAME. Using old genID created AID: " << carrierAid;
        }

        CarrierCommon carCmn;

        translateConfig(carrierMsg.carrier(idx).carrier().config(), carCmn);

        out << "*** Carrier Config Info for CarrierID: " <<  objId << " Aid: " << carrierAid << endl;

        dumpConfigInfo(out, carCmn);

        out << "*** Carrier Fault Info from cache for CarrierID: " <<  objId << " Aid: " << carrierAid << endl;

        if (carrierFaultInfo(out, carrierAid) == false)
        {
            out << "carrierFaultInfo Failed for Aid " << carrierAid << endl;
        }

        if (carrierMsg.carrier(idx).carrier().has_state())
        {
            CarrierStatus carStat;

            translateStatus(carrierMsg.carrier(idx).carrier().state(), carStat);

            out << "*** Carrier State Info for CarrierID: " <<  objId << " Aid: " << carrierAid << endl;

            dumpStatusInfo(out, carStat);
        }
        else
        {
            out << "*** Carrier State not present for CarrierID: " <<  objId << " Aid: " << carrierAid << endl;
        }

        out << "*** Carrier Notify State from cache for CarrierID: " <<  objId << " Aid: " << carrierAid << endl;

        carrierNotifyState(out, carrierMsg.carrier(idx).carrier_id());

        out << "*** Carrier PM Info from cache for CarrierID: " <<  objId << " Aid: " << carrierAid << endl;

        if (carrierPmInfo(out, carrierMsg.carrier(idx).carrier_id()) == false)
        {
            out << "carrierPmInfo Failed for Aid " << carrierAid << endl;
        }
    }
}

bool DcoCarrierAdapter::carrierConfigInfo(ostream& out, string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    CarrierCommon cfg;
    memset(&cfg, 0, sizeof(cfg));
    bool status = getCarrierConfig(aid, &cfg);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }
    out << " Carrier Config Info: "  << aid << endl;

    dumpConfigInfo(out, cfg);

    return status;
}

void DcoCarrierAdapter::dumpConfigInfo(std::ostream& out, const CarrierCommon& cfg)
{
    std::ios_base::fmtflags flags(out.flags());

    out << " Name (AID): " << cfg.name << endl;
    out << " enable: " << EnumTranslate::toString(cfg.enable) << endl;
    out << " frequency: " << cfg.frequency << endl;
    out << " frequency Offset: " << cfg.freqOffset << endl;
    out << " appId: " << cfg.appCode << endl;
    out << " capacity: " << cfg.capacity << endl;
    out << " baud rate: " << fixed << setprecision(DCO_BAUD_RATE_PRECISION) <<  cfg.baudRate << endl;
    out << " target power: " << cfg.targetPower << endl;
    out << " AD power: " << cfg.adTargetPower << endl;
    out << " AD Enable: " << EnumTranslate::toString(cfg.adEnable) << endl;
    out << " AD id: ";
    for (int i = 0; i<8; i++)
    {
        out << " 0x" << hex << (uint32)cfg.adId[i];
    }
    out << dec << endl;
    out << " Rege Enable: " << EnumTranslate::toString(cfg.regenEnable) << endl;
    out << " tx CD precomp: " << cfg.txCdPreComp << endl;
    out << " listen Only Enable: " << EnumTranslate::toString(cfg.listenOnlyEnable) << endl;
    out << " Advanced Parameters: " << endl;
    for (auto i = cfg.ap.begin(); i != cfg.ap.end(); i++)
    {
        out << "Name: " << i->apName << " Value: " << i->apValue << endl;
    }
    out << " DGD OORH Threshold: " << cfg.dgdOorhThresh << endl;
    out << " Post FEC SD Threshold: " << cfg.postFecQSdThresh << endl;
    out << " Post FEC SD Hysteresis: " << cfg.postFecQSdHysteresis << endl;
    out << " Pre FEC SD Threshold: " << cfg.preFecQSdThresh << endl;
    out << " Pre FEC SD Hysteresis: " << cfg.preFecQSdHysteresis << endl;
    out << " Provisioned: " << EnumTranslate::toString(cfg.provisioned) << endl;
    out << endl;

    out.flags(flags);
}

bool DcoCarrierAdapter::carrierConfigInfo(ostream& out)
{
    TypeMapCarrierConfig mapCfg;

    if (!getCarrierConfig(mapCfg))
    {
        out << "*** FAILED to get Carrier Status!!" << endl;
        return false;
    }

    out << "*** Carrier Config Info ALL: " << endl;

    //out << "size: " << mapCfg.size() << endl;

    for(auto& it : mapCfg)
    {
        out << " Carrier Config Info: "  << it.first << endl;
        dumpConfigInfo(out, *it.second);
        out << endl;
    }

    return true;
}

bool DcoCarrierAdapter::carrierStatusInfo(ostream& out, string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    CarrierStatus stat;
    memset(&stat, 0, sizeof(stat));
    bool status = getCarrierStatus(aid, &stat);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }

    out << " Carrier Status Info: "  << aid << endl;

    dumpStatusInfo(out, stat);

    return status;
}

void DcoCarrierAdapter::dumpStatusInfo(std::ostream& out, const CarrierStatus& stat)
{
    ios_base::fmtflags flag( out.flags() );

    out << " FEC : " << stat.fecIteration << endl;
    out << " FEC OH %: " << stat.fecOhPercentage << endl;
    out << " appCode: " << stat.appCode << endl;
    out << " capacity: " << stat.capacity << endl;
    out << " baud rate: " << fixed << setprecision(DCO_BAUD_RATE_PRECISION) << stat.baudRate << endl;
    out << " ref Clock config: " << stat.refClockConfig << endl;
    out << " TX State: " << EnumTranslate::toString(stat.txState) << endl;
    out << " RX State: " << EnumTranslate::toString(stat.rxState) << endl;
    out << " Line Mode: " << stat.lineMode << endl;
    out << " EQPT Fault: 0x" << hex << stat.eqptFault << dec << endl;
    out << " FAC Fault: 0x" << hex << stat.facFault << dec << endl;
    out << " txCdPreComp: " << stat.txCdPreComp << endl;
    out << " rxCd: " << stat.rxCd << endl;
    out << " target power: " << stat.targetPower << endl;
    out << " frequency: " << stat.frequency << endl;
    out << " frequency Offset: " << stat.freqOffset << endl;
    out << " Advanced Parameters: " << endl;
    for (auto i = stat.currentAp.begin(); i != stat.currentAp.end(); i++)
    {
        out << "Name: " << i->apName << " Value: " << i->apValue << " default: "  << (int) i->bDefault << " Default Value: " << i->defaultValue << " Status: " << i->apStatus << endl;
    }
    out << " DGD OORH Threshold: " << stat.dgdOorhThresh << endl;
    out << " Post FEC SD Threshold: " << stat.postFecQSdThresh << endl;
    out << " Post FEC SD Hysteresis: " << stat.postFecQSdHysteresis << endl;
    out << " Pre FEC SD Threshold: " << stat.preFecQSdThresh << endl;
    out << " Pre FEC SD Hysteresis: " << stat.preFecQSdHysteresis << endl;
    out << " operational state: " << EnumTranslate::toString(stat.operationState) << endl;
    out << " Rx Acq Time: " << stat.rxAcqTime << endl;
    out << " Rx Acq Count Last CLear: " << stat.rxAcqCountLc << endl;
    out << "\n mCarrierStateSubEnable: " << EnumTranslate::toString(mCarrierStateSubEnable) << endl;
    out << " Tx Actual Frequency: " << stat.txActualFrequency << endl;
    out << " Rx Actual Frequency: " << stat.rxActualFrequency << endl;
    out << endl;

    out.flags(flag);
}

bool DcoCarrierAdapter::carrierFaultInfo(ostream& out, string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    CarrierFault fault;
    status = getCarrierFault(aid, &fault);
    if ( status == false )
    {
        out << __func__ << " Get Carrier fault FAIL "  << endl;
        return status;
    }

    out << endl;
    out << "Carrier EQPT Fault: 0x" << hex << fault.eqptBmp  << dec << endl;
    uint bitPos = 0;
    for ( auto i = fault.eqpt.begin(); i != fault.eqpt.end(); i++, bitPos++)
    {
        out << " Fault: " << left << std::setw(25) << i->faultName << " Bit" << left << std::setw(2) << bitPos << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }

    out << endl;
    out << "Carrier FACILITY Fault: 0x" << hex << fault.facBmp << dec << endl;
    bitPos = 0;
    for ( auto i = fault.fac.begin(); i != fault.fac.end(); i++, bitPos++)
    {
        out << " Fault: " << left << std::setw(25) << i->faultName << " Bit" << left << std::setw(2) << bitPos << " Faulted: "<< (int) i->faulted << " Dir: " << i->direction << " Loc: " << i->location << endl;
    }
    out << "\n mCarrierFltSubEnable: " << EnumTranslate::toString(mCarrierFltSubEnable) << endl;
    return status;
}

//only used for cli
bool DcoCarrierAdapter::carrierPmInfo(ostream& out, int carrierId)
{
    if (mCarrierPms.HasPmData(carrierId) == false)
    {
        out << " no pm cache data for carrierId: " << carrierId<< endl;
    return false;
    }
    auto carrierPm = mCarrierPms.GetPmData(carrierId);
    if (carrierPm.carrierId != carrierId)
    {
        out << " not matching.. pm cache data carrierId is: " << carrierPm.carrierId << endl;
        return false;
    }
    auto rx = carrierPm.rx;
    auto tx = carrierPm.tx;
    out << " Carrier aid: " << carrierPm.aid << ", Pm Info: " << endl;
    out << " carrier id: " << carrierPm.carrierId << endl;
    out << " Rx acquisition_state: " << rx.toString(rx.acquisition_state) << endl;
    out << " Rx pre_fec_ber: " << rx.pre_fec_ber << endl;
    out << " Rx pre_fec_q: " << rx.pre_fec_q << endl;
    out << " Rx post_fec_q: " << rx.post_fec_q << endl;
    out << " Rx snr: " << rx.snr << endl;
    out << " Rx snr_avg_x: " << rx.snr_avg_x << endl;
    out << " Rx snr_avg_y: " << rx.snr_avg_y << endl;
    out << " Rx snr_inner: " << rx.snr_inner << endl;
    out << " Rx snr_outer: " << rx.snr_outer << endl;
    out << " Rx snr_diff_left: " << rx.snr_diff_left << endl;
    out << " Rx snr_diff_right: " << rx.snr_diff_right << endl;
    out << " Rx osnr: " << rx.osnr << endl;
    out << " Rx pass_frame_count: " << rx.pass_frame_count << endl;
    out << " Rx failed_frame_count: " << rx.failed_frame_count << endl;
    out << " Rx total_frame_count: " << rx.total_frame_count << endl;
    out << " Rx fas_q: " << rx.fas_q << endl;
    out << " Tx cd: " << tx.cd << endl;
    out << " Rx cd: " << rx.cd << endl;
    out << " Rx polarization_dependent_loss: " << rx.polarization_dependent_loss << endl;
    out << " Rx opr: " << rx.opr << endl;
    out << " Rx laser_current: " << rx.laser_current << endl;
    out << " Rx dgd: " << rx.dgd << endl;
    out << " Rx so_dgd: " << rx.so_dgd << endl;
    out << " Rx corrected_bits: " << rx.corrected_bits << endl;
    out << " Rx phase_correction: " << rx.phase_correction << endl;
    out << " Tx frequency: " << tx.frequency << endl;
    out << " Rx frequency: " << rx.frequency << endl;
    out << " Rx subcarrier snr counter: " << (int)carrierPm.subCarrierCnt << endl;
    out << " Rx subcarrier snr idx: " << endl;
    for (int i = 0; i < rx.subcarrier_snr_list.size(); i++)
    {
        out << " " << rx.subcarrier_snr_list[i].subCarrierIdx << " ,";
    }
    out << endl;
    out << " Rx subcarrier snr: " << endl;
    for (int i = 0; i < rx.subcarrier_snr_list.size(); i++)
    {
        out << " " << rx.subcarrier_snr_list[i].subcarrier_snr << " ,";
    }
    out << endl;
    out << " Tx opt: " << tx.opt << endl;
    out << " Tx laser_current: " << tx.laser_current << endl;
    out << " Tx edfa_input_power: " << tx.edfa_input_power << endl;
    out << " Tx edfa_output_power: " << tx.edfa_output_power << endl;
    out << " Tx edfa_pump_current: " << tx.edfa_pump_current<< endl;
    out << "mCarrierPmSubEnable: " << EnumTranslate::toString(mCarrierPmSubEnable) << endl;

    std::time_t last_c = std::chrono::system_clock::to_time_t(carrierPm.updateTime);
    std::string ts(ctime(&last_c));
    ts.erase(std::remove(ts.begin(), ts.end(), '\n'), ts.end());
    out << "Update Time: " << ts << endl;
    out << "Update Count: " << carrierPm.updateCount << endl;

    return true;
}

//only used for cli
bool DcoCarrierAdapter::carrierPmAccumInfo(ostream& out, int carrierId)
{
	if (mCarrierPms.HasPmData(carrierId) == false)
	{
		out << " no pm cache data for carrierId: " << carrierId<< endl;
		return false;
	}
	auto carrierPmAccum = mCarrierPms.GetPmDataAccum(carrierId);
	if (carrierPmAccum.carrierId != carrierId)
	{
		out << " not matching.. pm cache data carrierId is: " << carrierPmAccum.carrierId << endl;
		return false;
	}
	auto rx = carrierPmAccum.rx;
	auto tx = carrierPmAccum.tx;
	out << " Carrier aid: " << carrierPmAccum.aid << ", Pm Info: " << endl;
        out << " Accumulation Mode: " << (true == carrierPmAccum.accumEnable ? "enabled" : "disabled") << endl;
	out << " Rx corrected_bits: " << rx.corrected_bits << endl;
	out << " Rx pass_frame_count: " << rx.pass_frame_count << endl;
	out << " Rx failed_frame_count: " << rx.failed_frame_count << "\n\n";
	return true;
}

//only used for cli
bool DcoCarrierAdapter::carrierPmAccumClear(ostream& out, int carrierId)
{
    mCarrierPms.ClearPmDataAccum(carrierId);
    return true;
}

bool DcoCarrierAdapter::carrierPmAccumEnable(ostream& out, int carrierId, bool enable)
{
    mCarrierPms.SetAccumEnable(carrierId, enable);
    return true;
}

//spanTime for each dpms carrier pm collection start and end and execution time
void DcoCarrierAdapter::carrierPmTimeInfo(ostream& out)
{
	out << "carrier pm time info in last 20 seconds: " << endl;
	for (int i = 0; i < spanTime; i++)
	{
		out << " dco pm strobe delta, " << i << " :" << mStartSpanArray[i] << " ms" << endl;
	}
	for (int i = 0; i < spanTime; i++)
	{
		out << " send to redis delta, " << i << " :" << mEndSpanArray[i] << " ms" << endl;
	}
	for (int i = 0; i < spanTime; i++)
	{
		out << " dpms execution time, " << i << " :" << mDeltaArray[i] << " ms" << endl;
	}
}

bool DcoCarrierAdapter::getCarrierConfig(string aid, CarrierCommon *cfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getCarrierObj( &carrier, carrierId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Carrier Object  FAIL "  << endl;
        return status;
    }

    if (carrier.carrier(idx).carrier().has_config() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO Config Empty ... Carrier: " <<  carrierId << endl;
        return false;
    }

    translateConfig(carrier.carrier(idx).carrier().config(), *cfg);

    return status;
} // getCarrierConfig

void DcoCarrierAdapter::translateConfig(const CarrierConfig& dcoCfg, CarrierCommon& cfg)
{
    if (dcoCfg.has_name())
    {
        cfg.name = dcoCfg.name().value();
    }
    if (dcoCfg.has_enable_channel())
    {
        cfg.enable = dcoCfg.enable_channel().value();
    }
    if (dcoCfg.has_carrier_frequency())
    {
        int64 digit = dcoCfg.carrier_frequency().digits();
        uint32 precision = dcoCfg.carrier_frequency().precision();
        cfg.frequency = digit/pow(10, precision);
    }
    if (dcoCfg.has_carrier_frequency_offset())
    {
        int64 digit = dcoCfg.carrier_frequency_offset().digits();
        uint32 precision = dcoCfg.carrier_frequency_offset().precision();
        cfg.freqOffset = digit/pow(10, precision);
    }
    if (dcoCfg.has_app_code())
    {
        cfg.appCode = dcoCfg.app_code().value();
    }
    if (dcoCfg.has_capacity())
    {
        cfg.capacity = dcoCfg.capacity().value();
    }
    if (dcoCfg.has_baud_rate())
    {
        int64 digit = dcoCfg.baud_rate().digits();
        //precision is set to 12 for all decimal number due to 400 gige fec-deg-ser threshold on SB grpc server
        cfg.baudRate = digit/pow(10, dcoCfg.baud_rate().precision());
        //in grpc client when convert decimal to protobuf digit, pow or multiple with 10 will leads to precision loss issue
        //in dpms use static precison to round up to get correct decimal value
        getDecimal(cfg.baudRate, DCO_BAUD_RATE_PRECISION);
    }
    if (dcoCfg.has_optical_power_target())
    {
        int64 digit = dcoCfg.optical_power_target().digits();
        uint32 precision = dcoCfg.optical_power_target().precision();
        cfg.targetPower = digit/pow(10, precision);
    }
    if (dcoCfg.has_auto_discovery_optical_power_target())
    {
        int64 digit = dcoCfg.auto_discovery_optical_power_target().digits();
        uint32 precision = dcoCfg.auto_discovery_optical_power_target().precision();
        cfg.adTargetPower = digit/pow(10, precision);
    }
    if (dcoCfg.has_auto_discovery())
    {
        cfg.adEnable = dcoCfg.auto_discovery().value();
    }
    for (int i = 0; i < dcoCfg.auto_discovery_id_tx_size(); i++)
    {
        cfg.adId[i] = dcoCfg.auto_discovery_id_tx(i).value();
    }
    if (dcoCfg.has_regeneration())
    {
        cfg.regenEnable = dcoCfg.regeneration().value();
    }
    if (dcoCfg.has_tx_cd_pre_comp())
    {
        int64 digit = dcoCfg.tx_cd_pre_comp().digits();
        uint32 precision = dcoCfg.tx_cd_pre_comp().precision();
        cfg.txCdPreComp = digit/pow(10, precision);
    }
    if (dcoCfg.has_listen_only())
    {
        cfg.listenOnlyEnable = dcoCfg.listen_only().value();
    }
    for (int i = 0; i < dcoCfg.advanced_parameter_size(); i++)
    {
        const Carriers_Carrier_Config_AdvancedParameterKey apKey = dcoCfg.advanced_parameter(i);

        AdvancedParameter adv;

        adv.apName = apKey.ap_key();
        adv.apValue = apKey.advanced_parameter().ap_value().value();
        cfg.ap.push_back(adv);
    }

    if (dcoCfg.has_dgd_oorh_thresh())
    {
        int64 digit = dcoCfg.dgd_oorh_thresh().digits();
        uint32 precision = dcoCfg.dgd_oorh_thresh().precision();
        cfg.dgdOorhThresh = digit/pow(10, precision);
    }
    if (dcoCfg.has_post_fec_q_sd_thresh())
    {
        int64 digit = dcoCfg.post_fec_q_sd_thresh().digits();
        uint32 precision = dcoCfg.post_fec_q_sd_thresh().precision();
        cfg.postFecQSdThresh = digit/pow(10, precision);
    }
    if (dcoCfg.has_post_fec_q_sd_hysteresis())
    {
        int64 digit = dcoCfg.post_fec_q_sd_hysteresis().digits();
        uint32 precision = dcoCfg.post_fec_q_sd_hysteresis().precision();
        cfg.postFecQSdHysteresis = digit/pow(10, precision);
    }
    if (dcoCfg.has_pre_fec_q_sd_thresh())
    {
        int64 digit = dcoCfg.pre_fec_q_sd_thresh().digits();
        uint32 precision = dcoCfg.pre_fec_q_sd_thresh().precision();
        cfg.preFecQSdThresh = digit/pow(10, precision);
    }
    if (dcoCfg.has_pre_fec_q_sd_hysteresis())
    {
        int64 digit = dcoCfg.pre_fec_q_sd_hysteresis().digits();
        uint32 precision = dcoCfg.pre_fec_q_sd_hysteresis().precision();
        cfg.preFecQSdHysteresis = digit/pow(10, precision);
    }
    if (dcoCfg.has_provisioned())
    {
        cfg.provisioned = dcoCfg.provisioned().value();
    }
}

bool DcoCarrierAdapter::getCarrierConfig(TypeMapCarrierConfig& mapCfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    Carrier carrier;

    uint32 carrierId = 1;
    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);

    bool status = true;

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    INFN_LOG(SeverityLevel::info) << "size: " << carrier.carrier_size();

    for (int idx = 0; idx < carrier.carrier_size(); idx++)
    {
        string carrierAid;
        if (carrier.carrier(idx).carrier().config().has_name())
        {
            carrierAid = carrier.carrier(idx).carrier().config().name().value();

            INFN_LOG(SeverityLevel::info) << "Carrier has name: " << carrierAid;
        }
        else
        {
            carrierAid = IdGen::genLineAid((uint32)carrier.carrier(idx).carrier_id());

            INFN_LOG(SeverityLevel::info) << "Carrier has NO NAME. Using old genID created AID: " << carrierAid;
        }

        mapCfg[carrierAid] = make_shared<CarrierCommon>();
        translateConfig(carrier.carrier(idx).carrier().config(), *mapCfg[carrierAid]);
    }

    return status;
}

bool DcoCarrierAdapter::getCarrierStatus(string aid, CarrierStatus *stat)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getCarrierObj( &carrier, carrierId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco carrier Object  FAIL "  << endl;
        return status;
    }

    if (carrier.carrier(idx).carrier().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... carrier: " <<  carrierId << endl;
        return false;
    }

    translateStatus(carrier.carrier(idx).carrier().state(), *stat);

    return status;
} // getCarrierStatus

void DcoCarrierAdapter::translateStatus(const DcoCarrierState& dcoState, CarrierStatus& stat)
{
    uint32 carrierId = dcoState.carrier_id().value();

    if (dcoState.has_fec_iteration())
    {
        stat.fecIteration = (dcoState.fec_iteration().value());
    }
    if (dcoState.has_fec_oh_percentage())
    {
        stat.fecOhPercentage = (dcoState.fec_oh_percentage().value());
    }
    if (dcoState.has_app_code())
    {
        stat.appCode = (dcoState.app_code().value());
    }
    if (dcoState.has_capacity_reported())
    {
        int64 digit = dcoState.capacity_reported().digits();
        uint32 precision = dcoState.capacity_reported().precision();
        stat.capacity = digit/pow(10, precision);
    }
    if (dcoState.has_baud_rate_reported())
    {
        int64 digit = dcoState.baud_rate_reported().digits();
        //precision is set to 12 for all decimal number due to 400 gige fec-deg-ser threshold on SB grpc server
        uint32 precision = dcoState.baud_rate_reported().precision();
        stat.baudRate = digit/pow(10, precision);
        //in grpc client when convert decimal to protobuf digit, pow or multiple with 10 will leads to precision loss issue
        //in dpms use static precison to round up to get correct decimal value
        getDecimal(stat.baudRate, DCO_BAUD_RATE_PRECISION);
    }
    if (dcoState.has_ref_clock_config_reported())
    {
        int64 digit = dcoState.ref_clock_config_reported().digits();
        uint32 precision = dcoState.ref_clock_config_reported().precision();
        stat.refClockConfig = digit/pow(10, precision);
    }

    using carrierState = ::dcoyang::enums::InfineraDcoCarrierOpticalState;

    switch (dcoState.tx_carrier_state())
    {
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE:
            stat.txState = DataPlane::CARRIERSTATE_ACTIVE;
            break;
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_STANDBY:
            stat.txState = DataPlane::CARRIERSTATE_STANDBY;
            break;
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_AUTO_DISCOVERY:
            stat.txState = DataPlane::CARRIERSTATE_AUTODISCOVERY;
            break;
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_UNDEFINED:
            stat.txState = DataPlane::CARRIERSTATE_UNDEFINED;
            break;
        default:
            stat.txState = DataPlane::CARRIERSTATE_UNSPECIFIED;
            break;

    }
    switch (dcoState.rx_carrier_state())
    {
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE:
            stat.rxState = DataPlane::CARRIERSTATE_ACTIVE;
            break;
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_STANDBY:
            stat.rxState = DataPlane::CARRIERSTATE_STANDBY;
            break;
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_AUTO_DISCOVERY:
            stat.rxState = DataPlane::CARRIERSTATE_AUTODISCOVERY;
            break;
        case carrierState::INFINERADCOCARRIEROPTICALSTATE_UNDEFINED:
            stat.rxState = DataPlane::CARRIERSTATE_UNDEFINED;
            break;
        default:
            stat.rxState = DataPlane::CARRIERSTATE_UNSPECIFIED;
            break;

    }
    if (dcoState.has_line_mode())
    {
        stat.lineMode = (dcoState.line_mode().value());
    }
    if (dcoState.has_equipment_fault())
    {
        // todo: is this needed?

        if ((carrierId - 1) < MAX_CARRIERS)
        {
            std::lock_guard<std::recursive_mutex> lock(mMutex);

            stat.eqptFault = (dcoState.equipment_fault().value());
            mCarrierEqptFault[carrierId - 1].faultBitmap = stat.eqptFault;
        }
    }
    if (dcoState.has_facility_fault())
    {
        // todo: is this needed?

        if ((carrierId - 1) < MAX_CARRIERS)
        {
            std::lock_guard<std::recursive_mutex> lock(mMutex);

            stat.facFault = (dcoState.facility_fault().value());
            mCarrierFacFault[carrierId - 1].faultBitmap = stat.facFault;
        }
    }
    if (dcoState.has_tx_cd_pre_comp())
    {
        int64 digit = dcoState.tx_cd_pre_comp().digits();
        uint32 precision = dcoState.tx_cd_pre_comp().precision();
        stat.txCdPreComp = digit/pow(10, precision);
    }
    // TODO Checking with Paul to see if it need to be a double
    if (dcoState.has_rx_cd_status())
    {
        stat.rxCd = (dcoState.rx_cd_status().value());
    }
    if (dcoState.has_optical_power_target())
    {
        int64 digit = dcoState.optical_power_target().digits();
        uint32 precision = dcoState.optical_power_target().precision();
        stat.targetPower = digit/pow(10, precision);
    }
    if (dcoState.has_carrier_frequency())
    {
        int64 digit = dcoState.carrier_frequency().digits();
        uint32 precision = dcoState.carrier_frequency().precision();
        stat.frequency = digit/pow(10, precision);
    }
    if (dcoState.has_carrier_frequency_offset())
    {
        int64 digit = dcoState.carrier_frequency_offset().digits();
        uint32 precision = dcoState.carrier_frequency_offset().precision();
        stat.freqOffset = digit/pow(10, precision);
    }

    for (int i = 0; i < dcoState.advanced_parameter_current_size(); i++)
    {
        Carriers_Carrier_State_AdvancedParameterCurrentKey apKey = dcoState.advanced_parameter_current(i);

        AdvancedParameter adv;
        using apCurrent = Carriers_Carrier_State_AdvancedParameterCurrent;
        adv.apName = apKey.ap_key();
        adv.apValue = apKey.advanced_parameter_current().ap_value().value();
        adv.bDefault = apKey.advanced_parameter_current().default_().value();
        adv.defaultValue = apKey.advanced_parameter_current().default_ap_value().value();
        switch(apKey.advanced_parameter_current().ap_status())
        {
            case apCurrent::APSTATUS_SET:
                adv.apStatus = DataPlane::CARRIERADVPARMSTATUS_SET;
                break;
            case apCurrent::APSTATUS_UNKNOWN:
                adv.apStatus = DataPlane::CARRIERADVPARMSTATUS_UNKNOWN;
                break;
            case apCurrent::APSTATUS_IN_PROGRESS:
                adv.apStatus = DataPlane::CARRIERADVPARMSTATUS_INPROGRESS;
                break;
            case apCurrent::APSTATUS_FAILED:
                adv.apStatus = DataPlane::CARRIERADVPARMSTATUS_FAILED;
                break;
            case apCurrent::APSTATUS_NOT_SUPPORTED:
                adv.apStatus = DataPlane::CARRIERADVPARMSTATUS_NOT_SUPPORTED;
                break;
            default:
                adv.apStatus = DataPlane::CARRIERADVPARMSTATUS_UNSPECIFIED;
                break;
        }
        stat.currentAp.push_back(adv);
    }

    if (dcoState.has_dgd_oorh_thresh())
    {
        int64 digit = dcoState.dgd_oorh_thresh().digits();
        uint32 precision = dcoState.dgd_oorh_thresh().precision();
        stat.dgdOorhThresh = digit/pow(10, precision);
    }

    if (dcoState.has_post_fec_q_sd_thresh())
    {
        int64 digit = dcoState.post_fec_q_sd_thresh().digits();
        uint32 precision = dcoState.post_fec_q_sd_thresh().precision();
        stat.postFecQSdThresh = digit/pow(10, precision);
    }

    if (dcoState.has_post_fec_q_sd_hysteresis())
    {
        int64 digit = dcoState.post_fec_q_sd_hysteresis().digits();
        uint32 precision = dcoState.post_fec_q_sd_hysteresis().precision();
        stat.postFecQSdHysteresis = digit/pow(10, precision);
    }

    if (dcoState.has_pre_fec_q_sd_thresh())
    {
        int64 digit = dcoState.pre_fec_q_sd_thresh().digits();
        uint32 precision = dcoState.pre_fec_q_sd_thresh().precision();
        stat.preFecQSdThresh = digit/pow(10, precision);
    }

    if (dcoState.has_pre_fec_q_sd_hysteresis())
    {
        int64 digit = dcoState.pre_fec_q_sd_hysteresis().digits();
        uint32 precision = dcoState.pre_fec_q_sd_hysteresis().precision();
        stat.preFecQSdHysteresis = digit/pow(10, precision);
    }

    if (dcoState.has_operational_state())
    {
        stat.operationState = (dcoState.operational_state().value());
    }

    if (dcoState.has_rx_acquisition_time())
    {
        int64 digit = dcoState.rx_acquisition_time().digits();
        uint32 precision = dcoState.rx_acquisition_time().precision();
        stat.rxAcqTime = digit/pow(10, precision);
    }

    if (dcoState.has_rx_acquisition_count_lc())
    {
        stat.rxAcqCountLc = (dcoState.rx_acquisition_count_lc().value());
    }
    if (dcoState.has_tx_carrier_frequency_reported())
    {
        int64 digit = dcoState.tx_carrier_frequency_reported().digits();
        uint32 precision = dcoState.tx_carrier_frequency_reported().precision();
        stat.txActualFrequency = digit/pow(10, precision);
    }
    if (dcoState.has_rx_carrier_frequency_reported())
    {
        int64 digit = dcoState.rx_carrier_frequency_reported().digits();
        uint32 precision = dcoState.rx_carrier_frequency_reported().precision();
        stat.rxActualFrequency = digit/pow(10, precision);
    }
}

bool DcoCarrierAdapter::getCarrierStatusNotify(string aid, CarrierStatus *stat)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }
    Carrier carrier;

    stat->fecIteration = 0;
    stat->fecOhPercentage = 0;
    stat->appCode = "";
    stat->capacity = 0.0;
    stat->baudRate = 0.0;
    stat->refClockConfig = 0.0;
    stat->txState = mTxState[carrierId-1];
    stat->rxState = mRxState[carrierId-1];
    stat->lineMode = "";
    stat->txCdPreComp = 0.0;
    stat->rxCd = 0.0;
    stat->targetPower = 0.0;
    stat->frequency = 0.0;
    stat->freqOffset = 0.0;
    for (auto i = mAdvParam[carrierId-1].begin(); i != mAdvParam[carrierId-1].end(); i++)
    {
        AdvancedParameter adv;
        adv.apName = i->apName;
        adv.apValue = i->apValue;
        adv.bDefault = i->bDefault;
        adv.defaultValue = i->defaultValue;
        adv.apStatus = i->apStatus;
        stat->currentAp.push_back(adv);
    }
    stat->dgdOorhThresh = 0.0;
    stat->postFecQSdThresh = 0.0;
    stat->postFecQSdHysteresis = 0.0;
    stat->preFecQSdThresh = 0.0;
    stat->preFecQSdHysteresis = 0.0;
    stat->operationState = false;
    stat->rxAcqTime = 0.0;
    stat->rxAcqCountLc = 0;
    stat->txActualFrequency = mTxActualFrequency[carrierId-1];
    stat->rxActualFrequency = mRxActualFrequency[carrierId-1];
    return true;
} // getCarrierStatusNotify

bool DcoCarrierAdapter::getCarrierFault(string aid, uint64_t *fault)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " carrier: " << carrierId << endl;
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getCarrierObj( &carrier, carrierId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco carrier Object  FAIL "  << endl;
        return status;
    }

    if (carrier.carrier(idx).carrier().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... carrier: " <<  carrierId << endl;
        return false;
    }

    if (carrier.carrier(idx).carrier().state().has_facility_fault())
    {
        *fault = carrier.carrier(idx).carrier().state().facility_fault().value();
    }

    return status;
}

//force equal to true case should only be used for warm boot query
bool DcoCarrierAdapter::getCarrierFault(string aid, CarrierFault *faults, bool force)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::debug) << __func__ << " carrier: " << carrierId << endl;
    Carrier carrier;

    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    bool status = true;
    //if carrier fault is already been updated via notification, skip the retrieval
    if (force && (mCarrierEqptFault[carrierId - 1].isNotified == false || mCarrierFacFault[carrierId - 1].isNotified == false))
    {
	    int idx = 0;
	    status = getCarrierObj( &carrier, carrierId, &idx);
	    if ( status == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco carrier Object  FAIL "  << endl;
		    return status;
	    }

	    if (carrier.carrier(idx).carrier().has_state() == false )
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... carrier: " <<  carrierId << endl;
		    return false;
	    }

	    if (carrier.carrier(idx).carrier().state().has_facility_fault())
	    {
		    faults->facBmp = carrier.carrier(idx).carrier().state().facility_fault().value();
		    if(mCarrierFacFault[carrierId - 1].isNotified == false) //did not get notify update yet if there is a fault in dco
		    {
			    mCarrierFacFault[carrierId - 1].faultBitmap = faults->facBmp;
			    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve carrier fault " << carrierId << " with bitmap: 0x" <<  hex << mCarrierFacFault[carrierId - 1].faultBitmap << dec << endl;
		    }
		    else
		    {
			    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve carrier fault " << carrierId << " with fault notify flag:" <<  mCarrierFacFault[carrierId - 1].isNotified << endl;
		    }
	    }
	    else
	    {
		    INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no carrier fault in state, carrierId :" << carrierId;
	    }

	    if (carrier.carrier(idx).carrier().state().has_equipment_fault())
	    {
		    faults->eqptBmp = carrier.carrier(idx).carrier().state().equipment_fault().value();
		    if(mCarrierEqptFault[carrierId - 1].isNotified == false) //did not get notify update yet if there is a fault in dco
		    {
			    mCarrierEqptFault[carrierId - 1].faultBitmap = faults->eqptBmp;
			    INFN_LOG(SeverityLevel::info) << __func__ << " force retrieve carrier fault " << carrierId << " with bitmap: 0x" <<  hex << mCarrierEqptFault[carrierId - 1].faultBitmap << dec << endl;
		    }
	    }
    }
    else
    {
	    faults->eqptBmp = mCarrierEqptFault[carrierId - 1].faultBitmap;
	    faults->facBmp = mCarrierFacFault[carrierId - 1].faultBitmap;
    }

    uint64_t eqptFault = faults->eqptBmp;
    uint64_t facFault = faults->facBmp;
    AdapterFault tmpFault;
    for ( auto i = mFaultCapa.eqpt.begin(); i != mFaultCapa.eqpt.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (eqptFault & 0x1)
        {
            tmpFault.faulted = true;
        }
        faults->eqpt.push_back(tmpFault);
        eqptFault >>= 0x1;
    }

    for ( auto i = mFaultCapa.fac.begin(); i != mFaultCapa.fac.end(); i++)
    {
        tmpFault.faultName = i->faultName;
        tmpFault.direction = i->direction;
        tmpFault.location = i->location;
        tmpFault.faulted = false;
        if (facFault & 0x1)
        {
            tmpFault.faulted = true;
        }
        faults->fac.push_back(tmpFault);
        facFault >>= 0x1;
    }
    return status;
}

bool DcoCarrierAdapter::getCarrierPm(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    return true;
}

void DcoCarrierAdapter::subCarrierFault()
{
	CarrierFms cfm;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&cfm);
	::GnmiClient::AsyncSubCallback* cb = new CarrierFaultResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " carrier fault sub request sent to server.."  << endl;
}

void DcoCarrierAdapter::subCarrierState()
{
	dcoyang::infinera_dco_carrier::CarrierState carrstate;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrstate);
	::GnmiClient::AsyncSubCallback* cb = new CarrierStateResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " carrier state sub request sent to server.."  << endl;
}

void DcoCarrierAdapter::subCarrierPm()
{
	if (dp_env::DpEnvSingleton::Instance()->isSimEnv() && mIsGnmiSimModeEn) {
		//spawn a thread to feed up dgdata for sim
		if (mSimCarrierPmThread.joinable() == false) { //check whether the sim pm thread is running
			INFN_LOG(SeverityLevel::info) << "create mSimCarrierPmThread " << endl;
			mSimCarrierPmThread = std::thread(&DcoCarrierAdapter::SetSimPmData, this);
		}
		else {
			INFN_LOG(SeverityLevel::info) << "mSimCarrierPmThread is already running..." << endl;
		}
	}
	else {
		INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
		dcoCarrierPm cp;
	    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&cp);
		::GnmiClient::AsyncSubCallback* cb = new CarrierPmResponseContext(); //cleaned up in grpc client
		std::string cbId = mspCrud->Subscribe(msg, cb);
		INFN_LOG(SeverityLevel::info) << " carrier pm sub request sent to server.."  << endl;
	}
}

//set same sim pm data for all objects..
void DcoCarrierAdapter::SetSimPmData()
{
    while (1)
	{
		this_thread::sleep_for(seconds(1));

        bool isSimDcoConnect = DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)DCC_ZYNQ).isSimDcoConnect();
        if (!mIsGnmiSimModeEn || !isSimDcoConnect)
        {
            continue;
        }

		for (int i = 1; i <= MAX_CARRIERS; i++) {
			if (mCarrierPms.HasPmData(i) == false) continue;
			auto carrierPm = mCarrierPms.GetPmData(i);
			int carrierId = carrierPm.carrierId;
			INFN_LOG(SeverityLevel::debug) << __func__ << " carrierId: " << carrierId << ", carrierPm.aid: " << carrierPm.aid << ", i: " << i << endl;
			carrierPm.rx.acquisition_state = DpMsCarrierRx::ACQ_RX_ACTIVE;
			carrierPm.rx.pre_fec_ber = 100.12;
			carrierPm.rx.pre_fec_q = 200.34;
			carrierPm.rx.post_fec_q = 300.56;
			carrierPm.rx.snr = 400.78;
			carrierPm.rx.pass_frame_count = 600;
			carrierPm.rx.failed_frame_count = 700;
			carrierPm.rx.corrected_bits = 880;
			carrierPm.rx.fas_q = 30.0;
			carrierPm.rx.cd = 30.0;
			carrierPm.rx.dgd = 30.0;
			carrierPm.rx.polarization_dependent_loss = 30.0;
			carrierPm.rx.opr = 1.7;
			carrierPm.rx.laser_current = 404.6;

			carrierPm.tx.opt = 808.1;
			carrierPm.tx.laser_current = 901.2;
			carrierPm.tx.edfa_input_power = 101.3;
			carrierPm.tx.edfa_output_power = 201.4;
			carrierPm.tx.edfa_pump_current = 301.5;
			DcoCarrierAdapter::mCarrierPms.UpdatePmData(carrierPm, carrierId); //pmdata map key is 1 based
		}

	    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	    dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoCarrierAdapter::mCarrierPms), &DcoCarrierAdapter::mCarrierPms);
	}
}

bool DcoCarrierAdapter::updateNotifyCache()
{
    Carrier carrier;
    if (setKey(&carrier, 1) == false)
    {
        return false;
    }
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    bool status = true;

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " carrier size: " << carrier.carrier_size();

    for (int idx = 0; idx < carrier.carrier_size(); idx++)
    {
        int carrierId = 1;
        if (carrier.carrier(idx).carrier().state().has_carrier_id())
        {
            carrierId = carrier.carrier(idx).carrier().state().carrier_id().value();
            INFN_LOG(SeverityLevel::info) << " carrier has carrierId: " << carrierId;
        }
        if (carrier.carrier(idx).carrier().state().has_facility_fault())
        {
            //if carrier fault is already been updated via notification, skip the retrieval
            if (mCarrierEqptFault[carrierId - 1].isNotified == false)
            {
                mCarrierFacFault[carrierId - 1].faultBitmap = (carrier.carrier(idx).carrier().state().facility_fault().value());
	        INFN_LOG(SeverityLevel::info) << " Carrier has fac fault: 0x" << hex << mCarrierFacFault[carrierId - 1].faultBitmap << dec;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve carrier fault " << carrierId << " with fault notify flag:" <<  mCarrierFacFault[carrierId - 1].isNotified << endl;
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " skip force retrieve since no carrier fault in state, carrierId :" << carrierId;
        }
    }
    return status;
}

// ====== DcoCarrierAdapter private =========

int DcoCarrierAdapter::aidToCarrierId (string aid)
{
    INFN_LOG(SeverityLevel::debug) << __func__ << " run... AID: "  << aid << endl;
    if (aid == NULL)
        return -1;
    // AID format: 1-4-L1-1

    string sId;
    string aidPort = "-L";
    std::size_t found = aid.find(aidPort);
    if (found != std::string::npos)
    {
        std::size_t pos = found + aidPort.length();
        while (pos != std::string::npos)
        {
            if (aid[pos] == '-')
                break;
            sId.push_back(aid[pos]);
            INFN_LOG(SeverityLevel::debug) << __func__ << "sId: " << sId << endl;
            pos++;
        }
    }
    else
    {
        string cliPort = "Carrier ";
        found = aid.find(cliPort);
        if (found == std::string::npos)
            return -1;
        sId = aid.substr(cliPort.length());
    }

    INFN_LOG(SeverityLevel::debug) << __func__ << " sId" << sId << endl;
	return(stoi(sId,nullptr));
}

bool DcoCarrierAdapter::setKey (Carrier *carrier, int carrierId)
{
    bool status = true;
	if (carrier == NULL)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Fail *carrier = NULL... "  << endl;
        return false;
    }

    Carriers_CarrierKey *cKey;

    // Always use the first object in the list, since we
    // create new object every time we send.
    cKey = carrier->add_carrier();
    cKey->set_carrier_id(carrierId);
    cKey->mutable_carrier()->mutable_config()->mutable_carrier_id()->set_value(carrierId);

    return status;
}

//
// Fetch Carrier containter object and find the object that match with
// carrierId in list
//
bool DcoCarrierAdapter::getCarrierObj( Carrier *carrier, int carrierId, int *idx)
{
    // Save the PortId from setKey
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(carrier);
    INFN_LOG(SeverityLevel::debug) << __func__ << " \n\n ****Read ... \n"  << endl;
    bool status = true;

    std::lock_guard<std::recursive_mutex> lock(mMutex);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        carrier = static_cast<Carrier*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    // Search for the correct PortId.
    bool found = false;

    for (*idx = 0; *idx < carrier->carrier_size(); (*idx)++)
    {
        if (carrier->carrier(*idx).carrier_id() == carrierId )
        {
            found = true;
            break;
        }
    }
    if (found == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " cannot find CarrierId in DCO  FAIL... CarrierId: " <<  carrierId << endl;
        return false;
    }
    return status;
}

// setCarrierConfig() place holder, TBD remove if not use, not implemented
bool DcoCarrierAdapter::setCarrierConfig(string aid, CarrierCommon *cfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	bool status = false;
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Config Carrier Id: " << carrierId << endl;

    Carrier carrier;
    if (setKey(&carrier, carrierId) == false)
    {
        return false;
    }

    if (mCfg[carrierId-1].enable != cfg->enable)
    {
        mCfg[carrierId-1].enable = cfg->enable;
    }
    if (mCfg[carrierId-1].frequency != cfg->frequency)
    {
        mCfg[carrierId-1].frequency = cfg->frequency;
    }
    if (mCfg[carrierId-1].freqOffset != cfg->freqOffset)
    {
        mCfg[carrierId-1].freqOffset = cfg->freqOffset;
    }
    if (mCfg[carrierId-1].appCode != cfg->appCode)
    {
        mCfg[carrierId-1].appCode = cfg->appCode;
    }
    if (mCfg[carrierId-1].capacity != cfg->capacity)
    {
        mCfg[carrierId-1].capacity = cfg->capacity;
    }
    if (mCfg[carrierId-1].baudRate != cfg->baudRate)
    {
        mCfg[carrierId-1].baudRate = cfg->baudRate;
    }
    if (mCfg[carrierId-1].targetPower != cfg->targetPower)
    {
        mCfg[carrierId-1].targetPower = cfg->targetPower;
    }
    if (mCfg[carrierId-1].adEnable != cfg->adEnable)
    {
        mCfg[carrierId-1].adEnable = cfg->adEnable;
    }
    for (int i = 0; i < 8; i++)
    {
        if (mCfg[carrierId-1].adId[i] != cfg->adId[i])
        {
            memcpy( &mCfg[carrierId-1].adId[0], &cfg->adId[0], sizeof( mCfg[carrierId-1].adId));
        }
    }
    if (mCfg[carrierId-1].regenEnable != cfg->regenEnable)
    {
        mCfg[carrierId-1].regenEnable = cfg->regenEnable;
    }
    if (mCfg[carrierId-1].txCdPreComp != cfg->txCdPreComp)
    {
        mCfg[carrierId-1].txCdPreComp = cfg->txCdPreComp;
    }
    if (mCfg[carrierId-1].listenOnlyEnable != cfg->listenOnlyEnable)
    {
        mCfg[carrierId-1].listenOnlyEnable = cfg->listenOnlyEnable;
    }

    int j = 0;
    for ( auto i = cfg->ap.begin(); i != cfg->ap.end(); i++, j++)
    {
        if (mCfg[carrierId-1].ap[j].apName != i->apName ||
            mCfg[carrierId-1].ap[j].apValue != i->apValue)
        {
            mCfg[carrierId-1].ap[j].apName = i->apName;
            mCfg[carrierId-1].ap[j].apValue = i->apValue;
        }
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&carrier);
    grpc::Status gStatus = mspCrud->Update( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

void DcoCarrierAdapter::setSimModeEn(bool enable)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (enable)
    {
        mspCrud = mspSimCrud;
        mIsGnmiSimModeEn = true;
    }
    else
    {
        mspCrud = mspSbCrud;
        mIsGnmiSimModeEn = false;
    }
}

enum CarrierState {
    CARRIERSTATE_UNSPECIFIED = 0,
    CARRIERSTATE_ACTIVE = 1,
    CARRIERSTATE_STANDBY = 2,
    CARRIERSTATE_AUTODISCOVERY = 3,
};

bool DcoCarrierAdapter::getCarrierFaultCapa()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = false;
    INFN_LOG(SeverityLevel::info) << __func__ << " run... "  << endl;
    DcoCarrierFault fault;
    DcoCarrierFault *pFault;

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&fault);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        pFault = static_cast<DcoCarrierFault*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }
    mFaultCapa.eqpt.clear();
    mFaultCapa.fac.clear();

    for ( int i = 0; i < pFault->capabilities().equipment_faults_size(); i++ )
    {
        FaultCapability fault;
        DcoCarrierFault_Capabilities_EquipmentFaultsKey key;

        key = pFault->capabilities().equipment_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.equipment_faults().fault_name().value();
        if (key.equipment_faults().has_service_affecting())
        {
            fault.serviceAffecting = key.equipment_faults().service_affecting().value();
        }
        else
        {
            fault.serviceAffecting = false;
        }
        if (key.equipment_faults().has_bit_pos())
        {
            fault.bitPos = key.equipment_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.equipment_faults().has_fault_description())
        {
            fault.faultDescription = key.equipment_faults().fault_description().value();
        }
        using eqptSeverity = dcoyang::infinera_dco_carrier_fault::DcoCarrierFault_Capabilities_EquipmentFaults;
        switch( key.equipment_faults().severity())
        {
            case eqptSeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case eqptSeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case eqptSeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_carrier_fault::DcoCarrierFault_Capabilities_EquipmentFaults;
        switch( key.equipment_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_carrier_fault::DcoCarrierFault_Capabilities_EquipmentFaults;
        switch( key.equipment_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }

        mFaultCapa.eqpt.push_back(fault);
    }

    for ( int i = 0; i < pFault->capabilities().facility_faults_size(); i++ )
    {
        FaultCapability fault;
        fault.serviceAffecting = false;
        DcoCarrierFault_Capabilities_FacilityFaultsKey key;

        key = pFault->capabilities().facility_faults(i);

        fault.faultKey = key.fault();
        fault.faultName = key.facility_faults().fault_name().value();
        if (key.facility_faults().has_bit_pos())
        {
            fault.bitPos = key.facility_faults().bit_pos().value();
        }
        else
        {
            fault.bitPos = FAULT_BIT_POS_UNSPECIFIED;
        }
        if (key.facility_faults().has_fault_description())
        {
            fault.faultDescription = key.facility_faults().fault_description().value();
        }

        using facilitySeverity = dcoyang::infinera_dco_carrier_fault::DcoCarrierFault_Capabilities_FacilityFaults;
        switch( key.facility_faults().severity())
        {
            case facilitySeverity::SEVERITY_DEGRADE:
                fault.severity = FAULTSEVERITY_DEGRADED;
                break;
            case facilitySeverity::SEVERITY_CRITICAL:
                fault.severity = FAULTSEVERITY_CRITICAL;
                break;
            case facilitySeverity::SEVERITY_FAIL:
                fault.severity = FAULTSEVERITY_FAILURE;
                break;
            default:
                fault.severity = FAULTSEVERITY_UNSPECIFIED;
                break;
        }

        using dir = dcoyang::infinera_dco_carrier_fault::DcoCarrierFault_Capabilities_FacilityFaults;
        switch( key.facility_faults().direction())
        {
            case dir::DIRECTION_RX:
                fault.direction = FAULTDIRECTION_RX;
                break;
            case dir::DIRECTION_TX:
                fault.direction = FAULTDIRECTION_TX;
                break;
            case dir::DIRECTION_NA:
                fault.direction = FAULTDIRECTION_NA;
                break;
            default:
                fault.direction = FAULTDIRECTION_UNSPECIFIED;
                break;
        }

        using loc = dcoyang::infinera_dco_carrier_fault::DcoCarrierFault_Capabilities_FacilityFaults;
        switch( key.facility_faults().location())
        {
            case loc::LOCATION_NEAR_END:
                fault.location = FAULTLOCATION_NEAR_END;
                break;
            case loc::LOCATION_FAR_END:
                fault.location = FAULTLOCATION_FAR_END;
                break;
            case loc::LOCATION_NA:
                fault.location = FAULTLOCATION_NA;
                break;
            default:
                fault.location = FAULTLOCATION_UNSPECIFIED;
                break;
        }
        mFaultCapa.fac.push_back(fault);
    }
    return status;
}

void DcoCarrierAdapter::carrierNotifyState(std::ostream& out, int carrierId)
{
    if (carrierId < 1 || carrierId > 2)
    {
        out << "Bad Carrier ID: " << carrierId << endl;
        return;
    }

    out << " DCO Carrier Notify State: \n" <<  endl;
    out << "\n mCarrierPmSubEnable: " << mCarrierPmSubEnable  << endl;
    out << " Tx State: " << EnumTranslate::toString(mTxState[carrierId-1]) << endl;
    out << " Rx State: " << EnumTranslate::toString(mRxState[carrierId-1]) << endl;
    for (auto i = mAdvParam[carrierId-1].begin(); i != mAdvParam[carrierId-1].end(); i++)
    {
        out << "Name: " << i->apName << " Value: " << i->apValue << " Status: " << EnumTranslate::toString( i->apStatus) << endl;
    }
    out << " Tx Actual Frequency: " << mTxActualFrequency[carrierId-1] << endl;
    out << " Rx Actual Frequency: " << mRxActualFrequency[carrierId-1] << endl;
}

double DcoCarrierAdapter::getCacheCarrierDataRate( int carrierId)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << "Carrier ID " << carrierId << " mDataRate " << mDataRate[carrierId] << endl;
    return (mDataRate[carrierId]);
}

// Update Notify cache on warm boot
bool DcoCarrierAdapter::updateCarrierStatusNotify( std::string aid, CarrierStatus *stat)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    int carrierId = aidToCarrierId(aid);
    if (carrierId < 1 || carrierId > 2)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Carrier Id: " << carrierId << '\n';
        return false;
    }
    mTxState[carrierId-1] = stat->txState;
    mRxState[carrierId-1] = stat->rxState;

    for (auto i = stat->currentAp.begin(); i != stat->currentAp.end(); i++)
    {
        AdvancedParameter adv;
        adv.apName = i->apName;
        adv.apValue = i->apValue;
        adv.bDefault = i->bDefault;
        adv.defaultValue = i->defaultValue;
        adv.apStatus = i->apStatus;
		mAdvParam[carrierId-1].push_back(adv);
    }
    mTxActualFrequency[carrierId-1] = stat->txActualFrequency;
    mRxActualFrequency[carrierId-1] = stat->rxActualFrequency;
    return true;
}

void DcoCarrierAdapter::clearFltNotifyFlag()
{
    INFN_LOG(SeverityLevel::info) << __func__ << ", clear notify flag" << endl;
    for (int i = 0; i < MAX_CARRIERS; i++)
    {
        mCarrierEqptFault[i].isNotified = false;
        mCarrierFacFault[i].isNotified = false;
    }
}

} /* DpAdapter */
