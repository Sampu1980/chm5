#include "GearBoxAdapter.h"
#include "GearBoxUtils.h"
#include "Bcm81725_avs_util.h"
#include "InfnLogger.h"
#include <sstream>
#include "ZSys.h"
#include <iomanip>
#include "BcmFaults.h"
#include<ctime>
#include<cstdio>
#include <stdlib.h>
#include <iomanip>
#include <iostream>

#define PORT_TO_INDEX(port) (port-1)

namespace gearbox {


GearBoxAdapter::GearBoxAdapter()
{

    myDriverPtr = std::make_unique<Bcm81725>();


    Bcm81725Lane paramHostTop4 = {1, 0x4, cHostSide, 0xffff};
    Bcm81725Lane paramHostTop8 = {1, 0x8, cHostSide, 0xffff};
    Bcm81725Lane paramHostTopC = {1, 0xc, cHostSide, 0xffff};
    Bcm81725Lane paramHostBot4 = {0, 0x4, cHostSide, 0xffff};
    Bcm81725Lane paramHostBot8 = {0, 0x8, cHostSide, 0xffff};
    Bcm81725Lane paramHostBotC = {0, 0xc, cHostSide, 0xffff};

    Bcm81725Lane paramLineTop4 = {1, 0x4, cLineSide, 0xffff};
    Bcm81725Lane paramLineTop8 = {1, 0x8, cLineSide, 0xffff};
    Bcm81725Lane paramLineTopC = {1, 0xc, cLineSide, 0xffff};
    Bcm81725Lane paramLineBot4 = {0, 0x4, cLineSide, 0xffff};
    Bcm81725Lane paramLineBot8 = {0, 0x8, cLineSide, 0xffff};
    Bcm81725Lane paramLineBotC = {0, 0xc, cLineSide, 0xffff};


    Bcm81725Lane param[] = {paramHostTop4, paramHostTop8, paramHostTopC,
                            paramHostBot4, paramHostBot8, paramHostBotC,
                            paramLineTop4, paramLineTop8, paramLineTopC,
                            paramLineBot4, paramLineBot8, paramLineBotC};

    for (unsigned int i = 0; i < sizeof(param) / sizeof(Bcm81725Lane); i++)
    {
        warmInit(param[i]);
    }

    memset(&mBcmFltPmTime, 0, sizeof(mBcmFltPmTime));
}

int GearBoxAdapter::GetBcmInfo(BcmVers & bcmInfoTopMz, BcmVers & bcmInfoBottomMz)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    int rcTop = 0, rcBottom = 0;
    unsigned int fwVersion = 0;
    unsigned int fwCrc     = 0;
    std::vector<PortConfig> portConfig;


    GetPortConfigEntry(portConfig, Ports_t::PORT3, RATE_100GE);
    Bcm81725Lane param = {portConfig[0].bus, portConfig[0].bcmUnit, cHostSide, portConfig[0].host.laneMap};



    rcTop |= myDriverPtr->GetBcmInfo(param, bcmInfoTopMz.fwVer, bcmInfoTopMz.fwCrc);

    if (rcTop != 0)
    {
        INFN_LOG(SeverityLevel::info) << "GetBcmInfo failed to get top mz version! rc=" << rcTop << endl;
    }

    INFN_LOG(SeverityLevel::info) << "GetBcmInfo top fwVersion=" << std::hex << fwVersion << " top crc=" << fwCrc << std::dec << endl;

    portConfig.clear();
    GetPortConfigEntry(portConfig, Ports_t::PORT11, RATE_100GE);
    param = {portConfig[0].bus, portConfig[0].bcmUnit, cHostSide, portConfig[0].host.laneMap};



    rcBottom |= myDriverPtr->GetBcmInfo(param, bcmInfoBottomMz.fwVer, bcmInfoBottomMz.fwCrc);

    if (rcBottom != 0)
    {
        INFN_LOG(SeverityLevel::info) << "GetBcmInfo failed to get bottom mz version! rc=" << rcBottom << endl;
    }

    INFN_LOG(SeverityLevel::info) << "GetBcmInfo bottom fwVersion="  << std::hex << fwVersion << " bottom crc=" << fwCrc <<  std::dec << endl;
    return (rcTop | rcBottom);
}

int GearBoxAdapter::UpdateUpgradableDeviceList(chm6_common::Chm6DcoCardState &stateObj)
{
    BcmVers bcmInfoTopMz, bcmInfoBottomMz;
    memset(&bcmInfoTopMz, 0, sizeof(bcmInfoTopMz));
    memset(&bcmInfoBottomMz, 0, sizeof(bcmInfoBottomMz));
    string swVers = "0xd022"; // Default to d022, if there is SDK mismatch with BCM FW, we might not be able to read BCM FW. Default to last version. Must be hard coded.
    std::stringstream ss;


    int rc = GetBcmInfo(bcmInfoTopMz, bcmInfoBottomMz);

    if (rc == 0)
    {
        ss << std::hex << bcmInfoTopMz.fwVer << std::dec;
        swVers = "0x" + ss.str();
    }


    hal_common::UpgradableDeviceType* chmDev = (stateObj.mutable_upgradable_devices());
    auto chmDevMap = (chmDev->mutable_upgradable_devices());

    hal_common::UpgradableDeviceType::UpgradableDevice device;
    (device.mutable_device_name())->set_value("Retimer");
    (device.mutable_hw_version())->set_value( "hw version" );
    (device.mutable_sw_version())->set_value( swVers );
    (device.mutable_fw_update_state())->set_value( "unknown FW update state" );
    //(device.mutable_file_location())->set_value( ((iter.second).file_location()).value() );
    //device.mutable_fw_update_state(iter->second().device_name());
    (*chmDevMap)["Retimer"] = device;

    return 0; // always return ok since we will default to hard coded value if BCM get version fails
}



int GearBoxAdapter::loadFirmware(unsigned int bus, bool force)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->loadFirmware(bus);

}



int GearBoxAdapter::setPortRate(const string& aId, Bcm81725DataRate_t rate, bool fecEnable, bcm_plp_fec_mode_sel_t fecType)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    unsigned int port = aid2PortNum(aId);
    if (checkValidPort(port) == false)
    {
        return -1;
    }

    int newSpeed = rateEnum2Speed(rate);

    // SetPortRate speed if different than speed configured on BCM
    INFN_LOG(SeverityLevel::info) << "SetPortRate new speed=" << newSpeed  << " fecType=" << fecType <<  " aid=" << aId << endl;
    // update port rate

    int rcHost = 0;
    int rcLine = 0;
    int diffHost = 0;
    int diffLine = 0;

    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);
    Bcm81725Lane paramLine;                 // BCM params and lane configuration for host
    Bcm81725Lane paramHost;                 // BCM params and lane configuration for line
    ModeParam    modeParamHost;             // New mode configuration for host
    ModeParam    modeParamLine;             // New mode configuration for line
    ModeParam    modeParamGetHost;          // Retrieved mode configuration for host
    ModeParam    modeParamGetLine;          // Retrieved mode configuration for line
    bcm_plp_fec_mode_sel_t lineFecType;

    // If we don't clear getMode params (refClk), may cause BCM SDK to reject
    memset(&modeParamGetHost, 0, sizeof(modeParamGetHost));
    memset(&modeParamGetLine, 0, sizeof(modeParamGetLine));
    int interfaceMode = bcmplpInterfaceModeIEEE;
    switch(rate){
        //TODO: RATE_OTU4 needs a fix
        //For OTU4, actual interface mode needs to be OTN, but read back value seems to be getting as IEEE mode,
        //which is an issue. Until issue gets fixed, OTU4 is set to IEEE mode, for avoiding potential traffic hit.
        case RATE_OTU4:
            //interfaceMode = bcmplpInterfaceModeOTN;
            interfaceMode = bcmplpInterfaceModeIEEE;
            break;
        default:
            interfaceMode = bcmplpInterfaceModeIEEE;
            break;
    }

    std::vector<ModeParam> lineSide;


    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        // Host
        // Init bcm params for Host side
        paramHost         = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};

        // Init param setMode config for Host side
        modeParamHost     = {newSpeed, bcm_pm_InterfaceKR, bcm_pm_RefClk156Mhz, interfaceMode,
                         portConfig[i].host.dataRate, portConfig[i].host.modulation, portConfig[i].host.fecType};

        // Init bcm param for Line
        paramLine         = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};

        // Init param setMode config for Line side
        // Determine fec mode and type
        if (isLineSideFecEnable(port, rate, portConfig[i].bcmUnit, cLineSide, fecEnable) == true)
        {
            lineFecType = fecType;
        }
        else
        {
            lineFecType = (bcm_plp_fec_mode_sel_t)portConfig[i].line.fecType;
        }

        modeParamLine = {newSpeed, bcm_pm_InterfaceKR, bcm_pm_RefClk156Mhz, interfaceMode,
                        portConfig[i].line.dataRate, portConfig[i].line.modulation, (unsigned int)lineFecType};

        if (isOuterLineSide(port, rate, portConfig[i].bcmUnit, cLineSide) == true)
        {
            lineSide.push_back(modeParamLine);
            INFN_LOG(SeverityLevel::info) << "SetPortRate port=" << port << " speed=" << lineSide[i].speed << " fec=" << lineSide[i].fecType;
        }

        // Print host param and configs to logs
        INFN_LOG(SeverityLevel::info) << "SetPortRate host port=" << portConfig[i].portNum  << " bus=" << portConfig[i].bus
                          << " bcm=" << portConfig[i].bcmUnit << " lane=" << portConfig[i].host.laneMap << endl;

        INFN_LOG(SeverityLevel::info) << "host newSpeed=" << newSpeed << " ifType=" << modeParamHost.ifType << " refClk=" << modeParamHost.refClk <<
                " intFaceMode=" << modeParamHost.interfaceMode << " dataRate=" << modeParamHost.dataRate << " mod=" << modeParamHost.modulation <<
                " fecType=" << modeParamHost.fecType << endl;

        // Print line param and configs to logs
        INFN_LOG(SeverityLevel::info) << "SetPortRate line port=" << portConfig[i].portNum  << " bus=" << portConfig[i].bus
                            << " bcm=" << portConfig[i].bcmUnit << " lane=" << portConfig[i].line.laneMap << endl;

        INFN_LOG(SeverityLevel::info) << "line newSpeed=" << newSpeed << " ifType=" << modeParamLine.ifType << " refClk=" << modeParamLine.refClk <<
                " intFaceMode=" << modeParamLine.interfaceMode << " dataRate=" << modeParamLine.dataRate << " mod=" << modeParamLine.modulation <<
                " fecType=" << modeParamLine.fecType << endl;

        INFN_LOG(SeverityLevel::info) << "Setting FEC Type=" << lineFecType << " " << printFecType((bcm_plp_fec_mode_sel_t)fecType) << endl;

        // Retrieve from BCM the modes (getMode)

        // Get mode on host
        //warmInit(paramHost);
        rcHost = getMode(paramHost, modeParamGetHost);

        // Get mode on line
        //warmInit(paramLine);
        rcLine = getMode(paramLine, modeParamGetLine);

        // Compare mode read with new configuration
        // If rcHost == 0 then port was configured, then compare new mode with old mode
        // else, then port is NOT configured, no need to compare
        if (rcHost == 0)
        {
            if (modeParamHost.dataRate      == modeParamGetHost.dataRate &&
                modeParamHost.fecType       == modeParamGetHost.fecType  &&
                modeParamHost.ifType        == modeParamGetHost.ifType   &&
                modeParamHost.interfaceMode == modeParamGetHost.interfaceMode &&
                modeParamHost.modulation    == modeParamGetHost.modulation &&
                modeParamHost.refClk        == modeParamGetHost.refClk &&
                modeParamHost.speed         == modeParamGetHost.speed)
            {
                diffHost = 0;
            }
            else
            {
                diffHost = -1;
            }

            //diffHost = memcmp(&modeParamHost, &modeParamGetHost, sizeof(modeParamHost));
        }
        if (rcLine == 0)
        {
            if (modeParamLine.dataRate      == modeParamGetLine.dataRate &&
                modeParamLine.fecType       == modeParamGetLine.fecType  &&
                modeParamLine.ifType        == modeParamGetLine.ifType   &&
                modeParamLine.interfaceMode == modeParamGetLine.interfaceMode &&
                modeParamLine.modulation    == modeParamGetLine.modulation  &&
                modeParamLine.refClk        == modeParamGetLine.refClk  &&
                modeParamLine.speed         == modeParamGetLine.speed)
            {
                diffLine = 0;
            }
            else
            {
                diffLine = -1;
            }
            //diffLine = memcmp(&modeParamLine, &modeParamGetLine, sizeof(modeParamLine));
        }

        INFN_LOG(SeverityLevel::info) << "DiffHost=" << diffHost << " rcHost=" << rcHost << endl;
        INFN_LOG(SeverityLevel::info) << "DiffLine=" << diffLine << " rcLine=" << rcLine << endl;

        // Always call both host and line together
        if (diffHost != 0 ||
            rcHost   != 0 ||
            diffLine != 0 ||
            rcLine   != 0)
        {

            if (diffHost != 0)
            {
                INFN_LOG(SeverityLevel::info) << "BCM getMode Host, oldSpeed=" << modeParamGetHost.speed    << " ifType="        << modeParamGetHost.ifType        <<
                                                 " refClk="                    << modeParamGetHost.refClk   << " interfaceMode=" << modeParamGetHost.interfaceMode <<
                                                 " dataRate="                  << modeParamGetHost.dataRate << " mod="           << modeParamGetHost.modulation <<
                                                 " fecType="                   << modeParamGetHost.fecType;
            }
            INFN_LOG(SeverityLevel::info) << " Call set mode host!" << endl;
            //warmInit(paramHost);
            rcHost = setMode(paramHost, modeParamHost);


            if (diffLine != 0)
            {
                INFN_LOG(SeverityLevel::info) << "BCM getMode Line, oldSpeed=" << modeParamGetLine.speed    << " ifType="        << modeParamGetLine.ifType        <<
                                                 " refClk="                    << modeParamGetLine.refClk   << " interfaceMode=" << modeParamGetLine.interfaceMode <<
                                                 " dataRate="                  << modeParamGetLine.dataRate << " mod="           << modeParamGetLine.modulation <<
                                                 " fecType="                   << modeParamGetLine.fecType;
            }
            INFN_LOG(SeverityLevel::info) << " Call set mode line!" << endl;
            //warmInit(paramLine);
            rcLine = setMode(paramLine, modeParamLine);
        }
    }

    auto elem = mModeParams.find(port);

    if (elem != mModeParams.end())
    {
        // Set mode called before, update line side params
        elem->second.clear();

        elem->second = lineSide;

        //memcpy(&elem->second, &lineSide, sizeof(elem->second));
        INFN_LOG(SeverityLevel::info) << " Update lineSide port=" << port << " size=" << elem->second.size() << " elemSize=" << elem->second.size() <<
        " speed=" << elem->second[0].speed << " fecType=" << elem->second[0].fecType << endl;


    }
    else
    {
        // First time, create new
        mModeParams.insert(make_pair(port, lineSide));
        INFN_LOG(SeverityLevel::info) << " Create new lineSide port=" << port << " size=" << lineSide.size() << endl;
        INFN_LOG(SeverityLevel::info) << " dataRate=" << lineSide[0].dataRate << " fec=" << lineSide[0].fecType;
    }

    if (portConfig.size() == 0)
    {
        INFN_LOG(SeverityLevel::info) << " SetDataRate port= " << port << " is invalid! Rate=" << rate << endl;
        return -1;
    }

    if (rcHost + rcLine != 0)
    {
        INFN_LOG(SeverityLevel::info) << " SetPortRate port= " << port << " Rate=" << rate << " calling setMode failed!" << endl;

        return -1;
    }

    return 0;
}



int GearBoxAdapter::setPortLoopback(const string& aId, Bcm81725Loopback_t loopback)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    unsigned int port = 0;
    unsigned int count = 0;
    unsigned int rc = 0;
    bool found = false;
    Bcm81725DataRate_t rate = RATE_100GE;
    // Example: aidStr = 1-4-T1 .. 1-4-T16
    // 1-1-T1..T16

    port = aid2PortNum(aId);
    if (checkValidPort(port) == false)
    {
        return -1;
    }


    //IS_VALID_PORT(port);
     //IS_VALID_DATARATE(rate);

     cout << "\nsetPortLoopback port: " << port << " lpbk: " << loopback << endl;

     PortConfig *config = NULL;


     if (getPortRate(port, rate) == -1)
     {
         INFN_LOG(SeverityLevel::info) << " SetLoopback failed, port=" << port << " is not configured, cannot determine param lanes." << endl;
         return -1;
     }


     if (rate == RATE_100GE)
     {
         config = port_config_100GE;
         count = c100GeCount;
     }
     else if (rate == RATE_400GE)
     {
         config = port_config_400GE;
         count = c400GeCount;
     }
     else if (rate == RATE_OTU4)
     {
         config = port_config_100GE;
         count = cOtu4Count;
     }
     else if (rate == RATE_4_100GE)
     {
         config = port_config_100GE;
         count = c4_100GeCount;
     }

     for (unsigned int i = 0; i < count; i++)
     {
         if (config[i].portNum == port)
         {
             int mode = 2; //remote PMD loopback
             int enable = 0;
             if (loopback == LOOPBACK_FAC)
             {
                 enable = 1;
             }
             //configure line side
             Bcm81725Lane param = {config[i].bus, config[i].bcmUnit, 0 /* host */, config[i].line.laneMap};
             //warmInit(param);
             rc += setLoopback(param, mode, enable);

             found = true;
             //break out as loopback supported on first BCM towards TOM
             break;
         }
     }

     if (rc!= 0 || found == false)
     {
         cout << " SetLoopback failed, rc=" << rc << " found=" << found << endl;
         INFN_LOG(SeverityLevel::info) << " SetLoopback failed, rc=" << rc << " found=" << found;
         rc = -1;
     }
     return rc;
}

int GearBoxAdapter::setPortFecEnable(const string& aId, bool enable)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    Bcm81725DataRate_t rate = RATE_100GE;


    int port = 0; // AF0 todo, need to implement KR4 FEC to be able to test 100GE SR4
    // Example: aidStr = 1-4-T1 .. 1-4-T16
    // 1-1-T1..T16
    int rc = 0;

    port = aid2PortNum(aId);

    if (checkValidPort(port) == false)
    {
        return -1;
    }

    rc = getCachedPortRate(port, rate);

#if 0
    // Should I block this setting if port is not configured?
    // 400GE is always no FEC setting from BCM.
    if (getPortRate(port, rate) == -1)
    {
        INFN_LOG(SeverityLevel::info) << " SetPortFecEnable failed, port=" << port << " is not configured, cannot determine param lanes." << endl;
        return -1;
    }
#endif

    if ( rc == 0)
    {
        if (rate == RATE_100GE)
        {
            rc = setPortRate(aId, rate, enable);
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << " Rate is invalid or gearbox does not handle 400Ge/4x100Gbe Fec handling, " << " rate=" << rate << endl;
            rc = -1;
        }
    }

    return rc;
}


int GearBoxAdapter::getPolarity(const Bcm81725Lane& param, unsigned int& tx, unsigned int& rx)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->getPolarity(param, tx, rx);
}

int GearBoxAdapter::setPolarity(const Bcm81725Lane& param, unsigned int tx, unsigned int rx)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->setPolarity(param, tx, rx);
}

int GearBoxAdapter::warmInit(const Bcm81725Lane& param)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->warmInit(param);
}

int GearBoxAdapter::setLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int enable)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->setLoopback(param, mode, enable);
}

int GearBoxAdapter::getLoopback(const Bcm81725Lane& param, unsigned int mode, unsigned int &enable)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->getLoopback(param, mode, enable);
}

int GearBoxAdapter::setAvsConfig(const Bcm81725Lane& param, unsigned int enable, unsigned int margin)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->setAvsConfig(param, enable, margin);
}

int GearBoxAdapter::getAvsConfig(const Bcm81725Lane& param, unsigned int& enable, unsigned int& disableType,
                     unsigned int& ctrl, unsigned int& regulator, unsigned int& pkgShare, unsigned int& margin)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->getAvsConfig(param, enable, disableType, ctrl, regulator, pkgShare, margin);
}

int GearBoxAdapter::getAvsStatus(const Bcm81725Lane& param, unsigned int& status)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->getAvsStatus(param, status);
}

int GearBoxAdapter::getMode(const Bcm81725Lane& param, ModeParam &modeParam)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->getMode(param, modeParam.speed, modeParam.ifType, modeParam.refClk, modeParam.interfaceMode,
                                modeParam.dataRate, modeParam.modulation, modeParam.fecType);
}

int GearBoxAdapter::setMode(const Bcm81725Lane& param, const ModeParam &modeParam)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->setMode(param, modeParam.speed, modeParam.ifType, modeParam.refClk, modeParam.interfaceMode,
                                modeParam.dataRate, modeParam.modulation, modeParam.fecType);
}



int GearBoxAdapter::dumpBcmConfig(Ports_t port, std::string &dump)
{
    return 0;
}

int GearBoxAdapter::prbsLinkStatusCheck(const Bcm81725Lane& param, unsigned int& status)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->prbsLinkStatusCheck(param, status);
}

int GearBoxAdapter::prbsStatusCheck(const Bcm81725Lane& param, unsigned int timeVal, unsigned int& status)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->prbsStatusCheck(param, timeVal, status);
}

int GearBoxAdapter::setPrbsGen(const Bcm81725Lane& param, unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->setPrbsGen(param, tx, poly, inv, lb, enaDis);
}

int GearBoxAdapter::setPrbsCheck(const Bcm81725Lane& param, unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb, unsigned int enaDis)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->setPrbsCheck(param, tx, poly, inv, lb, enaDis);
}

int GearBoxAdapter::getLaneConfigDs(const Bcm81725Lane& param, bcm_plp_dsrds_firmware_lane_config_t &lnConf)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->getLaneConfigDS(param, &lnConf);
}

int GearBoxAdapter::setLaneConfigDs(const Bcm81725Lane& param, unsigned int fwLaneConfig, int64_t value)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->setLaneConfigDS(param, fwLaneConfig, value);
}

int GearBoxAdapter::setRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rx_control)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->setRxLaneControl(param, rx_control);
}



int GearBoxAdapter::getRxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t& rx_control)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    return myDriverPtr->getRxLaneControl(param, rx_control);
}


int GearBoxAdapter::setTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t tx_control)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);
    return myDriverPtr->setTxLaneControl(param, tx_control);

}

int GearBoxAdapter::getTxLaneControl(const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t& tx_control)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->getTxLaneControl(param, tx_control);
}


int GearBoxAdapter::getTxFir(const Bcm81725Lane& param, int& pre2, int& pre, int& main, int& post, int& post2, int& post3)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    bcm_plp_pam4_tx_s fir;
    memset(&fir, 0, sizeof(fir));
    fir.serdes_tx_tap_mode = (bcm_plp_serdes_tx_tap_mode_t)bcmplpTapModePAM4_6TAP;
    int rc = myDriverPtr->getFir(param, &fir);

    if (rc == 0)
    {
        pre2 = fir.pre2;
        pre = fir.pre;
        main = fir.main;
        post = fir.post;
        post2 = fir.post2;
        post3 = fir.post3;
    }

    return rc;
}



int GearBoxAdapter::setTxFir(const Bcm81725Lane& param, int pre2, int pre, int main, int post, int post2, int post3)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    bcm_plp_pam4_tx_s fir;

    int getPre2, getPre, getMain, getPost, getPost2, getPost3;
    memset(&fir, 0, sizeof(fir));



    INFN_LOG(SeverityLevel::info) << "bus=" << param.bus << " addr=" << param.mdioAddr << " side=" << param.side << " laneNum=" << param.laneNum << endl;

    fir.serdes_tx_tap_mode = (bcm_plp_serdes_tx_tap_mode_t)bcmplpTapModePAM4_6TAP;
    fir.pre2 = pre2;
    fir.pre = pre;
    fir.main = main;
    fir.post = post;
    fir.post2 = post2;
    fir.post3 = post3;


    return myDriverPtr->setFir(param, &fir);
}

int GearBoxAdapter::Write(const Bcm81725Lane& param, unsigned int regAddr, unsigned int regData)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->iWrite(param, (uint32)regAddr, (uint16)regData);
}

int GearBoxAdapter::Read(const Bcm81725Lane& param, unsigned int regAddr, unsigned int& regData)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    uint16 rData = 0;
    int rc = 0;

    rc = myDriverPtr->iRead(param, (uint32)regAddr, rData);

    regData = (unsigned int)rData;

    return rc;
}



int GearBoxAdapter::getPortRate(unsigned int port, Bcm81725DataRate_t &rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);
    Bcm81725Lane param;
    ModeParam    modeParamGet;

    // if modeParamGet.refClk != 0, getMode will fail
    memset(&modeParamGet, 0, sizeof(modeParamGet));

    // If portConfig.size() > 0, then we found entry in table.
    // We only need first table entry for port to find rate/speed.
    if (portConfig.size() > 0)
    {
        param = {portConfig[0].bus, portConfig[0].bcmUnit, cLineSide, portConfig[0].line.laneMap};
        rc = getMode(param, modeParamGet);

        if (rc == 0)
        {
            rate = speed2RateEnum(modeParamGet.speed, modeParamGet.dataRate, modeParamGet.fecType);
            INFN_LOG(SeverityLevel::info) << "Get rate=" << rate << " port=" << port << endl;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "Cannot get rate, port=" << port << " is not configured." << endl;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Bad port=" << port << ", cannot find in table." << endl;
        return -1;
    }


    return rc;
}

int GearBoxAdapter::getEyeScan(const Bcm81725Lane& param)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->getEyeScan(param);
}


int GearBoxAdapter::getEyeProjection(const Bcm81725Lane& param, unsigned int lineRate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr, std::string &errMsg)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->getEyeProjection(param, lineRate, scanMode, timerControl, maxErr, errMsg);
}

int GearBoxAdapter::getEyeScan(unsigned int port, Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);

    Bcm81725Lane param, paramMask;

    INFN_LOG(SeverityLevel::info) << "Check cout logs for eye scan output." << endl;


    // eye scan only prints to cout messages
    cout << "Printing eye scan for port=" << port << " rate=" << (int)rate << endl;

    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};
        //warmInit(param);


        cout << std::hex << "====Eye scan for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " host side " <<  "laneMap=0x"  << portConfig[i].host.laneMap << "====" << endl;

        for (uint16 laneMask = 0; laneMask < (sizeof(uint16) * 8 /*num bits*/); laneMask++)
        {
            uint16 bitMask = (1 << laneMask);

            if (portConfig[i].host.laneMap & bitMask)
            {
                cout << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                paramMask = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, bitMask};

                rc = getEyeScan(paramMask);

                if (rc != 0)
                {
                    INFN_LOG(SeverityLevel::info) << "getEyeScan host rc=" << rc;
                    break;
                }
            }
        }


        param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};
        //warmInit(param);

        cout << std::hex << "====Eye scan for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " line side " <<  "laneMap=0x"  << portConfig[i].line.laneMap << "====" << endl;

        for (uint16 laneMask = 0; laneMask < (sizeof(uint16) * 8 /*num bits*/); laneMask++)
        {
            uint16 bitMask = (1 << laneMask);

            if (portConfig[i].line.laneMap & bitMask)
            {
                cout << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                paramMask = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, bitMask};

                rc = getEyeScan(paramMask);

                if (rc != 0)
                {
                    INFN_LOG(SeverityLevel::info) << "getEyeScan line rc=" << rc;
                    break;
                }
            }
        }

    }

    if (portConfig.size() == 0)
    {
        rc = -1;
    }
    return rc;
}

int GearBoxAdapter::getEyeProjection(unsigned int port, Bcm81725DataRate_t rate, unsigned int scanMode,
                                     unsigned int timerControl, unsigned int maxErr, std::string &errMsg)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);

    Bcm81725Lane param, paramMask;

    INFN_LOG(SeverityLevel::info) << "Check cout logs for eye scan output." << endl;


    // eye scan only prints to cout messages
    cout << "Printing eye scan for port=" << port << " rate=" << (int)rate << endl;

    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};
        //warmInit(param);

        ModeParam    modeParamGet;
        memset(&modeParamGet, 0, sizeof(modeParamGet));

        rc = getMode(param, modeParamGet);

        if (rc != 0)
        {
            errMsg = "Can not getMode for getEyeProjection.\n";
            return -1;
        }

        cout << std::hex << "====Eye scan for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " host side " <<  "laneMap=0x"  << portConfig[i].host.laneMap << "====" << std::dec << endl;
        INFN_LOG(SeverityLevel::info) << std::hex << "====Eye scan for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " host side " <<  "laneMap=0x"  << portConfig[i].host.laneMap << "====" << std::dec << endl;

        for (uint16 laneMask = 0; laneMask < (sizeof(uint16) * 8 /*num bits*/); laneMask++)
        {
            uint16 bitMask = (1 << laneMask);

            if (portConfig[i].host.laneMap & bitMask)
            {
                cout << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                INFN_LOG(SeverityLevel::info) << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                paramMask = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, bitMask};

                rc = getEyeProjection(paramMask, modeParamGet.dataRate, scanMode, timerControl, maxErr, errMsg);

                if (rc != 0)
                {
                    INFN_LOG(SeverityLevel::info) << "getEyeProjection host rc=" << rc;
                    break;
                }
            }
        }


        param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};
        //warmInit(param);

        cout << std::hex << "====Eye scan for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " line side " <<  "laneMap=0x"  << portConfig[i].line.laneMap << "====" << std::dec << endl;
        INFN_LOG(SeverityLevel::info) << std::hex << "====Eye scan for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " line side " <<  "laneMap=0x"  << portConfig[i].line.laneMap << "====" << std::dec << endl;

        for (uint16 laneMask = 0; laneMask < (sizeof(uint16) * 8 /*num bits*/); laneMask++)
        {
            uint16 bitMask = (1 << laneMask);

            if (portConfig[i].line.laneMap & bitMask)
            {
                cout << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                INFN_LOG(SeverityLevel::info) << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                paramMask = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, bitMask};

                rc = getEyeProjection(paramMask, modeParamGet.dataRate, scanMode, timerControl, maxErr, errMsg);

                if (rc != 0)
                {
                    INFN_LOG(SeverityLevel::info) << "getEyeProjection line rc=" << rc;
                    break;
                }
            }
        }

    }

    if (portConfig.size() == 0)
    {
        rc = -1;
    }
    return rc;
}


int GearBoxAdapter::chipStatus(const Bcm81725Lane& param, unsigned int func)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->chipStatus(param, func);
}

int GearBoxAdapter::clrInterrupt(const Bcm81725Lane& param)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->clrInterrupt(param);
}


int GearBoxAdapter::getInterrupt(const Bcm81725Lane& param, unsigned int type, unsigned int *status)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->getInterrupt(param, type, status);
}



int GearBoxAdapter::setInterrupt(const Bcm81725Lane& param, unsigned int type, bool enable)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->setInterrupt(param, type, enable);
}



int GearBoxAdapter::getPcsStatus(const Bcm81725Lane& param, bcm_plp_pcs_status_t& st)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);
    memset(&st, 0, sizeof(st));
    return myDriverPtr->getPcsStatus(param, st);
}

int GearBoxAdapter::printPmCounts(unsigned int port, Bcm81725DataRate_t rate, unsigned int lane, std::string& counts, bool clear)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);
    ostringstream out;
    unsigned int numLanes = 0;
    int rc = 0;

    numLanes = GetPhysicalLanes(rate);

    if (lane == 0)
    {
        out << "Dumping sum of all lanes" << endl << endl;
        rc = printAllLanesPmCounts(port, counts, clear);
    }
    else if (lane <= numLanes)
    {
        out << "Dumping lane " << lane << endl << endl;
        lane--;
        counts = out.str();
        rc = printPmLaneCounts(port, lane, counts, clear);
    }
    return rc;
}

int GearBoxAdapter::printAllLanesPmCounts(unsigned int port, std::string& counts, bool clear)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    const unsigned int cWidth      = 6;
    const unsigned int cWidthColon = 3;
    int rc = 0;
    ostringstream out;


    // Get cached data
    auto it = mBcmPm.find(port);

    if (it != mBcmPm.end())
    {

        if (clear == true)
        {
            memset(&it->second.pcsAccum, 0, sizeof(it->second.pcsAccum));
            memset(&it->second.pcsLanesAccum, 0, sizeof(it->second.pcsLanesAccum));
            memset(&it->second.fecAccum, 0, sizeof(it->second.fecAccum));
            memset(&it->second.fecLanesAccum, 0, sizeof(it->second.fecLanesAccum));
            memset(&it->second.fecLanesPrev, 0, sizeof(it->second.fecLanesPrev));


            auto elem = mModeParams.find(port);
            if (it->second.fecMode == KR4FEC && elem != mModeParams.end())
            {
                std::vector<PortConfig> portConfig;
                Bcm81725Lane param;

                if (elem->second.size() == 0)
                {
                    out << "Error printing lanes, modeParam vector=0!" << endl << endl;
                    return -1;
                }
                unsigned int rate = speed2RateEnum(elem->second[0].speed, elem->second[0].dataRate, elem->second[0].fecType);
                GetPortConfigEntry(portConfig, port, rate);
                param = {portConfig[0].bus, portConfig[0].bcmUnit, cLineSide, portConfig[0].line.laneMap};
                bcm_plp_fec_status_t st;
                rc = getFecStatus(param, true, st);
            }
        }

        out << "===Sum of all physical lanes " << endl << endl;

        if (it->second.fecMode == PCSFEC)
        {
            out << "===PCS Mode Counters===" << endl;
            out << " Instant Counters" << endl;
            out << "  ICG Counts       " << " : " << setw(18) << it->second.pcs.icgCounts << endl;
            out << "  BIP8 Counts      " << " : " << setw(18) << it->second.pcs.bip8Counts << endl;

            for (unsigned int i = 0; i < cNumPcsLanes; i++)
            {
                out << "  BIP8 Error PCS Lane " << setw(cWidthColon) << i << " : " << setw(cWidth) << it->second.pcs.bip8CountsLane[i] << endl;
            }
            out << endl;
            out << " Accumulated Counters" << endl;
            out << "  ICG Counts " << " : " << setw(18) << it->second.pcsAccum.icgCounts << endl;
            out << "  BIP8 Counts" << " : " << setw(18) << it->second.pcsAccum.bip8Counts << endl;

            for (unsigned int i = 0; i < cNumPcsLanes; i++)
            {
                out << "  BIP8 Error PCS Lane " << setw(cWidthColon) << i << " : " << setw(cWidth) << it->second.pcsAccum.bip8CountsLane[i] << endl;
            }
        }
        else if (it->second.fecMode == KR4FEC)
        {
            out << "===FEC Mode Counters===" << endl;
            out << " Instant Counters" << endl;
            out << "  Rx uncorrected Words  " << " : " << setw(18) << it->second.fec.rxUnCorrWord << endl;
            out << "  Rx corrected Words    " << " : " << setw(18) << it->second.fec.rxCorrWord << endl;
            out << "  Rx corrected bits     " << " : " << setw(18) << it->second.fec.rxCorrBits << endl;
            out << "  Rx FEC Symbol Errors  " << " : " << setw(18) << it->second.fec.fecSymbolErr << endl;

            out << endl;
            out << " Accumulated Counters" << endl;
            out << "  Rx uncorrected Words  " << " : " << setw(18) << it->second.fecAccum.rxUnCorrWord << endl;
            out << "  Rx corrected Words    " << " : " << setw(18) << it->second.fecAccum.rxCorrWord << endl;
            out << "  Rx corrected bits     " << " : " << setw(18) << it->second.fecAccum.rxCorrBits << endl;
            out << "  Rx FEC Symbol Errors  " << " : " << setw(18) << it->second.fecAccum.fecSymbolErr << endl;
        }

        // Print delta time, convert to ms
        out << "  Time to retrieve last PCS status regs (ms) : " << setw(10) << (mBcmFltPmTime[PORT_TO_INDEX(port)].deltaTime / 1000000) << endl;
        out << "  Raw data - start (ns)                      : " << setw(10) << (mBcmFltPmTime[PORT_TO_INDEX(port)].start) << endl;
        out << "  Raw data - stop  (ns)                      : " << setw(10) << (mBcmFltPmTime[PORT_TO_INDEX(port)].stop) << endl;

    }
    else
    {
        out << "No cached PMs for port=" << port << endl;

        auto elem = mBcmPm.begin();

        for (; elem != mBcmPm.end(); elem++)
        {
            out << "Cached ports=" << elem->first << endl;
        }

        rc = -1;
    }

    counts = counts + out.str();
    return rc;

}

int GearBoxAdapter::printPmLaneCounts(unsigned int port, unsigned int lane, std::string& counts, bool clear)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    const unsigned int cWidth      = 6;
    const unsigned int cWidthColon = 3;
    int rc = 0;
    ostringstream out;


    // Get cached data
    auto it = mBcmPm.find(port);

    if (it != mBcmPm.end())
    {

        if (clear == true)
        {
            memset(&it->second.pcsLanesAccum[lane], 0, sizeof(it->second.pcsLanesAccum[lane]));
            memset(&it->second.fecLanesAccum[lane], 0, sizeof(it->second.fecLanesAccum[lane]));
            memset(&it->second.fecLanesPrev[lane], 0, sizeof(it->second.pcsLanesAccum[lane]));

            // Clear all lanes since BCM has no granularity to report with single physical lane
            auto elem = mModeParams.find(port);
            if (it->second.fecMode == KR4FEC && elem != mModeParams.end())
            {
                std::vector<PortConfig> portConfig;
                Bcm81725Lane param;

                unsigned int rate = speed2RateEnum(elem->second[0].speed, elem->second[0].dataRate, elem->second[0].fecType);
                GetPortConfigEntry(portConfig, port, rate);
                param = {portConfig[0].bus, portConfig[0].bcmUnit, cLineSide, portConfig[0].line.laneMap};
                bcm_plp_fec_status_t st;
                rc = getFecStatus(param, true, st);
            }
        }


        out << "===Physical Lane " << lane << "===" << endl << endl;
        if (it->second.fecMode == PCSFEC)
        {
            out << "===PCS Mode Counters===" << endl;
            out << " Instant Counters" << endl;
            out << "  ICG Counts       " << " : " << setw(18) << it->second.pcsLanes[lane].icgCounts << endl;
            out << "  BIP8 Counts      " << " : " << setw(18) << it->second.pcsLanes[lane].bip8Counts << endl;

            for (unsigned int i = 0; i < cNumPcsLanes; i++)
            {
                out << "  BIP8 Error PCS Lane " << setw(cWidthColon) << i << " : " << setw(cWidth) << it->second.pcsLanes[lane].bip8CountsLane[i] << endl;
            }
            out << endl;
            out << " Accumulated Counters" << endl;
            out << "  ICG Counts " << " : " << setw(18) << it->second.pcsLanesAccum[lane].icgCounts << endl;
            out << "  BIP8 Counts" << " : " << setw(18) << it->second.pcsLanesAccum[lane].bip8Counts << endl;

            for (unsigned int i = 0; i < cNumPcsLanes; i++)
            {
                out << "  BIP8 Error PCS Lane " << setw(cWidthColon) << i << " : " << setw(cWidth) << it->second.pcsLanesAccum[lane].bip8CountsLane[i] << endl;
            }
        }
        else if (it->second.fecMode == KR4FEC)
        {
            out << "===FEC Mode Counters===" << endl;
            out << " Instant Counters" << endl;
            out << "  Rx uncorrected Words  " << " : " << setw(18) << it->second.fecLanes[lane].rxUnCorrWord << endl;
            out << "  Rx corrected Words    " << " : " << setw(18) << it->second.fecLanes[lane].rxCorrWord << endl;
            out << "  Rx corrected bits     " << " : " << setw(18) << it->second.fecLanes[lane].rxCorrBits << endl;
            out << "  Rx FEC Symbol Errors  " << " : " << setw(18) << it->second.fecLanes[lane].fecSymbolErr << endl;

            out << endl;
            out << " Accumulated Counters" << endl;
            out << "  Rx uncorrected Words  " << " : " << setw(18) << it->second.fecLanesAccum[lane].rxUnCorrWord << endl;
            out << "  Rx corrected Words    " << " : " << setw(18) << it->second.fecLanesAccum[lane].rxCorrWord << endl;
            out << "  Rx corrected bits     " << " : " << setw(18) << it->second.fecLanesAccum[lane].rxCorrBits << endl;
            out << "  Rx FEC Symbol Errors  " << " : " << setw(18) << it->second.fecLanesAccum[lane].fecSymbolErr << endl;
        }

        // Print delta time, convert to ms
        out << "  Time to retrieve last PCS status regs (ms) : " << setw(10) << (mBcmFltPmTime[PORT_TO_INDEX(port)].deltaTime / 1000000) << endl;
        out << "  Raw data - start (ns)                      : " << setw(10) << (mBcmFltPmTime[PORT_TO_INDEX(port)].start) << endl;
        out << "  Raw data - stop  (ns)                      : " << setw(10) << (mBcmFltPmTime[PORT_TO_INDEX(port)].stop) << endl;

    }
    else
    {
        out << "No cached PMs for port=" << port << endl;

        auto elem = mBcmPm.begin();

        for (; elem != mBcmPm.end(); elem++)
        {
            out << "Cached ports=" << elem->first << endl;
        }

        rc = -1;
    }

    counts = counts + out.str();
    return rc;
}

int GearBoxAdapter::getPmCounts(string aid, BcmPm & bcmPm)
{
    int rc = 0;
    unsigned int port = aid2PortNum(aid);

    std::lock_guard<std::mutex> lock(mGearBoxPmMtx);

    // Get cached data
    auto it = mBcmPm.find(port);

    Bcm81725DataRate_t rate;
    rc = getCachedPortRate(port, rate);

    if (rc != 0)
    {
        // No cached data yet, send empty PMs back.
        memset(&bcmPm, 0, sizeof(bcmPm));
        return rc;
    }

    if (it == mBcmPm.end())
    {
        memset(&bcmPm, 0, sizeof(bcmPm));

        if ( rate == RATE_400GE   ||
             rate == RATE_4_100GE ||
             rate == RATE_OTU4)
        {
            bcmPm.fecMode = NoFEC;
        }
        else
        {
            // No cached data yet, send empty PMs back. mBcmPm is empty
            rc = -1;
            INFN_LOG(SeverityLevel::info) << "No cache yet for mBcmPm, AID=" << aid;
        }
    }
    else
    {
        // Have cached data
        if ( rate != RATE_100GE)
        {
            // Not 100Gbe, PMs are retrieved from DCO
            memset(&bcmPm, 0, sizeof(bcmPm));
            bcmPm.fecMode = NoFEC;
        }
        else
        {
            // 100Gbe, PMs are retrieved from retimer
            memcpy(&bcmPm, &it->second, sizeof (bcmPm));
        }
        // Clean up after read
        memset(&it->second.pcs,      0, sizeof(it->second.pcs));
        memset(&it->second.pcsLanes, 0, sizeof(it->second.pcsLanes));
        memset(&it->second.fec,      0, sizeof(it->second.fec));
        memset(&it->second.fecLanes, 0, sizeof(it->second.fecLanes));
    }

    return rc;
}

void GearBoxAdapter::cachePcsPmCounts(unsigned int port, unsigned int lane, const bcm_plp_pcs_status_t& st)
{
    std::lock_guard<std::mutex> lock(mGearBoxPmMtx);

    std::map<unsigned int, BcmPm>::iterator it;

    it = mBcmPm.find(port);

    if (it == mBcmPm.end())
    {
        // Does not exist, create it
        BcmPm bcmPm;
        memset(&bcmPm, 0, sizeof(bcmPm));
        mBcmPm.insert(make_pair(port, bcmPm));

        // Point iterator at newly created pm container
        it = mBcmPm.find(port);
    }


    if (it != mBcmPm.end())
    {
        // Save PCS counts
        // ber_low_cnt @8..13 (count[0:5]) ; latch_high_ber @14 ; latch_block_lock @15
        unsigned short lowCnt  = (st.ieee_pcs_sts.ieee_baser_pcs_status_2 >> 8) & 0x3f;


        // ber_high_cnt @0..15 (count [6:21])
        unsigned short highCnt = ((st.ieee_pcs_sts.ieee_ber_high_order_counter & 0xffff) << 6);


        // found
        mBcmPm[port].pcsLanes[lane].icgCounts       = (highCnt | lowCnt); // instant ICG counts per physical lane
        mBcmPm[port].pcsLanesAccum[lane].icgCounts += (highCnt | lowCnt); // accumulative ICG counts per physical lane


        mBcmPm[port].pcsLanes[lane].bip8Counts = 0;

        for (unsigned int i = 0; i < cNumPcsLanes; i++)
        {
            mBcmPm[port].pcsLanes[lane].bip8Counts        += st.ieee_pcs_sts.ieee_bip_err_count_pcsln[i];
            mBcmPm[port].pcsLanes[lane].bip8CountsLane[i]  = st.ieee_pcs_sts.ieee_bip_err_count_pcsln[i];

            mBcmPm[port].pcsLanesAccum[lane].bip8CountsLane[i]  += mBcmPm[port].pcsLanes[lane].bip8CountsLane[i];
        }

        mBcmPm[port].pcsLanesAccum[lane].bip8Counts += mBcmPm[port].pcsLanes[lane].bip8Counts; // accumulate all virtual 20 lanes

        mBcmPm[port].fecMode = PCSFEC;

        SumPcsPmPhysicalLanes(it->second, lane);
    }

}

void GearBoxAdapter::cacheFecPmCounts(unsigned int port, unsigned int lane, const bcm_plp_fec_status_t & st)
{
    std::lock_guard<std::mutex> lock(mGearBoxPmMtx);

    // Save FEC counts
    std::map<unsigned int, BcmPm>::iterator it;

    it = mBcmPm.find(port);

    if (it == mBcmPm.end())
    {
        // Does not exist, create it
        BcmPm bcmPm;
        memset(&bcmPm, 0, sizeof(bcmPm));
        mBcmPm.insert(make_pair(port, bcmPm));

        // Point iterator at newly created pm container
        it = mBcmPm.find(port);
    }
    if (it != mBcmPm.end())
    {

        // found
        // Instant counters

        // This subtraction should roll over and provide the delta since this is unsigned.
        it->second.fecLanes[lane].rxUnCorrWord = st.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt - it->second.fecLanesPrev[lane].rxUnCorrWord;


        it->second.fecLanes[lane].rxCorrWord   = st.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt - it->second.fecLanesPrev[lane].rxCorrWord;

        it->second.fecLanes[lane].fecSymbolErr = st.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt - it->second.fecLanesPrev[lane].fecSymbolErr;


        uint64 totBits = (st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[0]  +  /* [0] for correctable 0's */
                st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[1]);  /* [1] for correctable 1's */

        if (totBits > it->second.fecLanesPrev[lane].rxCorrBits)
        {
            it->second.fecLanes[lane].rxCorrBits   = totBits - it->second.fecLanesPrev[lane].rxCorrBits;
        }

        // Save previous values to perform next delta
        it->second.fecLanesPrev[lane].rxUnCorrWord = st.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt;
        it->second.fecLanesPrev[lane].rxCorrWord   = st.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt;
        it->second.fecLanesPrev[lane].fecSymbolErr = st.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt;
        it->second.fecLanesPrev[lane].rxCorrBits   = totBits;

        it->second.fecMode = KR4FEC;

        // Clearing the registers on reads is causing missed PMs. Instead do not clear registers, this should be accumulative.
        it->second.fecLanesAccum[lane].rxUnCorrWord = st.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt;
        it->second.fecLanesAccum[lane].rxCorrWord   = st.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt;
        it->second.fecLanesAccum[lane].fecSymbolErr = st.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt;
        it->second.fecLanesAccum[lane].rxCorrBits   = st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[0] + /* [0] for correctable 0's */
                st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[1];  /* [1] for correctable 1's */
        SumFecPmPhysicalLanes(it->second, lane);
    }

}

//removed mutex from this function since it does not contain any shared data structure
void GearBoxAdapter::SumPcsPmPhysicalLanes(BcmPm & bcmPm, unsigned int lane)
{
    // Sum up PCS counts
    bcmPm.pcs.icgCounts       += bcmPm.pcsLanes[lane].icgCounts;      // sum of all ICG counts on all physical lanes
    bcmPm.pcsAccum.icgCounts  += bcmPm.pcsLanes[lane].icgCounts;      // accumulative of all ICG counts on all physical lanes
    bcmPm.pcsAccum.bip8Counts += bcmPm.pcsLanes[lane].bip8Counts;     // accumulate all virtual 20 lanes and physical lanes


    for (unsigned int virtLane = 0; virtLane < cNumPcsLanes; virtLane++)
    {
        // Add all the virtual lanes and physical lanes
        bcmPm.pcs.bip8Counts += bcmPm.pcsLanes[lane].bip8CountsLane[virtLane];

        // Sum each physical lane for each virtual lanes
        bcmPm.pcs.bip8CountsLane[virtLane] += bcmPm.pcsLanes[lane].bip8CountsLane[virtLane];

        // accumulate sum of all virtual 20 lanes and physical lanes
        bcmPm.pcsAccum.bip8CountsLane[virtLane] += bcmPm.pcsLanes[lane].bip8CountsLane[virtLane];
    }

}

//removed mutex from this function since it does not contain any shared data structure
void GearBoxAdapter::SumFecPmPhysicalLanes(BcmPm & bcmPm, unsigned int lane)
{
    // Sum up FEC counts
    bcmPm.fec.rxUnCorrWord += bcmPm.fecLanes[lane].rxUnCorrWord;
    bcmPm.fec.rxCorrWord   += bcmPm.fecLanes[lane].rxCorrWord;
    bcmPm.fec.fecSymbolErr += bcmPm.fecLanes[lane].fecSymbolErr;
    bcmPm.fec.rxCorrBits   += bcmPm.fecLanes[lane].rxCorrBits;

    bcmPm.fecAccum.rxUnCorrWord += bcmPm.fecLanes[lane].rxUnCorrWord;
    bcmPm.fecAccum.rxCorrWord   += bcmPm.fecLanes[lane].rxCorrWord;
    bcmPm.fecAccum.fecSymbolErr += bcmPm.fecLanes[lane].fecSymbolErr;
    bcmPm.fecAccum.rxCorrBits   += bcmPm.fecLanes[lane].rxCorrBits;
}

int GearBoxAdapter::getFecMode(string aid, bcm_plp_fec_mode_sel_t &fecMode, Bcm81725Lane &param, int &port)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    int rc = 0;

    std::vector<ModeParam> lineSide;

    port          = aid2PortNum(aid);
    int childPort = aid2ChildPortNum(aid);

    INFN_LOG(SeverityLevel::debug) << "Childport=" << childPort << " port=" << port << " aid=" << aid;

    Bcm81725DataRate_t rate = RATE_100GE;
    rc = getCachedPortRate(port, rate);
    ModeParam    modeParamGetLine;          // Retrieved mode configuration for line
    memset(&modeParamGetLine, 0, sizeof(modeParamGetLine));
    fecMode = bcmplpNoFEC;

    auto elem = mModeParams.find(port);
    std::vector<PortConfig> portConfig;

    if (elem != mModeParams.end())
    {
        if (elem->second.size() == 0)
        {
            INFN_LOG(SeverityLevel::error) << "Error printing lanes, modeParam vector=0!";
            return -1;
        }

        if (rate == RATE_4_100GE)
        {

            if (childPort >= 1 &&
                childPort <= c4_100_numChildPorts)
            {
                unsigned int childPortIdx = ((unsigned int)(childPort - 1));
                rate    = speed2RateEnum(elem->second[0].speed, elem->second[0].dataRate, elem->second[0].fecType);
                fecMode = (bcm_plp_fec_mode_sel_t)elem->second[0].fecType;

                GetPortConfigEntry(portConfig, port, rate);
                param = {portConfig[0].bus, portConfig[0].bcmUnit, cLineSide, portConfig[0].line.laneMap};

            }
            else
            {
                rc = -1;
                INFN_LOG(SeverityLevel::error) << "SW BUG!! Childport is invalid fecMode=" << fecMode << " port=" << port << " rate=" << (int)rate << " childPort=" << childPort;
                return rc;
            }

        }
        else  // All other rates
        {
            rate    = speed2RateEnum(elem->second[0].speed, elem->second[0].dataRate, elem->second[0].fecType);
            fecMode = (bcm_plp_fec_mode_sel_t)elem->second[0].fecType;

            GetPortConfigEntry(portConfig, port, rate);
            param = {portConfig[0].bus, portConfig[0].bcmUnit, cLineSide, portConfig[0].line.laneMap};

        }


        // AF0 - Sanity check, if working can remove
        if ( (rate    == RATE_100GE   &&
              fecMode != bcmplpPCSFEC &&
              fecMode != bcmplpKR4FEC)
              ||
             (rate == RATE_400GE &&
              fecMode != bcmplpNoFEC)
              ||
             (rate == RATE_4_100GE &&
              fecMode != bcmplpNoFEC) )
        {
            INFN_LOG(SeverityLevel::error) << "SW BUG!! Cached FEC mode is bogus, cached FEC mode=" << fecMode << " port=" << port << " rate=" << (int)rate;
            rc = getMode(param, modeParamGetLine);

            if (rc == 0)
            {
                fecMode = (bcm_plp_fec_mode_sel_t)modeParamGetLine.fecType;
                lineSide.push_back(modeParamGetLine);
            }
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "AID=" << aid << " FEC mode=" << fecMode << " port=" << port << " rate=" << (int)rate;
        // Unknown fecMode, use default rate of 100GE
        GetPortConfigEntry(portConfig, port, rate);
        param = {portConfig[0].bus, portConfig[0].bcmUnit, cLineSide, portConfig[0].line.laneMap};
        rc = getMode(param, modeParamGetLine);


        if (rc == 0)
        {
            fecMode = (bcm_plp_fec_mode_sel_t)modeParamGetLine.fecType;
            lineSide.push_back(modeParamGetLine);
        }
    }

    if (elem == mModeParams.end() && lineSide.size() > 0)
    {
        mModeParams.insert(make_pair(port, lineSide));
    }

    return rc;
}

int GearBoxAdapter::getFecEnable(const string& aId, bool& isFecEnable)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    int rc = 0;
    int port = aid2PortNum(aId);
    Bcm81725Lane param;
    bcm_plp_fec_mode_sel_t fecMode = bcmplpNoFEC;
    ModeParam    modeParamGetLine;          // Retrieved mode configuration for line
    memset(&modeParamGetLine, 0, sizeof(modeParamGetLine));

    auto elem = mModeParams.find(port);
    std::vector<PortConfig> portConfig;

    if (elem != mModeParams.end())
    {
        fecMode = (bcm_plp_fec_mode_sel_t)elem->second[0].fecType;
    }
    else
    {
        // Unknown fecMode, use default rate of 100GE
        GetPortConfigEntry(portConfig, port, RATE_100GE);
        param = {portConfig[0].bus, portConfig[0].bcmUnit, cLineSide, portConfig[0].line.laneMap};
        rc = getMode(param, modeParamGetLine);

        if (rc == 0)
        {
            fecMode = (bcm_plp_fec_mode_sel_t)modeParamGetLine.fecType;
        }
    }

    if (fecMode == bcmplpKR4FEC)
    {
        isFecEnable = true;
    }
    else
    {
        isFecEnable = false;
    }

    return rc;
}


//removed mutex from this function, this function is been triggered periodically by gige state collector
//only hold mGearBoxPmMtx in cachePcsPmCounts and cacheFecPmCounts but not when retrieve info from hardware
//the other callback thread also need to access shared pm data structure via getPmCounts using mGearBoxPmMtx
int GearBoxAdapter::getBcmFaults(string aid, DpAdapter::GigeFault* faults)
{
    timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start); // get the start time

    int rc = 0;
    Bcm81725DataRate rate = RATE_100GE;
    int port = 0;
    faults->facBmp = 0;
    Bcm81725Lane param;
    bcm_plp_fec_mode_sel_t fecMode;
    std::vector<bool> bcmFaults(cNumBcmFaults, false);



    rc += getFecMode(aid, fecMode, param, port);
    rc += getCachedPortRate(port, rate);


    if (rc != 0)
    {
        faults = NULL;
        return rc;
    }

    if ( rate == RATE_100GE
          &&
         fecMode == bcmplpPCSFEC) // PCS
    {
        rc = getPcsFaults(param, port, bcmFaults);
    }
    else if (rate == RATE_100GE && fecMode == bcmplpKR4FEC) // FEC
    {
        rc = getFecFaults(param, port, bcmFaults);
    }
    else if (rate == RATE_400GE && fecMode == bcmplpNoFEC)
    {
        rc = get400gFaults(param, port, bcmFaults);
    }
    else if (rate == RATE_4_100GE)
    {
        rc = get4_100gFaults(param, port, bcmFaults);
    }
    else if (rate == RATE_OTU4)
    {
        rc = getOtu4Faults(param, port, bcmFaults);
    }
    else
    {
        rc = getEmptyFaults(param, port, bcmFaults);
    }

    if (rc == 0)
    {
        ParseAlarms(bcmFaults, faults);
    }
    else
    {

        faults = NULL;
        rc = -1;
    }


    clock_gettime(CLOCK_REALTIME, &end); // get the finishing time

    auto it = mBcmPm.find(port);

    if (it != mBcmPm.end())
    {
        mBcmFltPmTime[PORT_TO_INDEX(port)].start = (unsigned long)start.tv_nsec;
        mBcmFltPmTime[PORT_TO_INDEX(port)].stop  = (unsigned long)end.tv_nsec;
        mBcmFltPmTime[PORT_TO_INDEX(port)].deltaTime = (unsigned long)(end.tv_nsec - start.tv_nsec);
    }

    return rc;
}

int GearBoxAdapter::getEmptyFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    // Create empty container to prevent possible error condition from
    //  missing bcmPm entry. Return -1 to calling function so we revert to DCO faults.

    std::map<unsigned int, BcmPm>::iterator it;
    it = mBcmPm.find(port);
    int rc = -1; // no faults from BCM

    if (it == mBcmPm.end())
    {
        // Does not exist, create it
        BcmPm bcmPm;
        memset(&bcmPm, 0, sizeof(bcmPm));
        mBcmPm.insert(make_pair(port, bcmPm));
    }
    else
    {
        // Make sure to zero out the data
        memset(&mBcmPm[port], 0, sizeof(mBcmPm[port]));
        mBcmPm[port].fecMode = NoFEC;
    }


    // Create empty containers to return if a call is made.
    return rc;
}

int GearBoxAdapter::getOtu4Faults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    // No BCM faults and PMs for OTU4, just create empty container to prevent possible error condition from
    //  missing bcmPm entry. Return -1 to calling function so we revert to DCO faults.

    // No BCM faults for OTU4
    // No BCM PMs  for OTU4
    // Create empty containers to return if a call is made.
    return getEmptyFaults(param, port, faults);
}



int GearBoxAdapter::get400gFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    // No BCM faults and PMs for 400G, just create empty container to prevent possible error condition from
    //  missing bcmPm entry. Return -1 to calling function so we revert to DCO faults.

    // No BCM faults for 400G
    // No BCM PMs  for 400G
    // Create empty containers to return if a call is made.
    return getEmptyFaults(param, port, faults);
}

int GearBoxAdapter::get4_100gFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    // No BCM faults and PMs for 4x100G, just create empty container to prevent possible error condition from
    //  missing bcmPm entry. Return -1 to calling function so we revert to DCO faults.

    // No BCM faults for 4x100G
    // No BCM PMs  for 4x100G
    // Create empty containers to return if a call is made.

    return getEmptyFaults(param, port, faults);
}

//removed mutex from this function
//getFecStatus will use mGearBoxPtrMtx to retrieve hardware info
//cacheFecPmCounts will use mGearBoxPmMtx to access shared pm data structure mBcmPm
//so that getPmCounts(triggered from callback thread) will not be waiting for same mutex while reading from bcm
int GearBoxAdapter::getFecFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    bcm_plp_fec_status_t st;

    unsigned int losync = 0;
    unsigned int lolm = 0;
    unsigned int loa = 0;
    int rc = -1;

    unsigned int lane = 0;
    unsigned int numLanes = 0;

    rc = getFecStatus(param, false, st);

    // Originally, coded up to handle each individual physical lane, but after talking to Broadcom, we only need the 4 lane bit mask.
    // Lane will always be 0.
    cacheFecPmCounts(port, lane, st);


    // Currently, no FEC faults can be accurately captured, return failure to force GigeClient to ignore faults from BCM.
    rc = -1;
    return rc;
}

//removed mutex from this function
//getPcsStatus will use mGearBoxPtrMtx to retrieve hardware info
//cachePcsPmCounts will use mGearBoxPmMtx to access shared pm data structure mBcmPm
//so that getPmCounts(triggered from callback thread) will not be waiting for same mutex while reading from bcm
int GearBoxAdapter::getPcsFaults(const Bcm81725Lane & param, unsigned int port, std::vector<bool> & faults)
{
    bcm_plp_pcs_status_t st;

    const unsigned int cAlignmentStatusMask = 0xfffff;
    unsigned int losync = 0;
    unsigned int lolm = 0;
    unsigned int loa = 0;
    unsigned int hiber = 0;
    int rc = -1;
    Bcm81725Lane paramMask;
    unsigned int lane = 0;


    rc = getPcsStatus(param, st);
    // Originally, coded up to handle each individual physical lane, but after talking to Broadcom, we only need the 4 lane bit mask.
    // Lane will always be 0.
    cachePcsPmCounts(port, lane, st);


    // Determine alarms from status, mask bits  and counters
    losync = ( ((st.ieee_pcs_sts.ieee_pcs_alignment_status_2 << 8) | st.ieee_pcs_sts.ieee_pcs_alignment_status_1) & cAlignmentStatusMask);
    lolm   = ( ((st.ieee_pcs_sts.ieee_pcs_alignment_status_4 << 8) | st.ieee_pcs_sts.ieee_pcs_alignment_status_3) & cAlignmentStatusMask);
    loa    = st.ieee_pcs_sts.ieee_pcs_alignment_status_1 >> 12;
    hiber  = st.pcs_sts.pcs_hiber_stat;


    if (losync != cAlignmentStatusMask)
    {
        if ((size_t)LOSYNC < faults.size())
        {
            faults[LOSYNC] = true;
        }
    }
    else if (lolm != cAlignmentStatusMask)
    {
        if ((size_t)LOLM < faults.size())
        {
            faults[LOLM] = true;
        }
    }
    else if (loa != 1)
    {
        if ((size_t)LOA < faults.size())
        {
            faults[LOA] = true;
        }
    }

    // Check for HI-BER
    if (hiber > 0)
    {
        if ((size_t)HIBER < faults.size())
        {
            faults[HIBER] = true;
        }
    }



    return rc;
}

int GearBoxAdapter::printPcsStatus(const Bcm81725Lane& param, ostringstream& out)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    const unsigned int cWidth      = 6;
    const unsigned int cWidthColon = 3;
    bcm_plp_pcs_status_t st;

    int rc = 0;

    rc = getPcsStatus(param, st);

    if (rc == 0)
    {
        // ber_low_cnt @8..13 (count[0:5]) ; latch_high_ber @14 ; latch_block_lock @15
        unsigned short lowCnt  = (st.ieee_pcs_sts.ieee_baser_pcs_status_2 >> 8) & 0x3f;


        // ber_high_cnt @0..15 (count [6:21])
        unsigned short highCnt = ((st.ieee_pcs_sts.ieee_ber_high_order_counter & 0xffff) << 6);


        out << "===IEEE PCS Rx Status===" << endl;

        out <<  "  ieee_baser_pcs_status_1      : " << setw(cWidth) << st.ieee_pcs_sts.ieee_baser_pcs_status_1 << endl <<
                "  ieee_baser_pcs_status_2      : " << setw(cWidth) << st.ieee_pcs_sts.ieee_baser_pcs_status_2 << endl <<
                "  ieee_ber_high_order_counter  : " << setw(cWidth) << st.ieee_pcs_sts.ieee_ber_high_order_counter << endl <<
                "  ieee_pcs_alignment_status_1  : " << setw(cWidth) << st.ieee_pcs_sts.ieee_pcs_alignment_status_1 << endl <<
                "  ieee_pcs_alignment_status_2  : " << setw(cWidth) << st.ieee_pcs_sts.ieee_pcs_alignment_status_2 << endl <<
                "  ieee_pcs_alignment_status_3  : " << setw(cWidth) << st.ieee_pcs_sts.ieee_pcs_alignment_status_3 << endl <<
                "  ieee_pcs_alignment_status_4  : " << setw(cWidth) << st.ieee_pcs_sts.ieee_pcs_alignment_status_4 << endl <<
                "  Calculated ICG counts        : " << setw(cWidth) << (highCnt | lowCnt) << endl << endl;

        out << "  BIP Error Count PCS Lane" << endl;
        const unsigned int cBipErrCountPcsLnSize =  sizeof(st.ieee_pcs_sts.ieee_bip_err_count_pcsln) /
                sizeof(st.ieee_pcs_sts.ieee_bip_err_count_pcsln[0]);

        for (unsigned int i = 0; i < cBipErrCountPcsLnSize; i++)
        {
            out << "    Bip Error PCS Lane " << setw(cWidthColon) << i << " : " << setw(cWidth) << st.ieee_pcs_sts.ieee_bip_err_count_pcsln[i] << endl;
        }

        out << endl;

        const unsigned int cPcsLaneMappingSize = sizeof(st.ieee_pcs_sts.ieee_pcs_lane_mapping) /
                sizeof(st.ieee_pcs_sts.ieee_pcs_lane_mapping[0]);

        for (unsigned int i = 0; i < cBipErrCountPcsLnSize; i++)
        {
            out << "  PCS Lane Mapping " << setw(cWidthColon) << i << " : " << setw(cWidth) << st.ieee_pcs_sts.ieee_pcs_lane_mapping[i] << endl;
        }


        const unsigned int cPcsStatusPhylane = sizeof(st.pcs_sts.pcs_status_phylane) /
                sizeof(st.pcs_sts.pcs_status_phylane[0]);

        const unsigned int cPcsLaneMapping   = sizeof(st.pcs_sts.pcs_status_phylane[0].pcs_lane_mapping) /
                sizeof(st.pcs_sts.pcs_status_phylane[0].pcs_lane_mapping[0]);
        const unsigned int cPcsBipErrCnt     = sizeof(st.pcs_sts.pcs_status_phylane[0].pcs_bip_err_cnt) /
                sizeof(st.pcs_sts.pcs_status_phylane[0].pcs_bip_err_cnt[0]);

        out << endl;
        out << "===PCS Rx Status===" << endl;

        out << "  pcs_igbox_clsn_sticky    : " << setw(cWidth) << (unsigned int)st.pcs_sts.pcs_igbox_clsn_sticky << endl;

        for (unsigned int i = 0; i < cPcsStatusPhylane; i++)
        {
            out << "  pcs_status_phylane[" << i << "]"                                                                                        << endl <<
                    "    pcs_block_lock_stat    : " << setw(cWidth) << (unsigned int)st.pcs_sts.pcs_status_phylane[i].pcs_block_lock_stat     << endl <<
                    "    pcs_block_lolock_sticky: " << setw(cWidth) << (unsigned int)st.pcs_sts.pcs_status_phylane[i].pcs_block_lolock_sticky << endl <<
                    "    pcs_am_lock_stat       : " << setw(cWidth) << (unsigned int)st.pcs_sts.pcs_status_phylane[i].pcs_am_lock_stat        << endl <<
                    "    pcs_am_lolock_sticky   : " << setw(cWidth) << (unsigned int)st.pcs_sts.pcs_status_phylane[i].pcs_am_lolock_sticky    << endl <<
                    "    pcs_dskw_error_sticky  : " << setw(cWidth) << (unsigned int)st.pcs_sts.pcs_status_phylane[i].pcs_dskw_error_sticky   << endl;

            for (unsigned int j = 0; j < cPcsLaneMapping; j++)
            {
                out << "    pcs_lane_mapping[" << j << "]    : " << setw(cWidth) <<(unsigned int)st.pcs_sts.pcs_status_phylane[i].pcs_lane_mapping[j] << endl;
            }

            for (unsigned int j = 0; j < cPcsBipErrCnt; j++)
            {
                out << "    pcs_bip_err_cnt[" << j << "]     : " << setw(cWidth) <<(unsigned int)st.pcs_sts.pcs_status_phylane[i].pcs_bip_err_cnt[j] << endl;
            }
        }

        out << endl;

        out << "  pcs_dskw_align_stat        : " <<  setw(cWidth) << (unsigned int)st.pcs_sts.pcs_dskw_align_stat << endl
                << "  pcs_dskw_align_loss_sticky : " <<  setw(cWidth) << (unsigned int)st.pcs_sts.pcs_dskw_align_loss_sticky << endl
                << "  pcs_hiber_stat             : " <<  setw(cWidth) << (unsigned int)st.pcs_sts.pcs_hiber_stat << endl
                << "  pcs_hiber_sticky           : " <<  setw(cWidth) << (unsigned int)st.pcs_sts.pcs_hiber_sticky << endl
                << "  pcs_ber_cnt                : " <<  setw(cWidth) << (unsigned int)st.pcs_sts.pcs_ber_cnt << endl
                << "  pcs_link_stat              : " <<  setw(cWidth) << (unsigned int)st.pcs_sts.pcs_link_stat << endl
                << "  pcs_link_stat_sticky       : " <<  setw(cWidth) << (unsigned int)st.pcs_sts.pcs_link_stat_sticky << endl;

    }
    else
    {
        out << "Error retrieving PCS status!" << endl;
    }

    return rc;
}

int GearBoxAdapter::dumpPcsStatus(unsigned int port, Bcm81725DataRate_t rate, std::string &status)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    ostringstream out;
    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);

    Bcm81725Lane param, paramMask;


    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};

        out << std::hex << "====Dump PCS Status for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " host side " <<  "laneMap=0x"  << portConfig[i].host.laneMap << "====" << std::dec << endl;

        rc = printPcsStatus(param, out);

        if (rc != 0)
        {
            INFN_LOG(SeverityLevel::info) << "printPcsStatus rc=" << rc;
            break;
        }

        param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};

        out << std::hex << "====Dump PCS status for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " line side " <<  "laneMap=0x"  << portConfig[i].line.laneMap << "====" << std::dec << endl;
        rc = printPcsStatus(param, out);

        if (rc != 0)
        {
            INFN_LOG(SeverityLevel::info) << "printPcsStatus rc=" << rc;
            break;
        }
    }

    status = out.str();

    if (portConfig.size() == 0)
    {
        rc = -1;
    }
    return rc;

}

int GearBoxAdapter::printFecStatus(const Bcm81725Lane& param, bool clear, std::ostringstream& out)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    const unsigned int cWidth      = 6;
    const unsigned int cWidthColon = 3;
    bcm_plp_fec_status_t st;
    memset(&st, 0, sizeof(bcm_plp_fec_status_t));
    int rc = 0;

    st.fec_status_clear = clear != 0 ? 1 : 0;
    rc = getFecStatus(param, clear, st);



    if (rc == 0)
    {
        out << "st.align_lol_sticky : " << setw(cWidth) << st.align_lol_sticky <<  endl;
        out << "st.align_lock       : " << setw(cWidth) << st.align_lock <<  endl << endl;
        //fec_err_cnt =
        {
            out << "===FEC Error Count===" << endl;

            out << "  st.fec_err_cnt.ieee_uncorr_cnt       : " << st.fec_err_cnt.ieee_uncorr_cnt <<  endl;
            out << "  st.fec_err_cnt.ieee_symbols_corr_cnt : " << st.fec_err_cnt.ieee_symbols_corr_cnt <<  endl;
            out << "  st.fec_err_cnt.sys_am_lock           : " << st.fec_err_cnt.sys_am_lock <<  endl;
            out << "  st.fec_err_cnt.line_am_lock          : " << st.fec_err_cnt.line_am_lock <<  endl;
            out << "  st.fec_err_cnt.ieee_kp4_dec_ctrl     : " << st.fec_err_cnt.ieee_kp4_dec_ctrl <<  endl;
            out << "  st.fec_err_cnt.ieee_kp4_stat_ctrl    : " << st.fec_err_cnt.ieee_kp4_stat_ctrl <<  endl;
            out << "  st.fec_err_cnt.ieee_fec_lane_mapping : " << st.fec_err_cnt.ieee_fec_lane_mapping <<  endl;
            out << "  st.fec_err_cnt.ieee_kp4_hi_ser_th_sp : " << st.fec_err_cnt.ieee_kp4_hi_ser_th_sp <<  endl;
            out << "  st.fec_err_cnt.tot_frame_rev_cnt     : " << st.fec_err_cnt.tot_frame_rev_cnt <<  endl;
            out << "  st.fec_err_cnt.tot_frame_corr_cnt    : " << st.fec_err_cnt.tot_frame_corr_cnt <<  endl;
            out << "  st.fec_err_cnt.tot_frame_uncorr_cnt  : " << st.fec_err_cnt.tot_frame_uncorr_cnt <<  endl;
            out << "  st.fec_err_cnt.tot_symbols_corr_cnt  : " << st.fec_err_cnt.tot_symbols_corr_cnt <<  endl;
            out << "  st.fec_err_cnt.tot_bits_corr_cnt[0]  : " << st.fec_err_cnt.tot_bits_corr_cnt[0] <<  endl;
            out << "  st.fec_err_cnt.tot_bits_corr_cnt[1]  : " << st.fec_err_cnt.tot_bits_corr_cnt[1] <<  endl;

            out << endl;
            out << "  Total Bits Corrected Count" << endl;
            for (int j = 0; j < 16; j++)
            {
                out << "    st.fec_err_cnt.bcm_plp_tot_frames_err_cnt["<<j<<"] : " <<
                        st.fec_err_cnt.bcm_plp_tot_frames_err_cnt[j] <<  endl;
            }
        }
        //fec_dump_status =
        {
            //ieee_fec_sts =
            {
                out << endl;
                out << "  st.fec_dump_status.ieee_fec_sts.ieee_rsfec_ctrl       : " << st.fec_dump_status.ieee_fec_sts.ieee_rsfec_ctrl <<  endl;
                out << "  st.fec_dump_status.ieee_fec_sts.ieee_rsfec_stat       : " << st.fec_dump_status.ieee_fec_sts.ieee_rsfec_stat <<  endl;
                out << "  st.fec_dump_status.ieee_fec_sts.ieee_fec_lane_mapping : " << st.fec_dump_status.ieee_fec_sts.ieee_fec_lane_mapping <<  endl;
                out << "  st.fec_dump_status.ieee_fec_sts.ieee_corr_cw_cnt      : " << st.fec_dump_status.ieee_fec_sts.ieee_corr_cw_cnt <<  endl;
                out << "  st.fec_dump_status.ieee_fec_sts.ieee_uncorr_cw_cnt    : " << st.fec_dump_status.ieee_fec_sts.ieee_uncorr_cw_cnt <<  endl;

                out << "  FEC Status IEEE Symbols Corrected Count" << endl;

                for (int j = 0; j < 8; j++)
                {
                    out << "    st.fec_dump_status.ieee_fec_sts.ieee_symbols_corr_cnt_fln["<<j<<"] : " << st.fec_dump_status.ieee_fec_sts.ieee_symbols_corr_cnt_fln[j] <<  endl;
                }

                out << "  st.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_1 : " << st.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_1 <<  endl;
                out << "  st.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_3 : " << st.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_3 <<  endl;

                out << endl;
                out << "  IEEE FEC PCS Lane Mapping" << endl;
                for (int k = 0; k < 8; k++)
                {
                    out << "    st.fec_dump_status.ieee_fec_sts.ieee_fecpcs_lane_mapping["<<k<<"] : " << st.fec_dump_status.ieee_fec_sts.ieee_fecpcs_lane_mapping[k] <<  endl;
                }
            }
            //fec_sts =
            {
                out << endl;
                out << "  st.fec_dump_status.fec_sts.igbox_clsn_sticky    : " << (int) st.fec_dump_status.fec_sts.igbox_clsn_sticky <<  endl;
                out << "  st.fec_dump_status.fec_sts.am_lolock_sticky     : " << (int) st.fec_dump_status.fec_sts.am_lolock_sticky <<  endl;
                out << "  st.fec_dump_status.fec_sts.dgbox_clsn_sticky    : " << (int) st.fec_dump_status.fec_sts.dgbox_clsn_sticky <<  endl;
                out << "  st.fec_dump_status.fec_sts.hi_ser_sticky        : " << (int) st.fec_dump_status.fec_sts.hi_ser_sticky <<  endl;
                out << "  st.fec_dump_status.fec_sts.xdec_err_sticky      : " << (int) st.fec_dump_status.fec_sts.xdec_err_sticky <<  endl;
                out << "  st.fec_dump_status.fec_sts.fec_link_stat        : " << (int) st.fec_dump_status.fec_sts.fec_link_stat <<  endl;
                out << "  st.fec_dump_status.fec_sts.fec_link_stat_sticky : " << (int) st.fec_dump_status.fec_sts.fec_link_stat_sticky <<  endl << endl;
            }
            //fec_a_err_cnt =
            {
                out << endl;
                out << "  st.fec_dump_status.fec_a_err_cnt.tot_frame_rev_cnt    : " << st.fec_dump_status.fec_a_err_cnt.tot_frame_rev_cnt <<  endl;
                out << "  st.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt   : " << st.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt <<  endl;
                out << "  st.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt : " << st.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt <<  endl;
                out << "  st.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt : " << st.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt <<  endl;
                out << "  st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[0] : " << st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[0] <<  endl;
                out << "  st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[1] : " << st.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[1] <<  endl;

                out << endl;
                out << "  Total Frame Error Count" << endl;
                for (int j = 0; j < 16; j++)
                {
                    out << "  st.fec_dump_status.fec_a_err_cnt.tot_frames_err_cnt["<<j<<"] : " << st.fec_dump_status.fec_a_err_cnt.tot_frames_err_cnt[j] <<  endl;
                }
            }
            //fec_b_err_cnt =
            {
                out << endl;
                out << "  st.fec_dump_status.fec_b_err_cnt.tot_frame_rev_cnt    : " << st.fec_dump_status.fec_b_err_cnt.tot_frame_rev_cnt <<  endl;
                out << "  st.fec_dump_status.fec_b_err_cnt.tot_frame_corr_cnt   : " << st.fec_dump_status.fec_b_err_cnt.tot_frame_corr_cnt <<  endl;
                out << "  st.fec_dump_status.fec_b_err_cnt.tot_frame_uncorr_cnt : " << st.fec_dump_status.fec_b_err_cnt.tot_frame_uncorr_cnt <<  endl;
                out << "  st.fec_dump_status.fec_b_err_cnt.tot_symbols_corr_cnt : " << st.fec_dump_status.fec_b_err_cnt.tot_symbols_corr_cnt <<  endl;
                out << "  st.fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt[0] : " << st.fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt[0] <<  endl;
                out << "  st.fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt[1] : " << st.fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt[1] <<  endl;

                out << endl;
                out << "  Total Frames Error Count" << endl;
                for (int j = 0; j < 16; j++)
                {
                    out << "    st.fec_dump_status.fec_b_err_cnt.tot_frames_err_cnt["<<j<<"] : " << st.fec_dump_status.fec_b_err_cnt.tot_frames_err_cnt[j] <<  endl;
                }
            }
            //fec_ber =
            {
                out << endl;
                out << "  st.fec_dump_status.fec_ber.pre_fec_ber       : " << st.fec_dump_status.fec_ber.pre_fec_ber <<  endl;
                out << "  st.fec_dump_status.fec_ber.post_fec_ber      : " << st.fec_dump_status.fec_ber.post_fec_ber <<  endl;
                out << "  st.fec_dump_status.fec_ber.post_fec_ber_proj : " << st.fec_dump_status.fec_ber.post_fec_ber_proj <<  endl << endl;
            }
        }
        out << "  st.fec_status_clear : " << (int) st.fec_status_clear <<  endl;
    }

    return rc;
}


int GearBoxAdapter::getFecStatus(const Bcm81725Lane& param, bool clear, bcm_plp_fec_status_t& st)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);
    memset(&st, 0, sizeof(st));
    st.fec_status_clear = clear != 0 ? 1 : 0;
    return myDriverPtr->getFecStatus(param, clear, st);
}



int GearBoxAdapter::dumpFecStatus(unsigned int port, Bcm81725DataRate_t rate, bool clear, std::string &status)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    ostringstream out;
    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);

    Bcm81725Lane param, paramMask;


    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};
        //warmInit(param);


        out << std::hex << "====Get FEC Status for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " host side " <<  "laneMap=0x"  << portConfig[i].host.laneMap << "====" << endl;

        for (uint16 laneMask = 0; laneMask < (sizeof(uint16) * 8 /*num bits*/); laneMask++)
        {
            uint16 bitMask = (1 << laneMask);

            if (portConfig[i].host.laneMap & bitMask)
            {
                out << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                paramMask = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, bitMask};

                rc = printFecStatus(param, clear, out);

                if (rc != 0)
                {
                    INFN_LOG(SeverityLevel::info) << "getFecStatus rc=" << rc;
                    break;
                }

            }
        }


        param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};
        //warmInit(param);

        out << std::hex << "====Get FEC status for bus=0x" << portConfig[i].bus << " bcm=0x" << portConfig[i].bcmUnit << " line side " <<  "laneMap=0x"  << portConfig[i].line.laneMap << "====" << endl;



        for (uint16 laneMask = 0; laneMask < (sizeof(uint16) * 8 /*num bits*/); laneMask++)
        {
            uint16 bitMask = (1 << laneMask);

            if (portConfig[i].line.laneMap & bitMask)
            {
                out << std::hex << "Lane mask=0x" << bitMask << std::dec << endl;
                paramMask = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, bitMask};

                rc = printFecStatus(param, clear, out);

                if (rc != 0)
                {
                    INFN_LOG(SeverityLevel::info) << "getFecStatus rc=" << rc;
                    break;
                }
            }
        }

    }

    status = out.str();

    if (portConfig.size() == 0)
    {
        rc = -1;
    }
    return rc;



}

int GearBoxAdapter::getCachedPortRate(unsigned int port, Bcm81725DataRate_t & rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    int rc = 0;
    auto elem = mModeParams.find(port);
    rate = RATE_100GE;

    if (elem != mModeParams.end())
    {
        rate = speed2RateEnum(elem->second[0].speed, elem->second[0].dataRate, elem->second[0].fecType);
    }
    else
    {
        rc = -1;
    }
    return rc;
}

bool GearBoxAdapter::isPortConfigured(unsigned int port)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    bool found = false;
    auto elem = mModeParams.find(port);

    if (elem != mModeParams.end())
    {
        found = true;
    }
    return found;
}
int GearBoxAdapter::dumpPam4Diag(unsigned int port, Bcm81725DataRate_t & rate, std::string &status)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);


    ostringstream out;
    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);

    Bcm81725Lane param, paramMask;


    bcm_plp_pm_phy_pam4_diagnostics_t pam4;

    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};
        memset(&pam4, 0, sizeof(bcm_plp_pm_phy_pam4_diagnostics_t));
        rc += myDriverPtr->getPam4Diag(param, &pam4);
        out << "===Host side===" << endl;
        out << "sig          :i:"+to_string(pam4.signal_detect) << endl;
        out << "lock         :i:"+to_string(pam4.rx_lock) << endl;
        out << "vga          :i:"+to_string(pam4.vga) << endl;
        out << "osr          :i:"+to_string(pam4.osr_mode) << endl;
        out << "pmd          :i:"+to_string(pam4.pmd_mode) << endl;
        out << "snr          :d:"+to_string(pam4.snr) << endl;
        out << "rx_ppm_real  :d:"+to_string(pam4.rx_ppm_real) << endl;
        out << "tx_ppm_real  :d:"+to_string(pam4.tx_ppm_real) << endl;
        out << "pf           :i:"+to_string(pam4.peaking_filter) << endl;
        out << "txfir_pre2   :i:"+to_string(pam4.txfir_pre2) << endl;
        out << "txfir_pre    :i:"+to_string(pam4.txfir_pre) << endl;
        out << "txfir_main   :i:"+to_string(pam4.txfir_main) << endl;
        out << "txfir_post1  :i:"+to_string(pam4.txfir_post1) << endl;
        out << "txfir_post2  :i:"+to_string(pam4.txfir_post2) << endl;
        out << "txfir_post3  :i:"+to_string(pam4.txfir_post3) << endl << endl;


        param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};
        memset(&pam4, 0, sizeof(bcm_plp_pm_phy_pam4_diagnostics_t));
        rc += myDriverPtr->getPam4Diag(param, &pam4);

        out << "===Line side===" << endl;
        out << "sig        :i:"+to_string(pam4.signal_detect)  << endl;
        out << "lock       :i:"+to_string(pam4.rx_lock) << endl;
        out << "vga        :i:"+to_string(pam4.vga)  << endl;
        out << "osr        :i:"+to_string(pam4.osr_mode)  << endl;
        out << "rx_ppm     :i:"+to_string(pam4.rx_ppm)  << endl;
        out << "tx_ppm     :i:"+to_string(pam4.tx_ppm)  << endl;
        out << "txfir_pre2 :i:"+to_string(pam4.txfir_pre2)  << endl;
        out << "txfir_pre  :i:"+to_string(pam4.txfir_pre)  << endl;
        out << "txfir_main :i:"+to_string(pam4.txfir_main)  << endl;
        out << "txfir_post1:i:"+to_string(pam4.txfir_post1)  << endl;
        out << "txfir_post2:i:"+to_string(pam4.txfir_post2)  << endl;
        out << "txfir_post3:i:"+to_string(pam4.txfir_post3)  << endl;
        out << "eye_lt     :i:"+to_string(pam4.eyescan.heye_left)  << endl;
        out << "eye_rt     :i:"+to_string(pam4.eyescan.heye_right)  << endl;
        out << "eye_up     :i:"+to_string(pam4.eyescan.veye_upper)  << endl;
        out << "eye_dn     :i:"+to_string(pam4.eyescan.veye_lower)  << endl << endl;
    }

    status = out.str();
    return rc;

}

int GearBoxAdapter::setFailOverPrbs(const Bcm81725Lane& param, unsigned int mode)
{
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->setFailOverPrbs(param, mode);
}

int GearBoxAdapter::getFailOverPrbs(const Bcm81725Lane& param, unsigned int &mode)
{
    std::lock_guard<std::recursive_mutex> lock(mGearBoxPtrMtx);

    return myDriverPtr->getFailOverPrbs(param, mode);
}

int GearBoxAdapter::getAvsTpsData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData)
{
    INFN_LOG(SeverityLevel::info) << "mezzIdx: " << mezzIdx << " bcmIdx: " << bcmIdx << " opCode: " << opCode << " len: " << len;

    return getAvsPmbusData(mezzIdx, bcmIdx, opCode, len, pData);
}

int GearBoxAdapter::setAvsTpsData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData)
{
    INFN_LOG(SeverityLevel::info) << "mezzIdx: " << mezzIdx << " bcmIdx: " << bcmIdx << " opCode: " << opCode << " len: " << len;

    return setAvsPmbusData(mezzIdx, bcmIdx, opCode, len, pData);
}


}
