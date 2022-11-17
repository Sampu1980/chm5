/*
 * BcmPm.h
 *
 *  Created on: Dec 14, 2020
 *      Author: aforward
 */

#ifndef SRC_GEARBOX_BCMPM_H_
#define SRC_GEARBOX_BCMPM_H_

#include "types.h"
#include "AdapterCommon.h"
#include "dp_common.h"
#include "Bcm81725.h"
#include "epdm.h"


namespace gearbox
{

const unsigned int cNumPcsLanes = 20;

const unsigned int c100gNumPhysicalLanes = 4;
const unsigned int c200gNumPhysicalLanes = 4;
const unsigned int c400gNumPhysicalLanes = 8;
const unsigned int cOtu4NumPhysicalLanes = 4;

typedef struct
{
    uint64 icgCounts;
    uint64 bip8Counts;
    uint64 bip8CountsLane[cNumPcsLanes];
} BcmPcsPm;

typedef struct
{
    uint64 rxUnCorrWord;
    uint64 rxCorrWord;
    uint64 rxCorrBits;
    uint64 fecSymbolErr;
} BcmFecPm;

// Follows enum bcm_plp_fec_mode_sel_t
// Expose this enum to upper layers instead of sdk enum
typedef enum
{
    NoFEC,
    unused1,
    unused2,
    unused3,
    PCSFEC,
    unused5,
    KR4FEC
} FecMode;

typedef struct
{
    FecMode fecMode;
    BcmPcsPm pcs;                                    // sum of all physical lanes
    BcmPcsPm pcsLanes[c400gNumPhysicalLanes];        // PMs for single physical lane
    BcmPcsPm pcsAccum;                               // accumulative of all physical lanes
    BcmPcsPm pcsLanesAccum[c400gNumPhysicalLanes];   // accumulative of a single physical lane
    BcmFecPm fec;                                    // sum of all physical lanes
    BcmFecPm fecLanes[c400gNumPhysicalLanes];        // PMs for single physical lane
    BcmFecPm fecLanesPrev[c400gNumPhysicalLanes];    // PMs for single physical lane, previous snapshot
    BcmFecPm fecAccum;                               // accumulative of all physical lanes
    BcmFecPm fecLanesAccum[c400gNumPhysicalLanes];   // accumulative of a single physical lane
} BcmPm;

typedef struct
{
    unsigned long start;                             // log start time in ns
    unsigned long stop;                              // log stop time in ns
    unsigned long deltaTime;                         // calculate time to get status registers in ns
} BcmTime;

} // end namepace gearbox

#endif /* SRC_GEARBOX_BCMPM_H_ */
