/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_COMMON_PB_TRANSLATOR_H
#define DP_COMMON_PB_TRANSLATOR_H

#include <string>
#include <iostream>

#include "DpProtoDefs.h"
#include "dp_common.h"



namespace DataPlane
{

class DpCommonPbTranslator
{
public:
    DpCommonPbTranslator() {}
    virtual ~DpCommonPbTranslator() {}

    std::string toString(const bool                            ) const;
    std::string toString(const hal_dataplane::PortRate         ) const;
    std::string toString(const hal_dataplane::LoopBackType     ) const;
    std::string toString(const hal_dataplane::MaintenanceSignal) const;
    std::string toString(const hal_dataplane::TSGranularity    ) const;
    std::string toString(const hal_dataplane::FecMode          ) const;
    std::string toString(const hal_common::BandWidth           ) const;
    std::string toString(const hal_common::ClientMode          ) const;
    std::string toString(const hal_common::ServiceMode         ) const;
    std::string toString(const hal_common::MaintenanceAction   ) const;
    std::string toString(const hal_common::LoopbackType        ) const;
    std::string toString(const hal_common::TermLoopbackBehavior) const;
    std::string toString(const hal_common::CarrierChannel      ) const;
    std::string toString(const hal_common::TtiMismatchType     ) const;
    std::string toString(const hal_common::Cdi cdiVal          ) const;
    std::string toString(const wrapper::Bool                   ) const;
    std::string toString(const DataPlane::FaultDirection       ) const;
    std::string toString(const DataPlane::FaultLocation        ) const;
    std::string toString(const DataPlane::PmDirection          ) const;
    std::string toString(const DataPlane::PmLocation           ) const;
    std::string toString(const DataPlane::DcoState             ) const;

    void dumpJsonFormat(const std::string data, std::ostream& os) const;

    std::string getChassisSlotAid() const;

private:

};

} // namespace DataPlane

#endif /* DP_COMMON_PB_TRANSLATOR_H */
