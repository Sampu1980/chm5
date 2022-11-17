#include "epdm.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <cassert>
#include "Bcm81725.h"
#include "InfnLogger.h"
#include "GearBoxUtils.h"
#include <sstream>


namespace gearbox {

const uint32_t FPGA_BASE_ADDR = 0xA0000000;
const uint32_t FPGA_SIZE        = 0xFFFF;
const uint32_t FPGA_MDIO_OFFSET    = 0x0300;
const uint32_t MDIO_FREQ_RATIO  = 0x12;
const uint32_t REG_SIZE = 0xFFFF;
const unsigned int cTopMz    = 1;
const unsigned int cBottomMz = 0;
const unsigned int cTopMzMdioAddrMask = 0x10;

const unsigned int cPhyIdx0[] = {0x04, 0x08, 0x0c};
const unsigned int cPhyIdx1[] = {0x014, 0x018, 0x01c};
const unsigned int cNumPhyIdx0 = (sizeof(cPhyIdx0) / sizeof(cPhyIdx0[0]));
const unsigned int cNumPhyIdx1 = (sizeof(cPhyIdx1) / sizeof(cPhyIdx1[0]));


const std::string MDIO_RA_NAME = "Bcm81725MdioRA";

static FpgaMdioIf* mdioIfPtr;


int mdio_write(void *user, unsigned int mdioAddr, unsigned int regAddr, unsigned int data)
{
//    INFN_LOG(SeverityLevel::info) << "mdio_write: " << std::hex << regAddr << "  " << data << endl;
    if (!user)
    {
        INFN_LOG(SeverityLevel::info) << " mdio_write user empty!!!" << endl;
        return -1;
    }

    uint8_t busSel = (uint8_t)(*((unsigned int*)user));



    if ( (busSel == cTopMz) || (mdioAddr >> 4 == 1) )
    {

        // Top mz, get lower nibble
        mdioAddr &= 0xF;
    }


    try
    {
        mdioIfPtr->Write16(busSel, mdioAddr, regAddr, data);
    }
    catch ( ... )
    {
        INFN_LOG(SeverityLevel::info) << " mdio_write failed!!!" << endl;
        return -1;
    }

    return 0;
}

int mdio_read(void *user, unsigned int mdioAddr, unsigned int regAddr, unsigned int *data)
{
    if (!user || !data)
    {
        INFN_LOG(SeverityLevel::info) << " mdio_read invalid parameter!!!" << endl;
        return -1;
    }

    uint8_t busSel = (uint8_t)(*((unsigned int*)user));




    if ( (busSel == cTopMz) || (mdioAddr >> 4 == 1) )
    {

        // Top mz, get lower nibble
        mdioAddr &= 0xF;
    }

    try {
        *data = static_cast<unsigned int>(mdioIfPtr->Read16(busSel, mdioAddr, regAddr));
    }
    catch ( ... ) {
        INFN_LOG(SeverityLevel::info) << " mdio_read failed!!!" << endl;
        return -1;
    }
//    INFN_LOG(SeverityLevel::info) << "mdio_read: " << std::hex << regAddr << "  " << *data << endl;
    return 0;
}

Bcm81725::Bcm81725() : myLogPtr(new SimpleLog::Log(2000)), myMilleniObPtr("milleniob")
{


    // todo: switch to factory for creation
	myFpgaRAPtr = new FpgaRegIf(FPGA_BASE_ADDR, FPGA_SIZE);

    if (!myFpgaRAPtr)
    {
        return;
    }

    // todo: switch to factory for creation
	myMdioRAPtr = new FpgaMdioIf(MDIO_RA_NAME,
	                             myBcmMtx,
	                             FPGA_MDIO_OFFSET,
                                 MDIO_FREQ_RATIO,
		                         static_cast<RegIf*>(myFpgaRAPtr),
		                         REG_SIZE);
    if (!myMdioRAPtr) {
        return;
    }
	mdioIfPtr = myMdioRAPtr;
}

Bcm81725::~Bcm81725()
{
    delete myFpgaRAPtr;
    delete myMdioRAPtr;
}


int Bcm81725::warmInit(const Bcm81725Lane& param)
{
    int rv = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));
    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }
    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;
    phyInfo.flags = BCM_PLP_WARM_BOOT;

    INFN_LOG(SeverityLevel::info) << " Bus=" << param.bus << std::hex << " Addr=0x" << phyInfo.phy_addr << " side=" << phyInfo.if_side << " laneMask=0x" << phyInfo.lane_map <<
            " flags=" << phyInfo.flags << endl;

    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      Initialize chip and verify Firmware
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    bcm_plp_firmware_load_type_t fwLoadType;
    memset(&fwLoadType, 0, sizeof(bcm_plp_firmware_load_type_t));
    fwLoadType.firmware_load_method = bcmpmFirmwareLoadMethodInternal;

    // skip installs read/write accessors and checks version
    fwLoadType.force_load_method = bcmpmFirmwareLoadSkip;

#ifdef USE_SOCKDA
    /* CoreReset is needed for warm reset to zero info and install drivers */
    rv |= bcm_plp_init_fw_bcast(milleniob, phyInfo, mdio_read, mdio_write,
            &fwLoadType, bcmpmFirmwareBroadcastCoreReset);
    if (rv != 0) {
        INFN_LOG(SeverityLevel::info) << "USE_SOCKDA bcm_plp_init_fw_bcast failed." << endl;
        return rv;
    }

    usleep(100000); /* After Soft reset sleep */
#else
    rv |= bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
            &fwLoadType, bcmpmFirmwareBroadcastCoreReset);
    if (rv != 0) {
        INFN_LOG(SeverityLevel::info) << "step 1: bcm_plp_init_fw_bcast failed." << endl;
        return rv;
    }

    sleep(1); /* After Hard reset sleep */
    rv |= bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
            &fwLoadType, bcmpmFirmwareBroadcastEnable);
    if (rv != 0) {
        INFN_LOG(SeverityLevel::info) << "step 2: bcm_plp_init_fw_bcast failed." << endl;
        return rv;
    }

    rv |= bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
            &fwLoadType, bcmpmFirmwareBroadcastFirmwareExecute);
    if (rv != 0) {
        INFN_LOG(SeverityLevel::info) << "step 3: bcm_plp_init_fw_bcast failed." << endl;
        return rv;
    }

    rv |= bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
            &fwLoadType, bcmpmFirmwareBroadcastFirmwareVerify);
    if (rv != 0) {
        INFN_LOG(SeverityLevel::info) << "step 4: bcm_plp_init_fw_bcast failed." << endl;
        return rv;
    }

    rv |= bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
            &fwLoadType, bcmpmFirmwareBroadcastEnd);
    if (rv != 0) {
        INFN_LOG(SeverityLevel::info) << "step 5: bcm_plp_init_fw_bcast failed." << endl;
        return rv;
    }
#endif
    return rv;
}

int Bcm81725::GetBcmInfo(const Bcm81725Lane& param, unsigned int& fw_version, unsigned int& fw_crc)
{
    int rc = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));
    phyInfo.platform_ctxt = (void*)(&param.bus);


    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }
    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    rc |= bcm_plp_firmware_info_get(myMilleniObPtr, phyInfo, &fw_version, &fw_crc);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_firmware_info_get failed with rc:" << rc << endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_firmware_info_get fw_version = " << fw_version << " fw_crc = " << fw_crc << endl;
    }
    return rc;
}

string Bcm81725::PrintErrorCode(int errorCode)
{
    string errorString;

    switch(errorCode)
    {
    case -1:
        errorString = " Internal error returned by PHYMOD layer";
        break;
    case -2:
        errorString = " Memory Allocation error returned by PHYMOD layer";
        break;
    case -3:
        errorString = " I/O Error returned by PHYMOD layer";
        break;
    case -4:
        errorString = " Invalid Param error returned by PHYMOD layer";
        break;
    case -5:
        errorString = " Core error returned by PHYMOD layer";
        break;
    case -6:
        errorString = " Phy error returned by PHYMOD layer";
        break;
    case -7:
        errorString = " Busy error returned by PHYMOD layer";
        break;
    case -8:
        errorString = " Failure error returned by PHYMOD layer";
        break;
    case -9:
        errorString = " Timeout returned by PHYMOD layer";
        break;
    case -10:
        errorString = " Resource error returned by PHYMOD layer";
        break;
    case -11:
        errorString = " Configuration error returned by PHYMOD layer";
        break;
    case -12:
        errorString = " Unavailable error returned by PHYMOD layer";
        break;
    case -13:
        errorString = " Initialization error returned by PHYMOD layer";
        break;
    case -14:
        errorString = " Out of bound error returned by PHYMOD layer";
        break;
    case -20:
        errorString = " Chip not found returned by BCM API layer";
        break;
    case -21:
        errorString = " Memory allocation failed returned by BCM API layer";
        break;
    case -22:
        errorString = " Particular core not found returned by BCM API layer";
        break;
    case -23:
        errorString = " Internal code error returned by BCM API layer";
        break;
    case -24:
        errorString = " Trying to initialize existing/initialized PHY returned by BCM API layer";
        break;
    case -25:
        errorString = " Method not applicable returned by BCM API layer";
        break;
    case -26:
        errorString = " Invalid PHY (Mismatched IDs) returned by BCM API layer";
        break;
    case -27:
        errorString = " Functionality not supported or unavailable returned by BCM API layer";
        break;
    case -28:
        errorString = " Invalid Parameter returned by BCM API layer";
        break;
    default:
        errorString = " Unknown error code!";
        break;

    }


    return errorString;
}

int Bcm81725::loadFirmware(const unsigned int & bus) {

    int rv = 0;
    unsigned int *phyIdx = (unsigned int *)cPhyIdx0;
    unsigned int numPhyIdx = cNumPhyIdx0;

    unsigned int phyId = 0;
    unsigned int fwVer = 0, fwCrc = 0;

    bcm_plp_access_t  phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));
    phyInfo.platform_ctxt = (void*)(&bus);
    phyInfo.flags = BCM_PLP_COLD_BOOT;

    if (bus == cTopMz)
    {
        phyIdx = (unsigned int *)cPhyIdx1;
        numPhyIdx = cNumPhyIdx1;
    }


    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      Initialize chip and download Firmware
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

    bcm_plp_firmware_load_type_t fwLoadType;
    memset(&fwLoadType, 0, sizeof(bcm_plp_firmware_load_type_t));
    fwLoadType.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    /* In force firmware download method, downloads the firmware always */
    fwLoadType.force_load_method = bcmpmFirmwareLoadForce;

    /* If user wants to select auto firmware download method, it checks firmware version on chip if it is not latest firmware,
     * it downloads otherwise it won't downloads the firmware. User need to select force_load_method as bcmpmFirmwareLoadAuto.
     * if auto download needed, uncomment below code.
     */
    /* fwLoadType.force_load_method = bcmpmFirmwareLoadAuto; */


    /** Step-1: Reset the core for all phy id in mdio bus *   */
    for (phyId = 0; phyId < numPhyIdx; ++phyId) {
        phyInfo.phy_addr = phyIdx[phyId];
        if((rv = bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
                                       &fwLoadType, bcmpmFirmwareBroadcastCoreReset)) != 0) {

            return rv;
        }

    }
    sleep(1); /* After Hard reset sleep */

    /** Step-2: Enable the broadcast for all phy id in mdio bus * */
    for (phyId = 0; phyId < numPhyIdx; ++phyId) {
        phyInfo.phy_addr = phyIdx[phyId];
        if((rv = bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
                                       &fwLoadType, bcmpmFirmwareBroadcastEnable)) != 0) {
            return rv;
        }

    }

    /** Step-3: Load the FW for only one phyId, internally it will
     *  broadcast firmware of similar type of phys on same mdio bus
     *  * */
    phyId = 0;
    phyInfo.phy_addr = phyIdx[phyId];  /* Load Firmware for any one Phy/Core in BCAST_LIST */
    if((rv = bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
                                   &fwLoadType, bcmpmFirmwareBroadcastFirmwareExecute)) != 0) {
        return rv;
    }



    /** Step-4: Disable the broadcast and FW load verify for all phy
     *  id in mdio bus  * */
    for (phyId = 0; phyId < numPhyIdx; ++phyId) {
        phyInfo.phy_addr = phyIdx[phyId];
        if((rv = bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
                                       &fwLoadType, bcmpmFirmwareBroadcastFirmwareVerify)) != 0) {
            return rv;
        }


    }

    /** Step-5: End of the broadcast for all phy id in mdio bus
                 */
    for (phyId = 0; phyId < numPhyIdx; ++phyId) {
        phyInfo.phy_addr = phyIdx[phyId];
        if((rv = bcm_plp_init_fw_bcast(myMilleniObPtr, phyInfo, mdio_read, mdio_write,
                                       &fwLoadType, bcmpmFirmwareBroadcastEnd)) != 0) {
            return rv;
        }
    }

    /* FW Version get */
    for (phyId = 0; phyId < numPhyIdx; ++phyId) {
        phyInfo.phy_addr = phyIdx[phyId];
        rv  = bcm_plp_firmware_info_get(myMilleniObPtr, phyInfo, &fwVer, &fwCrc);
        if (rv != 0) {
            return rv;
        }

    }

    return rv;
}
int Bcm81725::init()
{

	if (loadFirmware(0)) {
        return -1;
	}

    if (loadFirmware(1)) {
        return -1;
    }

    return 0;
}


void Bcm81725::dumpLog(std::ostream &os)
{
    unique_lock<mutex> lck(myLogMtx);
    os << "<<<<<<<<<<<<<<<<<<< Bcm81725.dump_log >>>>>>>>>>>>>>>>>>>>>>" << endl << endl;

    if (myLogPtr) {
        myLogPtr->DisplayLog(os);
    }
    else {
        os << "TomDriver Log is not created!" << endl;
    }
}

void Bcm81725::dumpStatus(std::ostream &os, std::string cmd)
{
}

void Bcm81725::resetLog( std::ostream &os )
{
   unique_lock<mutex> lck(myLogMtx);

    if (myLogPtr) {
        myLogPtr->ResetLog();
        os << "Bcm81725 Log has been reset!" << endl;
    }
}

void Bcm81725::addLog(const std::string &func, uint32_t line, const std::string &text)
{
    unique_lock<mutex> lck(myLogMtx);

    ostringstream  os;
    os << "Bcm81725::" << func << ":" << line << ": " << text;

    if (myLogPtr) {
        myLogPtr->AddRecord(os.str().c_str());
    }
}

int Bcm81725::getPolarity(const Bcm81725Lane& param, unsigned int& tx, unsigned int& rx)
{
    std::ostringstream  log;
    log << " Bcm81725::getPolarity ";

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    // if (phyInfo.lane_map == 0) return -1;

    int rc = bcm_plp_polarity_get(myMilleniObPtr, phyInfo, &tx, &rx);
    if(!rc){
        log << "bcm_plp_polarity_get tx=0x" << tx << "rx=0x" << rx << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_polarity_get tx=0x" << tx << "rx=0x" << rx << endl;
    }
    else {
        log << "bcm_plp_polarity_get failed, rc = " << rc << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_polarity_get failed, rc = " << rc << endl;
    }

    addLog(__func__, __LINE__, log.str());
    return rc;
}

int Bcm81725::setPolarity(const Bcm81725Lane& param, unsigned int tx, unsigned int rx)
{
    std::ostringstream  log;
    log << " Bcm81725::setPolarity ";
    INFN_LOG(SeverityLevel::info) << " Bcm81725::setPolarity " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    //if (phyInfo.lane_map == 0) return -1;

    int rc = bcm_plp_polarity_set(myMilleniObPtr, phyInfo, tx, rx);
    if(!rc){
        log << "bcm_plp_polarity_set successfully tx=0x" << tx << "rx=0x" << rx << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_polarity_set successfully tx=0x" << tx << "rx=0x" << rx << endl;
    }
    else {
        log << "bcm_plp_polarity_get failed, rc = " << rc << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_polarity_get failed, rc = " << rc << endl;
    }

    addLog(__func__, __LINE__, log.str());
    return rc;
}

int Bcm81725::setAvsConfig(const Bcm81725Lane& param, unsigned int enable, unsigned int margin)
{
    std::ostringstream  log;
    log << " Bcm81725::setAvsConfig ";
    INFN_LOG(SeverityLevel::info) << " Bcm81725::setAvsConfig " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    bcm_plp_avs_config_t avsConfig;
    memset(&avsConfig, 0, sizeof(bcm_plp_avs_config_t));

    avsConfig.enable = enable;
    avsConfig.avs_disable_type = bcmplpAvsDisableTypeFirmwareControl;
    avsConfig.avs_disable_type = bcmplpAvsDisableTypeNoFirmwareControl;
    avsConfig.avs_ctrl = bcmplpAvsControlInternal;
    avsConfig.avs_regulator = bcmplpAvsRegulatorTPS546;      /**< TPS546 regulator */
    avsConfig.regulator_i2c_address = 0x1E;
    avsConfig.avs_pkg_share = bcmplpAvsPackageShare1;
    avsConfig.avs_dc_margin = (bcm_plp_avs_board_dc_margin_e) margin; // bcmplpAvsBoardDcMargin0mV;
    avsConfig.ref_clk = bcm_pm_RefClk156Mhz;

    int rc = bcm_plp_avs_config_set(myMilleniObPtr, phyInfo, avsConfig);
    if(!rc){
        log << "bcm_plp_avs_config_set successfully" << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_avs_config_set successfully" << endl;
    }
    else {
        log << "bcm_plp_avs_config_set failed, rc = " << rc << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_avs_config_set failed, rc = " << rc << endl;
    }

    addLog(__func__, __LINE__, log.str());
    return rc;
}

int Bcm81725::getAvsConfig(const Bcm81725Lane& param, unsigned int& enable, unsigned int& disableType,
                     unsigned int& ctrl, unsigned int& regulator, unsigned int& pkgShare, unsigned int& margin)
{
    std::ostringstream  log;
    log << " Bcm81725::getAvsConfig ";
    INFN_LOG(SeverityLevel::info) << " Bcm81725::getAvsConfig " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    bcm_plp_avs_config_t avsConfig;
    memset(&avsConfig, 0, sizeof(bcm_plp_avs_config_t));

    int rc = bcm_plp_avs_config_get(myMilleniObPtr, phyInfo, &avsConfig);
    if(!rc){
        enable = avsConfig.enable;
        disableType = avsConfig.avs_disable_type;
        ctrl = avsConfig.avs_ctrl;
        regulator = avsConfig.avs_regulator;
        pkgShare = avsConfig.avs_pkg_share;
        margin = avsConfig.avs_dc_margin;

        log << "bcm_plp_avs_config_get successfully" << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_avs_config_get successfully" << endl;
    }
    else {
        log << "bcm_plp_avs_config_get failed, rc = " << rc << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_avs_config_get failed, rc = " << rc << endl;
    }

    addLog(__func__, __LINE__, log.str());
    return rc;
 }

int Bcm81725::getAvsStatus(const Bcm81725Lane& param, unsigned int& status)
{
    std::ostringstream  log;
    log << " Bcm81725::getAvsConfig ";
    INFN_LOG(SeverityLevel::info) << " Bcm81725::getAvsConfig " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    bcm_plp_avs_config_status_t avsStatus;
    memset(&avsStatus, 0, sizeof(bcm_plp_avs_config_status_t));

    int rc = bcm_plp_avs_status_get(myMilleniObPtr, phyInfo, &avsStatus);
    if(!rc){
        status = avsStatus.enable;
        log << "bcm_plp_avs_status_get successfully" << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_avs_status_get successfully" << endl;
    }
    else {
        log << "bcm_plp_avs_status_get failed, rc = " << rc << endl;
        INFN_LOG(SeverityLevel::info) << "bcm_plp_avs_status_get failed, rc = " << rc << endl;
    }

    addLog(__func__, __LINE__, log.str());
    return rc;
}

int Bcm81725::getMode(const Bcm81725Lane& param, int& speed, int& ifType, int& refClk, int& interfaceMode,
                unsigned int& dataRate, unsigned int& modulation, unsigned int& fecType)
{
    INFN_LOG(SeverityLevel::debug) << " Bcm81725::getMode " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);


    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    bcm_plp_milleniob_device_aux_modes_t auxMode;
    memset(&auxMode, 0, sizeof(bcm_plp_milleniob_device_aux_modes_t));

    INFN_LOG(SeverityLevel::debug) << " getMode:" << " bus=" << param.bus
    		<< " mdioAddr=" << phyInfo.phy_addr << " side=" << phyInfo.if_side << " lane=" <<  phyInfo.lane_map << endl;

    int rc = bcm_plp_mode_config_get(myMilleniObPtr, phyInfo,  &speed, &ifType, &refClk, &interfaceMode, (void*)&auxMode);

    if(!rc){
        dataRate = auxMode.lane_data_rate;
        modulation = auxMode.modulation_mode;
        fecType = auxMode.fec_mode_sel;
        INFN_LOG(SeverityLevel::debug) << "bcm_plp_mode_config_get successfully" << endl;

        INFN_LOG(SeverityLevel::debug) << " getMode:" << " speed=" << speed << " ifType=" << ifType << " refClk=" << refClk << " interfaceMode=" << interfaceMode << " dataRate=" << dataRate << endl <<
                        " modulation=" << modulation << " fecType=" << fecType << endl;
    }
    else {

        INFN_LOG(SeverityLevel::info) << "bcm_plp_mode_config_get failed, rc = " << rc << endl;
    }


    return rc;
}

int Bcm81725::setMode(const Bcm81725Lane& param, int speed, int ifType, int refClk, int interfaceMode,
                unsigned int dataRate, unsigned int modulation, unsigned int fecType)
{


    INFN_LOG(SeverityLevel::info) << " Bcm81725::setMode " << endl;


    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    bcm_plp_milleniob_device_aux_modes_t auxMode;
    memset(&auxMode, 0, sizeof(bcm_plp_milleniob_device_aux_modes_t));

    auxMode.lane_data_rate   = (bcm_plp_lane_data_rate_t)dataRate;
    auxMode.modulation_mode  = (bcm_plp_modulation_mode_t)modulation;
    auxMode.fec_mode_sel     = (bcm_plp_fec_mode_sel_t)fecType;

    int rc = bcm_plp_mode_config_set(myMilleniObPtr, phyInfo, speed, ifType, refClk, interfaceMode, (void*)&auxMode);

    if(!rc)
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_mode_config_set successfully" << endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_mode_config_set failed, rc = " << rc << endl;
    }


    INFN_LOG(SeverityLevel::info) << "Bus=" << param.bus << " Addr=" << phyInfo.phy_addr << " Side=" << param.side << " Lane=" << param.laneNum <<
            " speed=" << speed << " ifType=" << ifType << " refClk=" << refClk << " intfMode=" << interfaceMode <<
            " dataRate=" << dataRate << " mod=" << modulation << " fecType=" << fecType << endl;
    return rc;
}

int Bcm81725::prbsLinkStatusCheck(const Bcm81725Lane& param, unsigned int& status)
{
    INFN_LOG(SeverityLevel::info) << " Bcm81725::setPrbsGen " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    // if (mask != 0) phy_info.lane_map &= mask;
    // if (phy_info.lane_map == 0) continue;

    status = 0;
    int rc = bcm_plp_link_status_get(myMilleniObPtr, phyInfo, &status);
    if(!rc)
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_link_status_get successfully" << endl;
    }
    else 
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_link_status_get failed, rc = " << rc << endl;
    }

    return rc;
}

int Bcm81725::setLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int enable)
{
    INFN_LOG(SeverityLevel::info) << " Bcm81725::setLoopback " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    int status = bcm_plp_loopback_set(myMilleniObPtr, phyInfo, mode, enable);
    if(!status){
        INFN_LOG(SeverityLevel::info) << "bcm_plp_loopback_set successfully, status is OK" << endl;
    }
    else {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_loopback_set failed, rc = " << status << endl;
    }

    return status;
}

int Bcm81725::getLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int &enable)
{
    INFN_LOG(SeverityLevel::info) << " Bcm81725::getLoopback " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    int status = bcm_plp_loopback_get(myMilleniObPtr, phyInfo, mode, &enable);
    if(!status){
        INFN_LOG(SeverityLevel::info) << "bcm_plp_loopback_get successfully, status is OK" << endl;
        INFN_LOG(SeverityLevel::info) << "loopback is " << enable << endl;
    }
    else {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_loopback_get failed, rc = " << status << endl;
    }

    return status;
}

int Bcm81725::prbsStatusCheck(const Bcm81725Lane& param, unsigned int timeVal, unsigned int& status)
{
    INFN_LOG(SeverityLevel::info) << " Bcm81725::setPrbsGen " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    status = bcm_plp_prbs_rx_stat(myMilleniObPtr, phyInfo, timeVal);
    if(!status){
        INFN_LOG(SeverityLevel::info) << "bcm_plp_prbs_set successfully, status is OK" << endl;
    }
    else {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_prbs_set failed, rc = " << status << endl;
    }

    return status;
}

int Bcm81725::setPrbsGen(const Bcm81725Lane& param, unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis)
{
    INFN_LOG(SeverityLevel::info) << " Bcm81725::setPrbsGen " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    // if (mask != 0) phy_info.lane_map &= mask;
    // if (phy_info.lane_map == 0) continue;

    int rc = bcm_plp_prbs_set(myMilleniObPtr, phyInfo, tx, poly, inv, lb, enaDis);
    if(!rc){
        INFN_LOG(SeverityLevel::info) << "bcm_plp_prbs_set successfully" << endl;
    }
    else {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_prbs_set failed, rc = " << rc << endl;
    }

    return rc;
}

int Bcm81725::setPrbsCheck(const Bcm81725Lane& param, unsigned int rx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis)
{
    INFN_LOG(SeverityLevel::info) << " Bcm81725::setPrbsGen " << endl;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    // if (mask != 0) phy_info.lane_map &= mask;
    // if (phy_info.lane_map == 0) continue;

    int rc = bcm_plp_prbs_set(myMilleniObPtr, phyInfo, rx, poly, inv, lb, enaDis);
    if(!rc){
        INFN_LOG(SeverityLevel::info) << "bcm_plp_prbs_set successfully" << endl;
    }
    else {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_prbs_set failed, rc = " << rc << endl;
    }

    return rc;
}



int Bcm81725::getFir(const Bcm81725Lane& param, bcm_plp_pam4_tx_t *pfir)
{
    int rc = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;


    rc = bcm_plp_pam4_tx_get(myMilleniObPtr, phyInfo, pfir);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::info) << "getFir bcm_plp_phy_pam4_tx_get failed with rc:" << rc << endl;
    }

    return rc;
}

int Bcm81725::setFir(const Bcm81725Lane& param, bcm_plp_pam4_tx_t *pfir)
{
    int rc = 0;
    std::ostringstream  log;
    log << " Bcm81725::setFir ";

    INFN_LOG(SeverityLevel::info) << " Bcm81725::setFir " << endl;


    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;

    rc |= bcm_plp_pam4_tx_set(myMilleniObPtr, phyInfo, pfir);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::info) << "setFir bcm_plp_phy_pam4_tx_set failed with rc:" << rc << endl;
    }

    addLog(__func__, __LINE__, log.str());

    return rc;
}


int Bcm81725::getLaneConfigDS(const Bcm81725Lane& param, bcm_plp_dsrds_firmware_lane_config_t *lnConf)
{
    int rc = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;


    rc |= bcm_plp_dsrds_firmware_lane_config_get(myMilleniObPtr, phyInfo, lnConf);

    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_dsrds_firmware_lane_config_get API failed with rc:" << rc << endl;
    }

    return rc;
}

/*
* set dsrds lane config
*/
int Bcm81725::setLaneConfigDS(const Bcm81725Lane& param, unsigned int field, int64_t value)
{
    int rc = 0;

    INFN_LOG(SeverityLevel::info) << " setLaneConfigDS " << endl;


    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;


    bcm_plp_dsrds_firmware_lane_config_t lnConf;
    memset(&lnConf, 0, sizeof(bcm_plp_dsrds_firmware_lane_config_t));


    rc |= bcm_plp_dsrds_firmware_lane_config_get(myMilleniObPtr, phyInfo, &lnConf);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_dsrds_firmware_lane_config_set API failed with rc:" << rc << endl;
        return rc;
    }

    switch(field)
    {
        case DS_MediaType:
            /* bcmplpFirmwareMediaTypePcbTraceBackPlane    0
               bcmplpFirmwareMediaTypeCopperCable          1
               bcmplpFirmwareMediaTypeOptics               2 */
            lnConf.MediaType = (bcm_pm_firmware_media_type_e) value;
            break;
        case DS_DfeOn:
            lnConf.DfeOn = value;
            break;
        case DS_AnEnabled:
            lnConf.AnEnabled = value;
            break;
        case DS_AutoPeakingEnable:
            lnConf.AutoPeakingEnable = value;
            break;
        case DS_OppositeCdrFirst:
            lnConf.OppositeCdrFirst = value;
            break;
        case DS_DcWanderMu:
            lnConf.DcWanderMu = value;
            break;
        case DS_ExSlicer:
            lnConf.ExSlicer = value;
            break;
        case DS_LinkTrainingReStartTimeOut:
            lnConf.LinkTrainingReStartTimeOut = value;
            break;
        case DS_LinkTrainingCL72_CL93PresetEn:
            // LT RX send cl72/cl93 preset request:
            // 0:Disable (non-IEEE compat mode);
            // 1: Enable(IEEE compat mode, default value)
            lnConf.LinkTrainingCL72_CL93PresetEn = value;
            break;
        case DS_LinkTraining802_3CDPresetEn:
            // LT RX send 802.3cd preset1 request:
            // 0:Disable (non-IEEE compat mode);
            // 1: Enable (IEEE compat mode, default value)
            lnConf.LinkTraining802_3CDPresetEn = value;
            break;
        case DS_LinkTrainingTxAmpTuning:
            // Copper PAM4 Link training,
            // 0: RX adjust LP's TX TXFIR with fixed 7 steps;
            // 1: Copper PAM4 Link training, RX adjust LP's TX TXFIR dynamically
            lnConf.LinkTrainingTxAmpTuning = value;
            break;
        case DS_LpDfeOn:
            // Enable Low power DFE
            lnConf.LpDfeOn  = value;
            break;
        case DS_AnTimer:
            //0:IEEE standard 3s link inhibit timer,
            //1:6s  link inhibit timer: Applicable only PAM4 modes
            lnConf.AnTimer  = value;
            break;
        default:
            INFN_LOG(SeverityLevel::info) << "Ooops program error. Undefined field=" << field << endl;
            rc = -1;
            break;
    }

    if (rc != -1)
    {
        rc |= bcm_plp_dsrds_firmware_lane_config_set(myMilleniObPtr, phyInfo, lnConf);

        if (rc != 0)
        {
            INFN_LOG(SeverityLevel::info) << "bcm_plp_dsrds_firmware_lane_config_set API failed, rc=" << rc << endl;
        }
    }

    return rc;
}


int Bcm81725::setRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rx_control)
{
    int rc = 0;


    /*  rx_control represents Rx lane control enum
          bcmpmRxReset            - Rx Datapath reset
          bcmpmRxSquelchOn        - Rx Squelch on
          bcmpmRxSquelchOff       - Rx Squelch off
          bcmpmRxCount            - Last enum
     */
    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;


    //printf("bcm_plp_rx_lane_control_set %s mask %x value %d\n", side == BCM_LINE_SIDE ? "PORT":"HOST",mask,rx_control);
    rc = bcm_plp_rx_lane_control_set(myMilleniObPtr, phyInfo, rx_control);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_rx_lane_control_set failed with rc=" << rc;
    }
    else
    {
        //getRxLaneControl(side, mask); --- BRCM SAYS THIS ALWAYS RETURNS ERROR
    }
    return rc;
}

int Bcm81725::getRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t& rx_control)
{
    int rc = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;

    bcm_plp_pm_phy_rx_lane_control_t rxControlGet;
    rc = bcm_plp_rx_lane_control_get(myMilleniObPtr, phyInfo, &rxControlGet);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_rx_lane_control_get failed with rc=" << rc;
    }
    else
    {
        rx_control = rxControlGet;
    }
    return rc;
}

int Bcm81725::setTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t tx_control)
{
    int rc = 0;


    /*  rx_control represents Tx lane control enum
          bcmpmTxReset            - Tx Datapath reset
          bcmpmTxSquelchOn        - Tx Squelch on
          bcmpmTxSquelchOff       - Tx Squelch off
          bcmpmTxCount            - Last enum
     */
    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;



    rc = bcm_plp_tx_lane_control_set(myMilleniObPtr, phyInfo, tx_control);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_tx_lane_control_set failed with rc=" << rc;
    }
    else
    {
        //getRxLaneControl(side, mask); --- BRCM SAYS THIS ALWAYS RETURNS ERROR
    }
    return rc;
}

int Bcm81725::getTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t& tx_control)
{
    int rc = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side       = param.side;
    phyInfo.lane_map      = param.laneNum;

    bcm_plp_pm_phy_tx_lane_control_t txControlGet;
    rc = bcm_plp_tx_lane_control_get(myMilleniObPtr, phyInfo, &txControlGet);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_tx_lane_control_get failed with rc=" << rc;
    }
    else
    {
        tx_control = txControlGet;
    }
    return rc;
}

int Bcm81725::getEyeScan(const Bcm81725Lane& param)
{
    int rc = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));

    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    rc = bcm_plp_display_eye_scan(myMilleniObPtr, phyInfo);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_display_eye_scan failed with rc=" << rc;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_display_eye_scan successful";
    }
    fflush(stdout);
     return rc;
}


int Bcm81725::getEyeProjection(const Bcm81725Lane& param, unsigned int lineRate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr, string &errMsg)
{
    int rc = 0;


    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side  = param.side;
    phyInfo.lane_map = param.laneNum;


    if (maxErr >  8)    maxErr = 8;
    if (maxErr == 0)    maxErr = 1;




    unsigned int status = 0;
    bcm_plp_link_status_get(myMilleniObPtr, phyInfo, &status);
    fflush(stdout);
    if (status == 0)
    {
        INFN_LOG(SeverityLevel::info) << "EYE Margin Projection PRBS not detected for lane: " << phyInfo.lane_map << endl;
        errMsg = "EYE Margin Projection PRBS not detected for lane: " + std::to_string(phyInfo.lane_map);
        return -1;
    }


    cout << "---------------- Get Eye Margin Projection for lane: %d ----------------" << endl;
/*
    bcm_plp_mode_config_get(myMilleniObPtr, phyInfo, &speed, &if_type, &ref_clk, &if_mode, (void*)&milb_aux_mode);
    cout << "bcm_plp_mod_config_get speed    : " <<  speed << endl;
    cout << "bcm_plp_mod_config_get if_type  : " << if_type << endl;
    cout << "bcm_plp_mod_config_get if_mode  : " << if_mode << endl;
    cout << "bcm_plp_mod_config_get ref_clk  : " <<  ref_clk << endl;
    cout << "bcm_plp_mod_config_get lane_rate: " <<  milb_aux_mode.lane_data_rate << endl;
    cout << "bcm_plp_mod_config_get modu_mode: " <<  milb_aux_mode.modulation_mode << endl;
    cout << "bcm_plp_mod_config_get fec_mode : " <<  milb_aux_mode.fec_mode_sel << endl;
    cout << "bcm_plp_mod_config_get mux_mode : " <<  milb_aux_mode.hitless_mux_mode << endl;
    cout << "bcm_plp_mod_config_get hyb_mode : " << milb_aux_mode.hybrid_port_mode << endl;
*/
    uint8_t mode;
    uint8_t merr = maxErr;
    int  rate = lineRate;


    rc |= bcm_plp_eye_margin_proj(myMilleniObPtr, phyInfo, (int)lineRate, (uint8_t)scanMode, (uint8_t)timerControl, merr);

    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_eye_margin_proj failed with rc:" <<  rc;
    }

    return rc;
}


int Bcm81725::chipStatus(const Bcm81725Lane& param, unsigned int func)
{
    int rc = 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side  = param.side;
    phyInfo.lane_map = param.laneNum;


    phyInfo.flags = func;

    rc = bcm_plp_phy_status_dump(myMilleniObPtr, phyInfo);
    if (rc != 0) {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_phy_status_dump failed with rc:" << rc;
    }
    fflush(stdout);
    return rc;
}

int Bcm81725::clrInterrupt(const Bcm81725Lane& param)
{
    int rc = 0;

    unsigned int type = 0xFFFFFF;
    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side  = param.side;
    phyInfo.lane_map = param.laneNum;

    rc |= bcm_plp_intr_status_clear(myMilleniObPtr, phyInfo, type);
    if (rc != 0) {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_intr_status_clear failed with rc: " << rc;
    }
    return rc;
}

int Bcm81725::getInterrupt(const Bcm81725Lane& param, unsigned int type, unsigned int *status)
{
    int rc = 0;
    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side  = param.side;
    phyInfo.lane_map = param.laneNum;

    rc |= bcm_plp_intr_status_get(myMilleniObPtr, phyInfo, type, status);
    if (rc != 0) {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_intr_status_get failed with rc: " << rc;
    }
    return rc;
}


int Bcm81725::setInterrupt(const Bcm81725Lane& param, unsigned int typeMask, bool enable)
{
    int rc = 0;
    unsigned int en = enable ? 1 : 0;

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side  = param.side;
    phyInfo.lane_map = param.laneNum;

    rc |= bcm_plp_intr_enable_set(myMilleniObPtr, phyInfo, typeMask, en);
    if (rc != 0) {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_intr_enable_set failed with rc: " << rc;
    }
    return rc;
}

int Bcm81725::getPcsStatus(const Bcm81725Lane& param, bcm_plp_pcs_status_t& st)
{
    int rc = 0;



    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side  = param.side;
    phyInfo.lane_map = param.laneNum;



    rc = bcm_plp_pcs_status_get(myMilleniObPtr, phyInfo, &st);

    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_pcs_status_get %s failed with rc=" << rc << " " << (param.side == BCM_LINE_SIDE ? "LINE":"HOST");
		memset(&st, 0, sizeof(bcm_plp_pcs_status_t));
    }


    return rc;
}


int Bcm81725::getFecStatus(const Bcm81725Lane& param, bool clear, bcm_plp_fec_status_t& st)
{
    int rc = 0;


    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    phyInfo.platform_ctxt = (void*)(&param.bus);

    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.if_side  = param.side;
    phyInfo.lane_map = param.laneNum;



    //phyInfo.lane_map = 0xff;

    st.fec_status_clear = clear != 0 ? 1 : 0;
    rc = bcm_plp_fec_status(myMilleniObPtr, phyInfo, &st);


    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << "bcm_plp_fec_status failed with rc=" << rc << " " << (param.side == BCM_LINE_SIDE ? "LINE":"HOST");
    }

    return rc;
}

int Bcm81725::iWrite(const Bcm81725Lane& param, uint32 regAddr, uint16 data)
{
    int rc = 0;
    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(phyInfo));
    phyInfo.platform_ctxt = (void*)(&param.bus);


    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side = 0;
    phyInfo.lane_map = 0xf;
    unsigned int dev = 1; // 1 - PMA/PMD, 3 - PCS, 7 - AN ... what are these?


    rc = bcm_plp_reg_value_set(myMilleniObPtr, phyInfo, dev, regAddr, (unsigned int)data);

    if (rc == 0)
    {
        INFN_LOG(SeverityLevel::info) << "Wrote Dev=" << dev << " regAddr=" << regAddr << " data=" << data << " successful." << endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Wrote Dev=" << dev << " regAddr=" << regAddr << " data=" << data << " unsuccessful!" << endl;
    }


    return rc;
}

int Bcm81725::iRead(const Bcm81725Lane& param, uint32 regAddr, uint16& data)
{
    int rc = 0;
    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(phyInfo));
    phyInfo.platform_ctxt = (void*)(&param.bus);


    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side = 0;
    phyInfo.lane_map = 0xf;
    unsigned int dev = 1; // 1 - PMA/PMD, 3 - PCS, 7 - AN ... what are these?

    unsigned int ldata = 0;

    rc = bcm_plp_reg_value_get(myMilleniObPtr, phyInfo, dev, regAddr, &ldata);

    data = ldata;

    if (rc == 0)
    {
        INFN_LOG(SeverityLevel::info) << "Read Dev=" << dev << " regAddr=" << regAddr << " data=" << data << " successful." << endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Read Dev=" << dev << " regAddr=" << regAddr << " data=" << data << " unsuccessful!" << endl;
    }


    return rc;
}

int Bcm81725::getPam4Diag(const Bcm81725Lane& param, bcm_plp_pm_phy_pam4_diagnostics_t *pam4)
{
    int rc = 0;
    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(phyInfo));
    phyInfo.platform_ctxt = (void*)(&param.bus);


    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }


    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;

    rc = bcm_plp_pam4_diag_get(myMilleniObPtr, phyInfo, pam4);

    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::info) << "bcm_plp_phy_pam4_diag_get failed with rc: " << rc <<  (param.side == BCM_LINE_SIDE ? " LINE":" HOST");
    }
    return 0;
}

int Bcm81725::setFailOverPrbs(const Bcm81725Lane& param, unsigned int mode)
{
    /*
     * Setup failover provisioning to PRBS
     */

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.platform_ctxt = (void*)(&param.bus);
    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;


    bcm_plp_pm_firmware_lane_config_t lnConf;
    memset(&lnConf, 0, sizeof(bcm_plp_pm_firmware_lane_config_t));


    int rc = bcm_plp_firmware_lane_config_get(myMilleniObPtr, phyInfo, &lnConf);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << " bcm_plp_dsrds_firmware_lane_config_get failed!";
        return -1;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << " bcm_plp_dsrds_firmware_lane_config_get=" << lnConf.TxSquelchOrPrbs;
        lnConf.TxSquelchOrPrbs = mode;
        rc = bcm_plp_firmware_lane_config_set(myMilleniObPtr, phyInfo, &lnConf);

        if (rc == 0)
        {
            INFN_LOG(SeverityLevel::info) << " bcm_plp_dsrds_firmware_lane_config_set=" << mode;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << " bcm_plp_dsrds_firmware_lane_config_set failed";
        }
    }
    return rc;
}

int Bcm81725::getFailOverPrbs(const Bcm81725Lane& param, unsigned int &mode)
{
    /*
     * Setup failover provisioning to PRBS
     */

    bcm_plp_access_t phyInfo;
    memset(&phyInfo, 0, sizeof(bcm_plp_access_t));


    if (param.bus == cTopMz)
    {
        phyInfo.phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
    }
    else
    {
        phyInfo.phy_addr = param.mdioAddr;
    }

    phyInfo.platform_ctxt = (void*)(&param.bus);
    phyInfo.if_side = param.side;
    phyInfo.lane_map = param.laneNum;


    bcm_plp_pm_firmware_lane_config_t lnConf;
    memset(&lnConf, 0, sizeof(bcm_plp_pm_firmware_lane_config_t));


    int rc = bcm_plp_firmware_lane_config_get(myMilleniObPtr, phyInfo, &lnConf);
    if (rc != 0)
    {
        INFN_LOG(SeverityLevel::error) << " bcm_plp_dsrds_firmware_lane_config_get failed!";
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << " bcm_plp_dsrds_firmware_lane_config_get=" << lnConf.TxSquelchOrPrbs;
        mode = lnConf.TxSquelchOrPrbs;
    }

    return rc;
}


}
