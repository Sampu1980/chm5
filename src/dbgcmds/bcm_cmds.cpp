#include <cli/clilocalsession.h> // include boost asio
#include <cli/remotecli.h>
#include <cli/cli.h>
#include <boost/function.hpp>
#include <chrono>
#include "dp_common.h"
#include "GearBoxAdapter.h"
#include "GearBoxUtils.h"
#include <vector>
#include "epdm_bcm_common_defines.h"
#include <iomanip>
#include "Bcm81725_avs_util.h"
#include "InfnLogger.h"

using namespace cli;
using namespace std;
using namespace boost;
using namespace gearbox;


GearBoxAdIf* gearBoxInit(const Bcm81725Lane& param)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        cout << "gearBoxInit: adapter nullptr" << endl;
        return nullptr;
    }

    if (adapter->warmInit(param))
    {
        cout << "failed on warm Init" << endl;
        return nullptr;
    }
    return adapter;
}

void cmdBcm81725LoadFirmware(ostream& out, unsigned int bus, bool force)
{
    std::cout << "cmdBcm81725LoadFirmware: " << force << endl;
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    adapter->loadFirmware(bus, force);
}




void cmdBcm81725WarmReboot(ostream& out)
{

}

void cmdSetPortRate(std::ostream& out, std::string aId, unsigned int rate, bool fecEnable, unsigned int fecType)
{
    std::cout << "cmdSetPortRate: " << endl;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setPortRate(aId, (Bcm81725DataRate_t)rate, fecEnable, (bcm_plp_fec_mode_sel_t)fecType) )
    {
        out << "aId: " << aId << " rate: " << rate << " fecEnable: " << fecEnable << endl;
    }
    else
    {
        out << "cmdSetPortDataRate failed" << endl;
    }

}

void cmdSetFecEnable(std::ostream& out, std::string aId, bool fecSetting)
{
    std::cout << "cmdSetPortRate: " << endl;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setPortFecEnable(aId, fecSetting) )
    {
        out << "aId: " << aId << " fecSetting=" << fecSetting << endl;
    }
    else
    {
        out << "cmdSetFecEnable failed" << endl;
    }
}

void cmdGetPolarity(std::ostream& out, const Bcm81725Lane& param)
{
    unsigned int tx, rx;
    std::cout << "cmdGetPolarity: " << endl;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->getPolarity(param, tx, rx)) {
        out << "tx: " << tx << " rx: " << rx << endl;
    }
    else
    {
        out << "cmdGetPolarity failed" << endl;
    }
}

void cmdSetPolarity(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int txPol, unsigned int rxPol)
{
    std::cout<< " cmdSetPolarity: " << txPol << "  " << rxPol << endl;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setPolarity(param, txPol, rxPol))
    {
        out << "setPolarity successfully" << endl;
    }
    else
    {
        out << "setPolarity failed" << endl;
    }
}

void cmdSetAvsConfig(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int enable, unsigned int margin)
{
    std::cout<< " cmdSetAvsConfig: " << enable << "  " << margin << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setAvsConfig(param, enable, margin)) {
        out << "cmdSetAvsConfig successfully" << endl;
    }
    else
    {
        out << "cmdSetAvsConfig failed" << endl;
    }
}

void cmdGetAvsConfig(std::ostream& out, const Bcm81725Lane& param)
{
    std::cout << "cmdGetAvsConfig: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    unsigned int enable, disableType, ctrl, regulator, pkgShare, margin;
    if (!adapter->getAvsConfig(param, enable, disableType, ctrl, regulator, pkgShare, margin))
    {
        out << "enable: " << enable << " disableType: " << disableType <<
               " ctrl: " << ctrl << " regulator: " << regulator <<
               " pkgShare: " <<  pkgShare << " margin: " << margin << endl;
    }
    else
    {
        out << "getAvsConfig failed" << endl;
    }
}

void cmdGetAvsStatus(std::ostream& out, const Bcm81725Lane& param)
{
    std::cout << "cmdGetAvsStatus: " << endl;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    unsigned int status;
    if (!adapter->getAvsStatus(param, status))
    {
        out << "Avs Status: " << status << endl;
    }
    else
    {
        out << "cmdGetAvsStatus failed" << endl;
    }
}


void cmdGetMode(std::ostream& out, const Bcm81725Lane& param)
{
    std::cout << "cmdGetMode: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    ModeParam modeParam;
    memset(&modeParam, 0, sizeof(modeParam));
    modeParam.refClk = bcm_pm_RefClk156Mhz; // must be set to this value (0) otherwise getMode might fail

    out << std::hex << " Bus=" << bus2String(param.bus) << " bcmUnit=0x" << param.mdioAddr << " side=" << side2String(param.side) << " laneNum=0x" << param.laneNum << std::dec << endl;

    if (!adapter->getMode(param, modeParam))
    {
    	out << "  Bus       : " << param.bus << endl <<
    	       "  BCM Unit  : " << std::hex << "0x" << param.mdioAddr << endl <<
    	       "  Side      : " << param.side << endl <<
    	       "  Lane      : " << "0x" << param.laneNum << endl << std::dec <<
               "  Speed     : " << modeParam.speed << endl <<
               "  IfType    : " << modeParam.ifType << endl <<
               "  RefClk    : " << modeParam.refClk << endl <<
               "  IntfMode  : " << modeParam.interfaceMode << endl <<
			   "  DataRate  : " << modeParam.dataRate << endl <<
               "  Modulation: " << modeParam.modulation << " " << printModulation(modeParam.modulation) << endl <<
			   "  FecType   : " << modeParam.fecType << " " << printFecType((bcm_plp_fec_mode_sel_t)modeParam.fecType) << endl;
    }
    else
    {
        out << "cmdGetMode failed" << endl;
    }
}


void cmdSetMode(std::ostream& out, const Bcm81725Lane& param,
                int speed, int ifType, int refClk, int interfaceMode,
                unsigned int dataRate, unsigned int modulation, unsigned int fecType)
{
    std::cout << "cmdSetMode: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    ModeParam modeParam = {speed, ifType, refClk, interfaceMode, dataRate, modulation, fecType};


    if (!adapter->setMode(param, modeParam))
    {
        out << "cmdSetMode successful" << endl;
    }
    else
    {
        out << "cmdSetMode failed" << endl;
    }
}

void cmdDumpBcmConfig(std::ostream& out, unsigned int port)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }


    std::vector<PortConfig> portConfig;
    Bcm81725DataRate_t rate = RATE_100GE;

    if (!adapter->getCachedPortRate(port, rate))
    {
        if (adapter->getPortRate(port, rate) == 0)
        {
            out << "CmdDumpBcmConfig port=" << port << " rate=" << rate << endl;
        }
        else
        {
            out << "CmdDumpBcmConfig port=" << port << " might not be configured, will attempt dump for 100GE lanes."<< endl;
        }
    }

    GetPortConfigEntry(portConfig, port, rate);
    Bcm81725Lane param;


    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        param = {portConfig[i].bus, portConfig[i].bcmUnit, 1, portConfig[i].host.laneMap};
        cmdGetMode(out, param);

        param = {portConfig[i].bus, portConfig[i].bcmUnit, 0, portConfig[i].line.laneMap};
        cmdGetMode(out, param);
    }
}

void cmdDumpBcmConfigAll(std::ostream& out)
{

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }


    for (unsigned int port = PORT1; port <= NUM_PORTS; port++)
    {
        if (adapter->isPortConfigured(port) == true)
        {
            out << "===Port " << port << "===" << endl;
            cmdDumpBcmConfig(out, port);
            out << endl << endl;
        }
    }
}



void cmdGetLaneConfigDs(std::ostream& out, const Bcm81725Lane& param)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }
    else
    {
        bcm_plp_dsrds_firmware_lane_config_t lnConf;
        memset(&lnConf, 0, sizeof(bcm_plp_dsrds_firmware_lane_config_t));

        out << std::hex << " Bus=" << bus2String(param.bus) << " bcmUnit=0x" << param.mdioAddr << " side=" << side2String(param.side) << " laneNum=0x" << param.laneNum << std::dec << endl;
        if (!adapter->getLaneConfigDs(param, lnConf))
        {
            out << "  DfeOn                         : " << lnConf.DfeOn << endl;
            out << "  AnEnabled                     : " << lnConf.AnEnabled << endl;
            out << "  MediaType                     : " << lnConf.MediaType << endl;
            out << "  AutoPeakingEn                 : " << lnConf.AutoPeakingEnable << endl;
            out << "  OppositeCdrFirst              : " << lnConf.OppositeCdrFirst << endl;
            out << "  DcWanderMu                    : " << lnConf.DcWanderMu << endl;
            out << "  ExSlicer                      : " << lnConf.ExSlicer << endl;
            out << "  LinkTrainingReStartTimeout    : " << lnConf.LinkTrainingReStartTimeOut << endl;
            out << "  LinkTrainingCL72_CL93PresetEn : " << lnConf.LinkTrainingCL72_CL93PresetEn << endl;
            out << "  LinkTraining802_3CDPresetEn   : " << lnConf.LinkTraining802_3CDPresetEn << endl;
            out << "  LinkTrainingTxAmpTuning       : " << lnConf.LinkTrainingTxAmpTuning << endl;
            out << "  LpDfeOn                       : " << lnConf.LpDfeOn << endl;
            out << "  AnTimer                       : " << lnConf.AnTimer << endl;
        }
        else
        {
            out << "cmdGetRxAutoPeakingEn failed" << endl;
        }
    }

}


void cmdGetPortConfigDs(std::ostream& out, unsigned int port)
{
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, RATE_100GE);
    Bcm81725Lane param;

    out << "ConfigDs only valid on Host side." << endl;
    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        param = {portConfig[i].bus, portConfig[i].bcmUnit, 1, portConfig[i].host.laneMap};
        cmdGetLaneConfigDs(out, param);
    }

}

void cmdSetLaneConfigDs(std::ostream& out, const Bcm81725Lane& param, unsigned int fwLaneConfig, int64_t value)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }
    else
    {
        if (!adapter->setLaneConfigDs(param, fwLaneConfig, value))
        {
            out << "setLaneConfigDs successful" << endl;
        }
        else
        {
            out << "setLaneConfigDs failed" << endl;
        }
    }
}

void cmdSetTxFir(std::ostream& out, const Bcm81725Lane& param, int pre2, int pre, int main, int post, int post2, int post3)
{
    std::cout << "cmdSetTxFir: " << endl;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }


    out << std::hex << " Bus=" << bus2String(param.bus) << " bcmUnit=0x" << param.mdioAddr << " side=" << side2String(param.side) << " laneNum=0x" << param.laneNum << std::dec << endl;

    if (!adapter->setTxFir(param, pre2, pre, main, post, post2, post3))
    {
        out << "cmdSetTxFir successful" << endl;
    }
    else
    {
        out << "cmdSetTxFir failed" << endl;
    }


}



void cmdGetTxFir(std::ostream& out, const Bcm81725Lane& param)
{
    std::cout << "cmdGetPortTxFir: " << endl;

    int pre2  = 0;
    int pre   = 0;
    int main  = 0;
    int post  = 0;
    int post2 = 0;
    int post3 = 0;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    out << std::hex << " Bus=" << bus2String(param.bus) << " bcmUnit=0x" << param.mdioAddr << " side=" << side2String(param.side) << " laneNum=0x" << param.laneNum << std::dec << endl;

    if (!adapter->getTxFir(param, pre2, pre, main, post, post2, post3))
    {
        out << "cmdGetFir successful" << endl;
        out << "  Pre2  : " << pre2 << endl;
        out << "  Pre   : " << pre << endl;
        out << "  Main  : " << main << endl;
        out << "  Post  : " << post << endl;
        out << "  Post2 : " << post2 << endl;
        out << "  Post3 : " << post3 << endl;
    }
    else
    {
        out << "cmdGetTxFir failed" << endl;
    }
}


void cmdGetPortTxFir(std::ostream& out, unsigned int port)
{
    std::cout << "cmdGetPortTxFir: " << endl;

    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, RATE_100GE);
    Bcm81725Lane param;

    int pre2  = 0;
    int pre   = 0;
    int main  = 0;
    int post  = 0;
    int post2 = 0;
    int post3 = 0;


    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        // Host side
        param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};
        cmdGetTxFir(out, param);

        // Line side
        param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};
        cmdGetTxFir(out, param);
    }
}



void cmdPrbsLinkStatusCheck(std::ostream& out, const Bcm81725Lane& param)
{
    std::cout << "cmdPrbsLinkStatusCheck: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    unsigned int status;
    if (!adapter->prbsLinkStatusCheck(param, status))
    {
        out << "Prbs link Status: " << status << endl;
    }
    else
    {
        out << "prbsLinkStatusCheck failed" << endl;
    }
}

void cmdPrbsStatusCheck(std::ostream& out, const Bcm81725Lane& param, unsigned int timeVal)
{
    std::cout << "cmdPrbsStatusCheck: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    unsigned int status;
    if (!adapter->prbsStatusCheck(param, timeVal, status))
    {
        out << "Prbs Status: " << status << endl;
    }
    else
    {
        out << "prbsStatusCheck failed" << endl;
    }
}

void cmdSetPrbsGen(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb,
                   unsigned int enaDis)
{
    std::cout << "cmdSetPrbsGen: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setPrbsGen(param, tx, poly, inv, lb, enaDis))
    {
        out << "setPrbsGen successfully" << endl;
    }
    else
    {
        out << "setPrbsGen failed" << endl;
    }
}


void cmdSetPortLoopback(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int mode, unsigned int enable)
{
    std::cout << "cmdSetLoopback: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setLoopback(param, mode, enable))
    {
        out << "cmdSetLoopback successfully" << endl;
    }
    else
    {
        out << "cmdSetLoopback failed" << endl;
    }
}

void cmdGetPortLoopback(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int mode)
{
    std::cout << "cmdGetLoopback: " << endl;
    unsigned int enable = 0;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->getLoopback(param, mode, enable))
    {
        out << "BCM loopback=" << enable << endl;
    }
    else
    {
        out << "cmdGetLoopback failed" << endl;
    }
}

void cmdSetPrbsCheck(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int rx, unsigned int poly,
                   unsigned int inv, unsigned int lb,
                   unsigned int enaDis)
{
    std::cout << "cmdSetPrbsCheck: " << endl;
    
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setPrbsCheck(param, rx, poly, inv, lb, enaDis))
    {
        out << "setPrbsCheck successfully" << endl;
    }
    else
    {
        out << "setPrbsCheck failed" << endl;
    }
}


void cmdGetBcmInfo(ostream& out)
{

    BcmVers bcmTop, bcmBottom;
    memset(&bcmTop, 0, sizeof(bcmTop));
    memset(&bcmBottom, 0, sizeof(bcmBottom));

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->GetBcmInfo(bcmTop, bcmBottom))
    {
        out << "==Bcm81725 top mz info==" << endl;
        out << "  FW Vers: " << std::hex << bcmTop.fwVer << endl;
        out << "  FW Crc : " << bcmTop.fwCrc << endl << endl;
        out << "==Bcm81725 bottom mz info==" << endl;
        out << "  FW Vers: " << bcmBottom.fwVer << endl;
        out << "  FW Crc : " << bcmBottom.fwCrc << std::dec << endl;
    }

}

void cmdGetEyeScan(ostream& out, const Bcm81725Lane& param)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->getEyeScan(param))
    {
        out << "Get eye scan successful. Check cout logs!" << endl;
    }
    else
    {
        out << "Get eye scan failed!" << endl;
    }
}

void cmdGetEyeScan(ostream& out, unsigned int port, Bcm81725DataRate_t rate)
{

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->getEyeScan(port, rate))
    {
        out << "getEyeScan successful" << endl;
    }
    else
    {
        out << "getEyeScan failed" << endl;
    }
}

void cmdGetEyeProjection(ostream& out, unsigned int port, Bcm81725DataRate_t rate, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr)
{
    std::string errMsg;
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->getEyeProjection(port, rate, scanMode, timerControl, maxErr, errMsg))
    {
        out << "getEyeProjection successful" << endl;
    }
    else
    {
        out << "getEyeProjection failed" << endl;
    }

    if (errMsg.empty() == false)
    {
        out << errMsg << endl;
    }
}

void cmdGetEyeProjection(ostream& out, const Bcm81725Lane& param, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr)
{
    std::string errMsg;

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    unsigned int lineRate = 0;

    ModeParam modeParam;
    memset(&modeParam, 0, sizeof(modeParam));
    modeParam.refClk = bcm_pm_RefClk156Mhz; // must be set to this value (0) otherwise getMode might fail


    if (!adapter->getMode(param, modeParam))
    {

        if (!adapter->getEyeProjection(param, modeParam.dataRate, scanMode, timerControl, maxErr, errMsg))
        {
            out << "getEyeProjection successful" << endl;
        }
        else
        {
            out << errMsg << endl;
            out << "getEyeProjection failed" << endl;
        }

        if (errMsg.empty() == false)
        {
            out << errMsg << endl;
        }
    }
}


void cmdChipStatus(ostream& out, const Bcm81725Lane& param, unsigned int func)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    out << "Use func=0 for basic dump." << endl;
    /* AF0 TODO enable when MILB 5.2+
    out << "   Flag (milb5.2+):" << endl <<
        "       0x0008 = INTERNAL_DUMP" << endl <<
        "       0x0010 = INTERNAL_CHIP_DUMP"  << endl <<
        "       0x0020 = INTERNAL_CORE_LANE_DUMP"  << endl <<
        "       0x0020 = BCM_PLP_INTERNAL_CORE_LANE_DUMP" << endl <<
        "       0x0040 = INTERNAL_CHIP_DSC_DUMP" << endl <<
        "       0x0080 = INTERNAL_DIAG_REG_DUMP" << endl <<
        "       0x0100 = INTERNAL_EVENT_UC_DUMP" << endl <<
        "       0x0200 = INTERNAL_DUMP_L1" << endl <<
        "       0x0400 = INTERNAL_DUMP_L2" << endl <<
        "       0x0800 = INTERNAL_DUMP_L3" << endl <<
        "       0x1000 = PCS_DIAG_DUMP_L1" << endl <<
        "       0x2000 = PCS_DIAG_DUMP_L2" << endl;
*/

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->chipStatus(param, func))
    {
        out << "chipStatus successful" << endl;
        out << "Check cout logs" << endl;
    }
    else
    {
        out << "chipStatus failed" << endl;
    }
}

void cmdClrInterrupt(ostream& out, const Bcm81725Lane& param)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->clrInterrupt(param))
    {
        out << "clrInterrupt successful" << endl;
    }
    else
    {
        out << "clrInterrupt failed" << endl;
    }
}

void cmdGetInterrupt(ostream& out, const Bcm81725Lane& param, unsigned int typeMask)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int status;
    string str[] = { "Chip MASK  INTR_TYPE_CHIP", "Port MASK  INTR_TYPE_PORT", "INTR Enab. INTR_TYPE_INTR_PIN",
                     "Chip Level INTR_TYPE_AVS_FAIL", "Chip Level INTR_TYPE_FW_FAIL", "Chip Level INTR_TYPE_MSG_OUT",
                     "Chip Level INTR_TYPE_FW_SER", "Chip Level INTR_TYPE_FW_DED", "INTR_TYPE_IN_LOS_CDR_LOL",
                     "Port Level INTR_TYPE_IN_FAULT", "Port Level INTR_TYPE_IN_LINK_UP", "Port Level INTR_TYPE_IN_LINK_DOWN",
                     "Host Level INTR_TYPE_EG_LOS_CDR_LOL", "Host Level INTR_TYPE_EG_FAULT", "Host Level INTR_TYPE_EG_LINK_UP",
                     "Host Level INTR_TYPE_EG_LINK_DOWN" };

    unsigned int masks[]  = {0x00003F, 0x0F0F00, 0x000020, 0x000001, 0x000002, 0x000004, 0x000008, 0x000010, 0x000100, 0x000200,
                             0x000400, 0x000800, 0x010000, 0x020000, 0x040000, 0x080000 };
    const unsigned int cNumMasks = sizeof(masks) / sizeof(masks[0]);
    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (typeMask == 0)
    {
        // List all interrupts
        out << "typemasks key:" << endl <<
                        "   0x00003F Chip MASK  INTR_TYPE_CHIP" << endl <<
                        "   0x0F0F00 Port MASK  INTR_TYPE_PORT" << endl <<
                        "   0x000020 INTR Enab. INTR_TYPE_INTR_PIN" << endl <<
                        "   0x000001 Chip Level INTR_TYPE_AVS_FAIL" << endl <<
                        "   0x000002 Chip Level INTR_TYPE_FW_FAIL" << endl <<
                        "   0x000004 Chip Level INTR_TYPE_MSG_OUT" << endl <<
                        "   0x000008 Chip Level INTR_TYPE_FW_SER" << endl <<
                        "   0x000010 Chip Level INTR_TYPE_FW_DED" << endl <<
                        "   0x000100 Port Level INTR_TYPE_IN_LOS_CDR_LOL" << endl <<
                        "   0x000200 Port Level INTR_TYPE_IN_FAULT" << endl <<
                        "   0x000400 Port Level INTR_TYPE_IN_LINK_UP" << endl <<
                        "   0x000800 Port Level INTR_TYPE_IN_LINK_DOWN" << endl <<
                        "   0x010000 Host Level INTR_TYPE_EG_LOS_CDR_LOL" << endl <<
                        "   0x020000 Host Level INTR_TYPE_EG_FAULT" << endl <<
                        "   0x040000 Host Level INTR_TYPE_EG_LINK_UP" << endl <<
                        "   0x080000 Host Level INTR_TYPE_EG_LINK_DOWN" << endl;
    }
    else
    {

        if (!adapter->getInterrupt(param, typeMask, &status))
        {
            out << "getInterrupt successful" << endl;

            out << "  Mask (hex)       Desc" << endl;
            for (unsigned int i = 0; i < cNumMasks; i++)
            {

                if (status & masks[i])
                {
                    out << setw(8) << std::hex << masks[i] << std::dec << " : " << str[i] << endl;
                }
            }
            out << endl << "    status=0x" << std::hex << status << std::dec << endl;


        }
        else
        {
            out << "getInterrupt failed" << endl;
        }
    }
}

void cmdSetInterrupt(ostream& out, const Bcm81725Lane& param, unsigned int typeMask, bool enable)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int *status = NULL;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setInterrupt(param, typeMask, enable))
    {
        out << "setInterrupt successful" << endl;
    }
    else
    {
        out << "setInterrupt failed" << endl;
    }
}

void cmdGetPcsStatus(ostream& out, unsigned int port, Bcm81725DataRate_t rate)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->dumpPcsStatus(port, rate, status))
    {
        out << "get PCS status successful" << endl;

    }
    else
    {
        out << "get PCS status failed" << endl;
    }
    out << status;
}

void cmdGetFecStatus(ostream& out, const Bcm81725Lane& param, bool clear)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;
    ostringstream fecDump;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->printFecStatus(param, clear, fecDump))
    {
        out << "dumpFecStatus successful" << endl;
    }
    else
    {
        out << "dumpFecStatus failed" << endl;
    }
    out << fecDump.str();
}

void cmdGetFaults(ostream& out, unsigned int port)
{
    DpAdapter::GigeFault faults;
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    string aid = "1-5-T" + to_string(port);

    if (!adapter->getBcmFaults(aid, &faults))
    {
        out << setw(8) << "facBmp" << " : "  << "0x" << std::hex << faults.facBmp << std::dec << endl;
        for (unsigned int i = 0; i < faults.fac.size(); i++)
        {
            out << setw(8) << faults.fac[i].faultName << " : " << faults.fac[i].faulted
                << " Direction : " << faults.fac[i].direction << " Location : " << faults.fac[i].location << endl;
        }
    }
    else
    {
        out << "getBcmFaults failed" << endl;
    }
}

void cmdGetPmCounts(ostream & out, unsigned int port, Bcm81725DataRate_t rate, unsigned int lane, bool clear)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;
    std::string counts;


    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->printPmCounts(port, rate, lane, counts, clear))
    {
        out << "PM count was successful." << endl;
    }
    else
    {
        out << "PM count failed." << endl;
    }
    out << counts;
}

void cmdSetRxSquelch(ostream& out, const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rxControl)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int *status = NULL;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setRxLaneControl(param, rxControl))
    {
        out << "cmdSetRxSquelch=" << (int)rxControl << " successful" << endl;
    }
    else
    {
        out << "cmdSetRxSquelch failed" << endl;
    }
}



void cmdGetRxSquelch(ostream& out, unsigned port, unsigned int side, Bcm81725DataRate_t rate)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int *status = NULL;


    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);
    Bcm81725Lane param;

    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        if (side == cHostSide)
        {
            param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};
        }
        else
        {
            param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};
        }
        bcm_plp_pm_phy_rx_lane_control_t rxControl;



        if (!adapter)
        {
            out << "gearbox init failed" << endl;
            return;
        }

        if (!adapter->getRxLaneControl(param, rxControl))
        {
            out << "RxControl=" << (int)rxControl << "-> ";

            switch (rxControl)
            {
            case bcmpmRxReset:
                out << "RxReset" << endl;
                break;
            case bcmpmRxSquelchOn:
                out << "Squelch On" << endl;
                break;
            case bcmpmRxSquelchOff:
                out << "Rx Squelch Off" << endl;
                break;
            default:
                out << "Rx Type Unknown";
                break;
            }
        }
        else
        {
            out << "cmdGetRxSquelch failed" << endl;
        }
    }
}


void cmdSetTxSquelch(ostream& out, const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t txControl)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int *status = NULL;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setTxLaneControl(param, txControl))
    {
        out << "cmdSetTxSquelch=" << (int)txControl << " successful" << endl;
    }
    else
    {
        out << "cmdSetTxSquelch failed" << endl;
    }
}

void cmdGetTxSquelch(ostream& out, unsigned port, unsigned int side, Bcm81725DataRate_t rate)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int *status = NULL;


    int rc = 0;
    std::vector<PortConfig> portConfig;
    GetPortConfigEntry(portConfig, port, rate);
    Bcm81725Lane param;

    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        if (side == cHostSide)
        {
            param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, portConfig[i].host.laneMap};
        }
        else
        {
            param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, portConfig[i].line.laneMap};
        }

        bcm_plp_pm_phy_tx_lane_control_t txControl;




        if (!adapter)
        {
            out << "gearbox init failed" << endl;
            return;
        }

        if (!adapter->getTxLaneControl(param, txControl))
        {
            out << "TxControl=" << (int)txControl << "-> ";

            switch (txControl)
            {
            case bcmpmTxReset:
                out << "TxReset" << endl;
                break;
            case bcmpmTxSquelchOn:
                out << "Tx Squelch On" << endl;
                break;
            case bcmpmTxSquelchOff:
                out << "Tx Squelch Off" << endl;
                break;
            default:
                out << "Rx Type Unknown";
                break;
            }
        }
        else
        {
            out << "cmdGetTxSquelch failed" << endl;
        }
    }
}

void cmdIwrite(ostream& out, const Bcm81725Lane& param, unsigned int regAddr, unsigned int regData)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int *status = NULL;


    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->Write(param, regAddr, regData))
    {
        out << "Write successful regAddr=" << regAddr << " regData=" << regData << endl;
    }
    else
    {
        out << "Write failed regAddr=" << regAddr << " regData=" << regData << endl;
    }
}

void cmdIread(ostream& out, const Bcm81725Lane& param, unsigned int regAddr)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    unsigned int *status = NULL;
    unsigned int regData = 0;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->Read(param, regAddr, regData))
    {
        out << "Read successful regAddr=" << regAddr << " regData=" << regData << endl;
    }
    else
    {
        out << "Read failed regAddr=" << regAddr << endl;
    }
}

void cmdDumpPam4Diag(ostream& out, unsigned int port)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    Bcm81725DataRate_t rate = RATE_100GE;

    int rc = adapter->getCachedPortRate(port, rate);

    if (rc == 0)
    {
        if (!adapter->dumpPam4Diag(port, rate, status))
        {
            out << "dumpPam4Diag successful" << endl;
            out << status;
        }
        else
        {
            out << "dumpPam4Diag failed" << endl;
        }
    }
}



void cmdDumpPam4DiagAll(ostream& out)
{

    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }


    for (unsigned int port = PORT1; port <= NUM_PORTS; port++)
    {
        if (adapter->isPortConfigured(port) == true)
        {
            out << "===Port " << port << "===" << endl;
            cmdDumpPam4Diag(out, port);
            out << endl << endl;
        }
    }
}

void cmdSetFailOver(ostream& out, const Bcm81725Lane& param, unsigned int mode)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->setFailOverPrbs(param, mode))
    {
        out << "SetFailOver successful set mode=" << mode << endl;
    }
    else
    {
        out << "SetFailOver failed" << endl;
    }
}

void cmdGetFailOver(ostream& out, const Bcm81725Lane& param)
{
    GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    std::string status;
    unsigned int mode = 0;
    string str;

    if (!adapter)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if (!adapter->getFailOverPrbs(param, mode))
    {
        out << "GetFailOver successful" << endl;
        str = (mode == 0) ? "Squelch" : "PRBS";
        out << "mode=" << str << endl;
    }
    else
    {
        out << "SetFailOver failed" << endl;
    }
}

void cmdAvsTpsReadData(ostream& out, uint32 mezzBrdIdx, uint32 bcmIdx, uint32 opCode, uint32 numBytes)
{
    out << "=== AVS TPS Read Data ===" << std::endl << std::endl;

    std::ios_base::fmtflags flags(out.flags());

    if (mezzBrdIdx >= NUM_MEZZ_BRDS)
    {
        out << "Error: mezzIdx out of range. Idx: " << mezzBrdIdx << std::endl;
        return;
    }

    if (bcmIdx >= NUM_BCMS_PER_MEZZ)
    {
        out << "Error: bcmIdx out of range. Idx: " << bcmIdx << std::endl;
        return;
    }

    GearBoxAdIf* pAd = GearBoxAdapterSingleton::Instance();

    if (!pAd)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    if ((numBytes == 0) || (numBytes > TPS_MAX_RW_BYTES))
    {
        out << "Error: numBytes out of range 0 to 4. numBytes: " << numBytes << std::endl;
        return;
    }

    out << "MezzIdx: " << mezzBrdIdx << " BcmIdx: " << bcmIdx << " NumBytes: " << numBytes << std::endl;

    uint8 data[TPS_MAX_RW_BYTES];
    std::memset(data, 0, sizeof(data));

    int retVal = pAd->getAvsTpsData(mezzBrdIdx, bcmIdx, opCode, numBytes, data);

    if (retVal < 0)
    {
        out << "Failed to read from device. RetVal: " << retVal;
        return;
    }

    out << "Data = 0x";
    uint32 i = numBytes - 1;
    do
    {
        out << std::hex << std::setfill('0') << std::setw(2) << (uint32)data[i];
    } while (i--);

    out << std::endl << std::endl;

    out.flags(flags);
}

void cmdAvsTpsWriteData(ostream& out, uint32 mezzBrdIdx, uint32 bcmIdx, uint32 opCode, uint32 numBytes, uint32 data)
{
    out << "=== AVS TPS Write Data ===" << std::endl << std::endl;

    if (mezzBrdIdx >= NUM_MEZZ_BRDS)
    {
        out << "Error: mezzIdx out of range. Idx: " << mezzBrdIdx << std::endl;
        return;
    }

    if (bcmIdx >= NUM_BCMS_PER_MEZZ)
    {
        out << "Error: bcmIdx out of range. Idx: " << bcmIdx << std::endl;
        return;
    }

    if ((numBytes == 0) || (numBytes > TPS_MAX_RW_BYTES))
    {
        out << "Error: numBytes out of range 0 to 4. numBytes: " << numBytes << std::endl;
        return;
    }

    GearBoxAdIf* pAd = GearBoxAdapterSingleton::Instance();

    if (!pAd)
    {
        out << "gearbox init failed" << endl;
        return;
    }

    std::ios_base::fmtflags flags(out.flags());

    out << "MezzIdx: " << mezzBrdIdx << " BcmIdx: " << bcmIdx << "NumBytes: " << numBytes << " Writing Data: 0x" << std::hex << data << std::endl << std::endl;

    int retVal = pAd->setAvsTpsData(mezzBrdIdx, bcmIdx, opCode, numBytes, (uint8*)&data);

    if (retVal < 0)
    {
        out << "Failed to write to device. RetVal: " << retVal;
    }

    out.flags(flags);
}

void cmdAvsTpsDump(ostream& out)
{
    out << "=== AVS TPS Read Data ===" << std::endl << std::endl;

    for(uint32 mezzIdx = 0; mezzIdx < NUM_MEZZ_BRDS; mezzIdx++)
    {
        for(uint32 bcmIdx = 0; bcmIdx < NUM_BCMS_PER_MEZZ; bcmIdx++)
        {
            cmdAvsTpsReadData(out, mezzIdx, bcmIdx, TPS_STATUS_WORD_ADDR, 2);
        }
    }
}

void cmdDumpBcmAll(ostream& out)
{
    auto startTime = std::chrono::steady_clock::now();

    INFN_LOG(SeverityLevel::info) << "";

    out << "===bcm81725_adapter===" << endl;
    out << "GetBcmVers" << endl;
    cmdGetBcmInfo(out);

    out << "DumpBcmConfigAll" << endl;
    cmdDumpBcmConfigAll(out);

    out << "getPam4Diag" << endl;
    cmdDumpPam4DiagAll(out);

    out << "DumpAvsTps" << endl;
    cmdAvsTpsDump(out);

    auto endTime = std::chrono::steady_clock::now();

    std::chrono::seconds elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    out << "**** Gearbox BCM Cli Collection Time Duration: " << elapsedSec.count() << " seconds" << endl;

    INFN_LOG(SeverityLevel::info) << "Gearbox BCM  Cli Collection Time Duration: " << elapsedSec.count() << " seconds";
}





