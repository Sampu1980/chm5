//-------------------------------------------------------------
//
// Copyright(c) 2020 Infinera Corporation. All right reserved.
//
//-------------------------------------------------------------

#ifndef BCM81725_AVS_UTIL_H_
#define BCM81725_AVS_UTIL_H_

#include <stdint.h>

namespace gearbox {

typedef enum MezzBrdIds
{
    MEZZ_BRD_BTM = 0,
    MEZZ_BRD_TOP,
    NUM_MEZZ_BRDS
}MezzBrdIds_t;

#define NUM_BCMS_PER_MEZZ 3

#define TPS_STATUS_WORD_ADDR 0x79

#define TPS_MAX_RW_BYTES 4

int getAvsPmbusData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData);
int setAvsPmbusData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData);

int getAvsPmbusData(const Bcm81725Lane& param, uint32_t op,uint32_t len,uint8_t *data);

int setAvsPmbusData(const Bcm81725Lane& param, uint32_t op,uint32_t len,uint8_t *data);

}


#endif /* CHM6_BOARD_MS_SRC_DRIVER_BCM81725_AVS_UTIL_H_ */
