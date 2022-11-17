#include "GearBoxUtils.h"
#include <iostream>
#include <string>
#include "BcmFaults.h"
#include "InfnLogger.h"
#include "dp_common.h"

using namespace std;
namespace gearbox
{


const unsigned int top_mz    = 1;
const unsigned int bottom_mz = 0;
const unsigned int bcm_1     = 0x04;
const unsigned int bcm_2     = 0x08;
const unsigned int bcm_3     = 0x0C;
const unsigned int cHostSide = 1;
const unsigned int cLineSide = 0;
const unsigned int c4_100_numChildPorts = 4;




PortConfig port_config_100GE [] =
{
    {1, top_mz, bcm_2,
        /* host */ {0xC0,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0xF000, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {2, top_mz, bcm_2,
        /* host */ {0x30,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {3, top_mz, bcm_3,
        /* host */ {0x03,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x000F, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {3, top_mz, bcm_2,
        /* host */ {0x03,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {4, top_mz, bcm_3,
        /* host */ {0x0C,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x00F0, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {4, top_mz, bcm_2,
        /* host */ {0x0C,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {5, top_mz, bcm_3,
        /* host */ {0x30,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {5, top_mz, bcm_1,
        /* host */ {0x03,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {6, top_mz, bcm_3,
        /* host */ {0xC0,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0xF000, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {6, top_mz, bcm_1,
        /* host */ {0x0C,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {7, top_mz, bcm_1,
        /* host */ {0x30,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {8, top_mz, bcm_1,
        /* host */ {0xC0,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0xF000, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    //Port 9-16 :: same as 1-8 on bottom mz
    {9, bottom_mz, bcm_2,
        /* host */ {0xC0,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0xF000, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {10, bottom_mz, bcm_2,
        /* host */ {0x30,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {11, bottom_mz, bcm_3,
        /* host */ {0x03,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x000F, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {11, bottom_mz, bcm_2,
        /* host */ {0x03,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {12, bottom_mz, bcm_3,
        /* host */ {0x0C,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x00F0, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {12, bottom_mz, bcm_2,
        /* host */ {0x0C,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {13, bottom_mz, bcm_3,
        /* host */ {0x30,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {13, bottom_mz, bcm_1,
        /* host */ {0x03,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {14, bottom_mz, bcm_3,
        /* host */ {0xC0,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0xF000, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {14, bottom_mz, bcm_1,
        /* host */ {0x0C,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {15, bottom_mz, bcm_1,
        /* host */ {0x30,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    },
    {16, bottom_mz, bcm_1,
        /* host */ {0xC0,   bcmplpLaneDataRate_53P125G,   bcmplpModulationPAM4, bcmplpKP4FEC},
        /* line */ {0xF000, bcmplpLaneDataRate_25P78125G, bcmplpModulationNRZ,  bcmplpPCSFEC}
    }
};



//TODO: cross check below config with diags
PortConfig port_config_400GE [] =
{
    {1, top_mz, bcm_2,
        /* host */ {0xFF,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF0F0, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
    },
    {8, top_mz, bcm_1,
        /* host */ {0xFF,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF0F0, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
    },
    {9, bottom_mz, bcm_2,
        /* host */ {0xFF,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF0F0, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
    },
    {16, bottom_mz, bcm_1,
        /* host */ {0xFF,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF0F0, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
    },
};

PortConfig port_config_4_100GE [] =
{
		{1, top_mz, bcm_2, /* 1.1 */
				/* host */ {0xC0,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
		},
		{2, top_mz, bcm_2, /* 1.2 */
				/* host */ {0x30,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x3000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{3, top_mz, bcm_2, /* 1.3 */
				/* host */ {0x3,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x30,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{4, top_mz, bcm_2, /* 1.4 */
				/* host */ {0xC,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC0,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{5, top_mz, bcm_1, /* 8.1 */
				/* host */ {0x3,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x30,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{6, top_mz, bcm_1, /* 8.2 */
				/* host */ {0xC,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC0,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{7, top_mz, bcm_1, /* 8.3 */
				/* host */ {0x30,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x3000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{8, top_mz, bcm_1, /* 8.4 */
				/* host */ {0xC0,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
		},
		{9, bottom_mz, bcm_2,  /* 9.1 */
			    /* host */ {0xC0,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
		},
		{10, bottom_mz, bcm_2, /* 9.2 */
				/* host */ {0x30,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x3000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{11, bottom_mz, bcm_2, /* 9.3 */
				/* host */ {0x3,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x30,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{12, bottom_mz, bcm_2, /* 9.4 */
				/* host */ {0xC,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC0,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{13, bottom_mz, bcm_1, /* 16.1 */
				/* host */ {0x3,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x30,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{14, bottom_mz, bcm_1, /* 16.2 */
				/* host */ {0xC,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC0,  bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{15, bottom_mz, bcm_1, /* 16.3 */
				/* host */ {0x30,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0x3000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
		},
		{16, bottom_mz, bcm_1, /* 16.4 */
				/* host */ {0xC0,   bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC},
				/* line */ {0xC000, bcmplpLaneDataRate_53P125G, bcmplpModulationPAM4, bcmplpNoFEC}
		},
};


PortConfig port_config_OTU4 [] =
{
    {1, top_mz, bcm_2,
        /* host */ {0xC0,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF000, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {2, top_mz, bcm_2,
        /* host */ {0x30,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {3, top_mz, bcm_3,
        /* host */ {0x03,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000F, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {3, top_mz, bcm_2,
        /* host */ {0x03,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {4, top_mz, bcm_3,
        /* host */ {0x0C,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x00F0, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {4, top_mz, bcm_2,
        /* host */ {0x0C,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {5, top_mz, bcm_3,
        /* host */ {0x30,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {5, top_mz, bcm_1,
        /* host */ {0x03,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {6, top_mz, bcm_3,
        /* host */ {0xC0,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF000, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {6, top_mz, bcm_1,
        /* host */ {0x0C,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {7, top_mz, bcm_1,
        /* host */ {0x30,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {8, top_mz, bcm_1,
        /* host */ {0xC0,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF000, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    //Port 9-16 :: same as 1-8 on bottom mz
    {9, bottom_mz, bcm_2,
        /* host */ {0xC0,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF000, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {10, bottom_mz, bcm_2,
        /* host */ {0x30,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {11, bottom_mz, bcm_3,
        /* host */ {0x03,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000F, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {11, bottom_mz, bcm_2,
        /* host */ {0x03,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {12, bottom_mz, bcm_3,
        /* host */ {0x0C,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x00F0, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {12, bottom_mz, bcm_2,
        /* host */ {0x0C,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {13, bottom_mz, bcm_3,
        /* host */ {0x30,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {13, bottom_mz, bcm_1,
        /* host */ {0x03,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0003, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {14, bottom_mz, bcm_3,
        /* host */ {0xC0,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF000, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {14, bottom_mz, bcm_1,
        /* host */ {0x0C,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x000C, bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
    },
    {15, bottom_mz, bcm_1,
        /* host */ {0x30,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0x0F00, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    },
    {16, bottom_mz, bcm_1,
        /* host */ {0xC0,   bcmplpLaneDataRate_55P9049G, bcmplpModulationPAM4, bcmplpNoFEC},
        /* line */ {0xF000, bcmplpLaneDataRate_27P9525G, bcmplpModulationNRZ,  bcmplpNoFEC}
    }
};



const unsigned int c100GeCount     = sizeof(port_config_100GE) / sizeof(PortConfig);
const unsigned int c400GeCount     = sizeof(port_config_400GE) / sizeof(PortConfig);
const unsigned int c4_100GeCount     = sizeof(port_config_4_100GE) / sizeof(PortConfig);
const unsigned int cOtu4Count      = sizeof(port_config_OTU4)  / sizeof(PortConfig);

const unsigned int c100G   = 100000;
const unsigned int c200G   = 200000;
const unsigned int c400G   = 400000;
const unsigned int cOTU4   = 100000;
const unsigned int c4_100G = 100000;




void GetPortConfigEntry(std::vector<PortConfig>& portConfig, unsigned int port, unsigned int rate)
{
	unsigned int count = 0;
	PortConfig * config = NULL;


	if (rate == RATE_100GE)
	{
		config = port_config_100GE;
		count = c100GeCount;
	}
	else if ( (rate == RATE_400GE)
			&&
			((port != PORT1) &&
					(port != PORT8) &&
					(port != PORT9) &&
					(port != PORT16)) )
	{
		return;
	}
	else if (rate == RATE_400GE)
	{
		config = port_config_400GE;
		count = c400GeCount;
	}
    else if (rate == RATE_OTU4)
    {
		config = port_config_OTU4;
		count = cOtu4Count;
    }
    else if (rate == RATE_4_100GE )
    {
		config = port_config_4_100GE;
		count = c4_100GeCount;
    }

	for (unsigned int i = 0; i < count; i++)
	{
		if (config[i].portNum == port)
		{
			portConfig.push_back(config[i]);
		}
	}

}

bool checkValidPort(unsigned int portNum)
{
    bool result = false;
    if (portNum >= PORT1 && portNum <= PORT16)
    {
        return true;
    }
    return result;
}

int aid2ParentPortNum(const std::string & aidStr)
{
    // Example: aidStr = 1-4-T1 .. 1-4-T16

    if (aidStr.empty())
    {
        return -1;
    }

    std::string sId;
    std::string aidPort = "-T";
    std::size_t pos = aidStr.find(aidPort);
    if (pos != std::string::npos)
    {
        sId = aidStr.substr(pos + aidPort.length());
        std::size_t temp = sId.find(".");
        if( temp != std::string::npos)
        {
            return stoi(sId.substr(0,temp), nullptr);
        }
    }
    else
    {
        return -1;
    }

    return (stoi(sId,nullptr));
}

unsigned int aid2ChildPortNum(const std::string & aidStr)
{
    // Example: aidStr = 1-4-T1 .. 1-4-T16

    if (aidStr.empty())
    {
        return -1;
    }

    std::string sId;
    std::string aidPort = "-T";
    std::size_t pos = aidStr.find(aidPort);
    if (pos != std::string::npos)
    {
        sId = aidStr.substr(pos + aidPort.length());
        std::size_t temp = sId.find(".");
        if( temp != std::string::npos)
        {
            int parent = stoi(sId.substr(0,temp), nullptr);
            return stoi(sId.substr(temp+1));

        }
    }
    else
    {
        return -1;
    }

    return (stoi(sId,nullptr));
}


unsigned int aid2PortNum(const std::string & aidStr)
{
    // Example: aidStr = 1-4-T1 .. 1-4-T16

    if (aidStr.empty())
    {
        return -1;
    }

    std::string sId;
    std::string aidPort = "-T";
    std::size_t pos = aidStr.find(aidPort);
    if (pos != std::string::npos)
    {
        sId = aidStr.substr(pos + aidPort.length());
        std::size_t temp = sId.find(".");
        if( temp != std::string::npos)
        {
            int parent = stoi(sId.substr(0,temp), nullptr);
            int child = stoi(sId.substr(temp+1));


            if (parent == PORT1)
            {

                return DataPlane::portLow0[child-1];

            }

            else if (parent == PORT8)
            {

                return DataPlane::portHigh0[child-1];

            }
            else if (parent == PORT9)
            {

                return DataPlane::portLow1[child-1];

            }
            else if (parent == PORT16)
            {

                return DataPlane::portHigh1[child-1];
            }
        }
    }
    else
    {
        return -1;
    }

    return (stoi(sId,nullptr));
}

std::string printModulation(int modulation)
{
    std::string mod;
    switch(modulation)
    {
    case bcmplpModulationNONE:
        mod = "None";
        break;
    case bcmplpModulationNRZ:
        mod = "NRZ";
        break;
    case bcmplpModulationPAM4:
        mod = "PAM4";
        break;
    default:
        mod = "Unknown modulation";
        break;
    }
    return mod;
}

std::string printFecType(bcm_plp_fec_mode_sel_t fecType)
{
	std::string fec;

	switch(fecType)
	{
	case bcmplpNoFEC:
		fec = "No FEC";
		break;
	case bcmplpSFEC:
		fec = "SFEC";
		break;
	case bcmplpKP4FEC: /* RS544 KP4 FEC */
	    fec = "KP4 FEC";
		break;
	case bcmplpJFEC:   /* Prop FEC */
		fec = "JFEC";
		break;
	case bcmplpPCSFEC: /* 100G, 50G, 25G, 10gG 40G PCS, depending upon mode different IEEE clause is applicable */
		fec = "PCS FEC";
		break;
	case bcmplpFCFEC:  /* FC_FEC CL74 */
		fec = "FC_FEC CL74";
		break;
	case bcmplpKR4FEC: /* RS528 KR4 FEC */
		fec = "KR4 FEC";
		break;
	default:
		fec = "Unknown FEC type";
		break;
	}
	return fec;
}

bool isOuterLineSide(unsigned int port, Bcm81725DataRate_t rate, unsigned int bcmUnit, unsigned int side)
{
    bool result = false;

    if (rate == RATE_100GE && side == BcmSides_t::BCM_LINE)
    {
        if (bcmUnit == bcm_3)
        {
            result = true;
        }
        else if (port == PORT1  ||
                 port == PORT2  ||
                 port == PORT7  ||
                 port == PORT8  ||
                 port == PORT9  ||
                 port == PORT10 ||
                 port == PORT15 ||
                 port == PORT16)
        {
            result = true;
        }
    }
    else if (rate == RATE_400GE && side == BcmSides_t::BCM_LINE)
    {
        result = true;
    }
    else if (rate == RATE_OTU4 && side == BcmSides_t::BCM_LINE)
    {
        if (bcmUnit == bcm_3)
        {
            result = true;
        }
        else if (port == PORT1  ||
                 port == PORT2  ||
                 port == PORT7  ||
                 port == PORT8  ||
                 port == PORT9  ||
                 port == PORT10 ||
                 port == PORT15 ||
                 port == PORT16)
        {
            result = true;
        }
    }
    else if (rate == RATE_4_100GE && side == BcmSides_t::BCM_LINE)
    {
        result = true;
    }
    return result;
}

bool isLineSideFecEnable(unsigned int port, Bcm81725DataRate_t rate, unsigned int bcmUnit, unsigned int side, bool fecEnable)
{
    bool result = false;

    if (fecEnable == true && rate == RATE_100GE && side == BcmSides_t::BCM_LINE)
    {
        if (bcmUnit == bcm_3)
        {
            result = true;
        }
        else if (port == PORT1  ||
                 port == PORT2  ||
                 port == PORT7  ||
                 port == PORT8  ||
                 port == PORT9  ||
                 port == PORT10 ||
                 port == PORT15 ||
                 port == PORT16)
        {
            result = true;
        }
    }
    return result;
}


std::string rate2SerdesCalDataString(Bcm81725DataRate_t rate)
{
    // Example G = 50G, 4 lanes 50x4 = 200G
    // C = 25G
    std::string strRate;

    switch(rate)
    {
    case RATE_100GE:
        strRate = "CAUI4";
        break;
    case RATE_200GE:
        strRate = "GAUI4";
        break;
    case RATE_400GE:
    case RATE_4_100GE:
        strRate = "GAUI8";
        break;
    case RATE_OTU4:
        strRate = "OTL4";
        break;

    default:
        break;
    }

   return strRate;
}

int rateEnum2Speed(Bcm81725DataRate_t rate)
{
    int speed = 0;

    switch(rate)
    {
    case RATE_100GE:
        speed = c100G;
        break;
    case RATE_400GE:
        speed = c400G;
        break;
    case RATE_200GE:
        speed = c200G;
        break;
    case RATE_OTU4:
        speed = cOTU4;
        break;
    case RATE_4_100GE:
    	speed = c4_100G;
    	break;
    default:
        break;
    }
    return speed;
}

Bcm81725DataRate_t speed2RateEnum(int speed, int dataRate, int fecMode)
{
    Bcm81725DataRate_t rate = RATE_100GE;

    switch(speed)
    {
    case c100G:
        if ( (fecMode == bcmplpPCSFEC ||
              fecMode == bcmplpKR4FEC)
                &&
             (dataRate == bcmplpLaneDataRate_25P78125G ||
              dataRate == bcmplpLaneDataRate_53P125G  ) )
        {
            rate = RATE_100GE;
        }
        else if ( (fecMode == bcmplpNoFEC)
                   &&
                  (dataRate == bcmplpLaneDataRate_53P125G) )
        {
        	rate = RATE_4_100GE;
        }
        else
        {
            rate = RATE_OTU4;
        }
        break;
    case c400G:
        rate = RATE_400GE;
        break;
    case c200G:
        rate = RATE_200GE;
        break;

    default:
        INFN_LOG(SeverityLevel::error) << "Invalid speed=" << speed << " using default 100G!!"<< endl;
        break;
    }
    return rate;
}

std::string rateEnum2String(Bcm81725DataRate_t rate)
{
    std::string speedStr;

    switch(rate)
    {
    case RATE_100GE:
        speedStr = "100GE";
        break;
    case RATE_400GE:
        speedStr = "400GE";
        break;
    case RATE_200GE:
        speedStr = "200GE";
        break;
    case RATE_OTU4:
        speedStr = "OTU4";
        break;
    case RATE_4_100GE:
    	speedStr = "4x100GE";
    	break;
    default:
        speedStr = "Unknown rate=" + std::to_string(rate);
        break;
    }
    return speedStr;
}

std::string bus2String(unsigned int bus)
{
    std::string busStr;

    switch(bus)
    {
    case top_mz:
        busStr = "Top Mz";
        break;
    case bottom_mz:
        busStr = "Bottom Mz";
        break;
    default:
        busStr = "Unknown Mz: " + std::to_string(bus);
        break;
    }
    return busStr;
}

std::string side2String(unsigned int side)
{
    std::string sideStr;

    switch(side)
    {
    case cLineSide:
        sideStr ="Line Side";
        break;
    case cHostSide:
        sideStr = "Host Side";
        break;
    default:
        sideStr = "Unknown BCM side: " + std::to_string(side);
        break;
    }
    return sideStr;
}

uint16 GetBcmUnit(uint16 bcmNum)
{
    uint16 bcmUnit = 0;

    switch(bcmNum)
    {
    case 1:
        bcmUnit = bcm_1;
        break;
    case 2:
        bcmUnit = bcm_2;
        break;
    case 3:
        bcmUnit = bcm_3;
        break;
    case 4:
        bcmUnit = bcm_1;
        break;
    case 5:
        bcmUnit = bcm_2;
        break;
    case 6:
        bcmUnit = bcm_3;
        break;
    default:
        INFN_LOG(SeverityLevel::error) << "Invalid bcmNum=" << bcmNum << endl;
        break;
    }
    return bcmUnit;
}

uint16 GetBus(uint16 bcmNum)
{
    uint16 bus = top_mz;

    switch(bcmNum)
    {
    case 1:
        bus = top_mz;
        break;
    case 2:
        bus = top_mz;
        break;
    case 3:
        bus = top_mz;
        break;
    case 4:
        bus = bottom_mz;
        break;
    case 5:
        bus = bottom_mz;
        break;
    case 6:
        bus = bottom_mz;
        break;
    default:
        INFN_LOG(SeverityLevel::error) << "Invalid bcmNum=" << bcmNum << endl;
        break;
    }
    return bus;
}


void ParseAlarms(const std::vector<bool> & bcmFaults, DpAdapter::GigeFault* faults)
{
    DpAdapter::AdapterFault adapterFault;

    faults->facBmp = 0;

    for (unsigned int i = 0; i < cNumBcmFaults; i++)
    {
        // Check index is matching with BcmFaultIdx enums
        if (i == bcmAdapterFaults[i].faultIdx)
        {
            adapterFault.direction = bcmAdapterFaults[i].adapterFaults.direction;
            adapterFault.faultName = bcmAdapterFaults[i].adapterFaults.faultName;
            adapterFault.faulted   = bcmAdapterFaults[i].adapterFaults.faulted;
            adapterFault.location  = bcmAdapterFaults[i].adapterFaults.location;

            if (bcmFaults[i] == true)
            {
                adapterFault.faulted = true;
                faults->facBmp |= (1 << i);
            }

            faults->fac.push_back(adapterFault);
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "SW BUG!! Bcm fault indexes are not matching in enum and table! i=" << i << " faultIdx=" << bcmAdapterFaults[i].faultIdx;
        }
    }
}

unsigned int GetPhysicalLanes(Bcm81725DataRate_t rate)
{
    unsigned int numLanes = 0;

    switch (rate)
    {
    case RATE_100GE:
        numLanes = c100gNumPhysicalLanes;
        break;
    case RATE_200GE:
        numLanes = c200gNumPhysicalLanes;
        break;
    case RATE_400GE:
    case RATE_4_100GE:
        numLanes = c400gNumPhysicalLanes;
        break;
    case RATE_OTU4:
        numLanes = cOtu4NumPhysicalLanes;
        break;
    default:
        INFN_LOG(SeverityLevel::error) << "Invalid rate=" << (int)rate << endl;
        break;
    }

    return numLanes;
}



}


