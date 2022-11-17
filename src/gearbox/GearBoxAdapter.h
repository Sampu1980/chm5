#ifndef __GEARBOXADAPTER_H__
#define __GEARBOXADAPTER_H__

#include "GearBoxIf.h"
#include "GearBoxAdIf.h"
#include "Bcm81725.h"
#include <vector>
#include "SingletonService.h"
#include "GearBoxUtils.h"
#include "bcm_pm_if/bcm_pm_if_api.h"
#include <map>
#include <thread>
#include <mutex>

using namespace std;


namespace gearbox {

class GearBoxAdapter : public GearBoxAdIf
{
public:
	GearBoxAdapter();
	virtual ~GearBoxAdapter() {}

	int init() { return 1; }
	int GetBcmInfo(BcmVers & bcmInfoTopMz, BcmVers & bcmInfoBottomMz);


	// API called by DP manager
	int loadFirmware(unsigned int bus, bool force = true);
	int setPortRate(const string& aId, Bcm81725DataRate_t rate, bool fecEnable=false, bcm_plp_fec_mode_sel_t fecType=bcmplpKR4FEC);
	int setPortLoopback(const string& aId, Bcm81725Loopback_t mode);
	int setPortFecEnable(const string& aId, bool enable);
	int setTxSerdesParams(const string& aId);
	int setRxSerdesParams(const string& aId);


	// internal API
	int getPolarity(const Bcm81725Lane& param, unsigned int& tx, unsigned int& rx);
	int setPolarity(const Bcm81725Lane& param, unsigned int tx, unsigned int rx);
	int warmInit(const Bcm81725Lane& param);
	int setAvsConfig(const Bcm81725Lane& param, unsigned int enable, unsigned int margin);
	int getAvsConfig(const Bcm81725Lane& param, unsigned int& enable, unsigned int& disableType, 
		             unsigned int& ctrl, unsigned int& regulator, unsigned int& pkgShare, unsigned int& margin);
	int getAvsStatus(const Bcm81725Lane& param, unsigned int& status);
	int getMode(const Bcm81725Lane& param, ModeParam &modeParam);
	int setMode(const Bcm81725Lane& param, const ModeParam &modeParam);
	int dumpBcmConfig(Ports_t port, std::string &dump);
	int prbsLinkStatusCheck(const Bcm81725Lane& param, unsigned int& status);
	int prbsStatusCheck(const Bcm81725Lane& param, unsigned int timeVal, unsigned int& status);
	int setPrbsGen(const Bcm81725Lane& param, unsigned int tx, unsigned int poly, 
                   unsigned int inv, unsigned int lb, unsigned int enaDis);
	int setPrbsCheck(const Bcm81725Lane& param, unsigned int tx, unsigned int poly, 
                   unsigned int inv, unsigned int lb, unsigned int enaDis);
	int setLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int enable);
	int getLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int &enable);
	int getLaneConfigDs(const Bcm81725Lane& param, bcm_plp_dsrds_firmware_lane_config_t &lnConf);
	int setLaneConfigDs(const Bcm81725Lane& param, unsigned int fwLaneConfig, int64_t value);

	int setRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rx_control);
	int setRxLaneControl(unsigned int port, bcm_plp_pm_phy_rx_lane_control_t rxControl, Bcm81725DataRate_t rate);
	int getRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t& rx_control);

    int setTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t tx_control);

    int getTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t& tx_control);

    int setTxFir(const Bcm81725Lane& param, int pre2, int pre, int main, int post, int post2, int post3);
    int getTxFir(const Bcm81725Lane& param, int& pre2, int& pre, int& main, int& post, int& post2, int& post3);

    int Write(const Bcm81725Lane& param, unsigned int regAddr, unsigned int regData);
    int Read(const Bcm81725Lane& param, unsigned int regAddr, unsigned int& regData);

    int getPortRate(unsigned int port, Bcm81725DataRate_t &rate);
    void updatePortParamCache(unsigned int port, const ModeParam& modeParamHost, const ModeParam& modeParamLine);
    int getEyeScan(const Bcm81725Lane& param);
    int getEyeScan(unsigned int port, Bcm81725DataRate_t rate);

    int getEyeProjection(const Bcm81725Lane& param, unsigned int lineRate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr, std::string &errMsg);
    int getEyeProjection(unsigned int port, Bcm81725DataRate_t rate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr, std::string &errMsg);
    int chipStatus(const Bcm81725Lane& param, unsigned int func);

    int clrInterrupt(const Bcm81725Lane& param);


    int getInterrupt(const Bcm81725Lane& param, unsigned int type, unsigned int *status);


    int setInterrupt(const Bcm81725Lane& param, unsigned int type, bool enable);


    int getPcsStatus(const Bcm81725Lane& param, bcm_plp_pcs_status_t& st);
    int printPcsStatus(const Bcm81725Lane& param, ostringstream& out);
    int getPcsFaults(const Bcm81725Lane& param, unsigned int port, std::vector<bool> & faults);
    int dumpPcsStatus(unsigned int port, Bcm81725DataRate_t rate, std::string &status);

    int printFecStatus(const Bcm81725Lane& param, bool clear, std::ostringstream& out);
    int getFecStatus(const Bcm81725Lane& param, bool clear, bcm_plp_fec_status_t& st);
    int dumpFecStatus(unsigned int port, Bcm81725DataRate_t rate, bool clear, std::string &status);
    int getFecFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults);

    int getBcmFaults(std::string aid, DpAdapter::GigeFault* faults);

    int getPmCounts(string aid, BcmPm & bcmPm);

    int getFecEnable(const string& aId, bool& isFecEnable);



    int printPmCounts(unsigned int port, Bcm81725DataRate_t rate, unsigned int lane, std::string& counts, bool clear);
    int printAllLanesPmCounts(unsigned int port, std::string& counts, bool clear);
    int printPmLaneCounts(unsigned int port, unsigned int lane, std::string& counts, bool clear);


    int getCachedPortRate(unsigned int port, Bcm81725DataRate_t & rate);
    bool isPortConfigured(unsigned int port);
    int dumpPam4Diag(unsigned int port, Bcm81725DataRate_t & rate, std::string &status);
    int setFailOverPrbs(const Bcm81725Lane& param, unsigned int mode);
    int getFailOverPrbs(const Bcm81725Lane& param, unsigned int &mode);
    int UpdateUpgradableDeviceList(chm6_common::Chm6DcoCardState &stateObj);

    int getAvsTpsData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData);
    int setAvsTpsData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData);

private:
    void cachePcsPmCounts(unsigned int port, unsigned int lane, const bcm_plp_pcs_status_t& st);
    void cacheFecPmCounts(unsigned int port, unsigned int lane, const bcm_plp_fec_status_t & st);

    void SumFecPmPhysicalLanes(BcmPm & bcmPm, unsigned int lane);
    void SumPcsPmPhysicalLanes(BcmPm & bcmPm, unsigned int numLanes);
    int getEmptyFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults);
    int getOtu4Faults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults);
    int get400gFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults);
    int get4_100gFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults);
    int getFecMode(string aid, bcm_plp_fec_mode_sel_t &fecMode, Bcm81725Lane &param, int &port);

	unique_ptr<Bcm81725> myDriverPtr;
	std::map<unsigned int, BcmPm> mBcmPm;
	std::map<unsigned int, std::vector<ModeParam>> mModeParams;
	std::recursive_mutex       mGearBoxPtrMtx;

	std::mutex mGearBoxPmMtx;
	BcmTime mBcmFltPmTime[NUM_PORTS];

};

typedef SingletonService<GearBoxAdIf> GearBoxAdapterSingleton;

} // namespace DpGearBoxAdapter
#endif // __GEARBOXADAPTER_H_

