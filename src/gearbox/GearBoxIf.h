#ifndef __GEARBOXIF_H__
#define __GEARBOXIF_H__

namespace gearbox {

struct Bcm81725Lane
{
	unsigned int bus;
	unsigned int mdioAddr;
	unsigned int side;
	unsigned int laneNum;
};

struct ModeParam
{
    int          speed;
    int          ifType;
    int          refClk;
    int          interfaceMode;
    unsigned int dataRate;
    unsigned int modulation;
    unsigned int fecType;
};


class GearBoxIf
{
public:
	virtual int init() = 0;
	virtual int loadFirmware(const unsigned int &bus) = 0;
};

}
#endif // __GEARBOXIF_H_
