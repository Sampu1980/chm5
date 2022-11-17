/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef _SIM_CMDS_H
#define _SIM_CMDS_H

#include <cli/cli.h>

namespace DpSim {

extern void cmdDpMgrSubMenu(std::unique_ptr< cli::Menu > & subMenu);
extern void cmdDumpMgrCfgCache(std::ostream& osOut, uint mgrIdx, uint objId);
extern void cmdClearMgrCfgCache(std::ostream& osOut, uint mgrIdx, uint objId);
extern void cmdDumpMgrNames(std::ostream& osOut);
extern void cmdDumpStateParamNames(std::ostream& osOut, std::string mgrName);
extern void cmdSetStateParam(std::ostream& osOut, std::string mgrName, uint objId, std::string paramName, std::string valueStr);
extern void cmdUpdateCardEqptFault(std::ostream& osOut, std::string faultName, int set);
extern void cmdUpdateCardPostFault(std::ostream& osOut, std::string faultName, int set);
extern void cmdClearAllCardEqptFault(std::ostream& osOut);
extern void cmdClearAllCardPostFault(std::ostream& osOut);

}   //  namespace DpSim

#endif /* _SIM_CMDS_H */
