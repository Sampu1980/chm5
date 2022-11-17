/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef CARRIER_PB_TRANSLATOR_H_
#define CARRIER_PB_TRANSLATOR_H_

#include <string>

#include <DcoCarrierAdapter.h>
#include <DcoCardAdapter.h>
#include <DpPm.h>

#include "DpProtoDefs.h"
#include "DpCommonPbTranslator.h"


namespace DataPlane {

static const double CARRIER_CD_THRESHOLD = 0.01;

class CarrierPbTranslator : public DpCommonPbTranslator
{
public:
    enum CARRIER_FAULT_PARAMS
    {
        CAR_FAULT_BDI                   = 0,
        CAR_FAULT_LOF                      ,
        CAR_FAULT_OLOS                     ,
        CAR_FAULT_OPT_OORL                 ,
        CAR_FAULT_OPT_OORH                 ,
        CAR_FAULT_DGD_OORH                 ,
        CAR_FAULT_POST_FEC_Q_SIGNAL_FAILURE,
        CAR_FAULT_POST_FEQ_Q_SIGNAL_DEGRADE,
        CAR_FAULT_PRE_FEQ_Q_SIGNAL_DEGRADE ,
        CAR_FAULT_BSD                      ,
        NUM_CARRIER_FAULT_PARAMS
    };
    
    CarrierPbTranslator() {}

    ~CarrierPbTranslator() {}

    std::string numToAidCar(uint carNum) const;
    std::string aidScgToCar(std::string aidStr) const;
    std::string aidCarToScg(std::string aidStr) const;

    void configPbToAd(const chm6_dataplane::Chm6CarrierConfig& pbCfg, DpAdapter::CarrierCommon& adCfg) const;
    void configAdToPb(const DpAdapter::CarrierCommon& adCfg, DpAdapter::CardConfig& cardCfg, chm6_dataplane::Chm6CarrierState& pbStat) const;
    void statusAdToPb(const DpAdapter::CarrierStatus& adStat, chm6_dataplane::Chm6CarrierState& pbStat) const;
    void faultAdToPb(std::string aid, const uint64_t& faultBmp, const DpAdapter::CarrierFault& adFault, chm6_dataplane::Chm6CarrierFault& pbFault, bool force) const;
    void faultAdToPb(std::string aid, const uint64_t& faultBmp, const DpAdapter::CarrierFault& adFault, chm6_dataplane::Chm6ScgFault& pbFault, bool force) const;
    void faultAdToPbSim(const uint64_t& fault, chm6_dataplane::Chm6CarrierFault& pbFault) const;
    void faultAdToPbSim(const uint64_t& fault, chm6_dataplane::Chm6ScgFault& pbFault) const;
    void pmAdToPb(const DataPlane::DpMsCarrierPMContainer& adPm, bool validity, chm6_dataplane::Chm6CarrierPm& pbPm) const;
    void pmAdToPb(const DataPlane::DpMsCarrierPMContainer& adPm, bool validity, chm6_dataplane::Chm6ScgPm& pbPm) const;
    void pmAdDefault(DataPlane::DpMsCarrierPMContainer& adPm);
    bool isPbChanged(const chm6_dataplane::Chm6CarrierState& pbStatPrev, const chm6_dataplane::Chm6CarrierState& pbStatCur) const;
    bool isAdChanged(const DpAdapter::CarrierFault& adFaultPrev, const DpAdapter::CarrierFault& adFaultCur);
    void configPbToAd(const chm6_dataplane::Chm6CarrierConfig& pbCfg, DpAdapter::CardConfig& adCfg) const;
    void statusAdToPb(const DpAdapter::CardStatus& adStat, chm6_dataplane::Chm6CarrierState& pbStat) const;

    bool isCarrierSdFault(const DpAdapter::CarrierFault& adFault) const;
    bool isCarrierSfFault(const DpAdapter::CarrierFault& adFault) const;
    bool isCarrierPreFecSdFault(const DpAdapter::CarrierFault& adFault) const;
    bool isCarrierBsdFault(const DpAdapter::CarrierFault& adFault) const;
    bool isCarrierOlosFault(const DpAdapter::CarrierFault& adFault) const;
    bool isOfecCCFault(const DpAdapter::CarrierFault& adFault) const;

    bool isCarrierTxStateChanged(std::string aid, const chm6_dataplane::Chm6CarrierState& pbStat, hal_dataplane::CarrierOpticalState& pbCarrTxState) const;
    void addPbScgCarrierLockFault(std::string scgAid, const hal_dataplane::CarrierOpticalState& pbCarrTxState, chm6_dataplane::Chm6ScgFault& pbFault) const;

    void mergeObj(chm6_dataplane::Chm6CarrierConfig& toCfg, const chm6_dataplane::Chm6CarrierConfig& fromCfg);

private:


    const boost::unordered_map<CARRIER_FAULT_PARAMS,const char*> carrierFaultParamsToString = boost::assign::map_list_of
        (CAR_FAULT_BDI                      , "BDI"                      )
        (CAR_FAULT_LOF                      , "LOF"                      )
        (CAR_FAULT_OLOS                     , "OLOS"                     )
        (CAR_FAULT_OPT_OORL                 , "OPT-OORL"                 )
        (CAR_FAULT_OPT_OORH                 , "OPT-OORH"                 )
        (CAR_FAULT_DGD_OORH                 , "DGD-OORH"                 )
        (CAR_FAULT_POST_FEC_Q_SIGNAL_FAILURE, "POST-FEC-Q-SIGNAL-FAILURE")
        (CAR_FAULT_POST_FEQ_Q_SIGNAL_DEGRADE, "POST-FEC-Q-SIGNAL-DEGRADE")
        (CAR_FAULT_PRE_FEQ_Q_SIGNAL_DEGRADE , "PRE-FEC-Q-SIGNAL-DEGRADE" )
        (CAR_FAULT_BSD                      , "BSD" );

    void addCarrierFaultPair(std::string faultName, DataPlane::FaultDirection faultDirection, DataPlane::FaultLocation faultLocation, bool value, chm6_dataplane::Chm6CarrierFault& pbFault) const;

    enum SCG_FAULT_PARAMS
    {
        SCG_FAULT_OLOS = 0,
        SCG_FAULT_LOCK,
        NUM_SCG_FAULT_PARAMS
    };

    const boost::unordered_map<SCG_FAULT_PARAMS,const char*> scgFaultParamsToString = boost::assign::map_list_of
         (SCG_FAULT_OLOS, "OLOS")
         (SCG_FAULT_LOCK, "LS-ACTIVE");

    void addScgFaultPair(std::string faultName, DataPlane::FaultDirection faultDirection, DataPlane::FaultLocation faultLocation, bool value, chm6_dataplane::Chm6ScgFault& pbFault) const;

    enum CARRIER_PM_PARAMS
    {
        CAR_PM_LBC_EGRESS         = 0,
        CAR_PM_LBC_INGRESS           ,
        CAR_PM_CD                    ,
        CAR_PM_DGD                   ,
        CAR_PM_SO_DGD                ,
        CAR_PM_SNR_AVG_X             ,
        CAR_PM_SNR_AVG_Y             ,
        CAR_PM_SNR_INNER             ,
        CAR_PM_SNR_OUTER             ,
        CAR_PM_SNR_DIFF_LEFT         ,
        CAR_PM_SNR_DIFF_RIGHT        ,
        CAR_PM_SUBCARRIER0_SNR       ,
        CAR_PM_SUBCARRIER1_SNR       ,
        CAR_PM_SUBCARRIER2_SNR       ,
        CAR_PM_SUBCARRIER3_SNR       ,
        CAR_PM_SUBCARRIER4_SNR       ,
        CAR_PM_SUBCARRIER5_SNR       ,
        CAR_PM_SUBCARRIER6_SNR       ,
        CAR_PM_SUBCARRIER7_SNR       ,
        CAR_PM_SNR_TOTAL             ,
        CAR_PM_PRE_FEC_Q             ,
        CAR_PM_POST_FEC_Q            ,
        CAR_PM_PRE_FEC_BER           ,
        CAR_PM_PASSING_FEC_FRAMES    ,
        CAR_PM_FAILED_FEC_FRAMES     ,
        CAR_PM_TOTAL_FEC_FRAMES      ,
        CAR_PM_CORRECTED_BITS        ,
        CAR_PM_PHASE_CORRECTION      ,
        CAR_PM_OSNR                  , 
        CAR_PM_TX_CD                 , 
        CAR_PM_TX_FREQUENCY          , 
        CAR_PM_RX_FREQUENCY          , 
        NUM_CARRIER_PM_PARAMS
    };

    const boost::unordered_map<CARRIER_PM_PARAMS,const char*> carrierPmParamsToString = boost::assign::map_list_of
        (CAR_PM_LBC_EGRESS            , "lbc"                   )
        (CAR_PM_LBC_INGRESS           , "lbc"                   )
        (CAR_PM_CD                    , "cd"                    )
        (CAR_PM_DGD                   , "dgd"                   )
        (CAR_PM_SO_DGD                , "so-dgd"                )
        (CAR_PM_SNR_AVG_X             , "snr-avg-x"             )
        (CAR_PM_SNR_AVG_Y             , "snr-avg-y"             )
        (CAR_PM_SNR_INNER             , "snr-inner"             )
        (CAR_PM_SNR_OUTER             , "snr-outer"             )
        (CAR_PM_SNR_DIFF_LEFT         , "snr-diff-left"         )
        (CAR_PM_SNR_DIFF_RIGHT        , "snr-diff-right"        )
        (CAR_PM_SUBCARRIER0_SNR       , "subcarrier0-snr"       )
        (CAR_PM_SUBCARRIER1_SNR       , "subcarrier1-snr"       )
        (CAR_PM_SUBCARRIER2_SNR       , "subcarrier2-snr"       )
        (CAR_PM_SUBCARRIER3_SNR       , "subcarrier3-snr"       )
        (CAR_PM_SUBCARRIER4_SNR       , "subcarrier4-snr"       )
        (CAR_PM_SUBCARRIER5_SNR       , "subcarrier5-snr"       )
        (CAR_PM_SUBCARRIER6_SNR       , "subcarrier6-snr"       )
        (CAR_PM_SUBCARRIER7_SNR       , "subcarrier7-snr"       )
        (CAR_PM_SNR_TOTAL             , "snr-total"             )
        (CAR_PM_PRE_FEC_Q             , "pre-fec-q"             )
        (CAR_PM_POST_FEC_Q            , "post-fec-q"            )
        (CAR_PM_PRE_FEC_BER           , "pre-fec-ber"           )
        (CAR_PM_PASSING_FEC_FRAMES    , "passing-fec-frames"    )
        (CAR_PM_FAILED_FEC_FRAMES     , "failed-fec-frames"     )
        (CAR_PM_TOTAL_FEC_FRAMES      , "total-fec-frames"      )
        (CAR_PM_CORRECTED_BITS        , "corrected-bits"        )
        (CAR_PM_PHASE_CORRECTION      , "phase-correction"      )
        (CAR_PM_OSNR                  , "osnr"                  )
        (CAR_PM_TX_CD                 , "cd"                    )
        (CAR_PM_TX_FREQUENCY          , "frequency"             )
        (CAR_PM_RX_FREQUENCY          , "frequency"             );

    void addCarrierPmPair(CARRIER_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, uint64_t value, bool validity, chm6_dataplane::Chm6CarrierPm& pbPm) const;
    void addCarrierPmPair(CARRIER_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, double   value, bool validity, chm6_dataplane::Chm6CarrierPm& pbPm) const;

    enum SCG_PM_PARAMS
    {
        SCG_PM_OPT     = 0,
        SCG_PM_OPR        ,
        SCG_PM_TX_EDFA_OPT,
        SCG_PM_TX_EDFA_OPR,
        SCG_PM_TX_EDFA_LBC,
        NUM_SCG_PM_PARAMS
    };

    const boost::unordered_map<SCG_PM_PARAMS,const char*> scgPmParamsToString = boost::assign::map_list_of
        (SCG_PM_OPT        , "opt"      )
        (SCG_PM_OPR        , "opr"      )
        (SCG_PM_TX_EDFA_OPT, "edfa-opt")
        (SCG_PM_TX_EDFA_OPR, "edfa-opr")
        (SCG_PM_TX_EDFA_LBC, "edfa-lbc");

    void addScgPmPair(SCG_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, double value, bool validity, chm6_dataplane::Chm6ScgPm& pbPm) const;
};


}; // namespace DataPlane

#endif /* CARRIER_PB_TRANSLATOR_H_ */
