/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef ODU_PB_TRANSLATOR_H_
#define ODU_PB_TRANSLATOR_H_

#include <string>

#include <DcoOduAdapter.h>
#include <DpPm.h>

#include "DpProtoDefs.h"
#include "DpCommonPbTranslator.h"


namespace DataPlane {

class OduPbTranslator : public DpCommonPbTranslator
{
public:
    OduPbTranslator() {}

    ~OduPbTranslator() {}

    int aidToOduId(std::string aid, chm6_dataplane::Chm6OduConfig& oduCfg) const;

    void configPbToAd(const chm6_dataplane::Chm6OduConfig& pbCfg, DpAdapter::OduCommon& adCfg) const; 
    void configAdToPb(const DpAdapter::OduCommon& adCfg, chm6_dataplane::Chm6OduState& pbStat) const;
    void statusAdToPb(const DpAdapter::OduStatus& adStat, chm6_dataplane::Chm6OduState& pbStat) const; 
    void statusAdToPb(const uint8_t* rxTtiPtr, chm6_dataplane::Chm6OduState& pbStat) const;
    void statusAdToPb(const DpAdapter::Cdi cdi, chm6_dataplane::Chm6OduState& pbStat) const;
    void faultAdToPb(std::string aid, const uint64_t& faultBmp, const DpAdapter::OduFault& adFault, chm6_dataplane::Chm6OduFault& pbFault, bool force) const;
    void faultAdToPbSim(const uint64_t& fault, chm6_dataplane::Chm6OduFault& pbFault) const;
    void pmAdToPb(const DataPlane::DpMsOduPMContainer& adPm, bool isOtuLof, bool validity, chm6_dataplane::Chm6OduPm& pbPm) const;
    void pmAdDefault(DataPlane::DpMsOduPMContainer& adPm);
    bool isPbChanged(const chm6_dataplane::Chm6OduState& pbStatPrev, const chm6_dataplane::Chm6OduState& pbStatCur) const;
    bool isAdChanged(const DpAdapter::OduFault& adFaultPrev, const DpAdapter::OduFault& adFaultCur) const;
    bool isOduHardFault(const DpAdapter::OduFault& adFault, const FaultDirection direction) const;
    void addOduTimFault(bool faulted, DpAdapter::OduFault& adFault) const;
    void addOduSdFault(bool faulted, DpAdapter::OduFault& adFault, FaultDirection direction) const;
    void addOduAisOnLoflomMsim(DpAdapter::OduFault& adFault) const;

private:

    enum ODU_FAULT_PARAMS
    {
        ODU_FAULT_AIS           = 0,
        ODU_FAULT_BDI           ,
        ODU_FAULT_CSF           ,
        ODU_FAULT_LCK           ,
        ODU_FAULT_LOFLOM_INGRESS,
        ODU_FAULT_MSIM          ,
        ODU_FAULT_OCI           ,
        ODU_FAULT_PARTFAIL      ,
        ODU_FAULT_PLM           ,
        ODU_FAULT_SD            ,
        ODU_FAULT_TIM           ,
        ODU_FAULT_LOFLOM_EGRESS ,
        ODU_PRBS_OOS_EGRESS     ,
        ODU_PRBS_OOS_INGRESS    ,
        ODU_FAULT_SSF           ,
        NUM_ODU_FAULT_PARAMS
    };

    const boost::unordered_map<ODU_FAULT_PARAMS,const char*> oduFaultParamsToString = boost::assign::map_list_of
        (ODU_FAULT_AIS           ,  "AIS"     )
        (ODU_FAULT_BDI           ,  "BDI"     )
        (ODU_FAULT_CSF           ,  "CSF"     )
        (ODU_FAULT_LCK           ,  "LCK"     )
        (ODU_FAULT_LOFLOM_INGRESS,  "LOFLOM"  )
        (ODU_FAULT_MSIM          ,  "MSIM"    )
        (ODU_FAULT_OCI           ,  "OCI"     )
        (ODU_FAULT_PARTFAIL      ,  "PARTFAIL")
        (ODU_FAULT_PLM           ,  "PLM"     )
        (ODU_FAULT_SD            ,  "SD"      )
        (ODU_FAULT_TIM           ,  "TIM"     )
        (ODU_FAULT_LOFLOM_EGRESS ,  "LOFLOM"  )
        (ODU_PRBS_OOS_EGRESS     ,  "PRBS-OOS")
        (ODU_PRBS_OOS_INGRESS    ,  "PRBS-OOS")
        (ODU_FAULT_SSF           ,  "SSF");

    void addOduFaultPair(std::string faultName, DataPlane::FaultDirection faultDirection, DataPlane::FaultLocation faultLocation, bool value, chm6_dataplane::Chm6OduFault& pbFault) const;

    enum ODU_PM_PARAMS
    {
        ODU_PM_ERRORED_BLOCKS_NEAR = 0  ,
        ODU_PM_ERRORED_BLOCKS_FAR       ,
        ODU_PM_PRBS_ERRORS_INGRESS      ,
        ODU_PM_PRBS_ERRORS_EGRESS       ,
        ODU_PM_PRBS_SYNC_ERRORS_INGRESS ,
        ODU_PM_PRBS_SYNC_ERRORS_EGRESS  ,
        NDM_ODU_PM_PARAMS
    };

    const boost::unordered_map<ODU_PM_PARAMS,const char*> oduPmParamsToString = boost::assign::map_list_of
        (ODU_PM_ERRORED_BLOCKS_NEAR       , "errored-blocks"     )
        (ODU_PM_ERRORED_BLOCKS_FAR        , "errored-blocks"     )
        (ODU_PM_PRBS_ERRORS_INGRESS       , "prbs-errors"        )
        (ODU_PM_PRBS_ERRORS_EGRESS        , "prbs-errors"        )
        (ODU_PM_PRBS_SYNC_ERRORS_INGRESS  , "prbs-sync-errors"   )
        (ODU_PM_PRBS_SYNC_ERRORS_EGRESS   , "prbs-sync-errors"   );

    void addOduPmPair(ODU_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, uint64_t value, bool validity, chm6_dataplane::Chm6OduPm& pbPm) const;
};


}; // namespace DataPlane

#endif /* ODU_PB_TRANSLATOR_H_ */
