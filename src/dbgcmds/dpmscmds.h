/*
 * dpMscmds.h
 *
 *  Created on: Mar 9, 2020
 */

#ifndef DP_MS_CMDS_H
#define DP_MS_CMDS_H

class DpMsCmds
{
public:
    DpMsCmds();
    ~DpMsCmds() {}
    void DumpLog(std::ostream& out);
    void DumpStatus(std::ostream& out, std::string instr);
    void ResetLog(std::ostream& out);

private:
};

boost::function< void (DpMsCmds*, std::ostream&) > dpMsLog = &DpMsCmds::DumpLog;
boost::function< void (DpMsCmds*, std::ostream&, std::string) > dpMsStatus = &DpMsCmds::DumpStatus;
boost::function< void (DpMsCmds*, std::ostream&) > dpMsResetLog = &DpMsCmds::ResetLog;

#endif /*DP_MS_CMDS_H*/
