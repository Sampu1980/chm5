/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef _DATAPLANEMGRCMDS_H_
#define _DATAPLANEMGRCMDS_H_

#include <cli/cli.h>

namespace DataPlane {

extern void cmdCompleteInit  (ostream& osOut);
extern void cmdForceDcoState(ostream& osOut, uint32_t state);
extern void cmdDumpDcoState(ostream& osOut);

extern void cmdDumpPollInfo(ostream& osOut);
extern void cmdSetStatePollMode(ostream& osOut, uint32_t mode);
extern void cmdSetStatePollInterval(ostream& osOut, uint32_t intvl);
extern void cmdSetFaultPollMode(ostream& osOut, uint32_t mode);
extern void cmdSetFaultPollInterval(ostream& osOut, uint32_t intvl);

extern void cmdDpMgrSubMenu          (unique_ptr< cli::Menu > & subMenu);
extern void cmdDpMgrCardMgrSubMenu   (unique_ptr< cli::Menu > & subMenu);
extern void cmdDpMgrCarrierMgrSubMenu(unique_ptr< cli::Menu > & subMenu);
extern void cmdDpMgrGigeMgrSubMenu   (unique_ptr< cli::Menu > & subMenu);
extern void cmdDpMgrOtuMgrSubMenu    (unique_ptr< cli::Menu > & subMenu);
extern void cmdDpMgrOduMgrSubMenu    (unique_ptr< cli::Menu > & subMenu);
extern void cmdDpMgrXconMgrSubMenu   (unique_ptr< cli::Menu > & subMenu);
extern void cmdDpMgrGccMgrSubMenu   (unique_ptr< cli::Menu > & subMenu);

extern void cmdDumpCardState           (ostream& osOut);
extern void cmdDumpDcoCapabilities     (ostream& osOut);
extern void cmdSetCardStatePollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetCardStatePollInterval(ostream& osOut, uint32_t intvl);
extern void cmdSetCardFaultPollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetCardFaultPollInterval(ostream& osOut, uint32_t intvl);
extern void cmdDumpCardPollInfo        (ostream& osOut);
extern void cmdSetDcoFaultReportMode   (ostream& osOut, uint32_t mode);
extern void cmdForceFanTempFlags(ostream& osOut, uint32_t tempHi, uint32_t tempStable);
extern void cmdDumpFanTempFlags(std::ostream& osOut);
extern void cmdDumpCardFault           (ostream& osOut);
extern void cmdCardBootDco(ostream& osOut, uint32 bootAction);

extern void cmdCreateCarrier   (ostream& osOut, uint carNum, uint clientMode);
extern void cmdModifyCarrier   (ostream& osOut, uint carNum);
extern void cmdModifyCarrierState(ostream& osOut, uint carNum, bool isRxAcq);
extern void cmdModifyCarrierFault(ostream& osOut, uint carNum, bool isOfecUp);
extern void cmdDeleteCarrier   (ostream& osOut, uint carNum);
extern void cmdCreateScg       (ostream& osOut, uint scgNum);
extern void cmdTriggerCarrierPm(ostream& osOut, uint carNum);
extern void cmdDumpCarrierFault(ostream& osOut, std::string carAid);
extern void cmdSetCarrierStatePollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetCarrierStatePollInterval(ostream& osOut, uint32_t intvl);
extern void cmdSetCarrierFaultPollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetCarrierFaultPollInterval(ostream& osOut, uint32_t intvl);
extern void cmdDumpCarrierPollInfo        (ostream& osOut);

extern void cmdCreateGigeClient         (ostream& osOut, uint gigeNum);
extern void cmdModifyGigeClientLoopback (ostream& osOut, uint gigeNum, uint value);
extern void cmdModifyGigeClientFecEnable(ostream& osOut, uint gigeNum, uint value);
extern void cmdDeleteGigeClient         (ostream& osOut, uint gigeNum);
extern void cmdDumpGigeClientFault(ostream& osOut, std::string gigeAid);
extern void cmdDumpGigeClientPm   (ostream& osOut, std::string gigeAid);
extern void cmdSetGigeClientStatePollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetGigeClientStatePollInterval(ostream& osOut, uint32_t intvl);
extern void cmdSetGigeClientFaultPollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetGigeClientFaultPollInterval(ostream& osOut, uint32_t intvl);
extern void cmdSetGigeClientLldpPollMode     (ostream& osOut, uint32_t mode);
extern void cmdSetGigeClientLldpPollInterval (ostream& osOut, uint32_t intvl);
extern void cmdDumpGigeClientPollInfo        (ostream& osOut);
extern void cmdDumpGigeClientLldpCounters    (ostream& osOut);

extern void cmdCreateOtu              (ostream& osOut, uint otuNum, uint subType);
extern void cmdDeleteOtu              (ostream& osOut, uint otuNum, uint subType);
extern void cmdDumpOtuState           (ostream& osOut, std::string otuAid);
extern void cmdDumpOtuFault           (ostream& osOut, std::string otuAid);
extern void cmdDumpOtuPm              (ostream& osOut, std::string otuAid);
extern void cmdDumpOtuStateConfig     (ostream& osOut, std::string otuAid);
extern void cmdDumpOtuSdTracking      (ostream& osOut, std::string otuAid);
extern void cmdDumpOtuTimTracking     (ostream& osOut, std::string otuAid);
extern void cmdSetOtuTimAlarmMode     (ostream& osOut, uint32_t mode);
extern void cmdSetOtuSdAlarmMode      (ostream& osOut, uint32_t mode);
extern void cmdSetOtuStatePollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetOtuStatePollInterval(ostream& osOut, uint32_t intvl);
extern void cmdSetOtuFaultPollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetOtuFaultPollInterval(ostream& osOut, uint32_t intvl);
extern void cmdDumpOtuPollInfo        (ostream& osOut);
extern void cmdCreateOdu              (ostream& osOut, uint oduNum, uint subType);
extern void cmdDeleteOdu              (ostream& osOut, uint oduNum, uint subType);
extern void cmdCreateOduAid           (ostream& osOut, std::string oduAid, uint32 oduIdx);
extern void cmdCreateOduNewAid        (ostream& osOut, uint oduNum, uint otuNum, bool isOdu4i);
extern void cmdDeleteOduStr           (ostream& osOut, std::string oduAid);
extern void cmdDumpOduState           (ostream& osOut, std::string oduAid);
extern void cmdDumpOduFault           (ostream& osOut, std::string oduAid);
extern void cmdDumpOduPm              (ostream& osOut, std::string oduAid);
extern void cmdDumpOduStateConfig     (ostream& osOut, std::string oduAid);
extern void cmdDumpOduSdTracking      (ostream& osOut, std::string oduAid);
extern void cmdSetOduTimAlarmMode     (ostream& osOut, uint32_t mode);
extern void cmdSetOduSdAlarmMode      (ostream& osOut, uint32_t mode);
extern void cmdSetOduStatePollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetOduStatePollInterval(ostream& osOut, uint32_t intvl);
extern void cmdSetOduFaultPollMode    (ostream& osOut, uint32_t mode);
extern void cmdSetOduFaultPollInterval(ostream& osOut, uint32_t intvl);
extern void cmdDumpOduPollInfo        (ostream& osOut);
extern void cmdCreateXcon             (ostream& osOut, std::string xconAid);
extern void cmdCreateXpressXcon       (ostream& osOut, std::string xconId, std::string srcClientAid, std::string dstClientAid, uint32_t payload);
extern void cmdDeleteXcon             (ostream& osOut, std::string xconId);
extern void cmdDumpXpressXconOhFpga   (ostream& osOut);

extern void cmdDumpLldpFrame (ostream& osOut, uint portId);
extern void cmdDumpLldp (ostream& osOut);
extern void cmdDumpGccMgrAll(ostream& osOut);

extern void cmdListObjMgrs(ostream& out);
extern void cmdListConfigCache(ostream& out);
extern void cmdClearConfigCache(ostream& out, uint32_t mgrIdx);
extern void cmdClearConfigCache(ostream& out, uint32_t mgrIdx, string configId);
extern void cmdDumpConfigCache(ostream& out, uint32_t mgrIdx);
extern void cmdDumpConfigCache(ostream& out, uint32_t mgrIdx, string configId);
extern void cmdSyncConfigCache(ostream& out, uint32_t mgrIdx);
extern void cmdListStateCache(ostream& out);
extern void cmdDumpStateCache(ostream& out, uint32_t mgrIdx, string configId);
extern void cmdDumpConfigProcess(ostream& out, uint32_t mgrIdx);
extern void cmdDumpConfigProcessLast(ostream& out, uint32_t mgrIdx);
extern void cmdGetSyncState(ostream& out);
extern void cmdForceSyncReady(ostream& out, bool val);
extern void cmdForceAgentSyncDone(ostream& out, bool val);
extern void cmdPopInstalledConfig(ostream& out);
extern void cmdIsConfigFailure(ostream& out);
extern void cmdCancelConfig(ostream& out);
extern void cmdDumpDpMgrAll(ostream& out);


}   // namespace DataPlane

#endif /* _DATAPLANEMGRCMDS_H_ */
