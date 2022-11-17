/*
 * AtlanticSerdesTable.cpp
 *
 *  Created on: Feb 10, 2021
 *      Author: aforward
 */
#include "AtlanticSerdesTable.h"

namespace tuning
{



// 1 based indexing since DCO is 1 based
const Polarity polarityTbl[] =
{
    { false,  false },  // No lane 0
    { true ,  false },  // Lane 1
    { true ,  false },  // Lane 2
    { true ,  false },  // Lane 3
    { true ,  false },  // Lane 4
    { true ,  false },  // Lane 5
    { true ,  false },  // Lane 6
    { true ,  false },  // Lane 7
    { false,  true  },  // Lane 8
    { false,  false },  // Lane 9
    { true ,  false },  // Lane 10
    { false,  false },  // Lane 11
    { true ,  false },  // Lane 12
    { false,  false },  // Lane 13
    { true ,  false },  // Lane 14
    { false,  false },  // Lane 15
    { false,  false },  // Lane 16
    { true ,  false },  // Lane 17
    { false,  true  },  // Lane 18
    { false,  true  },  // Lane 19
    { false,  true  },  // Lane 20
    { false,  true  },  // Lane 21
    { false,  true  },  // Lane 22
    { false,  true  },  // Lane 23
    { false,  true  },  // Lane 24
    { false,  true  },  // Lane 25
    { false,  true  },  // Lane 26
    { true ,  true  },  // Lane 27
    { false,  true  },  // Lane 28
    { true ,  true  },  // Lane 29
    { false,  true  },  // Lane 30
    { true ,  true  },  // Lane 31
    { false,  true  }   // Lane 32
};

// 1 based indexing since DCO is 1 based
// lanes are 1 based
const AtlPortLanes100GE atlPortLanes100Ge[] =
{
        { 0,  0}, // offset so index matches portId
        {15, 16}, // Port 1
        {13, 14}, // Port 2
        { 9, 10}, // Port 3
        {11, 12}, // Port 4
        { 1,  2}, // Port 5
        { 3,  4}, // Port 6
        { 5,  6}, // Port 7
        { 7,  8}, // Port 8
        {25, 26}, // Port 9
        {27, 28}, // Port 10
        {31, 32}, // Port 11
        {29, 30}, // Port 12
        {23, 24}, // Port 13
        {21, 22}, // Port 14
        {19, 20}, // Port 15
        {17, 18}  // Port 16
};

const size_t cAtlPortLanesTblSize = sizeof(atlPortLanes100Ge) / sizeof(AtlPortLanes100GE);

// 1 based indexing since DCO is 1 based
// lanes are 1 based
const AtlPortLanes200GE atlPortLanes200Ge[] =
{
        { 0,  0,  0,  0}, // offset so index matches portId
        {15, 16, 13, 14}, // Port 1
        { 0,  0,  0,  0}, // no port 2
        { 9, 10, 11, 12}, // Port 3
        { 0,  0,  0,  0}, // no port 4
        { 0,  0,  0,  0}, // no port 5
        { 1,  2,  3,  4}, // Port 6
        { 0,  0,  0,  0}, // no port 7
        { 5,  6,  7,  8}, // Port 8
        {25, 26, 27, 28}, // Port 9
        { 0,  0,  0,  0}, // no port 10
        {31, 32, 29, 30}, // Port 11
        { 0,  0,  0,  0}, // no port 12
        { 0,  0,  0,  0}, // no port 13
        {23, 24, 21, 22}, // Port 14
        { 0,  0,  0,  0}, // no port 15
        {19, 20, 17, 18}, // Port 16
};

// 1 based indexing since DCO is 1 based
// lanes are 1 based
const AtlPortLanes400GE atlPortLanes400Ge[] =
{
        { 0,  0,  0,  0,  0,  0,  0,  0}, // offset so index matches portId
        {15, 16, 13, 14,  9, 10, 11, 12}, // Port 1
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 2
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 3
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 4
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 5
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 6
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 7
        { 1,  2,  3,  4,  5,  6,  7,  8}, // Port 8
        {25, 26, 27, 28, 31, 32, 29, 30}, // Port 9
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 10
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 11
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 12
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 13
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 14
        { 0,  0,  0,  0,  0,  0,  0,  0}, // no port 15
        {23, 24, 21, 22, 19, 20, 17, 18}, // Port 16
};

// 1 based indexing since DCO is 1 based
// lanes are 1 based
const AtlPortLanesOtu4 atlPortLanesOtu4[] =
{
        { 0,  0}, // offset so index matches portId
        {15, 16}, // Port 1
        {13, 14}, // Port 2
        { 9, 10}, // Port 3
        {11, 12}, // Port 4
        { 1,  2}, // Port 5
        { 3,  4}, // Port 6
        { 5,  6}, // Port 7
        { 7,  8}, // Port 8
        {25, 26}, // Port 9
        {27, 28}, // Port 10
        {31, 32}, // Port 11
        {29, 30}, // Port 12
        {23, 24}, // Port 13
        {21, 22}, // Port 14
        {19, 20}, // Port 15
        {17, 18}  // Port 16
};

}
