/**************************************************************************
   Copyright (c) 2021 Infinera
**************************************************************************/

#include <iostream>
#include <sstream>

#include "DataPlaneMgr.h"
#include "GccPbTranslator.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"

using chm6_dataplane::Chm6GccPm;

using namespace ::std;

namespace DataPlane {

void GccPbTranslator::addGccPmPair(GCC_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, uint64_t value, bool validity, Chm6GccPm& pbPm) const
{
    hal_dataplane::GccPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_pm()->mutable_pm();

    std::string pmKey(gccPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(gccPmParamsToString.at(pmParam));

    hal_common::PmType_PmDataType pmData;
    pmData.mutable_pm_data_name()->set_value(pmName);
    pmData.mutable_uint64_val()->set_value(value);

    switch (pmDirection)
    {
        case PMDIRECTION_NA:
            pmData.set_direction(hal_common::DIRECTION_NA);
            break;
        case PMDIRECTION_RX:
            pmData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case PMDIRECTION_TX:
            pmData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (pmLocation)
    {
        case PMLOCATION_NA:
            pmData.set_location(hal_common::LOCATION_NA);
            break;
        case PMLOCATION_NEAR_END:
            pmData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case PMLOCATION_FAR_END:
            pmData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    wrapper::Bool validityValue = (validity == true) ? wrapper::BOOL_TRUE: wrapper::BOOL_FALSE;
    pmData.set_validity_flag(validityValue);

    (*pmMap)[pmKey] = pmData;
}

void GccPbTranslator::pmAdToPb(const DataPlane::DpMsGccPMContainer& adPm, bool validity, Chm6GccPm& pbPm) const
{
    hal_dataplane::GccPm_OperationalPm* pmHal = pbPm.mutable_hal();
    pmHal->clear_pm();

    uint64_t undefinedField = 0;

    addGccPmPair(TX_PACKETS          , PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.controlpm.tx_packets          , validity, pbPm);
    addGccPmPair(RX_PACKETS          , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.controlpm.rx_packets          , validity, pbPm);
    addGccPmPair(TX_BYTES            , PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.controlpm.tx_bytes            , validity, pbPm);
    addGccPmPair(RX_BYTES            , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.controlpm.rx_bytes            , validity, pbPm);
    addGccPmPair(TX_PACKETS_DROPPED  , PMDIRECTION_TX, PMLOCATION_NEAR_END, adPm.controlpm.tx_packets_dropped  , validity, pbPm);
    addGccPmPair(RX_PACKETS_DROPPED  , PMDIRECTION_RX, PMLOCATION_NEAR_END, adPm.controlpm.rx_packets_dropped  , validity, pbPm);
}

void GccPbTranslator::pmAdDefault(DataPlane::DpMsGccPMContainer& adPm)
{
    memset(&adPm.controlpm, 0, sizeof(adPm.controlpm));
}


}; // namespace DataPlane
