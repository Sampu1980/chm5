/*
 * dpMscmds.h
 *
 *  Created on: Mar 9, 2020
 */

#ifndef ADAPTER_CMDS_H
#define ADAPTER_CMDS_H

//
// DCO Adapter commands
//
extern void cmdCreateCard (ostream& out);
extern void cmdDeleteCard (ostream& out);
extern void cmdCardSetClientMode (ostream& out, uint32_t mode);
extern void cmdCardSetMaxPktLen (ostream& out, uint32_t pktLen);
extern void cmdCardEnable (ostream& out, uint32_t enable);
extern void cmdCardBoot (ostream& out, uint32_t boot);
extern void cmdCardConfigInfo (ostream& out);
extern void cmdCardStatusInfo (ostream& out);
extern void cmdCardStatusInfoForce (ostream& out);
extern void cmdCardFaultInfo (ostream& out);
extern void cmdCardFaultCapaInfo (ostream& out);
extern void cmdCardCapabilityInfo (ostream& out);
extern void cmdCardPmInfo (ostream& out);
extern void cmdCardIsSimEnable(ostream& out);
extern void cmdCardSetSimEnable(ostream& out, uint32_t enable);
extern void cmdCardNotifyState(ostream& out);
extern void cmdDumpCardAdapterAll(ostream& out);
extern void cmdGigeCreate (ostream& out, uint32_t port, uint32_t rate, uint32_t fecEnable);
extern void cmdGigeDelete (ostream& out, uint32_t port);
extern void cmdGigeSetFecEnable (ostream& out, uint32_t port, uint32_t enable);
extern void cmdGigeSetLldpDrop(ostream& out, uint32_t port, uint32_t dir, uint32_t enable);
extern void cmdGigeSetLldpSnoop(ostream& out, uint32_t port, uint32_t dir, uint32_t enable);
extern void cmdGigeSetLoopBack (ostream& out, uint32_t port, uint32_t lpbk);
extern void cmdGigeSetMtu (ostream& out, uint32_t port, uint32_t mtu);
extern void cmdGigeSetFwdTdaTrigger (ostream& out, uint32_t port, uint32_t enable);
extern void cmdGigeConfigInfo (ostream& out, uint32_t port);
extern void cmdGigeConfigInfo (ostream& out);
extern void cmdGigeStatusInfo (ostream& out, uint32_t port);
extern void cmdGigeFaultInfo (ostream& out, uint32_t port);
extern void cmdGigeIsSimEnable(ostream& out);
extern void cmdGigeSetSimEnable(ostream& out, uint32_t enable);
extern void cmdGetGigePmInfo (ostream& out, uint32_t port);
extern void cmdGetGigePmAccumInfo (ostream& out, uint32_t port);
extern void cmdClearGigePmAccum (ostream& out, uint32_t port);
extern void cmdEnableGigePmAccum (ostream& out, uint32_t port, bool enable);
extern void cmdDumpGigeAdapterAll(ostream& out);
extern void cmdDumpPmTime(ostream& out, uint32_t opt);
extern void cmdCarrierCreate (ostream& out, uint32_t carrier, uint32_t capa, double baud, string appId);
extern void cmdCarrierDelete (ostream& out, uint32_t carrier);
extern void cmdCarrierCapacity (ostream& out, uint32_t carrier, uint32_t rate);
extern void cmdCarrierBaud (ostream& out, uint32_t carrier, double rate);
extern void cmdCarrierAppId (ostream& out, uint32_t carrier, string appCode);
extern void cmdCarrierEnable (ostream& out, uint32_t carrier, uint32_t enable);
extern void cmdCarrierFrequency (ostream& out, uint32_t carrier, double frequency);
extern void cmdCarrierOffSet (ostream& out, uint32_t carrier, double offset);
extern void cmdCarrierPower (ostream& out, uint32_t carrier, double power);
extern void cmdCarrierTxCd (ostream& out, uint32_t carrier, double txCd);
extern void cmdCarrierAdvParm (ostream& out, uint32_t carrier, string name, string value);
extern void cmdDelCarrierAdvParm(ostream& out, uint32_t carrier, string key);
extern void cmdDelCarrierAdvParmAll(ostream& out, uint32_t carrier);
extern void cmdCarrierDgdOorhTh (ostream& out, uint32_t carrier, double thresh);
extern void cmdCarrierPostFecSdTh (ostream& out, uint32_t carrier, double thresh);
extern void cmdCarrierPostFecSdHyst (ostream& out, uint32_t carrier, double hysteresis);
extern void cmdCarrierPreFecSdTh (ostream& out, uint32_t carrier, double thresh);
extern void cmdCarrierPreFecSdHyst (ostream& out, uint32_t carrier, double hysteresis);
extern void cmdGetCarrierConfigInfo (ostream& out, uint32_t carrier);
extern void cmdGetCarrierConfigInfo (ostream& out);
extern void cmdGetCarrierStatusInfo (ostream& out, uint32_t carrier);
extern void cmdGetCarrierFaultInfo (ostream& out, uint32_t carrier);
extern void cmdGetCarrierPmInfo (ostream& out, uint32_t carrier);
extern void cmdCarrierNotifyState(ostream& out, uint32_t carrier);
extern void cmdGetCarrierPmAccumInfo (ostream& out, uint32_t carrier);
extern void cmdClearCarrierPmAccum (ostream& out, uint32_t carrier);
extern void cmdEnableCarrierPmAccum (ostream& out, uint32_t carrierId, bool enable);
extern void cmdCarrierIsSimEnable(ostream& out);
extern void cmdCarrierSetSimEnable(ostream& out, uint32_t enable);
extern void cmdDeleteCarrierBaudRate(ostream& out, uint32_t carrier);
extern void cmdDumpCarrierAdapterAll(ostream& out);
extern void cmdOtuCreate (ostream& out, uint32_t otuId, uint32_t payLoad, uint32_t channel);
extern void cmdOtuDelete (ostream& out, uint32_t otuId);
extern void cmdOtuSetLoopBack (ostream& out, uint32_t otuId, uint32_t lpbk);
extern void cmdOtuSetPrbsMon(ostream& out, uint32_t otuId, uint32_t dir, uint32_t prbs);
extern void cmdOtuSetPrbsGen(ostream& out, uint32_t otuId, uint32_t dir, uint32_t prbs);
extern void cmdOtuSetTxTti (ostream& out, uint32_t otuId, string tti);
extern void cmdOtuSetForceMs(ostream& out, uint32_t otuId, uint32_t dir, uint32_t ms);
extern void cmdOtuSetFaultStatus (ostream& out, uint32_t otuId, uint32_t dir, string injectFaultBitmap);
extern void cmdGetOtuConfigInfo (ostream& out, uint32_t otuId);
extern void cmdGetOtuConfigInfo (ostream& out);
extern void cmdGetOtuStatusInfo (ostream& out, uint32_t otuId);
extern void cmdOtuIsSimEnable(ostream& out);
extern void cmdOtuSetSimEnable(ostream& out, uint32_t enable);
extern void cmdGetOtuPmInfo (ostream& out, uint32_t otuId);
extern void cmdGetOtuPmAccumInfo (ostream& out, uint32_t otuId);
extern void cmdClearOtuPmAccum (ostream& out, uint32_t otuId);
extern void cmdEnableOtuPmAccum (ostream& out, uint32_t otuId, bool enable);
extern void cmdGetOtuFaultInfo (ostream& out, uint32_t otuId);
extern void cmdOtuNotifyState (ostream& out, uint32_t otuId);
extern void cmdDumpOtuAdapterAll(ostream& out);
extern void cmdOduCreate (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t payLoad, string ts);
extern void cmdOduDelete (ostream& out, uint32_t oduId);
extern void cmdOduSetLoopBack (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t lpbk);
extern void cmdOduSetPrbsMon(ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t prbs);
extern void cmdOduSetPrbsGen(ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t prbs);
extern void cmdOduSetTS (ostream& out, uint32_t oduId, uint32_t otuId, string ts);
extern void cmdOduSetForceMs (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t ms);
extern void cmdOduSetAutoMs (ostream& out, uint32_t oduId, uint32_t otuId, uint32_t dir, uint32_t ms);
extern void cmdOduSetFaultStatus (ostream& out, uint32_t oduId, uint32_t otuId, string injectFaultBitmap);
extern void cmdGetOduConfigInfo (ostream& out, uint32_t oduId);
extern void cmdGetOduConfigInfo (ostream& out);
extern void cmdGetOduStatusInfo (ostream& out, uint32_t oduId);
extern void cmdGetOduStatusInfoAll (ostream& out);
extern void cmdGetOduFaultInfo (ostream& out, uint32_t oduId);
extern void cmdGetOduFaultInfoAll (ostream& out);
extern void cmdGetOduCdiInfo (ostream& out, uint32_t oduId);
extern void cmdOduIsSimEnable(ostream& out);
extern void cmdOduSetSimEnable(ostream& out, uint32_t enable);
extern void cmdGetOduPmInfo (ostream& out, uint32_t oduId);
extern void cmdGetOduPmInfoAll (ostream& out);
extern void cmdOduNotifyState (ostream& out, uint32_t otuId);
extern void cmdOduNotifyStateAll (ostream& out);
extern void cmdDumpOduAdapterAll(ostream& out);
extern void cmdGetOduPmAccumInfo (ostream& out, uint32_t oduId);
extern void cmdClearOduPmAccum (ostream& out, uint32_t oduId);
extern void cmdEnableOduPmAccum (ostream& out, uint32_t oduId, bool enable);
extern void cmdXconCreate (ostream& out, uint32_t xconId, uint32_t clientId , uint32_t loOduId, uint32_t hoOtuId, uint32_t direction);
extern void cmdXconDelete (ostream& out, uint32_t xconId);
extern void cmdGetXconConfigInfo (ostream& out, uint32_t xconId);
extern void cmdGetXconConfigInfo (ostream& out);
extern void cmdGetXconStatusInfo (ostream& out, uint32_t xconId);
extern void cmdXconIsSimEnable(ostream& out);
extern void cmdXconSetSimEnable(ostream& out, uint32_t enable);
extern void cmdDumpXconAdapterAll(ostream& out);
extern void cmdDcoAdapterSimEnable(ostream& out, uint32_t enable);
extern void cmdDcoAdapterIsSimEnable(ostream& out);
extern void cmdDcoSetGccVlan(ostream& out);
extern void cmdDcoGetGccVlanCfg(ostream& out);
extern void cmdDcoGetGccVlanState(ostream& out);
extern void cmdDcoCreateCcVlan(ostream& out, uint32_t vIdx, uint32_t carrier, uint32_t txVtag, uint32_t txVretag, uint32_t rxVtag, uint32_t rxVretag, uint32_t gcc_type);
extern void cmdDcoEnableCcVlan(ostream& out, uint32_t vIdx, uint32_t enable);
extern void cmdDcoDeleteCcVlan(ostream& out, uint32_t vIdx);
extern void cmdGetDcoCcVlanConfigInfo(ostream& out, uint32_t vIdx);
extern void cmdGetDcoCcVlanStatusInfo(ostream& out, uint32_t vIdx);
extern void cmdGetGccPmInfo (ostream& out, uint32_t vIdx);
extern void cmdGetGccPmAccumInfo (ostream& out, uint32_t vIdx);
extern void cmdClearGccPmAccum (ostream& out, uint32_t vIdx);
extern void cmdDumpDcoAdapterAll(ostream& out);
extern void cmdDumpGccAdapterAll(ostream& out);

// Dco Connection Handler Cmds
extern void cmdListDcoConnStates(ostream& out);
extern void cmdListDcoConnErrors(ostream& out);
extern void cmdListDcoConnIds(ostream& out);
extern void cmdDumpDcoConnStatus(ostream& out);
extern void cmdDumpDcoSimConnStatus(ostream& out);
extern void cmdSetDcoSimConnStatus(ostream& out, uint32 connId, bool isEn, uint32 connStat, uint32 connErr);
extern void cmdDumpDcoSocketAddr(ostream& out, uint32 connId);
extern void cmdSetDcoSocketAddr(ostream& out, uint32 connId, std::string sockAddr);
extern void cmdDumpGrpcConnectStatus(ostream& out);
extern void cmdGetDcoConnectPollEnable(ostream& out, uint32 connId);
extern void cmdSetDcoConnectPollEnable(ostream& out, uint32 connId, bool isEnable);

//DCO secproc adapter commands
extern void cmdDumpActiveCardSubscriptions(ostream& out);
extern void cmdDumpActiveVlanSubscriptions(ostream& out);
extern void cmdDumpActiveIcdpSubscriptions(ostream& out);

#endif /*ADAPTER_CMDS_H*/
