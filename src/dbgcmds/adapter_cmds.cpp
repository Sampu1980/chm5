#include <cli/clilocalsession.h> // include boost asio
#include <cli/remotecli.h>
// TODO. NB: remotecli.h and clilocalsession.h both includes boost asio,
// so in Windows it should appear before cli.h that include rang
// (consider to provide a global header file for the library)
#include <cli/cli.h>
#include <boost/function.hpp>
#include <chrono>
#include "dp_common.h"
#include "DcoCardAdapter.h"
#include "DcoGigeClientAdapter.h"
#include "DcoCarrierAdapter.h"
#include "DcoOtuAdapter.h"
#include "DcoOduAdapter.h"
#include "DcoXconAdapter.h"
#include "DcoGccControlAdapter.h"

#include "DataPlaneMgr.h"
#include "DpCarrierMgr.h"
#include "DpGigeClientMgr.h"
#include "DpOtuMgr.h"
#include "DpOduMgr.h"
#include "DpXcMgr.h"
#include "DpCardMgr.h"
#include "DpGccMgr.h"
#include "DcoConnectHandler.h"
#include "DpPeerDiscoveryMgr.h"

using namespace cli;
using namespace std;
using namespace boost;
using namespace DpAdapter;


////////////////////////////////////////////////////////////
//
//
// Card Adapter commands
//
void cmdCreateCard (ostream& out)
{
    bool status =  DpCardMgrSingleton::Instance()->getAdPtr()->createCard();
    if (status == false )
        out << " FAIL Create DCO Card Adapter " << endl;
}

void cmdDeleteCard (ostream& out)
{
    bool status =  DpCardMgrSingleton::Instance()->getAdPtr()->deleteCard();
    if (status == false )
        out << " FAIL Delete DCO Card Adapter " << endl;
}

void cmdCardSetClientMode (ostream& out, uint32_t mode)
{
    ClientMode cmode;

    switch (mode)
    {
        case 1:
            cmode = CLIENTMODE_LXTP_E;
            break;
        case 2:
            cmode = CLIENTMODE_LXTP_M;
            break;
        case 3:
            cmode = CLIENTMODE_LXTP;
            break;
        default:
            out << " FAIL not supported Client Mode: " << mode << endl;
            return;
    }

    bool status = DpCardMgrSingleton::Instance()->getAdPtr()->setClientMode(cmode);
    if (status == false )
        out << " FAIL to set Card Client Mode " << endl;
}

void cmdCardSetMaxPktLen (ostream& out, uint32_t pktLen)
{
    if (pktLen < 1518 || pktLen > 18000)
    {
        out << " FAIL packet len 1518..18000 : " << pktLen << endl;

    }

    bool status = DpCardMgrSingleton::Instance()->getAdPtr()->setClientMaxPacketLength(pktLen);
    if (status == false )
        out << " FAIL to set Max Packet Length " << endl;
}

void cmdCardEnable (ostream& out, uint32_t enable)
{
    if (enable > 1)
    {
        out << " FAIL enable = 1 or 0 " << endl;
        return;
    }

    INFN_LOG(SeverityLevel::info) << "Enable: " << enable;

    bool status =  DpCardMgrSingleton::Instance()->getAdPtr()->setCardEnabled((bool)enable);
    if (status == false )
        out << " FAIL to set Card Enable " << endl;
}

void cmdCardBoot (ostream& out, uint32_t boot)
{
    if (boot > 2)
    {
        out << " FAIL boot = 2 (shutdown), 1 (cold) or 0 (warm) " << endl;
        return;
    }

    bool status = false;
    if (boot == 1)
        status = DpCardMgrSingleton::Instance()->getAdPtr()->coldBoot();
    else if (boot == 2)
        status = DpCardMgrSingleton::Instance()->getAdPtr()->shutdown();
    else
        status = DpCardMgrSingleton::Instance()->getAdPtr()->warmBoot();
    if (status == false )
        out << " FAIL to set Card Boot " << endl;
}

void cmdCardConfigInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpCardMgrSingleton::Instance()->getAdPtr()->cardConfigInfo(out);
    if (status == false )
        out << " FAIL to Get Card config Info " << endl;
}

void cmdCardStatusInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpCardMgrSingleton::Instance()->getAdPtr()->cardStatusInfo(out, false);
    if (status == false )
        out << " FAIL to Get Card Status Info " << endl;
}

void cmdCardStatusInfoForce (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrBoardInitDone(out)) return;

    bool status = false;
    status = DpCardMgrSingleton::Instance()->getAdPtr()->cardStatusInfo(out, true);
    if (status == false )
        out << " FAIL to Get Card Status Info " << endl;
}

void cmdCardCapabilityInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpCardMgrSingleton::Instance()->getAdPtr()->cardCapabilityInfo(out);
    if (status == false )
        out << " FAIL to Get Card Capabitites Info " << endl;
}

void cmdCardFaultInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpCardMgrSingleton::Instance()->getAdPtr()->cardFaultInfo(out);
    if (status == false )
        out << " FAIL to Get Card Fault Info " << endl;
}

void cmdCardFaultCapaInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpCardMgrSingleton::Instance()->getAdPtr()->cardFaultCapaInfo(out);
    if (status == false )
        out << " FAIL to Get Card Fault Capability Info " << endl;
}

void cmdCardPmInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpCardMgrSingleton::Instance()->getAdPtr()->cardPmInfo(out);
    if (status == false )
        out << " FAIL to Get Card Pm Info " << endl;
}

void cmdCardIsSimEnable (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = DpCardMgrSingleton::Instance()->getAdPtr()->isSimModeEn();

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdCardSetSimEnable (ostream& out, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = enable;

    DpCardMgrSingleton::Instance()->getAdPtr()->setSimModeEn(isEn);

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdCardNotifyState(ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    DpCardMgrSingleton::Instance()->getAdPtr()->cardNotifyState(out);
}

//
// Gige Client Adapter commands
//
void cmdGigeCreate (ostream& out, uint32_t port, uint32_t rate, uint32_t fecEnable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }
    if ( rate != 100 && rate != 400)
    {
        out << " FAIL rate <100 or 400>: " << rate << endl;
        return;
    }
    if ( fecEnable > 1 )
    {
        out << " FAIL enable <1 or 0>: " << fecEnable << endl;
        return;
    }

    GigeCommon cfg;
    memset(&cfg, 0, sizeof(cfg));

    string aid = "Port ";
    aid += std::to_string(port);
    out << " Aid: " << aid << endl;

    cfg.rate = PORT_RATE_ETH100G;
    if (rate == 400)
    {
        cfg.rate = PORT_RATE_ETH400G;
        cfg.fecDegSer.activateThreshold = 0.0000001;
        cfg.fecDegSer.deactivateThreshold = 0.00000001;
        cfg.fecDegSer.enable = false;
        cfg.fecDegSer.monitorPeriod = 10;
    }

    cfg.fecEnable = (bool) fecEnable;
    // cfg.mtu = MAX_MTU_SIZE;
    cfg.mtu = 10000;  // Set to 10K for default
    cfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsTx = MAINTENANCE_SIGNAL_IDLE;
    cfg.autoMsRx = MAINTENANCE_SIGNAL_IDLE;
   //  cfg.autoMsTx = MAINTENANCE_SIGNAL_LF;
    // cfg.autoMsRx = MAINTENANCE_SIGNAL_LF;
    cfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;

    cfg.lpbk = LOOP_BACK_TYPE_OFF;

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->createGigeClient(aid, &cfg);
    if (status == false )
        out << " FAIL to Set Port Rate" << endl;
}

void cmdGigeDelete (ostream& out, uint32_t port)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }
    string aid = "Port ";
    aid += std::to_string(port);
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->deleteGigeClient(aid);
    if (status == false )
        out << " FAIL to Delete Client" << endl;
}

void cmdGigeSetFecEnable (ostream& out, uint32_t port, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }
    if ( enable > 1)
    {
        out << " FAIL enable <1 or 0>: " << enable << endl;
        return;
    }

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->setGigeFecEnable(aid, (bool) enable);
    if (status == false )
    out << " FAIL to set FEC " << endl;
}

void cmdGigeSetLldpDrop(ostream& out, uint32_t port, uint32_t dir, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }
    if ( enable > 1)
    {
        out << " FAIL enable <1 or 0>: " << enable << endl;
        return;
    }
    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << enable << endl;
        return;
    }
    Direction direction;

    if (dir == 1 )
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->setGigeLldpDrop(aid, direction, (bool) enable);
    if (status == false )
    out << " FAIL to set LLDP Drop " << endl;
}

void cmdGigeSetLldpSnoop(ostream& out, uint32_t port, uint32_t dir, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }
    if ( enable > 1)
    {
        out << " FAIL enable <1 or 0>: " << enable << endl;
        return;
    }
    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << enable << endl;
        return;
    }
    Direction direction;

    if (dir == 1 )
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->setGigeLldpSnoop(aid, direction, (bool) enable);
    if (status == false )
    out << " FAIL to set LLDP Snoop " << endl;
}

void cmdGigeSetLoopBack (ostream& out, uint32_t port, uint32_t lpbk)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }
    if ( lpbk > 2 )
    {
        out << " FAIL loopback <off=0 or fac=1 or term=2>: " << lpbk << endl;
        return;
    }

    LoopBackType mode;
    if (lpbk == 2)
        mode = LOOP_BACK_TYPE_TERMINAL;
    else if (lpbk == 1)
        mode = LOOP_BACK_TYPE_FACILITY;
    else
        mode = LOOP_BACK_TYPE_OFF;

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->setGigeLoopBack(aid, mode);
    if (status == false )
    out << " FAIL to set LoopBack " << endl;
}

void cmdGigeSetMtu (ostream& out, uint32_t port, uint32_t mtu)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->setGigeMtu(aid, mtu);
    if (status == false )
    out << " FAIL to set Mtu " << endl;
}

void cmdGigeSetFwdTdaTrigger (ostream& out, uint32_t port, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }
    if ( enable > 1)
    {
        out << " FAIL enable <1 or 0>: " << enable << endl;
        return;
    }

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->setGigeFwdTdaTrigger(aid, (bool) enable);
    if (status == false )
    out << " FAIL to set FWD TDA trigger " << endl;
}

void cmdGigeConfigInfo (ostream& out, uint32_t port)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigeConfigInfo(out, aid);
    if (status == false )
        out << " FAIL to Get GigeClient config Info " << endl;
}

void cmdGigeConfigInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigeConfigInfo(out);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGigeStatusInfo (ostream& out, uint32_t port)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port < 1 || port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    string aid = "Port ";
    aid += std::to_string(port);

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigeStatusInfo(out, aid);
    if (status == false )
        out << " FAIL to Get GigeClient Status Info " << endl;
}

void cmdGigeFaultInfo (ostream& out, uint32_t port)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port < 1 || port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    string aid = "Port ";
    aid += std::to_string(port);
    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigeFaultInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Gige Fault Info " << endl;
}

void cmdGigeIsSimEnable (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = DpGigeClientMgrSingleton::Instance()->getAdPtr()->isSimModeEn();

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdGigeSetSimEnable (ostream& out, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = enable;

    DpGigeClientMgrSingleton::Instance()->getAdPtr()->setSimModeEn(isEn);

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdGetGigePmInfo (ostream& out, uint32_t port)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port < 1 || port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigePmInfo(out, port);
    if ( status == false )
        out << " FAIL to Get Pm info, pls check gige object existence " << endl;
}

void cmdGetGigePmAccumInfo (ostream& out, uint32_t port)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port < 1 || port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigePmAccumInfo(out, port);
    if ( status == false )
        out << " FAIL to Get Pm Accumulated info, pls check gige object existence " << endl;
}

void cmdClearGigePmAccum (ostream& out, uint32_t port)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port < 1 || port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigePmAccumClear(out, port);
    if ( status == false )
        out << " FAIL to Clear Pm Accumulated data, pls check gige object existence " << endl;
}

void cmdEnableGigePmAccum (ostream& out, uint32_t port, bool enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (port < 1 || port > 16)
    {
        out << " FAIL port <1-16>: " << port << endl;
        return;
    }

    bool status = false;
    status = DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigePmAccumEnable(out, port, enable);
    if ( status == false )
        out << " FAIL to Enable Pm Accumulated data, pls check gige object existence " << endl;
}

void cmdDumpGigeAdapterAll(ostream& out)
{
    out << "===gige_adapter===" << endl;

    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out))
    {
        out << "Dp Mgr Init is NOT DONE YET. Aborting" << endl;

        return;
    }

    DpGigeClientMgrSingleton::Instance()->getAdPtr()->dumpAll(out);
}

//
// Carrier Adapter commands
//
void cmdCarrierCreate (ostream& out, uint32_t carrier, uint32_t capa, double baud, string appId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    CarrierCommon cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.capacity = capa;
    cfg.baudRate = baud;
    cfg.appCode = appId;

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->createCarrier(aid, &cfg);
    if (status == false )
        out << " FAIL to Create Carrier" << endl;

    out << "*** DONE Create Carrier" << endl;
}

void cmdCarrierDelete (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }
    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->deleteCarrier(aid);
    if (status == false )
        out << " FAIL to Delete Carrier" << endl;
}

void cmdCarrierCapacity (ostream& out, uint32_t carrier, uint32_t rate)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    if (rate < 100 || rate > 800)
    {
        out << " FAIL rate <100-800>: " << rate << endl;
        return;
    }
    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierCapacity(aid, rate);
    if (status == false )
        out << " FAIL to set capacity Carrier" << endl;
}

void cmdCarrierBaud (ostream& out, uint32_t carrier, double rate)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierBaudRate(aid, rate);
    if (status == false )
        out << " FAIL to set baudRate " << endl;
}

void cmdCarrierAppId (ostream& out, uint32_t carrier, string appCode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierAppCode(aid, appCode);
    if (status == false )
        out << " FAIL to set appCode " << endl;
}

void cmdCarrierEnable (ostream& out, uint32_t carrier, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierEnable(aid, (bool) enable);
    if (status == false )
        out << " FAIL to set enable " << endl;
}

void cmdCarrierFrequency (ostream& out, uint32_t carrier, double frequency)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierFrequency(aid, frequency);
    if (status == false )
        out << " FAIL to set frequency " << endl;
}

void cmdCarrierOffSet (ostream& out, uint32_t carrier, double offset)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierFreqOffset(aid, offset);
    if (status == false )
        out << " FAIL to set frequency offset" << endl;
}

void cmdCarrierPower (ostream& out, uint32_t carrier, double power)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierTargetPower(aid, power);
    if (status == false )
        out << " FAIL to set Target Power " << endl;
}

void cmdCarrierTxCd (ostream& out, uint32_t carrier, double txCd)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierTxCdPreComp(aid, txCd);
    if (status == false )
        out << " FAIL to set Tx CD Pre-Comp " << endl;
}

void cmdCarrierAdvParm (ostream& out, uint32_t carrier, string name, string value)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;

    AdvancedParameter parm;
    parm.apName = name;
    parm.apValue = value;
    std::vector<AdvancedParameter> ap;
    ap.push_back(parm);
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierAdvParam(aid, ap);
    if (status == false )
        out << " FAIL to set Advance Parameter " << endl;
}

void cmdDelCarrierAdvParm(ostream& out, uint32_t carrier, std::string key)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;

    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->deleteCarrierAdvParam(aid, key);
    if (status == false )
        out << " FAIL to delete Advance Parameter " << endl;
}

void cmdDelCarrierAdvParmAll(ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;

    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->deleteCarrierAdvParamAll(aid);
    if (status == false )
        out << " FAIL to delete Advance Parameter " << endl;
}

void cmdCarrierDgdOorhTh (ostream& out, uint32_t carrier, double thresh)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierDgdOorhThresh(aid, thresh);
    if (status == false )
        out << " FAIL to set DGD OORH Threshold " << endl;
}

void cmdCarrierPostFecSdTh (ostream& out, uint32_t carrier, double thresh)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierPostFecQSdThresh(aid, thresh);
    if (status == false )
        out << " FAIL to set Post FEC SD Threshold " << endl;
}

void cmdCarrierPostFecSdHyst (ostream& out, uint32_t carrier, double hysteresis)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierPostFecQSdHyst(aid, hysteresis);
    if (status == false )
        out << " FAIL to set Post FEC SD Hysteresis " << endl;
}

void cmdCarrierPreFecSdTh (ostream& out, uint32_t carrier, double thresh)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierPreFecQSdThresh(aid, thresh);
    if (status == false )
        out << " FAIL to set Pre FEC SD Threshold " << endl;
}

void cmdCarrierPreFecSdHyst (ostream& out, uint32_t carrier, double hysteresis)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->setCarrierPreFecQSdHyst(aid, hysteresis);
    if (status == false )
        out << " FAIL to set Pre FEC SD Hysteresis " << endl;
}

void cmdGetCarrierConfigInfo (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierConfigInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetCarrierConfigInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierConfigInfo(out);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetCarrierStatusInfo (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierStatusInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Status info " << endl;
}

void cmdGetCarrierFaultInfo (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }
    string aid = "Carrier ";
    aid += std::to_string(carrier);
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierFaultInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Carrier Fault Info " << endl;
}

void cmdGetCarrierPmInfo (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierPmInfo(out, carrier);
    if (status == false )
        out << " FAIL to Get Pm info, pls check carrier object existence" << endl;
}

void cmdCarrierNotifyState (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }
    DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierNotifyState(out, carrier);
}

void cmdGetCarrierPmAccumInfo (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierPmAccumInfo(out, carrier);
    if ( status == false )
        out << " FAIL to Get Pm Accumulated info, pls check carrier object existence " << endl;
}

void cmdClearCarrierPmAccum (ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierPmAccumClear(out, carrier);
    if ( status == false )
        out << " FAIL to Clear Pm Accumulated data, pls check carrier object existence " << endl;
}

void cmdEnableCarrierPmAccum (ostream& out, uint32_t carrier, bool enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    bool status = false;
    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierPmAccumEnable(out, carrier, enable);
    if ( status == false )
        out << " FAIL to Enable Pm Accumulated data, pls check carrier object existence " << endl;
}

void cmdCarrierIsSimEnable (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = DpCarrierMgrSingleton::Instance()->getAdPtr()->isSimModeEn();

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdCarrierSetSimEnable (ostream& out, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = enable;

    DpCarrierMgrSingleton::Instance()->getAdPtr()->setSimModeEn(isEn);

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdDeleteCarrierBaudRate(ostream& out, uint32_t carrier)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL Carrier <1-2>: " << carrier << endl;
        return;
    }

    string aid = "Carrier ";
    aid += std::to_string(carrier);

    bool status = false;

    status = DpCarrierMgrSingleton::Instance()->getAdPtr()->deleteCarrierBaudRate(aid);
    if (status == false )
        out << " FAIL to delete Baud Rate " << endl;
}

void cmdDumpCarrierAdapterAll(ostream& out)
{
    out << "===carrier_adapter===" << endl;
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out))
    {
        out << "Dp Mgr Init is NOT DONE YET. Aborting" << endl;

        return;
    }

    DpCarrierMgrSingleton::Instance()->getAdPtr()->dumpAll(out);
}

//
// OTU Adapter commands
//
void cmdOtuCreate (ostream& out, uint32_t otuId, uint32_t payLoad, uint32_t channel)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }
    if (payLoad < 1 || payLoad > 2)
    {
        out << " FAIL payLoad <1=OTUCni or 2=OTU4>: " << payLoad << endl;
        return;
    }
    if ( channel < 1 || channel > 3)
    {
        out << " FAIL channel <1 or 2 or 3=both>: " << channel << endl;
        return;
    }

    OtuCommon cfg;
    memset(&cfg, 0, sizeof(cfg));

    string aid = "Otu ";
    aid += std::to_string(otuId);
    out << " Aid: " << aid << endl;

    switch (payLoad)
    {
        case 1:
            cfg.payLoad = OTUPAYLOAD_OTUCNI;
            cfg.type    = OTUSUBTYPE_LINE;
            cfg.srvMode = SERVICEMODE_NONE;
            break;
        case 2:
            cfg.payLoad = OTUPAYLOAD_OTU4;
            cfg.type    = OTUSUBTYPE_CLIENT;
            cfg.srvMode = SERVICEMODE_ADAPTATION;
            break;
    }

    if (channel == 1)
        cfg.channel = CARRIERCHANNEL_ONE;
    else if (channel == 2)
        cfg.channel = CARRIERCHANNEL_TWO;
    else
        cfg.channel = CARRIERCHANNEL_BOTH;

    cfg.lpbk = LOOP_BACK_TYPE_OFF;
    cfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    // Set defaul TTI;
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        cfg.ttiTx[i] = i;
    }
    cfg.transactionId = getTimeNanos();
    out << " OTU transId: 0x" << hex << cfg.transactionId << dec << endl;

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->createOtu(aid, &cfg);
    if (status == false )
        out << " FAIL to create OTU" << endl;
}
void cmdOtuDelete (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    string aid = "Otu ";
    aid += std::to_string(otuId);
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->deleteOtu(aid);
    if (status == false )
        out << " FAIL to delete OTU" << endl;
}

void cmdOtuSetLoopBack (ostream& out, uint32_t otuId, uint32_t lpbk)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    if ( lpbk > 2 )
    {
        out << " FAIL loopback <off=0 or fac=1 or term=2>: " << lpbk << endl;
        return;
    }

    LoopBackType mode;
    if (lpbk == 2)
        mode = LOOP_BACK_TYPE_TERMINAL;
    else if (lpbk == 1)
        mode = LOOP_BACK_TYPE_FACILITY;
    else
        mode = LOOP_BACK_TYPE_OFF;

    string aid = "Otu ";
    aid += std::to_string(otuId);
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->setOtuLoopBack(aid, mode);
    if (status == false )
        out << " FAIL to set OTU Loop Back" << endl;
}

void cmdOtuSetPrbsMon(ostream& out, uint32_t otuId, uint32_t dir, uint32_t prbs)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }
    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }
    if ( prbs > 2 )
    {
        out << " FAIL prbs <none=0 or prbsx31=1 or prbsx31inv=2>: " << prbs << endl;
        return;
    }

    Direction direction;

    if (dir == 1 )
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    PrbsMode mode;
    if (prbs == 0)
        mode = PRBSMODE_PRBS_NONE;
    else if (prbs == 1)
        mode = PRBSMODE_PRBS_X31;
    else
        mode = PRBSMODE_PRBS_X31_INV;

    string aid = "Otu ";
    aid += std::to_string(otuId);

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->setOtuPrbsMon(aid, direction, mode);
    if (status == false )
        out << " FAIL to set OTU PRBS Monitoring " << endl;
}
void cmdOtuSetPrbsGen(ostream& out, uint32_t otuId, uint32_t dir, uint32_t prbs)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }
    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }
    if ( prbs > 2 )
    {
        out << " FAIL prbs <none=0 or prbsx31=1 or prbsx31inv=2>: " << prbs << endl;
        return;
    }

    Direction direction;

    if (dir == 1 )
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    PrbsMode mode;
    if (prbs == 0)
        mode = PRBSMODE_PRBS_NONE;
    else if (prbs == 1)
        mode = PRBSMODE_PRBS_X31;
    else
        mode = PRBSMODE_PRBS_X31_INV;

    string aid = "Otu ";
    aid += std::to_string(otuId);

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->setOtuPrbsGen(aid, direction, mode);
    if (status == false )
        out << " FAIL to set OTU PRBS Generation " << endl;
}

void cmdOtuSetTxTti (ostream& out, uint32_t otuId, string tti)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <1-18>: " << otuId << endl;
        return;
    }

    uint8_t txTti[MAX_TTI_LENGTH + 1];
    memset( txTti, '\0', MAX_TTI_LENGTH + 1 );
    strncpy((char *) txTti,tti.c_str(),MAX_TTI_LENGTH);

    string aid = "Otu ";
    aid += std::to_string(otuId);
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->setOtuTti(aid, txTti);
    if (status == false )
        out << " FAIL to set OTU Tx TTI" << endl;
}

void cmdOtuSetForceMs (ostream& out, uint32_t otuId, uint32_t dir, uint32_t ms)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }
    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }
    if ( ms > 6 )
    {
        out << " FAIL ms <NoReplace=0, LaserOff=1, X31=2, X31INV=3, AIS=4, OCI=5, LCK=6>: " << ms << endl;
        return;
    }

    Direction direction;

    if (dir == 1)
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    MaintenanceSignal mode;
    switch (ms)
    {
        case 0:  mode = MAINTENANCE_SIGNAL_NOREPLACE;    break;
        case 1:  mode = MAINTENANCE_SIGNAL_LASEROFF;     break;
        case 2:  mode = MAINTENANCE_SIGNAL_PRBS_X31;     break;
        case 3:  mode = MAINTENANCE_SIGNAL_PRBS_X31_INV; break;
        case 4:  mode = MAINTENANCE_SIGNAL_AIS;          break;
        case 5:  mode = MAINTENANCE_SIGNAL_OCI;          break;
        case 6:  mode = MAINTENANCE_SIGNAL_LCK;          break;
        default: mode = MAINTENANCE_SIGNAL_NOREPLACE;    break;
    }

    string aid = "Otu ";
    aid += std::to_string(otuId);

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->setOtuForceMS(aid, direction, mode);
    if (status == false )
    {
        out << " FAIL to set OTU Force MS " << endl;
    }
}

// the fault inject bitmap is based the fault capabilities bit position.
void cmdOtuSetFaultStatus (ostream& out, uint32_t otuId, uint32_t dir, string injectFaultBitmap)
{
    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <1-18>: " << otuId << endl;
        return;
    }
    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }

    Direction direction;

    if (dir == 1)
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    uint64_t bitmap = strtol(injectFaultBitmap.c_str(), NULL, 16);

    string aid = "Otu ";
    aid += std::to_string(otuId);

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->setOtuFaultStatus(aid, direction, bitmap);
    if (status == false )
    {
        out << " FAIL to Inject OTU Fault Status  " << endl;
    }
}

void cmdGetOtuConfigInfo (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    string aid = "Otu ";
    aid += std::to_string(otuId);

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuConfigInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetOtuConfigInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuConfigInfo(out);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetOtuStatusInfo (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    string aid = "Otu ";
    aid += std::to_string(otuId);

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuStatusInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Status info " << endl;
}

void cmdOtuIsSimEnable (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = DpOtuMgrSingleton::Instance()->getAdPtr()->isSimModeEn();

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdOtuSetSimEnable (ostream& out, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = enable;

    DpOtuMgrSingleton::Instance()->getAdPtr()->setSimModeEn(isEn);

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdGetOtuPmInfo (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL otuId <1-18>: " << otuId << endl;
        return;
    }

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuPmInfo(out, otuId);
    if (status == false )
        out << " FAIL to Get Pm info, pls check otu object existence " << endl;
}

void cmdGetOtuPmAccumInfo (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuPmAccumInfo(out, otuId);
    if ( status == false )
        out << " FAIL to Get Pm Accumulated info, pls check otu object existence " << endl;
}

void cmdClearOtuPmAccum (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuPmAccumClear(out, otuId);
    if ( status == false )
        out << " FAIL to Clear Pm Accumulated data, pls check otu object existence " << endl;
}

void cmdEnableOtuPmAccum (ostream& out, uint32_t otuId, bool enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuPmAccumEnable(out, otuId, enable);
    if ( status == false )
        out << " FAIL to Enable Pm Accumulated data, pls check otu object existence " << endl;
}

void cmdGetOtuFaultInfo (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    string aid = "Otu ";
    aid += std::to_string(otuId);

    bool status = false;
    status = DpOtuMgrSingleton::Instance()->getAdPtr()->otuFaultInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Fault info " << endl;
}

void cmdOtuNotifyState (ostream& out, uint32_t otuId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    DpOtuMgrSingleton::Instance()->getAdPtr()->otuNotifyState(out, otuId);
}

void cmdDumpOtuAdapterAll(ostream& out)
{
    out << "===otu_adapter===" << endl;
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out))
    {
        out << "Dp Mgr Init is NOT DONE YET. Aborting" << endl;

        return;
    }

    DpOtuMgrSingleton::Instance()->getAdPtr()->dumpAll(out);
}

//
// ODU Adapter commands
//
void cmdOduCreate (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t payLoad, string ts)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }
    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }
    if ( payLoad < 1 || payLoad > 4)
    {
        out << " FAIL payLoad <1=ODU4i or 2=FLEXi or 3=ODUCni or 4=ODU4>: " << payLoad << endl;
        return;
    }
    if (ts.empty())
    {
        out << " FAIL Time slot string <\"0\"=none or \"1 2 .. 15 16\">" << endl;
        return;
    }

    bool has_only_digits = (ts.find_first_not_of( "0123456789 " ) == string::npos);
    if (has_only_digits == false)
    {
        out << " FAIL Time slot string <\"0\"=none or \"1 2 .. 15 16\">" << endl;
        return;
    }

    out << " Time slot: " << ts << endl;

    OduCommon cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.hoOtuId = otuId;
    cfg.loOduId = oduId;  // LoOdu Index

    int tsNum = 0;
    switch (payLoad)
    {
        case 1:
            cfg.payLoad = ODUPAYLOAD_LO_ODU4I;
            cfg.type = ODUSUBTYPE_LINE;
            cfg.srvMode = SERVICEMODE_NONE;
            tsNum = 4;
            break;
        case 2:
            cfg.payLoad = ODUPAYLOAD_LO_FLEXI;
            cfg.type = ODUSUBTYPE_LINE;
            cfg.srvMode = SERVICEMODE_NONE;
            tsNum = 16;  // Supported only 400GE for now
            break;
        case 3:
            cfg.payLoad = ODUPAYLOAD_HO_ODUCNI;
            cfg.type = ODUSUBTYPE_LINE;
            cfg.srvMode = SERVICEMODE_NONE;
            tsNum = 0;
            break;
        case 4:
            cfg.payLoad = ODUPAYLOAD_ODU4;
            cfg.type = ODUSUBTYPE_CLIENT;
            cfg.srvMode = SERVICEMODE_SWITCHING;
            tsNum = 2;
            break;
        default:
            out << " FAIL payLoad <1=ODU4i or 2=FLEXi or 3=ODUCni or 4=ODU4>: " << payLoad << endl;
            return;
    }

    // Sparse the time slot string
    int i = 0;
    if (tsNum != 0)
    {
        std::string delimiter = " ";
        size_t pos = 0;
        std::string token;
        while ((pos = ts.find(delimiter)) != std::string::npos)
        {
            token = ts.substr(0, pos);
	    out << " token: " << token << std::endl;
            cfg.tS.push_back(stoi(token,nullptr));
            ts.erase(0, pos + delimiter.length());
            i++;
        }
        out << " ts: " << ts << std::endl;
        cfg.tS.push_back(stoi(ts,nullptr));
        i++;
        if (i!=4 && i!=16 && i!=2)
        {
            out << "Create Odu: TSstring <\"0\"=none or \"1 2 .. 15 16\">"  << endl;
            out << "Number of time slot must be 2, 4 or 16" << endl;
            return;
        }
    }

    string aid;
    aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;

    cfg.forceMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
    cfg.forceMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    if (cfg.payLoad == ODUPAYLOAD_ODU4)
    {
        cfg.autoMsTx = MAINTENANCE_SIGNAL_AIS;
        cfg.autoMsRx = MAINTENANCE_SIGNAL_AIS;
    }
    else
    {
        cfg.autoMsTx = MAINTENANCE_SIGNAL_NOREPLACE;
        cfg.autoMsRx = MAINTENANCE_SIGNAL_NOREPLACE;
    }
    // Set default TTI
    for (int i = 0; i < MAX_TTI_LENGTH; i++)
    {
        cfg.ttiTx[i] = i;
    }

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->createOdu(aid, &cfg);
    if (status == false )
        out << " FAIL to create ODU" << endl;
}

void cmdOduDelete (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }

    string aid = "Odu ";
    aid += std::to_string(oduId);
    // aid = "1-5-L2-1-ODU4i-81";
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->deleteOdu(aid);
    if (status == false )
        out << " FAIL to delete ODU" << endl;
}

void cmdOduSetLoopBack (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t lpbk)
{
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    if ( lpbk > 2 )
    {
        out << " FAIL loopback <off=0 or fac=1 or term=2>: " << lpbk << endl;
        return;
    }

    LoopBackType mode;
    if (lpbk == 2)
        mode = LOOP_BACK_TYPE_TERMINAL;
    else if (lpbk == 1)
        mode = LOOP_BACK_TYPE_FACILITY;
    else
        mode = LOOP_BACK_TYPE_OFF;

    string aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->setOduLoopBack(aid, mode);
    if (status == false )
        out << " FAIL to set ODU Loop Back" << endl;
}

void cmdOduSetPrbsMon(ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t prbs)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }
    if ( prbs > 2 )
    {
        out << " FAIL prbs <none=0 or prbsx31=1 or prbsx31inv=2>: " << prbs << endl;
        return;
    }

    Direction direction;

    if (dir == 1 )
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    PrbsMode mode;
    if (prbs == 0)
        mode = PRBSMODE_PRBS_NONE;
    else if (prbs == 1)
        mode = PRBSMODE_PRBS_X31;
    else
        mode = PRBSMODE_PRBS_X31_INV;

    string aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->setOduPrbsMon(aid, direction, mode);
    if (status == false )
        out << " FAIL to set ODU PRBS Monitoring " << endl;
}
void cmdOduSetPrbsGen(ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t prbs)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }

    if (otuId < MIN_OTU_ID || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }
    if ( prbs > 2 )
    {
        out << " FAIL prbs <none=0 or prbsx31=1 or prbsx31inv=2>: " << prbs << endl;
        return;
    }

    Direction direction;

    if (dir == 1 )
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    PrbsMode mode;
    if (prbs == 0)
        mode = PRBSMODE_PRBS_NONE;
    else if (prbs == 1)
        mode = PRBSMODE_PRBS_X31;
    else
        mode = PRBSMODE_PRBS_X31_INV;

    string aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->setOduPrbsGen(aid, direction, mode);
    if (status == false )
        out << " FAIL to set ODU PRBS Generation " << endl;
}

void cmdOduSetTS (ostream& out, uint32_t oduId, uint32_t otuId, string ts)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }

    if (otuId < 1 || otuId > MAX_ODU_ID)
    {
        out << " FAIL HO-OTUCni <1-18>: " << otuId << endl;
        return;
    }

    if (ts.empty())
    {
        out << " FAIL Time slot string <\"0\"=none or \"1 2 .. 15 16\">" << endl;
        return;
    }

    bool has_only_digits = (ts.find_first_not_of( "0123456789 " ) == string::npos);
    if (has_only_digits == false)
    {
        out << " FAIL Time slot string <\"0\"=none or \"1 2 .. 15 16\">" << endl;
        return;
    }

    out << " Time slot: " << ts << endl;

    // Sparse the time slot string
    int i = 0;
    std::vector<uint32_t> tS;  // Time slot for this LO-ODUi
    std::string delimiter = " ";
    size_t pos = 0;
    std::string token;
    while ((pos = ts.find(delimiter)) != std::string::npos)
    {
        token = ts.substr(0, pos);
        out << token << std::endl;
        tS.push_back(stoi(token,nullptr));
        ts.erase(0, pos + delimiter.length());
        i++;
    }
    out << ts << std::endl;
    tS.push_back(stoi(ts,nullptr));
    i++;
    if (i!=4 && i!=16 && i!= 2)
    {
        out << "setOduTs: TSstring <\"0\"=none or \"1 2 .. 15 16\">"  << endl;
        out << "Number of time slot must be 2, 4 or 16" << endl;
        return;
    }

    string aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;
    // aid = "1-5-L2-1-ODU4i-81";

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->setOduTimeSlot(aid, tS);
    if (status == false )
    out << " FAIL to change ODU time slot" << endl;
}

void cmdOduSetForceMs (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t ms)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }

    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }
    if ( ms > 6 )
    {
        out << " FAIL ms <NoReplace=0, LaserOff=1, X31=2, X31INV=3, AIS=4, OCI=5, LCK=6>: " << ms << endl;
        return;
    }

    Direction direction;

    if (dir == 1)
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    MaintenanceSignal mode;
    switch (ms)
    {
        case 0:  mode = MAINTENANCE_SIGNAL_NOREPLACE;    break;
        case 1:  mode = MAINTENANCE_SIGNAL_LASEROFF;     break;
        case 2:  mode = MAINTENANCE_SIGNAL_PRBS_X31;     break;
        case 3:  mode = MAINTENANCE_SIGNAL_PRBS_X31_INV; break;
        case 4:  mode = MAINTENANCE_SIGNAL_AIS;          break;
        case 5:  mode = MAINTENANCE_SIGNAL_OCI;          break;
        case 6:  mode = MAINTENANCE_SIGNAL_LCK;          break;
        default: mode = MAINTENANCE_SIGNAL_NOREPLACE;    break;
    }

    string aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;
    // aid = "1-5-L2-1-ODU4i-81";

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->setOduForceMS(aid, direction, mode);
    if (status == false )
    {
        out << " FAIL to set ODU Force MS " << endl;
    }
}

void cmdOduSetAutoMs (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t ms)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <" << MIN_ODU_ID << "-" << MAX_ODU_ID << ">: " << oduId << endl;
        return;
    }

    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <" << MIN_OTU_ID << "-" << MAX_OTU_ID << ">: " << otuId << endl;
        return;
    }

    if ( dir > 1 )
    {
        out << " FAIL direction <tx=1 or rx=0>: " << dir << endl;
        return;
    }
    if ( ms > 6 )
    {
        out << " FAIL ms <NoReplace=0, LaserOff=1, X31=2, X31INV=3, AIS=4, OCI=5, LCK=6>: " << ms << endl;
        return;
    }

    Direction direction;

    if (dir == 1)
        direction = DIRECTION_TX;
    else
        direction = DIRECTION_RX;

    MaintenanceSignal mode;
    switch (ms)
    {
        case 0:  mode = MAINTENANCE_SIGNAL_NOREPLACE;    break;
        case 1:  mode = MAINTENANCE_SIGNAL_LASEROFF;     break;
        case 2:  mode = MAINTENANCE_SIGNAL_PRBS_X31;     break;
        case 3:  mode = MAINTENANCE_SIGNAL_PRBS_X31_INV; break;
        case 4:  mode = MAINTENANCE_SIGNAL_AIS;          break;
        case 5:  mode = MAINTENANCE_SIGNAL_OCI;          break;
        case 6:  mode = MAINTENANCE_SIGNAL_LCK;          break;
        default: mode = MAINTENANCE_SIGNAL_NOREPLACE;    break;
    }

    string aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;
    // aid = "1-5-L2-1-ODU4i-81";

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->setOduAutoMS(aid, direction, mode);
    if (status == false )
    {
        out << " FAIL to set ODU Auto MS " << endl;
    }
}

// the fault inject bitmap is based the fault capabilities bit position.
void cmdOduSetFaultStatus (ostream& out, uint32_t oduId, uint32_t otuId, string injectFaultBitmap)
{
    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL ODU <1-32>: " << oduId << endl;
        return;
    }

    if (otuId < 1 || otuId > MAX_OTU_ID)
    {
        out << " FAIL OTU <1-18>: " << otuId << endl;
        return;
    }

    uint64_t bitmap = strtol(injectFaultBitmap.c_str(), NULL, 16);

    string aid = "Ho-Otu ";
    aid += std::to_string(otuId);
    aid += " Odu ";
    aid += std::to_string(oduId);
    out << " Aid: " << aid << endl;
    // aid = "1-5-L2-1-ODU4i-81";

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->setOduFaultStatus(aid, bitmap);
    if (status == false )
    {
        out << " FAIL to inject ODU RX Fault Status " << endl;
    }
}

void cmdGetOduConfigInfo (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL Otu <1-34>: " << oduId << endl;
        return;
    }

    string aid = "Odu ";
    aid += std::to_string(oduId);

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduConfigInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetOduConfigInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduConfigInfo(out);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetOduStatusInfo (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL Odu <1-MAX_ODU_ID>: " << oduId << endl;
        return;
    }

    string aid = "Odu ";
    aid += std::to_string(oduId);

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduStatusInfo(out, aid);
    if (status == false )
        out << " FAIL to Get status info " << endl;
}

void cmdGetOduStatusInfoAll (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduStatusInfoAll(out);
    if (status == false )
        out << " FAIL to Get status info " << endl;
}

void cmdGetOduFaultInfo (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL Odu <1-MAX_ODU_ID>: " << oduId << endl;
        return;
    }

    string aid = "Odu ";
    aid += std::to_string(oduId);

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduFaultInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Fault info " << endl;
}

void cmdGetOduFaultInfoAll (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduFaultInfoAll(out);
    if (status == false )
        out << " FAIL to Get Fault info " << endl;
}

void cmdGetOduCdiInfo (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL Odu <1-MAX_ODU_ID>: " << oduId << endl;
        return;
    }

    string aid = "Odu ";
    aid += std::to_string(oduId);

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduCdiInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Fault info " << endl;
}

void cmdOduIsSimEnable (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = DpOduMgrSingleton::Instance()->getAdPtr()->isSimModeEn();

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdOduSetSimEnable (ostream& out, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = enable;

    DpOduMgrSingleton::Instance()->getAdPtr()->setSimModeEn(isEn);

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdGetOduPmInfo (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL oduId <1-34>: " << oduId << endl;
        return;
    }

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduPmInfo(out, oduId);
    if (status == false )
        out << " FAIL to Get Pm info, pls check odu object existence " << endl;
}

void cmdGetOduPmInfoAll (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduPmInfoAll(out);
    if (status == false )
        out << " FAIL to Get Pm info " << endl;
}

void cmdOduNotifyState (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL oduId <1-34>: " << oduId << endl;
        return;
    }
    DpOduMgrSingleton::Instance()->getAdPtr()->oduNotifyState(out, oduId);
}

void cmdOduNotifyStateAll (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    DpOduMgrSingleton::Instance()->getAdPtr()->oduNotifyStateAll(out);
}

void cmdGetOduPmAccumInfo (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL oduId <1-34>: " << oduId << endl;
        return;
    }

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduPmAccumInfo(out, oduId);
    if ( status == false )
        out << " FAIL to Get Pm Accumulated info, pls check odu object existence " << endl;
}

void cmdClearOduPmAccum (ostream& out, uint32_t oduId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL oduId <1-34>: " << oduId << endl;
        return;
    }

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduPmAccumClear(out, oduId);
    if ( status == false )
        out << " FAIL to Clear Pm Accumulated data, pls check odu object existence " << endl;
}

void cmdEnableOduPmAccum (ostream& out, uint32_t oduId, bool enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (oduId < 1 || oduId > MAX_ODU_ID)
    {
        out << " FAIL oduId <1-34>: " << oduId << endl;
        return;
    }

    bool status = false;
    status = DpOduMgrSingleton::Instance()->getAdPtr()->oduPmAccumEnable(out, oduId, enable);
    if ( status == false )
        out << " FAIL to Enable Pm Accumulated data, pls check odu object existence " << endl;
}

void cmdDumpOduAdapterAll(ostream& out)
{
    out << "===odu_adapter===" << endl;
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out))
    {
        out << "Dp Mgr Init is NOT DONE YET. Aborting" << endl;

        return;
    }

    DpOduMgrSingleton::Instance()->getAdPtr()->dumpAll(out);
}

//
// XCON Adapter commands
//
void cmdXconCreate (ostream& out, uint32_t xconId, uint32_t clientId, uint32_t loOduId, uint32_t hoOtuId, uint32_t direction )
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        out << " FAIL XCON ID <1-40>: " << xconId << endl;
        return;
    }
    //1-16 for gige client, 33-48 for odu client
    if (clientId < 1 || clientId > 48 || (clientId > 16 && clientId < 33))
    {
        out << " FAIL Client ID <1-16> or <33-48>: " << clientId << endl;
        return;
    }
    if (loOduId < 1 || loOduId > MAX_ODU_ID)
    {
        out << " FAIL LO-ODU ID <1-32>: " << loOduId << endl;
        return;
    }
    if (hoOtuId < 17 || hoOtuId > 18)
    {
        out << " FAIL HO-OTU ID <17-18>: " << hoOtuId << endl;
        return;
    }

    XconCommon cfg;
    memset(&cfg, 0, sizeof(cfg));

    switch(direction)
    {
        case 1:
            cfg.direction = XCONDIRECTION_INGRESS;
            break;
        case 2:
            cfg.direction = XCONDIRECTION_EGRESS;
            break;
        case 3:
            cfg.direction = XCONDIRECTION_BI_DIR;
            break;
        default:
            out << " FAIL dir <1=ingress or 2=egress or 3=bi-dir>: " << direction << endl;
            return;
    }

    // cfg.source = "1-4-T3";
    // cfg.destination = "1-5-L2-1-ODU4i-81";
    cfg.clientId = clientId;
    cfg.loOduId = loOduId;
    cfg.hoOtuId = hoOtuId;

    string aid = "Xcon ";
    aid += std::to_string(xconId);
    out << " Aid: " << aid << endl;
    // aid = "1-4-T3,1-5-L2-1-ODU4i-81";
    // aid = "1-5-L2-1-ODU4i-81,1-4-T3";

    bool status = false;
    status = DpXcMgrSingleton::Instance()->getAdPtr()->createXcon(aid, &cfg);
    if (status == false )
        out << " FAIL to create XCON" << endl;
}
void cmdXconDelete (ostream& out, uint32_t xconId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        out << " FAIL XCON ID <1-40>: " << xconId << endl;
        return;
    }

    string aid = "Xcon ";
    aid += std::to_string(xconId);
    // aid = "1-4-T15,1-5-L2-1-ODU4i-81";
    out << " Aid: " << aid << endl;

    bool status = false;
    status = DpXcMgrSingleton::Instance()->getAdPtr()->deleteXcon(aid);
    if (status == false )
        out << " FAIL to create XCON" << endl;
}
void cmdGetXconConfigInfo (ostream& out, uint32_t xconId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        out << " FAIL xcon <1-40>: " << xconId << endl;
        return;
    }

    string aid = "Xcon ";
    aid += std::to_string(xconId);

    bool status = false;
    status = DpXcMgrSingleton::Instance()->getAdPtr()->xconConfigInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetXconConfigInfo (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool status = false;
    status = DpXcMgrSingleton::Instance()->getAdPtr()->xconConfigInfo(out);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdGetXconStatusInfo (ostream& out, uint32_t xconId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        out << " FAIL xcon <1-40>: " << xconId << endl;
        return;
    }
    string aid = "Xcon ";
    aid += std::to_string(xconId);

    bool status = false;
    status = DpXcMgrSingleton::Instance()->getAdPtr()->xconStatusInfo(out, aid);
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdXconIsSimEnable (ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = DpXcMgrSingleton::Instance()->getAdPtr()->isSimModeEn();

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdXconSetSimEnable (ostream& out, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = enable;

    DpXcMgrSingleton::Instance()->getAdPtr()->setSimModeEn(isEn);

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdDumpXconAdapterAll(ostream& out)
{
    out << "===xcon_adapter===" << endl;

    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out))
    {
        out << "Dp Mgr Init is NOT DONE YET. Aborting" << endl;

        return;
    }

    DpXcMgrSingleton::Instance()->getAdPtr()->dumpAll(out);
}

void cmdDcoSetGccVlan(ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	DpGccMgrSingleton::Instance()->getAdPtr()->setLldpVlan();
	out << "set lldp vlan" << endl;
}

void cmdDcoGetGccVlanCfg(ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	bool status = false;
	status = DpGccMgrSingleton::Instance()->getAdPtr()->getLldpVlanConfig(out);;
    if (status == false )
        out << " FAIL to Get Cfg info " << endl;
}

void cmdDcoGetGccVlanState(ostream& out)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	bool status = false;
	status = DpGccMgrSingleton::Instance()->getAdPtr()->getLldpVlanState(out);;
    if (status == false )
        out << " FAIL to Get State info " << endl;

}

void cmdDcoCreateCcVlan(ostream& out, uint32_t vIdx, uint32_t carrier, uint32_t txVtag, uint32_t txVretag, uint32_t rxVtag, uint32_t rxVretag, uint32_t gcc_type)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	bool status = false;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        out << " FAIL vIdx <2-32>: " << vIdx << endl;
        return;
    }
    if (carrier < 1 || carrier > 2)
    {
        out << " FAIL carrier <1-2>: " << carrier << endl;
        return;
    }

    ControlChannelVlanCommon cfg;
    cfg.carrier = (ControlChannelCarrier) carrier;
    cfg.type = (ControlChannelType) gcc_type;
    cfg.enable = 0;
    cfg.rxVlanTag = rxVtag;
    cfg.rxVlanReTag = rxVretag;
    cfg.txVlanTag = txVtag;
    cfg.txVlanReTag = txVretag;
    cfg.deleteTxTags = false;

	status = DpGccMgrSingleton::Instance()->getAdPtr()->createCcVlan((ControlChannelVlanIdx) vIdx, &cfg);

    if (status == false )
        out << " FAIL to Create Control Channel VLAN " << endl;
}

void cmdDcoEnableCcVlan(ostream& out, uint32_t vIdx, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	bool status = false;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        out << " FAIL vIdx <2-32>: " << vIdx << endl;
        return;
    }
    if (enable > 1)
    {
        out << " FAIL enable <0 or 1>: " << vIdx << endl;
        return;
    }

	status = DpGccMgrSingleton::Instance()->getAdPtr()->enableCcVlan((ControlChannelVlanIdx) vIdx, enable);

    if (status == false )
        out << " FAIL to Enable Control Channel VLAN " << endl;
}

void cmdDcoDeleteCcVlan(ostream& out, uint32_t vIdx)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	bool status = false;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        out << " FAIL vIdx <2-32>: " << vIdx << endl;
        return;
    }

	status = DpGccMgrSingleton::Instance()->getAdPtr()->deleteCcVlan((ControlChannelVlanIdx) vIdx);

    if (status == false )
        out << " FAIL to delete Control Channel VLAN " << endl;
}

void cmdGetDcoCcVlanConfigInfo(ostream& out, uint32_t vIdx)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	bool status = false;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        out << " FAIL vIdx <2-32>: " << vIdx << endl;
        return;
    }

	status = DpGccMgrSingleton::Instance()->getAdPtr()->ccVlanConfigInfo(out, (ControlChannelVlanIdx) vIdx);

    if (status == false )
        out << " FAIL to Get Control Channel VLAN Config: " << vIdx << endl;
}

void cmdGetDcoCcVlanStatusInfo(ostream& out, uint32_t vIdx)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

	bool status = false;

    if (vIdx < CONTROLCHANNELVLANIDX_NB_DISCOVERY_CARRIER_1 || vIdx > CONTROLCHANNELVLANIDX_MAX)
    {
        out << " FAIL vIdx <2-32>: " << vIdx << endl;
        return;
    }

	status = DpGccMgrSingleton::Instance()->getAdPtr()->ccVlanStatusInfo(out, (ControlChannelVlanIdx) vIdx);

    if (status == false )
        out << " FAIL to Get Control Channel VLAN Status: " << vIdx << endl;
}

void cmdGetGccPmInfo (ostream& out, uint32_t carrierId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrierId < 1 || carrierId > MAX_CARRIERS)
    {
        out << " FAIL carrierId <1-2>: " << carrierId << endl;
        return;
    }

    bool status = false;
    status = DpGccMgrSingleton::Instance()->getAdPtr()->gccPmInfo(out, carrierId);
    if (status == false )
        out << " FAIL to Get Pm info, pls check gcc object existence " << endl;
}

void cmdGetGccPmAccumInfo (ostream& out, uint32_t carrierId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrierId < 1 || carrierId > MAX_CARRIERS)
    {
        out << " FAIL carrierId <1-2>: " << carrierId << endl;
        return;
    }

    bool status = false;
    status = DpGccMgrSingleton::Instance()->getAdPtr()->gccPmAccumInfo(out, carrierId);
    if ( status == false )
        out << " FAIL to Get Pm Accumulated info, pls check gcc object existence " << endl;
}

void cmdClearGccPmAccum (ostream& out, uint32_t carrierId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    if (carrierId < 1 || carrierId > MAX_CARRIERS)
    {
        out << " FAIL carrierId <1-2>: " << carrierId << endl;
        return;
    }

    bool status = false;
    status = DpGccMgrSingleton::Instance()->getAdPtr()->gccPmAccumClear(out, carrierId);
    if ( status == false )
        out << " FAIL to Clear Pm Accumulated data, pls check gcc object existence " << endl;
}

void cmdDumpGccAdapterAll (ostream& out)
{
    out << "===gcc_adapter===" << endl;
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out))
    {
        out << "Dp Mgr Init is NOT DONE YET. Aborting" << endl;

        return;
    }

    DpGccMgrSingleton::Instance()->getAdPtr()->dumpAll(out);
}

void cmdDcoAdapterSimEnable (ostream& out, uint32_t enable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    bool isEn = enable;

    DpCardMgrSingleton      ::Instance()->getAdPtr()->setSimModeEn(isEn);
    DpGigeClientMgrSingleton::Instance()->getAdPtr()->setSimModeEn(isEn);
    DpCarrierMgrSingleton   ::Instance()->getAdPtr()->setSimModeEn(isEn);
    DpOtuMgrSingleton       ::Instance()->getAdPtr()->setSimModeEn(isEn);
    DpOduMgrSingleton       ::Instance()->getAdPtr()->setSimModeEn(isEn);
    DpXcMgrSingleton        ::Instance()->getAdPtr()->setSimModeEn(isEn);
    DpGccMgrSingleton       ::Instance()->getAdPtr()->setSimModeEn(isEn);

    out << " isSimModeEnable: " << isEn << endl;
}

void cmdDcoAdapterIsSimEnable (ostream& out)
{
    out << "*** Dco Adapter Sim Enables " << endl;

    out << "Dco Card    ";
    cmdCardIsSimEnable(out);

    out << "Dco Gige    ";
    cmdGigeIsSimEnable(out);

    out << "Dco Carrier ";
    cmdCarrierIsSimEnable(out);

    out << "Dco Otu     ";
    cmdOtuIsSimEnable(out);

    out << "Dco Odu     ";
    cmdOduIsSimEnable(out);

    out << "Dco Xcon    ";
    cmdXconIsSimEnable(out);

}

void cmdListDcoConnStates(ostream& out)
{
    DcoConnectHandlerSingleton::Instance()->dumpConnectStates(out);
}

void cmdListDcoConnErrors(ostream& out)
{
    DcoConnectHandlerSingleton::Instance()->dumpConnectErrors(out);
}

void cmdListDcoConnIds(ostream& out)
{
    DcoConnectHandlerSingleton::Instance()->dumpDcoConnectIds(out);
}

void cmdDumpDcoConnStatus(ostream& out)
{
    out << endl;

    for(uint32 connId = 0; connId < NUM_CPU_CONN_TYPES; connId++)
    {
        DcoConnectMonitor& pConnMon =
                DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

        pConnMon.dumpConnectStatus(out, connId == 0);
    }

    out << endl;
}

void cmdDumpDcoSimConnStatus(ostream& out)
{
    out << endl;

    for(uint32 connId = 0; connId < NUM_CPU_CONN_TYPES; connId++)
    {
        DcoConnectMonitor& pConnMon =
                DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

        pConnMon.dumpSimStatus(out, connId == 0);
    }

    out << endl;
}

void cmdSetDcoSimConnStatus(ostream& out, uint32 connId, bool isEn, uint32 connState, uint32 connErr)
{
    out << "********** Set Sim Connection Status **********" << std::endl << std::endl;

    if (connId >= NUM_CPU_CONN_TYPES)
    {
        out << " ERROR: connId must be <= " << (uint32)NUM_CPU_CONN_TYPES << std::endl;

        return;
    }

    if (connState >= NUM_DCO_CONN_STATES)
    {
        out << " ERROR: connStat must be <= " << (uint32)NUM_DCO_CONN_STATES << std::endl;

        return;
    }

    if (connErr >= NUM_DCO_CONN_FAILURES)
    {
        out << " ERROR: connErr must be <= " << (uint32)NUM_DCO_CONN_FAILURES << std::endl;

        return;
    }

    DcoConnectMonitor& pConnMon =
            DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

    DcoConnectionStatus dcoConnStat;
    DcoConnectStateType dcoState = (DcoConnectStateType)connState;
    DcoConnectFailureType dcoErr = (DcoConnectFailureType)connErr;

    INFN_LOG(SeverityLevel::info) << "dcoState: " << ToString(dcoState) << " dcoErr: " << ToString(dcoErr);

    dcoConnStat.mConnState   = dcoState;
    dcoConnStat.mConnFailure = dcoErr;
    pConnMon.setSimStatus(isEn, dcoConnStat);

    pConnMon.dumpSimStatus(out);
}

void cmdDumpDcoSocketAddr(ostream& out, uint32 connId)
{
    out << "********** DCO Socket Address **********" << std::endl << std::endl;

    if (connId >= NUM_CPU_CONN_TYPES)
    {
        out << " ERROR: connId must be <= " << (uint32)NUM_CPU_CONN_TYPES << std::endl;

        return;
    }

    DcoConnectMonitor& pConnMon =
            DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

    std::string sockAddr = pConnMon.getSockAddr();

    out << "Socket Address: "  << sockAddr << std::endl;
}

void cmdSetDcoSocketAddr(ostream& out, uint32 connId, std::string sockAddrIn)
{
    out << "********** DCO Socket Address **********" << std::endl << std::endl;

    if (connId >= NUM_CPU_CONN_TYPES)
    {
        out << " ERROR: connId must be <= " << (uint32)NUM_CPU_CONN_TYPES << std::endl;

        return;
    }

    DcoConnectMonitor& pConnMon =
            DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

    pConnMon.setSockAddr(sockAddrIn);

    std::string sockAddr = pConnMon.getSockAddr();

    out << "Socket Address: "  << sockAddr << std::endl;
}

void cmdDumpGrpcConnectStatus(ostream& out)
{
    out << "********** DCO GRPC Connection Status **********" << std::endl << std::endl;

    for(uint32 connId = 0; connId < NUM_CPU_CONN_TYPES; connId++)
    {
        DcoConnectMonitor& pConnMon =
                DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

        pConnMon.dumpGrpcConnectStatus(out, connId == 0);
    }

    out << endl;
}

void cmdGetDcoConnectPollEnable(ostream& out, uint32 connId)
{
    out << "********** DCO Connection Monitoring Polling Enable **********" << std::endl << std::endl;

    if (connId >= NUM_CPU_CONN_TYPES)
    {
        out << " ERROR: connId must be <= " << (uint32)NUM_CPU_CONN_TYPES << std::endl;

        return;
    }

    DcoConnectMonitor& pConnMon =
            DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

    bool isEnable = pConnMon.getPollEn();

    std::string sockAddr = pConnMon.getSockAddr();

    out << "Connection " << ToString((DcoCpuConnectionsType)connId) << " Is Polling Enabled: "  << isEnable << std::endl;
}

void cmdSetDcoConnectPollEnable(ostream& out, uint32 connId, bool isEnable)
{
    out << "********** DCO Connection Monitoring Polling Enable **********" << std::endl << std::endl;

    if (connId >= NUM_CPU_CONN_TYPES)
    {
        out << " ERROR: connId must be <= " << (uint32)NUM_CPU_CONN_TYPES << std::endl;

        return;
    }

    DcoConnectMonitor& pConnMon =
            DcoConnectHandlerSingleton::Instance()->getDcoConnMon((DcoCpuConnectionsType)connId);

    pConnMon.setPollEn(isEnable);

    INFN_LOG(SeverityLevel::info) << "Connection " << ToString((DcoCpuConnectionsType)connId) << " Set Polling Enabled: "  << isEnable;

    out << "Connection " << ToString((DcoCpuConnectionsType)connId) << " Is Polling Enabled: "  << isEnable << std::endl;
}

void cmdDumpPmTime(ostream& out, uint32_t opt)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out)) return;

    switch (opt)
    {
	    case 1: DpGigeClientMgrSingleton::Instance()->getAdPtr()->gigePmTimeInfo(out); break;
	    case 2: DpOduMgrSingleton::Instance()->getAdPtr()->oduPmTimeInfo(out); break;
	    case 3: DpOtuMgrSingleton::Instance()->getAdPtr()->otuPmTimeInfo(out); break;
	    case 4: DpCarrierMgrSingleton::Instance()->getAdPtr()->carrierPmTimeInfo(out); break;
	    default: out << "invalid parameter, gige=1, odu=2, otu=3, carrier=4" << endl; break;
    }
}

void cmdDumpActiveCardSubscriptions(ostream& out)
{
    DpCardMgrSingleton::Instance()->DumpBoardSubscriptions(out);
}

void cmdDumpActiveVlanSubscriptions(ostream& out)
{
    DpPeerDiscoveryMgrSingleton::Instance()->DumpVlanSubscriptions(out);
}

void cmdDumpActiveIcdpSubscriptions(ostream& out)
{
    DpPeerDiscoveryMgrSingleton::Instance()->DumpIcdpSubscriptions(out);
}



void cmdDumpCardAdapterAll(ostream& out)
{
    out << "===card_adapter===" << endl;

    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(out))
    {
        out << "Dp Mgr Init is NOT DONE YET. Aborting" << endl;

        return;
    }

    DpCardMgrSingleton::Instance()->getAdPtr()->dumpAll(out);
}

void cmdDumpDcoAdapterAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    auto startTime = std::chrono::steady_clock::now();

    cmdDumpCardAdapterAll(out);
    cmdDumpGigeAdapterAll(out);
    cmdDumpCarrierAdapterAll(out);
    cmdDumpOtuAdapterAll(out);
    cmdDumpOduAdapterAll(out);
    cmdDumpXconAdapterAll(out);
    cmdDumpGccAdapterAll(out);

    auto endTime = std::chrono::steady_clock::now();

    std::chrono::seconds elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    out << "**** DcoAdapter Cli Collection Time Duration: " << elapsedSec.count() << " seconds" << endl;

    INFN_LOG(SeverityLevel::info) << "DcoAdapter Cli Collection Time Duration: " << elapsedSec.count() << " seconds" ;
}
