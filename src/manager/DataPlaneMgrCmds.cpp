/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#include <iostream>
#include <cli/cli.h>
#include <string>

#include "DataPlaneMgr.h"
#include "DataPlaneMgrCmds.h"
#include "DpCardMgr.h"
#include "DpCarrierMgr.h"
#include "DpGigeClientMgr.h"
#include "DpOtuMgr.h"
#include "DpOduMgr.h"
#include "DpProtoDefs.h"
#include "DpGccMgr.h"
#include "DcoSecProcActor.h"
#include "DpXcMgr.h"

using namespace std;

using chm6_dataplane::Chm6ScgConfig;
using chm6_dataplane::Chm6CarrierConfig;
using chm6_dataplane::Chm6CarrierState;
using chm6_dataplane::Chm6CarrierFault;
using chm6_dataplane::Chm6GigeClientConfig;
using chm6_dataplane::Chm6OtuConfig;
using chm6_dataplane::Chm6OduConfig;
using chm6_dataplane::Chm6XCConfig;

namespace DataPlane
{

const string cStrCliBaseLineId = "1-4-L";

void cmdDpMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{
    //
    // Dp Mgr Cmds - General
    //

    subMenu->Insert(
            "complete_init",
            [](std::ostream& out)
            {
                DataPlane::cmdCompleteInit(out);
            },
            "Trigger Dataplane Mgr to complete init"  );

    subMenu->Insert(
            "force_dco_state",
            [](std::ostream& out, uint32_t state)
            {
                DataPlane::cmdForceDcoState(out, state);
            },
            "Force Dataplane Mgr Dco State", {"0=Unspec,1=BrdInit,2=Confg,3=LoPwr,4=PwrUp,5=PwrDwn,6=Fault"} );

    subMenu->Insert(
            "dump_dco_state",
            [](std::ostream& out)
            {
                DataPlane::cmdDumpDcoState(out);
            },
            "Dump Dco State"  );

    subMenu->Insert(
            "dump_poll_info",
            [](std::ostream& out)
            {
                DataPlane::cmdDumpPollInfo(out);
            },
            "Dump Polling Information for all Managers"  );

    subMenu->Insert(
            "set_state_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetStatePollMode(out, mode);
            },
            "Set State Polling Mode for all Managers"  );

    subMenu->Insert(
            "set_state_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetStatePollInterval(out, intvl);
            },
            "Set State Polling Interval for all Managers"  );

    subMenu->Insert(
            "set_fault_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetFaultPollMode(out, mode);
            },
            "Set Fault Polling Mode for all Managers"  );

    subMenu->Insert(
            "set_fault_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetFaultPollInterval(out, intvl);
            },
            "Set Fault Polling Interval for all Managers"  );

    subMenu->Insert(
            "list_obj_mgrs",
            [](std::ostream& out )
            {
                DataPlane::cmdListObjMgrs(out);
            },
            "List the Object Managers"  );

    subMenu->Insert(
            "list_config_cache",
            [](std::ostream& out )
            {
                DataPlane::cmdListConfigCache(out);
            },
            "List the Config Cache for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)");

    subMenu->Insert(
            "clear_config_cache",
            [](std::ostream& out, uint32_t mgrIdx )
            {
                DataPlane::cmdClearConfigCache(out, mgrIdx);
            },
            "Clear the Config Cache for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)");

    subMenu->Insert(
            "delete_config_cache",
            [](std::ostream& out, uint32_t mgrIdx, string configId )
            {
                DataPlane::cmdClearConfigCache(out, mgrIdx, configId);
            },
            "Clear the Config Cache for mgrIdx and configId");

    subMenu->Insert(
            "dump_config_cache",
            [](std::ostream& out, uint32_t mgrIdx )
            {
                DataPlane::cmdDumpConfigCache(out, mgrIdx);
            },
            "Dump the Config Cache for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)");

    subMenu->Insert(
            "dump_config_cache_obj",
            [](std::ostream& out, uint32_t mgrIdx, string configId )
            {
                DataPlane::cmdDumpConfigCache(out, mgrIdx, configId);
            },
            "Dump the Config Cache for mgrIdx and configId");

    subMenu->Insert(
            "sync_config_cache",
            [](std::ostream& out, uint32_t mgrIdx )
            {
                DataPlane::cmdSyncConfigCache(out, mgrIdx);
            },
            "Sync the Config Cache for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)");

    subMenu->Insert(
            "list_state_cache",
            [](std::ostream& out )
            {
                DataPlane::cmdListStateCache(out);
            },
            "List the State Cache for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)");

    subMenu->Insert(
            "dump_state_cache",
            [](std::ostream& out, uint32_t mgrIdx, std::string stateId )
            {
                DataPlane::cmdDumpStateCache(out, mgrIdx, stateId);
            },
            "Dump the State Cache for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)"
            "  (use stateId = ALL to dump all state)");

    subMenu->Insert(
            "dump_config_proc",
            [](std::ostream& out, uint32_t mgrIdx )
            {
                DataPlane::cmdDumpConfigProcess(out, mgrIdx);
            },
            "Dump the Config Process Data for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)");

    subMenu->Insert(
            "dump_config_proc_last",
            [](std::ostream& out, uint32_t mgrIdx )
            {
                DataPlane::cmdDumpConfigProcessLast(out, mgrIdx);
            },
            "Dump the Config Process Data for mgrIdx (use NUM_OBJ_MGRS or greater for ALL)");

    subMenu->Insert(
            "get_sync_state",
            [](std::ostream& out)
            {
                DataPlane::cmdGetSyncState(out);
            },
            "Is Dataplane MS Sync Ready?");

    subMenu->Insert(
            "force_sync_ready",
            [](std::ostream& out, bool isForce )
            {
                DataPlane::cmdForceSyncReady(out, isForce);
            },
            "Force Sync Ready: 1 or 0");

    subMenu->Insert(
            "force_agent_sync_done",
            [](std::ostream& out, bool isForce )
            {
                DataPlane::cmdForceAgentSyncDone(out, isForce);
            },
            "Force L1Agent sync done: 1 or 0");

    subMenu->Insert(
            "populate_installed_config",
            [](std::ostream& out )
            {
                DataPlane::cmdPopInstalledConfig(out);
            },
            "Trigger installed config retrieval in each Dp Obj Mgr which will get from DCO thru adapters");

    subMenu->Insert(
            "is_config_failure",
            [](std::ostream& out )
            {
                DataPlane::cmdIsConfigFailure(out);
            },
            "Check if config has failed (i.e. config timed out after all retries)");

    subMenu->Insert(
            "config_proc_cancel",
            [](std::ostream& out )
            {
                DataPlane::cmdCancelConfig(out);
            },
            "Cancel config processing (abort retries and fail)");

    subMenu->Insert(
            "dumpDpMgrAll",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpDpMgrAll(out);
            },
            "Dump dp_mgr debug xfr");
}

void cmdDpMgrCardMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{
    subMenu->Insert(
            "dump_card_state",
            [](std::ostream& out)
            {
                DataPlane::cmdDumpCardState(out);
            },
            "Dump Cached Card State Object"   );

    subMenu->Insert(
            "dump_dco_capabilities",
            [](std::ostream& out)
            {
                DataPlane::cmdDumpDcoCapabilities(out);
            },
            "Dump Cached DCO Capabilities State Object"   );

    subMenu->Insert(
            "set_card_state_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetCardStatePollMode(out, mode);
            },
            "Set Card State Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_card_state_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetCardStatePollInterval(out, intvl);
            },
            "Set Card State Polling Interval" );
    subMenu->Insert(
            "set_card_fault_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetCardFaultPollMode(out, mode);
            },
            "Set Card Fault Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_card_fault_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetCardFaultPollInterval(out, intvl);
            },
            "Set Card Fault Polling Interval" );
    subMenu->Insert(
            "dump_card_poll_info",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpCardPollInfo(out);
            },
            "Dump Card Polling Information" );
    subMenu->Insert(
            "set_DCO_fault_report_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetDcoFaultReportMode(out, mode);
            },
            "Set DCO Fault Report Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "force_fan_temps",
            [](std::ostream& out, uint32_t tempHi, uint32_t tempStable)
            {
                DataPlane::cmdForceFanTempFlags(out, tempHi, tempStable);
            },
            "Force temperature flags for Fan algorithm\n"
            "\t1=True, 2=False, other=reset flag to Unspecified",
            {"tempHiFlag" , "tempStableFlag"} );
    subMenu->Insert(
            "dump_fan_temps",
            [](std::ostream& out)
            {
                DataPlane::cmdDumpFanTempFlags(out);
            },
            "Dump temperature flags for Fan algorithm"  );
    subMenu->Insert(
            "dump_card_faults",
            [](std::ostream& out)
            {
                DataPlane::cmdDumpCardFault(out);
            },
            "Dump Cached Card Fault Object"   );
    subMenu->Insert(
            "boot_dco",
            [](std::ostream& out, uint32 action)
            {
                DataPlane::cmdCardBootDco(out, action);
            },
            "Boot Dco\n"
            "\tAction: 0=Warm; 1=Cold; 2=Shutdown" );

}

void cmdDpMgrCarrierMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{
    char *showNxpDbgCmds;
    bool isShowNxpDbgCmds = false;

    if(NULL != (showNxpDbgCmds = getenv("NXP_DBG_CMDS")))
    {
        if(0 == strcmp(showNxpDbgCmds,"true"))
        {
          isShowNxpDbgCmds = true;
        }
    }
    subMenu->Insert(
            "create_carrier",
            [](std::ostream& out, uint carNum, uint clientMode )
            {
                DataPlane::cmdCreateCarrier(out, carNum, clientMode);
            },
            "Trigger Carrier OnCreate to DataPlaneMgr", {"Carrier: 1 - 2, ClientMode: 1 = E, 2 - M"} );


    subMenu->Insert(
            "modify_carrier",
            [](std::ostream& out, uint carNum )
            {
                DataPlane::cmdModifyCarrier(out, carNum);
            },
            "Trigger OnModify to DataPlaneMgr with CarrierConfig object"  );

    if(true == isShowNxpDbgCmds)
    {
        subMenu->Insert(
                "modify_carrier_state",
                [](std::ostream& out, uint carNum, bool isRxAcq)
                {
                DataPlane::cmdModifyCarrierState(out, carNum, isRxAcq);
                },
                "Trigger OnModify to DataPlaneMgr with CarrierState object", {" Carrier: 1 - 2, RxAcq: 1(aquired) 0(not acquired)"}  );

        subMenu->Insert(
                "modify_carrier_fault",
                [](std::ostream& out, uint carNum, bool isOfecUp)
                {
                DataPlane::cmdModifyCarrierFault(out, carNum, isOfecUp);
                },
                "Trigger OnModify to DataPlaneMgr with CarrierFault object", {"Carrier 1 - 2, isOfecUp -  1 = UP, 0 = DOWN" }  );
    }

    subMenu->Insert(
            "delete_carrier",
            [](std::ostream& out, uint carNum )
            {
                DataPlane::cmdDeleteCarrier(out, carNum);
            },
            "Trigger OnDelete to DataPlaneMgr with CarrierConfig object");

    subMenu->Insert(
            "create_scg",
            [](std::ostream& out, uint scgNum )
            {
                DataPlane::cmdCreateScg(out, scgNum);
            },
            "Create SCG" );

    subMenu->Insert(
            "trigger_carrier_pm",
            [](std::ostream& out, uint carNum )
            {
                DataPlane::cmdTriggerCarrierPm(out, carNum);
            },
            "Trigger Carrier PM object stream" );

    subMenu->Insert(
            "dump_carrier_fault",
            [](std::ostream& out, std::string carAid )
            {
                DataPlane::cmdDumpCarrierFault(out, carAid);
            },
            "Dump Cached Carrier Fault Objects"   );
    subMenu->Insert(
            "set_carrier_state_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetCarrierStatePollMode(out, mode);
            },
            "Set Carrier State Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_carrier_state_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetCarrierStatePollInterval(out, intvl);
            },
            "Set Carrier State Polling Interval" );
    subMenu->Insert(
            "set_carrier_fault_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetCarrierFaultPollMode(out, mode);
            },
            "Set Carrier Fault Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_carrier_fault_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetCarrierFaultPollInterval(out, intvl);
            },
            "Set Carrier Fault Polling Interval" );
    subMenu->Insert(
            "dump_carrier_poll_info",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpCarrierPollInfo(out);
            },
            "Dump Carrier Polling Information" );
}

void cmdDpMgrGigeMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{
    subMenu->Insert(
            "create_gige",
            [](std::ostream& out, uint gigeNum )
            {
                DataPlane::cmdCreateGigeClient(out, gigeNum);
            },
            "Trigger OnCreate to DataPlaneMgr with GigeClientConfig object"  );

    subMenu->Insert(
            "modify_gige_loopback",
            [](std::ostream& out, uint gigeNum, uint value )
            {
                DataPlane::cmdModifyGigeClientLoopback(out, gigeNum, value);
            },
            "Trigger OnModify to DataPlaneMgr with GigeClientConfig object"  );

    subMenu->Insert(
            "modify_gige_fecenable",
            [](std::ostream& out, uint gigeNum, uint value )
            {
                DataPlane::cmdModifyGigeClientFecEnable(out, gigeNum, value);
            },
            "Trigger OnModify to DataPlaneMgr with GigeClientConfig object"  );

    subMenu->Insert(
            "delete_gige",
            [](std::ostream& out, uint gigeNum )
            {
                DataPlane::cmdDeleteGigeClient(out, gigeNum);
            },
            "Trigger OnDelete to DataPlaneMgr with GigeClientConfig object"   );

    subMenu->Insert(
            "dump_gigeclient_fault",
            [](std::ostream& out, std::string gigeAid)
            {
                DataPlane::cmdDumpGigeClientFault(out, gigeAid);
            },
            "Dump Cached GigeClient Fault Objects"   );

    subMenu->Insert(
            "dump_gigeclient_pm",
            [](std::ostream& out, std::string gigeAid)
            {
                DataPlane::cmdDumpGigeClientPm(out, gigeAid);
            },
            "Dump Cached GigeClient Pm Objects"   );
    subMenu->Insert(
            "set_gige_state_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetGigeClientStatePollMode(out, mode);
            },
            "Set GigeClient State Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_gige_state_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetGigeClientStatePollInterval(out, intvl);
            },
            "Set GigeClient State Polling Interval" );
    subMenu->Insert(
            "set_gige_fault_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetGigeClientFaultPollMode(out, mode);
            },
            "Set GigeClient Fault Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_gige_fault_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetGigeClientFaultPollInterval(out, intvl);
            },
            "Set GigeClient Fault Polling Interval" );
    subMenu->Insert(
            "set_gige_lldp_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetGigeClientLldpPollMode(out, mode);
            },
            "Set GigeClient Lldp Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_gige_lldp_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetGigeClientLldpPollInterval(out, intvl);
            },
            "Set GigeClient Lldp Polling Interval" );
    subMenu->Insert(
            "dump_gige_poll_info",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpGigeClientPollInfo(out);
            },
            "Dump GigeClient Polling Information" );
    subMenu->Insert(
            "dump_gige_lldp_counters",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpGigeClientLldpCounters(out);
            },
            "Dump GigeClient Lldp Rx and Tx counters" );
}

void cmdDpMgrOtuMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{
    subMenu->Insert(
            "create_otu",
            [](std::ostream& out, uint otuNum, uint subType)
            {
                DataPlane::cmdCreateOtu(out, otuNum, subType);
            },
            "Trigger Otu OnCreate to DataPlaneMgr", {"Otu: 1 - 32, SubType: 1 - Line, 2 - Client"} );

    subMenu->Insert(
            "delete_otu",
            [](std::ostream& out, uint otuNum, uint subType)
            {
                DataPlane::cmdDeleteOtu(out, otuNum, subType);
            },
            "Trigger Otu OnDelete to DataPlaneMgr", {"Otu: 1 - 32, SubType: 1 - Line, 2 - Client"} );

    subMenu->Insert(
            "dump_otu_state",
            [](std::ostream& out, std::string otuAid)
            {
                DataPlane::cmdDumpOtuState(out, otuAid);
            },
            "Dump Cached Otu State Objects"   );

    subMenu->Insert(
            "dump_otu_fault",
            [](std::ostream& out, std::string otuAid)
            {
                DataPlane::cmdDumpOtuFault(out, otuAid);
            },
            "Dump Cached Otu Fault Objects"   );

    subMenu->Insert(
            "dump_otu_pm",
            [](std::ostream& out, std::string otuAid)
            {
                DataPlane::cmdDumpOtuPm(out, otuAid);
            },
            "Dump Cached Otu Pm Objects"   );

    subMenu->Insert(
            "dump_otu_state_config",
            [](std::ostream& out, std::string otuAid)
            {
                DataPlane::cmdDumpOtuStateConfig(out, otuAid);
            },
            "Dump Cached Otu State Config Attributes"   );

    subMenu->Insert(
            "dump_otu_sd_tracking",
            [](std::ostream& out, std::string otuAid)
            {
                DataPlane::cmdDumpOtuSdTracking(out, otuAid);
            },
            "Dump Cached Otu SD Tracking Attributes" );

    subMenu->Insert(
            "dump_otu_tim_tracking",
            [](std::ostream& out, std::string otuAid)
            {
                DataPlane::cmdDumpOtuTimTracking(out, otuAid);
            },
            "Dump Cached Otu Tim Tracking Attributes" );

    subMenu->Insert(
            "set_otu_tim_alarm_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOtuTimAlarmMode(out, mode);
            },
            "Set Otu TIM Alarm Mode", {"Mode: 0 - disable, 1 - enable"} );

    subMenu->Insert(
            "set_otu_sd_alarm_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOtuSdAlarmMode(out, mode);
            },
            "Set Otu SD Alarm Mode", {"Mode: 0 - disable, 1 - enable"} );

    subMenu->Insert(
            "set_otu_state_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOtuStatePollMode(out, mode);
            },
            "Set Otu State Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_otu_state_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetOtuStatePollInterval(out, intvl);
            },
            "Set Otu State Polling Interval" );
    subMenu->Insert(
            "set_otu_fault_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetOtuFaultPollInterval(out, intvl);
            },
            "Set Otu Fault Polling Interval" );
    subMenu->Insert(
            "set_otu_fault_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOtuFaultPollMode(out, mode);
            },
            "Set Otu Fault Polling Mode", {"Mode: 0 - disable, 1 - enable"} );

    subMenu->Insert(
            "dump_otu_poll_info",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpOtuPollInfo(out);
            },
            "Dump Otu Polling Information" );
}

void cmdDpMgrOduMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{
    subMenu->Insert(
            "create_odu",
            [](std::ostream& out, uint otuNum, uint subType)
            {
                DataPlane::cmdCreateOdu(out, otuNum, subType);
            },
            "Trigger Odu OnCreate to DataPlaneMgr", {"Odu: 1 - 32, SubType: 1 - Line, 2 - Client"} );

    subMenu->Insert(
            "create_odu_aid",
            [](std::ostream& out, string oduAid, uint32 oduNum )
            {
                DataPlane::cmdCreateOduAid(out, oduAid, oduNum);
            },
            "Trigger OnCreate to DataPlaneMgr with OduConfig object"
              );

    subMenu->Insert(
            "create_odu_new_aid",
            [](std::ostream& out, uint oduNum, uint otuNum, bool isOdu4i )
            {
                DataPlane::cmdCreateOduNewAid(out, oduNum, otuNum, isOdu4i);
            },
            "Trigger OnCreate to DataPlaneMgr with OduConfig object \n"
            "\t\t oduId otuId isOdu4i"  );

    subMenu->Insert(
            "delete_odu",
            [](std::ostream& out, uint oduNum, uint subType)
            {
                DataPlane::cmdDeleteOdu(out, oduNum, subType);
            },
            "Trigger Odu OnDelete to DataPlaneMgr", {"Odu: 1 - 32, SubType: 1 - Line, 2 - Client"} );

    subMenu->Insert(
            "delete_odu_aid",
            [](std::ostream& out, string oduAid)
            {
                DataPlane::cmdDeleteOduStr(out, oduAid);
            },
            "Trigger OnDelete to DataPlaneMgr with OduConfig object"   );

    subMenu->Insert(
            "dump_odu_state",
            [](std::ostream& out, std::string oduAid)
            {
                DataPlane::cmdDumpOduState(out, oduAid);
            },
            "Dump Cached Odu State Objects"   );

    subMenu->Insert(
            "dump_odu_fault",
            [](std::ostream& out, std::string oduAid)
            {
                DataPlane::cmdDumpOduFault(out, oduAid);
            },
            "Dump Cached Odu Fault Objects"   );

    subMenu->Insert(
            "dump_odu_pm",
            [](std::ostream& out, std::string oduAid)
            {
                DataPlane::cmdDumpOduPm(out, oduAid);
            },
            "Dump Cached Odu Pm Objects"   );

    subMenu->Insert(
            "dump_odu_state_config",
            [](std::ostream& out, std::string oduAid)
            {
                DataPlane::cmdDumpOduStateConfig(out, oduAid);
            },
            "Dump Cached Odu State Config Attributes"   );

    subMenu->Insert(
            "dump_odu_sd_tracking",
            [](std::ostream& out, std::string oduAid)
            {
                DataPlane::cmdDumpOduSdTracking(out, oduAid);
            },
            "Dump Cached Odu SD Tracking Attributes" );

    subMenu->Insert(
            "set_odu_tim_alarm_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOduTimAlarmMode(out, mode);
            },
            "Set Odu TIM Alarm Mode", {"Mode: 0 - disable, 1 - enable"} );

    subMenu->Insert(
            "set_odu_sd_alarm_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOduSdAlarmMode(out, mode);
            },
            "Set Odu SD Alarm Mode", {"Mode: 0 - disable, 1 - enable"} );

    subMenu->Insert(
            "set_odu_state_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOduStatePollMode(out, mode);
            },
            "Set Odu State Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_odu_state_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetOduStatePollInterval(out, intvl);
            },
            "Set Odu State Polling Interval" );
    subMenu->Insert(
            "set_odu_fault_poll_mode",
            [](std::ostream& out, uint32_t mode )
            {
                DataPlane::cmdSetOduFaultPollMode(out, mode);
            },
            "Set Odu Fault Polling Mode", {"Mode: 0 - disable, 1 - enable"} );
    subMenu->Insert(
            "set_odu_fault_poll_interval",
            [](std::ostream& out, uint32_t intvl )
            {
                DataPlane::cmdSetOduFaultPollInterval(out, intvl);
            },
            "Set Odu Fault Polling Interval" );
    subMenu->Insert(
            "dump_odu_poll_info",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpOduPollInfo(out);
            },
            "Dump Odu Polling Information" );
}

void cmdDpMgrXconMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{
    subMenu->Insert(
            "create_xcon",
            [](std::ostream& out, string xconAid )
            {
                DataPlane::cmdCreateXcon(out, xconAid);
            },
            "Trigger OnCreate to DataPlaneMgr with XCConfig object"
              );

    subMenu->Insert(
            "create_xpress_xcon",
            [](std::ostream& out, string xconAid, string srcClientAid, string dstClientAid, uint32_t payload )
            {
                DataPlane::cmdCreateXpressXcon(out, xconAid, srcClientAid, dstClientAid, payload);
            },
            "Trigger OnCreate to DataPlaneMgr with Xpress XCConfig object\n <XconAid> <srcClientAid> <dstClientAid> <PayloadType> \n 1=100GBE, 2=400GBE, 3=OTU4, 4=ODU4 "
              );

    subMenu->Insert(
            "delete_xcon",
            [](std::ostream& out, string xconAid )
            {
                DataPlane::cmdDeleteXcon(out, xconAid);
            },
            "Trigger OnDelete to DataPlaneMgr with XCConfig object"
              );

    subMenu->Insert(
            "dump_xpress_oh",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpXpressXconOhFpga(out);
            },
            "Dump Xpress XCON FPGA OH Forwarding Table"
              );
}

void cmdDpMgrGccMgrSubMenu(unique_ptr< cli::Menu > & subMenu)
{

    subMenu->Insert(
            "dump_lldp_frame",
            [](std::ostream& out, uint portId )
            {
                DataPlane::cmdDumpLldpFrame(out, portId);
            },
            "Dump captured lldp frames on a flow id"   );

    subMenu->Insert(
            "dump_lldp",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpLldp(out);
            },
            "Dump lldp control data and stats"   );

    subMenu->Insert(
            "dumpGccMgrAll",
            [](std::ostream& out )
            {
                DataPlane::cmdDumpGccMgrAll(out);
            },
            "Dump GCC Mgr All"   );

}

void cmdCompleteInit(ostream& osOut)
{
    osOut << "DataPlaneMgr Complete Init ******** " << endl;

    DataPlaneMgrSingleton::Instance()->completeInit();

    osOut << "Is Init Complete: " << DataPlaneMgrSingleton::Instance()->getIsInitDone() << endl;

    osOut << "Done *******" << endl;
}

void cmdForceDcoState(ostream& osOut, uint32_t state)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DataPlaneMgr Force Dco State ******** " << endl;

    DpCardMgrSingleton::Instance()->forceDcoState(state, osOut);

    osOut << "Done *******" << endl;
}

void cmdDumpDcoState(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpDcoState ******** " << endl;

    DpCardMgrSingleton::Instance()->dumpDcoState(osOut);

    osOut << "DumpDcoState Done *******" << endl;
}

void cmdDumpPollInfo(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpPollInfo ******** " << endl << endl;

    osOut << "Card" << endl;
    DpCardMgrSingleton::Instance()->dumpCardPollInfo(osOut);
    osOut << "GigeClient" << endl;
    DpGigeClientMgrSingleton::Instance()->dumpGigeClientPollInfo(osOut);
    osOut << "Carrier" << endl;
    DpCarrierMgrSingleton::Instance()->dumpCarrierPollInfo(osOut);
    osOut << "Otu" << endl;
    DpOtuMgrSingleton::Instance()->dumpOtuPollInfo(osOut);
    osOut << "Odu" << endl;
    DpOduMgrSingleton::Instance()->dumpOduPollInfo(osOut);

    osOut << "DumpPollInfo DONE!! ******** " << endl;
}

void cmdSetStatePollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetStatePollMode ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->setCardStatePollMode(mode, osOut);
    DpGigeClientMgrSingleton::Instance()->setGigeClientStatePollMode(mode, osOut);
    DpCarrierMgrSingleton::Instance()->setCarrierStatePollMode(mode, osOut);
    DpOtuMgrSingleton::Instance()->setOtuStatePollMode(mode, osOut);
    DpOduMgrSingleton::Instance()->setOduStatePollMode(mode, osOut);

    osOut << "SetStatePollMode DONE!! ******** " << endl;
}

void cmdSetStatePollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetStatePollInterval ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->setCardStatePollInterval(intvl, osOut);
    DpGigeClientMgrSingleton::Instance()->setGigeClientStatePollInterval(intvl, osOut);
    DpCarrierMgrSingleton::Instance()->setCarrierStatePollInterval(intvl, osOut);
    DpOtuMgrSingleton::Instance()->setOtuStatePollInterval(intvl, osOut);
    DpOduMgrSingleton::Instance()->setOduStatePollInterval(intvl, osOut);

    osOut << "SetStatePollInterval DONE!! ******** " << endl;
}

void cmdSetFaultPollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetFaultPollMode ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->setCardFaultPollMode(mode, osOut);
    DpGigeClientMgrSingleton::Instance()->setGigeClientFaultPollMode(mode, osOut);
    DpCarrierMgrSingleton::Instance()->setCarrierFaultPollMode(mode, osOut);
    DpOtuMgrSingleton::Instance()->setOtuFaultPollMode(mode, osOut);
    DpOduMgrSingleton::Instance()->setOduFaultPollMode(mode, osOut);

    osOut << "SetFaultPollMode DONE!! ******** " << endl;
}

void cmdSetFaultPollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetFaultPollInterval ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->setCardFaultPollInterval(intvl, osOut);
    DpGigeClientMgrSingleton::Instance()->setGigeClientFaultPollInterval(intvl, osOut);
    DpCarrierMgrSingleton::Instance()->setCarrierFaultPollInterval(intvl, osOut);
    DpOtuMgrSingleton::Instance()->setOtuFaultPollInterval(intvl, osOut);
    DpOduMgrSingleton::Instance()->setOduFaultPollInterval(intvl, osOut);

    osOut << "SetFaultPollInterval DONE!! ******** " << endl;
}

void cmdDumpCardState(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpCardState ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->dumpCardCacheState(osOut);

    osOut << "DumpCardState DONE!! ******** " << endl;
}

void cmdDumpDcoCapabilities(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpDcoCapabilities ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->dumpDcoCapabilities(osOut);

    osOut << "DumpDcoCapabilities DONE!! ******** " << endl;
}

void cmdSetCardStatePollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCardStatePollMode ******** " << endl << endl;

    bool enable = mode;
    DpCardMgrSingleton::Instance()->setCardStatePollMode(enable, osOut);

    osOut << "SetCardStatePollMode DONE!! ******** " << endl;
}

void cmdSetCardStatePollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCardStatePollInterval ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->setCardStatePollInterval(intvl, osOut);

    osOut << "SetCardStatePollInterval DONE!! ******** " << endl;
}

void cmdSetCardFaultPollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCardFaultPollMode ******** " << endl << endl;

    bool enable = mode;
    DpCardMgrSingleton::Instance()->setCardFaultPollMode(enable, osOut);

    osOut << "SetCardFaultPollMode DONE!! ******** " << endl;
}

void cmdSetCardFaultPollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCardFaultPollInterval ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->setCardFaultPollInterval(intvl, osOut);

    osOut << "SetCardFaultPollInterval DONE!! ******** " << endl;
}

void cmdDumpCardPollInfo(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpCardPollInfo ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->dumpCardPollInfo(osOut);

    osOut << "DumpCardPollInfo DONE!! ******** " << endl;
}

void cmdSetDcoFaultReportMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "setDcoFaultReportMode ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->setDcoFaultReportMode(osOut, mode);

    osOut << "setDcoFaultReportMode DONE!! ******** " << endl;
}

void cmdForceFanTempFlags(ostream& osOut, uint32_t tempHi, uint32_t tempStable)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DataPlaneMgr ForceFanTempFlags: tempHi = " << tempHi
          << " tempStable = " << tempStable << " ******** " << endl;

    DpCardMgrSingleton::Instance()->forceFanTempFlags(tempHi, tempStable, osOut);

    osOut << "Done *******" << endl;
}

void cmdDumpFanTempFlags(std::ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpFanTempFlags ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->dumpFanTempFlags(osOut);

    osOut << endl << "DumpFanTempFlags Done *******" << endl;
}

void cmdDumpCardFault(std::ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpCardFault ******** " << endl << endl;

    DpCardMgrSingleton::Instance()->dumpCardCacheFault(osOut);

    osOut << "DumpCardState DONE!! ******** " << endl;
}

void cmdCardBootDco(ostream& osOut, uint32 bootAction)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DcoBoot - bootAction: " << bootAction << " ******** " << endl;

    INFN_LOG(SeverityLevel::info) << "DcoBoot - bootAction: " << bootAction;

    hal_common::BoardAction dcoAction;
    switch (bootAction)
    {
        case 0:
            dcoAction = hal_common::BOARD_ACTION_RESTART_WARM;
            break;
        case 1:
            dcoAction = hal_common::BOARD_ACTION_RESTART_COLD;
            break;
        case 2:
            dcoAction = hal_common::BOARD_ACTION_GRACEFUL_SHUTDOWN;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << "Invalid DcoBoot - bootAction: " << bootAction << " Aborting!";
            osOut << "Invalid DcoBoot - bootAction: " << bootAction << " Aborting!" << endl;
            return;
    }

    chm6_common::Chm6DcoConfig cardCfg;
    cardCfg.mutable_base_state()->mutable_config_id()->set_value("1");

    cardCfg.set_dco_card_action(dcoAction);

    DataPlaneMgrSingleton::Instance()->onCreate(&cardCfg);

    osOut << "Dco Card Action Processing ******** " << endl << endl;
}

void cmdCreateCarrier(ostream& osOut, uint carNum, uint clientMode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "CreateCarrier ******** " << endl;

    string carAid = cStrCliBaseLineId + to_string(carNum) + "-1";

    osOut << "   Carrier AID: " << carAid << endl;

    Chm6CarrierConfig createCarCfg;
    createCarCfg.mutable_base_config()->mutable_config_id()->set_value(carAid);
    createCarCfg.mutable_hal()->mutable_enable()->set_value(false);
    createCarCfg.mutable_hal()->mutable_frequency()->set_value(193100000000000);
    createCarCfg.mutable_hal()->mutable_frequency_offset()->set_value(0);
    createCarCfg.mutable_hal()->mutable_target_power()->set_value(-6);
    createCarCfg.mutable_hal()->mutable_tx_cd_pre_comp()->set_value(0);
    if (clientMode == 2)
    {
        createCarCfg.mutable_hal()->mutable_mode()->mutable_application_code()->set_value("P_DL");
        createCarCfg.mutable_hal()->mutable_mode()->mutable_baud()->set_value(95.2965203);
        createCarCfg.mutable_hal()->mutable_mode()->mutable_capacity()->set_value(800);
        createCarCfg.mutable_hal()->mutable_mode()->set_client(hal_common::CLIENT_MODE_LXTP_M);
    }
    else
    {
        createCarCfg.mutable_hal()->mutable_mode()->mutable_application_code()->set_value("P");
        createCarCfg.mutable_hal()->mutable_mode()->mutable_baud()->set_value(95.6390657);
        createCarCfg.mutable_hal()->mutable_mode()->mutable_capacity()->set_value(400);
        createCarCfg.mutable_hal()->mutable_mode()->set_client(hal_common::CLIENT_MODE_LXTP_E);
    }
    createCarCfg.mutable_hal()->mutable_alarm_threshold()->mutable_dgd_oorh_threshold()->set_value(300);
    createCarCfg.mutable_hal()->mutable_alarm_threshold()->mutable_post_fec_q_signal_degrade_threshold()->set_value(18);
    createCarCfg.mutable_hal()->mutable_alarm_threshold()->mutable_post_fec_q_signal_degrade_hysteresis()->set_value(2.5);
    createCarCfg.mutable_hal()->mutable_alarm_threshold()->mutable_pre_fec_q_signal_degrade_threshold()->set_value(6);
    createCarCfg.mutable_hal()->mutable_alarm_threshold()->mutable_pre_fec_q_signal_degrade_hysteresis()->set_value(0.5);
    createCarCfg.mutable_hal()->mutable_target_power()->set_value(12.34);
    createCarCfg.mutable_hal()->mutable_tx_cd_pre_comp()->set_value(9.87);
    DataPlaneMgrSingleton::Instance()->onCreate(&createCarCfg);

    Chm6CarrierConfig modifyCarCfg;
    modifyCarCfg.mutable_base_config()->mutable_config_id()->set_value(carAid);
    modifyCarCfg.mutable_hal()->mutable_enable()->set_value(true);
    DataPlaneMgrSingleton::Instance()->onModify(&modifyCarCfg);

    osOut << "CreateCarrier DONE!!!" << endl << endl;
}

void cmdModifyCarrier(ostream& osOut, uint carNum)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "ModifyCarrier ******** " << endl;

    string carAid = cStrCliBaseLineId + to_string(carNum) + "-1";

    osOut << "   Carrier AID: " << carAid << endl;

    Chm6CarrierConfig* pCarCfg = new Chm6CarrierConfig();

    pCarCfg->mutable_base_config()->mutable_config_id()->set_value(carAid);
    pCarCfg->mutable_hal()->mutable_enable()->set_value(false);
    pCarCfg->mutable_hal()->mutable_frequency()->set_value(0.0);
    pCarCfg->mutable_hal()->mutable_frequency_offset()->set_value(0.0);
    pCarCfg->mutable_hal()->mutable_target_power()->set_value(0.0);
    pCarCfg->mutable_hal()->mutable_tx_cd_pre_comp()->set_value(0.0);

    DataPlaneMgrSingleton::Instance()->onModify(pCarCfg);

    delete pCarCfg;

    osOut << "ModifyCarrier DONE!!!" << endl << endl;
}

void cmdModifyCarrierState(ostream& osOut, uint carNum, bool isRxAcq)
{
    osOut << "modify_carrier_state ******** " << endl;

    string carAid = cStrCliBaseLineId + to_string(carNum) + "-1";

    osOut << "   Carrier AID: " << carAid << endl;

    DpCarrierMgrSingleton::Instance()->GetmspSecProcCarrierCfgHdlr()->onModify(carAid, isRxAcq);

    osOut << "modify_carrier_state DONE!!!" << endl << endl;
}

void cmdModifyCarrierFault(ostream& osOut, uint carNum, bool isOfecUp)
{

    osOut << "modify_carrier_fault  ******** " << endl;

    string carAid = cStrCliBaseLineId + to_string(carNum) + "-1";

    osOut << "   Carrier AID: " << carAid << endl;

    DpCarrierMgrSingleton::Instance()->GetmspSecProcCarrierCfgHdlr()->onModify(carAid, isOfecUp);

    osOut << "modify_carrier_fault DONE!!!" << endl << endl;
}



void cmdDeleteCarrier(ostream& osOut, uint carNum)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DeleteCarrier ******** " << endl;

    string carAid = cStrCliBaseLineId + to_string(carNum) + "-1";

    osOut << "   Carrier AID: " << carAid << endl;

    Chm6CarrierConfig* pCarCfg = new Chm6CarrierConfig();

    pCarCfg->mutable_base_config()->mutable_config_id()->set_value(carAid);

    DataPlaneMgrSingleton::Instance()->onDelete(pCarCfg);

    delete pCarCfg;

    osOut << "DeleteCarrier DONE!!!" << endl << endl;
}

void cmdCreateScg(ostream& osOut, uint scgNum)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "CreateScg ******** " << endl;

    string scgAid = "1-7-L" + to_string(scgNum);

    osOut << "   SCG AID: " << scgNum << endl;

    Chm6ScgConfig* pScgCfg = new Chm6ScgConfig();

    pScgCfg->mutable_base_config()->mutable_config_id()->set_value(scgAid);
    pScgCfg->mutable_hal()->set_supported_facilities(wrapper::BOOL_FALSE);

    DataPlaneMgrSingleton::Instance()->onCreate(pScgCfg);

    delete pScgCfg;

    osOut << "CreateScg DONE!!!" << endl << endl;
}

void cmdTriggerCarrierPm(ostream& osOut, uint carNum)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "TriggerCarrierPm ******** " << endl;

    string carAid = "1-4-L" + to_string(carNum) + "-1";

    DataPlane::DpMsCarrierPMContainer carrierAdPm;
    carrierAdPm.aid.assign(carAid);
    carrierAdPm.carrierId = carNum;
    carrierAdPm.rx.acquisition_state = DpMsCarrierRx::ACQ_RX_ACTIVE;
    carrierAdPm.rx.pre_fec_ber = 0;
    carrierAdPm.rx.pre_fec_q = 0;
    carrierAdPm.rx.post_fec_q = 0;
    carrierAdPm.rx.snr = 0;
    carrierAdPm.rx.pass_frame_count = 0;
    carrierAdPm.rx.failed_frame_count = 0;
    carrierAdPm.rx.corrected_bits = 0;
    carrierAdPm.rx.fas_q = 0;
    carrierAdPm.rx.cd = 0;
    carrierAdPm.rx.dgd = 0;
    carrierAdPm.rx.polarization_dependent_loss = 0;
    carrierAdPm.rx.opr = 0;
    carrierAdPm.rx.laser_current = 0;

    carrierAdPm.tx.opt = 0;
    carrierAdPm.tx.laser_current = 0;
    carrierAdPm.tx.cd = 0;
    carrierAdPm.tx.edfa_input_power = 0;
    carrierAdPm.tx.edfa_output_power = 0;
    carrierAdPm.tx.edfa_pump_current = 0;

    DataPlane::DpMsCarrierPMs* pCarrierAdPmList = new DataPlane::DpMsCarrierPMs();
    pCarrierAdPmList->AddPmData(carAid, carNum);
    pCarrierAdPmList->UpdatePmData(carrierAdPm, carNum);

    std::string className = "CARRIER";
    DPMSPmCallbacks* dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    dpMsPmCallbacksSingleton->CallCallback(className, static_cast<DcoBasePM*>(pCarrierAdPmList));

    delete pCarrierAdPmList;

    osOut << "TriggerCarrierPm DONE!!!" << endl << endl;
}

void cmdDumpCarrierFault(ostream& osOut, std::string carAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpCarrierFault ******** " << endl << endl;

    DpCarrierMgrSingleton::Instance()->dumpCarrierCacheFault(carAid, osOut);

    osOut << "DumpCarrierFault DONE!! ******** " << endl;
}

void cmdSetCarrierStatePollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCarrierStatePollMode ******** " << endl << endl;

    bool enable = mode;
    DpCarrierMgrSingleton::Instance()->setCarrierStatePollMode(enable, osOut);

    osOut << "SetCarrierStatePollMode DONE!! ******** " << endl;
}

void cmdSetCarrierStatePollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCarrierStatePollInterval ******** " << endl << endl;

    DpCarrierMgrSingleton::Instance()->setCarrierStatePollInterval(intvl, osOut);

    osOut << "SetCarrierStatePollInterval DONE!! ******** " << endl;
}

void cmdSetCarrierFaultPollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCarrierFaultPollMode ******** " << endl << endl;

    bool enable = mode;
    DpCarrierMgrSingleton::Instance()->setCarrierFaultPollMode(enable, osOut);

    osOut << "SetCarrierFaultPollMode DONE!! ******** " << endl;
}

void cmdSetCarrierFaultPollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetCarrierFaultPollInterval ******** " << endl << endl;

    DpCarrierMgrSingleton::Instance()->setCarrierFaultPollInterval(intvl, osOut);

    osOut << "SetCarrierFaultPollInterval DONE!! ******** " << endl;
}

void cmdDumpCarrierPollInfo(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpCarrierPollInfo ******** " << endl << endl;

    DpCarrierMgrSingleton::Instance()->dumpCarrierPollInfo(osOut);

    osOut << "DumpCarrierPollInfo DONE!! ******** " << endl;
}

void cmdCreateGigeClient(ostream& osOut, uint gigeNum)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "CreateGigeClient ******** " << endl;

    string gigeAid = "1-4-T" + to_string(gigeNum);

    osOut << "   GigeClient AID: " << gigeAid << endl;

    Chm6GigeClientConfig* pGigeCfg = new Chm6GigeClientConfig();

    pGigeCfg->mutable_base_config()->mutable_config_id()->set_value(gigeAid);
    pGigeCfg->mutable_hal()->set_rate(hal_dataplane::PORT_RATE_ETH100G);
    pGigeCfg->mutable_hal()->set_loopback(hal_dataplane::LOOP_BACK_TYPE_OFF);
    pGigeCfg->mutable_hal()->mutable_fec_enable()->set_value(false);
    pGigeCfg->mutable_hal()->mutable_mtu()->set_value(10000);
    pGigeCfg->mutable_hal()->mutable_mode()->mutable_rx_snoop_enable()->set_value(false);
    pGigeCfg->mutable_hal()->mutable_mode()->mutable_tx_snoop_enable()->set_value(false);
    pGigeCfg->mutable_hal()->mutable_mode()->mutable_rx_drop_enable()->set_value(false);
    pGigeCfg->mutable_hal()->mutable_mode()->mutable_tx_drop_enable()->set_value(false);
    pGigeCfg->mutable_hal()->set_auto_rx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);
    pGigeCfg->mutable_hal()->set_auto_tx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);
    //pGigeCfg->mutable_hal()->set_force_rx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);
    //pGigeCfg->mutable_hal()->set_force_tx(hal_dataplane::MAINTENANCE_SIGNAL_IDLE);

    DataPlaneMgrSingleton::Instance()->onCreate(pGigeCfg);

    delete pGigeCfg;

    osOut << "CreateGigeClient DONE!!!" << endl << endl;
}

void cmdModifyGigeClientLoopback(ostream& osOut, uint gigeNum, uint value)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "ModifyGigeClientLoopback ******** " << endl;

    string gigeAid = "1-4-T" + to_string(gigeNum);

    osOut << "   GigeClient AID: " << gigeAid << endl;

    Chm6GigeClientConfig* pGigeCfg = new Chm6GigeClientConfig();

    hal_dataplane::LoopBackType loopBack;
    switch (value)
    {
        case 1:  loopBack = hal_dataplane::LOOP_BACK_TYPE_OFF;      break;
        case 2:  loopBack = hal_dataplane::LOOP_BACK_TYPE_FACILITY; break;
        case 3:  loopBack = hal_dataplane::LOOP_BACK_TYPE_TERMINAL; break;
        default: loopBack = hal_dataplane::LOOP_BACK_TYPE_OFF;      break;
    }
    pGigeCfg->mutable_base_config()->mutable_config_id()->set_value(gigeAid);
    pGigeCfg->mutable_hal()->set_loopback(loopBack);

    DataPlaneMgrSingleton::Instance()->onModify(pGigeCfg);

    delete pGigeCfg;

    osOut << "ModifyGigeClient DONE!!!" << endl << endl;
}

void cmdModifyGigeClientFecEnable(ostream& osOut, uint gigeNum, uint value)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "ModifyGigeClientFecEnable ******** " << endl;

    string gigeAid = "1-4-T" + to_string(gigeNum);

    osOut << "   GigeClient AID: " << gigeAid << endl;

    Chm6GigeClientConfig* pGigeCfg = new Chm6GigeClientConfig();

    bool enable = (0 == value) ? false : true;
    pGigeCfg->mutable_base_config()->mutable_config_id()->set_value(gigeAid);
    pGigeCfg->mutable_hal()->mutable_fec_enable()->set_value(enable);

    DataPlaneMgrSingleton::Instance()->onModify(pGigeCfg);

    delete pGigeCfg;

    osOut << "ModifyGigeClient DONE!!!" << endl << endl;
}

void cmdDeleteGigeClient(ostream& osOut, uint gigeNum)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DeleteGigeClient ******** " << endl;

    string gigeAid = "1-4-T" + to_string(gigeNum);

    osOut << "   GigeClient AID: " << gigeAid << endl;

    Chm6GigeClientConfig* pGigeCfg = new Chm6GigeClientConfig();

    pGigeCfg->mutable_base_config()->mutable_config_id()->set_value(gigeAid);

    DataPlaneMgrSingleton::Instance()->onDelete(pGigeCfg);

    delete pGigeCfg;

    osOut << "DeleteGigeClient DONE!!!" << endl << endl;
}

void cmdDumpGigeClientFault(ostream& osOut, std::string gigeAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpGigeClientFault ******** " << endl << endl;

    GigeClientStateCollector* pGigeClientStateCol;

    DpGigeClientMgrSingleton::Instance()->dumpGigeClientCacheFault(gigeAid, osOut);

    osOut << "DumpGigeClientFault DONE!! ******** " << endl;
}

void cmdDumpGigeClientPm(ostream& osOut, std::string gigeAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpGigeClientPm ******** " << endl << endl;

    GigeClientStateCollector* pGigeClientStateCol;

    DpGigeClientMgrSingleton::Instance()->dumpGigeClientCachePm(gigeAid, osOut);

    osOut << "DumpGigeClientPm DONE!! ******** " << endl;
}

void cmdSetGigeClientStatePollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetGigeClientStatePollMode ******** " << endl << endl;

    bool enable = mode;
    DpGigeClientMgrSingleton::Instance()->setGigeClientStatePollMode(enable, osOut);

    osOut << "SetGigeClientStatePollMode DONE!! ******** " << endl;
}

void cmdSetGigeClientStatePollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetGigeClientStatePollInterval ******** " << endl << endl;

    DpGigeClientMgrSingleton::Instance()->setGigeClientStatePollInterval(intvl, osOut);

    osOut << "SetGigeClientStatePollInterval DONE!! ******** " << endl;
}

void cmdSetGigeClientFaultPollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetGigeClientFaultPollMode ******** " << endl << endl;

    bool enable = mode;
    DpGigeClientMgrSingleton::Instance()->setGigeClientFaultPollMode(enable, osOut);

    osOut << "SetGigeClientFaultPollMode DONE!! ******** " << endl;
}

void cmdSetGigeClientFaultPollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetGigeClientFaultPollInterval ******** " << endl << endl;

    DpGigeClientMgrSingleton::Instance()->setGigeClientFaultPollInterval(intvl, osOut);

    osOut << "SetGigeClientFaultPollInterval DONE!! ******** " << endl;
}

void cmdSetGigeClientLldpPollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetGigeClientLldpPollMode ******** " << endl << endl;

    bool enable = mode;
    DpGigeClientMgrSingleton::Instance()->setGigeClientLldpPollMode(enable, osOut);

    osOut << "SetGigeClientLldpPollMode DONE!! ******** " << endl;
}

void cmdSetGigeClientLldpPollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetGigeClientLldpPollInterval ******** " << endl << endl;

    DpGigeClientMgrSingleton::Instance()->setGigeClientLldpPollInterval(intvl, osOut);

    osOut << "SetGigeClientLldpPollInterval DONE!! ******** " << endl;
}

void cmdDumpGigeClientPollInfo(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpGigeClientPollInfo ******** " << endl << endl;

    DpGigeClientMgrSingleton::Instance()->dumpGigeClientPollInfo(osOut);

    osOut << "DumpGigeClientPollInfo DONE!! ******** " << endl;
}

void cmdDumpGigeClientLldpCounters    (ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpGigeClientLldpCounters ******** " << endl << endl;
    DpGigeClientMgrSingleton::Instance()->dumpGigeClientLldpCounters(osOut);
    osOut << "DumpGigeClientLldpCounters DONE!! ******** " << endl;
}

void cmdCreateOtu(ostream& osOut, uint otuNum, uint subType)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "CreateOtu ******** " << endl;

    bool isLine = (subType == 1) ? true : false;
    std::string otuAid = (true == isLine) ? ("1-7-L" + to_string(otuNum) + "-1") : ("1-7-T" + to_string(otuNum));

    osOut << "   Otu AID: " << otuAid << endl;

    Chm6OtuConfig* pOtuCfg = new Chm6OtuConfig();

    if (true == isLine)
    {
        pOtuCfg->mutable_hal()->set_config_svc_type(hal_common::BAND_WIDTH_OTUCNI);
    }
    else
    {
        pOtuCfg->mutable_hal()->set_config_svc_type(hal_common::BAND_WIDTH_OTU4);
    }

    pOtuCfg->mutable_base_config()->mutable_config_id()->set_value(otuAid);
    pOtuCfg->mutable_hal()->set_service_mode(hal_common::SERVICE_MODE_ADAPTATION);
    pOtuCfg->mutable_hal()->mutable_otu_ms_config()->mutable_fac_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
    pOtuCfg->mutable_hal()->mutable_otu_ms_config()->mutable_fac_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
    pOtuCfg->mutable_hal()->mutable_otu_ms_config()->mutable_term_ms()->set_forced_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
    pOtuCfg->mutable_hal()->mutable_otu_ms_config()->mutable_term_ms()->set_auto_ms(hal_common::MAINTENANCE_ACTION_NOREPLACE);
    pOtuCfg->mutable_hal()->set_loopback(hal_common::LOOPBACK_TYPE_NONE);
    pOtuCfg->mutable_hal()->set_loopback_behavior(hal_common::TERM_LOOPBACK_BEHAVIOR_BRIDGESIGNAL);
    pOtuCfg->mutable_hal()->mutable_otu_prbs_config()->mutable_fac_prbs_gen()->set_value(false);
    pOtuCfg->mutable_hal()->mutable_otu_prbs_config()->mutable_fac_prbs_mon()->set_value(false);
    pOtuCfg->mutable_hal()->mutable_otu_prbs_config()->mutable_term_prbs_gen()->set_value(false);
    pOtuCfg->mutable_hal()->mutable_otu_prbs_config()->mutable_term_prbs_mon()->set_value(false);
    pOtuCfg->mutable_hal()->set_carrier_channel(hal_common::CARRIER_CHANNEL_ONE);
    pOtuCfg->mutable_hal()->mutable_alarm_threshold()->mutable_signal_degrade_interval()->set_value(7);
    pOtuCfg->mutable_hal()->mutable_alarm_threshold()->mutable_signal_degrade_threshold()->set_value(30);
    pOtuCfg->mutable_hal()->mutable_alarm_threshold()->set_tti_mismatch_type(hal_common::TTI_MISMATCH_TYPE_DISABLED);

    pOtuCfg->mutable_hal()->clear_tx_tti_cfg();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(0);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOtuCfg->mutable_hal()->mutable_tx_tti_cfg()->mutable_tti()->insert(ttiPair);
    }

    pOtuCfg->mutable_hal()->mutable_alarm_threshold()->clear_tti_expected();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(0);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOtuCfg->mutable_hal()->mutable_alarm_threshold()->mutable_tti_expected()->mutable_tti()->insert(ttiPair);
    }

    DataPlaneMgrSingleton::Instance()->onCreate(pOtuCfg);

    delete pOtuCfg;

    osOut << "CreateOtu DONE!!!" << endl << endl;
}

void cmdDeleteOtu(ostream& osOut, uint otuNum, uint subType)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DeleteOtu ******** " << endl;

    bool isLine = (subType == 1) ? true : false;
    std::string otuAid = (true == isLine) ? ("1-7-L" + to_string(otuNum) + "-1") : ("1-7-T" + to_string(otuNum));

    osOut << "   Otu AID: " << otuAid << endl;

    Chm6OtuConfig* pOtuCfg = new Chm6OtuConfig();

    pOtuCfg->mutable_base_config()->mutable_config_id()->set_value(otuAid);

    DataPlaneMgrSingleton::Instance()->onDelete(pOtuCfg);

    delete pOtuCfg;

    osOut << "DeleteOtu DONE!!!" << endl << endl;
}

void cmdDumpOtuState(ostream& osOut, std::string otuAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpOtuState ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->dumpOtuCacheState(otuAid, osOut);

    osOut << "DumpOtuState DONE!! ******** " << endl;
}

void cmdDumpOtuFault(ostream& osOut, std::string otuAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpOtuFault ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->dumpOtuCacheFault(otuAid, osOut);

    osOut << "DumpOtuFault DONE!! ******** " << endl;
}

void cmdDumpOtuPm(ostream& osOut, std::string otuAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "OtuDumpPm ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->dumpOtuCachePm(otuAid, osOut);

    osOut << "DumpOtuPm DONE!! ******** " << endl;
}

void cmdDumpOtuStateConfig(ostream& osOut, std::string otuAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "OtuDumpStateConfig ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->dumpOtuCacheStateConfig(otuAid, osOut);

    osOut << "DumpOtuStateConfig DONE!! ******** " << endl;
}

void cmdDumpOtuSdTracking(ostream& osOut, std::string otuAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "OtuDumpSdTracking ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->dumpOtuSdTracking(otuAid, osOut);

    osOut << "DumpOtuSdTracking DONE!! ******** " << endl;
}

void cmdDumpOtuTimTracking(ostream& osOut, std::string otuAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "OtuDumpTimTracking ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->dumpOtuTimTracking(otuAid, osOut);

    osOut << "DumpOtuTimTracking DONE!! ******** " << endl;
}

void cmdSetOtuTimAlarmMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOtuTimAlarmMode ******** " << endl << endl;

    bool enable = mode;
    DpOtuMgrSingleton::Instance()->setOtuTimAlarmMode(enable, osOut);

    osOut << "SetOtuTimAlarmMode DONE!! ******** " << endl;
}

void cmdSetOtuSdAlarmMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOtuSdAlarmMode ******** " << endl << endl;

    bool enable = mode;
    DpOtuMgrSingleton::Instance()->setOtuSdAlarmMode(enable, osOut);

    osOut << "SetOtuSdAlarmMode DONE!! ******** " << endl;
}

void cmdSetOtuStatePollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOtuStatePollMode ******** " << endl << endl;

    bool enable = mode;
    DpOtuMgrSingleton::Instance()->setOtuStatePollMode(enable, osOut);

    osOut << "SetOtuStatePollMode DONE!! ******** " << endl;
}

void cmdSetOtuStatePollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOtuStatePollInterval ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->setOtuStatePollInterval(intvl, osOut);

    osOut << "SetOtuStatePollInterval DONE!! ******** " << endl;
}

void cmdSetOtuFaultPollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOtuFaultPollMode ******** " << endl << endl;

    bool enable = mode;
    DpOtuMgrSingleton::Instance()->setOtuFaultPollMode(enable, osOut);

    osOut << "SetOtuFaultPollMode DONE!! ******** " << endl;
}

void cmdSetOtuFaultPollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOtuFaultPollInterval ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->setOtuFaultPollInterval(intvl, osOut);

    osOut << "SetOtuFaultPollInterval DONE!! ******** " << endl;
}

void cmdDumpOtuPollInfo(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpOtuPollInfo ******** " << endl << endl;

    DpOtuMgrSingleton::Instance()->dumpOtuPollInfo(osOut);

    osOut << "DumpOtuPollInfo DONE!! ******** " << endl;
}

void cmdCreateOdu(ostream& osOut, uint oduNum, uint subType)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "CreateOdu ******** " << endl;

    bool isLine = (subType == 1) ? true : false;
    std::string oduAid = (true == isLine) ? ("1-7-L" + to_string(oduNum) + "-1-ODU4i-1") : ("1-7-T" + to_string(oduNum));

    osOut << "   Odu AID: " << oduAid << endl;

    Chm6OduConfig* pOduCfg = new Chm6OduConfig();

    if (true == isLine)
    {
        pOduCfg->mutable_hal()->mutable_odu_index()->set_value(oduNum);
        pOduCfg->mutable_hal()->set_config_svc_type(hal_common::BAND_WIDTH_LO_ODU4I);
        pOduCfg->mutable_hal()->set_ts_granularity(hal_dataplane::TS_GRANULARITY_25G);

        pOduCfg->mutable_hal()->clear_timeslot();
        for (uint keyIndex = 0; keyIndex < 4; keyIndex++)
        {
            hal_common::TsType_TsDataType tsData;
            tsData.mutable_value()->set_value(keyIndex+1);
            google::protobuf::MapPair<uint32_t, hal_common::TsType_TsDataType> tsPair(keyIndex, tsData);
            pOduCfg->mutable_hal()->mutable_timeslot()->mutable_ts()->insert(tsPair);
        }
    }
    else
    {
        pOduCfg->mutable_hal()->mutable_odu_index()->set_value(oduNum+32);

        pOduCfg->mutable_hal()->set_config_svc_type(hal_common::BAND_WIDTH_ODU4);
        pOduCfg->mutable_hal()->set_ts_granularity(hal_dataplane::TS_GRANULARITY_25G); // Should be 50G

        pOduCfg->mutable_hal()->clear_timeslot();
        for (uint keyIndex = 0; keyIndex < 2; keyIndex++)
        {
            hal_common::TsType_TsDataType tsData;
            tsData.mutable_value()->set_value(keyIndex+1);
            google::protobuf::MapPair<uint32_t, hal_common::TsType_TsDataType> tsPair(keyIndex, tsData);
            pOduCfg->mutable_hal()->mutable_timeslot()->mutable_ts()->insert(tsPair);
        }
    }
    pOduCfg->mutable_base_config()->mutable_config_id()->set_value(oduAid);
    pOduCfg->mutable_hal()->set_service_mode(hal_common::SERVICE_MODE_SWITCHING);
    pOduCfg->mutable_hal()->set_loopback(hal_common::LOOPBACK_TYPE_NONE);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_NOREPLACE);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_term_forceaction(hal_common::MAINTENANCE_ACTION_NOREPLACE);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_term_autoaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->set_fac_prbs_gen_bool(wrapper::BOOL_FALSE);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->set_fac_prbs_mon_bool(wrapper::BOOL_FALSE);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->set_term_prbs_gen_bool(wrapper::BOOL_FALSE);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->set_term_prbs_mon_bool(wrapper::BOOL_FALSE);
    pOduCfg->mutable_hal()->mutable_alarm_threshold()->mutable_signal_degrade_interval()->set_value(7);
    pOduCfg->mutable_hal()->mutable_alarm_threshold()->mutable_signal_degrade_threshold()->set_value(30);
    pOduCfg->mutable_hal()->mutable_alarm_threshold()->set_tti_mismatch_type(hal_common::TTI_MISMATCH_TYPE_DISABLED);

    pOduCfg->mutable_hal()->clear_tx_tti_cfg();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(0);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOduCfg->mutable_hal()->mutable_tx_tti_cfg()->mutable_tti()->insert(ttiPair);
    }

    pOduCfg->mutable_hal()->mutable_alarm_threshold()->clear_tti_expected();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(0);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOduCfg->mutable_hal()->mutable_alarm_threshold()->mutable_tti_expected()->mutable_tti()->insert(ttiPair);
    }

    DataPlaneMgrSingleton::Instance()->onCreate(pOduCfg);

    delete pOduCfg;

    osOut << "CreateOdu DONE!!!" << endl << endl;
}

void cmdCreateOduAid(ostream& osOut, std::string oduAid, uint32 oduIdx)
{
    osOut << "CreateOdu " << oduAid << " ******** " << endl;

    Chm6OduConfig* pOduCfg = new Chm6OduConfig();

    pOduCfg->mutable_base_config()->mutable_config_id()->set_value(oduAid);
    pOduCfg->mutable_hal()->set_config_svc_type(hal_common::BAND_WIDTH_100GB_ELAN);
    pOduCfg->mutable_hal()->set_service_mode(hal_common::SERVICE_MODE_SWITCHING);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_SENDOCI);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_term_forceaction(hal_common::MAINTENANCE_ACTION_SENDLCK);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_term_autoaction(hal_common::MAINTENANCE_ACTION_PRBS_X31);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_fac_prbs_gen()->set_value(true);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_fac_prbs_mon()->set_value(false);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_term_prbs_gen()->set_value(true);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_term_prbs_mon()->set_value(false);
    pOduCfg->mutable_hal()->set_ts_granularity(hal_dataplane::TS_GRANULARITY_25G);
    pOduCfg->mutable_hal()->mutable_odu_index()->set_value(oduIdx);

    pOduCfg->mutable_hal()->clear_timeslot();
    for (uint keyIndex = 0; keyIndex < 4; keyIndex++)
    {
        hal_common::TsType_TsDataType tsData;
        tsData.mutable_value()->set_value(keyIndex+1);
        google::protobuf::MapPair<uint32_t, hal_common::TsType_TsDataType> tsPair(keyIndex, tsData);
        pOduCfg->mutable_hal()->mutable_timeslot()->mutable_ts()->insert(tsPair);
    }

    pOduCfg->mutable_hal()->clear_tx_tti_cfg();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(keyIndex);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOduCfg->mutable_hal()->mutable_tx_tti_cfg()->mutable_tti()->insert(ttiPair);
    }

    pOduCfg->mutable_hal()->mutable_alarm_threshold()->clear_tti_expected();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(keyIndex);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOduCfg->mutable_hal()->mutable_alarm_threshold()->mutable_tti_expected()->mutable_tti()->insert(ttiPair);
    }

    pOduCfg->mutable_hal()->mutable_alarm_threshold()->set_tti_mismatch_type(hal_common::TTI_MISMATCH_TYPE_DAPI);

    DataPlaneMgrSingleton::Instance()->onCreate(pOduCfg);

    delete pOduCfg;

    osOut << "CreateOdu DONE!!!" << endl << endl;
}

void cmdCreateOduNewAid(ostream& osOut, uint oduNum, uint otuNum, bool isOdu4i)
{
    osOut << "CreateOdu ******** " << endl;

    string oduAid ="1-4-L" + to_string(otuNum) + "-1-";
    ::infinera::hal::common::v2::BandWidth bandWidth;
    if (isOdu4i)
    {
        oduAid += "ODU4i-" + to_string(oduNum);
        bandWidth = hal_common::BAND_WIDTH_100GB_ELAN;
    }
    else
    {
        oduAid += "ODUflexi-" + to_string(oduNum);
        bandWidth = hal_common::BAND_WIDTH_400GB_ELAN;
    }

    osOut << "   Odu AID: " << oduAid << endl;

    Chm6OduConfig* pOduCfg = new Chm6OduConfig();

    pOduCfg->mutable_base_config()->mutable_config_id()->set_value(oduAid);
    pOduCfg->mutable_hal()->set_config_svc_type(hal_common::BAND_WIDTH_100GB_ELAN);
    pOduCfg->mutable_hal()->set_service_mode(hal_common::SERVICE_MODE_SWITCHING);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_fac_forceaction(hal_common::MAINTENANCE_ACTION_SENDAIS);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_fac_autoaction(hal_common::MAINTENANCE_ACTION_SENDOCI);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_term_forceaction(hal_common::MAINTENANCE_ACTION_SENDLCK);
    pOduCfg->mutable_hal()->mutable_odu_ms_config()->set_term_autoaction(hal_common::MAINTENANCE_ACTION_PRBS_X31);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_fac_prbs_gen()->set_value(true);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_fac_prbs_mon()->set_value(false);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_term_prbs_gen()->set_value(true);
    pOduCfg->mutable_hal()->mutable_odu_prbs_config()->mutable_term_prbs_mon()->set_value(false);
    pOduCfg->mutable_hal()->set_ts_granularity(hal_dataplane::TS_GRANULARITY_25G);
    pOduCfg->mutable_hal()->set_config_svc_type(bandWidth);
    pOduCfg->mutable_hal()->mutable_odu_index()->set_value(oduNum);

    pOduCfg->mutable_hal()->clear_timeslot();
    for (uint keyIndex = 0; keyIndex < 4; keyIndex++)
    {
        hal_common::TsType_TsDataType tsData;
        tsData.mutable_value()->set_value(keyIndex+1);
        google::protobuf::MapPair<uint32_t, hal_common::TsType_TsDataType> tsPair(keyIndex, tsData);
        pOduCfg->mutable_hal()->mutable_timeslot()->mutable_ts()->insert(tsPair);
    }

    pOduCfg->mutable_hal()->clear_tx_tti_cfg();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(keyIndex);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOduCfg->mutable_hal()->mutable_tx_tti_cfg()->mutable_tti()->insert(ttiPair);
    }

    pOduCfg->mutable_hal()->mutable_alarm_threshold()->clear_tti_expected();
    for (uint keyIndex = 0; keyIndex < MAX_TTI_LENGTH; keyIndex++)
    {
        hal_common::TtiType_TtiDataType ttiData;
        ttiData.mutable_value()->set_value(keyIndex);
        google::protobuf::MapPair<uint32_t, hal_common::TtiType_TtiDataType> ttiPair(keyIndex, ttiData);
        pOduCfg->mutable_hal()->mutable_alarm_threshold()->mutable_tti_expected()->mutable_tti()->insert(ttiPair);
    }

    pOduCfg->mutable_hal()->mutable_alarm_threshold()->set_tti_mismatch_type(hal_common::TTI_MISMATCH_TYPE_DAPI);

    DataPlaneMgrSingleton::Instance()->onCreate(pOduCfg);

    delete pOduCfg;

    osOut << "CreateOdu DONE!!!" << endl << endl;
}

void cmdDeleteOdu(ostream& osOut, uint oduNum, uint subType)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DeleteOdu ******** " << endl;

    bool isLine = (subType == 1) ? true : false;
    std::string oduAid = (true == isLine) ? ("1-7-L" + to_string(oduNum) + "-1-ODU4i-1") : ("1-7-T" + to_string(oduNum));

    osOut << "   Odu AID: " << oduAid << endl;

    Chm6OduConfig* pOduCfg = new Chm6OduConfig();

    pOduCfg->mutable_base_config()->mutable_config_id()->set_value(oduAid);

    DataPlaneMgrSingleton::Instance()->onDelete(pOduCfg);

    delete pOduCfg;

    osOut << "DeleteOdu DONE!!!" << endl << endl;
}

void cmdDeleteOduStr(ostream& osOut, std::string oduAid)
{
    osOut << "DeleteOdu " << oduAid << " ******** " << endl;

    Chm6OduConfig* pOduCfg = new Chm6OduConfig();

    pOduCfg->mutable_base_config()->mutable_config_id()->set_value(oduAid);

    DataPlaneMgrSingleton::Instance()->onDelete(pOduCfg);

    delete pOduCfg;

    osOut << "DeleteOdu DONE!!!" << endl << endl;
}

void cmdDumpOduState(ostream& osOut, std::string oduAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpOduState ******** " << endl << endl;

    OduStateCollector* pOduStateCol;

    DpOduMgrSingleton::Instance()->dumpOduCacheState(oduAid, osOut);

    osOut << "DumpOduState DONE!! ******** " << endl;
}

void cmdDumpOduFault(ostream& osOut, std::string oduAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpOduFault ******** " << endl << endl;

    OduStateCollector* pOduStateCol;

    DpOduMgrSingleton::Instance()->dumpOduCacheFault(oduAid, osOut);

    osOut << "DumpOduFault DONE!! ******** " << endl;
}

void cmdDumpOduPm(ostream& osOut, std::string oduAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpOduPm ******** " << endl << endl;

    OduStateCollector* pOduStateCol;

    DpOduMgrSingleton::Instance()->dumpOduCachePm(oduAid, osOut);

    osOut << "DumpOduPm DONE!! ******** " << endl;
}

void cmdDumpOduStateConfig(ostream& osOut, std::string oduAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "OduDumpStateConfig ******** " << endl << endl;

    DpOduMgrSingleton::Instance()->dumpOduCacheStateConfig(oduAid, osOut);

    osOut << "DumpOduStateConfig DONE!! ******** " << endl;
}

void cmdDumpOduSdTracking(ostream& osOut, std::string oduAid)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "OduDumpSdTracking ******** " << endl << endl;

    DpOduMgrSingleton::Instance()->dumpOduSdTracking(oduAid, osOut);

    osOut << "DumpOduSdTracking DONE!! ******** " << endl;
}

void cmdCreateXcon(ostream& osOut)
{
    osOut << "CreateXcon ******** " << endl;

    string xconAid = "1-7-L1-1-ODU4i-1,1-7-T1";

    osOut << "   Xcon AID: " << xconAid << endl;

    Chm6XCConfig* pXconCfg = new Chm6XCConfig();

    pXconCfg->mutable_base_config()->mutable_config_id()->set_value(xconAid);
    pXconCfg->mutable_hal()->mutable_source()->set_value("1-7-L1-1-ODU4i-1");
    pXconCfg->mutable_hal()->mutable_destination()->set_value("1-7-T1");
    pXconCfg->mutable_hal()->set_xc_direction(hal_dataplane::DIRECTION_TWO_WAY);

    DataPlaneMgrSingleton::Instance()->onCreate(pXconCfg);

    delete pXconCfg;

    osOut << "CreateXcon DONE!!!" << endl << endl;
}

void cmdDeleteXcon(ostream& osOut)
{
    osOut << "DeleteXcon ******** " << endl;

    string xconAid = "1-7-L1-1-ODU4i-1,1-7-T1";

    osOut << "   Xcon AID: " << xconAid << endl;

    Chm6XCConfig* pXconCfg = new Chm6XCConfig();

    pXconCfg->mutable_base_config()->mutable_config_id()->set_value(xconAid);

    DataPlaneMgrSingleton::Instance()->onDelete(pXconCfg);

    delete pXconCfg;

    osOut << "DeleteXcon DONE!!!" << endl << endl;
}

void cmdDumpLldpFrame(ostream& osOut, uint portId)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

   osOut << "DumpLldpFrame ******** " << endl << endl;
   DpGccMgrSingleton::Instance()->dumpActLldpPool(portId, osOut);
   osOut << " DumpLldpFrame DONE!! ******** " << endl;
}

void cmdDumpLldp(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

   osOut << "DumpLldp ******** " << endl << endl;
   DpGccMgrSingleton::Instance()->dumpLldp(osOut);
   osOut << " DumpLldp DONE!! ******** " << endl;

}

void cmdDumpGccMgrAll(ostream& osOut)
{
    osOut << "dump_lldp" << endl;
    cmdDumpLldp(osOut);

    for (uint portId = 0; portId < 16; portId++)
    {
        osOut << "dump_lldp_frame " << portId << endl;
        cmdDumpLldpFrame(osOut, portId);
    }
}

void cmdSetOduTimAlarmMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOduTimAlarmMode ******** " << endl << endl;

    bool enable = mode;
    DpOduMgrSingleton::Instance()->setOduTimAlarmMode(enable, osOut);

    osOut << "SetOduTimAlarmMode DONE!! ******** " << endl;
}

void cmdSetOduSdAlarmMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOduSdAlarmMode ******** " << endl << endl;

    bool enable = mode;
    DpOduMgrSingleton::Instance()->setOduSdAlarmMode(enable, osOut);

    osOut << "SetOduSdAlarmMode DONE!! ******** " << endl;
}

void cmdSetOduStatePollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOduStatePollMode ******** " << endl << endl;

    bool enable = mode;
    DpOduMgrSingleton::Instance()->setOduStatePollMode(enable, osOut);

    osOut << "SetOduStatetPollMode DONE!! ******** " << endl;
}

void cmdSetOduStatePollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOduStatePollInterval ******** " << endl << endl;

    DpOduMgrSingleton::Instance()->setOduStatePollInterval(intvl, osOut);

    osOut << "SetOduStatePollInterval DONE!! ******** " << endl;
}

void cmdSetOduFaultPollMode(ostream& osOut, uint32_t mode)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOduFaultPollMode ******** " << endl << endl;

    bool enable = mode;
    DpOduMgrSingleton::Instance()->setOduFaultPollMode(enable, osOut);

    osOut << "SetOduFaultPollMode DONE!! ******** " << endl;
}

void cmdSetOduFaultPollInterval(ostream& osOut, uint32_t intvl)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "SetOduFaultPollInterval ******** " << endl << endl;

    DpOduMgrSingleton::Instance()->setOduFaultPollInterval(intvl, osOut);

    osOut << "SetOduFaultPollInterval DONE!! ******** " << endl;
}

void cmdDumpOduPollInfo(ostream& osOut)
{
    if (false == DataPlaneMgrSingleton::Instance()->isDpMgrInitDone(osOut)) return;

    osOut << "DumpOduPollInfo ******** " << endl << endl;

    DpOduMgrSingleton::Instance()->dumpOduPollInfo(osOut);

    osOut << "DumpOduPollInfo DONE!! ******** " << endl;
}

void cmdCreateXpressXcon(ostream& osOut, std::string xconId, std::string srcClientAid, std::string dstClientAid, uint32_t payload)
{
    osOut << "CreateXpressXcon with ID " << xconId << "   ******** " << endl;

    std::size_t found = xconId.find(",");
    if (found == std::string::npos)
    {
        osOut << "ERROR: Invalid ID. Missing comma : " << xconId;
        return;
    }

    string strSrc = xconId.substr(0, found);
    string strDst = xconId.substr(found + 1);
    bool errorStatus = false;

    Chm6XCConfig* pXcCfg = new Chm6XCConfig();

    pXcCfg->mutable_base_config()->mutable_config_id()->set_value(xconId);

    pXcCfg->mutable_hal()->mutable_destination()->set_value(strDst);
    pXcCfg->mutable_hal()->mutable_source()->set_value(strSrc);

    pXcCfg->mutable_hal()->set_xc_direction(hal_dataplane::DIRECTION_TWO_WAY);

    pXcCfg->mutable_hal()->mutable_regen_source_client()->set_value(srcClientAid);
    pXcCfg->mutable_hal()->mutable_regen_destination_client()->set_value(dstClientAid);
    pXcCfg->mutable_hal()->set_payload_treatment(hal_dataplane::PAYLOAD_TREATMENT_REGEN);
    switch (payload)
    {
        case 1:
            pXcCfg->mutable_hal()->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_100GBE_LAN);
            break;
        case 2:
            pXcCfg->mutable_hal()->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_400GBE_LAN);
            break;
        case 3:
            pXcCfg->mutable_hal()->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_OTU4);
            osOut << "ERROR: OTU4 Payload not supported yet: " << payload;
            errorStatus = true;
            break;
        case 4:
            pXcCfg->mutable_hal()->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_100G);
            break;
        default:
            osOut << "ERROR: Invalid Payload: " << payload;
            errorStatus = true;
            break;
    }

    if (errorStatus == false)
    {
        DataPlaneMgrSingleton::Instance()->onCreate(pXcCfg);
        osOut << "CreateXcon with ID " << xconId << " DONE!!  ******** " << endl;
    }

    delete pXcCfg;
}

void cmdCreateXcon(ostream& osOut, std::string xconId)
{
    osOut << "CreateXcon with ID " << xconId << "   ******** " << endl;

    std::size_t found = xconId.find(",");
    if (found == std::string::npos)
    {
        osOut << "ERROR: Invalid ID. Missing comma : " << xconId;
        return;
    }

    string strSrc = xconId.substr(0, found);
    string strDst = xconId.substr(found + 1);

    Chm6XCConfig* pXcCfg = new Chm6XCConfig();

    pXcCfg->mutable_base_config()->mutable_config_id()->set_value(xconId);

    pXcCfg->mutable_hal()->mutable_destination()->set_value(strDst);
    pXcCfg->mutable_hal()->mutable_source()     ->set_value(strSrc);

    pXcCfg->mutable_hal()->set_xc_direction(hal_dataplane::DIRECTION_TWO_WAY);

    DataPlaneMgrSingleton::Instance()->onCreate(pXcCfg);

    delete pXcCfg;

    osOut << "CreateXcon with ID " << xconId << " DONE!!  ******** " << endl;
}

void cmdDeleteXcon(ostream& osOut, std::string xconId)
{
    osOut << "DeleteXcon with ID " << xconId << "   ******** " << endl;

    std::size_t found = xconId.find(",");
    if (found == std::string::npos)
    {
        osOut << "ERROR: Invalid ID. Missing comma : " << xconId;
        return;
    }

    string strSrc = xconId.substr(0, found);
    string strDst = xconId.substr(found + 1);

    Chm6XCConfig* pXcCfg = new Chm6XCConfig();

    pXcCfg->mutable_base_config()->mutable_config_id()->set_value(xconId);
    pXcCfg->mutable_base_config()->mutable_mark_for_delete()->set_value(true);

    pXcCfg->mutable_hal()->mutable_destination()->set_value(strDst);
    pXcCfg->mutable_hal()->mutable_source()     ->set_value(strSrc);

    DataPlaneMgrSingleton::Instance()->onDelete(pXcCfg);

    delete pXcCfg;

    osOut << "DeleteXcon with ID " << xconId << " DONE!!  ******** " << endl;
}

void cmdDumpXpressXconOhFpga(ostream& osOut)
{
    osOut << "Dump Xpress XCON FPGA OH Forwarding table  ******** " << endl;

    DpXcMgrSingleton::Instance()->dumpXpressXconOhFpga(osOut);
}

void cmdListObjMgrs(ostream& out)
{
    DataPlaneMgrSingleton::Instance()->listObjMgrs(out);
}

void cmdListConfigCache(ostream& out)
{
    DataPlaneMgrSingleton::Instance()->listConfigCache(out, NUM_OBJ_MGRS);
}

void cmdClearConfigCache(ostream& out, uint32_t mgrIdx)
{
    DataPlaneMgrSingleton::Instance()->clearConfigCache(out, mgrIdx);
}

void cmdClearConfigCache(ostream& out, uint32_t mgrIdx, string configId)
{
    DataPlaneMgrSingleton::Instance()->clearConfigCache(out, mgrIdx, configId);
}

void cmdDumpConfigCache(ostream& out, uint32_t mgrIdx)
{
    DataPlaneMgrSingleton::Instance()->dumpConfigCache(out, mgrIdx);
}

void cmdDumpConfigCache(ostream& out, uint32_t mgrIdx, string configId)
{
    DataPlaneMgrSingleton::Instance()->dumpConfigCache(out, mgrIdx, configId);
}

void cmdSyncConfigCache(ostream& out, uint32_t mgrIdx)
{
    DataPlaneMgrSingleton::Instance()->syncConfigCache(out, mgrIdx);
}

void cmdListStateCache(ostream& out)
{
    DataPlaneMgrSingleton::Instance()->listStateCache(out, NUM_OBJ_MGRS);
}

void cmdDumpStateCache(ostream& out, uint32_t mgrIdx, string configId)
{
    DataPlaneMgrSingleton::Instance()->dumpStateCache(out, mgrIdx, configId);
}

void cmdDumpConfigProcess(ostream& out, uint32_t mgrIdx)
{
    DataPlaneMgrSingleton::Instance()->dumpConfigProcess(out, mgrIdx);
}

void cmdDumpConfigProcessLast(ostream& out, uint32_t mgrIdx)
{
    DataPlaneMgrSingleton::Instance()->dumpConfigProcessLast(out, mgrIdx);
}

void cmdGetSyncState(ostream& out)
{
    string statStr;
    if (DataPlaneMgrSingleton::Instance()->getIsBrdInitDone())
    {
        if (DataPlaneMgrSingleton::Instance()->getIsBrdInitFail())
        {
            statStr = "FAILED";
        }
        else
        {
            statStr = "SUCCESS";
        }
    }
    else
    {
        statStr = "NOT DONE";
    }

    out << endl;

    out << "BoardInit Status       : " << statStr << endl;
    out << "Power Mode             : " << DataPlaneMgrSingleton::Instance()->getPwrModeStr() << endl;
    out << "DP Init Done           : " << (DataPlaneMgrSingleton::Instance()->getIsInitDone() ? "true" : "false") << endl;
    out << "DP OnResync Done       : " << (DataPlaneMgrSingleton::Instance()->getIsOnResyncDone() ? "true" : "false") << endl;
    out << "DP Sync Ready          : " << (DataPlaneMgrSingleton::Instance()->getSyncReady() ? "true" : "false") << endl;
    out << "DP Sync State          : " << DataPlaneMgrSingleton::Instance()->toString(DataPlaneMgrSingleton::Instance()->getDpSyncState()) << endl;
    out << "L1Agent SyncConfig Done: " << (DataPlaneMgrSingleton::Instance()->getIsAgentSyncConfigDone() ? "true" : "false") << endl;
    out << "BootUp Traffic         : " << (DataPlaneMgrSingleton::Instance()->getBootUpTraffic() ? "true" : "false") << endl;
    out << "BootUp Traffic Timer   : " << DataPlaneMgrSingleton::Instance()->getBootUpTrafficTimer() << endl;
    out << "Fault Stable           : " << (DataPlaneMgrSingleton::Instance()->getFaultStable() ? "true" : "false") << endl;
    out << "Fault Stable Timer     : " << DataPlaneMgrSingleton::Instance()->getFaultStableTimer() << endl;
}

void cmdForceSyncReady(ostream& out, bool val)
{
    out << "Set Force Sync Ready to : " << val << endl << endl;

    DataPlaneMgrSingleton::Instance()->setIsForceSyncReady(val);
}

void cmdPopInstalledConfig(ostream& out)
{
    out << "***** Populate Installed Config For All Object Mgrs *****" << endl << endl;

    DataPlaneMgrSingleton::Instance()->populateInstalledConfig();
}

void cmdIsConfigFailure(ostream& out)
{
    out << "***** Is Config Failure *****" << endl << endl;

    bool isCfgFail   = DataPlaneMgrSingleton::Instance()->isConfigFailure();
    uint32 cancelCnt = DataPlaneMgrSingleton::Instance()->getConfigCancelCount();

    out << " Config Fail? : " << (isCfgFail ? "true" : "false") <<  endl;
    out << " Cancel Count : " << cancelCnt <<  endl << endl;
}

void cmdCancelConfig(ostream& out)
{
    out << "***** Config Cancellation Triggered *****" << endl << endl;

    DataPlaneMgrSingleton::Instance()->cancelConfigProcess();
}

void cmdForceAgentSyncDone(ostream& out, bool val)
{
    out << "Set L1Agent SyncConfig Done to : " << val << endl << endl;

    DataPlaneMgrSingleton::Instance()->setIsAgentSyncConfigDone(val);
}

void cmdDumpDpMgrAll(ostream& out)
{
    auto startTime = std::chrono::steady_clock::now();

    INFN_LOG(SeverityLevel::info) << "";

    out << "===dp_mgr===" << endl << endl;

    out << "dump_poll_info" << endl;
    DataPlane::cmdDumpPollInfo(out);

    out << "dump_gige_lldp_counters" << endl;
    DataPlane::cmdDumpGigeClientLldpCounters(out);

    out << "dump_otu_state_config all" << endl;
    DataPlane::cmdDumpOtuStateConfig(out, "all");

    out << "dump_otu_sd_tracking all" << endl;
    DataPlane::cmdDumpOtuSdTracking(out, "all");

    out << "dump_otu_tim_tracking all" << endl;
    DataPlane::cmdDumpOtuTimTracking(out, "all");

    out << "dump_odu_state_config" << endl;
    DataPlane::cmdDumpOduStateConfig(out, "all");

    out << "dump_sd_tracking all" << endl;
    DataPlane::cmdDumpOduSdTracking(out, "all");

    out << "dump_card_state" << endl;
    DataPlane::cmdDumpCardState(out);

    out << "dump_fan_temps" << endl;
    DataPlane::cmdDumpFanTempFlags(out);

    out << "get_sync_state" << endl;
    DataPlane::cmdGetSyncState(out);

    out << "is_config_failure" << endl;
    DataPlane::cmdIsConfigFailure(out);

    out << "dump_config_proc  255" << endl;
    DataPlane::cmdDumpConfigProcess(out, 255);

    out << "dump_config_proc_last 255" << endl;
    DataPlane::cmdDumpConfigProcessLast(out, 255);

    out << "list_config_cache" << endl;
    DataPlane::cmdListConfigCache(out);

    out << "list_state_cache" << endl;
    DataPlane::cmdListStateCache(out);

    out << "dump_xpress_oh" << endl;
    DataPlane::cmdDumpXpressXconOhFpga(out);

    out << "===gcc_mgr===" << endl;
    DataPlane::cmdDumpGccMgrAll(out);

    auto endTime = std::chrono::steady_clock::now();

    std::chrono::seconds elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    out << "**** DpMgr Cli Collection Time Duration: " << elapsedSec.count() << " seconds" << endl;

    INFN_LOG(SeverityLevel::info) << "DpMgr Cli Collection Time Duration: " << elapsedSec.count() << " seconds" ;

}
}   // namespace DataPlane
