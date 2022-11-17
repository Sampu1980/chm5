#ifndef SerdesConnMap_h
#define SerdesConnMap_h

#include "SerdesTuner.h"

namespace tuning
{
//v1.1 Loss ID
// ============= 100GE BCM to TOM =============
const LinkData BCM_to_qsfp28_connections[] =
{
    //Src DevId, Src lane#, Dest DevId, Dest Port, LinkID
    // BCM #2 - TMZ
    { 2, 12,  1,  0, 213},  // Port#1
    { 2, 13,  1,  1, 212},
    { 2, 14,  1,  2, 215},
    { 2, 15,  1,  3, 214},
    
    { 2,  8,  2,  0, 130},  // Port#2
    { 2,  9,  2,  1, 131},
    { 2, 10,  2,  2, 128},
    { 2, 11,  2,  3, 129},
    
    // BCM #3 - TMZ
    { 3,  0,  3,  0, 122},  // Port#3
    { 3,  1,  3,  1, 123},
    { 3,  2,  3,  2, 120},
    { 3,  3,  3,  3, 121},
                        
    { 3,  4,  4,  0, 126},  // Port#4
    { 3,  5,  4,  1, 127},
    { 3,  6,  4,  2, 124},
    { 3,  7,  4,  3, 127},
    
    { 3,  8,  5,  0,  96},  // Port#5
    { 3,  9,  5,  1,  97},
    { 3, 10,  5,  2,  98},
    { 3, 11,  5,  3,  99},
                        
    { 3, 12,  6,  0, 100},  // Port#6
    { 3, 13,  6,  1, 101},
    { 3, 14,  6,  2, 102},
    { 3, 15,  6,  3, 103},

    // BCM #1 - TMZ
    { 1,  8,  7,  0, 104},  // Port#7
    { 1,  9,  7,  1, 105},
    { 1, 10,  7,  2, 106},
    { 1, 11,  7,  3, 107},
                        
    { 1, 12,  8,  0, 196},  // Port#8
    { 1, 13,  8,  1, 197},
    { 1, 14,  8,  2, 198},
    { 1, 15,  8,  3, 199},

    // BCM #5 = BMZ Bcm#2
    { 5, 12,  9,  0, 229},  // Port#9
    { 5, 13,  9,  1, 228},
    { 5, 14,  9,  2, 231},
    { 5, 15,  9,  3, 230},
                        
    { 5,  8, 10,  0, 154},  // Port#10
    { 5,  9, 10,  1, 155},
    { 5, 10, 10,  2, 152},
    { 5, 11, 10,  3, 153},
    
    // BCM #6 - BMZ BCM#3
    { 6,  0, 11,  0, 146},  // Port#11
    { 6,  1, 11,  1, 147},
    { 6,  2, 11,  2, 144},
    { 6,  3, 11,  3, 145},
                        
    { 6,  4, 12,  0, 150},  // Port#12
    { 6,  5, 12,  1, 151},
    { 6,  6, 12,  2, 148},
    { 6,  7, 12,  3, 149},
    
    { 6,  8, 13,  0, 168},  // Port#13
    { 6,  9, 13,  1, 169},
    { 6, 10, 13,  2, 170},
    { 6, 11, 13,  3, 171},
                        
    { 6, 12, 14,  0, 172},  // Port#14
    { 6, 13, 14,  1, 173},
    { 6, 14, 14,  2, 174},
    { 6, 15, 14,  3, 175},

    // BCM #4 - BMZ BCM#1
    { 4,  8, 15,  0, 176},  // Port#15
    { 4,  9, 15,  1, 177},
    { 4, 10, 15,  2, 178},
    { 4, 11, 15,  3, 179},
                        
    { 4, 12, 16,  0, 244},  // Port#16
    { 4, 13, 16,  1, 245},
    { 4, 14, 16,  2, 246},
    { 4, 15, 16,  3, 247},
};
const size_t kLinksFromBcmToQsfp28 = sizeof(BCM_to_qsfp28_connections) / sizeof(tuning::LinkData);

// v1.1 Loss ID
// ============= 100GE TOM to BCM =============
const LinkData Qsfp28_to_BCM_connections[] =
{
    //Src DevId, Src lane#, Dest DevId, Dest Port, LinkID
    // BCM #2 - TMZ
    { 1,  0, 2, 12, 220},  // Port#1
    { 1,  1, 2, 13, 221},
    { 1,  2, 2, 14, 222},
    { 1,  3, 2, 15, 223},

    { 2,  0, 2,  8, 140},  // Port#2
    { 2,  1, 2,  9, 141},
    { 2,  2, 2, 10, 142},
    { 2,  3, 2, 11, 143},

    // BCM #3 - TMZ
    {3,  0,  3,  0,  132},  // Port#3
    {3,  1,  3,  1,  133},
    {3,  2,  3,  2,  134},
    {3,  3,  3,  3,  135},

    {4,  0,  3,  4,  138},  // Port#4
    {4,  1,  3,  5,  139},
    {4,  2,  3,  6,  136},
    {4,  3,  3,  7,  137},

    {5,  0,  3,  8,  108},  // Port#5
    {5,  1,  3,  9,  109},
    {5,  2,  3, 10,  110},
    {5,  3,  3, 11,  111},

    {6,  0,  3, 12,  112},  // Port#6
    {6,  1,  3, 13,  113},
    {6,  2,  3, 14,  114},
    {6,  3,  3, 15,  115},

    // BCM #1 - TMZ
    { 7,  0,  1,  8, 104},  // Port#7
    { 7,  1,  1,  9, 105},
    { 7,  2,  1, 10, 106},
    { 7,  3,  1, 11, 107},

    { 8,  0,  1, 12, 204},  // Port#8
    { 8,  1,  1, 13, 205},
    { 8,  2,  1, 14, 206},
    { 8,  3,  1, 15, 207},

    // BCM #5 = BMZ Bcm#2
    { 9,  0,  5, 12, 237},  // Port#9
    { 9,  1,  5, 13, 236},
    { 9,  2,  5, 14, 239},
    { 9,  3,  5, 15, 238},

    {10,  0,  5,  8, 166},  // Port#10
    {10,  1,  5,  9, 167},
    {10,  2,  5, 10, 164},
    {10,  3,  5, 11, 165},

    // BCM #6 - BMZ BCM#3
    {11,  0,  6,  0, 158},  // Port#11
    {11,  1,  6,  1, 159},
    {11,  2,  6,  2, 156},
    {11,  3,  6,  3, 157},

    {12,  0,  6,  4, 162},  // Port#12
    {12,  1,  6,  5, 163},
    {12,  2,  6,  6, 160},
    {12,  3,  6,  7, 161},

    {13,  0,  6,  8, 180},  // Port#13
    {13,  1,  6,  9, 181},
    {13,  2,  6, 10, 182},
    {13,  3,  6, 11, 183},

    {14,  0,  6, 12, 184},  // Port#14
    {14,  1,  6, 13, 185},
    {14,  2,  6, 14, 186},
    {14,  3,  6, 15, 187},

    // BCM #4 - BMZ BCM#1
    {15,  0,  4,  8, 188},  // Port#15
    {15,  1,  4,  9, 189},
    {15,  2,  4, 10, 190},
    {15,  3,  4, 11, 191},

    {16,  0,  4, 12, 252},  // Port#16
    {16,  1,  4, 13, 253},
    {16,  2,  4, 14, 254},
    {16,  3,  4, 15, 255},
};
const size_t kLinksFromQsfp28ToBcm = sizeof(Qsfp28_to_BCM_connections) / sizeof(tuning::LinkData);

const unsigned int c100gNumLanes = 4;



// 400G BCM to QSFPDD
const tuning::LinkData BCM_to_qsfpdd_connections_400GE[] =
{
    // BCM #2 - TMZ
    { 2,  4,  1,  4, 209},  // Port#1
    { 2,  5,  1,  5, 208},
    { 2,  6,  1,  6, 211},
    { 2,  7,  1,  7, 210},
    { 2, 12,  1,  0, 213},
    { 2, 13,  1,  1, 212},
    { 2, 14,  1,  2, 215},
    { 2, 15,  1,  3, 214},
    
    // BCM #1 - TMZ
    { 1,  4,  8,  4, 192},  // Port#8
    { 1,  5,  8,  5, 193},
    { 1,  6,  8,  6, 194},
    { 1,  7,  8,  7, 195},
    { 1, 12,  8,  0, 196},
    { 1, 13,  8,  1, 197},
    { 1, 14,  8,  2, 198},
    { 1, 15,  8,  3, 199},

    // BCM #5 = BMZ Bcm#2
    { 5,  4,  9,  4, 225},  // Port#9
    { 5,  5,  9,  5, 224},
    { 5,  6,  9,  6, 227},
    { 5,  7,  9,  7, 226},
    { 5, 12,  9,  0, 229},
    { 5, 13,  9,  1, 228},
    { 5, 14,  9,  2, 231},
    { 5, 15,  9,  3, 230},
    
    // BCM #4 - BMZ BCM#1
    { 4,  4, 16,  4, 240},  // Port#16
    { 4,  5, 16,  5, 241},
    { 4,  6, 16,  6, 242},
    { 4,  7, 16,  7, 243},
    { 4, 12, 16,  0, 244},
    { 4, 13, 16,  1, 245},
    { 4, 14, 16,  2, 246},
    { 4, 15, 16,  3, 247},
};
const size_t kLinksFromBcmToQsfpdd_400GE = sizeof(BCM_to_qsfpdd_connections_400GE) / sizeof(tuning::LinkData);
const unsigned int c400gNumLanes = 8;

// 400G QSFPDD to BCM
const tuning::LinkData Qsfpdd_to_BCM_connections_400GE[] =
{
    // BCM #2 - TMZ
    { 1,  4,  2,  4, 216},  // Port#1
    { 1,  5,  2,  5, 217},
    { 1,  6,  2,  6, 219},
    { 1,  7,  2,  7, 218},
    { 1,  0,  2, 12, 220},
    { 1,  1,  2, 13, 221},
    { 1,  2,  2, 14, 222},
    { 1,  3,  2, 15, 223},

    // BCM #1 - TMZ
    { 8,  4,  1,  4, 200},  // Port#8
    { 8,  5,  1,  5, 201},
    { 8,  6,  1,  6, 202},
    { 8,  7,  1,  7, 203},
    { 8,  0,  1, 12, 204},
    { 8,  1,  1, 13, 205},
    { 8,  2,  1, 14, 206},
    { 8,  3,  1, 15, 207},

    // BCM #5 = BMZ Bcm#2
    { 9,  4,  5,  4, 233},  // Port#9
    { 9,  5,  5,  5, 232},
    { 9,  6,  5,  6, 235},
    { 9,  7,  5,  7, 234},
    { 9,  0,  5, 12, 237},
    { 9,  1,  5, 13, 236},
    { 9,  2,  5, 14, 239},
    { 9,  3,  5, 15, 238},

    // BCM #4 - BMZ BCM#1
    { 16,  4, 4,  4, 248},  // Port#16
    { 16,  5, 4,  5, 249},
    { 16,  6, 4,  6, 250},
    { 16,  7, 4,  7, 251},
    { 16,  0, 4, 12, 252},
    { 16,  1, 4, 13, 253},
    { 16,  2, 4, 14, 254},
    { 16,  3, 4, 15, 255},
};
const size_t kLinksFromQsfpddToBcm_400GE = sizeof(Qsfpdd_to_BCM_connections_400GE) / sizeof(tuning::LinkData);

///////////////////

// 4x100GE

// 400G BCM to QSFPDD
const tuning::LinkData BCM_to_qsfpdd_connections_4_100GE[] =
{
    // BCM #2 - TMZ
    { 2, 14,  1,  2, 215},  // Port#1.1, BCM line=0xC000
    { 2, 15,  1,  3, 214},

    { 2, 12,  2,  0, 213},  // Port#1.2, BCM line=0x3000
    { 2, 13,  2,  1, 212},

    { 2,  4,  3,  4, 209},  // Port#1.3, BCM line=0x30
    { 2,  5,  3,  5, 208},

    { 2,  6,  4,  6, 211},  // Port#1.4, BCM line=0xC0
    { 2,  7,  4,  7, 210},

    // BCM #1 - TMZ
    { 1,  4,  5,  4, 192},  // Port#8.1, BCM line=0x30
    { 1,  5,  5,  5, 193},

    { 1,  6,  6,  6, 194},  // Port#8.2, BCM line=0xC0
    { 1,  7,  6,  7, 195},

    { 1, 12,  7,  0, 196},  // Port#8.3, BCM line=0x3000
    { 1, 13,  7,  1, 197},

    { 1, 14,  8,  2, 198},  // Port#8.4, BCM line=0xC000
    { 1, 15,  8,  3, 199},


    // BCM #5 = BMZ Bcm#2
    { 5, 14, 9,  2, 231},  // Port#9.1, BCM line=0xC000
    { 5, 15, 9,  3, 230},

    { 5, 12, 10,  0, 229},  // Port#9.2, BCM line=0x3000
    { 5, 13, 10,  1, 228},

    { 5,  4, 11,  4, 225},  // Port#9.3, BCM line=0x30
    { 5,  5, 11,  5, 224},

    { 5,  6, 12,  6, 227},  // Port#9.1, BCM line=0xC0
    { 5,  7, 12,  7, 226},


    // BCM #4 - BMZ BCM#1
    { 4,  4, 13,  4, 240},  // Port#16.1, BCM line=0x30
    { 4,  5, 13,  5, 241},

    { 4,  6, 14,  6, 242},  // Port#16.2, BCM line=0xC0
    { 4,  7, 14,  7, 243},

    { 4, 12, 15,  0, 244},  // Port#16.3, BCM line=0x3000
    { 4, 13, 15,  1, 245},

    { 4, 14, 16,  2, 246},  // Port#16.4, BCM line=0xC000
    { 4, 15, 16,  3, 247},
};

const size_t kLinksFromBcmToQsfpdd_4_100GE = sizeof(BCM_to_qsfpdd_connections_4_100GE) / sizeof(tuning::LinkData);
const unsigned int c4_100gNumLanes = 2;


// 400G QSFPDD to BCM
const tuning::LinkData Qsfpdd_to_BCM_connections_4_100GE[] =
{
    // BCM #2 - TMZ
    { 1,  2,  2, 14, 222},  // Port#1.1
    { 1,  3,  2, 15, 223},

    { 2,  0,  2, 12, 220},  // Port#1.2
    { 2,  1,  2, 13, 221},

    { 3,  4,  2,  4, 216},  // Port#1.3
    { 3,  5,  2,  5, 217},

    { 4,  6,  2,  6, 219},  // Port#1.4
    { 4,  7,  2,  7, 218},

    // BCM #1 - TMZ
    { 5,  4,  1,  4, 200},  // Port#8.1
    { 5,  5,  1,  5, 201},

    { 6,  6,  1,  6, 202},  // Port#8.2
    { 6,  7,  1,  7, 203},

    { 7,  0,  1, 12, 204},  // Port#8.3
    { 7,  1,  1, 13, 205},

    { 8,  2,  1, 14, 206},  // Port#8.4
    { 8,  3,  1, 15, 207},

    // BCM #5 = BMZ Bcm#2
    {9,  2,  5, 14, 239},  // Port#9.1
    {9,  3,  5, 15, 238},

    {10,  0,  5, 12, 237},  // Port#9.2
    {10,  1,  5, 13, 236},

    {11,  4,  5,  4, 233},  // Port#9.3
    {11,  5,  5,  5, 232},

    {12,  6,  5,  6, 235},  // Port#9.4
    {12,  7,  5,  7, 234},


    // BCM #4 - BMZ BCM#1
    { 13,  4, 4,  4, 248},  // Port#16.1
    { 13,  5, 4,  5, 249},

    { 14,  6, 4,  6, 250},  // Port#16.2
    { 14,  7, 4,  7, 251},

    { 15,  0, 4, 12, 252},  // Port#16.3
    { 15,  1, 4, 13, 253},

    { 16,  2, 4, 14, 254},  // Port#16.4
    { 16,  3, 4, 15, 255},
};

const size_t kLinksFromQsfpddToBcm_4_100GE = sizeof(Qsfpdd_to_BCM_connections_4_100GE) / sizeof(tuning::LinkData);


////////////////////


// v1.1 Loss ID
const LinkData BcmLine_to_BcmHost_connections[] =
{
    // === Top Mezz ===
    // TOM Port 3 and 4
    // BCM2 --> BCM3
    { 2,  0,  3,  0, 73},
    { 2,  1,  3,  1, 72},
    { 2,  2,  3,  2, 75},
    { 2,  3,  3,  3, 74},

    // =================

    // TOM Port 5 and 6
    // BCM1 --> BCM3
    { 1,  0,  3,  4, 64},
    { 1,  1,  3,  5, 65},
    { 1,  2,  3,  6, 66},
    { 1,  3,  3,  7, 67},

    // === Bottom Mezz ===

    // TOM Port 11 and 12
    // BCM2 --> BCM3
    { 5,  0,  6,  0, 81},
    { 5,  1,  6,  1, 80},
    { 5,  2,  6,  2, 83},
    { 5,  3,  6,  3, 82},

    // =================

    // TOM Port 13 and 14
    // BCM1 --> BCM3
    { 4,  0,  6,  4, 88},
    { 4,  1,  6,  5, 89},
    { 4,  2,  6,  6, 90},
    { 4,  3,  6,  7, 91},
};

const size_t kLinksFromBcmLineToBcmHost = sizeof(BcmLine_to_BcmHost_connections) / sizeof(LinkData);


// v1.1 Loss ID
const LinkData BcmHost_to_BcmLine_connections[] =
{
    // === Top Mezz ===
    // TOM Port 3 and 4
    // BCM3 --> BCM2
    { 3,  0,  2,  0, 76},
    { 3,  1,  2,  1, 77},
    { 3,  2,  2,  2, 79},
    { 3,  3,  2,  3, 78},

    // =================

    // TOM Port 5 and 6
    // BCM3 --> BCM1
    { 3,  4,  1,  0, 68},
    { 3,  5,  1,  1, 69},
    { 3,  6,  1,  2, 70},
    { 3,  7,  1,  3, 71},


    // === Bottom Mezz ===

    // TOM Port 11 and 12
    // BCM6 (BCM3 BMZ) --> BCM5 (BCM2 BMZ)
    { 6,  0,  5,  0, 85},
    { 6,  1,  5,  1, 84},
    { 6,  2,  5,  2, 87},
    { 6,  3,  5,  3, 86},

    // =================

    // TOM Port 13 and 14
    // BCM6 (BCM3 BMZ) --> BCM4 (BCM1 BMZ)
    { 6,  4,  4,  0, 92},
    { 6,  5,  4,  1, 93},
    { 6,  6,  4,  2, 94},
    { 6,  7,  4,  3, 95},

};

const unsigned int c100gNumBcmToBcmLanes = 2;
const size_t kLinksFromBcmHostToBcmLine = sizeof(BcmHost_to_BcmLine_connections) / sizeof(LinkData);

// v1.1 Loss ID
const LinkData Bcm_to_Atl_connections[] =
{
    // To keep tables consistent,
    //  there is only one Atlantic
    //  which we assign devID of 1
    // Prob could use port/TOM ID
    //  for rxDevId, but might have
    //  been confusing.

    // === Top Mezz ===
    // BCM 2 to Atlantic
    { 2,  0,  1,  9, 24}, // Port 3
    { 2,  1,  1,  8, 25}, // Port 3
    { 2,  2,  1, 11, 26}, // Port 4
    { 2,  3,  1, 10, 27}, // Port 4
    { 2,  4,  1, 13, 28}, // Port 2
    { 2,  5,  1, 12, 29}, // Port 2
    { 2,  6,  1, 15, 30}, // Port 1
    { 2,  7,  1, 14, 31}, // Port 1

    // BCM 1 to Atlantic
    { 1,  0,  1,  0,  8}, // Port 5
    { 1,  1,  1,  1,  9}, // Port 5
    { 1,  2,  1,  2, 10}, // Port 6
    { 1,  3,  1,  3, 11}, // Port 6
    { 1,  4,  1,  4, 12}, // Port 7
    { 1,  5,  1,  5, 13}, // Port 7
    { 1,  6,  1,  6, 14}, // Port 8
    { 1,  7,  1,  7, 15}, // Port 8

    // === Bottom Mezz ===
    // BCM 5 (BCM 2) to Atlantic
    { 5,  0,  1, 31, 41}, // Port 11
    { 5,  1,  1, 30, 40}, // Port 11
    { 5,  2,  1, 29, 43}, // Port 12
    { 5,  3,  1, 28, 42}, // Port 12
    { 5,  4,  1, 27, 45}, // Port 10
    { 5,  5,  1, 26, 44}, // Port 10
    { 5,  6,  1, 25, 47}, // Port 9
    { 5,  7,  1, 24, 46}, // Port 9

    // BCM 4 (BCM 1) to Atlantic
    { 4,  0,  1, 23, 56}, // Port 13
    { 4,  1,  1, 22, 57}, // Port 13
    { 4,  2,  1, 21, 58}, // Port 14
    { 4,  3,  1, 20, 59}, // Port 14
    { 4,  4,  1, 19, 60}, // Port 15
    { 4,  5,  1, 18, 61}, // Port 15
    { 4,  6,  1, 17, 62}, // Port 16
    { 4,  7,  1, 16, 63}, // Port 16
};
const size_t kLinksFromBcmHostToAtlHost = sizeof(Bcm_to_Atl_connections) / sizeof(LinkData);

// v1.1 Loss ID
const LinkData Atl_to_Bcm_connections[] =
{
    // To keep tables consistent,
    //  there is only one Atlantic
    //  which we assign devID of 1

    // === Top Mezz ===
    // Atlantic to BCM 2
    { 1,  8,  2,  0,  17}, // Port 3
    { 1,  9,  2,  1,  16}, // Port 3
    { 1, 10,  2,  2,  19}, // Port 4
    { 1, 11,  2,  3,  18}, // Port 4
    { 1, 12,  2,  4,  21}, // Port 2
    { 1, 13,  2,  5,  20}, // Port 2
    { 1, 14,  2,  6,  23}, // Port 1
    { 1, 15,  2,  7,  22}, // Port 1

    // Atlantic to BCM 1
    { 1,  0,  1,  0,   0}, // Port 5
    { 1,  1,  1,  1,   1}, // Port 5
    { 1,  2,  1,  2,   2}, // Port 6
    { 1,  3,  1,  3,   3}, // Port 6
    { 1,  4,  1,  4,   4}, // Port 7
    { 1,  5,  1,  5,   5}, // Port 7
    { 1,  6,  1,  6,   6}, // Port 8
    { 1,  7,  1,  7,   7}, // Port 8

    // === Bottom Mezz ===
    // Atlantic to BCM 5 (BCM 2)
    { 1, 30,  5,  1,  32}, // Port 11
    { 1, 31,  5,  0,  33}, // Port 11
    { 1, 28,  5,  3,  34}, // Port 12
    { 1, 29,  5,  2,  35}, // Port 12
    { 1, 26,  5,  5,  36}, // Port 10
    { 1, 27,  5,  4,  37}, // Port 10
    { 1, 24,  5,  7,  38}, // Port 9
    { 1, 25,  5,  6,  39}, // Port 9

    // Atlantic to BCM 4 (BCM 1)
    { 1, 22,  4,  1,  49}, // Port 13
    { 1, 23,  4,  0,  48}, // Port 13
    { 1, 16,  4,  7,  55}, // Port 16
    { 1, 17,  4,  6,  54}, // Port 16
    { 1, 18,  4,  5,  53}, // Port 15
    { 1, 19,  4,  4,  52}, // Port 15
    { 1, 20,  4,  3,  51}, // Port 14
    { 1, 21,  4,  2,  50}, // Port 14
};

const size_t kLinksFromAtlHostToBcmHost = sizeof(Atl_to_Bcm_connections) / sizeof(LinkData);

}
#endif //SerdesConnMap_h
