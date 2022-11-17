/*
 * serdes_tuner_cmds.h
 *
 *  Created on: Sep 9, 2020
 *      Author: aforward
 */

#ifndef SRC_DBGCMDS_SERDES_TUNER_CMDS_H_
#define SRC_DBGCMDS_SERDES_TUNER_CMDS_H_
#include <map>
#include <vector>
#include "SerdesTuner.h"
#include "SerdesConnMap.h"


extern void cmdSetBcmTomParams(ostream& out, unsigned int slot, unsigned int port, unsigned int rate);
extern void cmdSetBcmBcmParams(ostream& out, unsigned int port, unsigned int rate);
extern void cmdSetBcmAtlParams(ostream& out, unsigned int port, unsigned int rate);
extern void cmdDumpSerdesPortValues(ostream& out, unsigned int port);
extern void cmdDumpSrdsTnrMgrAll(ostream& out);

bool GetBcmTxDump(std::map<unsigned int, std::vector<tuning::SerdesDumpData>> dump, unsigned int port, unsigned int& srcDevNum,
                             unsigned int lane, int& pre2, int& pre, int& main, int & post, int& post2, int & post3,
                             unsigned int& destDeviceNum, unsigned int& destLaneNum, dp::DeviceInfo& src, dp::DeviceInfo& dest, unsigned int& linkId, unsigned int& linkLoss);
std::string GetFoundTitle(bool found);
bool GetBcmRxDump(std::map<unsigned int, std::vector<tuning::SerdesDumpData>> dump, unsigned int port, unsigned int lane,
        std::map<unsigned int, int64_t>& rxValue, unsigned int& srcDevNum, unsigned int& srcLaneNum,
        dp::DeviceInfo& src, dp::DeviceInfo& dest, unsigned int& linkId, unsigned int& linkLoss);
bool GetTomRxDump(std::map<unsigned int, std::vector<tuning::SerdesDumpData>> dump, unsigned int port, unsigned int lane, tuning::SerdesDumpData & laneDump);
bool GetTomTxDump(std::map<unsigned int, std::vector<tuning::SerdesDumpData>> dump, unsigned int port, unsigned int lane, tuning::SerdesDumpData & laneDump);
bool IsBcmLineValid(unsigned int port, unsigned int bcmNum);

unsigned int DumpTomRx(ostream& out, unsigned int port, unsigned int rate);
unsigned int DumpTomTx(ostream& out, unsigned int port, unsigned int rate);
unsigned int DumpBcmLineToRxHost(ostream& out, unsigned int port, unsigned int rate);
unsigned int DumpBcmTxHostToBcmLine(ostream& out, unsigned int port, unsigned int rate);
unsigned int DumpBcmTxLineToTom(ostream& out, unsigned int port, unsigned int rate);
unsigned int DumpBcmTxToAtl(ostream& out, unsigned int port, unsigned int rate);
unsigned int DumpAtlToBcmRx(ostream& out, unsigned int port, unsigned int rate);
void ShowResults(unsigned int status, ostream& out);

#endif /* SRC_DBGCMDS_SERDES_TUNER_CMDS_H_ */
