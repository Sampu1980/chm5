#ifndef GEARBOXUTILS_H_
#define GEARBOXUTILS_H_

#include "epdm.h"
#include <string>
#include <vector>
#include "GearBoxIf.h"
#include "types.h"

#include "GigeClientAdapter.h"
#include "AdapterCommon.h"
#include "dp_common.h"
#include "BcmPm.h"

namespace gearbox
{
 

typedef struct {
    unsigned int laneMap;
    unsigned int dataRate;
    unsigned int modulation;
    unsigned int fecType;
} modeCfg;

typedef struct {
    unsigned int  portNum;
    unsigned int  bus; //1 = tmz, 0 = bmz
    unsigned int  bcmUnit;
    modeCfg       host;
    modeCfg       line;
}PortConfig;




typedef struct
{
    unsigned int fwVer;
    unsigned int fwCrc;
} BcmVers;





typedef enum Bcm81725DataRate {
    RATE_INVALID = -1,
    RATE_100GE = 0,
    RATE_400GE,
    RATE_200GE,
    RATE_OTU4,
	RATE_4_100GE,
    NUM_RATES,
}Bcm81725DataRate_t;

typedef enum Bcm81725Loopback {
    LOOPBACK_NONE = 0,
    LOOPBACK_FAC,
    LOOPBACK_TERM
}Bcm81725Loopback_t;

typedef enum Bcm81725FecEnable
{
    FEC_DISABLED,
    FEC_ENABLED
}Bcm81725FecEnable_t;

typedef enum BcmNum
{
    BCM1_TOP = 1,
    BCM2_TOP,
    BCM3_TOP,
    BCM1_BOT,
    BCM2_BOT,
    BCM3_BOT
} BcmNum_t;

typedef enum Ports
{
    PORT1 = 1, // 1
    PORT2,     // 2
    PORT3,     // 3
    PORT4,     // 4
    PORT5,     // 5
    PORT6,     // 6
    PORT7,     // 7
    PORT8,     // 8
    PORT9,     // 9
    PORT10,    // 10
    PORT11,    // 11
    PORT12,    // 12
    PORT13,    // 13
    PORT14,    // 14
    PORT15,    // 15
    PORT16,    // 16
    NUM_PORTS = 16
}Ports_t;






typedef enum BcmSides
{
    BCM_LINE,
    BCM_HOST,
    BCM_NUM_SIDES
}BcmSides_t;

typedef enum BcmLaneConfigDs
{
    DS_MediaType                     = 1,
    DS_DfeOn                         = 2,
    DS_AnEnabled                     = 3,
    DS_AutoPeakingEnable             = 4,
    DS_OppositeCdrFirst              = 5,
    DS_DcWanderMu                    = 6,
    DS_ExSlicer                      = 7,
    DS_LinkTrainingReStartTimeOut    = 8,
    DS_LinkTrainingCL72_CL93PresetEn = 9,
    DS_LinkTraining802_3CDPresetEn   = 10,
    DS_LinkTrainingTxAmpTuning       = 11,
    DS_LpDfeOn                       = 12,
    DS_AnTimer                       = 13
} BcmLaneConfigDs_t;



extern PortConfig port_config_100GE [];
extern PortConfig port_config_400GE [];
extern PortConfig port_config_4_100GE [];
extern const unsigned int c100GeCount;
extern const unsigned int c400GeCount;
extern const unsigned int c4_100GeCount;
extern const unsigned int cOtu4Count;
extern const unsigned int c100G;
extern const unsigned int c200G;
extern const unsigned int c400G;
extern const unsigned int cOTU4;

extern const unsigned int cHostSide;
extern const unsigned int cLineSide;
extern const unsigned int top_mz;
extern const unsigned int bottom_mz;
extern const unsigned int c4_100_numChildPorts;


void GetPortConfigEntry(std::vector<PortConfig>& portConfig, unsigned int port, unsigned int rate);
bool checkValidPort(unsigned int portNum);
int aid2ParentPortNum(const std::string & aidStr);
unsigned int aid2ChildPortNum(const std::string & aidStr);
unsigned int aid2PortNum(const std::string & aidStr);
std::string printFecType(bcm_plp_fec_mode_sel_t fecType);
std::string printModulation(int modulation);
bool isOuterLineSide(unsigned int port, Bcm81725DataRate_t rate, unsigned int bcmUnit, unsigned int side);
bool isLineSideFecEnable(unsigned int port, Bcm81725DataRate_t rate, unsigned int bcmUnit, unsigned int side, bool fecEnable);
std::string rate2SerdesCalDataString(Bcm81725DataRate_t rate);
int rateEnum2Speed(Bcm81725DataRate_t rate);
Bcm81725DataRate_t speed2RateEnum(int speed, int dataRate, int fecMode);
std::string rateEnum2String(Bcm81725DataRate_t rate);
std::string bus2String(unsigned int bus);
std::string side2String(unsigned int side);
uint16 GetBcmUnit(uint16 bcmNum);
uint16 GetBus(uint16 bcmNum);

void ParseAlarms(const std::vector<bool> & bcmFaults, DpAdapter::GigeFault* faults);
unsigned int GetPhysicalLanes(Bcm81725DataRate_t rate);



} // End gearbox namespace


#endif /* GEARBOXUTILS_H_ */
