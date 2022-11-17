#ifndef __GEARBOXADIF_H__
#define __GEARBOXADIF_H__

#include "GearBoxIf.h"
 
#include <vector>
#include <string>
#include <iostream>
#include "GearBoxUtils.h"
#include "epdm_bcm_common_defines.h"
#include <string>
#include <sstream>

#include "GigeClientAdapter.h"
#include "AdapterCommon.h"
#include "dp_common.h"
#include "DpProtoDefs.h"
#include <infinera/hal/board/v2/board_state.pb.h>
#include "BcmFaults.h"
#include "BcmPm.h"


namespace gearbox {


class GearBoxAdIf
{
public:
    virtual ~GearBoxAdIf() {};

	virtual int init() = 0;
	virtual int GetBcmInfo(BcmVers & bcmInfoTopMz, BcmVers & bcmInfoBottomMz) = 0;
	virtual int loadFirmware(unsigned int bus, bool force = true) = 0;
    virtual int setPortRate(const std::string& aId, Bcm81725DataRate_t rate, bool fecEnable=false, bcm_plp_fec_mode_sel_t fecType=bcmplpKR4FEC) = 0;
    virtual int setPortLoopback(const std::string& aId, Bcm81725Loopback_t loopback) = 0;
    virtual int setPortFecEnable(const std::string& aId, bool enable) = 0;


    // internal API
    virtual int getPolarity(const Bcm81725Lane& param, unsigned int& tx, unsigned int& rx) = 0;
    virtual int setPolarity(const Bcm81725Lane& param, unsigned int tx, unsigned int rx) = 0;
    virtual int warmInit(const Bcm81725Lane& param) = 0;
    virtual int setAvsConfig(const Bcm81725Lane& param, unsigned int enable, unsigned int margin) = 0;
    virtual int getAvsConfig(const Bcm81725Lane& param, unsigned int& enable, unsigned int& disableType,
                     unsigned int& ctrl, unsigned int& regulator, unsigned int& pkgShare, unsigned int& margin) = 0;
    virtual int getAvsStatus(const Bcm81725Lane& param, unsigned int& status) = 0;
    virtual int getMode(const Bcm81725Lane& param, ModeParam &modeParam) = 0;
    virtual int setMode(const Bcm81725Lane& param, const ModeParam &modeParam) = 0;
    virtual int dumpBcmConfig(Ports_t port, std::string &dump) = 0;
    virtual int prbsLinkStatusCheck(const Bcm81725Lane& param, unsigned int& status) = 0;
    virtual int prbsStatusCheck(const Bcm81725Lane& param, unsigned int timeVal, unsigned int& status) = 0;
    virtual int setPrbsGen(const Bcm81725Lane& param, unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis) = 0;
    virtual int setPrbsCheck(const Bcm81725Lane& param, unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis) = 0;
    virtual int setLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int enable) = 0;
    virtual int getLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int &enable) = 0;
    virtual int getLaneConfigDs(const Bcm81725Lane& param, bcm_plp_dsrds_firmware_lane_config_t &lnConf) = 0;
    virtual int setLaneConfigDs(const Bcm81725Lane& param, unsigned int fwLaneConfig, int64_t value) = 0;
    virtual int setTxFir(const Bcm81725Lane& param, int pre2, int pre, int main, int post, int post2, int post3) = 0;
    virtual int getTxFir(const Bcm81725Lane& param, int& pre2, int& pre, int& main, int& post, int& post2, int& post3) = 0;

    virtual int Write(const Bcm81725Lane& param, unsigned int regAddr, unsigned int regData) = 0;
    virtual int Read(const Bcm81725Lane& param, unsigned int regAddr, unsigned int& regData) = 0;

    virtual int setRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rx_control) = 0;

    virtual int getRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t& rx_control) = 0;

    virtual int setTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t tx_control) = 0;
    virtual int getTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t& tx_control) = 0;


    virtual int getPortRate(unsigned int port, Bcm81725DataRate_t &rate) = 0;
    virtual bool isPortConfigured(unsigned int port) = 0;
    virtual int getEyeScan(const Bcm81725Lane& param) = 0;
    virtual int getEyeScan(unsigned int port, Bcm81725DataRate_t rate) = 0;
    virtual int getEyeProjection(const Bcm81725Lane& param,  unsigned int lineRate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr, std::string &errMsg) = 0;
    virtual int getEyeProjection(unsigned int port, Bcm81725DataRate_t rate, unsigned int scanMode,
                                 unsigned int timerControl, unsigned int maxErr, std::string &errMsg) = 0;
    virtual int chipStatus(const Bcm81725Lane& param, unsigned int func) = 0;

    virtual int clrInterrupt(const Bcm81725Lane& param) = 0;


    virtual int getInterrupt(const Bcm81725Lane& param, unsigned int type, unsigned int *status) = 0;


    virtual int setInterrupt(const Bcm81725Lane& param, unsigned int type, bool enable) = 0;


    virtual int getPcsFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults) = 0;
    virtual int getPcsStatus(const Bcm81725Lane& param, bcm_plp_pcs_status_t& st) = 0;
    virtual int printPcsStatus(const Bcm81725Lane& param, std::ostringstream& out) = 0;
    virtual int dumpPcsStatus(unsigned int port, Bcm81725DataRate_t rate, std::string &status) = 0;

    virtual int printFecStatus(const Bcm81725Lane& param, bool clear, std::ostringstream& out) = 0;
    virtual int getFecStatus(const Bcm81725Lane& param, bool clear, bcm_plp_fec_status_t& st) = 0;
    virtual int dumpFecStatus(unsigned int port, Bcm81725DataRate_t rate, bool clear, std::string &status) = 0;
    virtual int getFecFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults) = 0;


    virtual int getBcmFaults(std::string aid, DpAdapter::GigeFault* faults) = 0;

    virtual int getPmCounts(string aid, BcmPm & bcmPm) = 0;

    virtual int getFecEnable(const string& aId, bool& isFecEnable) = 0;


    virtual int printPmCounts(unsigned int port, Bcm81725DataRate_t rate, unsigned int lane, std::string& counts, bool clear) = 0;
    virtual int printAllLanesPmCounts(unsigned int port, std::string& counts, bool clear) = 0;
    virtual int printPmLaneCounts(unsigned int port, unsigned int lane, std::string& counts, bool clear) = 0;



    virtual int getCachedPortRate(unsigned int port, Bcm81725DataRate_t & rate) = 0;
    virtual int dumpPam4Diag(unsigned int port, Bcm81725DataRate_t & rate, std::string &status) = 0;
    virtual int setFailOverPrbs(const Bcm81725Lane& param, unsigned int mode) = 0;
    virtual int getFailOverPrbs(const Bcm81725Lane& param, unsigned int &mode) = 0;
    virtual int UpdateUpgradableDeviceList(chm6_common::Chm6DcoCardState &stateObj) = 0;

    virtual int getAvsTpsData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData) = 0;
    virtual int setAvsTpsData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData) = 0;


};

}
#endif // __GEARBOXADIF_H_
