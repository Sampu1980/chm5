/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef CARD_PB_TRANSLATOR_H_
#define CARD_PB_TRANSLATOR_H_

#include <string>

#include <DcoCardAdapter.h>
#include <DpPm.h>

#include "DpProtoDefs.h"
#include "DpCommonPbTranslator.h"
#include "FaultCacheMap.h"

namespace DataPlane {

class CardPbTranslator : public DpCommonPbTranslator
{
public:
    CardPbTranslator() {}

    ~CardPbTranslator() {}

    bool isDcoStateReady(const DcoState dcoState) const;
    bool isDcoStateLowPower(const DcoState dcoState) const;

    void configPbToAd(const chm6_common::Chm6DcoConfig& pbCfg, DpAdapter::CardConfig& adCfg) const;
    void statusAdToPb(const DpAdapter::CardStatus& adStat, chm6_common::Chm6DcoCardState& pbStat) const;
    void capaAdToPb(const DpAdapter::CardCapability& adCapa, chm6_dataplane::DcoCapabilityState& pbCapa) const;
    void faultAdToPb( const DpAdapter::CardFault& adFault,
                      chm6_common::Chm6DcoCardFault& pbFault,
                      bool isConnectFault ) const;
    void faultPCPAdToPb( chm6_common::Chm6DcoCardFault& pbFault,
                         const FaultCacheMap& faultMap ) const;
    void pmAdToPb(const DataPlane::DpMsCardPm& adPm, chm6_common::Chm6DcoCardPm& pbPm) const;
    void pmAdToPb(const DataPlane::DpMsCardPm& adPm, chm6_common::Chm6DcoCardState& pbStat) const;

    bool isPbChanged(const chm6_common::Chm6DcoCardState& pbStatPrev, const chm6_common::Chm6DcoCardState& pbStatCur);
    bool isAdChanged(const DpAdapter::CardFault& adFaultPrev, const DpAdapter::CardFault& adFaultCur);

private:

    enum CARD_FAULT_PARAMS
    {
        CARD_FAULT_DCO_LINE_ADC_PLL_LOL  = 0,
        CARD_FAULT_DCO_LINE_DAC_PLL_LOL     ,
        CARD_FAULT_DCO_CLEANUP_PLL_INPUT_LOL,
        CARD_FAULT_DCO_PROC_UNRESPONSIVE,
        CARD_FAULT_DCO_REBOOT_WARM,
        CARD_FAULT_DCO_REBOOT_COLD,
        CARD_FAULT_DCO_ZYNQ_PS_LINK_DOWN,
        CARD_FAULT_DCO_ZYNQ_PL_LINK_DOWN,
        CARD_FAULT_DCO_NXP_PL_LINK_DOWN,
        CARD_FAULT_DCO_ZYNQ_PS_LINK_CRC_ERRORS,
        CARD_FAULT_DCO_ZYNQ_PL_LINK_CRC_ERRORS,
        CARD_FAULT_DCO_NXP_PL_LINK_CRC_ERRORS,
        NUM_CARD_FAULT_PARAMS
    };
  
    const boost::unordered_map<CARD_FAULT_PARAMS,const char*> cardFaultParamsToString = boost::assign::map_list_of
        (CARD_FAULT_DCO_LINE_ADC_PLL_LOL     , "DCO-LINE-ADC-PLL-LOL"     )
        (CARD_FAULT_DCO_LINE_DAC_PLL_LOL     , "DCO-LINE-DAC-PLL-LOL"     )
        (CARD_FAULT_DCO_CLEANUP_PLL_INPUT_LOL, "DCO-CLEANUP-PLL-INPUT-LOL")
        (CARD_FAULT_DCO_PROC_UNRESPONSIVE    , "DCO-PROC-UNRESPONSIVE")
        (CARD_FAULT_DCO_REBOOT_WARM          , "SUBCOMPONENT-WARM-RESET")
        (CARD_FAULT_DCO_REBOOT_COLD          , "SUBCOMPONENT-COLD-RESET")
        (CARD_FAULT_DCO_ZYNQ_PS_LINK_DOWN    , "DCO-ZYNQ-PS-LINK-DOWN")
        (CARD_FAULT_DCO_ZYNQ_PL_LINK_DOWN    , "DCO-ZYNQ-PL-LINK-DOWN")
        (CARD_FAULT_DCO_NXP_PL_LINK_DOWN     , "DCO-NXP-PL-LINK-DOWN")
        (CARD_FAULT_DCO_ZYNQ_PS_LINK_CRC_ERRORS, "DCO-ZYNQ-PS-LINK-CRC-ERRORS")
        (CARD_FAULT_DCO_ZYNQ_PL_LINK_CRC_ERRORS, "DCO-ZYNQ-PL-LINK-CRC-ERRORS")
        (CARD_FAULT_DCO_NXP_PL_LINK_CRC_ERRORS, "DCO-NXP-PL-LINK-CRC-ERRORS");

    void addCardFaultPair(std::string faultName, DataPlane::FaultDirection faultDirection, DataPlane::FaultLocation, bool isFaulted, chm6_common::Chm6DcoCardFault& pbFault) const;

    void addCardFaultPairPCP( std::string faultName,
                              DataPlane::FaultDirection faultDirection,
                              DataPlane::FaultLocation,
                              bool isFaulted,
                              chm6_common::Chm6DcoCardFault& pbFault) const;

    enum CARD_PM_PARAMS
    {
        CARD_PM_DSP_TEMPERATURE = 0,
        CARD_PM_PIC_TEMPERATURE    ,
        CARD_PM_CASE_TEMPERATURE   ,
        NUM_CARD_PM_PARAMS
    };

    const boost::unordered_map<CARD_PM_PARAMS,const char*> cardPmParamsToString = boost::assign::map_list_of
        (CARD_PM_DSP_TEMPERATURE , "dsp-temperature" )
        (CARD_PM_PIC_TEMPERATURE , "pic-temperature" )
        (CARD_PM_CASE_TEMPERATURE, "case-temperature");

    void addCardPmPair(CARD_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, double value, chm6_common::Chm6DcoCardPm& pbPm) const;

    enum GCC_PM_PARAMS
    {
        GCC_TX_PACKETS       = 0,
        GCC_TX_BYTES            ,
        GCC_TX_PACKETS_DROPPED  ,
        GCC_RX_PACKETS          ,
        GCC_RX_BYTES            ,
        GCC_RX_PACKETS_DROPPED  ,
        GCC_TX_PACKETS_RE_TAGGED,
        GCC_RX_PACKETS_RE_TAGGED,
        NUM_GCC_PM_PARAMS
    };

    const boost::unordered_map<GCC_PM_PARAMS,const char*> gccPmParamsToString = boost::assign::map_list_of
        (GCC_TX_PACKETS          , "tx-packets"          )
        (GCC_TX_BYTES            , "tx-bytes"            )
        (GCC_TX_PACKETS_DROPPED  , "tx-packets-dropped"  )
        (GCC_RX_PACKETS          , "rx-packets"          )
        (GCC_RX_BYTES            , "rx-bytes"            )
        (GCC_RX_PACKETS_DROPPED  , "rx-packets-dropped"  )
        (GCC_TX_PACKETS_RE_TAGGED, "tx-packets-re-tagged")
        (GCC_RX_PACKETS_RE_TAGGED, "rx-packets-re-tagged");

    void addGccPmPair(GCC_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, uint64_t value, chm6_dataplane::Chm6GccPm& pbPm) const;

    void eqptFaultAdToPb( const std::vector<DpAdapter::AdapterFault>& adFault,
                         chm6_common::Chm6DcoCardFault& pbFault) const;

    void postFaultAdToPb( const std::vector<DpAdapter::AdapterFault>& adFault,
                         chm6_common::Chm6DcoCardFault& pbFault) const;
};


}; // namespace DataPlane

#endif /* CARD_PB_TRANSLATOR_H_ */
