/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#include <cli/cli.h>

#include "SimCmds.h"
#include "DpCarrierMgr.h"
#include "SimCrudService.h"

using namespace DataPlane;

namespace DpSim {

void cmdDpMgrSubMenu(std::unique_ptr< cli::Menu > & subMenu)
{
    subMenu->Insert(
            "dumpMgrConfig",
            [](std::ostream& out, uint mgrIdx, uint objId)
            {
                cmdDumpMgrCfgCache(out, mgrIdx, objId);
            },
            "dumpMgrConfig - dumps config cache\n"
            "                   mgrIdx - set beyond range (such as 0xFFFF) to dump all \n"
            "                   objId  - set to 0 to dump all"  );

    subMenu->Insert(
            "clearMgrData",
            [](std::ostream& out, uint mgrIdx, uint objId)
            {
                cmdClearMgrCfgCache(out, mgrIdx, objId);
            },
            "clearMgrData - clears config cache\n"
            "                   mgrIdx - set beyond range (such as 0xFFFF) to dump all \n"
            "                   objId  - set to 0 to dump all"  );

    subMenu->Insert(
            "dumpMgrNames",
            [](std::ostream& out)
            {
                cmdDumpMgrNames(out);
            },
            "dumpMgrNames - dumps manager names\n" );

    subMenu->Insert(
            "dumpStateParamNames",
            [](std::ostream& out, string mgrName)
            {
                cmdDumpStateParamNames(out, mgrName);
            },
            "dumpStateParamNames - dumps manager state parameter names \n" 
            "                   mgrName - manager name of state parameters" );

    subMenu->Insert(
            "setStateParam",
            [](std::ostream& out, string mgrName, uint objId, string paramName, string valueStr)
            {
                cmdSetStateParam(out, mgrName, objId, paramName, valueStr);
            },
            "setStateParam - sets the state parameters\n"
            "                   mgrName   - mananger name \n"
            "                   objId     - unique object identifier \n"
            "                   paramName - state parameter name \n"
            "                   value     - value to set" );

    subMenu->Insert(
            "updateCardEqptFault",
            [](std::ostream& out, string faultName, int set)
            {
                cmdUpdateCardEqptFault(out, faultName, set);
            },
            "updateCardEqptFault - set or clear DCO card equipment fault parameters\n"
            "                   faultName - equipment fault name shown in DcoFaultCapabilities\n"
            "                   set       - \"0\" to clear; \"1\" to set" );

    subMenu->Insert(
            "updateCardPostFault",
            [](std::ostream& out, string faultName, int set)
            {
                cmdUpdateCardPostFault(out, faultName, set);
            },
            "updateCardPostFault - set or clear DCO card post fault parameters\n"
            "                   faultName - post fault name shown in DcoFaultCapabilities\n"
            "                   set       - \"0\" to clear; \"1\" to set" );

    subMenu->Insert(
            "clearAllCardEqptFault",
            [](std::ostream& out)
            {
                cmdClearAllCardEqptFault(out);
            },
            "clearAllCardEqptFault - clear all DCO card equipment faults\n" );

    subMenu->Insert(
            "clearAllCardPostFault",
            [](std::ostream& out)
            {
                cmdClearAllCardPostFault(out);
            },
            "clearAllCardPostFault - clear all DCO card post faults\n" );
}


void cmdDumpMgrCfgCache(std::ostream& osOut, uint mgrIdx, uint objId)
{
    DpSim::SimSbGnmiClient::getInstance()->dumpObjMgrConfigCache(osOut, mgrIdx, objId);
}

void cmdClearMgrCfgCache(std::ostream& osOut, uint mgrIdx, uint objId)
{
    DpSim::SimSbGnmiClient::getInstance()->clearObjMgrCache(osOut, mgrIdx, objId);
}

void cmdDumpMgrNames(std::ostream& osOut)
{
    DpSim::SimSbGnmiClient::getInstance()->dumpMgrNames(osOut);
}

void cmdDumpStateParamNames(std::ostream& osOut, std::string mgrName)
{
    DpSim::SimSbGnmiClient::getInstance()->dumpStateParamNames(osOut, mgrName);
}

void cmdSetStateParam(std::ostream& osOut, std::string mgrName, uint objId, std::string paramName, std::string valueStr)
{
    DpSim::SimSbGnmiClient::getInstance()->setStateParam(osOut, mgrName, objId, paramName, valueStr);
}

void cmdUpdateCardEqptFault(std::ostream& osOut, std::string faultName, int set)
{
    DpSim::SimSbGnmiClient::getInstance()->updateCardEqptFault(osOut, faultName, (bool)set);
}

void cmdUpdateCardPostFault(std::ostream& osOut, std::string faultName, int set)
{
    DpSim::SimSbGnmiClient::getInstance()->updateCardPostFault(osOut, faultName, (bool)set);
}

void cmdClearAllCardEqptFault(std::ostream& osOut)
{
    DpSim::SimSbGnmiClient::getInstance()->clearAllCardEqptFault(osOut);
}

void cmdClearAllCardPostFault(std::ostream& osOut)
{
    DpSim::SimSbGnmiClient::getInstance()->clearAllCardPostFault(osOut);
}

}   //  namespace DpSim

