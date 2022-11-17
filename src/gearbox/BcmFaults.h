/*
 * BcmFaults.h
 *
 *  Created on: Dec 8, 2020
 *      Author: aforward
 */

#ifndef SRC_GEARBOX_BCMFAULTS_H_
#define SRC_GEARBOX_BCMFAULTS_H_

#include "types.h"
#include "AdapterCommon.h"
#include "dp_common.h"
#include "Bcm81725.h"
#include "epdm.h"
namespace gearbox
{




typedef enum BcmFaultIdx
{
    LOSYNC,
    LOLM,
    LOA,
    HIBER
} BcmFaultIdx_t;


typedef struct
{
    BcmFaultIdx_t faultIdx;
    DpAdapter::AdapterFault adapterFaults;
} BcmAdapterFaults;

const BcmAdapterFaults bcmAdapterFaults[] =
{
    { LOSYNC, {"LOSYNC", false, DataPlane::FAULTDIRECTION_RX, DataPlane::FAULTLOCATION_NEAR_END} },
    { LOLM,   {"LOLM"  , false, DataPlane::FAULTDIRECTION_RX, DataPlane::FAULTLOCATION_NEAR_END} },
    { LOA,    {"LOA"   , false, DataPlane::FAULTDIRECTION_RX, DataPlane::FAULTLOCATION_NEAR_END} },
    { HIBER,  {"HIBER" , false, DataPlane::FAULTDIRECTION_RX, DataPlane::FAULTLOCATION_NEAR_END} },
};

const unsigned int cNumBcmFaults = sizeof(bcmAdapterFaults) / sizeof(BcmAdapterFaults);






}
#endif /* SRC_GEARBOX_BCMFAULTS_H_ */
