#include "GearBoxAdapterSim.h"
#include "InfnLogger.h"


namespace gearbox {

GearBoxAdapterSim::GearBoxAdapterSim()
{
}

int GearBoxAdapterSim::GetBcmInfo(BcmVers & bcmInfoTopMz, BcmVers & bcmInfoBottomMz)
{
    INFN_LOG(SeverityLevel::info) << "BCM81725 info dumped!" << endl;
    return 0;
}

int GearBoxAdapterSim::UpdateUpgradableDeviceList(chm6_common::Chm6DcoCardState &stateObj)
{
    INFN_LOG(SeverityLevel::info) << "BCM81725 UpdateDeviceList" << endl;
    return 0;
}

int GearBoxAdapterSim::loadFirmware(unsigned int bus, bool force)
{
	INFN_LOG(SeverityLevel::info) << "load bcm81725 Sim Firmware done" << endl;
	return 0;
}

int GearBoxAdapterSim::setPortRate(const string& aId, Bcm81725DataRate_t rate, bool fecEnable, bcm_plp_fec_mode_sel_t fecType)
{
	INFN_LOG(SeverityLevel::info) << "set bcm81725 Sim Date Rate Done, aId: " << aId << " rate: " << rate <<  " fecEnable: " << fecEnable << endl;
	return 0;
}



int GearBoxAdapterSim::setPortLoopback(const string& aId, Bcm81725Loopback_t loopback)
{
	INFN_LOG(SeverityLevel::info) << "set bcm81725 Sim Loopback Done, aId: " << aId << " loopback: " << loopback << endl;
	return 0;
}

int GearBoxAdapterSim::setPortFecEnable(const string& aId, bool enable)
{
	INFN_LOG(SeverityLevel::info) << "set bcm81725 Sim Fec enable done, aId: " << aId << " enable: " << enable << endl;
	return 0;
}


int GearBoxAdapterSim::getPolarity(const Bcm81725Lane& param, unsigned int& tx, unsigned int& rx)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim get polarity done" << endl;
	return 0;
}

int GearBoxAdapterSim::setPolarity(const Bcm81725Lane& param, unsigned int tx, unsigned int rx)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim set polarity done" << endl;
	return 0;
}

int GearBoxAdapterSim::warmInit(const Bcm81725Lane& param)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim warm init done" << endl;
	return 0;
}

int GearBoxAdapterSim::setLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int enable)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim set looopback done" << endl;
	return 0;
}

int GearBoxAdapterSim::getLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int &enable)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 sim get looopback done" << endl;
    return 0;
}

int GearBoxAdapterSim::setAvsConfig(const Bcm81725Lane& param, unsigned int enable, unsigned int margin)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim set avs config done" << endl;
	return 0;
}

int GearBoxAdapterSim::getAvsConfig(const Bcm81725Lane& param, unsigned int& enable, unsigned int& disableType,
		             unsigned int& ctrl, unsigned int& regulator, unsigned int& pkgShare, unsigned int& margin)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim get avs config done" << endl;
	return 0;
}

int GearBoxAdapterSim::getAvsStatus(const Bcm81725Lane& param, unsigned int& status)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim get avs status done" << endl;
	return 0;
}

int GearBoxAdapterSim::getMode(const Bcm81725Lane& param, ModeParam &modeParam)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim get mode done" << endl;
	return 0;
}

int GearBoxAdapterSim::setMode(const Bcm81725Lane& param, const ModeParam &modeParam)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim set mode done" << endl;
	return 0;
}

int GearBoxAdapterSim::dumpBcmConfig(Ports_t port, std::string &dump)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 dumpBcmConfig done" << endl;
    return 0;
}

int GearBoxAdapterSim::prbsLinkStatusCheck(const Bcm81725Lane& param, unsigned int& status)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim prbs link status check done" << endl;
	return 0;
}

int GearBoxAdapterSim::prbsStatusCheck(const Bcm81725Lane& param, unsigned int timeVal, unsigned int& status)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim prbs status check done" << endl;
	return 0;
}

int GearBoxAdapterSim::setPrbsGen(const Bcm81725Lane& param, unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim set prbs gen done" << endl;
	return 0;
}

int GearBoxAdapterSim::setPrbsCheck(const Bcm81725Lane& param, unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis)
{
	INFN_LOG(SeverityLevel::info) << "bcm81725 sim set prbs check done" << endl;
	return 0;
}

int GearBoxAdapterSim::getLaneConfigDs(const Bcm81725Lane& param, bcm_plp_dsrds_firmware_lane_config_t &lnConf)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 sim getLaneConfigDs done" << endl;
    return 0;
}

int GearBoxAdapterSim::setLaneConfigDs(const Bcm81725Lane& param, unsigned int fwLaneConfig, int64_t value)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 sim setLaneConfigDs done" << endl;
    return 0;
}


int GearBoxAdapterSim::getTxFir(const Bcm81725Lane& param, int& pre2, int& pre, int& main, int& post, int& post2, int& post3)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 sim getFir done" << endl;
    return 0;
}
int GearBoxAdapterSim::setTxFir(const Bcm81725Lane& param, int pre2, int pre, int main, int post, int post2, int post3)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 sim setFir done" << endl;
    return 0;
}

int GearBoxAdapterSim::Write(const Bcm81725Lane& param, unsigned int regAddr, unsigned int regData)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 Write done" << endl;
    return 0;
}
int GearBoxAdapterSim::Read(const Bcm81725Lane& param, unsigned int regAddr, unsigned int& regData)
{
    INFN_LOG(SeverityLevel::info) << "bcm81725 Read done" << endl;
    return 0;
}

int GearBoxAdapterSim::getPortRate(unsigned int port, Bcm81725DataRate_t &rate)
{
    INFN_LOG(SeverityLevel::info) << "Get cached port rate." << endl;
    return 0;
}

bool GearBoxAdapterSim::isPortConfigured(unsigned int port)
{
    INFN_LOG(SeverityLevel::info) << "isPortConfigured called." << endl;
    return true;
}

int GearBoxAdapterSim::getEyeScan(const Bcm81725Lane& param)
{
    INFN_LOG(SeverityLevel::info) << "Get eye scan done." << endl;
    return 0;
}

int GearBoxAdapterSim::getEyeScan(unsigned int port, Bcm81725DataRate_t rate)
{
    INFN_LOG(SeverityLevel::info) << "Get eye scan port done." << endl;
    return 0;
}

int GearBoxAdapterSim::getEyeProjection(const Bcm81725Lane& param, unsigned int lineRate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr, std::string &errMsg)
{
    INFN_LOG(SeverityLevel::info) << "Get eye scan projection done." << endl;
    return 0;
}

int GearBoxAdapterSim::getEyeProjection(unsigned int port, Bcm81725DataRate_t rate, unsigned int scanMode,
                                     unsigned int timerControl, unsigned int maxErr, std::string &errMsg)
{
    INFN_LOG(SeverityLevel::info) << "Get eye scan projection port done." << endl;
    return 0;
}

int GearBoxAdapterSim::chipStatus(const Bcm81725Lane& param, unsigned int func)
{
    INFN_LOG(SeverityLevel::info) << "Chip status done." << endl;
    return 0;
}

int GearBoxAdapterSim::clrInterrupt(const Bcm81725Lane& param)
{
    INFN_LOG(SeverityLevel::info) << "Clear interrupt done." << endl;
    return 0;
}

int GearBoxAdapterSim::getInterrupt(const Bcm81725Lane& param, unsigned int type, unsigned int *status)
{
    INFN_LOG(SeverityLevel::info) << "getInterrupt done." << endl;
    return 0;
}


int GearBoxAdapterSim::setInterrupt(const Bcm81725Lane& param, unsigned int type, bool enable)
{
    INFN_LOG(SeverityLevel::info) << "setInterrupt done." << endl;
    return 0;

}

int GearBoxAdapterSim::getPcsStatus(const Bcm81725Lane& param, bcm_plp_pcs_status_t& st)
{
    INFN_LOG(SeverityLevel::info) << "getPcsStatus done." << endl;
    return 0;
}

int GearBoxAdapterSim::printPcsStatus(const Bcm81725Lane& param, std::ostringstream& out)
{
    out << "No PCS data to print in sim." << endl;
    INFN_LOG(SeverityLevel::info) << "printPcsStatus done." << endl;
    return 0;
}

int GearBoxAdapterSim::getPcsFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    INFN_LOG(SeverityLevel::info) << "getPcsStatus faults done." << endl;
    return 0;
}


int GearBoxAdapterSim::dumpPcsStatus(unsigned int port, Bcm81725DataRate_t rate, std::string &status)
{
    INFN_LOG(SeverityLevel::info) << "getPcsStatus port done." << endl;
    return 0;
}

int GearBoxAdapterSim::printFecStatus(const Bcm81725Lane& param, bool clear, std::ostringstream& out)
{
    out << "No FEC data to print in sim." << endl;
    INFN_LOG(SeverityLevel::info) << "printFecStatus done." << endl;
    return 0;
}

int GearBoxAdapterSim::getFecStatus(const Bcm81725Lane& param, bool clear, bcm_plp_fec_status_t& st)
{
    INFN_LOG(SeverityLevel::info) << "getFecStatus done." << endl;
    return 0;
}

int GearBoxAdapterSim::dumpFecStatus(unsigned int port, Bcm81725DataRate_t rate, bool clear, std::string &status)
{
    INFN_LOG(SeverityLevel::info) << "dumpFecStatus port done." << endl;
    return 0;
}

int GearBoxAdapterSim::getFecFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    INFN_LOG(SeverityLevel::info) << "getFecFaults port done." << endl;
    return 0;
}

int GearBoxAdapterSim::getBcmFaults(string aid, DpAdapter::GigeFault* faults)
{

    INFN_LOG(SeverityLevel::info) << "getBcmFaults aid faults." << endl;
    return 0;
}

int GearBoxAdapterSim::setRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rx_control)
{
    INFN_LOG(SeverityLevel::info) << "setRxLaneControl done." << endl;
    return 0;
}


int GearBoxAdapterSim::getRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t& rx_control)
{
    INFN_LOG(SeverityLevel::info) << "getRxLaneControl done." << endl;
    return 0;
}

int GearBoxAdapterSim::setTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t tx_control)
{
    INFN_LOG(SeverityLevel::info) << "getTxLaneControl done." << endl;
    return 0;

}

int GearBoxAdapterSim::getTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t& tx_control)
{
    INFN_LOG(SeverityLevel::info) << "getTxLaneControl done." << endl;
    return 0;

}

int GearBoxAdapterSim::getPmCounts(string aid, BcmPm & bcmPm)
{
    INFN_LOG(SeverityLevel::debug) << "getPmCounts done." << endl;
    return 0;
}

int GearBoxAdapterSim::getFecEnable(const string& aId, bool& isFecEnable)
{
    INFN_LOG(SeverityLevel::info) << "getFecEnable done." << endl;

    isFecEnable = false;

    return 0;
}


int GearBoxAdapterSim::printPmCounts(unsigned int port, Bcm81725DataRate_t rate, unsigned int lane, std::string& counts, bool clear)
{
    INFN_LOG(SeverityLevel::info) << "printPmCounts done." << endl;
    return 0;
}


int GearBoxAdapterSim::printAllLanesPmCounts(unsigned int port, std::string& counts, bool clear)
{
    INFN_LOG(SeverityLevel::info) << "printAllLanesPmCounts done." << endl;
    return 0;
}


int GearBoxAdapterSim::printPmLaneCounts(unsigned int port, unsigned int lane, std::string& counts, bool clear)
{
    INFN_LOG(SeverityLevel::info) << "printPmLaneCounts done." << endl;
    return 0;
}



int GearBoxAdapterSim::getCachedPortRate(unsigned int port, Bcm81725DataRate_t & rate)
{
    INFN_LOG(SeverityLevel::info) << "getCachedPortRate done." << endl;
    return 0;
}

int GearBoxAdapterSim::dumpPam4Diag(unsigned int port, Bcm81725DataRate_t & rate, std::string& status)
{
    INFN_LOG(SeverityLevel::info) << "getPam4Diag done." << endl;
    return 0;
}

int GearBoxAdapterSim::setFailOverPrbs(const Bcm81725Lane& param, unsigned int mode)
{
    INFN_LOG(SeverityLevel::info) << "setFailOverPrbs done." << endl;
    return 0;
}

int GearBoxAdapterSim::getFailOverPrbs(const Bcm81725Lane& param, unsigned int &mode)
{
    INFN_LOG(SeverityLevel::info) << "getFailOverPrbs done." << endl;
    return 0;
}


}
