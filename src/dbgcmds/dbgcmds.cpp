#include <cli/clilocalsession.h> // include boost asio
#include <cli/remotecli.h>
// TODO. NB: remotecli.h and clilocalsession.h both includes boost asio,
// so in Windows it should appear before cli.h that include rang
// (consider to provide a global header file for the library)
#include <cli/cli.h>
#include <boost/function.hpp>
#include <chrono>
#include "GearBoxAdapter.h"
#include "DataPlaneMgrCmds.h"
#include "SimCmds.h"
#include "adapter_cmds.h"
#include "InfnLogger.h"
#include "serdes_tuner_cmds.h"
#include "dp_common.h"


using namespace cli;
using namespace std;
using namespace boost;
using namespace gearbox;
using namespace DataPlane;

// Enable this define to enable local CLI session
#define ENABLE_LOCAL_CLI_SESSION 0

//debug flag, only used for cli
bool DataPlane::mPmDebugEnable = false;
bool DataPlane::mPmTimeEnable = false;

extern void cmdBcm81725LoadFirmware(ostream& out, unsigned int bus, bool force);
extern void cmdBcm81725WarmReboot(ostream& out);
extern void cmdSetPortRate(std::ostream& out, std::string aId, unsigned int rate, bool fecEnable, unsigned int fecType);
extern void cmdSetFecEnable(std::ostream& out, std::string aId, bool fecSetting);
extern void cmdGetPolarity(std::ostream& out, const Bcm81725Lane& param);
extern void cmdSetPolarity(std::ostream& out, const Bcm81725Lane& param, unsigned int txPol, unsigned int rxPol);
extern void cmdSetAvsConfig(std::ostream& out, const Bcm81725Lane& param, unsigned int enable, unsigned int margin);
extern void cmdGetAvsConfig(std::ostream& out, const Bcm81725Lane& param);
extern void cmdGetAvsStatus(std::ostream& out, const Bcm81725Lane& param);
extern void cmdGetMode(std::ostream& out, const Bcm81725Lane& param);
extern void cmdSetMode(std::ostream& out, const Bcm81725Lane& param,
                int speed, int ifType, int refClk, int interfaceMode,
                unsigned int dataRate, unsigned int modulation,
                unsigned int fecType);
extern void cmdDumpBcmConfig(std::ostream& out, unsigned int port);
extern void cmdDumpBcmConfigAll(std::ostream& out);
extern void cmdGetLaneConfigDs(std::ostream& out, const Bcm81725Lane& param);
extern void cmdGetPortConfigDs(std::ostream& out, unsigned int port);
extern void cmdSetLaneConfigDs(std::ostream& out, const Bcm81725Lane& param, unsigned int fwLaneConfig, int64_t value);
extern void cmdGetTxFir(std::ostream& out, const Bcm81725Lane& param);
extern void cmdGetPortTxFir(std::ostream& out, unsigned int port);
extern void cmdSetTxFir(std::ostream& out, const Bcm81725Lane& param, int pre2, int pre, int main, int post, int post2, int post3);
extern void cmdPrbsLinkStatusCheck(std::ostream& out, const Bcm81725Lane& param);
extern void cmdPrbsStatusCheck(std::ostream& out, const Bcm81725Lane& param, unsigned int timeVal);
extern void cmdSetPrbsGen(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int tx, unsigned int poly,
                   unsigned int inv, unsigned int lb,
                   unsigned int enaDis);
extern void cmdSetPortLoopback(std::ostream& out, const Bcm81725Lane& param, unsigned int mode, unsigned int enable);
extern void cmdGetPortLoopback(std::ostream& out, const Bcm81725Lane& param, unsigned int mode);
extern void cmdSetPrbsCheck(std::ostream& out, const Bcm81725Lane& param,
                   unsigned int rx, unsigned int poly,
                   unsigned int inv, unsigned int lb,
                   unsigned int enaDis);
extern void cmdGetBcmInfo(ostream& out);
extern void cmdGetEyeScan(ostream& out, const Bcm81725Lane& param);
extern void cmdGetEyeScan(ostream& out, unsigned int port, Bcm81725DataRate_t rate);
extern void cmdGetEyeProjection(ostream& out, unsigned int port, Bcm81725DataRate_t rate, unsigned int scanMode,
                                unsigned int timerControl, unsigned int maxErr);
extern void cmdGetEyeProjection(ostream& out, const Bcm81725Lane& param, unsigned int scanMode, unsigned int timerControl, unsigned int maxErr);
extern void cmdChipStatus(ostream& out, const Bcm81725Lane& param, unsigned int func);
extern void cmdClrInterrupt(ostream& out, const Bcm81725Lane& param);
extern void cmdSetInterrupt(ostream& out, const Bcm81725Lane& param, unsigned int typeMask, bool enable);
extern void cmdGetInterrupt(ostream& out, const Bcm81725Lane& param, unsigned int typeMask);
extern void cmdGetPcsStatus(ostream& out, unsigned int port, Bcm81725DataRate_t rate);
extern void cmdGetFecStatus(ostream& out, const Bcm81725Lane& param, bool clear);
extern void cmdGetFaults(ostream& out, unsigned int port);
extern void cmdGetPmCounts(ostream & out, unsigned int port, Bcm81725DataRate_t rate, unsigned int lane, bool clear);
extern void cmdSetRxSquelch(ostream& out, const Bcm81725Lane& param, bcm_plp_pm_phy_rx_lane_control_t rxControl);
extern void cmdGetRxSquelch(ostream& out, unsigned int port, unsigned int side, Bcm81725DataRate_t rate);
extern void cmdSetTxSquelch(ostream& out, const Bcm81725Lane& param, bcm_plp_pm_phy_tx_lane_control_t txControl);
extern void cmdGetTxSquelch(ostream& out, unsigned int port, unsigned int side, Bcm81725DataRate_t rate);
extern void cmdIwrite(ostream& out, const Bcm81725Lane& param, unsigned int regAddr, unsigned int regData);
extern void cmdIread(ostream& out, const Bcm81725Lane& param, unsigned int regAddr);
extern void cmdDumpPam4Diag(ostream& out, unsigned int port);
extern void cmdDumpPam4DiagAll(ostream& out);
extern void cmdSetFailOver(ostream& out, const Bcm81725Lane& param, unsigned int mode);
extern void cmdGetFailOver(ostream& out, const Bcm81725Lane& param);
extern void cmdDumpBcmAll(ostream& out);
extern void cmdAvsTpsReadData(ostream& out, uint32 mezzBrdIdx, uint32 bcmIdx, uint32 opCode, uint32 numBytes);
extern void cmdAvsTpsWriteData(ostream& out, uint32 mezzBrdIdx, uint32 bcmIdx, uint32 opCode, uint32 numBytes, uint32 data);
extern void cmdAvsTpsDump(ostream& out);


void cmdDcoAdapterMenu(unique_ptr< Menu > & subMenu);
void cmdGigeAdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdCardAdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdCarrierAdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdOtuAdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdBcm81725AdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdOduAdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdXconAdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdGccAdapterSubMenu(unique_ptr< Menu > & subMenu);
void cmdDcoSecprocAdapterMenu(unique_ptr< Menu > & subMenu);

void cmdDpBertSubMenu(unique_ptr< Menu > & subMenu);
void cmdSetBertMonitor(ostream& out, unsigned int facility, unsigned int port, unsigned int mode);
void cmdGetBertCounters(ostream& out, unsigned int facility, unsigned int port);
void cmdClearBertCounters(ostream& out, unsigned int facility, unsigned int port);

void cmdDpLogSubMenu(unique_ptr< Menu > & subMenu);
void cmdDpmsLog(ostream& out, uint32_t logPriorities);
void cmdDpDebugSubMenu(unique_ptr< Menu > & subMenu);
void cmdDpmsDebug(ostream& out, bool enable);
void cmdPmTimeDebug(ostream& out, bool enable);
void cmdDpExceptionDebug(ostream& out, std::string password);
void cmdSrdsTnrMgrSubMenu(unique_ptr< Menu > & subMenu);
void cmdDcoConnectMonSubMenu(unique_ptr< Menu > & subMenu);
void cmdTimePmSubMenu(unique_ptr< Menu > & subMenu);

void cmdSecprocCardAdpSubSubMenu(unique_ptr< Menu > & subMenu);
void cmdSecprocIcdpAdpSubSubMenu(unique_ptr< Menu > & subMenu);
void cmdSecprocVlanAdpSubSubMenu(unique_ptr< Menu > & subMenu);


#define CLI_PORT_NUM 5004

////////////////////////////////////////////////////////////
//
void cmdDpMgr (ostream& out)
{
    out << "Data Plane Manager CMDs " << endl;
}


int DbgCmds()
{

#if BOOST_VERSION < 106600
    boost::asio::io_service ios;ostream& out, bool force
#else
    boost::asio::io_context ios;
#endif

    // setup cli

    auto rootMenu = make_unique< Menu >( "dp_ms_cli" );

    // Sub Menu:  Dp Mgr
    //
    auto subMenuDpMgr           = make_unique< Menu >( "dp_mgr"      );
    auto subMenuDpMgrCardMgr    = make_unique< Menu >( "card_mgr"    );
    auto subMenuDpMgrCarrierMgr = make_unique< Menu >( "carrier_mgr" );
    auto subMenuDpMgrGigeMgr    = make_unique< Menu >( "gige_mgr"    );
    auto subMenuDpMgrOtuMgr     = make_unique< Menu >( "otu_mgr"     );
    auto subMenuDpMgrOduMgr     = make_unique< Menu >( "odu_mgr"     );
    auto subMenuDpMgrXconMgr    = make_unique< Menu >( "xcon_mgr"    );
    auto subMenuDpMgrGccMgr    = make_unique< Menu >( "gcc_mgr"    );

    DataPlane::cmdDpMgrSubMenu          (subMenuDpMgr          );
    DataPlane::cmdDpMgrCardMgrSubMenu   (subMenuDpMgrCardMgr   );
    DataPlane::cmdDpMgrCarrierMgrSubMenu(subMenuDpMgrCarrierMgr);
    DataPlane::cmdDpMgrGigeMgrSubMenu   (subMenuDpMgrGigeMgr   );
    DataPlane::cmdDpMgrOtuMgrSubMenu    (subMenuDpMgrOtuMgr    );
    DataPlane::cmdDpMgrOduMgrSubMenu    (subMenuDpMgrOduMgr    );
    DataPlane::cmdDpMgrXconMgrSubMenu   (subMenuDpMgrXconMgr   );
    DataPlane::cmdDpMgrGccMgrSubMenu    (subMenuDpMgrGccMgr   );

    subMenuDpMgr->Insert( std::move(subMenuDpMgrCardMgr   ) );
    subMenuDpMgr->Insert( std::move(subMenuDpMgrCarrierMgr) );
    subMenuDpMgr->Insert( std::move(subMenuDpMgrGigeMgr   ) );
    subMenuDpMgr->Insert( std::move(subMenuDpMgrOtuMgr    ) );
    subMenuDpMgr->Insert( std::move(subMenuDpMgrOduMgr    ) );
    subMenuDpMgr->Insert( std::move(subMenuDpMgrXconMgr   ) );
    subMenuDpMgr->Insert( std::move(subMenuDpMgrGccMgr   ) );


    //
    // Sub Menu:  DCO Adapters
    //
    auto dcoSubMenu = make_unique< Menu >( "dco_adapters" );
    auto gigeAdpSubSubMenu = make_unique< Menu >( "gige_adapter" );
    auto cardAdpSubSubMenu = make_unique< Menu >( "card_adapter" );
    auto carrierAdpSubSubMenu = make_unique< Menu >( "carrier_adapter" );
    auto otuAdpSubSubMenu = make_unique< Menu >( "otu_adapter" );
    auto bcm81725SubMenu = make_unique< Menu >( "bcm81725_adapter" );
    auto oduAdpSubSubMenu = make_unique< Menu >( "odu_adapter" );
    auto xconAdpSubSubMenu = make_unique< Menu >( "xcon_adapter" );
    auto gccAdpSubSubMenu = make_unique< Menu >( "gcc_adapter" );
    auto srdsTnrMgrSubMenu = make_unique< Menu >( "srds_tnr_mgr");
    auto dcoConnMonSubMenu = make_unique< Menu >( "dco_connection_handler");
    auto timePmSubMenu = make_unique< Menu >( "time_pm");
    auto dcoSecprocSubMenu = make_unique< Menu >( "dco_secproc_adapters");

    cmdDcoAdapterMenu(dcoSubMenu);
    cmdCardAdapterSubMenu(cardAdpSubSubMenu);
    cmdGigeAdapterSubMenu(gigeAdpSubSubMenu);
    cmdCarrierAdapterSubMenu(carrierAdpSubSubMenu);
    cmdOtuAdapterSubMenu(otuAdpSubSubMenu);
    cmdDpLogSubMenu(xconAdpSubSubMenu);
    cmdGccAdapterSubMenu(gccAdpSubSubMenu);
    cmdDcoConnectMonSubMenu(dcoConnMonSubMenu);
    cmdTimePmSubMenu(timePmSubMenu);
    cmdDcoSecprocAdapterMenu(dcoSecprocSubMenu);

    //
    // Sub Menu:  DCO Secproc Adapters
    //
    auto secprocCardAdpSubSubMenu = make_unique< Menu >( "secproc_card_adapter" );
    auto secprocVlanAdpSubSubMenu = make_unique< Menu >( "secproc_vlan_adapter" );
    auto secprocIcdpAdpSubSubMenu = make_unique< Menu >( "secproc_icdp_adapter" );

    cmdSecprocCardAdpSubSubMenu(secprocCardAdpSubSubMenu);
    cmdSecprocIcdpAdpSubSubMenu(secprocIcdpAdpSubSubMenu);
    cmdSecprocVlanAdpSubSubMenu(secprocVlanAdpSubSubMenu);

    // Sub Menu: BERT
    auto bertSubMenu = make_unique< Menu >( "bert");
    cmdDpBertSubMenu(bertSubMenu);

    //set log for dpms
    auto subMenuLog = make_unique< Menu >( "log" );
    cmdDpLogSubMenu(subMenuLog);

    //debug flag for dpms
    auto subMenuDebug = make_unique< Menu >( "debug" );
    cmdDpDebugSubMenu(subMenuDebug);

    // Sub Menu: DCO Gnmi Sim
    auto subMenuSim = make_unique< Menu >("sim");

    DpSim::cmdDpMgrSubMenu(subMenuSim);

    cmdBcm81725AdapterSubMenu(bcm81725SubMenu);
    cmdOduAdapterSubMenu(oduAdpSubSubMenu);
    cmdXconAdapterSubMenu(xconAdpSubSubMenu);
    cmdSrdsTnrMgrSubMenu(srdsTnrMgrSubMenu);

    dcoSecprocSubMenu -> Insert(std::move(secprocCardAdpSubSubMenu));
    dcoSecprocSubMenu -> Insert(std::move(secprocVlanAdpSubSubMenu));
    dcoSecprocSubMenu -> Insert(std::move(secprocIcdpAdpSubSubMenu));

    dcoSubMenu -> Insert( std::move(cardAdpSubSubMenu) );
    dcoSubMenu -> Insert( std::move(gigeAdpSubSubMenu) );
    dcoSubMenu -> Insert( std::move(carrierAdpSubSubMenu) );
    dcoSubMenu -> Insert( std::move(otuAdpSubSubMenu) );
    dcoSubMenu -> Insert( std::move(oduAdpSubSubMenu) );
    dcoSubMenu -> Insert( std::move(xconAdpSubSubMenu) );
    dcoSubMenu -> Insert( std::move(gccAdpSubSubMenu) );
    dcoSubMenu -> Insert( std::move(dcoConnMonSubMenu) );
    dcoSubMenu -> Insert( std::move(timePmSubMenu) );
    dcoSubMenu -> Insert( std::move(subMenuSim) );
    dcoSubMenu -> Insert ( std::move(dcoSecprocSubMenu) );


    rootMenu -> Insert( std::move(dcoSubMenu) );
    rootMenu -> Insert( std::move(bcm81725SubMenu) );
    rootMenu -> Insert( std::move(subMenuDpMgr) );
    rootMenu -> Insert( std::move(subMenuLog) );
    rootMenu -> Insert( std::move(subMenuDebug) );
    rootMenu -> Insert( std::move(srdsTnrMgrSubMenu) );
    rootMenu -> Insert( std::move(bertSubMenu) );

    Cli cli( std::move(rootMenu) );
    // global exit action
    cli.ExitAction( [](auto& out){ out << "Goodbye from cli!\n"; } );

#if ENABLE_LOCAL_CLI_SESSION //Disable local session
    CliLocalTerminalSession localSession(cli, ios, std::cout, 300);
    localSession.ExitAction(
        [&ios](auto& out) // session exit action
        {
            out << "Closing App...\n";
            ios.stop();
        }
    );
#endif

    for(;;)
    {
        try
        {
            // setup server
            CliTelnetServer server(ios, CLI_PORT_NUM, cli);
            // exit action for all the connections
            server.ExitAction( [](auto& out) { out << "Terminating this session...\n"; } );

            ios.run();
        }
        catch(const boost::system::system_error& e)
        {
            INFN_LOG(SeverityLevel::error) << "Caught and handled boost exception for CLI: " << e.what() << " : " << e.code();
        }
    }

    return 0;
}

//
// Card Adapter commands
//
void cmdCardAdapterSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "create",
            [](std::ostream& out )
            {
                cmdCreateCard(out);
            },
            "Create DCO Card"  );

    subMenu -> Insert(
            "delete",
            [](std::ostream& out )
            {
                cmdDeleteCard(out);
            },
            "Delete DCO Card"  );

    subMenu -> Insert(
            "set_client_mode",
            [](std::ostream& out, uint32_t mode)
            {
                cmdCardSetClientMode(out, mode);
            },

            "Set DCO Card Client Mode: <mode: 1(E), 2(M), 3(LXTP-only)>"  );

    subMenu -> Insert(
            "set_max_packet_len",
            [](std::ostream& out, uint32_t pktLen)
            {
                cmdCardSetMaxPktLen(out, pktLen);
            },
            "Set DCO Card Max Packet Length: <1518..18000>"  );

    subMenu -> Insert(
            "enable",
            [](std::ostream& out, uint32_t enable)
            {
                cmdCardEnable(out, enable);
            },
            "Set DCO Card enable: <mode: 1 (enable) 0 (disable)"  );

    subMenu -> Insert(
            "boot",
            [](std::ostream& out, uint32_t boot)
            {
                cmdCardBoot(out, boot);
            },
            "Set DCO Card boot: <mode: 2 (shutdown) 1 (cold) 0 (warm)"  );

    subMenu -> Insert(
            "cfg_info",
            [](std::ostream& out)
            {
                cmdCardConfigInfo(out);
            },
            "Print Dco Card config Info"  );

    subMenu -> Insert(
            "stat_info",
            [](std::ostream& out)
            {
                cmdCardStatusInfo(out);
            },
            "Print Dco Card Status Info"  );

    subMenu -> Insert(
            "stat_info_force",
            [](std::ostream& out)
            {
                cmdCardStatusInfoForce(out);
            },
            "Print Dco Card Status Info. Not from Cache, from DCO directly"  );

    subMenu -> Insert(
            "capa_info",
            [](std::ostream& out)
            {
                cmdCardCapabilityInfo(out);
            },
            "Print Dco Card Capabitites Info"  );

    subMenu -> Insert(
            "fault_info",
            [](std::ostream& out)
            {
                cmdCardFaultInfo(out);
            },
            "Print Dco Card Fault Info"  );

    subMenu -> Insert(
            "fault_capa_info",
            [](std::ostream& out)
            {
                cmdCardFaultCapaInfo(out);
            },
            "Print Dco Card Fault Capability Info"  );

    subMenu -> Insert(
            "pm_info",
            [](std::ostream& out)
            {
                cmdCardPmInfo(out);
            },
            "Print Dco Card Pm Info"  );

    subMenu -> Insert(
            "notify_state",
            [](std::ostream& out)
            {
                cmdCardNotifyState(out);
            },
            "Print Card Adapter Notify State Info"  );

    subMenu -> Insert(
            "SimEnable",
            [](std::ostream& out, uint32_t enable)
            {
                cmdCardSetSimEnable(out, enable);
            },
            "Enable Adapter Sim"  );

    subMenu -> Insert(
            "IsSimEnable",
            [](std::ostream& out)
            {
                cmdCardIsSimEnable(out);
            },
            "Is Adapter Sim Enabled"  );

    subMenu -> Insert(
            "DumpCardAdapterAll",
            [](std::ostream& out)
            {
                cmdDumpCardAdapterAll(out);
            },
            "DumpCardAdapterAll"  );


}

//
// Gige Adapter commands
//
void cmdGigeAdapterSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "create",
            [](std::ostream& out, uint32_t port, uint32_t rate, uint32_t fecEnable)
            {
                cmdGigeCreate(out, port, rate, fecEnable);
            },
            "Set Gige Create Port: <1-16> Rate: <100 or 400> fecEnable: <1 or 0>"  );

    subMenu -> Insert(
            "delete",
            [](std::ostream& out, uint32_t port)
            {
                cmdGigeDelete(out, port);
            },
            "Set Gige Delete Port: <1-16>"  );

    subMenu -> Insert(
            "set_fec",
            [](std::ostream& out, uint32_t port, uint32_t enable)
            {
                cmdGigeSetFecEnable(out, port, enable);
            },
            "Set Gige FEC Port: <1-16> enable: <1 or 0>"  );

    subMenu -> Insert(
            "set_lldp_drop",
            [](std::ostream& out, uint32_t port, uint32_t dir, uint32_t enable)
            {
                cmdGigeSetLldpDrop(out, port, dir, enable);
            },
            "Set Gige LLDP Drop Port: <1-16> Direction: <tx=1 or rx=0> enable: <1 or 0>"  );

    subMenu -> Insert(
            "set_lldp_snoop",
            [](std::ostream& out, uint32_t port, uint32_t dir, uint32_t enable)
            {
                cmdGigeSetLldpSnoop(out, port, dir, enable);
            },
            "Set Gige LLDP Snoop Port: <1-16> Direction: <tx=1 or rx=0> enable: <1 or 0>"  );

    subMenu -> Insert(
            "set_loopback",
            [](std::ostream& out, uint32_t port, uint32_t lpbk)
            {
                cmdGigeSetLoopBack(out, port, lpbk);
            },
            "Set Gige LoopBack Port: <1-16> mode: <off=0 or fac=1 or term=2>"  );

    subMenu -> Insert(
            "set_mtu",
            [](std::ostream& out, uint32_t port, uint32_t mtu)
            {
                cmdGigeSetMtu(out, port, mtu);
            },
            "Set Gige Mtu, Port: <1-16> MTU: <size> "  );

    subMenu -> Insert(
            "set_fwd_tda",
            [](std::ostream& out, uint32_t port, uint32_t enable)
            {
                cmdGigeSetFwdTdaTrigger(out, port, enable);
            },
            "Set Gige Fwd Tda Trigger Port: <1-16> enable: <1 or 0>"  );

    subMenu -> Insert(
            "cfg_info",
            [](std::ostream& out, uint32_t port)
            {
                cmdGigeConfigInfo(out, port);
            },
            "Get Gige Client Config Info, Port: <1-16> "  );

    subMenu -> Insert(
            "cfg_info_all",
            [](std::ostream& out)
            {
                cmdGigeConfigInfo(out);
            },
            "Get Gige Client Config Info, ALL "  );

    subMenu -> Insert(
            "status_info",
            [](std::ostream& out, uint32_t port)
            {
                cmdGigeStatusInfo(out, port);
            },
            "Get Gige Client Status Info, Port: <1-16> "  );

    subMenu -> Insert(
            "fault_info",
            [](std::ostream& out, uint32_t port)
            {
                cmdGigeFaultInfo(out, port);
            },
            "Get Gige Client Fault Info, Port: <1-16> "  );

    subMenu -> Insert(
            "SimEnable",
            [](std::ostream& out, uint32_t enable)
            {
                cmdGigeSetSimEnable(out, enable);
            },
            "Enable Adapter Sim"  );

    subMenu -> Insert(
            "IsSimEnable",
            [](std::ostream& out)
            {
                cmdGigeIsSimEnable(out);
            },
            "Is Adapter Sim Enabled"  );

    subMenu -> Insert(
            "pm_info",
            [](std::ostream& out, uint32_t port)
            {
                cmdGetGigePmInfo(out, port);
            },
            "Get Gige Pm Info, Port: <1-16>"  );

    subMenu -> Insert(
            "pm_accum_info",
            [](std::ostream& out, uint32_t port)
            {
                cmdGetGigePmAccumInfo(out, port);
            },
            "Get Gige Pm Accumulated Info, Port: <1-16>"  );

    subMenu -> Insert(
            "pm_accum_clear",
            [](std::ostream& out, uint32_t port)
            {
                cmdClearGigePmAccum(out, port);
            },
            "Clear Gige Pm Accumulated Data, Port: <1-16>"  );

    subMenu -> Insert(
            "pm_accum_enable",
            [](std::ostream& out, uint32_t port, uint32_t enable)
            {
                cmdEnableGigePmAccum(out, port, enable);
            },
            "Enable Gige Pm Accumulated Data, Port: <1-16> <1=enable|0=disable>"  );

    subMenu -> Insert(
            "dumpAll",
            [](std::ostream& out, uint32_t port)
            {
                cmdDumpGigeAdapterAll(out);
            },
            "Dump all Gige"  );

}

void cmdCarrierAdapterSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "create",
            [](std::ostream& out, uint32_t carrier, uint32_t capa, double baud, string appId)
            {
                cmdCarrierCreate(out, carrier, capa, baud, appId);
            },
            "Set Carrier Create: <1-2> capa <800..> baud <95.6390657 or 91.2918355 or ..> id <P..>"  );

    subMenu -> Insert(
            "delete",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdCarrierDelete(out, carrier);
            },
            "Set Carrier Delete: <1-2>"  );

    subMenu -> Insert(
            "set_capacity",
            [](std::ostream& out, uint32_t carrier, uint32_t rate)
            {
                cmdCarrierCapacity(out, carrier, rate);
            },
            "Set Carrier Set Capacity: <1-2> rate: < 800 or 600 or ... >"  );

    subMenu -> Insert(
            "set_baudrate",
            [](std::ostream& out, uint32_t carrier, double rate)
            {
                cmdCarrierBaud(out, carrier, rate);
            },
            "Set Carrier Set Baud Rate: <1-2> rate: < 95.6390657 or 91.2918355 or .. >"  );

    subMenu -> Insert(
            "set_appId",
            [](std::ostream& out, uint32_t carrier, string appCode)
            {
                cmdCarrierAppId(out, carrier, appCode);
            },
            "Set Carrier Set Baud Rate: <1-2> appId: < P or S or ... >"  );

    subMenu -> Insert(
            "enable",
            [](std::ostream& out, uint32_t carrier, uint32_t enable)
            {
                cmdCarrierEnable(out, carrier, enable);
            },
            "Set Carrier enable: <1-2> enable: < 1 or 0 >"  );

    subMenu -> Insert(
            "set_frequency",
            [](std::ostream& out, uint32_t carrier, double frequency)
            {
                cmdCarrierFrequency(out, carrier, frequency);
            },
            "Set Carrier frequency: <1-2> frequency: < value >"  );

    subMenu -> Insert(
            "set_freq_offset",
            [](std::ostream& out, uint32_t carrier, double offset)
            {
                cmdCarrierOffSet(out, carrier, offset);
            },
            "Set Carrier frequency offset: <1-2> offset: < value >"  );

    subMenu -> Insert(
            "set_power",
            [](std::ostream& out, uint32_t carrier, double power)
            {
                cmdCarrierPower(out, carrier, power);
            },
            "Set Carrier power: <1-2> power: < value >"  );

    subMenu -> Insert(
            "set_tx_cd",
            [](std::ostream& out, uint32_t carrier, double txCd)
            {
                cmdCarrierTxCd(out, carrier, txCd);
            },
            "Set Carrier power: <1-2> power: < value >"  );

    subMenu -> Insert(
            "set_adv_parm",
            [](std::ostream& out, uint32_t carrier, string name, string value)
            {
                cmdCarrierAdvParm(out, carrier, name, value);
            },
            "Set Carrier Adv Param: <1-2> name: < string > value < string>"  );

    subMenu -> Insert(
            "delete_adv_parm",
            [](std::ostream& out, uint32_t carrier, std::string key)
            {
                cmdDelCarrierAdvParm(out, carrier, key);
            },
            "Delete Carrier adv parameter: <1-2> <key>"  );

    subMenu -> Insert(
            "delete_adv_parm_all",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdDelCarrierAdvParmAll(out, carrier);
            },
            "Delete Carrier adv parameter whole list: <1-2>"  );

    subMenu -> Insert(
            "set_dgd_oorh_th",
            [](std::ostream& out, uint32_t carrier, double thresh)
            {
                cmdCarrierDgdOorhTh(out, carrier, thresh);
            },
            "Set Carrier DGD OORH Threshold: <1-2> threshold: < 180.0 - 350.0 >"  );

    subMenu -> Insert(
            "set_post_fec_sd_th",
            [](std::ostream& out, uint32_t carrier, double thresh)
            {
                cmdCarrierPostFecSdTh(out, carrier, thresh);
            },
            "Set Carrier POST FEC SD Threshold: <1-2> threshold: < 12.5 - 18.0 | 30.0 >"  );

    subMenu -> Insert(
            "set_post_fec_sd_hyst",
            [](std::ostream& out, uint32_t carrier, double hysteresis)
            {
                cmdCarrierPostFecSdHyst(out, carrier, hysteresis);
            },
            "Set Carrier POST FEC SD Hysteresis: <1-2> hysteresis: < 0.10 - 3.0 >"  );

    subMenu -> Insert(
            "set_pre_fec_sd_th",
            [](std::ostream& out, uint32_t carrier, double thresh)
            {
                cmdCarrierPreFecSdTh(out, carrier, thresh);
            },
            "Set Carrier PRE FEC SD Threshold: <1-2> threshold: < 5.60-7.0 >"  );

    subMenu -> Insert(
            "set_pre_fec_sd_hyst",
            [](std::ostream& out, uint32_t carrier, double hysteresis)
            {
                cmdCarrierPreFecSdHyst(out, carrier, hysteresis);
            },
            "Set Carrier PRE FEC SD Hysteresis: <1-2> hysteresis: < 0.10-1.0 >"  );

    subMenu -> Insert(
            "cfg_info",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdGetCarrierConfigInfo(out, carrier);
            },
            "Get Carrier Config Info: <1-2>"  );

    subMenu -> Insert(
            "cfg_info_all",
            [](std::ostream& out)
            {
                cmdGetCarrierConfigInfo(out);
            },
            "Get Carrier Config Info, ALL"  );

    subMenu -> Insert(
            "status_info",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdGetCarrierStatusInfo(out, carrier);
            },
            "Get Carrier Status Info: <1-2>"  );

    subMenu -> Insert(
            "fault_info",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdGetCarrierFaultInfo(out, carrier);
            },
            "Get Carrier Fault Info: <1-2>"  );

    subMenu -> Insert(
            "pm_info",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdGetCarrierPmInfo(out, carrier);
            },
            "Get Carrier Pm Info: <1-2>"  );

    subMenu -> Insert(
            "notify_state",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdCarrierNotifyState(out, carrier);
            },
            "Get Carrier Notify State: <1-2>"  );

    subMenu -> Insert(
            "pm_accum_info",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdGetCarrierPmAccumInfo(out, carrier);
            },
            "Get Carrier Accumulated Info, Port: <1-2>"  );

    subMenu -> Insert(
            "pm_accum_clear",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdClearCarrierPmAccum(out, carrier);
            },
            "Clear Carrier Accumulated Data, Port: <1-2>"  );

    subMenu -> Insert(
            "pm_accum_enable",
            [](std::ostream& out, uint32_t carrier, uint32_t enable)
            {
                cmdEnableCarrierPmAccum(out, carrier, enable);
            },
            "Enable Carrier Accumulated Data, Port: <1-2> <1=enable|0=disable>"  );

    subMenu -> Insert(
            "SimEnable",
            [](std::ostream& out, uint32_t enable)
            {
                cmdCarrierSetSimEnable(out, enable);
            },
            "Enable Adapter Sim"  );

    subMenu -> Insert(
            "IsSimEnable",
            [](std::ostream& out)
            {
                cmdCarrierIsSimEnable(out);
            },
            "Is Adapter Sim Enabled"  );

    subMenu -> Insert(
            "deleteBaudRate",
            [](std::ostream& out, uint32_t carrier)
            {
                cmdDeleteCarrierBaudRate(out, carrier);
            },
            "Delete Carrier BaudRate, Carrier: <1-2>"  );

    subMenu -> Insert(
            "DumpCarrierAdapterAll",
            [](std::ostream& out)
            {
                cmdDumpCarrierAdapterAll(out);
            },
            "DumpCarrierAdapterAll"  );

}

void cmdBcm81725AdapterSubMenu(unique_ptr< Menu > & bcm81725SubMenu)
{
    bcm81725SubMenu -> Insert(
            "loadFw",
            [](std::ostream& out, std::string busStr, bool force)
            {
                cmdBcm81725LoadFirmware(out, static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)), force);
            },
            "Load Firmware", {"bus sel: top-1, bottom-0", "force (1 - default, 0 - compare version)"});
    bcm81725SubMenu->Insert(
            "GetBcmVers",
            [](std::ostream& out)
            {
                cmdGetBcmInfo(out);
            },
            "Bcm81725 Get version/CRC" );

    bcm81725SubMenu -> Insert(
            "warmReboot",
            [](std::ostream& out)
            {
                cmdBcm81725WarmReboot(out);
            },
            "Bcm81725 warm reboot" );

    bcm81725SubMenu -> Insert(
            "setPortRate",
            [](std::ostream& out, std::string aId, unsigned int rate, bool fecEnable, unsigned int fecType)
            {
                cmdSetPortRate(out, aId, rate, fecEnable, fecType);
            },
            "Bcm81725 set data rate", {"AID: 1-4-T1 .. 1-4-T16", "rate: 0 - 100GE, 1 - 400GE", "fecEnable: 0 - false, 1 - true", "fecType 0-6"} );

    bcm81725SubMenu -> Insert(
            "setPortFecEnable",
            [](std::ostream& out, std::string aId, bool enable)
            {
                cmdSetFecEnable(out, aId, enable);
            },
            "Bcm81725 set FEC enable", {"AID: 1-4-T1 .. 1-4-T16", "FEC: 0 - disable, 1 - enable"} );


    bcm81725SubMenu -> Insert(
            "getPolarity",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneMapStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneMapStr, nullptr, 16))};
                cmdGetPolarity(out, param);
            },
            "Get Polarity", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)", "host-1, line-0", "lane number"} );

    bcm81725SubMenu -> Insert(
            "setPolarity",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string txPolStr, std::string rxPolStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                cmdSetPolarity(out, param, std::stoul(txPolStr), std::stoul(rxPolStr));
            },
            "Set Polarity", {"bus sel: top-0 bottom-1", "Mdio Address(4, 8, 12)",
            "host-1, line-0", "lane number", "tx polarity", "rx polarity"} );

    bcm81725SubMenu -> Insert(
            "setAvsConfig",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string enableStr, std::string marginStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                cmdSetAvsConfig(out, param, std::stoul(enableStr), std::stoul(marginStr));
            },
            "Set AVS Config", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane number", "enable", "margin"} );

    bcm81725SubMenu -> Insert(
            "getAvsConfig",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneMapStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneMapStr, nullptr, 16))};
                cmdGetAvsConfig(out, param);
            },
            "Get Avs Config", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)", "host-1, line-0", "lane number"} );

    bcm81725SubMenu -> Insert(
            "getAvsStatus",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneMapStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneMapStr, nullptr, 16))};
                cmdGetAvsStatus(out, param);
            },
            "Get Avs Status", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)", "host-1, line-0", "lane number"} );

    bcm81725SubMenu -> Insert(
            "getMode",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneMapStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneMapStr, nullptr, 16))};
                cmdGetMode(out, param);
            },
            "GetMode", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)", "host-1, line-0", "lane number"} );

    bcm81725SubMenu -> Insert(
            "setMode",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string speedStr,
               std::string ifTypeStr, std::string refClkStr, std::string ifModeStr,
               std::string dataRateStr, std::string modulationStr, std::string fecTypeStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                cmdSetMode(out, param, std::stoi(speedStr), std::stoi(ifTypeStr),
                           std::stoi(refClkStr), std::stoi(ifModeStr), std::stoul(dataRateStr),
                           std::stoul(modulationStr), std::stoul(fecTypeStr));
            },
            "Set Mode", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane number", "speed", "if_type", "ref_clk", "interface_mode",
            "data_rate", "modulation", "fec_type"} );

    bcm81725SubMenu -> Insert(
            "DumpBcmConfig",
            [](std::ostream& out, unsigned int port)
            {
                cmdDumpBcmConfig(out, port);
            },
            "Dump BCM Config", {"port: 1-16"} );

    bcm81725SubMenu -> Insert(
            "DumpBcmConfigAll",
            [](std::ostream& out)
            {
        cmdDumpBcmConfigAll(out);
            },
            "Dump BCM Config" );

    bcm81725SubMenu -> Insert(
            "SetLaneConfigDs",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                    std::string sideStr, std::string laneStr, unsigned int fwLaneConfig, int64_t value)
            {
                 Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                    static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                    static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                    static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                 cmdSetLaneConfigDs(out, param, fwLaneConfig, value);
            },
            "GetLaneConfigDs", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "enum - 1-DfeOn, 2-AnEnabled, 3-MediaType, 4-AutoPeakEn, 5-OppoCdrFirst, 6-DcWanderMu, 7-ExSlicer, "
                    " 8-LinkTrainReStartTimeOut, 9-LinkTrainCL72_CL93PresetEn, 10-LinkTrain802_3CDPresetEn, 11-LinkTrainTxAmpTuning, 12-LpDfeOn, "
                    "13-AnTimer, value: (enum dependent)"} );

    bcm81725SubMenu -> Insert(
                "GetPortConfigDs",
                [](std::ostream& out, unsigned int port)
                {

                    cmdGetPortConfigDs(out, port);
                },
                "GetPortConfigDs", {"port: 1-16"} );

    bcm81725SubMenu -> Insert(
            "GetLaneConfigDs",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                    std::string sideStr, std::string laneStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                cmdGetLaneConfigDs(out, param);
            },
            "GetLaneConfigDs", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane"} );

    bcm81725SubMenu -> Insert(
                "GetPortTxFir",
                [](std::ostream& out, unsigned int port)
                {

                    cmdGetPortTxFir(out, port);
                },
                "GetPortTxFir", {"Port: 1-16"} );




    bcm81725SubMenu -> Insert(
                "GetTxFir",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr)
                {
                    Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                       static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                       static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                       static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};

                    cmdGetTxFir(out, param);
                },
                "GetTxFir", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane"} );

    bcm81725SubMenu -> Insert(
                "SetTxFir",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, int pre2, int pre, int main, int post, int post2, int post3)
                {
                        Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                           static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                           static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                           static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdSetTxFir(out, param, pre2, pre, main, post, post2, post3);
                },
                "SetTxFir", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane", "pre2", "pre", "main", "post", "post2", "post3"} );

    bcm81725SubMenu -> Insert(
                "SetRxSquelchPort", [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, unsigned int rxControl)
                {
                        Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdSetRxSquelch(out, param, (bcm_plp_pm_phy_rx_lane_control_t)rxControl);
                },
                "SetRxSquelchPort", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane", "RxControl: 0-Reset Rx datapath, 1-RxSquelch On, 2-RxSquelch Off", "rate: 0 - 100GE, 1 - 400GE"} );

    bcm81725SubMenu -> Insert(
                "GetRxSquelchPort",
                [](std::ostream& out, unsigned int port, unsigned int side, unsigned int rate)
                {
                    cmdGetRxSquelch(out, port, side, (Bcm81725DataRate_t)rate);
                },
                "GetRxSquelchPort", {"Port: 1-16", "side: 1-Host, 0-Line", "rate: 0 - 100GE, 1 - 400GE"} );

    bcm81725SubMenu -> Insert(
                "SetTxSquelchPort", [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, unsigned int txControl)
                {
                        Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdSetTxSquelch(out, param, (bcm_plp_pm_phy_tx_lane_control_t)txControl);
                },
                "SetRxSquelchPort", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane", "TxControl: 0-Reset Tx datapath, 1-TxSquelch On, 2-TxSquelch Off"} );

    bcm81725SubMenu -> Insert(
                "GetTxSquelchPort",
                [](std::ostream& out, unsigned int port, unsigned int side, unsigned int rate)
                {
                    cmdGetTxSquelch(out, port, side, (Bcm81725DataRate_t)rate);
                },
                "GetRxSquelchPort", {"Port: 1-16", "side: 1-Host, 0-Line", "rate: 0 - 100GE, 1 - 400GE"} );



    bcm81725SubMenu -> Insert(
                "ClrInterrupt",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr)
                {
                    Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                                                           static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                                                           static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                                                           static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdClrInterrupt(out, param);
                },
                "ClrInterrupt", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane"} );

    bcm81725SubMenu -> Insert(
                "GetInterrupt",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, std::string typeMask)
                {

                    Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdGetInterrupt(out, param, static_cast<unsigned int>(std::stoul(typeMask, nullptr, 32)));
                },
                "GetInterrupt", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane", "typeMask=0-0xFFFFFF"} );

    bcm81725SubMenu -> Insert(
                "SetInterrupt",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, std::string typeMask, unsigned int enable)
                {
                    Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                                           static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                                           static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                                           static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdSetInterrupt(out, param, static_cast<unsigned int>(std::stoul(typeMask, nullptr, 32)), (enable > 0));
                },
                "SetInterrupt", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane", "typeMask=0-0xFFFFFF", "enable 0=false, 1=true"} );

    bcm81725SubMenu -> Insert(
                "GetPcsStatus",
                [](std::ostream& out, unsigned int port, unsigned int rate)
                {
                    cmdGetPcsStatus(out, port, (Bcm81725DataRate_t)rate);
                },
                "GetPcsStatus", {"port: 1-16", "rate: 0 - 100GE, 1 - 400GE"} );


    bcm81725SubMenu -> Insert(
                "GetFecStatus",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, unsigned int clear)
                {

                    Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdGetFecStatus(out, param, (clear > 0));
                },
                "GetFecStatus", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane", "clear 0:false, 1:true"} );

    bcm81725SubMenu -> Insert(
                "GetFaults",
                [](std::ostream& out, unsigned int port)
                {
                    cmdGetFaults(out, port);
                },
                "GetFaults", {"port: 1-16"} );

    bcm81725SubMenu -> Insert(
                "GetPmCounts",
                [](std::ostream& out, unsigned int port, unsigned int rate, unsigned int lane, unsigned int clear)
                {

                    cmdGetPmCounts(out, port, (Bcm81725DataRate_t)rate, lane, (bool)(clear > 0));
                },
                "GetPmCounts", {"port: 1-16", "rate: 0 - 100GE, 1 - 400GE", "lane 0=all, 1-4, 1-8", "clear: 0=false, 1=true"} );

    bcm81725SubMenu -> Insert(
                "ChipStatus",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, unsigned int func)
                {
                        Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                           static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                           static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                           static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                    cmdChipStatus(out, param, func);
                },
                "ChipStatus", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                        "host-1, line-0", "lane", "func (bitmask) use 0 for default"} );

    bcm81725SubMenu -> Insert(
                "GetEyeScan",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                   std::string sideStr, std::string laneStr)
                   {
                            Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                               static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                               static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                               static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                        cmdGetEyeScan(out, param);
                    },
                                "GetEyeScan", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                                        "host-1, line-0", "lane"} );

    bcm81725SubMenu -> Insert(
                "GetEyeScanPort",
                [](std::ostream& out, unsigned int port, unsigned int rate)
                   {
                        cmdGetEyeScan(out, port, (Bcm81725DataRate_t)rate);
                    },
                                "GetEyeScanPort", {"port: 1-16", "rate: 0 - 100GE, 1 - 400GE"} );

    bcm81725SubMenu -> Insert(
                "GetEyeProjection",
                [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
                        std::string sideStr, std::string laneStr, unsigned int berScanMode,
                        unsigned int timerControl, unsigned int maxErr)
                   {
                             Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                                       static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                                       static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                                       static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                        cmdGetEyeProjection(out, param, berScanMode, timerControl, maxErr);
                   },
                                "GetEyeScanProjection", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                                        "host-1, line-0", "lane", "BER Scan Mode 0,1,2", "Timer Control units meas ~1.3s",
                                        "Max Err Control threshold in meas units of 16"} );

    bcm81725SubMenu -> Insert(
                "GetEyeProjectionPort",
                [](std::ostream& out, unsigned int port, unsigned int rate, unsigned int berScanMode,
                   unsigned int timerControl, unsigned int maxErr)
                   {

                        cmdGetEyeProjection(out, port, (Bcm81725DataRate_t)rate, berScanMode, timerControl, maxErr);
                   },
                                "GetEyeScanProjection", {"port: 1-16", "rate: 0 - 100GE, 1 - 400GE", "BER Scan Mode 0,1,2", "Timer Control units meas ~1.3s",
                   "Max Err Control threshold in meas units of 16"} );

    bcm81725SubMenu -> Insert(
            "setPrbsGen",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string txStr,
               std::string polyStr, std::string invStr, std::string lbStr,
               std::string enaDisStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};

                cmdSetPrbsGen(out, param, std::stoul(txStr), std::stoul(polyStr), std::stoul(invStr),
                           std::stoul(lbStr), std::stoul(enaDisStr));
            },
            "Set Prbs Gen", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "tx", "poly", "inv", "lb", "ena_dis"} );


    bcm81725SubMenu -> Insert(
            "setPrbsCheck",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string rxStr,
               std::string polyStr, std::string invStr, std::string lbStr,
               std::string enaDisStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};

                cmdSetPrbsCheck(out, param, std::stoul(rxStr), std::stoul(polyStr), std::stoul(invStr),
                           std::stoul(lbStr), std::stoul(enaDisStr));
            },
            "Set Prbs Check", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "rx", "poly", "inv", "lb", "ena_dis"} );

    bcm81725SubMenu -> Insert(
            "prbsStatusCheck",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string timeVal)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};

                cmdPrbsStatusCheck(out, param, std::stoul(timeVal));
            },
            "Prbs Status Check", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "time_val"} );

    bcm81725SubMenu -> Insert(
            "prbsLinkStatusCheck",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};

                cmdPrbsLinkStatusCheck(out, param);
            },
            "Prbs Link Status Check", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane"} );

    bcm81725SubMenu -> Insert(
            "setLoopback",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string modeStr, std::string enableStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};

                cmdSetPortLoopback(out, param, std::stoul(modeStr), std::stoul(enableStr));
            },
            "Set Loopback", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "mode remote-1 digit-2", "enable 1"} );

    bcm81725SubMenu -> Insert(
            "getLoopback",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string modeStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};

                cmdGetPortLoopback(out, param, std::stoul(modeStr));
            },
            "Get Loopback", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "mode remote-1 digit-2"} );

    bcm81725SubMenu -> Insert(
            "Write",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string regAddr, std::string regData)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};



                cmdIwrite(out, param, static_cast<unsigned int>(std::stoul(regAddr, nullptr, 16)),
                        static_cast<unsigned int>(std::stoul(regData, nullptr, 16)));
            },
            "Write", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "regAddr (hex)", "regData (hex)"} );

    bcm81725SubMenu -> Insert(
            "Read",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, std::string regAddr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};



                cmdIread(out, param, static_cast<unsigned int>(std::stoul(regAddr, nullptr, 16)));
            },
            "Read", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
            "host-1, line-0", "lane", "regAddr (hex)"} );

    bcm81725SubMenu -> Insert(
            "getPam4Diag",
            [](std::ostream& out, unsigned int port)
            {
                cmdDumpPam4Diag(out, port);
            },
            "getPam4Diag", {"port: 1-16"} );

    bcm81725SubMenu -> Insert(
            "getPam4DiagAll",
            [](std::ostream& out)
            {
                cmdDumpPam4DiagAll(out);
            },
            "getPam4DiagAll", {} );

    bcm81725SubMenu -> Insert(
            "setFailOver",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr, unsigned int mode)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                cmdSetFailOver(out, param, mode);
            },
            "setFailOver", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                    "host-1, line-0", "lane", "mode=0 Squelch, 1=PRBS"} );

    bcm81725SubMenu -> Insert(
            "getFailOver",
            [](std::ostream& out, std::string busStr, std::string mdioAddrStr,
               std::string sideStr, std::string laneStr)
            {
                Bcm81725Lane param{static_cast<unsigned int>(std::stoul(busStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(mdioAddrStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(sideStr, nullptr, 16)),
                                   static_cast<unsigned int>(std::stoul(laneStr, nullptr, 16))};
                cmdGetFailOver(out, param);
            },
            "getFailOver", {"bus sel: top-1 bottom-0", "Mdio Address(4, 8, c)",
                    "host-1, line-0", "lane"} );
    bcm81725SubMenu -> Insert(
                "DumpBcmAll",
                [](std::ostream& out)
                {
                    cmdDumpBcmAll(out);
                },
                "DumpBcmAll", {} );

    bcm81725SubMenu -> Insert(
            "avsTpsRead",
            [](std::ostream& out, unsigned int mezzIdx, unsigned int bcmIdx, std::string opCodeStr, unsigned int len)
            {
                cmdAvsTpsReadData(out, mezzIdx, bcmIdx, static_cast<unsigned int>(std::stoul(opCodeStr, nullptr, 16)), len);
            },
            "avsTpsRead", {"mezzIdx: top-1 bottom-0", "bcmIdx(0,1,2)", "opCode: hex regAddr", "length: Max 4"} );

    bcm81725SubMenu -> Insert(
            "avsTpsWrite",
            [](std::ostream& out, unsigned int mezzIdx, unsigned int bcmIdx, std::string opCodeStr, unsigned int len, std::string dataStr)
            {
                cmdAvsTpsWriteData(out, mezzIdx, bcmIdx, static_cast<unsigned int>(std::stoul(opCodeStr, nullptr, 16)), len, static_cast<unsigned int>(std::stoul(dataStr, nullptr, 16)));
            },
            "avsTpsRead", {"mezzIdx: top-1 bottom-0", "bcmIdx(0,1,2)", "opCode: hex regAddr", "length: Max 4", "data(hex)"} );

    bcm81725SubMenu -> Insert(
                "DumpAvsTps",
                [](std::ostream& out)
                {
                    cmdAvsTpsDump(out);
                },
                "DumpAvsTps", {} );
}

void cmdSrdsTnrMgrSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "SetBcmTomParams",
            [](std::ostream& out, unsigned int slot, unsigned int port, unsigned int rate)
            {
                  cmdSetBcmTomParams(out, slot, port, rate);
            },
            "SetBcmTomParams", {"Slot 4-7", "Port 1-16", "rate 0: 100GE, 1: 400GE, 3: OTU4"});

    subMenu -> Insert(
            "SetBcmBcmParams",
            [](std::ostream& out, unsigned int port, unsigned int rate)
            {
                  cmdSetBcmBcmParams(out, port, rate);
            },
            "SetBcmBcmParams", {"Port 1-16", "rate 0: 100GE, 1: 400GE, 3: OTU4"});

    subMenu -> Insert(
             "SetBcmAtlParams",
             [](std::ostream& out, unsigned int port, unsigned int rate)
             {
                   cmdSetBcmAtlParams(out, port, rate);
             },
             "SetBcmAtlParams", {"Port 1-16", "rate 0: 100GE, 1: 400GE, 3: OTU4"});

    subMenu -> Insert(
             "DumpSerdesPortValues",
             [](std::ostream& out, unsigned int port)
             {
        cmdDumpSerdesPortValues(out, port);
             },
             "DumpSerdesPortValues", {"Port 1-16"});




}

void cmdOtuAdapterSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "create",
            [](std::ostream& out, uint32_t otuId, uint32_t payLoad, uint32_t channel)
            {
                cmdOtuCreate(out, otuId, payLoad, channel);
            },
            "Create Otu: <1-32> payLoad <1=OTUCni or 2=OTU4> channel <1 or 2 or 3=both>"  );

    subMenu -> Insert(
            "delete",
            [](std::ostream& out, uint32_t otuId)
            {
                cmdOtuDelete(out, otuId);
            },
            "Delete Otu: <1-32>"  );

    subMenu -> Insert(
            "set_loopback",
            [](std::ostream& out, uint32_t otuId, uint32_t lpbk)
            {
                cmdOtuSetLoopBack(out, otuId, lpbk);
            },
            "Set Otu Loopback: <1-32> < mode: <off=0 or fac=1 or term=2>"  );

    subMenu -> Insert(
            "set_prbs_mon",
            [](std::ostream& out, uint32_t otuId, uint32_t dir, uint32_t prbs)
            {
                cmdOtuSetPrbsMon(out, otuId, dir, prbs);
            },
            "Set Otu PRBS Monitor: <1-32> Direction: <tx=1 or rx=0> prbs: <None=0 or X31=1 or X31INV=2>"  );

    subMenu -> Insert(
            "set_prbs_gen",
            [](std::ostream& out, uint32_t otuId, uint32_t dir, uint32_t prbs)
            {
                cmdOtuSetPrbsGen(out, otuId, dir, prbs);
            },
            "Set Otu PRBS Generation: <1-32> Direction: <tx=1 or rx=0> prbs: <None=0 or X31=1 or X31INV=2>"  );

    subMenu -> Insert(
            "set_tx_tti",
            [](std::ostream& out, uint32_t otuId, string tti)
            {
                cmdOtuSetTxTti(out, otuId, tti);
            },
            "Set Otu TX TTI: <1-32> tti_string: <\"tti content\">"  );

    subMenu -> Insert(
            "set_force_ms",
            [](std::ostream& out, uint32_t otuId, uint32_t dir, uint32_t ms)
            {
                cmdOtuSetForceMs(out, otuId, dir, ms);
            },
            "Set Otu Force MS: <1-32> Direction: <tx=1 or rx=0> ms: <NoReplace=0, LaserOff=1, X31=2, X31INV=3, AIS=4, OCI=5, LCK=6>" );

    subMenu -> Insert(
            "set_fault_status",
            [](std::ostream& out, uint32_t otuId, uint32_t dir, string injectFaultBitmap)
            {
                cmdOtuSetFaultStatus(out, otuId, dir, injectFaultBitmap);
            },
            "Inject Otu Fault Status: <1-2> Direction: <tx=1 or rx=0> injectFaultBitmapString: <hex example: f08a>" );

    subMenu -> Insert(
            "cfg_info",
            [](std::ostream& out, uint32_t otuId)
            {
                cmdGetOtuConfigInfo(out, otuId);
            },
            " Get OTU Config Info: Client<1-16> Line<17-18>"  );

    subMenu -> Insert(
            "cfg_info_all",
            [](std::ostream& out)
            {
                cmdGetOtuConfigInfo(out);
            },
            " Get OTU Config Info, ALL"  );

    subMenu -> Insert(
            "status_info",
            [](std::ostream& out, uint32_t otuId)
            {
                cmdGetOtuStatusInfo(out, otuId);
            },
            " Get OTU Status Info: Client<1-16> Line<17-18>"  );

    subMenu -> Insert(
            "SimEnable",
            [](std::ostream& out, uint32_t enable)
            {
                cmdOtuSetSimEnable(out, enable);
            },
            "Enable Adapter Sim"  );

    subMenu -> Insert(
            "IsSimEnable",
            [](std::ostream& out)
            {
                cmdOtuIsSimEnable(out);
            },
            "Is Adapter Sim Enabled"  );

    subMenu -> Insert(
            "pm_info",
            [](std::ostream& out, uint32_t port)
            {
                cmdGetOtuPmInfo(out, port);
            },
            "Get OTU Pm Info: Client<1-16> Line<17-18>"  );

    subMenu -> Insert(
            "pm_accum_info",
            [](std::ostream& out, uint32_t port)
            {
                cmdGetOtuPmAccumInfo(out, port);
            },
            "Get OTU Accumulated Pm Info: Client<1-16> Line<17-18>"  );

    subMenu -> Insert(
            "pm_accum_clear",
            [](std::ostream& out, uint32_t port)
            {
                cmdClearOtuPmAccum(out, port);
            },
            "Clear OTU Accumulated Pm Data: Client<1-16> Line<17-18>"  );

    subMenu -> Insert(
            "pm_accum_enable",
            [](std::ostream& out, uint32_t port, uint32_t enable)
            {
                cmdEnableOtuPmAccum(out, port, enable);
            },
            "Enable OTU Accumulated Pm Data: Client<1-16> Line<17-18> <1=enable|0=disable>"  );

    subMenu -> Insert(
            "fault_info",
            [](std::ostream& out, uint32_t otuId)
            {
                cmdGetOtuFaultInfo(out, otuId);
            },
            "Get OTU Fault Info: Client<1-16> Line<17-18>"  );

    subMenu -> Insert(
            "notify_state",
            [](std::ostream& out, uint32_t otuId)
            {
                cmdOtuNotifyState(out, otuId);
            },
            "Get OTU Notify Info: Client<1-16> Line<17-18>"  );

    subMenu -> Insert(
            "dumpOtuAdapterAll",
            [](std::ostream& out, uint32_t otuId)
            {
                cmdDumpOtuAdapterAll(out);
            },
            "dumpOtuAdapterAll"  );

}

void cmdOduAdapterSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "create",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, uint32_t payLoad, string ts)
            {
                cmdOduCreate(out, oduId, otuId, payLoad, ts);
            },
            "Create Odu: <1-64> HO-OtucniId <1-64> payLoad <1=ODU4i or 2=Flexi or 3=ODUCni or 4=ODU4> TSstring <\"0\"=none or \"1 2 .. 15 16\">"  );

    subMenu -> Insert(
            "delete",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdOduDelete(out, oduId);
            },
            "Delete Odu LoOdu is 1-16, HoOdu is 33-34: <1-34> "  );

    subMenu -> Insert(
            "set_loopback",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, uint32_t lpbk)
            {
                cmdOduSetLoopBack(out, oduId, otuId, lpbk);
            },
            "Set Odu Loopback: OduId <1-64>, OtuId <1-32>, lpbk <off=0 or fac=1 or term=2>"  );

    subMenu -> Insert(
            "set_prbs_mon",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t prbs)
            {
                cmdOduSetPrbsMon(out, oduId, otuId, dir, prbs);
            },
            "Set Odu PRBS Monitor OduId: <33-48> OtuId: <1-16> Direction: <tx=1 or rx=0> prbs: <None=0 or X31=1 or X31INV=2>"  );

    subMenu -> Insert(
            "set_prbs_gen",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t prbs)
            {
                cmdOduSetPrbsGen(out, oduId, otuId, dir, prbs);
            },
            "Set Odu PRBS Generation OduId: <33-48> OtuId: <1-16> Direction: <tx=1 or rx=0> prbs: <None=0 or X31=1 or X31INV=2>"  );


    subMenu -> Insert(
            "set_ts",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, string ts)
            {
                cmdOduSetTS(out, oduId, otuId, ts);
            },
            "Set Odu Time Slot: <1-64> TSstring <\"0\"=none or \"1 2 .. 15 16\">"  );

    subMenu -> Insert(
            "set_force_ms",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t ms)
            {
                cmdOduSetForceMs(out, oduId, otuId, dir, ms);
            },
            "Set Odu Force MS: oduId: <1-48> otuId: <1-18> Direction: <tx=1 or rx=0> ms: <NoReplace=0, LaserOff=1, X31=2, X31INV=3, AIS=4, OCI=5, LCK=6>" );

    subMenu -> Insert(
            "set_fault_status",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, string injectFaultBitmap)
            {
                cmdOduSetFaultStatus(out, oduId, otuId, injectFaultBitmap);
            },
            "Inject Odu Fault Status: <1-32> injectFaultBitmapString: <hex example: f08a>" );

    subMenu -> Insert(
            "set_auto_ms",
            [](std::ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t ms)
            {
                cmdOduSetAutoMs(out, oduId, otuId, dir, ms);
            },
            "Set Odu Auto MS: oduId: <1-48> otuId: <1-18> Direction: <tx=1 or rx=0> ms: <NoReplace=0, LaserOff=1, X31=2, X31INV=3, AIS=4, OCI=5, LCK=6>" );

    subMenu -> Insert(
            "cfg_info",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdGetOduConfigInfo(out, oduId);
            },
            " Get ODU Config Info LoOdu is 1-8, 17-24, HoOdu is 33-34, 35-36, Client Odu is 33-48: <1-48>"  );

    subMenu -> Insert(
            "cfg_info_all",
            [](std::ostream& out)
            {
                cmdGetOduConfigInfo(out);
            },
            " Get ODU Config Info, ALL"  );

    subMenu -> Insert(
            "status_info",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdGetOduStatusInfo(out, oduId);
            },
            " Get ODU Status Info LoOdu is 1-8, 17-24, HoOdu is 33-34, 35-36, Client Odu is 33-48: <1-48>"  );

    subMenu -> Insert(
            "SimEnable",
            [](std::ostream& out, uint32_t enable)
            {
                cmdOduSetSimEnable(out, enable);
            },
            "Enable Adapter Sim"  );

    subMenu -> Insert(
            "IsSimEnable",
            [](std::ostream& out)
            {
                cmdOduIsSimEnable(out);
            },
            "Is Adapter Sim Enabled"  );

    subMenu -> Insert(
            "pm_info",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdGetOduPmInfo(out, oduId);
            },
            "Get Odu Pm Info, Odu: <1-48>"  );

    subMenu -> Insert(
            "pm_accum_info",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdGetOduPmAccumInfo(out, oduId);
            },
            "Get Odu Pm Accumulated Info, Port: <1-48>"  );

    subMenu -> Insert(
            "pm_accum_clear",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdClearOduPmAccum(out, oduId);
            },
            "Clear Odu Pm Accumulated Data, Port: <1-48>"  );

    subMenu -> Insert(
            "pm_accum_enable",
            [](std::ostream& out, uint32_t oduId, uint32_t enable)
            {
                cmdEnableOduPmAccum(out, oduId, enable);
            },
            "Enable ODU PM accumulation: <LoOdu (1-8,17-24), HoOdu(33-34,35-36), ClientOdu(33-48)>  <1=enable|0=disable>" );

    subMenu -> Insert(
            "fault_info",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdGetOduFaultInfo(out, oduId);
            },
            " Get ODU Fault Info LoOdu is 1-8, 17-24, HoOdu is 33-34, 35-36, Client Odu is 33-48: <1-48>"  );

    subMenu -> Insert(
            "cdi_info",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdGetOduCdiInfo(out, oduId);
            },
            " Get ODU Status Info LoOdu is 1-16, HoOdu is 33-34: <1-34>"  );

    subMenu -> Insert(
            "notify_state",
            [](std::ostream& out, uint32_t oduId)
            {
                cmdOduNotifyState(out, oduId);
            },
            "Get Odu Notify State, Odu: <1-48>"  );
}

void cmdXconAdapterSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "create",
            [](std::ostream& out, uint32_t xconId, uint32_t clientId, uint32_t loOduId, uint32_t hoOtuId, uint32_t direction )
            {
                cmdXconCreate(out, xconId, clientId, loOduId, hoOtuId, direction);
            },
	     //1-16 for gige client, 33-48 for odu client
	     "Create Xcon: <1-32> clientId <1-16>or<33-48> loOduId <1-16> hoOtuId <17-18> direction <1=ingress or 2=egress or 3=bi-dir"  );

    subMenu -> Insert(
            "delete",
            [](std::ostream& out, uint32_t xconId)
            {
                cmdXconDelete(out, xconId);
            },
            "Delete Xcon: <1-32>"  );

    subMenu -> Insert(
            "cfg_info",
            [](std::ostream& out, uint32_t xconId)
            {
                cmdGetXconConfigInfo(out, xconId);
            },
            " Get Xcon Config Info: <1-32>"  );

    subMenu -> Insert(
            "cfg_info_all",
            [](std::ostream& out)
            {
                cmdGetXconConfigInfo(out);
            },
            " Get Xcon Config Info, ALL"  );

    subMenu -> Insert(
            "status_info",
            [](std::ostream& out, uint32_t xconId)
            {
                cmdGetXconStatusInfo(out, xconId);
            },
            " Get Xcon Status Info: <1-32>"  );

    subMenu -> Insert(
            "SimEnable",
            [](std::ostream& out, uint32_t enable)
            {
                cmdXconSetSimEnable(out, enable);
            },
            "Enable Adapter Sim"  );

    subMenu -> Insert(
            "IsSimEnable",
            [](std::ostream& out)
            {
                cmdXconIsSimEnable(out);
            },
            "Is Adapter Sim Enabled"  );

    subMenu -> Insert(
            "DumpXconAdapterAll",
            [](std::ostream& out)
            {
                cmdDumpXconAdapterAll(out);
            },
            "DumpXconAdapterAll"  );

}

void cmdDcoAdapterMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "GetSimEnables",
            [](std::ostream& out )
            {
                cmdDcoAdapterIsSimEnable(out);
            },
            "Prints Dco Adapter Sim Enables"  );

    subMenu -> Insert(
            "SetSimEnables",
            [](std::ostream& out, uint32_t enable )
            {
                cmdDcoAdapterSimEnable(out, enable);
            },
            "Applies sim enable to all Dco Adapters"  );

}

void cmdGccAdapterSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "setGccVlan",
            [](std::ostream& out)
            {
                cmdDcoSetGccVlan(out);
            },
            "set gcc vlan"  );

    subMenu -> Insert(
            "getGccVlanCfg",
            [](std::ostream& out)
            {
                cmdDcoGetGccVlanCfg(out);
            },
            "get gcc vlan cfg "  );

    subMenu -> Insert(
            "getGccVlanState",
            [](std::ostream& out)
            {
                cmdDcoGetGccVlanState(out);
            },
            "get gcc vlan state "  );

    subMenu -> Insert(
            "create",
            [](std::ostream& out, uint32_t vIdx, uint32_t carrier, uint32_t txVtag, uint32_t txVretag, uint32_t rxVtag, uint32_t rxVretag, uint32_t gcc_type)
            {
                cmdDcoCreateCcVlan(out, vIdx, carrier, txVtag, txVretag,  rxVtag, rxVretag, gcc_type);
            },
            "Create CcVlan: <2-32> carrier <1 or 2> txVtag txVretag rxVtag rxVretag gcc_type" );

    subMenu -> Insert(
            "enable",
            [](std::ostream& out, uint32_t vIdx, uint32_t enable )
            {
                cmdDcoEnableCcVlan(out, vIdx, enable);
            },
            "Enable CcVlan: <2-32> enable <0 or 1>" );

    subMenu -> Insert(
            "delete",
            [](std::ostream& out, uint32_t vIdx)
            {
                cmdDcoDeleteCcVlan(out, vIdx);
            },
            "Delete CcVlan: <2-32>" );

    subMenu -> Insert(
            "cfg_info",
            [](std::ostream& out, uint32_t vIdx )
            {
                cmdGetDcoCcVlanConfigInfo(out, vIdx);
            },
            "Get CcVlan Config: <2-32>" );

    subMenu -> Insert(
            "status_info",
            [](std::ostream& out, uint32_t vIdx )
            {
                cmdGetDcoCcVlanStatusInfo(out, vIdx);
            },
            "Get CcVlan StatusConfig: <2-32>" );

    subMenu -> Insert(
            "pm_info",
            [](std::ostream& out, uint32_t carrierId)
            {
                cmdGetGccPmInfo(out, carrierId);
            },
            "Get Gcc Pm Info, carrierId: <1-2>"  );

    subMenu -> Insert(
            "pm_accum_info",
            [](std::ostream& out, uint32_t carrierId)
            {
                cmdGetGccPmAccumInfo(out, carrierId);
            },
            "Get Gcc Pm Accumulated Info, carrierId: <1-2>"  );

    subMenu -> Insert(
            "pm_accum_clear",
            [](std::ostream& out, uint32_t carrierId)
            {
                cmdClearGccPmAccum(out, carrierId);
            },
            "Clear Gcc Pm Accumulated Data, carrierId: <1-2>"  );

    subMenu -> Insert(
                "DumpGccAdapterAll",
                [](std::ostream& out, uint32_t carrierId)
                {
                    cmdDumpGccAdapterAll(out);
                },
                "DumpGccAdapterAll"  );

}

void cmdDpBertSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "SetBertMonitor",
            [](std::ostream& out, unsigned int facility, unsigned int port, unsigned int mode)
            {
                  cmdSetBertMonitor(out, facility, port, mode);
            },
            "SetBertMonitor", {"Facility 1:Gige, 2:ODU", "Port 1-16", "Mode 0:disable, 1:enable"});

    subMenu -> Insert(
            "GetBertCounters",
            [](std::ostream& out, unsigned int facility, unsigned int port)
            {
                  cmdGetBertCounters(out, facility, port);
            },
            "GetBertCounters", {"Facility 1:Gige, 2:ODU", "Port 1-16"});

    subMenu -> Insert(
            "ClearBertCounters",
            [](std::ostream& out, unsigned int facility, unsigned int port)
            {
                  cmdGetBertCounters(out, facility, port);
            },
            "ClearBertCounters", {"Facility 1:Gige, 2:ODU", "Port 1-16"});
}

void cmdSetBertMonitor(ostream& out, unsigned int facility, unsigned int port, unsigned int mode)
{
    if (facility < 1 || facility > 2)
    {
        out << " Invalid Facility <1-2>: " << facility << endl;
        out << " Facility 1:Gige, 2:ODU" << endl;
        return;
    }

    if (port < 1 || port > 16)
    {
        out << " Invalid Port <1-16>: " << port << endl;
        return;
    }

    if (mode > 1)
    {
        out << " Invalid Mode <0-1>: " << mode << endl;
        out << " Mode 0:disable, 1:enable" << endl;
        return;
    }

    switch (facility)
    {
        case 1:
            if (0 == mode)
            {
                cmdEnableGigePmAccum(out, port, true);
            }
            else
            {
                cmdEnableGigePmAccum(out, port, false);
            }
            break;
        case 2:
            if (0 == mode)
            {
                cmdEnableOduPmAccum(out, CLIENT_ODU_ID_OFFSET+port, true);
            }
            else
            {
                cmdEnableOduPmAccum(out, CLIENT_ODU_ID_OFFSET+port, false);
            }
            break;
    }
}

void cmdGetBertCounters(ostream& out, unsigned int facility, unsigned int port)
{
    if (facility < 1 || facility > 2)
    {
        out << " Invalid Facility <1-2>: " << facility << endl;
        out << " Facility 1:Gige, 2:ODU" << endl;
        return;
    }

    if (port < 1 || port > 16)
    {
        out << " Invalid Port <1-16>: " << port << endl;
        return;
    }

    switch (facility)
    {
        case 1:
            cmdGetGigePmAccumInfo(out, port);
            break;
        case 2:
            cmdGetOduPmAccumInfo(out, CLIENT_ODU_ID_OFFSET+port);
            break;
    }
}

void cmdClearBertCounters(ostream& out, unsigned int facility, unsigned int port)
{
    if (facility < 1 || facility > 2)
    {
        out << " Invalid Facility <1-2>: " << facility << endl;
        out << " Facility 1:Gige, 2:ODU" << endl;
        return;
    }

    if (port < 1 || port > 16)
    {
        out << " Invalid Port <1-16>: " << port << endl;
        return;
    }

    switch (facility)
    {
        case 1:
            cmdClearGigePmAccum(out, port);
            break;
        case 2:
            cmdClearOduPmAccum(out, CLIENT_ODU_ID_OFFSET+port);
            break;
    }
}

void cmdDpLogSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "SetLogPriorities",
            [](std::ostream& out, uint32_t loglevel )
            {
                cmdDpmsLog(out, loglevel);
            },
            "Set Dpms Log Priorities, debug=7, info=6(default), notice=5, warning=4, error=3, crit=2, alert=1, fatal=0" );

    subMenu->Insert("DumpDpMsDbgXfr",
            [](std::ostream& out)
            {
                auto startTime = std::chrono::steady_clock::now();

                cmdDumpDpMgrAll(out);
                cmdDumpDcoAdapterAll(out);
                cmdDumpBcmAll(out);
                cmdDumpSrdsTnrMgrAll(out);

                auto endTime = std::chrono::steady_clock::now();

                std::chrono::seconds elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

                out << "**** DP MS Cli Collection Time Duration: " << elapsedSec.count() << " seconds" << endl;

                INFN_LOG(SeverityLevel::info) << "DP MS Cli Collection Time Duration: " << elapsedSec.count() << " seconds" ;
            },
            "Dump all DP MS for debug xfr" );
}

void cmdDpmsLog (ostream& out, uint32_t logPriorities)
{
    if (logPriorities < 0 || logPriorities > 7)
    {
        out << " invalid log prioritie <0-7>: " << logPriorities << endl;
		out << " debug=7, info=6(default), notice=5, warning=4, error=3, crit=2, alert=1, fatal=0" << endl;
        return;
    }
	InfnLogger::setLoggingSeverity(static_cast<SeverityLevel>(logPriorities));
}

void cmdDpDebugSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "SetPmDebug",
            [](std::ostream& out, bool enable )
            {
                cmdDpmsDebug(out, enable);
            },
            " Toggle Pm Debug Flag, 1(enable), 0(disable)" );

    subMenu -> Insert(
            "PmTimeSet",
            [](std::ostream& out, bool enable )
            {
                cmdPmTimeDebug(out, enable);
            },
            " Toggle Pm Time Flag, 1(enable), 0(disable)" );

    subMenu -> Insert(
            "TriggerException",
            [](std::ostream& out, std::string pass)
            {
                cmdDpExceptionDebug(out, pass);
            },
            "Cause DP MS to throw exception and crash. Checks the given password first" );

}

void cmdDpmsDebug (ostream& out, bool enable)
{
	if (DataPlane::mPmDebugEnable)
	{
		out << " previous Pm Debug Flag is enabled" << endl;
	}
	else
	{
		out << " previous Pm Debug Flag is disabled" << endl;
	}

	DataPlane::mPmDebugEnable = enable;

	if (DataPlane::mPmDebugEnable)
	{
		out << " current Pm Debug Flag is enabled, Pm accumulation started" << endl;
	}
	else
	{
		out << " current Pm Debug Flag is disabled, Pm accumulation stopped" << endl;
	}
}

void cmdPmTimeDebug (ostream& out, bool enable)
{
	if (DataPlane::mPmTimeEnable)
	{
		out << " previous Pm Time Flag is enabled" << endl;
	}
	else
	{
		out << " previous Pm Time Flag is disabled" << endl;
	}

	DataPlane::mPmTimeEnable = enable;

	if (DataPlane::mPmTimeEnable)
	{
		out << " current Pm Time Flag is enabled, Pm time debug started" << endl;
	}
	else
	{
		out << " current Pm Time Flag is disabled, Pm time debug stopped" << endl;
	}
}

void cmdDpExceptionDebug (ostream& out, std::string password)
{
    INFN_LOG(SeverityLevel::info) << "Debug Command - Triggering Exception";

    if (password == "infinera")
    {
        out << "Triggering Exception. DP MS going down..." << endl;

        throw 0;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Password does not match. Doing nothing";

        out << "Password does not match. Doing nothing." << endl;
    }
}

void cmdTimePmSubMenu (unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "dump_pm_time",
            [](std::ostream& out, uint32_t opt)
            {
                cmdDumpPmTime(out, opt);
            },
            "Dump Gige Pm time info in last 20 seconds, gige=1, odu=2, otu=3, carrier=4"  );
}

void cmdDcoConnectMonSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "listDcoConnStates",
            [](std::ostream& out)
            {
                cmdListDcoConnStates(out);
            },
            "Dump Dco Connection States"  );

    subMenu -> Insert(
            "listDcoConnErrors",
            [](std::ostream& out)
            {
                cmdListDcoConnErrors(out);
            },
            "Dump Dco Connection Errors"  );

    subMenu -> Insert(
            "listDcoConnIds",
            [](std::ostream& out)
            {
                cmdListDcoConnIds(out);
            },
            "Dump Dco Connection IDs"  );

    subMenu -> Insert(
            "dumpDcoConnStatus",
            [](std::ostream& out)
            {
                cmdDumpDcoConnStatus(out);
            },
            "Dump Dco Connection Status"  );

    subMenu -> Insert(
            "dumpDcoSimConnStatus",
            [](std::ostream& out)
            {
                cmdDumpDcoSimConnStatus(out);
            },
            "Dump Dco Sim Connection Status"  );

    subMenu -> Insert(
            "setDcoSimConnStatus",
            [](std::ostream& out, uint32 connectionId, bool isEn, uint32 connState, uint32 connErr)
            {
                cmdSetDcoSimConnStatus(out, connectionId, isEn, connState, connErr);
            },
            "Set Dco Sim Connection Status"  );

    subMenu -> Insert(
            "dumpDcoSocketAddr",
            [](std::ostream& out, uint32 connectionId)
            {
                cmdDumpDcoSocketAddr(out, connectionId);
            },
            "Dump Dco Socket Address"  );

    subMenu -> Insert(
            "setDcoSocketAddr",
            [](std::ostream& out, uint32 connectionId, std::string sockAddr)
            {
                cmdSetDcoSocketAddr(out, connectionId, sockAddr);
            },
            "Set Dco Socket Address"  );

    subMenu -> Insert(
            "dumpGrpcConnectStatus",
            [](std::ostream& out)
            {
                cmdDumpGrpcConnectStatus(out);
            },
            "Dump GRPC Connection Status"  );

    subMenu -> Insert(
            "getDcoConnectPollEnable",
            [](std::ostream& out, uint32 connectionId)
            {
                cmdGetDcoConnectPollEnable(out, connectionId);
            },
            "Get Dco Connection Monitoring Polling Enable"  );

    subMenu -> Insert(
            "setDcoConnectPollEnable",
            [](std::ostream& out, uint32 connectionId, bool isEnable)
            {
                cmdSetDcoConnectPollEnable(out, connectionId, isEnable);
            },
            "Set Dco Connection Monitoring Polling Enable"  );
}


void cmdSecprocCardAdpSubSubMenu(unique_ptr< Menu > & subMenu)
{
subMenu -> Insert(
            "DumpActiveSubscriptions",
            [](std::ostream& out)
            {
                cmdDumpActiveCardSubscriptions(out);
            },
            " Dump active retries on dco secproc board object" );
}

void cmdSecprocVlanAdpSubSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "DumpActiveSubscriptions",
            [](std::ostream& out)
            {
                cmdDumpActiveVlanSubscriptions(out);
            },
            " Dump active subscriptions on dco secproc vlan objects " );
}

void cmdSecprocIcdpAdpSubSubMenu(unique_ptr< Menu > & subMenu)
{
    subMenu -> Insert(
            "DumpActiveSubscriptions",
            [](std::ostream& out)
            {
                cmdDumpActiveIcdpSubscriptions(out);
            },
            " Dump active subscriptions on dco secproc icdp objects " );
}

void cmdDcoSecprocAdapterMenu(unique_ptr< Menu > & subMenu)
{

}
