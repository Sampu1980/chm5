#ifndef __GEARBOXDRIVER_H__
#define __GEARBOXDRIVER_H__



#include <iostream>
#include <thread>
#include <mutex>
#include "GearBoxIf.h"
#include "FpgaMdioIf.h"
#include "FpgaRegIf.h"
 
#include "SimpleLog.h"
#include "bcm_pm_if/bcm_pm_if_api.h"
#include "GearBoxUtils.h"
#include "epdm_bcm_common_defines.h"

using namespace std;

namespace gearbox {



class Bcm81725 :  public GearBoxIf
{
public:
	Bcm81725();
	~Bcm81725();
    
    int init();
    int GetBcmInfo(const Bcm81725Lane& param, unsigned int& fw_version, unsigned int& fw_crc);
    string PrintErrorCode(int errorCode);
	int loadFirmware(const unsigned int &bus);
	int warmInit(const Bcm81725Lane& param);

    void dumpLog(std::ostream &os);
    void dumpStatus(std::ostream &os, std::string cmd);
    void resetLog( std::ostream &os );
    void addLog(const std::string &func, uint32_t line, const std::string &text);

    int getPolarity(const Bcm81725Lane& param, unsigned int& tx, unsigned int& rx);
	int setPolarity(const Bcm81725Lane& param, unsigned int tx, unsigned int rx);
	int setAvsConfig(const Bcm81725Lane& param, unsigned int enable, unsigned int margin);
	int getAvsConfig(const Bcm81725Lane& param, unsigned int& enable, unsigned int& disableType, 
		             unsigned int& ctrl, unsigned int& regulator, unsigned int& pkgShare, unsigned int& margin);
	int getAvsStatus(const Bcm81725Lane& param, unsigned int& status);
	int getMode(const Bcm81725Lane& param, int& speed, int& ifType, int& refClk, int& interfaceMode,
                unsigned int& dataRate, unsigned int& modulation, unsigned int& fecType);
	int setMode(const Bcm81725Lane& param, int speed, int ifType, int refClk, int interfaceMode,
                unsigned int dataRate, unsigned int modulation, unsigned int fecType);
	int prbsLinkStatusCheck(const Bcm81725Lane& param, unsigned int& status);
	int prbsStatusCheck(const Bcm81725Lane& param, unsigned int timeVal, unsigned int& status);
	int setPrbsGen(const Bcm81725Lane& param, unsigned int tx, unsigned int poly, 
                   unsigned int inv, unsigned int lb, unsigned int enaDis);
	int setPrbsCheck(const Bcm81725Lane& param, unsigned int rx, unsigned int poly, 
                   unsigned int inv, unsigned int lb, unsigned int enaDis);
	int setLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int enable);
	int getLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int &enable);
	int setFir(const Bcm81725Lane& param, bcm_plp_pam4_tx_t *pfir);
	int getFir(const Bcm81725Lane& param, bcm_plp_pam4_tx_t *pfir);
	int getLaneConfigDS(const Bcm81725Lane& param, bcm_plp_dsrds_firmware_lane_config_t *lnConf);
	int setLaneConfigDS(const Bcm81725Lane& param, unsigned int field, int64_t value);

	int setRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rx_control);
	int getRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t& rx_control);
	int setTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t tx_control);
	int getTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t& tx_control);

	int getEyeScan(const Bcm81725Lane& param);
	int getEyeProjection(const Bcm81725Lane& param, unsigned int lineRate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr, string &errMsg);
	int chipStatus(const Bcm81725Lane& param, unsigned int func);

	int clrInterrupt(const Bcm81725Lane& param);
	int getInterrupt(const Bcm81725Lane& param, unsigned int type, unsigned int *status);
	int setInterrupt(const Bcm81725Lane& param, unsigned int typeMask, bool enable);

	int getPcsStatus(const Bcm81725Lane& param, bcm_plp_pcs_status_t& st);
	int getFecStatus(const Bcm81725Lane& param, bool clear, bcm_plp_fec_status_t& st);
	int iWrite(const Bcm81725Lane& param, uint32 regAddr, uint16 data);
	int iRead(const Bcm81725Lane& param, uint32 regAddr, uint16& data);
	int getPam4Diag(const Bcm81725Lane& param, bcm_plp_pm_phy_pam4_diagnostics_t *pam4);
    int setFailOverPrbs(const Bcm81725Lane& param, unsigned int mode);
    int getFailOverPrbs(const Bcm81725Lane& param, unsigned int &mode);

private:
	std::recursive_mutex myBcmMtx;
	const char*  myMilleniObPtr;
	FpgaMdioIf* myMdioRAPtr;
	FpgaRegIf* myFpgaRAPtr;
    // std::recursive_mutex myMdioMtx;
	std::mutex myLogMtx;
	SimpleLog::Log*  myLogPtr;
};

int mdio_read(void *user, unsigned int mdioAddr, unsigned int regAddr, unsigned int *data);
int mdio_write(void *user, unsigned int mdioAddr, unsigned int regAddr, unsigned int data);

}
#endif // __GEARBOXDRIVER_H_
