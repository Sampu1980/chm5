#ifndef DPPMH
#define DPPMH
#include <typeinfo>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <list>
#include <vector>
#include <iostream>
#include <cstring>
#include <chrono>
#include "dp_common.h"
#include "InfnLogger.h"

using namespace std;
using namespace std::chrono;

typedef unsigned short uint16_t;
typedef unsigned long uint64_t;

namespace DataPlane {

class DcoBasePM
{
    public:
        int n;
        DcoBasePM() {}
        ~DcoBasePM() {}
};

class DpMsCardPm : public DcoBasePM {
public:
    uint16_t name;
    struct {
        float DspTemperature;
        float PicTemperature;
        float ModuleCaseTemperature;
    } SystemState;
};

///////////////////////////////////////////////////////////////////////////
class DpMsSubCarrierSnr {
public:
	uint32_t subCarrierIdx;
	double   subcarrier_snr;
};

class DpMsCarrierRx {
public:
    enum acquisition_state_enum {
        ACQ_UNSPECIFIED,
        ACQ_IDLE,
        ACQ_SIG_DETECT,
        ACQ_ACQUISITION,
        ACQ_ADC_CAL,
        ACQ_RX_ACTIVE,
    };

    std::string toString(acquisition_state_enum acqState)
    {
        switch (acqState)
        {
            case ACQ_IDLE:        return ("IDLE");        break;
            case ACQ_SIG_DETECT:  return ("SIG_DETECT");  break;
            case ACQ_ACQUISITION: return ("ACQUISITION"); break;
            case ACQ_ADC_CAL:     return ("ADC_CAL");     break;
            case ACQ_RX_ACTIVE:   return ("RX_ACTIVE");   break;
            default:              return ("UNKNOWN");     break;
        }
    }

    acquisition_state_enum acquisition_state;

    double   pre_fec_ber;
    double   pre_fec_q;
    double   post_fec_q;
    double   snr;
    double   snr_avg_x;
    double   snr_avg_y;
    double   snr_inner;
    double   snr_outer;
    double   snr_diff_left;
    double   snr_diff_right;
    double   osnr;
    uint64_t pass_frame_count;
    uint64_t failed_frame_count;
    uint64_t total_frame_count;
    double   fas_q;
    double   cd;    // chromatic-dispersion
    double   pmd;   // polarization-mode-dispersion
    double   polarization_dependent_loss;
    double   opr;
    double   laser_current;
    double   dgd;
    double   so_dgd;
    double   phase_correction;
    uint64_t corrected_bits;
    double   frequency;

    std::array<DpMsSubCarrierSnr, 8> subcarrier_snr_list;
};

class DpMsCarrierTx
{ //MC-Och
public:
    double opt;
    double laser_current;
    double cd;
    double edfa_input_power;
    double edfa_output_power;
    double edfa_pump_current;
    double frequency;
};

class DpMsCarrierPMContainer {
public:
	string   aid;
	uint32_t carrierId;
	uint8_t  subCarrierCnt;
	DpMsCarrierRx rx;
	DpMsCarrierTx tx;
	bool accumEnable;
        std::chrono::system_clock::time_point updateTime;
        uint64_t updateCount;
	DpMsCarrierPMContainer() = default;
};

// carrierId is 1 based for all interfaces
class DpMsCarrierPMs : public DcoBasePM
{
public:

	//add carrierId and aid before subscription
	//in grpc callback, use carrierId to match entry
	void AddPmData(string aid, int carrierId)
	{
		DpMsCarrierPMContainer carrierPm;
		memset(&carrierPm, 0, sizeof(carrierPm));
		carrierPm.aid = aid;
		carrierPm.carrierId = carrierId;
		if (HasPmData(carrierId)) {
			return;
		}
		carrierPms[carrierId] = carrierPm;
		carrierPmsAccum[carrierId] = carrierPm;
	}

	void RemovePmData(int carrierId)
	{
		if (HasPmData(carrierId) == false) return;
		carrierPms.erase(carrierId);
		carrierPmsAccum.erase(carrierId);
	}

	void UpdatePmData(DpMsCarrierPMContainer carrierPm, int carrierId)
	{
		if (HasPmData(carrierId) == false) {
	        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for carrierId: " << carrierId << endl;
			return;
		}
		carrierPms[carrierId] = carrierPm;
	}

	void UpdatePmDataAccum(DpMsCarrierPMContainer carrierPmAccum, int carrierId)
	{
		if (HasPmData(carrierId) == false) {
			INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for carrierId: " << carrierId << endl;
			return;
		}
		carrierPmsAccum[carrierId] = carrierPmAccum;
	}

        void ClearPmDataAccum(int carrierId)
        {
                if (HasPmData(carrierId) == false) {
                        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for carrierId: " << carrierId << endl;
                        return;
                }

		DpMsCarrierRx carrierRx;
		DpMsCarrierTx carrierTx;

                memset(&carrierRx, 0, sizeof(carrierRx));
                memset(&carrierTx, 0, sizeof(carrierTx));

		carrierPmsAccum[carrierId].rx = carrierRx;
		carrierPmsAccum[carrierId].tx = carrierTx;
        }

	DpMsCarrierPMContainer GetPmData(int carrierId)
	{
		//return a copy in case delete occurs asynchronously
		auto carrierPm = carrierPms[carrierId];
		return carrierPm;
	}

	DpMsCarrierPMContainer GetPmDataAccum(int carrierId)
	{
		return carrierPmsAccum[carrierId];
	}

	int GetPmDataSize()
	{
		return carrierPms.size();
	}

	bool HasPmData(int carrierId)
	{
		return carrierPms.count(carrierId);
	}

	std::map<int, DpMsCarrierPMContainer> GetPmMap()
	{
		return carrierPms;
	}

	void SetAccumEnable(int carrierId, bool enable)
	{
		if (HasPmData(carrierId) == false)
                {
                    return;
                }
		carrierPmsAccum[carrierId].accumEnable = enable;
	}

private:
	//key is carrierId, 1 based
	std::map<int, DpMsCarrierPMContainer> carrierPms;
	std::map<int, DpMsCarrierPMContainer> carrierPmsAccum;
};

///////////////////////////////////////////////////////////////////////////
class DpMsClientGigePmMac {
public:
    uint64_t packets;
    uint64_t error_packets;
    uint64_t ok_packets;
    uint64_t octets;
    uint64_t error_octets;
    uint64_t jabber_frames;
    uint64_t fragmented_frames;
    uint64_t crc_aligned_error;
    uint64_t under_sized;
    uint64_t over_sized;
    uint64_t broadcast_frames;
    uint64_t multicast_frames;
    uint64_t unicast_frames;
    uint64_t pause_frames;
    uint64_t bpdu_frames;
    uint64_t lacp_frames;
    uint64_t oam_frames;
    uint64_t fcs_error;
    uint64_t vlan_ok;
    uint64_t size_64;
    uint64_t size_65_to_127;
    uint64_t size_128_to_255;
    uint64_t size_256_to_511;
    uint64_t size_512_to_1023;
    uint64_t size_1024_to_1518;
    uint64_t jumbo_frames;
    uint64_t jumbo_to_extreme_jumbo_frames;
};

class  DpMsClientGigePmPcs {
public:
    uint64_t  control_block;
    uint64_t  errored_blocks;
    uint64_t  idle_error;
    uint64_t  bip_total;
    uint64_t  test_pattern_error;
};

class DpMsClientGigePmPhyFec {
public:
    uint64_t fec_corrected;
    uint64_t fec_un_corrected;
    uint64_t bit_error;
    uint64_t fec_symbol_error;
    uint64_t invalid_transcode_block;
    uint64_t bip_8_error;
    double pre_fec_ber;
    double fec_symbol_error_rate;
};

class DpMsClientGigePmContainer {
public:
    string aid;
    int gigeId;
    DpMsClientGigePmMac    DpMsClientGigePmMacRx;
    DpMsClientGigePmMac    DpMsClientGigePmMacTx;
    DpMsClientGigePmPcs    DpMsClientGigePmPcsRx;
    DpMsClientGigePmPcs    DpMsClientGigePmPcsTx;
    DpMsClientGigePmPhyFec DpMsClientGigePmPhyFecRx;
    bool accumEnable;
    std::chrono::system_clock::time_point updateTime;
    uint64_t updateCount;
    DpMsClientGigePmContainer() = default;
};

class DpMsClientGigePms : public DcoBasePM
{
public:
	//add portId and aid before subscription
	//in grpc callback, use portId to match entry
	void AddPmData(string aid, int gigeId)
	{
		DpMsClientGigePmContainer gigePm;
		memset(&gigePm, 0, sizeof(gigePm));
		gigePm.aid = aid;
		gigePm.gigeId = gigeId;
		if (HasPmData(gigeId)) {
			return;
		}
		gigePms[gigeId] = gigePm;
		gigePmsAccum[gigeId] = gigePm;
	}

	void RemovePmData(int gigeId)
	{
		if (HasPmData(gigeId) == false) return;
		gigePms.erase(gigeId);
		gigePmsAccum.erase(gigeId);
	}

	void UpdatePmData(DpMsClientGigePmContainer gigePm, int gigeId)
	{
		if (HasPmData(gigeId) == false) {
	        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for gigeId: " << gigeId << endl;
			return;
		}
		gigePms[gigeId] = gigePm;
	}

	void UpdatePmDataAccum(DpMsClientGigePmContainer gigePmAccum, int gigeId)
	{
		if (HasPmData(gigeId) == false) {
			INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for gigeId: " << gigeId << endl;
			return;
		}
		gigePmsAccum[gigeId] = gigePmAccum;
	}

        void ClearPmDataAccum(int gigeId)
        {
                if (HasPmData(gigeId) == false) {
                        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for gigeId: " << gigeId << endl;
                        return;
                }

                DpMsClientGigePmMac    gigePmMac;   
                DpMsClientGigePmPcs    gigePmPcs; 
                DpMsClientGigePmPhyFec gigePmPhyFec;

                memset(&gigePmMac,    0, sizeof(gigePmMac));
                memset(&gigePmPcs,    0, sizeof(gigePmPcs));
                memset(&gigePmPhyFec, 0, sizeof(gigePmPhyFec));

		gigePmsAccum[gigeId].DpMsClientGigePmMacRx    = gigePmMac;
		gigePmsAccum[gigeId].DpMsClientGigePmMacTx    = gigePmMac;
		gigePmsAccum[gigeId].DpMsClientGigePmPcsRx    = gigePmPcs;
		gigePmsAccum[gigeId].DpMsClientGigePmPcsTx    = gigePmPcs;
		gigePmsAccum[gigeId].DpMsClientGigePmPhyFecRx = gigePmPhyFec;
        }

	DpMsClientGigePmContainer GetPmData(int gigeId)
	{
		//return a copy in case delete occurs asynchronously
		auto gigePm = gigePms[gigeId];
		return gigePm;
	}

	DpMsClientGigePmContainer GetPmDataAccum(int gigeId)
	{
		return gigePmsAccum[gigeId];
	}

	int GetPmDataSize()
	{
		return gigePms.size();
	}

	bool HasPmData(int gigeId)
	{
		return gigePms.count(gigeId);
	}

	void SetAccumEnable(int gigeId, bool enable)
	{
		if (HasPmData(gigeId) == false)
                {
                    return;
                }
		gigePmsAccum[gigeId].accumEnable = enable;
	}

private:
    std::map<int, DpMsClientGigePmContainer> gigePms;
    std::map<int, DpMsClientGigePmContainer> gigePmsAccum;
};

///////////////////////////////////////////////////////////////////////////
class DpMsOduPmCommon {
public:
	uint64_t bip_err;
	uint64_t err_blocks;
	uint64_t far_end_err_blocks;
	uint64_t bei;
	uint64_t iae;
	uint64_t biae;
	uint64_t prbs_sync_err_count;
	uint64_t prbs_err_count;
	double   latency;
};

class DpMsOduPMContainer {
public:
    uint16_t oduId;
    string aid;
    DpMsOduPmCommon rx;
    DpMsOduPmCommon tx;
    bool valid;
    bool accumEnable;
    std::chrono::system_clock::time_point updateTime;
    uint64_t updateCount;
    DpMsOduPMContainer() = default;
};

class DpMsODUPm  : public DcoBasePM
{
public:
	//add portId and aid before subscription
	//in grpc callback, use portId to match entry
	void AddPmData(string aid, int oduId)
	{
		DpMsOduPMContainer oduPm;
		memset(&oduPm, 0, sizeof(oduPm));
		oduPm.aid = aid;
		oduPm.oduId = oduId;
		if (HasPmData(oduId)) {
			return;
		}
		oduPm.valid = false;
		oduPms[oduId] = oduPm;
		oduPmsAccum[oduId] = oduPm;
	}

	void RemovePmData(int oduId)
	{
		if (HasPmData(oduId) == false) return;
		oduPms.erase(oduId);
		oduPmsAccum.erase(oduId);
	}

	void UpdatePmData(DpMsOduPMContainer oduPm, int oduId)
	{
		if (HasPmData(oduId) == false) {
	        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for oduId: " << oduId << endl;
			return;
		}
		oduPms[oduId] = oduPm;
	}

	void UpdatePmDataAccum(DpMsOduPMContainer oduPmAccum, int oduId)
	{
		if (HasPmData(oduId) == false) {
			INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for oduId: " << oduId << endl;
			return;
		}
		oduPmsAccum[oduId] = oduPmAccum;
	}

	void ClearPmDataAccum(int oduId)
	{
		if (HasPmData(oduId) == false) {
			INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for oduId: " << oduId << endl;
			return;
		}

		DpMsOduPmCommon oduPmCommon;;
		memset(&oduPmCommon, 0, sizeof(oduPmCommon));

		oduPmsAccum[oduId].rx = oduPmCommon;
		oduPmsAccum[oduId].tx = oduPmCommon;
	}

	DpMsOduPMContainer GetPmData(int oduId)
	{
		//return a copy in case delete occurs asynchronously
		auto oduPm = oduPms[oduId];
		return oduPm;
	}

	DpMsOduPMContainer GetPmDataAccum(int oduId)
	{
		return oduPmsAccum[oduId];
	}

	int GetPmDataSize()
	{
		return oduPms.size();
	}

	bool HasPmData(int oduId)
	{
		return oduPms.count(oduId);
	}

	void UpdatePmValid(int oduId, bool valid)
	{
		if (HasPmData(oduId) == false) return;
		oduPms[oduId].valid = valid;
	}

	bool IsPmValid(int oduId)
	{
		if (HasPmData(oduId) == false) return false;
		return oduPms[oduId].valid;
	}

	void SetAccumEnable(int oduId, bool enable)
	{
		if (HasPmData(oduId) == false)
                {
                    return;
                }
		oduPmsAccum[oduId].accumEnable = enable;
	}

private:
    std::map<uint16_t, DpMsOduPMContainer> oduPms;
    std::map<uint16_t, DpMsOduPMContainer> oduPmsAccum;
};

///////////////////////////////////////////////////////////////////////////
class DpMsOtuPmCommon {
public:
	uint64_t bip_err;
	uint64_t err_blocks;
	uint64_t far_end_err_blocks;
	uint64_t bei;
	uint64_t iae;
	uint64_t biae;
	uint64_t prbs_sync_err_count;
	uint64_t prbs_err_count;
	double   latency;
};

class DpMsOtuPmFecStat {
public:
    uint64_t num_collected;
    uint64_t corrected_bits;
    uint64_t corrected_1;
    uint64_t corrected_0;
    uint64_t uncorrected_words;
    uint64_t uncorrected_bits;
    uint64_t total_code_words;
    uint64_t code_word_size;
    uint64_t total_bits;
    double   pre_fec_ber;
};

class DpMsOtuPMSContainer {
public:
    uint16_t otuId;
    string aid;
    class rx {
	public:
        DpMsOtuPmCommon pmcommon;
        DpMsOtuPmFecStat fecstat;
    } rx;
    DpMsOtuPmCommon tx;
    bool valid;
    bool accumEnable;
    std::chrono::system_clock::time_point updateTime;
    uint64_t updateCount;
    DpMsOtuPMSContainer() = default;
};

class DpMsOTUPm  : public DcoBasePM
{
public:
	//add portId and aid before subscription
	//in grpc callback, use portId to match entry
	void AddPmData(string aid, int otuId)
	{
		DpMsOtuPMSContainer otuPm;
		memset(&otuPm, 0, sizeof(otuPm));
		otuPm.aid = aid;
		otuPm.otuId = otuId;
		if (HasPmData(otuId)) {
			return;
		}
		otuPm.valid = false;
		otuPms[otuId] = otuPm;
		otuPmsAccum[otuId] = otuPm;
	}

	void RemovePmData(int otuId)
	{
		if (HasPmData(otuId) == false) return;
		otuPms.erase(otuId);
		otuPmsAccum.erase(otuId);
	}

	void UpdatePmData(DpMsOtuPMSContainer otuPm, int otuId)
	{
		if (HasPmData(otuId) == false) {
	        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for otuId: " << otuId << endl;
			return;
		}
		otuPms[otuId] = otuPm;
	}

	void UpdatePmDataAccum(DpMsOtuPMSContainer otuPmAccum, int otuId)
	{
		if (HasPmData(otuId) == false) {
			INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for otuId: " << otuId << endl;
			return;
		}
		otuPmsAccum[otuId] = otuPmAccum;
	}

        void ClearPmDataAccum(int otuId)
        {
                if (HasPmData(otuId) == false) {
                        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for otuId: " << otuId << endl;
                        return;
                }

		DpMsOtuPmCommon  otuCommon;
		DpMsOtuPmFecStat otuFecStat;

                memset(&otuCommon,  0, sizeof(otuCommon));
                memset(&otuFecStat, 0, sizeof(otuFecStat));

		otuPmsAccum[otuId].rx.pmcommon = otuCommon;
		otuPmsAccum[otuId].rx.fecstat  = otuFecStat;
		otuPmsAccum[otuId].tx = otuCommon;
        }

	DpMsOtuPMSContainer GetPmData(int otuId)
	{
		//return a copy in case delete occurs asynchronously
		auto otuPm = otuPms[otuId];
		return otuPm;
	}

	DpMsOtuPMSContainer GetPmDataAccum(int otuId)
	{
		return otuPmsAccum[otuId];
	}

	int GetPmDataSize()
	{
		return otuPms.size();
	}

	bool HasPmData(int otuId)
	{
		return otuPms.count(otuId);
	}

	void UpdatePmValid(int otuId, bool valid)
	{
		if (HasPmData(otuId) == false) return;
		otuPms[otuId].valid = valid;
	}

	bool IsPmValid(int otuId)
	{
		if (HasPmData(otuId) == false) return false;
		return otuPms[otuId].valid;
	}

	void SetAccumEnable(int otuId, bool enable)
	{
		if (HasPmData(otuId) == false)
                {
                    return;
                }
		otuPmsAccum[otuId].accumEnable = enable;
	}

private:
    std::map<uint16_t, DpMsOtuPMSContainer> otuPms;
    std::map<uint16_t, DpMsOtuPMSContainer> otuPmsAccum;
};

///////////////////////////////////////////////////////////////////////////
class DpMsGccControlPm {
public:
	uint64_t tx_packets;
	uint64_t rx_packets;
	uint64_t tx_packets_dropped;
	uint64_t rx_packets_dropped;
	uint64_t tx_bytes;
	uint64_t rx_bytes;
};

class DpMsGccPMContainer {
public:
	uint16_t carrierId; //1-2
	string aid;
	DpMsGccControlPm controlpm;
};

class DpMsGCCPm  : public DcoBasePM
{
public:
	//add carrierId and aid before subscription
	//in grpc callback, use carrierId to match entry
	//pmAgent will map gcc pm to comm-channel object
	void AddPmData(string aid, int carrierId)
	{
		DpMsGccPMContainer gccPm;
		memset(&gccPm, 0, sizeof(gccPm));
		gccPm.carrierId = carrierId;
		gccPm.aid = aid;
		if (HasPmData(carrierId)) {
			return;
		}
		gccPms[carrierId] = gccPm;
		gccPmsAccum[carrierId] = gccPm;
	}

	void RemovePmData(int carrierId)
	{
		if (HasPmData(carrierId) == false) return;
		gccPms.erase(carrierId);
		gccPmsAccum.erase(carrierId);
	}

	void UpdatePmData(DpMsGccPMContainer gccPm, int carrierId)
	{
		if (HasPmData(carrierId) == false) {
	        INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for carrierId: " << carrierId << endl;
			return;
		}
		gccPms[carrierId] = gccPm;
	}

	void UpdatePmDataAccum(DpMsGccPMContainer gccPmAccum, int carrierId)
	{
		if (HasPmData(carrierId) == false) {
			INFN_LOG(SeverityLevel::debug) << __func__ << ", no entry for carrierId: " << carrierId << endl;
			return;
		}
		gccPmsAccum[carrierId] = gccPmAccum;
	}

	DpMsGccPMContainer GetPmData(int carrierId)
	{
		//return a copy in case delete occurs asynchronously
		auto gccPm = gccPms[carrierId];
		return gccPm;
	}

	DpMsGccPMContainer GetPmDataAccum(int carrierId)
	{
		return gccPmsAccum[carrierId];
	}

	int GetPmDataSize()
	{
		return gccPms.size();
	}

	bool HasPmData(int carrierId)
	{
		return gccPms.count(carrierId);
	}

private:
    std::unordered_map<uint16_t, DpMsGccPMContainer> gccPms;
    std::unordered_map<uint16_t, DpMsGccPMContainer> gccPmsAccum;
};

class DpmsCbHandler {
    public:
        //Return true on success and false on any failure
        virtual bool OnRcvPm(string className, DcoBasePM &basePm) = 0;
};

#define CLASS_NAME(x) typeid(x).name()

class DPMSPmCallbacks
{
    public:
        static DPMSPmCallbacks *getInstance();
        static bool RegisterCallBack(string className, DpmsCbHandler* callbackHandler);
        static void UnregisterCallBack(string className);
        static bool CallCallback(string className, DcoBasePM *basePMp);
    private:
        DPMSPmCallbacks();
        static map<string, DpmsCbHandler*> cbhMap;
        static DPMSPmCallbacks *instance;
};

}; // namespace DataPlane

#endif /* DPPMH */
