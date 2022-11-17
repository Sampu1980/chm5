/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef OTU_PB_TRANSLATOR_H_
#define OTU_PB_TRANSLATOR_H_

#include <string>

#include <DcoOtuAdapter.h>
#include <DpPm.h>

#include "DpProtoDefs.h"
#include "DpCommonPbTranslator.h"


namespace DataPlane {

class OtuPbTranslator : public DpCommonPbTranslator
{
public:
    OtuPbTranslator() {}

    ~OtuPbTranslator() {}

    std::string tomToOtnAid(uint tomId) const;
    uint otnAidToTom(std::string otnAid) const;

    void configPbToAd(const chm6_dataplane::Chm6OtuConfig& pbCfg, DpAdapter::OtuCommon& adCfg) const;
    void configAdToPb(const DpAdapter::OtuCommon& adCfg, chm6_dataplane::Chm6OtuState& pbStat) const;
    void statusAdToPb(const DpAdapter::OtuStatus& adStat, chm6_dataplane::Chm6OtuState& pbStat) const;
    void statusAdToPb(const uint8_t* rxTtiPtr, chm6_dataplane::Chm6OtuState& pbStat) const;
    void faultAdToPb(std::string aid, const uint64_t& faultBmp, const DpAdapter::OtuFault& adFault, chm6_dataplane::Chm6OtuFault& pbFault, bool force) const;
    void faultAdToPbSim(const uint64_t& fault, chm6_dataplane::Chm6OtuFault& pbFault) const;
    void pmAdToPb(const DataPlane::DpMsOtuPMSContainer& adPm, bool fecCorrEnable, bool otuTimFault, bool validity, chm6_dataplane::Chm6OtuPm& pbPm) const;
    void pmAdDefault(DataPlane::DpMsOtuPMSContainer& adPm);
    bool isPbChanged(const chm6_dataplane::Chm6OtuState& pbStatPrev, const chm6_dataplane::Chm6OtuState& pbStatCur) const;
    bool isAdChanged(const DpAdapter::OtuFault& adFaultPrev, const DpAdapter::OtuFault& adFaultCur) const;
    bool isOtuHardFault(const DpAdapter::OtuFault& adFault, const FaultDirection direction) const;
    bool isOtuIaeFault(const DpAdapter::OtuFault& adFault, const FaultDirection direction) const;
    bool isOtuLofFault(const DpAdapter::OtuFault& adFault, const FaultDirection direction) const;
    void addOtuTimFault(bool faulted, DpAdapter::OtuFault& adFault) const;
    void addOtuSdFault(bool faulted, DpAdapter::OtuFault& adFault) const;

private:

    enum OTU_FAULT_PARAMS
    {
        OTU_FAULT_AIS  = 0,
        OTU_FAULT_BDI     ,
        OTU_FAULT_LOFLOM  ,
        OTU_FAULT_PARTFAIL,
        OTU_FAULT_SD      ,
        OTU_FAULT_TIM     ,
        OTU_FAULT_LOF     ,
        OTU_FAULT_LOM     ,
        OTU_FAULT_LOFLANE ,
        OTU_FAULT_LOL     ,
        OTU_FAULT_IAE     ,
        NUM_OTU_FAULT_PARAMS
    };

    const boost::unordered_map<OTU_FAULT_PARAMS,const char*> otuFaultParamsToString = boost::assign::map_list_of
        (OTU_FAULT_AIS     , "AIS"     )
        (OTU_FAULT_BDI     , "BDI"     )
        (OTU_FAULT_LOFLOM  , "LOFLOM"  )
        (OTU_FAULT_PARTFAIL, "PARTFAIL")
        (OTU_FAULT_SD      , "SD"      )
        (OTU_FAULT_TIM     , "TIM"     )
        (OTU_FAULT_LOF     , "LOF"     )
        (OTU_FAULT_LOM     , "LOM"     )
        (OTU_FAULT_LOFLANE , "LOFLANE" )
        (OTU_FAULT_LOL     , "LOL"     )
        (OTU_FAULT_IAE     , "IAE"     );

    void addOtuFaultPair(std::string faultName, DataPlane::FaultDirection faultDirection, DataPlane::FaultLocation faultLocation, bool value, chm6_dataplane::Chm6OtuFault& pbFault) const;

    enum OTU_PM_PARAMS
    {
        OTU_PM_ERRORED_BLOCKS_NEAR = 0,
        OTU_PM_ERRORED_BLOCKS_FAR     ,
        OTU_PM_PROPAGATION_DELAY      ,
        OTU_PM_CODE_WORDS             ,
        OTU_PM_UNCORRECTED_WORDS      ,
        OTU_PM_CORRECTED_BITS         ,
        OTU_PM_PRE_FEC_BER            ,
        OTU_PM_IAE                    ,
        OTU_PM_BIAE                   ,
        NUM_OTU_PM_PARAMS
    };

    const boost::unordered_map<OTU_PM_PARAMS,const char*> otuPmParamsToString = boost::assign::map_list_of
        (OTU_PM_ERRORED_BLOCKS_NEAR, "errored-blocks"     )
        (OTU_PM_ERRORED_BLOCKS_FAR , "errored-blocks"     )
        (OTU_PM_PROPAGATION_DELAY  , "propagation-delay"  )
        (OTU_PM_CODE_WORDS         , "code-words"         )
        (OTU_PM_UNCORRECTED_WORDS  , "uncorrected-words"  )
        (OTU_PM_CORRECTED_BITS     , "corrected-bits"     )
        (OTU_PM_PRE_FEC_BER        , "pre-fec-ber"        )
        (OTU_PM_IAE                , "iae"                )
        (OTU_PM_BIAE               , "biae"               );

    void addOtuPmPair(OTU_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, uint64_t value, bool validity, chm6_dataplane::Chm6OtuPm& pbPm) const;
    void addOtuPmPair(OTU_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, double   value, bool validity, chm6_dataplane::Chm6OtuPm& pbPm) const;
};


}; // namespace DataPlane

#endif /* OTU_PB_TRANSLATOR_H_ */
