/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef CARD_ADAPTER_H
#define CARD_ADAPTER_H

#include "dp_common.h"
#include "AdapterCommon.h"
#include <string>
#include <vector>

using namespace std;
using namespace DataPlane;

namespace DpAdapter {


typedef struct
{
    string cardName;
    ClientMode mode;
    RefClockConfig refClk;
    SerdesSpeed speed;
    uint32_t hostToClientLane[DCO_MAX_SERDES_LANES];
    uint32_t maxPacketLength;  // Set client max packet length
} CardConfig;

enum IskCpu
{
    ISK_CPU_HOST_OR_UNSPECIFIED = 0,
	ISK_CPU_NXP,
	ISK_CPU_DCO

} ;

typedef struct
{
    uint32  shelf_id;
	uint32  slot_id;
	IskCpu  cpu;
	string  key_name;
    string  key_serial_number;
    string  issuer_name;
    uint32  key_length;
    string  key_payload;
    string  krk_name;
    bool    is_key_in_use;
    bool    is_key_verified;
    bool    being_deleted;
    string  signature_hash_scheme;
    string  signature_algorithm;
    string  signature_key_id;
    string  signature_payload;
    string  signature_gen_time;
} ImageSigningKey;

typedef struct
{
    string cardName;
    string description;
    bool enable;
    ClientMode mode;
    double refClkFrq;
    PostStatus post;
    DcoState state;
    string status;
    uint64_t eqptFault;
    uint64_t dacEqptFault;
    uint64_t psEqptFault;
    uint64_t postFault;
    uint64_t dacPostFault;
    uint64_t psPostFault;
    string serialNo;
    string dspSerialNo;
    string moduleSerialNo;
    uint32_t hwRevision;
    uint32_t dspRevision;
    string apiVer;
    string   fwVersion;
    uint32_t fpgaVer;
    uint32_t mcuVer;
    string  bootLoaderVer;
    bool dspSupported;
    string fwUpgradeStatus;
    FwUpgradeState fwUpgradeState;
    BootState bootState;
    bool tempHiFanIncrease;         // True, chassis fan increase
    bool tempStableFanDecrease;     // True, chassis fan decrease
    bool operationState;            // True, state report is valid
    string keyShow;                 // all keys in flash
    uint32_t maxPacketLength;       // Set client max packet length
    string idevidCert;
    map< string, ImageSigningKey > isk;
    ClientMode activeMode;          // DCO active running client mode
} CardStatus;

typedef struct
{
    string     carrierModeDesignation;
    uint32_t   dataRate;
    ClientMode clientMode;
    double     baudRate;
    string     primaryAppCode;
    uint32_t   compatabilityId;
    CarrierModeStatus  carrierModeStatus;
    uint32_t   maxDcoPower;
} LineMode;

typedef struct
{
    string   name;
    string   dataType ;
    string   value ;
    AdvParmDirection dir;
    uint16_t multiplicity;
    AdvParmConfigImpact cfgImpact;
    AdvParmServiceImpact servImpact;
    string  description;
} SupportedAdvancedParameter;

typedef struct
{
    std::vector<PortRate> supportedClientRate;
    std::vector<LineMode> supportedLineMode;
    std::vector<SupportedAdvancedParameter> supportedAp;
} CardCapability;

typedef struct
{
    std::vector<FaultCapability> eqpt;
    std::vector<FaultCapability> dacEqpt;
    std::vector<FaultCapability> psEqpt;
    std::vector<FaultCapability> post;
    std::vector<FaultCapability> dacPost;
    std::vector<FaultCapability> psPost;
} CardFaultCapability;

enum DcoCardReboot
{
    DCO_CARD_REBOOT_NONE    = 0,
    DCO_CARD_REBOOT_WARM    = 1,
    DCO_CARD_REBOOT_COLD    = 2,
};

typedef struct
{
    std::vector<AdapterFault> eqpt;
    std::vector<AdapterFault> dacEqpt;
    std::vector<AdapterFault> psEqpt;
    std::vector<AdapterFault> post;
    std::vector<AdapterFault> dacPost;
    std::vector<AdapterFault> psPost;
    uint64_t eqptBmp;
    uint64_t dacEqptBmp;
    uint64_t psEqptBmp;
    uint64_t postBmp;
    uint64_t dacPostBmp;
    uint64_t psPostBmp;
    DcoCardReboot rebootState;
} CardFault;

// Card Adapter Base class
class CardAdapter
{
public:
    CardAdapter() {}
    virtual ~CardAdapter() {}

    virtual bool createCard() = 0;
    virtual bool deleteCard() = 0;
    virtual bool initializeCard() = 0;

    virtual bool warmBoot() = 0;
    virtual bool coldBoot() = 0;  // Not supported in CHM6
    virtual bool shutdown(bool beforeColdReboot) = 0;
    virtual bool setCardEnabled(bool enable) = 0;
    virtual bool setClientMode(ClientMode mode) = 0;
    virtual bool setClientMaxPacketLength(uint32_t len) = 0;

    virtual bool getCardConfig( CardConfig *cfg) = 0;
    virtual bool getCardStatus( CardStatus *stat, bool force = false) = 0;
    virtual bool getCardCapabilities(CardCapability *capa) = 0;
    virtual bool getCardFault() = 0;
    virtual bool getCardFault(CardFault *faults, bool force = false) = 0;
    virtual bool getCardPm() = 0;
    virtual ClientMode getCurClientMode() = 0;

    virtual bool cardConfigInfo(ostream& out) = 0;
    virtual bool cardStatusInfo(ostream& out, bool force = true) = 0;
    virtual bool cardCapabilityInfo(ostream& out) = 0;
    virtual bool cardFaultInfo(ostream& out) = 0;

    virtual bool addIsk(string& keyPath) = 0;
    virtual bool removeIsk(string& keyName) = 0;
};

}
#endif // CARD_ADAPTER_H
