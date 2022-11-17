/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "DataPlaneMgr.h"
#include "DpEnv.h"
#include "DpCommonPbTranslator.h"
#include "InfnLogger.h"

#include <iostream>
#include <sstream>


namespace DataPlane {

std::string DpCommonPbTranslator::toString(const bool enable) const
{
    std::string statusStr = (true == enable) ? "enable" : "disable";
    return statusStr;
}

std::string DpCommonPbTranslator::toString(const hal_dataplane::PortRate portRate) const
{
    switch (portRate)
    {
        case hal_dataplane::PORT_RATE_ETH100G: return ("ETH100G");
        case hal_dataplane::PORT_RATE_ETH400G: return ("ETH400G");
        default:                               return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_dataplane::LoopBackType lpbkType) const
{
    switch (lpbkType)
    {
        case hal_dataplane::LOOP_BACK_TYPE_OFF:      return ("OFF");
        case hal_dataplane::LOOP_BACK_TYPE_FACILITY: return ("FACILITY");
        case hal_dataplane::LOOP_BACK_TYPE_TERMINAL: return ("TERMINAL");
        default:                                     return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_dataplane::MaintenanceSignal maintenanceSignal) const
{
    switch (maintenanceSignal)
    {
        case hal_dataplane::MAINTENANCE_SIGNAL_IDLE:      return ("IDLE");
        case hal_dataplane::MAINTENANCE_SIGNAL_LF:        return ("LF");
        case hal_dataplane::MAINTENANCE_SIGNAL_RF:        return ("RF");
        case hal_dataplane::MAINTENANCE_SIGNAL_NOREPLACE: return ("NOREPLACE");
        case hal_dataplane::MAINTENANCE_SIGNAL_LASER_OFF: return ("LASER_OFF");
        default:                                          return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_dataplane::TSGranularity tsGranularity) const
{
    switch (tsGranularity)
    {
        case hal_dataplane::TS_GRANULARITY_25G: return ("25G");
        default:                                return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_dataplane::FecMode fecMode) const
{
    switch (fecMode)
    {
        case hal_dataplane::FEC_MODE_DISABLE_FEC: return ("DISABLE");
        case hal_dataplane::FEC_MODE_G709:        return ("G709");
        case hal_dataplane::FEC_MODE_I4:          return ("I4");
        case hal_dataplane::FEC_MODE_I7:          return ("I7");
        case hal_dataplane::FEC_MODE_C_FEC:       return ("C-FEC");
        case hal_dataplane::FEC_MODE_O_FEC:       return ("O-FEC");
        case hal_dataplane::FEC_MODE_SC_FEC:      return ("SC-FEC");
        case hal_dataplane::FEC_MODE_SDFEC_15:    return ("SDFEC-15");
        case hal_dataplane::FEC_MODE_SDFEC_15_ND: return ("SDFEC-15-ND");
        case hal_dataplane::FEC_MODE_UFEC_7:      return ("UFEC-7");
        default:                                  return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::BandWidth bandwidth) const
{
    switch (bandwidth)
    {
        case hal_common::BAND_WIDTH_100GB_ELAN: return ("100GB_ELAN");
        case hal_common::BAND_WIDTH_400GB_ELAN: return ("400GB_ELAN");
        case hal_common::BAND_WIDTH_OTUCNI    : return ("OTUCNI");
        case hal_common::BAND_WIDTH_OTU4      : return ("OTU4");
        case hal_common::BAND_WIDTH_LO_ODU4I  : return ("LO_ODU4I");
        case hal_common::BAND_WIDTH_LO_FLEXI  : return ("LO_FLEXI");
        case hal_common::BAND_WIDTH_ODU4      : return ("ODU4");
        default:                                return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::ClientMode clientMode) const
{
    switch (clientMode)
    {
        case hal_common::CLIENT_MODE_LXTP_E: return ("Ethernet");
        case hal_common::CLIENT_MODE_LXTP_M: return ("Mixed");
        case hal_common::CLIENT_MODE_LXTP  : return ("LXTP");
        default:                             return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::ServiceMode serviceMode) const
{
    switch (serviceMode)
    {
        case hal_common::SERVICE_MODE_NONE:       return ("NONE");
        case hal_common::SERVICE_MODE_SWITCHING:  return ("SWITCHING");
        case hal_common::SERVICE_MODE_WRAPPER:    return ("WRAPPER");
        case hal_common::SERVICE_MODE_MUXED:      return ("MUXED");
        case hal_common::SERVICE_MODE_ADAPTATION: return ("ADAPTATION");
        case hal_common::SERVICE_MODE_TRANSPORT:  return ("TRANSPORT");
        default:                                  return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::MaintenanceAction maintenanceAction) const
{
    switch (maintenanceAction)
    {
        case hal_common::MAINTENANCE_ACTION_SENDAIS:      return ("SENDAIS");
        case hal_common::MAINTENANCE_ACTION_SENDOCI:      return ("SENDOCI");
        case hal_common::MAINTENANCE_ACTION_SENDLCK:      return ("SENDLCK");
        case hal_common::MAINTENANCE_ACTION_PRBS_X31:     return ("PRBS_X31");
        case hal_common::MAINTENANCE_ACTION_PRBS_X31_INV: return ("PRBS_X31_INV");
        case hal_common::MAINTENANCE_ACTION_NOREPLACE:    return ("NOREPLACE");
        default:                                          return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::LoopbackType lpbkType) const
{
    switch (lpbkType)
    {
        case hal_common::LOOPBACK_TYPE_NONE:     return ("NONE");
        case hal_common::LOOPBACK_TYPE_TERMINAL: return ("TERMINAL");
        case hal_common::LOOPBACK_TYPE_FACILITY: return ("FACILITY");
        default:                                 return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::TermLoopbackBehavior lpbkBehavior) const
{
    switch (lpbkBehavior)
    {
        case hal_common::TERM_LOOPBACK_BEHAVIOR_BRIDGESIGNAL: return ("BRIDGESIGNAL");
        case hal_common::TERM_LOOPBACK_BEHAVIOR_TURNOFFLASER: return ("TURNOFFLASER");
        default:                                              return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::CarrierChannel carrierChannel) const
{
    switch (carrierChannel)
    {
        case hal_common::CARRIER_CHANNEL_ONE:  return ("ONE");
        case hal_common::CARRIER_CHANNEL_TWO:  return ("TWO");
        case hal_common::CARRIER_CHANNEL_BOTH: return ("BOTH");
        default:                               return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::TtiMismatchType ttiMismatchType) const
{
    switch (ttiMismatchType)
    {
        case hal_common::TTI_MISMATCH_TYPE_DISABLED:        return ("DISABLED");
        case hal_common::TTI_MISMATCH_TYPE_FULL_64BYTES:    return ("FULL_64BYTES");
        case hal_common::TTI_MISMATCH_TYPE_SAPI:            return ("SAPI");
        case hal_common::TTI_MISMATCH_TYPE_DAPI:            return ("DAPI");
        case hal_common::TTI_MISMATCH_TYPE_OPER:            return ("OPER");
        case hal_common::TTI_MISMATCH_TYPE_SAPI_DAPI:       return ("SAPI_DAPI");
        case hal_common::TTI_MISMATCH_TYPE_SAPI_OPER:       return ("SAPI_OPER");
        case hal_common::TTI_MISMATCH_TYPE_DAPI_OPER:       return ("DAPI_OPER");
        case hal_common::TTI_MISMATCH_TYPE_SAPI_DAPI_OPER:  return ("SAPI_DAPI_OPER");
        default:                                            return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const hal_common::Cdi cdiVal) const
{
    switch (cdiVal)
    {
        case hal_common::CDI_UNSPECIFIED:               return ("UNSPECIFIED");
        case hal_common::CDI_LOCAL_DEGRADED:            return ("LOCAL-DEGRADED");
        case hal_common::CDI_REMOTE_DEGRADED:           return ("REMOTE-DEGRADED");
        case hal_common::CDI_NONE:                      return ("NONE");
        case hal_common::CDI_UNKNOWN:                   return ("UNKNOWN");
        case hal_common::CDI_LOCAL_AND_REMOTE_DEGRADED: return ("LOCAL_AND_REMOTE_DEGRADED");
        default:                                        return ("UNKNOWN");
    }
}


std::string DpCommonPbTranslator::toString(const wrapper::Bool boolVal) const
{
    switch (boolVal)
    {
        case wrapper::BOOL_TRUE:  return ("True");
        case wrapper::BOOL_FALSE: return ("False");
        default:                  return ("UNKNOWN");
    }
}

std::string DpCommonPbTranslator::toString(const FaultDirection direction) const
{
    switch (direction)
    {
        case FAULTDIRECTION_NA: return ("NA"         );
        case FAULTDIRECTION_RX: return ("INGRESS"    );
        case FAULTDIRECTION_TX: return ("EGRESS"     );
        default:                return ("UNSPECIFIED");
    }
}

std::string DpCommonPbTranslator::toString(const FaultLocation location) const
{
    switch (location)
    {
        case FAULTLOCATION_NA:       return ("NA"         );
        case FAULTLOCATION_NEAR_END: return ("NEAR-END"   );
        case FAULTLOCATION_FAR_END:  return ("FAR-END"    );
        default:                     return ("UNSPECIFIED");
    }
}

std::string DpCommonPbTranslator::toString(const PmDirection direction) const
{
    switch (direction)
    {
        case PMDIRECTION_NA: return ("na"         );
        case PMDIRECTION_RX: return ("ingress"    );
        case PMDIRECTION_TX: return ("egress"     );
        default:             return ("unspecified");
    }
}

std::string DpCommonPbTranslator::toString(const PmLocation location) const
{
    switch (location)
    {
        case PMLOCATION_NA:       return ("na"         );
        case PMLOCATION_NEAR_END: return ("near-end"   );
        case PMLOCATION_FAR_END:  return ("far-end"    );
        default:                  return ("unspecified");
    }
}

std::string DpCommonPbTranslator::toString(const DcoState docState) const
{
    switch (docState)
    {
        case DCOSTATE_UNSPECIFIED: return ("UNSPECIFIED");
        case DCOSTATE_BRD_INIT:    return ("BRD_INIT");
        case DCOSTATE_DSP_CONFIG:  return ("DSP_CONFIG");
        case DCOSTATE_LOW_POWER:   return ("LOW_POWER");
        case DCOSTATE_POWER_UP:    return ("POWER_UP");
        case DCOSTATE_POWER_DOWN:  return ("POWER_DOWN");
        case DCOSTATE_FAULTED:     return ("FAULTED");
        default:                   return ("UNKNOWN");
    }
}

void DpCommonPbTranslator::dumpJsonFormat(const std::string data, std::ostream& os) const
{
    std::string jsonError;
    Json::Value jsonResponse;
    Json::CharReaderBuilder jsonReaderBuilder;
    std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());
    reader->parse(data.c_str(), data.c_str() + data.size(), &jsonResponse, &jsonError);

    Json::StreamWriterBuilder jsonWriterBuilder;
    std::unique_ptr<Json::StreamWriter> const writer(jsonWriterBuilder.newStreamWriter());
    writer->write(jsonResponse, &os);
}

std::string DpCommonPbTranslator::getChassisSlotAid() const
{
    std::string aid =        DataPlaneMgrSingleton::Instance()->getChassisIdStr() +
                      "-"  + dp_env::DpEnvSingleton::Instance()->getSlotNum();
    return (aid);
}


}; // namespace DataPlane

