/*
 * AtlanticSerdesTable.h
 *
 *  Created on: Oct 10, 2020
 *      Author: aforward
 */

#ifndef SRC_COMMON_ATLANTICSERDESTABLE_H_
#define SRC_COMMON_ATLANTICSERDESTABLE_H_
#include <iostream>
#include "dp_common.h"



namespace tuning
{

const unsigned int cAtlNumLanes100Ge   = 2;
const unsigned int cAtlNumLanes200Ge   = 4;
const unsigned int cAtlNumLanes400Ge   = 8;
const unsigned int cAtlNumLanesOtu4    = 2;
const unsigned int cAtlNumLanes4_100Ge = 2; // if there are additional payloads added, be sure to add to cAtlNumLanes

const unsigned int cAtlNumLanes[] = { cAtlNumLanes100Ge, cAtlNumLanes400Ge, cAtlNumLanes200Ge, cAtlNumLanesOtu4, cAtlNumLanes4_100Ge };
const size_t cAtlNumLanesSize = (sizeof(cAtlNumLanes) / sizeof(unsigned int));

struct Polarity
{
    bool txPolarity;
    bool rxPolarity;
};

typedef struct
{
    bool         txPolarity;
    bool         rxPolarity;
    int          txPre2;
    int          txPre1;
    int          txMain;
    int          txPost1;
    int          txPost2;
    int          txPost3;
} AtlBcmSerdesTxCfg;

typedef struct
{
    unsigned int laneNum[cAtlNumLanes100Ge];
} AtlPortLanes100GE;

typedef struct
{
    unsigned int laneNum[cAtlNumLanes200Ge];
} AtlPortLanes200GE;

typedef struct
{
    unsigned int laneNum[cAtlNumLanes400Ge];
} AtlPortLanes400GE;

typedef struct
{
    unsigned int laneNum[cAtlNumLanesOtu4];
} AtlPortLanesOtu4;

extern const Polarity polarityTbl[];
extern const size_t cAtlPortLanesTblSize;
extern const AtlPortLanes100GE atlPortLanes100Ge[];
extern const AtlPortLanes200GE atlPortLanes200Ge[];
extern const AtlPortLanes400GE atlPortLanes400Ge[];
extern const AtlPortLanesOtu4  atlPortLanesOtu4[];

// Make the table one base
static tuning::AtlBcmSerdesTxCfg serdesCfgTbl[DataPlane::DCO_MAX_SERDES_LANES+1] =
{
    // No lane 0: one based table
    { false, false, 0,     0,     0,     0,      0,      0},
    // txPol rxPol TxPre2 TxPre1 TxMain TxPost1 TxPost2 TxPost3
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 1
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 2
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 3
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 4
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 5
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 6
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 7
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 8
    { false, false, 0,     -16,   128,     0,      0,      0}, // Lane 9
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 10
    { false, false, 0,     -16,   128,     0,      0,      0}, // Lane 11
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 12
    { false, false, 0,     -16,   128,     0,      0,      0}, // Lane 13
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 14
    { false, false, 0,     -16,   128,     0,      0,      0}, // Lane 15
    { false, false, 0,     -16,   128,     0,      0,      0}, // Lane 16
    { true,  false, 0,     -16,   128,     0,      0,      0}, // Lane 17
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 18
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 19
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 20
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 21
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 22
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 23
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 24
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 25
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 26
    { true,  true,  0,     -16,   128,     0,      0,      0}, // Lane 27
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 28
    { true,  true,  0,     -16,   128,     0,      0,      0}, // Lane 29
    { false, true,  0,     -16,   128,     0,      0,      0}, // Lane 30
    { true,  true,  0,     -16,   128,     0,      0,      0}, // Lane 31
    { false, true,  0,     -16,   128,     0,      0,      0} // Lane 32
};  // serdesCfgTbl



}

#endif /* SRC_COMMON_ATLANTICSERDESTABLE_H_ */
