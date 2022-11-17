#include <chrono>
#include <thread>
#include <cmath>
#include "InfnLogger.h"
#include "DpPm.h"
#include "DcoCarrierAdapter.h"
#include "DcoCardAdapter.h"
#include "DcoGigeClientAdapter.h"
#include "DcoOduAdapter.h"
#include "DcoOtuAdapter.h"
#include "DcoGccControlAdapter.h"
#include "DcoXconAdapter.h"
#include "DcoPmSBAdapter.h"
#include "dp_common.h"
#include "DataPlaneMgr.h"

using namespace std;
using std::make_shared;
using std::make_unique;

using namespace chrono;
using namespace google::protobuf;
using namespace GnmiClient;
using namespace DataPlane;
using namespace dcoyang::infinera_dco_card_pm;
using namespace dcoyang::infinera_dco_carrier_pm;
using namespace dcoyang::infinera_dco_client_gige_pm;
using namespace dcoyang::infinera_dco_otu_pm;
using namespace dcoyang::infinera_dco_odu_pm;
using namespace dcoyang::infinera_dco_gcc_control_pm;
using cardState = dcoyang::infinera_dco_card::CardState;
using carrierState = ::dcoyang::enums::InfineraDcoCarrierOpticalState;
using oduOpStatus = dcoyang::infinera_dco_odu::OpStatus;
using otuOpStatus = dcoyang::infinera_dco_otu::OpStatus;
using gigeOpStatus = dcoyang::infinera_dco_client_gige::OpStatus;
using xconOpStatus = dcoyang::infinera_dco_xcon::OpStatus;

namespace DpAdapter {

//card sub callback context, please copy proto obj locally. obj will be cleaned up by client after callback context returns.
void CardPmResponseContext::HandleSubResponse(::google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoCardAdapter::mCardPmSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Card pm grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << __func__ << "CardPmResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoCardPm* cp = static_cast<dcoCardPm*>(&obj);
	int cpId = cp->name().value();
	auto dspTemp = cp->system_state().dsp_temperature();
	auto mcTemp = cp->system_state().module_case_temperature();
	auto picTemp = cp->system_state().pic_temperature();
	//Layer of conversion is required from Protobuf Msg to C++ data structure
	DcoCardAdapter::mCardPm.name = cpId;
	DcoCardAdapter::mCardPm.SystemState.DspTemperature =  (double)dspTemp.digits() / pow(10, dspTemp.precision());
	DcoCardAdapter::mCardPm.SystemState.PicTemperature =  (double)picTemp.digits() / pow(10, picTemp.precision());
	DcoCardAdapter::mCardPm.SystemState.ModuleCaseTemperature = (double)mcTemp.digits() / pow(10, mcTemp.precision());

	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoCardAdapter::mCardPm), &DcoCardAdapter::mCardPm);
}

//carrier sub callback context, please copy proto obj locally. obj will be cleaned up by client after callback context returns.
void CarrierPmResponseContext::HandleSubResponse(::google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	high_resolution_clock::time_point curStart;
	if (mPmTimeEnable)
	{
		curStart = std::chrono::high_resolution_clock::now();
		DcoCarrierAdapter::mStartSpan = duration_cast<milliseconds>( curStart - DcoCarrierAdapter::mStart ).count();
		if (DcoCarrierAdapter::mStartSpanArray.size() ==  spanTime) {
			DcoCarrierAdapter::mStartSpanArray.pop_front();
		}
		DcoCarrierAdapter::mStartSpanArray.emplace_back(DcoCarrierAdapter::mStartSpan);
		DcoCarrierAdapter::mStart = curStart;
	}

	if (!status.ok())
	{
		DcoCarrierAdapter::mCarrierPmSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Carrier pm grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << __func__ << " CarrierPmResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoCarrierPm* cp = static_cast<dcoCarrierPm*>(&obj);
	int size = cp->carrier_pm_size();
	int pmDataSize = DcoCarrierAdapter::mCarrierPms.GetPmDataSize();
	if (size > pmDataSize) {
        INFN_LOG(SeverityLevel::debug) << __func__ << ", invalid pm proto size: " << size << ", cached pm size: " << pmDataSize << std::endl;
	}
	for (int i = 0; i < size; i++) {
		auto carrierPmKey = cp->mutable_carrier_pm(i);
		int carrierId = carrierPmKey->carrier_id();
		if (DcoCarrierAdapter::mCarrierPms.HasPmData(carrierId) == false) {
            INFN_LOG(SeverityLevel::debug) << "cannot find pm data entry for carrierId: " << carrierId << endl;
			continue;
		}
		auto curCarrierPm = DcoCarrierAdapter::mCarrierPms.GetPmData(carrierId);
		DpMsCarrierPMContainer carrierPm;
		memset(&carrierPm, 0, sizeof(carrierPm));
		carrierPm.aid = curCarrierPm.aid;
		carrierPm.carrierId = carrierPmKey->carrier_id();
		carrierPm.updateCount = curCarrierPm.updateCount+1;

		auto protoCarrierPm = carrierPmKey->carrier_pm();
		auto protoRx = protoCarrierPm.rx();
		auto protoTx = protoCarrierPm.tx();

		using acqState = ::dcoyang::infinera_dco_carrier_pm::CarrierPms_CarrierPm_Rx;
		switch(protoRx.acquisition_state())
		{
			case acqState::ACQUISITIONSTATE_IDLE:
				carrierPm.rx.acquisition_state = DpMsCarrierRx::ACQ_IDLE;
				break;
			case acqState::ACQUISITIONSTATE_SIG_DETECT:
				carrierPm.rx.acquisition_state = DpMsCarrierRx::ACQ_SIG_DETECT;
				break;
			case acqState::ACQUISITIONSTATE_ACQUISITION:
				carrierPm.rx.acquisition_state = DpMsCarrierRx::ACQ_ACQUISITION;
				break;
			case acqState::ACQUISITIONSTATE_ADC_CAL:
				carrierPm.rx.acquisition_state = DpMsCarrierRx::ACQ_ADC_CAL;
				break;
			case acqState::ACQUISITIONSTATE_RX_ACTIVE:
				carrierPm.rx.acquisition_state = DpMsCarrierRx::ACQ_RX_ACTIVE;
				break;
			default:
				carrierPm.rx.acquisition_state = DpMsCarrierRx::ACQ_UNSPECIFIED;
				break;
		}

		carrierPm.rx.pre_fec_ber = (double)protoRx.pre_fec_bit_error_rate().digits() / (pow(10, protoRx.pre_fec_bit_error_rate().precision()) * BERSER_CORRECTION);
		carrierPm.rx.pre_fec_q = (double)protoRx.pre_fec_q_factor().digits() / pow(10, protoRx.pre_fec_q_factor().precision());
		carrierPm.rx.post_fec_q = (double)protoRx.post_fec_q_factor().digits() / pow(10, protoRx.post_fec_q_factor().precision());
		carrierPm.rx.snr = (double)protoRx.signal_to_noise_ratio().digits() / pow(10, protoRx.signal_to_noise_ratio().precision());
		carrierPm.rx.snr_avg_x = (double)protoRx.signal_to_noise_ratio_x_polarization().digits() / pow(10, protoRx.signal_to_noise_ratio_x_polarization().precision());
		carrierPm.rx.snr_avg_y = (double)protoRx.signal_to_noise_ratio_y_polarization().digits() / pow(10, protoRx.signal_to_noise_ratio_y_polarization().precision());
		carrierPm.rx.snr_inner = (double)protoRx.signal_to_noise_ratio_inner().digits() / pow(10, protoRx.signal_to_noise_ratio_inner().precision());
		carrierPm.rx.snr_outer = (double)protoRx.signal_to_noise_ratio_outer().digits() / pow(10, protoRx.signal_to_noise_ratio_outer().precision());
		carrierPm.rx.snr_diff_left = (double)protoRx.signal_to_noise_ratio_diff_left().digits() / pow(10, protoRx.signal_to_noise_ratio_diff_left().precision());
		carrierPm.rx.snr_diff_right = (double)protoRx.signal_to_noise_ratio_diff_right().digits() / pow(10, protoRx.signal_to_noise_ratio_diff_right().precision());
		carrierPm.rx.osnr = (double)protoRx.optical_signal_to_noise_ratio().digits() / pow(10, protoRx.optical_signal_to_noise_ratio().precision());
		carrierPm.rx.pass_frame_count = protoRx.pass_frame_count().value();
		carrierPm.rx.failed_frame_count = protoRx.fail_frame_count().value();
		carrierPm.rx.total_frame_count = protoRx.total_frame_count().value();
		carrierPm.rx.fas_q = (double)protoRx.fas_q_factor().digits() / pow(10, protoRx.fas_q_factor().precision());
		carrierPm.rx.cd = (double)protoRx.rx_chromatic_dispersion().digits() / pow(10, protoRx.rx_chromatic_dispersion().precision());
		carrierPm.tx.cd = (double)protoTx.tx_chromatic_dispersion().digits() / pow(10, protoTx.tx_chromatic_dispersion().precision());
		carrierPm.rx.polarization_dependent_loss = (double)protoRx.polarization_dependent_loss().digits() / pow(10, protoRx.polarization_dependent_loss().precision());
		carrierPm.rx.opr = (double)protoRx.receive_optical_power().digits() / pow(10, protoRx.receive_optical_power().precision());
		carrierPm.rx.laser_current = (double)protoRx.laser_current().digits() / pow(10, protoRx.laser_current().precision()) * AMPS_TO_MA;
		carrierPm.rx.dgd = (double)protoRx.differential_group_delay().digits() / pow(10, protoRx.differential_group_delay().precision());
		carrierPm.rx.so_dgd = (double)protoRx.differential_group_delay_so().digits() / pow(10, protoRx.differential_group_delay_so().precision());
		carrierPm.rx.phase_correction = (double)protoRx.cycle_slip_ratio().digits() / pow(10, protoRx.cycle_slip_ratio().precision());
		carrierPm.rx.corrected_bits = protoRx.total_corrected_bits().value();
		carrierPm.tx.frequency = (double)protoTx.tx_carrier_frequency().digits() / (pow(10, protoTx.tx_carrier_frequency().precision()) * HZ_TO_MHZ);
		carrierPm.rx.frequency = (double)protoRx.rx_carrier_frequency().digits() / (pow(10, protoRx.rx_carrier_frequency().precision()) * HZ_TO_MHZ);

		carrierPm.subCarrierCnt = protoRx.sub_carrier_count().value();
		for (int i = 0; i < protoRx.signal_to_noise_ratio_sub_carrier_list_size(); i++)
		{
			DpMsSubCarrierSnr subCarrierSnr;
			subCarrierSnr.subCarrierIdx = protoRx.signal_to_noise_ratio_sub_carrier_list(i).sub_carrier_index();
			if (subCarrierSnr.subCarrierIdx > 8 || subCarrierSnr.subCarrierIdx < 1) continue;
			auto snrSubCarrierListKey = protoRx.signal_to_noise_ratio_sub_carrier_list(i).signal_to_noise_ratio_sub_carrier_list();
			subCarrierSnr.subcarrier_snr = (double)snrSubCarrierListKey.signal_to_noise_ratio_sub_carrier().digits() / pow(10, snrSubCarrierListKey.signal_to_noise_ratio_sub_carrier().precision());

			carrierPm.rx.subcarrier_snr_list.at(subCarrierSnr.subCarrierIdx - 1) = subCarrierSnr;
		}

		carrierPm.tx.opt = (double)protoTx.transmit_optical_power().digits() / pow(10, protoTx.transmit_optical_power().precision());
		carrierPm.tx.laser_current = (double)protoTx.laser_current().digits() / pow(10, protoTx.laser_current().precision()) * AMPS_TO_MA;
		carrierPm.tx.edfa_input_power = (double)protoTx.edfa_input_power().digits() / pow(10, protoTx.edfa_input_power().precision());
		carrierPm.tx.edfa_output_power = (double)protoTx.edfa_output_power().digits() / pow(10, protoTx.edfa_output_power().precision());
		carrierPm.tx.edfa_pump_current = (double)protoTx.edfa_pump_current().digits() / pow(10, protoTx.edfa_pump_current().precision()) * AMPS_TO_MA;

                carrierPm.updateTime = std::chrono::system_clock::now();

		DcoCarrierAdapter::mCarrierPms.UpdatePmData(carrierPm, carrierId);

		auto carrierPmAccum = DcoCarrierAdapter::mCarrierPms.GetPmDataAccum(carrierId);
		if ((carrierPmAccum.accumEnable) || (mPmDebugEnable))
		{
			carrierPmAccum.rx.pass_frame_count += protoRx.pass_frame_count().value();
			carrierPmAccum.rx.failed_frame_count += protoRx.fail_frame_count().value();
			carrierPmAccum.rx.corrected_bits += protoRx.total_corrected_bits().value();
			DcoCarrierAdapter::mCarrierPms.UpdatePmDataAccum(carrierPmAccum, carrierId);
		}
	}

    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoCarrierAdapter::mCarrierPms), &DcoCarrierAdapter::mCarrierPms);

    if (mPmTimeEnable)
    {
	    auto curEnd = std::chrono::high_resolution_clock::now();
	    DcoCarrierAdapter::mEndSpan = duration_cast<milliseconds> ( curEnd - DcoCarrierAdapter::mEnd ).count();
	    if (DcoCarrierAdapter::mEndSpanArray.size() ==  spanTime) {
		    DcoCarrierAdapter::mEndSpanArray.pop_front();
	    }
	    DcoCarrierAdapter::mEndSpanArray.emplace_back(DcoGigeClientAdapter::mEndSpan);
	    DcoCarrierAdapter::mEnd = curEnd;
	    DcoCarrierAdapter::mDelta = duration_cast<milliseconds> ( curEnd - curStart ).count();
	    if (DcoCarrierAdapter::mDeltaArray.size() ==  spanTime) {
		    DcoCarrierAdapter::mDeltaArray.pop_front();
	    }
	    DcoCarrierAdapter::mDeltaArray.emplace_back(DcoCarrierAdapter::mDelta);
    }

}

//gige sub callback context, please copy proto obj locally. obj will be cleaned up by client after callback context returns.
void GigePmResponseContext::HandleSubResponse(::google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	high_resolution_clock::time_point curStart;
	if (mPmTimeEnable)
	{
		curStart = std::chrono::high_resolution_clock::now();
		DcoGigeClientAdapter::mStartSpan = duration_cast<milliseconds>( curStart - DcoGigeClientAdapter::mStart ).count();
		if (DcoGigeClientAdapter::mStartSpanArray.size() ==  spanTime) {
			DcoGigeClientAdapter::mStartSpanArray.pop_front();
		}
		DcoGigeClientAdapter::mStartSpanArray.emplace_back(DcoGigeClientAdapter::mStartSpan);
		DcoGigeClientAdapter::mStart = curStart;
	}

	if (!status.ok())
	{
		DcoGigeClientAdapter::mGigePmSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Gige pm grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << __func__ << " GigePmResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoGigePm* gp = static_cast<dcoGigePm*>(&obj);
	int size = gp->client_gige_pm_size();
	int pmDataSize = DcoGigeClientAdapter::mGigePms.GetPmDataSize();
	if (size > pmDataSize) {
        INFN_LOG(SeverityLevel::debug) << __func__ << ", invalid pm proto size: " << size << ", cached pm size: " << pmDataSize << std::endl;
	}

	for (int i = 0; i < size; i++) {
		auto gigePmKey = gp->mutable_client_gige_pm(i);
		int gigeId = gigePmKey->name();
		if (DcoGigeClientAdapter::mGigePms.HasPmData(gigeId) == false) {
			INFN_LOG(SeverityLevel::debug) << "cannot find pm data entry for gigeId: " << gigeId << endl;
			continue;
		}
		auto curGigePm = DcoGigeClientAdapter::mGigePms.GetPmData(gigeId);
		DpMsClientGigePmContainer gigePm;
		memset(&gigePm, 0, sizeof(gigePm));
		gigePm.aid = curGigePm.aid;
		gigePm.gigeId = gigeId;
		gigePm.updateCount = curGigePm.updateCount+1;

		auto protoGigerPm = gigePmKey->client_gige_pm();
		auto protoMacTx = protoGigerPm.dco_client_gige_pm_mac_tx();
		auto protoMacRx = protoGigerPm.dco_client_gige_pm_mac_rx();
		auto protoPcsTx = protoGigerPm.dco_client_gige_pm_pcs_tx();
		auto protoPcsRx = protoGigerPm.dco_client_gige_pm_pcs_rx();
		auto protoPhyFecRx = protoGigerPm.dco_client_gige_pm_phy_fec_rx();

		gigePm.DpMsClientGigePmMacRx.packets = protoMacRx.packets_received().value();
		gigePm.DpMsClientGigePmMacRx.error_packets = protoMacRx.errored_packets().value();
		gigePm.DpMsClientGigePmMacRx.ok_packets = protoMacRx.ok_packets().value();
		gigePm.DpMsClientGigePmMacRx.octets = protoMacRx.octets_received().value();
		gigePm.DpMsClientGigePmMacRx.error_octets = protoMacRx.errored_octets().value();
		gigePm.DpMsClientGigePmMacRx.jabber_frames = protoMacRx.jabber_frames().value();
		gigePm.DpMsClientGigePmMacRx.fragmented_frames = protoMacRx.fragmented_frames().value();
		gigePm.DpMsClientGigePmMacRx.crc_aligned_error = protoMacRx.fcs_error().value();
		gigePm.DpMsClientGigePmMacRx.under_sized = protoMacRx.under_sized().value();
		gigePm.DpMsClientGigePmMacRx.over_sized = protoMacRx.over_sized().value();
		gigePm.DpMsClientGigePmMacRx.broadcast_frames = protoMacRx.broadcast_frames().value();
		gigePm.DpMsClientGigePmMacRx.multicast_frames= protoMacRx.multicast_frames().value();
		gigePm.DpMsClientGigePmMacRx.unicast_frames = protoMacRx.unicast_frames().value();
		gigePm.DpMsClientGigePmMacRx.pause_frames = protoMacRx.pause_frames().value();
		gigePm.DpMsClientGigePmMacRx.bpdu_frames = protoMacRx.bpdu_frames().value();
		gigePm.DpMsClientGigePmMacRx.lacp_frames = protoMacRx.lacp_frames().value();
		gigePm.DpMsClientGigePmMacRx.oam_frames = protoMacRx.oam_frames().value();
		gigePm.DpMsClientGigePmMacRx.fcs_error = protoMacRx.fcs_error().value();
		gigePm.DpMsClientGigePmMacRx.vlan_ok = protoMacRx.vlan_received_ok().value();
		gigePm.DpMsClientGigePmMacRx.size_64 = protoMacRx.size_64().value();
		gigePm.DpMsClientGigePmMacRx.size_65_to_127 = protoMacRx.size_65_to_127().value();
		gigePm.DpMsClientGigePmMacRx.size_128_to_255= protoMacRx.size_128_to_255().value();
		gigePm.DpMsClientGigePmMacRx.size_256_to_511 = protoMacRx.size_256_to_511().value();
		gigePm.DpMsClientGigePmMacRx.size_512_to_1023 = protoMacRx.size_512_to_1023().value();
		gigePm.DpMsClientGigePmMacRx.size_1024_to_1518 = protoMacRx.size_1024_to_1518().value();
		gigePm.DpMsClientGigePmMacRx.jumbo_frames = protoMacRx.jumbo_frames().value();
		gigePm.DpMsClientGigePmMacRx.jumbo_to_extreme_jumbo_frames = protoMacRx.jumbo_to_extreme_jumbo_frames().value();

		gigePm.DpMsClientGigePmMacTx.packets = protoMacTx.packets_transmitted().value();
		gigePm.DpMsClientGigePmMacTx.error_packets = protoMacTx.errored_packets().value();
		gigePm.DpMsClientGigePmMacTx.ok_packets = protoMacTx.ok_packets().value();
		gigePm.DpMsClientGigePmMacTx.octets = protoMacTx.octets_transmitted().value();
		gigePm.DpMsClientGigePmMacTx.error_octets = protoMacTx.errored_octets().value();
		gigePm.DpMsClientGigePmMacTx.jabber_frames = protoMacTx.jabber_frames().value();
		gigePm.DpMsClientGigePmMacTx.fragmented_frames = protoMacTx.fragmented_frames().value();
		gigePm.DpMsClientGigePmMacTx.crc_aligned_error = protoMacTx.fcs_error().value();
		gigePm.DpMsClientGigePmMacTx.under_sized = protoMacTx.under_sized().value();
		gigePm.DpMsClientGigePmMacTx.over_sized = protoMacTx.over_sized().value();
		gigePm.DpMsClientGigePmMacTx.broadcast_frames = protoMacTx.broadcast_frames().value();
		gigePm.DpMsClientGigePmMacTx.multicast_frames= protoMacTx.multicast_frames().value();
		gigePm.DpMsClientGigePmMacTx.unicast_frames = protoMacTx.unicast_frames().value();
		gigePm.DpMsClientGigePmMacTx.pause_frames = protoMacTx.pause_frames().value();
		gigePm.DpMsClientGigePmMacTx.bpdu_frames = protoMacTx.bpdu_frames().value();
		gigePm.DpMsClientGigePmMacTx.lacp_frames = protoMacTx.lacp_frames().value();
		gigePm.DpMsClientGigePmMacTx.oam_frames = protoMacTx.oam_frames().value();
		gigePm.DpMsClientGigePmMacTx.fcs_error = protoMacTx.fcs_error().value();
		gigePm.DpMsClientGigePmMacTx.vlan_ok = protoMacTx.vlan_transmitted_ok().value();
		gigePm.DpMsClientGigePmMacTx.size_64 = protoMacTx.size_64().value();
		gigePm.DpMsClientGigePmMacTx.size_65_to_127 = protoMacTx.size_65_to_127().value();
		gigePm.DpMsClientGigePmMacTx.size_128_to_255= protoMacTx.size_128_to_255().value();
		gigePm.DpMsClientGigePmMacTx.size_256_to_511 = protoMacTx.size_256_to_511().value();
		gigePm.DpMsClientGigePmMacTx.size_512_to_1023 = protoMacTx.size_512_to_1023().value();
		gigePm.DpMsClientGigePmMacTx.size_1024_to_1518 = protoMacTx.size_1024_to_1518().value();
		gigePm.DpMsClientGigePmMacTx.jumbo_frames = protoMacTx.jumbo_frames().value();
		gigePm.DpMsClientGigePmMacTx.jumbo_to_extreme_jumbo_frames = protoMacTx.jumbo_to_extreme_jumbo_frames().value();

		gigePm.DpMsClientGigePmPcsRx.control_block = protoPcsRx.control_block().value();
		gigePm.DpMsClientGigePmPcsRx.errored_blocks = protoPcsRx.error_blocks().value();
		gigePm.DpMsClientGigePmPcsRx.idle_error = protoPcsRx.idle_error().value();
		gigePm.DpMsClientGigePmPcsRx.test_pattern_error = protoPcsRx.test_pattern_error().value();
		gigePm.DpMsClientGigePmPcsRx.bip_total = protoPcsRx.bip_total().value();

		gigePm.DpMsClientGigePmPcsTx.control_block = protoPcsTx.control_block().value();
		gigePm.DpMsClientGigePmPcsTx.errored_blocks = protoPcsTx.error_blocks().value();
		gigePm.DpMsClientGigePmPcsTx.idle_error = protoPcsTx.idle_error().value();
		gigePm.DpMsClientGigePmPcsTx.test_pattern_error = protoPcsTx.test_pattern_error().value();
		gigePm.DpMsClientGigePmPcsTx.bip_total = protoPcsTx.bip_total().value();

		gigePm.DpMsClientGigePmPhyFecRx.fec_corrected = protoPhyFecRx.fec_corrected().value();
		gigePm.DpMsClientGigePmPhyFecRx.fec_un_corrected = protoPhyFecRx.fec_un_corrected().value();
		gigePm.DpMsClientGigePmPhyFecRx.bit_error = protoPhyFecRx.bit_error().value();
		gigePm.DpMsClientGigePmPhyFecRx.pre_fec_ber = (double)protoPhyFecRx.pre_fec_ber().digits() / pow(10, protoPhyFecRx.pre_fec_ber().precision()) ;
		gigePm.DpMsClientGigePmPhyFecRx.fec_symbol_error = protoPhyFecRx.fec_symbol_error().value();
		gigePm.DpMsClientGigePmPhyFecRx.fec_symbol_error_rate = (double)protoPhyFecRx.fec_symbol_error_rate().digits() / pow(10, protoPhyFecRx.fec_symbol_error_rate().precision());

                gigePm.updateTime = std::chrono::system_clock::now();

		DcoGigeClientAdapter::mGigePms.UpdatePmData(gigePm, gigeId);

		auto gigePmAccum = DcoGigeClientAdapter::mGigePms.GetPmDataAccum(gigeId);
		if ((gigePmAccum.accumEnable) || (mPmDebugEnable))
		{
			gigePmAccum.DpMsClientGigePmMacRx.packets += protoMacRx.packets_received().value();
			gigePmAccum.DpMsClientGigePmMacRx.error_packets += protoMacRx.errored_packets().value();
			gigePmAccum.DpMsClientGigePmMacRx.ok_packets += protoMacRx.ok_packets().value();
			gigePmAccum.DpMsClientGigePmMacRx.octets += protoMacRx.octets_received().value();
			gigePmAccum.DpMsClientGigePmMacRx.error_octets += protoMacRx.errored_octets().value();
			gigePmAccum.DpMsClientGigePmMacRx.jabber_frames += protoMacRx.jabber_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.fragmented_frames += protoMacRx.fragmented_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.crc_aligned_error += protoMacRx.fcs_error().value();
			gigePmAccum.DpMsClientGigePmMacRx.under_sized += protoMacRx.under_sized().value();
			gigePmAccum.DpMsClientGigePmMacRx.over_sized += protoMacRx.over_sized().value();
			gigePmAccum.DpMsClientGigePmMacRx.broadcast_frames += protoMacRx.broadcast_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.multicast_frames += protoMacRx.multicast_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.unicast_frames += protoMacRx.unicast_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.pause_frames += protoMacRx.pause_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.bpdu_frames += protoMacRx.bpdu_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.lacp_frames += protoMacRx.lacp_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.oam_frames += protoMacRx.oam_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.fcs_error += protoMacRx.fcs_error().value();
			gigePmAccum.DpMsClientGigePmMacRx.vlan_ok += protoMacRx.vlan_received_ok().value();
			gigePmAccum.DpMsClientGigePmMacRx.size_64 += protoMacRx.size_64().value();
			gigePmAccum.DpMsClientGigePmMacRx.size_65_to_127 += protoMacRx.size_65_to_127().value();
			gigePmAccum.DpMsClientGigePmMacRx.size_128_to_255 += protoMacRx.size_128_to_255().value();
			gigePmAccum.DpMsClientGigePmMacRx.size_256_to_511 += protoMacRx.size_256_to_511().value();
			gigePmAccum.DpMsClientGigePmMacRx.size_512_to_1023 += protoMacRx.size_512_to_1023().value();
			gigePmAccum.DpMsClientGigePmMacRx.size_1024_to_1518 += protoMacRx.size_1024_to_1518().value();
			gigePmAccum.DpMsClientGigePmMacRx.jumbo_frames += protoMacRx.jumbo_frames().value();
			gigePmAccum.DpMsClientGigePmMacRx.jumbo_to_extreme_jumbo_frames += protoMacRx.jumbo_to_extreme_jumbo_frames().value();

			gigePmAccum.DpMsClientGigePmMacTx.packets += protoMacTx.packets_transmitted().value();
			gigePmAccum.DpMsClientGigePmMacTx.error_packets += protoMacTx.errored_packets().value();
			gigePmAccum.DpMsClientGigePmMacTx.ok_packets += protoMacTx.ok_packets().value();
			gigePmAccum.DpMsClientGigePmMacTx.octets += protoMacTx.octets_transmitted().value();
			gigePmAccum.DpMsClientGigePmMacTx.error_octets += protoMacTx.errored_octets().value();
			gigePmAccum.DpMsClientGigePmMacTx.jabber_frames += protoMacTx.jabber_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.fragmented_frames += protoMacTx.fragmented_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.crc_aligned_error += protoMacTx.fcs_error().value();
			gigePmAccum.DpMsClientGigePmMacTx.under_sized += protoMacTx.under_sized().value();
			gigePmAccum.DpMsClientGigePmMacTx.over_sized += protoMacTx.over_sized().value();
			gigePmAccum.DpMsClientGigePmMacTx.broadcast_frames += protoMacTx.broadcast_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.multicast_frames += protoMacTx.multicast_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.unicast_frames += protoMacTx.unicast_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.pause_frames += protoMacTx.pause_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.bpdu_frames += protoMacTx.bpdu_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.lacp_frames += protoMacTx.lacp_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.oam_frames += protoMacTx.oam_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.fcs_error += protoMacTx.fcs_error().value();
			gigePmAccum.DpMsClientGigePmMacTx.vlan_ok += protoMacTx.vlan_transmitted_ok().value();
			gigePmAccum.DpMsClientGigePmMacTx.size_64 += protoMacTx.size_64().value();
			gigePmAccum.DpMsClientGigePmMacTx.size_65_to_127 += protoMacTx.size_65_to_127().value();
			gigePmAccum.DpMsClientGigePmMacTx.size_128_to_255 += protoMacTx.size_128_to_255().value();
			gigePmAccum.DpMsClientGigePmMacTx.size_256_to_511 += protoMacTx.size_256_to_511().value();
			gigePmAccum.DpMsClientGigePmMacTx.size_512_to_1023 += protoMacTx.size_512_to_1023().value();
			gigePmAccum.DpMsClientGigePmMacTx.size_1024_to_1518 += protoMacTx.size_1024_to_1518().value();
			gigePmAccum.DpMsClientGigePmMacTx.jumbo_frames += protoMacTx.jumbo_frames().value();
			gigePmAccum.DpMsClientGigePmMacTx.jumbo_to_extreme_jumbo_frames += protoMacTx.jumbo_to_extreme_jumbo_frames().value();

			gigePmAccum.DpMsClientGigePmPcsRx.control_block += protoPcsRx.control_block().value();
			gigePmAccum.DpMsClientGigePmPcsRx.errored_blocks += protoPcsRx.error_blocks().value();
			gigePmAccum.DpMsClientGigePmPcsRx.idle_error += protoPcsRx.idle_error().value();
			gigePmAccum.DpMsClientGigePmPcsRx.test_pattern_error += protoPcsRx.test_pattern_error().value();
			gigePmAccum.DpMsClientGigePmPcsRx.bip_total += protoPcsRx.bip_total().value();

			gigePmAccum.DpMsClientGigePmPcsTx.control_block += protoPcsTx.control_block().value();
			gigePmAccum.DpMsClientGigePmPcsTx.errored_blocks += protoPcsTx.error_blocks().value();
			gigePmAccum.DpMsClientGigePmPcsTx.idle_error += protoPcsTx.idle_error().value();
			gigePmAccum.DpMsClientGigePmPcsTx.test_pattern_error += protoPcsTx.test_pattern_error().value();
			gigePmAccum.DpMsClientGigePmPcsTx.bip_total += protoPcsTx.bip_total().value();

			//400g fec pm
			gigePmAccum.DpMsClientGigePmPhyFecRx.fec_corrected += protoPhyFecRx.fec_corrected().value();
			gigePmAccum.DpMsClientGigePmPhyFecRx.fec_un_corrected += protoPhyFecRx.fec_un_corrected().value();
			gigePmAccum.DpMsClientGigePmPhyFecRx.bit_error += protoPhyFecRx.bit_error().value();
			gigePmAccum.DpMsClientGigePmPhyFecRx.fec_symbol_error += protoPhyFecRx.fec_symbol_error().value();

			DcoGigeClientAdapter::mGigePms.UpdatePmDataAccum(gigePmAccum, gigeId);
		}
	}

	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoGigeClientAdapter::mGigePms), &DcoGigeClientAdapter::mGigePms);

	if (mPmTimeEnable)
	{
		auto curEnd = std::chrono::high_resolution_clock::now();
		DcoGigeClientAdapter::mEndSpan = duration_cast<milliseconds> ( curEnd - DcoGigeClientAdapter::mEnd ).count();
		if (DcoGigeClientAdapter::mEndSpanArray.size() ==  spanTime) {
			DcoGigeClientAdapter::mEndSpanArray.pop_front();
		}
		DcoGigeClientAdapter::mEndSpanArray.emplace_back(DcoGigeClientAdapter::mEndSpan);
		DcoGigeClientAdapter::mEnd = curEnd;
		DcoGigeClientAdapter::mDelta = duration_cast<milliseconds> ( curEnd - curStart ).count();
		if (DcoGigeClientAdapter::mDeltaArray.size() ==  spanTime) {
			DcoGigeClientAdapter::mDeltaArray.pop_front();
		}
		DcoGigeClientAdapter::mDeltaArray.emplace_back(DcoGigeClientAdapter::mDelta);
	}
}

//otu sub callback context, please copy proto obj locally. obj will be cleaned up by client after callback context returns.
void OtuPmResponseContext::HandleSubResponse(::google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	high_resolution_clock::time_point curStart;
	if (mPmTimeEnable)
	{
		curStart = std::chrono::high_resolution_clock::now();
		DcoOtuAdapter::mStartSpan = duration_cast<milliseconds>( curStart - DcoOtuAdapter::mStart ).count();
		if (DcoOtuAdapter::mStartSpanArray.size() ==  spanTime) {
			DcoOtuAdapter::mStartSpanArray.pop_front();
		}
		DcoOtuAdapter::mStartSpanArray.emplace_back(DcoOtuAdapter::mStartSpan);
		DcoOtuAdapter::mStart = curStart;
	}

	if (!status.ok())
	{
		INFN_LOG(SeverityLevel::info) << " Otu pm grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		DcoOtuAdapter::mOtuPmSubEnable = false;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << __func__ << " OtuPmResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOtuPm* op = static_cast<dcoOtuPm*>(&obj);
	int size = op->otu_pm_size();
	int pmDataSize = DcoOtuAdapter::mOtuPms.GetPmDataSize();
	if (size > pmDataSize) {
	    INFN_LOG(SeverityLevel::debug) << __func__ << ", invalid pm proto size: " << size << ", cached pm size: " << pmDataSize << std::endl;
	}

	for (int i = 0; i < size; i++) {
		auto otuPmKey = op->mutable_otu_pm(i);
		int otuId = otuPmKey->otu_id();
		if (DcoOtuAdapter::mOtuPms.HasPmData(otuId) == false) {
	        INFN_LOG(SeverityLevel::debug) << "cannot find pm data entry for otuId: " << otuId << endl;
			continue;
		}
		auto curOtuPm = DcoOtuAdapter::mOtuPms.GetPmData(otuId);
		DpMsOtuPMSContainer otuPm;
		memset(&otuPm, 0, sizeof(otuPm));
		otuPm.aid = curOtuPm.aid;
		otuPm.otuId = otuId;
		otuPm.updateCount = curOtuPm.updateCount+1;
		otuPm.valid = true; //mark this entry is valid for this transaction

		auto protoOtuPm = otuPmKey->otu_pm();
		auto protoRxPmCommon = protoOtuPm.rx();
		auto protoRxPmFec = protoOtuPm.rx();
		auto protoTxPmCommon = protoOtuPm.tx();

		otuPm.rx.pmcommon.bip_err = protoRxPmCommon.bip_err().value();
		otuPm.rx.pmcommon.err_blocks = protoRxPmCommon.err_blocks().value();
		otuPm.rx.pmcommon.far_end_err_blocks = protoRxPmCommon.far_end_err_blocks().value();
		otuPm.rx.pmcommon.bei = protoRxPmCommon.bei().value();
		otuPm.rx.pmcommon.iae = protoRxPmCommon.iae().value();
		otuPm.rx.pmcommon.biae = protoRxPmCommon.biae().value();
		otuPm.rx.pmcommon.prbs_sync_err_count = protoRxPmCommon.prbs_sync_err_count().value();
		otuPm.rx.pmcommon.prbs_err_count = protoRxPmCommon.prbs_err_count().value();
		otuPm.rx.pmcommon.latency = (double) protoRxPmCommon.latency().digits() / pow(10, protoRxPmCommon.latency().precision());

		otuPm.rx.fecstat.num_collected = protoRxPmFec.num_collected().value();
		otuPm.rx.fecstat.corrected_bits = protoRxPmFec.corrected_bits().value();
		otuPm.rx.fecstat.corrected_1 = protoRxPmFec.corrected_1().value();
		otuPm.rx.fecstat.corrected_0 = protoRxPmFec.corrected_0().value();
		otuPm.rx.fecstat.uncorrected_words = protoRxPmFec.uncorrected_words().value();
		otuPm.rx.fecstat.uncorrected_bits = protoRxPmFec.uncorrected_bits().value();
		otuPm.rx.fecstat.total_code_words = protoRxPmFec.total_code_words().value();
		otuPm.rx.fecstat.code_word_size = protoRxPmFec.code_word_size().value();
		otuPm.rx.fecstat.total_bits = protoRxPmFec.total_bits().value();
		otuPm.rx.fecstat.pre_fec_ber = (double) protoRxPmCommon.pre_fec_ber().digits() / (pow(10, protoRxPmCommon.pre_fec_ber().precision()) * BERSER_CORRECTION);

		otuPm.tx.bip_err = protoTxPmCommon.bip_err().value();
		otuPm.tx.err_blocks = protoTxPmCommon.err_blocks().value();
		otuPm.tx.far_end_err_blocks = protoTxPmCommon.far_end_err_blocks().value();
		otuPm.tx.bei = protoTxPmCommon.bei().value();
		otuPm.tx.iae = protoTxPmCommon.iae().value();
		otuPm.tx.biae = protoTxPmCommon.biae().value();
		otuPm.tx.prbs_sync_err_count = protoTxPmCommon.prbs_sync_err_count().value();
		otuPm.tx.prbs_err_count = protoTxPmCommon.prbs_err_count().value();
		otuPm.tx.latency = (double) protoTxPmCommon.latency().digits() / pow(10, protoTxPmCommon.latency().precision());

                otuPm.updateTime = std::chrono::system_clock::now();

		DcoOtuAdapter::mOtuPms.UpdatePmData(otuPm, otuId);

		auto otuPmAccum = DcoOtuAdapter::mOtuPms.GetPmDataAccum(otuId);
		if ((otuPmAccum.accumEnable) || (mPmDebugEnable))
		{
			otuPmAccum.rx.pmcommon.bip_err += protoRxPmCommon.bip_err().value();
			otuPmAccum.rx.pmcommon.err_blocks += protoRxPmCommon.err_blocks().value();
			otuPmAccum.rx.pmcommon.far_end_err_blocks += protoRxPmCommon.far_end_err_blocks().value();
			otuPmAccum.rx.pmcommon.bei += protoRxPmCommon.bei().value();
			otuPmAccum.rx.pmcommon.prbs_sync_err_count += protoRxPmCommon.prbs_sync_err_count().value();
			otuPmAccum.rx.pmcommon.prbs_err_count += protoRxPmCommon.prbs_err_count().value();

			otuPmAccum.rx.fecstat.num_collected += protoRxPmFec.num_collected().value();
			otuPmAccum.rx.fecstat.corrected_bits += protoRxPmFec.corrected_bits().value();
			otuPmAccum.rx.fecstat.corrected_1 += protoRxPmFec.corrected_1().value();
			otuPmAccum.rx.fecstat.corrected_0 += protoRxPmFec.corrected_0().value();
			otuPmAccum.rx.fecstat.uncorrected_words += protoRxPmFec.uncorrected_words().value();
			otuPmAccum.rx.fecstat.uncorrected_bits += protoRxPmFec.uncorrected_bits().value();
			otuPmAccum.rx.fecstat.total_code_words += protoRxPmFec.total_code_words().value();
			otuPmAccum.rx.fecstat.code_word_size += protoRxPmFec.code_word_size().value();
			otuPmAccum.rx.fecstat.total_bits += protoRxPmFec.total_bits().value();

			otuPmAccum.tx.bip_err += protoTxPmCommon.bip_err().value();
			otuPmAccum.tx.err_blocks += protoTxPmCommon.err_blocks().value();
			otuPmAccum.tx.far_end_err_blocks += protoTxPmCommon.far_end_err_blocks().value();
			otuPmAccum.tx.bei += protoTxPmCommon.bei().value();
			otuPmAccum.tx.prbs_sync_err_count += protoTxPmCommon.prbs_sync_err_count().value();
			otuPmAccum.tx.prbs_err_count += protoTxPmCommon.prbs_err_count().value();

			DcoOtuAdapter::mOtuPms.UpdatePmDataAccum(otuPmAccum, otuId);
		}
	}

	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoOtuAdapter::mOtuPms), &DcoOtuAdapter::mOtuPms);

	if (mPmTimeEnable)
	{
		auto curEnd = std::chrono::high_resolution_clock::now();
		DcoOtuAdapter::mEndSpan = duration_cast<milliseconds> ( curEnd - DcoOtuAdapter::mEnd ).count();
		if (DcoOtuAdapter::mEndSpanArray.size() ==  spanTime) {
			DcoOtuAdapter::mEndSpanArray.pop_front();
		}
		DcoOtuAdapter::mEndSpanArray.emplace_back(DcoGigeClientAdapter::mEndSpan);
		DcoOtuAdapter::mEnd = curEnd;
		DcoOtuAdapter::mDelta = duration_cast<milliseconds> ( curEnd - curStart ).count();
		if (DcoOtuAdapter::mDeltaArray.size() ==  spanTime) {
			DcoOtuAdapter::mDeltaArray.pop_front();
		}
		DcoOtuAdapter::mDeltaArray.emplace_back(DcoOtuAdapter::mDelta);
	}
}

//odu sub callback context, please copy proto obj locally. obj will be cleaned up by client after callback context returns.
void OduPmResponseContext::HandleSubResponse(::google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	high_resolution_clock::time_point curStart;
	if (mPmTimeEnable)
	{
		curStart = std::chrono::high_resolution_clock::now();
		DcoOduAdapter::mStartSpan = duration_cast<milliseconds>( curStart - DcoOduAdapter::mStart ).count();
		if (DcoOduAdapter::mStartSpanArray.size() ==  spanTime) {
			DcoOduAdapter::mStartSpanArray.pop_front();
		}
		DcoOduAdapter::mStartSpanArray.emplace_back(DcoOduAdapter::mStartSpan);
		DcoOduAdapter::mStart = curStart;
	}

	if (!status.ok())
	{
		INFN_LOG(SeverityLevel::info) << " Odu pm grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		DcoOduAdapter::mOduPmSubEnable = false;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << __func__ << " OduPmResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOduPm* op = static_cast<dcoOduPm*>(&obj);
	int size = op->odu_pm_size();
	int pmDataSize = DcoOduAdapter::mOduPms.GetPmDataSize();
	if (size > pmDataSize) {
		INFN_LOG(SeverityLevel::debug) << __func__ << ", invalid pm proto size: " << size << ", cached pm size: " << pmDataSize << std::endl;
	}

	for (int i = 0; i < size; i++) {
		auto oduPmKey = op->mutable_odu_pm(i);
		int oduId = oduPmKey->odu_id();
		if (DcoOduAdapter::mOduPms.HasPmData(oduId) == false) {
			INFN_LOG(SeverityLevel::debug) << "cannot find pm data entry for oduId: " << oduId<< endl;
			continue;
		}
		auto curOduPm = DcoOduAdapter::mOduPms.GetPmData(oduId);
		DpMsOduPMContainer oduPm;
		memset(&oduPm, 0, sizeof(oduPm));
		oduPm.aid = curOduPm.aid;
		oduPm.oduId = oduId;
		oduPm.updateCount = curOduPm.updateCount+1;
		oduPm.valid = true; //mark this entry is valid for this transaction

		auto protoOduPmRx = oduPmKey->odu_pm().rx();
		auto protoOduPmTx = oduPmKey->odu_pm().tx();

		oduPm.rx.bip_err = protoOduPmRx.bip_err().value();
		oduPm.rx.err_blocks = protoOduPmRx.err_blocks().value();
		oduPm.rx.far_end_err_blocks = protoOduPmRx.far_end_err_blocks().value();
		oduPm.rx.bei = protoOduPmRx.bei().value();
		oduPm.rx.iae = protoOduPmRx.iae().value();
		oduPm.rx.biae = protoOduPmRx.biae().value();
		oduPm.rx.prbs_sync_err_count = protoOduPmRx.prbs_sync_err_count().value();
		oduPm.rx.prbs_err_count = protoOduPmRx.prbs_err_count().value();
		oduPm.rx.latency = (double)protoOduPmRx.latency().digits() / pow(10, protoOduPmRx.latency().precision());

		oduPm.tx.bip_err = protoOduPmTx.bip_err().value();
		oduPm.tx.err_blocks = protoOduPmTx.err_blocks().value();
		oduPm.tx.far_end_err_blocks = protoOduPmTx.far_end_err_blocks().value();
		oduPm.tx.bei = protoOduPmTx.bei().value();
		oduPm.tx.iae = protoOduPmTx.iae().value();
		oduPm.tx.biae = protoOduPmTx.biae().value();
		oduPm.tx.prbs_sync_err_count = protoOduPmTx.prbs_sync_err_count().value();
		oduPm.tx.prbs_err_count = protoOduPmTx.prbs_err_count().value();
		oduPm.tx.latency = (double)protoOduPmTx.latency().digits() / pow(10, protoOduPmTx.latency().precision());

                oduPm.updateTime = std::chrono::system_clock::now();

		DcoOduAdapter::mOduPms.UpdatePmData(oduPm, oduId);

		auto oduPmAccum = DcoOduAdapter::mOduPms.GetPmDataAccum(oduId);
		if ((oduPmAccum.accumEnable) || (mPmDebugEnable))
		{
			oduPmAccum.rx.bip_err += protoOduPmRx.bip_err().value();
			oduPmAccum.rx.err_blocks += protoOduPmRx.err_blocks().value();
			oduPmAccum.rx.far_end_err_blocks += protoOduPmRx.far_end_err_blocks().value();
			oduPmAccum.rx.bei += protoOduPmRx.bei().value();
			oduPmAccum.rx.prbs_sync_err_count += protoOduPmRx.prbs_sync_err_count().value();
			oduPmAccum.rx.prbs_err_count += protoOduPmRx.prbs_err_count().value();

			oduPmAccum.tx.bip_err += protoOduPmTx.bip_err().value();
			oduPmAccum.tx.err_blocks += protoOduPmTx.err_blocks().value();
			oduPmAccum.tx.far_end_err_blocks += protoOduPmTx.far_end_err_blocks().value();
			oduPmAccum.tx.bei += protoOduPmTx.bei().value();
			oduPmAccum.tx.prbs_sync_err_count += protoOduPmTx.prbs_sync_err_count().value();
			oduPmAccum.tx.prbs_err_count += protoOduPmTx.prbs_err_count().value();

			DcoOduAdapter::mOduPms.UpdatePmDataAccum(oduPmAccum, oduId);
		}
	}

	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoOduAdapter::mOduPms), &DcoOduAdapter::mOduPms);

	if (mPmTimeEnable)
	{
		auto curEnd = std::chrono::high_resolution_clock::now();
		DcoOduAdapter::mEndSpan = duration_cast<milliseconds> ( curEnd - DcoOduAdapter::mEnd ).count();
		if (DcoOduAdapter::mEndSpanArray.size() ==  spanTime) {
			DcoOduAdapter::mEndSpanArray.pop_front();
		}
		DcoOduAdapter::mEndSpanArray.emplace_back(DcoGigeClientAdapter::mEndSpan);
		DcoOduAdapter::mEnd = curEnd;
		DcoOduAdapter::mDelta = duration_cast<milliseconds> ( curEnd - curStart ).count();
		if (DcoOduAdapter::mDeltaArray.size() ==  spanTime) {
			DcoOduAdapter::mDeltaArray.pop_front();
		}
		DcoOduAdapter::mDeltaArray.emplace_back(DcoOduAdapter::mDelta);
	}
}

//gcc sub callback context, please copy proto obj locally. obj will be cleaned up by client after callback context returns.
void GccPmResponseContext::HandleSubResponse(::google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	high_resolution_clock::time_point curStart;
	if (mPmTimeEnable)
	{
		curStart = std::chrono::high_resolution_clock::now();
		DcoGccControlAdapter::mStartSpan = duration_cast<milliseconds>( curStart - DcoGccControlAdapter::mStart ).count();
		if (DcoGccControlAdapter::mStartSpanArray.size() ==  spanTime) {
			DcoGccControlAdapter::mStartSpanArray.pop_front();
		}
		DcoGccControlAdapter::mStartSpanArray.emplace_back(DcoGccControlAdapter::mStartSpan);
		DcoGccControlAdapter::mStart = curStart;
	}

	if (!status.ok())
	{
		INFN_LOG(SeverityLevel::info) << " Gcc pm grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << __func__ << " GccPmResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoGccPm* op = static_cast<dcoGccPm*>(&obj);
	int size = op->gcc_control_pm_size();
	int pmDataSize = DcoGccControlAdapter::mGccPms.GetPmDataSize();
	if (size > pmDataSize) {
		INFN_LOG(SeverityLevel::debug) << __func__ << ", invalid pm proto size: " << size << ", cached pm size: " << pmDataSize << std::endl;
	}

	for (int i = 0; i < size; i++) {
		auto gccPmKey = op->mutable_gcc_control_pm(i);
		int carrierId = gccPmKey->carrier_id();
		if (DcoGccControlAdapter::mGccPms.HasPmData(carrierId) == false) {
			INFN_LOG(SeverityLevel::debug) << "cannot find pm data entry for carrierId: " << carrierId<< endl;
			continue;
		}
		auto curGccPm = DcoGccControlAdapter::mGccPms.GetPmData(carrierId);
		DpMsGccPMContainer gccPm;
		memset(&gccPm, 0, sizeof(gccPm));
		gccPm.carrierId = carrierId;
		gccPm.aid = curGccPm.aid;

		auto protoGccPm = gccPmKey->gcc_control_pm();
		gccPm.controlpm.tx_packets = protoGccPm.tx_packets_processed().value();
		gccPm.controlpm.rx_packets = protoGccPm.rx_packets_processed().value();
		gccPm.controlpm.tx_packets_dropped = protoGccPm.tx_packets_dropped().value();
		gccPm.controlpm.rx_packets_dropped = protoGccPm.rx_packets_dropped().value();
		gccPm.controlpm.tx_bytes = protoGccPm.tx_bytes().value();
		gccPm.controlpm.rx_bytes = protoGccPm.rx_bytes().value();
		DcoGccControlAdapter::mGccPms.UpdatePmData(gccPm, carrierId);

		if (mPmDebugEnable)
		{
			//accumulate pm, for debug purpose only
			auto gccPmAccum = DcoGccControlAdapter::mGccPms.GetPmDataAccum(carrierId);

			gccPm.controlpm.tx_packets += protoGccPm.tx_packets_processed().value();
			gccPm.controlpm.rx_packets += protoGccPm.rx_packets_processed().value();
			gccPmAccum.controlpm.tx_packets += protoGccPm.tx_packets_processed().value();
			gccPmAccum.controlpm.rx_packets += protoGccPm.rx_packets_processed().value();
			gccPmAccum.controlpm.tx_packets_dropped += protoGccPm.tx_packets_dropped().value();
			gccPmAccum.controlpm.rx_packets_dropped += protoGccPm.rx_packets_dropped().value();
			gccPmAccum.controlpm.tx_bytes += protoGccPm.tx_bytes().value();
			gccPmAccum.controlpm.rx_bytes += protoGccPm.rx_bytes().value();

			DcoGccControlAdapter::mGccPms.UpdatePmDataAccum(gccPmAccum, carrierId);
		}

	}

	DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
	dpMsPmCallbacksSingleton->CallCallback(CLASS_NAME(DcoGccControlAdapter::mGccPms), &DcoGccControlAdapter::mGccPms);

	if (mPmTimeEnable)
	{
		auto curEnd = std::chrono::high_resolution_clock::now();
		DcoGccControlAdapter::mEndSpan = duration_cast<milliseconds> ( curEnd - DcoGccControlAdapter::mEnd ).count();
		if (DcoGccControlAdapter::mEndSpanArray.size() ==  spanTime) {
			DcoGccControlAdapter::mEndSpanArray.pop_front();
		}
		DcoGccControlAdapter::mEndSpanArray.emplace_back(DcoGigeClientAdapter::mEndSpan);
		DcoGccControlAdapter::mEnd = curEnd;
		DcoGccControlAdapter::mDelta = duration_cast<milliseconds> ( curEnd - curStart ).count();
		if (DcoGccControlAdapter::mDeltaArray.size() ==  spanTime) {
			DcoGccControlAdapter::mDeltaArray.pop_front();
		}
		DcoGccControlAdapter::mDeltaArray.emplace_back(DcoGccControlAdapter::mDelta);
	}
}

void GigeFaultResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoGigeClientAdapter::mGigeFltSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Gige Flt grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " GigeFaultResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoGigeFault* gigeflt = static_cast<dcoGigeFault*>(&obj);
	if (gigeflt->has_client_port_id())
	{
		int gigeId = gigeflt->client_port_id().value();
		if (gigeId >= 1 && gigeId <= MAX_CLIENTS)
		{
			if (gigeflt->has_facility_fault())
			{
				uint64_t facflt = gigeflt->facility_fault().value();
				DcoGigeClientAdapter::mGigeFault[gigeId - 1].faultBitmap = facflt;
				DcoGigeClientAdapter::mGigeFault[gigeId - 1].isNotified = true;
				INFN_LOG(SeverityLevel::info) << " gige " << gigeId << " update adapter cache facflt: 0x" << hex << facflt << std::dec << std::endl;
			}
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid gigeId: " << gigeId << std::endl;
		}
	}
}

void OduFaultResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoOduAdapter::mOduFltSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Odu Flt grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " OduFaultResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOduFault* oduflt = static_cast<dcoOduFault*>(&obj);
	if (oduflt->has_odu_id())
	{
		int oduId = oduflt->odu_id().value();
		if (oduId >= 1 && oduId <= MAX_ODU_ID)
		{
			if (oduflt->has_facility_fault())
			{
				uint64_t facflt = oduflt->facility_fault().value();
				DcoOduAdapter::mOduFault[oduId - 1].faultBitmap = facflt;
				DcoOduAdapter::mOduFault[oduId - 1].isNotified = true;
				INFN_LOG(SeverityLevel::info) << " odu " << oduId << " update adapter cache facflt: 0x" << hex << facflt << std::dec << std::endl;
			}
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid oduId : " << oduId << std::endl;
		}
	}
}

void OtuFaultResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoOtuAdapter::mOtuFltSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Otu Flt grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " OtuFaultResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOtuFault* otuflt = static_cast<dcoOtuFault*>(&obj);
	if (otuflt->has_otu_id())
	{
		int otuId = otuflt->otu_id().value();
		if (otuId >= 1 && otuId <= MAX_OTU_ID)
		{
			if (otuflt->has_facility_fault())
			{
				uint64_t facflt = otuflt->facility_fault().value();
				DcoOtuAdapter::mOtuFault[otuId - 1].faultBitmap = facflt;
				DcoOtuAdapter::mOtuFault[otuId - 1].isNotified = true;
				LOG_FLT() << " otu " << otuId << " update adapter cache facflt: 0x" << hex << facflt << std::dec << std::endl;
			}
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid otuId : " << otuId << std::endl;
		}
	}
}

void CardFaultResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoCardAdapter::mCardFltSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Card Flt grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " CardFaultResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoCardFault* cardflt = static_cast<dcoCardFault*>(&obj);
	if (cardflt->has_equipment_fault())
	{
		uint64_t eqptflt = cardflt->equipment_fault().value();
		DcoCardAdapter::mCardEqptFault.faultBitmap = eqptflt;
		DcoCardAdapter::mCardEqptFault.isNotified = true;
//		LOG_FLT() << " update card cache eqptflt: 0x" << hex << eqptflt << std::dec << std::endl;
	}
	if (cardflt->has_dac_equipment_fault())
	{
		uint64_t daceqptflt = cardflt->dac_equipment_fault().value();
		DcoCardAdapter::mCardDacEqptFault.faultBitmap = daceqptflt;
		DcoCardAdapter::mCardDacEqptFault.isNotified = true;
//		LOG_FLT() << " update card cache daceqptflt: 0x" << hex << daceqptflt << std::dec << std::endl;
	}
	if (cardflt->has_ps_equipment_fault())
	{
		uint64_t pseqptflt = cardflt->ps_equipment_fault().value();
		DcoCardAdapter::mCardPsEqptFault.faultBitmap = pseqptflt;
		DcoCardAdapter::mCardPsEqptFault.isNotified = true;
//		LOG_FLT() << " update card cache pseqptflt: 0x" << hex << pseqptflt << std::dec << std::endl;
	}
	if (cardflt->has_post_fault())
	{
		uint64_t postflt = cardflt->post_fault().value();
		DcoCardAdapter::mCardPostFault.faultBitmap = postflt;
		DcoCardAdapter::mCardPostFault.isNotified = true;
//		LOG_FLT() << " update card cache postflt: 0x" << hex << postflt << std::dec << std::endl;
	}
	if (cardflt->has_dac_post_fault())
	{
		uint64_t dacpostflt = cardflt->dac_post_fault().value();
		DcoCardAdapter::mCardDacPostFault.faultBitmap = dacpostflt;
		DcoCardAdapter::mCardDacPostFault.isNotified = true;
//		LOG_FLT() << " update card cache dacpostflt: 0x" << hex << dacpostflt << std::dec << std::endl;
	}
	if (cardflt->has_ps_post_fault())
	{
		uint64_t pspostflt = cardflt->ps_post_fault().value();
		DcoCardAdapter::mCardPsPostFault.faultBitmap = pspostflt;
		DcoCardAdapter::mCardPsPostFault.isNotified = true;
//		LOG_FLT() << " update card cache pspostflt: 0x" << hex << pspostflt << std::dec << std::endl;
	}
}

void CarrierFaultResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoCarrierAdapter::mCarrierFltSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Carrier Flt grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " CarrierFaultResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoCarrierFault* carrierflt = static_cast<dcoCarrierFault*>(&obj);
	if (carrierflt->has_carrier_id())
	{
		int carrierId = carrierflt->carrier_id().value();
		if (carrierId >= 1 && carrierId <= MAX_CARRIERS)
		{
			if (carrierflt->has_facility_fault())
			{
				uint64_t facflt = carrierflt->facility_fault().value();
				DcoCarrierAdapter::mCarrierFacFault[carrierId - 1].faultBitmap = facflt;
				DcoCarrierAdapter::mCarrierFacFault[carrierId - 1].isNotified = true;
				LOG_FLT() << " carrier " << carrierId << " update adapter cache facflt: 0x" << hex << facflt << std::dec << std::endl;
			}
			if (carrierflt->has_equipment_fault())
			{
				uint64_t eqptflt = carrierflt->equipment_fault().value();
				DcoCarrierAdapter::mCarrierEqptFault[carrierId - 1].faultBitmap = eqptflt;
				DcoCarrierAdapter::mCarrierEqptFault[carrierId - 1].isNotified = true;
				LOG_FLT() << " carrier " << carrierId << " update adapter cache eqptflt: 0x" << hex << eqptflt << std::dec << std::endl;
			}
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid carrierId : " << carrierId << std::endl;
		}
	}
}

void OduStateResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoOduAdapter::mOduStateSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Odu State grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " OduStateResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOduState* odustate = static_cast<dcoOduState*>(&obj);
	if (odustate->has_odu_id())
	{
		int oduId = odustate->odu_id().value();
		if (oduId >= 1 && oduId <= MAX_ODU_ID)
		{
			if (odustate->rx_tti_size() != 0)
			{
				for (int i = 0; i < odustate->rx_tti_size(); i++)
				{
					DcoOduAdapter::mTtiRx[oduId - 1].ttiRx[i] = odustate->rx_tti(i).rx_tti().tti().value();
				}
				DcoOduAdapter::mTtiRx[oduId - 1].isNotified = true;
                DcoOduAdapter::mTtiRx[oduId - 1].notifyCount++;
				INFN_LOG(SeverityLevel::debug) << " odu " << oduId << " update adapter rx tti: ";
				for (int i = 0; i < MAX_TTI_LENGTH; i++)
				{
					INFN_LOG(SeverityLevel::debug) << " 0x" << hex << (int)DcoOduAdapter::mTtiRx[oduId - 1].ttiRx[i] << dec;
				}
			}

			// When CDI state changes, update cache
			// Fill mCdi data structure.
			// DcoOduAdapter::mCdi[oduId - 1]
			// Notification container
			if (odustate->has_cdi())
			{
			    DcoOduAdapter::mCdi[oduId - 1] = odustate->cdi().value();
			    INFN_LOG(SeverityLevel::debug) << " mCdi[" << oduId - 1 << "]=" << (int)DcoOduAdapter::mCdi[oduId - 1];
			}
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid oduId : " << oduId << std::endl;
		}
	}
}

void OtuStateResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoOtuAdapter::mOtuStateSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Otu State grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " OtuStateResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOtuState* otustate = static_cast<dcoOtuState*>(&obj);
	if (otustate->has_otu_id())
	{
		int otuId = otustate->otu_id().value();
		if (otuId >= 1 && otuId <= MAX_OTU_ID)
		{
			if (otustate->rx_tti_size() != 0)
			{
				for (int i = 0; i < otustate->rx_tti_size(); i++)
				{
					DcoOtuAdapter::mTtiRx[otuId - 1].ttiRx[i] = otustate->rx_tti(i).rx_tti().tti().value();
				}
				DcoOtuAdapter::mTtiRx[otuId - 1].isNotified = true;
                DcoOtuAdapter::mTtiRx[otuId - 1].notifyCount++;
				INFN_LOG(SeverityLevel::debug) << " otu " << otuId << " update adapter rx tti: ";
				for (int i = 0; i < MAX_TTI_LENGTH; i++)
				{
					INFN_LOG(SeverityLevel::debug) << " 0x" << hex << (int)DcoOtuAdapter::mTtiRx[otuId - 1].ttiRx[i] << dec;
				}
			}
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid otuId : " << otuId << std::endl;
		}
	}
}

void CardStateResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoCardAdapter::mCardStateSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Card State grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " CardStateResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoCardState* cardstate = static_cast<dcoCardState*>(&obj);
	switch(cardstate->state())
	{
		case cardState::STATE_BOARD_INIT:
			DcoCardAdapter::mCardNotifyState.state = DCOSTATE_BRD_INIT;
			break;
		case cardState::STATE_DSP_CONFIG:
			DcoCardAdapter::mCardNotifyState.state = DCOSTATE_DSP_CONFIG;
			break;
		case cardState::STATE_LOW_POWER:
			DcoCardAdapter::mCardNotifyState.state = DCOSTATE_LOW_POWER;
			break;
		case cardState::STATE_POWER_UP:
			DcoCardAdapter::mCardNotifyState.state = DCOSTATE_POWER_UP;
			break;
		case cardState::STATE_POWER_DOWN:
			DcoCardAdapter::mCardNotifyState.state = DCOSTATE_POWER_DOWN;
			break;
		case cardState::STATE_FAULTED:
			DcoCardAdapter::mCardNotifyState.state = DCOSTATE_FAULTED;
			break;
		default:
			DcoCardAdapter::mCardNotifyState.state = DCOSTATE_UNSPECIFIED;
			break;
	}
	//only print when the value is not default value
	if (DcoCardAdapter::mCardNotifyState.state != DCOSTATE_UNSPECIFIED)
	{
		INFN_LOG(SeverityLevel::info) << " update card state: " << EnumTranslate::toString(DcoCardAdapter::mCardNotifyState.state) << std::endl;
	}

	switch(cardstate->firmware_upgrade_state())
	{
		case cardState::FIRMWAREUPGRADESTATE_IDLE:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_IDLE;
			break;
		case cardState::FIRMWAREUPGRADESTATE_DOWNLOAD_IN_PROGRESS:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_DOWNLOAD_IN_PROGESS;
			break;
		case cardState::FIRMWAREUPGRADESTATE_DOWNLOAD_COMPLETE:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_DOWNLOAD_COMPLETE;
			break;
		case cardState::FIRMWAREUPGRADESTATE_INSTALL_COMPLETE:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_INSTALL_COMPLETE;
			break;
		case cardState::FIRMWAREUPGRADESTATE_ACTIVATE_IN_PROGRESS:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_ACTIVATE_IN_PROGRESS;
			break;
		case cardState::FIRMWAREUPGRADESTATE_ACTIVATE_COMPLETE:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_ACTIVATE_COMPLETE;
			break;
		case cardState::FIRMWAREUPGRADESTATE_ISK_OP_IN_PROGRESS:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_ISK_OP_IN_PROGRESS;
			break;
		case cardState::FIRMWAREUPGRADESTATE_ISK_OP_COMPLETE:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_ISK_OP_COMPLETE;
			break;
		case cardState::FIRMWAREUPGRADESTATE_DOWNLOAD_FAILED:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_DOWNLOAD_FAILED;
			break;
		case cardState::FIRMWAREUPGRADESTATE_INSTALL_FAILED:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_INSTALL_FAILED;
			break;
		case cardState::FIRMWAREUPGRADESTATE_ACTIVATE_FAILED:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_ACTIVATE_FAILED;
			break;
		case cardState::FIRMWAREUPGRADESTATE_ISK_OP_FAILED:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_ISK_OP_FAILED;
			break;
		default:
			DcoCardAdapter::mCardNotifyState.fwUpgradeState = FWUPGRADESTATE_UNSPECIFIED;
			break;
	}
	//only print when the value is not default value
	if (DcoCardAdapter::mCardNotifyState.fwUpgradeState != FWUPGRADESTATE_UNSPECIFIED)
	{
		INFN_LOG(SeverityLevel::info) << " update fw upgrade state: " << EnumTranslate::toString(DcoCardAdapter::mCardNotifyState.fwUpgradeState) << std::endl;
	}

	if (cardstate->has_firmware_upgrade_status())
	{
		DcoCardAdapter::mCardNotifyState.fwUpgradeStatus = cardstate->firmware_upgrade_status().value();
		INFN_LOG(SeverityLevel::info) << " update fw upgrade status: " << DcoCardAdapter::mCardNotifyState.fwUpgradeStatus << std::endl;
	}

	switch(cardstate->boot_state())
	{
		case cardState::BOOTSTATE_CARD_UP:
			DcoCardAdapter::mCardNotifyState.bootState = BOOTSTATE_CARD_UP;
			break;
		case cardState::BOOTSTATE_IN_PROGRESS:
			DcoCardAdapter::mCardNotifyState.bootState = BOOTSTATE_IN_PROGRESS;
			break;
		case cardState::BOOTSTATE_REJECTED:
			DcoCardAdapter::mCardNotifyState.bootState = BOOTSTATE_REJECTED;
			break;
		default:
			DcoCardAdapter::mCardNotifyState.bootState = BOOTSTATE_UNSPECIFIED;
			break;
	}
	//only print when the value is not default value
	if (DcoCardAdapter::mCardNotifyState.bootState != BOOTSTATE_UNSPECIFIED)
	{
		INFN_LOG(SeverityLevel::info) << " update boot state: " << EnumTranslate::toString(DcoCardAdapter::mCardNotifyState.bootState) << std::endl;
	}

	if (cardstate->has_temp_high_fan_increase())
	{
		DcoCardAdapter::mCardNotifyState.tempHiFanIncrease = cardstate->temp_high_fan_increase().value();
		INFN_LOG(SeverityLevel::info) << " update temp High Fan Increase: " << DcoCardAdapter::mCardNotifyState.tempHiFanIncrease << std::endl;
	}

	if (cardstate->has_temp_stable_fan_decrease())
	{
		DcoCardAdapter::mCardNotifyState.tempStableFanDecrease = cardstate->temp_stable_fan_decrease().value();
		INFN_LOG(SeverityLevel::info) << " update temp Stable Fan Decrease: " << DcoCardAdapter::mCardNotifyState.tempStableFanDecrease << std::endl;
	}

	ClientMode prevClientMode = DcoCardAdapter::mCardNotifyState.mode;
	switch(cardstate->client_mode())
	{
		case cardState::CLIENTMODE_LXTF20E:
			DcoCardAdapter::mCardNotifyState.mode = CLIENTMODE_LXTP_E;
			break;
		case cardState::CLIENTMODE_LXTF20M:
			DcoCardAdapter::mCardNotifyState.mode = CLIENTMODE_LXTP_M;
			break;
		case cardState::CLIENTMODE_LXTF20:
			DcoCardAdapter::mCardNotifyState.mode = CLIENTMODE_LXTP;
			break;
		default:
			DcoCardAdapter::mCardNotifyState.mode = CLIENTMODE_UNSET;
			break;
	}
	//only print when the value is not default value
	if (DcoCardAdapter::mCardNotifyState.mode != CLIENTMODE_UNSET)
	{
		INFN_LOG(SeverityLevel::info) << " updateClientMode dump cfgMode: " << EnumTranslate::toString(DcoCardAdapter::mCfgClientMode) << ", prevClientMode: " << EnumTranslate::toString(prevClientMode) << std::endl;
		INFN_LOG(SeverityLevel::info) << " updateClientMode: " << EnumTranslate::toString(DcoCardAdapter::mCardNotifyState.mode) << std::endl;
		if (IsReadyTriggerDcoColdBoot(DcoCardAdapter::mCfgClientMode, prevClientMode, DcoCardAdapter::mCardNotifyState.mode))
		{
			INFN_LOG(SeverityLevel::info) << " updateClientMode triggerDcoCodeBoot: " << EnumTranslate::toString(DcoCardAdapter::mCfgClientMode) << ", prevClientMode: " << EnumTranslate::toString(prevClientMode) << std::endl;
			DataPlaneMgrSingleton::Instance()->triggerDcoColdBoot();
		}
	}

	switch(cardstate->active_running_client_mode())
	{
		case cardState::CLIENTMODE_LXTF20E:
			DcoCardAdapter::mCurClientMode = CLIENTMODE_LXTP_E;
			break;
		case cardState::CLIENTMODE_LXTF20M:
			DcoCardAdapter::mCurClientMode = CLIENTMODE_LXTP_M;
			break;
		case cardState::CLIENTMODE_LXTF20:
			DcoCardAdapter::mCurClientMode = CLIENTMODE_LXTP;
			break;
		default:
			DcoCardAdapter::mCurClientMode = CLIENTMODE_UNSET;
			break;
	}
    DcoCardAdapter::mCardNotifyState.activeMode = DcoCardAdapter::mCurClientMode;

	//only update when active running client mode is valid
	if (DcoCardAdapter::mCurClientMode != CLIENTMODE_UNSET)
	{
		INFN_LOG(SeverityLevel::info) << " update curClientMode: " << EnumTranslate::toString(DcoCardAdapter::mCurClientMode) << std::endl;
	}

	if (cardstate->has_key_show())
	{
		DcoCardAdapter::mCardNotifyState.keyShow = cardstate->key_show().value();
		INFN_LOG(SeverityLevel::info) << " update key show: " << DcoCardAdapter::mCardNotifyState.keyShow << std::endl;
	}
}

void CardIskResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoCardAdapter::mCardIskSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Card Isk grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " CardIskResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoCardIsk* cardisk = static_cast<dcoCardIsk*>(&obj);

	string isk_key_length_str;
	uint32 isk_key_length = 0;
	string isk_is_key_in_use ;
	string isk_is_key_verified;
    int prevIskSize= DcoCardAdapter::mIskSize;

    // for isk-keys size refer  "/dco-card/state/isk-keys" in infinera_dco_card_pb.h
    DcoCardAdapter::mIskSize = cardisk->isk_keys_size();

	int isk_sigs_size = 0;

    // Clear existing map.  DCO ISK will full list of isk keys
	DcoCardAdapter::mCardNotifyState.isk.clear();

    if ( prevIskSize != DcoCardAdapter::mIskSize )
    {
        INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: size ***  " << DcoCardAdapter::mIskSize << endl;
    }

	for(int i = 0; i < DcoCardAdapter::mIskSize; ++i)
	{
        ImageSigningKey isk;

        isk.key_name           = cardisk->isk_keys(i).isk_name();
        if ( prevIskSize != DcoCardAdapter::mIskSize )
        {
            INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: isk.key_name  *** " << isk.key_name   << endl;
        }
        isk.key_serial_number  = cardisk->isk_keys(i).isk_keys().isk_serial().value();

        isk.issuer_name        = cardisk->isk_keys(i).isk_keys().isk_issuer_name().value();

        isk_key_length_str     = cardisk->isk_keys(i).isk_keys().isk_length().value();
        isk_key_length         = std::stoul (isk_key_length_str, nullptr, 0);
        isk.key_length         = isk_key_length;
        // INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: length proto uint32 *** " << isk.key_length << endl;

        isk.key_payload        = cardisk->isk_keys(i).isk_keys().isk_payload().value();

        isk.is_key_in_use      = ( ( cardisk->isk_keys(i).isk_keys().isk_inuse().value() == "true" ) ? true : false);

        isk.is_key_verified    = (( cardisk->isk_keys(i).isk_keys().isk_isverified().value() == "true" ) ? true : false);
        INFN_LOG(SeverityLevel::info) <<__func__ << " isk key: isk.is_key_verified *** " << isk.is_key_verified  << endl;

        isk_sigs_size = cardisk->isk_keys(i).isk_keys().isk_sigs_size();

        INFN_LOG(SeverityLevel::info) <<__func__ << "isk sigs: size ***  " << isk_sigs_size << endl;

        // TODO: Remove the following after the ISKRing file is properly formed. Otherwise size may be 0 causing a sigabort.
        if (isk_sigs_size != 1)
        {
            isk_sigs_size = 1;
            INFN_LOG(SeverityLevel::info) <<__func__ << " Setting isk_sigs_size to " << isk_sigs_size << " *** "<< endl;
        }

        for (int j = 0; j <  isk_sigs_size; j++)
        {
            isk.signature_key_id = cardisk->isk_keys(i).isk_keys().isk_sigs(j).isk_sig_id();

            if ( prevIskSize != DcoCardAdapter::mIskSize )
            {
                INFN_LOG(SeverityLevel::info) <<__func__ << " isk sig: signature key id ***  " << isk.signature_key_id << endl;
            }

            auto iskSigKey = cardisk->isk_keys(i).isk_keys().isk_sigs(j).isk_sigs();
            isk.signature_hash_scheme = iskSigKey.isk_sig_hash_scheme().value();
            isk.signature_payload = iskSigKey.isk_sig_payload().value();
            isk.signature_gen_time = iskSigKey.isk_sig_gen_time().value();

            // Hard code the algorithm for now as the DCO Yang does not have it.
            // TODO: Remove the hard coded value when DCO Yang has this in the ISK definition
            isk.signature_algorithm = "ECDSA";
        }
	    DcoCardAdapter::mCardNotifyState.isk.insert({isk.key_name, isk});
    }
}

void CarrierStateResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoCarrierAdapter::mCarrierStateSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Carrier State grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " CarrierStateResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoCarrierState* carrierstate = static_cast<dcoCarrierState*>(&obj);
	if (carrierstate->has_carrier_id())
	{
		int carrierId = carrierstate->carrier_id().value();
		if (carrierId >= 1 && carrierId <= MAX_CARRIERS)
		{
			switch (carrierstate->tx_carrier_state())
			{
				case carrierState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE:
					DcoCarrierAdapter::mTxState[carrierId-1] = DataPlane::CARRIERSTATE_ACTIVE;
					break;
				case carrierState::INFINERADCOCARRIEROPTICALSTATE_STANDBY:
					DcoCarrierAdapter::mTxState[carrierId-1] = DataPlane::CARRIERSTATE_STANDBY;
					break;
				case carrierState::INFINERADCOCARRIEROPTICALSTATE_AUTO_DISCOVERY:
					DcoCarrierAdapter::mTxState[carrierId-1] = DataPlane::CARRIERSTATE_AUTODISCOVERY;
					break;
				default:
					DcoCarrierAdapter::mTxState[carrierId-1] = DataPlane::CARRIERSTATE_UNSPECIFIED;
					break;
			}
			if (DcoCarrierAdapter::mTxState[carrierId-1] != DataPlane::CARRIERSTATE_UNSPECIFIED)
			{
				INFN_LOG(SeverityLevel::info) << " carrier " << carrierId << " update tx state: " << EnumTranslate::toString(DcoCarrierAdapter::mTxState[carrierId-1]) << std::endl;
			}

			switch (carrierstate->rx_carrier_state())
			{
				case carrierState::INFINERADCOCARRIEROPTICALSTATE_ACTIVE:
					DcoCarrierAdapter::mRxState[carrierId-1] = DataPlane::CARRIERSTATE_ACTIVE;
					break;
				case carrierState::INFINERADCOCARRIEROPTICALSTATE_STANDBY:
					DcoCarrierAdapter::mRxState[carrierId-1] = DataPlane::CARRIERSTATE_STANDBY;
					break;
				case carrierState::INFINERADCOCARRIEROPTICALSTATE_AUTO_DISCOVERY:
					DcoCarrierAdapter::mRxState[carrierId-1] = DataPlane::CARRIERSTATE_AUTODISCOVERY;
					break;
				default:
					DcoCarrierAdapter::mRxState[carrierId-1] = DataPlane::CARRIERSTATE_UNSPECIFIED;
					break;
			}
			if (DcoCarrierAdapter::mRxState[carrierId-1] != DataPlane::CARRIERSTATE_UNSPECIFIED)
			{
				INFN_LOG(SeverityLevel::info) << " carrier " << carrierId << " update rx state: " << EnumTranslate::toString(DcoCarrierAdapter::mRxState[carrierId-1]) << std::endl;
			}
			if (carrierstate->advanced_parameter_current_size() != 0)
			{
				DcoCarrierAdapter::mAdvParam[carrierId-1].clear();

				for (int i = 0; i < carrierstate->advanced_parameter_current_size(); i++)
				{
					AdvancedParameter ap;
					auto apKey = carrierstate->advanced_parameter_current(i);
					ap.apName = apKey.ap_key();
					ap.apValue = apKey.advanced_parameter_current().ap_value().value();
                    ap.bDefault = apKey.advanced_parameter_current().default_().value();
                    ap.defaultValue = apKey.advanced_parameter_current().default_ap_value().value();
                    INFN_LOG(SeverityLevel::info) << " carrier " << carrierId << " update ap with name: " << ap.apName << ", value: " << ap.apValue << " default " << (int) ap.bDefault <<   " default value: " << ap.defaultValue << std::endl;
					using apCurrent = ::dcoyang::infinera_dco_carrier::CarrierState_AdvancedParameterCurrent;
					switch (apKey.advanced_parameter_current().ap_status())
					{
						case apCurrent::APSTATUS_SET:
							ap.apStatus = DataPlane::CARRIERADVPARMSTATUS_SET;
							break;
						case apCurrent::APSTATUS_UNKNOWN:
							ap.apStatus = DataPlane::CARRIERADVPARMSTATUS_UNKNOWN;
							break;
						case apCurrent::APSTATUS_IN_PROGRESS:
							ap.apStatus = DataPlane::CARRIERADVPARMSTATUS_INPROGRESS;
							break;
						case apCurrent::APSTATUS_FAILED:
							ap.apStatus = DataPlane::CARRIERADVPARMSTATUS_FAILED;
							break;
						case apCurrent::APSTATUS_NOT_SUPPORTED:
							ap.apStatus = DataPlane::CARRIERADVPARMSTATUS_NOT_SUPPORTED;
							break;
						default:
							ap.apStatus = DataPlane::CARRIERADVPARMSTATUS_UNSPECIFIED;
							break;
					}
					if (ap.apStatus != DataPlane::CARRIERADVPARMSTATUS_UNSPECIFIED)
					{
						INFN_LOG(SeverityLevel::info) << " carrier " << carrierId << " update ap status: " << EnumTranslate::toString(ap.apStatus) << std::endl;
					}
					DcoCarrierAdapter::mAdvParam[carrierId-1].push_back(ap);
				}
			}
	        if (carrierstate->has_tx_carrier_frequency_reported())
            {

                int64 digit = carrierstate->tx_carrier_frequency_reported().digits();
                uint32 precision = carrierstate->tx_carrier_frequency_reported().precision();

				DcoCarrierAdapter::mTxActualFrequency[carrierId-1] = digit/pow(10, precision);
				INFN_LOG(SeverityLevel::info) << " carrier " << carrierId << " Tx Actual Frequency: " << DcoCarrierAdapter::mTxActualFrequency[carrierId-1] << std::endl;
            }
	        if (carrierstate->has_rx_carrier_frequency_reported())
            {

                int64 digit = carrierstate->rx_carrier_frequency_reported().digits();
                uint32 precision = carrierstate->rx_carrier_frequency_reported().precision();

				DcoCarrierAdapter::mRxActualFrequency[carrierId-1] = digit/pow(10, precision);
				INFN_LOG(SeverityLevel::info) << " carrier " << carrierId << " Rx Actual Frequency: " << DcoCarrierAdapter::mRxActualFrequency[carrierId-1] << std::endl;
            }
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid carrierId : " << carrierId << std::endl;
		}
	}
}

void OduOpStatusResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoOduAdapter::mOduOpStatusSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Odu Op Status grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " OduOpStatusResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOduOpStatus* opstate = static_cast<dcoOduOpStatus*>(&obj);
	if (opstate->has_odu_id())
	{
		int oduId = opstate->odu_id().value();
		if (oduId >= 1 && oduId <= MAX_ODU_ID)
		{
			switch (opstate->operational_status())
			{
				case oduOpStatus::OPERATIONALSTATUS_SUCCESS:
					DcoOduAdapter::mOpStatus[oduId - 1].opStatus = OPSTATUS_SUCCESS;
					break;
				case oduOpStatus::OPERATIONALSTATUS_SDK_EXCEPTION:
					DcoOduAdapter::mOpStatus[oduId - 1].opStatus = OPSTATUS_SDK_EXCEPTION;
					break;
				case oduOpStatus::OPERATIONALSTATUS_INVALID_PARAM:
					DcoOduAdapter::mOpStatus[oduId - 1].opStatus = OPSTATUS_INVALID_PARAM;
					break;
				case oduOpStatus::OPERATIONALSTATUS_OPERATIONAL_ERROR:
					DcoOduAdapter::mOpStatus[oduId - 1].opStatus = OPSTATUS_OPERATIONAL_ERROR;
					break;
				case oduOpStatus::OPERATIONALSTATUS_UNDEFINED:
					DcoOduAdapter::mOpStatus[oduId - 1].opStatus = OPSTATUS_UNDEFINED;
					break;
				default:
					DcoOduAdapter::mOpStatus[oduId - 1].opStatus = OPSTATUS_UNSPECIFIED;
					break;
			}
			INFN_LOG(SeverityLevel::info) << " odu opstatus: " << EnumTranslate::toString(DcoOduAdapter::mOpStatus[oduId - 1].opStatus) << std::endl;

	        if (opstate->has_tid())
            {
					DcoOduAdapter::mOpStatus[oduId - 1].transactionId = opstate->tid().value();
            }

			INFN_LOG(SeverityLevel::info) << " odu transId: 0x" << hex << DcoOduAdapter::mOpStatus[oduId - 1].transactionId << dec << std::endl;
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid oduId : " << oduId << std::endl;
		}
	}
}

void OtuOpStatusResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoOtuAdapter::mOtuOpStatusSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Otu Op Status grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " OtuOpStatusResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoOtuOpStatus* opstate = static_cast<dcoOtuOpStatus*>(&obj);
	if (opstate->has_otu_id())
	{
		int otuId = opstate->otu_id().value();
		if (otuId >= 1 && otuId <= MAX_OTU_ID)
		{
			switch (opstate->operational_status())
			{
				case otuOpStatus::OPERATIONALSTATUS_SUCCESS:
					DcoOtuAdapter::mOpStatus[otuId - 1].opStatus = OPSTATUS_SUCCESS;
					break;
				case otuOpStatus::OPERATIONALSTATUS_SDK_EXCEPTION:
					DcoOtuAdapter::mOpStatus[otuId - 1].opStatus = OPSTATUS_SDK_EXCEPTION;
					break;
				case otuOpStatus::OPERATIONALSTATUS_INVALID_PARAM:
					DcoOtuAdapter::mOpStatus[otuId - 1].opStatus = OPSTATUS_INVALID_PARAM;
					break;
				case otuOpStatus::OPERATIONALSTATUS_OPERATIONAL_ERROR:
					DcoOtuAdapter::mOpStatus[otuId - 1].opStatus = OPSTATUS_OPERATIONAL_ERROR;
					break;
				case otuOpStatus::OPERATIONALSTATUS_UNDEFINED:
					DcoOtuAdapter::mOpStatus[otuId - 1].opStatus = OPSTATUS_UNDEFINED;
					break;
				default:
					DcoOtuAdapter::mOpStatus[otuId - 1].opStatus = OPSTATUS_UNSPECIFIED;
					break;
			}
			INFN_LOG(SeverityLevel::info) << " otu opstatus: " << EnumTranslate::toString(DcoOtuAdapter::mOpStatus[otuId - 1].opStatus) << std::endl;

	        if (opstate->has_tid())
            {
					DcoOtuAdapter::mOpStatus[otuId - 1].transactionId = opstate->tid().value();
            }
			INFN_LOG(SeverityLevel::info) << " otu transId: 0x" << hex << DcoOtuAdapter::mOpStatus[otuId - 1].transactionId << dec << std::endl;
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid otuId : " << otuId << std::endl;
		}
	}
}

void GigeOpStatusResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoGigeClientAdapter::mGigeOpStatusSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Gige Op Status grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " GigeOpStatusResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoGigeOpStatus* opstate = static_cast<dcoGigeOpStatus*>(&obj);
	if (opstate->has_client_port_id())
	{
		int gigeId = opstate->client_port_id().value();
		if (gigeId >= 1 && gigeId <= MAX_CLIENTS)
		{
			switch (opstate->operational_status())
			{
				case gigeOpStatus::OPERATIONALSTATUS_SUCCESS:
					DcoGigeClientAdapter::mOpStatus[gigeId - 1].opStatus = OPSTATUS_SUCCESS;
					break;
				case gigeOpStatus::OPERATIONALSTATUS_SDK_EXCEPTION:
					DcoGigeClientAdapter::mOpStatus[gigeId - 1].opStatus = OPSTATUS_SDK_EXCEPTION;
					break;
				case gigeOpStatus::OPERATIONALSTATUS_INVALID_PARAM:
					DcoGigeClientAdapter::mOpStatus[gigeId - 1].opStatus = OPSTATUS_INVALID_PARAM;
					break;
				case gigeOpStatus::OPERATIONALSTATUS_OPERATIONAL_ERROR:
					DcoGigeClientAdapter::mOpStatus[gigeId - 1].opStatus = OPSTATUS_OPERATIONAL_ERROR;
					break;
				case gigeOpStatus::OPERATIONALSTATUS_UNDEFINED:
					DcoGigeClientAdapter::mOpStatus[gigeId - 1].opStatus = OPSTATUS_UNDEFINED;
					break;
				default:
					DcoGigeClientAdapter::mOpStatus[gigeId - 1].opStatus = OPSTATUS_UNSPECIFIED;
					break;
			}
			INFN_LOG(SeverityLevel::info) << " gige opstatus: " << EnumTranslate::toString(DcoGigeClientAdapter::mOpStatus[gigeId - 1].opStatus) << std::endl;

	        if (opstate->has_tid())
            {
					DcoGigeClientAdapter::mOpStatus[gigeId - 1].transactionId = opstate->tid().value();
            }

			INFN_LOG(SeverityLevel::info) << " gige transId: 0x" << hex << DcoGigeClientAdapter::mOpStatus[gigeId - 1].transactionId << dec << std::endl;
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid otuId : " << gigeId << std::endl;
		}
	}
}

void XconOpStatusResponseContext::HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId)
{
	if (!status.ok())
	{
		DcoXconAdapter::mXconOpStatusSubEnable = false;
		INFN_LOG(SeverityLevel::info) << " Xcon Op Status grpc error:  << [" << status.error_code() << "]: " << status.error_message() << std::endl;
		return;
	}
	INFN_LOG(SeverityLevel::debug) << " XconOpStatusResponseContext triggerred!!!" << " , cbId: " << cbId << std::endl;
	dcoXconOpStatus* opstate = static_cast<dcoXconOpStatus*>(&obj);
	if (opstate->has_xcon_id())
	{
		int xconId = opstate->xcon_id().value();
		if (xconId >= 1 && xconId <= MAX_XCON_ID)
		{
			switch (opstate->operational_status())
			{
				case xconOpStatus::OPERATIONALSTATUS_SUCCESS:
					DcoXconAdapter::mOpStatus[xconId - 1].opStatus = OPSTATUS_SUCCESS;
					break;
				case xconOpStatus::OPERATIONALSTATUS_SDK_EXCEPTION:
					DcoXconAdapter::mOpStatus[xconId - 1].opStatus = OPSTATUS_SDK_EXCEPTION;
					break;
				case xconOpStatus::OPERATIONALSTATUS_INVALID_PARAM:
					DcoXconAdapter::mOpStatus[xconId - 1].opStatus = OPSTATUS_INVALID_PARAM;
					break;
				case xconOpStatus::OPERATIONALSTATUS_OPERATIONAL_ERROR:
					DcoXconAdapter::mOpStatus[xconId - 1].opStatus = OPSTATUS_OPERATIONAL_ERROR;
					break;
				case xconOpStatus::OPERATIONALSTATUS_UNDEFINED:
					DcoXconAdapter::mOpStatus[xconId - 1].opStatus = OPSTATUS_UNDEFINED;
					break;
				default:
					DcoXconAdapter::mOpStatus[xconId - 1].opStatus = OPSTATUS_UNSPECIFIED;
					break;
			}
			INFN_LOG(SeverityLevel::info) << " xcon opstatus: " << EnumTranslate::toString(DcoXconAdapter::mOpStatus[xconId - 1].opStatus) << std::endl;

	        if (opstate->has_tid())
            {
					DcoXconAdapter::mOpStatus[xconId - 1].transactionId = opstate->tid().value();
            }

			INFN_LOG(SeverityLevel::info) << " xcon transId: 0x" << hex << DcoXconAdapter::mOpStatus[xconId - 1].transactionId << dec << std::endl;
		}
		else
		{
			INFN_LOG(SeverityLevel::error) << " invalid xconId : " << xconId << std::endl;
		}
	}
}


} /* DpAdapter */
