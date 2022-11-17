#ifndef CHM6_DATAPLANE_MS_SRC_SIM_SIMDCOCARDFAULTS_H_
#define CHM6_DATAPLANE_MS_SRC_SIM_SIMDCOCARDFAULTS_H_

#include "dcoyang/infinera_dco_card_fault/infinera_dco_card_fault.pb.h"

namespace DpSim
{
using DcoCardFault = ::dcoyang::infinera_dco_card_fault::DcoCardFault;

using DcoCardFault_Capabilities_EquipmentFaultsKey    = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaultsKey;
using DcoCardFault_Capabilities_EquipmentFaults       = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults;

using DcoCardFault_Capabilities_DacEquipmentFaultsKey = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacEquipmentFaultsKey;
using DcoCardFault_Capabilities_DacEquipmentFaults    = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacEquipmentFaults;

using DcoCardFault_Capabilities_PsEquipmentFaultsKey  = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsEquipmentFaultsKey;
using DcoCardFault_Capabilities_PsEquipmentFaults     = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsEquipmentFaults;

using DcoCardFault_Capabilities_PostFaultsKey         = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaultsKey;
using DcoCardFault_Capabilities_PostFaults            = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults;

using DcoCardFault_Capabilities_DacPostFaultsKey      = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacPostFaultsKey;
using DcoCardFault_Capabilities_DacPostFaults         = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacPostFaults;

using DcoCardFault_Capabilities_PsPostFaultsKey       = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsPostFaultsKey;
using DcoCardFault_Capabilities_PsPostFaults          = ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsPostFaults;

typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults_Direction eqptDir;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults_Location eqptLoc;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_EquipmentFaults_Severity eqptSeverity;

struct dcoEqptFaultCapa
{
    dcoEqptFaultCapa(std::string k, std::string n, int sa, int b, int d, int l, int s)
    : key(k)
    , name(n)
    , serviceAffecting(static_cast<bool>(sa))
    , bitPos(b)
    , direction(static_cast<eqptDir>(d))
    , location(static_cast<eqptLoc>(l))
    , severity(static_cast<eqptSeverity>(s))
    {}

    std::string  key;
    std::string  name;
    bool         serviceAffecting;
    int          bitPos;
    eqptDir      direction;
    eqptLoc      location;
    eqptSeverity severity;
};

static std::map<std::string, dcoEqptFaultCapa> eqptFaultCapa =
    {
        {"TPIC-LOW",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_TPIC_LOW",              "TPIC-LOW",                                 1,     0,      3,      3,      2)
        },
        {"TPIC-HIGH",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_TPIC_HIGH",             "TPIC-HIGH",                                1,     1,      3,      3,      2)
        },
        {"TDCO-LOW",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_TDCO_LOW",              "TDCO-LOW",                                 1,     2,      3,      3,      2)
        },
        {"TDCO-HIGH",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_TDCO_HIGH",             "TDCO-HIGH",                                1,     3,      3,      3,      2)
        },
        {"HEATER-FAILED-TO-SETTLE",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_HEATER_FAILED_TO_SETTLE","HEATER-FAILED-TO-SETTLE",                  1,    4,      3,      3,      2)
        },
        {"MZM-FAILED-TO-SETTLE",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_MZM_FAILED_TO_SETTLE",  "MZM-FAILED-TO-SETTLE",                     1,     5,      3,      3,      2)
        },
        {"MC-TONE-POWER-OOR-FAULT",
        dcoEqptFaultCapa("DCO_TONE_PWR_OOR_FAULT",                   "MC-TONE-POWER-OOR-FAULT",                  1,     6,      3,      3,      2)
        },
        {"MC-I2C-BUS-0-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_0_FAULT",                       "MC-I2C-BUS-0-FAULT",                       0,     7,      3,      3,      2)
        },
        {"MC-I2C-BUS-1-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_1_FAULT",                       "MC-I2C-BUS-1-FAULT",                       0,     8,      3,      3,      2)
        },
        {"MC-I2C-BUS-2-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_2_FAULT",                       "MC-I2C-BUS-2-FAULT",                       0,     9,      3,      3,      2)
        },
        {"MC-I2C-BUS-3-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_3_FAULT",                       "MC-I2C-BUS-3-FAULT",                       0,     10,     3,      3,      2)
        },
        {"MC-I2C-BUS-4-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_4_FAULT",                       "MC-I2C-BUS-4-FAULT",                       0,     11,     3,      3,      2)
        },
        {"MC-I2C-BUS-5-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_5_FAULT",                       "MC-I2C-BUS-5-FAULT",                       0,     12,     3,      3,      2)
        },
        {"MC-I2C-BUS-6-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_6_FAULT",                       "MC-I2C-BUS-6-FAULT",                       0,     13,     3,      3,      2)
        },
        {"MC-I2C-BUS-7-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_7_FAULT",                       "MC-I2C-BUS-7-FAULT",                       0,     14,     3,      3,      2)
        },
        {"MC-I2C-BUS-8-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_8_FAULT",                       "MC-I2C-BUS-8-FAULT",                       0,     15,     3,      3,      2)
        },
        {"MC-I2C-BUS-9-FAULT",
        dcoEqptFaultCapa("MC_I2C_BUS_9_FAULT",                       "MC-I2C-BUS-9-FAULT",                       0,     16,     3,      3,      2)
        },
        {"MC-TEC-FAULT-NON-SPECIFIC",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_MCU_TEC",               "MC-TEC-FAULT-NON-SPECIFIC",                1,     17,     3,      3,      2)
        },
        {"MC-FAULT-FTPIC-LOW",
        dcoEqptFaultCapa("DCO_TEC_CONTROL_LOOP_SHUTDOWN",            "MC-FAULT-FTPIC-LOW",                       1,     19,     3,      3,      2)
        },
        {"MC-MCU-TEC-I2C-FAULT",
        dcoEqptFaultCapa("DCO_TEC_ACCESS_FAULT",                     "MC-MCU-TEC-I2C-FAULT",                     0,     20,     3,      3,      2)
        },
        {"MC-MCU-TEC-CONTROL-WARNING",
        dcoEqptFaultCapa("DCO_TEC_CONTROL_LOOP_WARNING",             "MC-MCU-TEC-CONTROL-WARNING",               1,     21,     3,      3,      2)
        },
        {"MC-TEC-TEMP-READ-FAILURE",
        dcoEqptFaultCapa("DCO_TEC_TEMP_READ_FAILURE",                "MC-TEC-TEMP-READ-FAILURE",                 1,     22,     3,      3,      2)
        },
        {"MC-SAC-BUS-RX-FAULT",
        dcoEqptFaultCapa("SAC_BUS_RX_FAULT",                         "MC-SAC-BUS-RX-FAULT",                      0,     23,     3,      3,      1)
        },
        {"VDCO-LOW",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_VDCO_LOW",              "VDCO-LOW",                                 1,     24,     3,      3,      2)
        },
        {"VDCO-HIGH",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_VDCO_HIGH",             "VDCO-HIGH",                                1,     25,     3,      3,      2)
        },
        {"IDCO-LOW",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_IDCO_LOW",              "IDCO-LOW",                                 1,     26,     3,      3,      2)
        },
        {"IDCO-HIGH",
        dcoEqptFaultCapa("DCO_FACILITY_FAULT_IDCO_HIGH",             "IDCO-HIGH",                                1,     27,     3,      3,      2)
        },
        {"MC-LINE-ADC-PLL-LOL-FAULT",
        dcoEqptFaultCapa("MC_LINE_ADC_PLL_LOL_FAULT",                "MC-LINE-ADC-PLL-LOL-FAULT",                1,     28,     3,      3,      2)
        },
        {"MC-LINE-DAC-PLL-LOL-FAULT",
        dcoEqptFaultCapa("MC_LINE_DAC_PLL_LOL_FAULT",                "MC-LINE-DAC-PLL-LOL-FAULT",                1,     29,     3,      3,      2)
        },
        {"MC-TPLANE-PLL-LOL-FAULT",
        dcoEqptFaultCapa("MC_TPLANE_PLL_LOL_FAULT",                  "MC-TPLANE-PLL-LOL-FAULT",                  1,     30,     3,      3,      2)
        },
        {"MC-DAC-TONE-PLL-LOL-FAULT",
        dcoEqptFaultCapa("MC_DAC_TONE_PLL_LOL_FAULT",                "MC-DAC-TONE-PLL-LOL-FAULT",                1,     31,     3,      3,      2)
        },
        {"DCO-SECUREBOOT-SIGNATURE-FAULT",
        dcoEqptFaultCapa("DCO-SECUREBOOT-SIGNATURE-FAULT",           "DCO-SECUREBOOT-SIGNATURE-FAULT",           1,     32,     3,      3,      2)
        },
        {"MC-CLEANUP-PLL-GEN-FAULT",
        dcoEqptFaultCapa("MC_CLEANUP_PLL_GEN_FAULT",                 "MC-CLEANUP-PLL-GEN-FAULT",                 1,     33,     3,      3,      2)
        },
        {"DC-ICELAND-HEARTBEAT-FAILURE",
        dcoEqptFaultCapa("EQPT_ICELAND_HEARTBEAT_FAILURE",           "DC-ICELAND-HEARTBEAT-FAILURE",             1,     34,     3,      3,      2)
        },
        {"DC-DSP-TEMP-OORH-WARN",
        dcoEqptFaultCapa("EQPT_DSP_TEMP_OORH_WARN",                  "DC-DSP-TEMP-OORH-WARN",                    0,     35,     3,      3,      2)
        },
        {"DC-DSP-TEMP-OORL-WARN",
        dcoEqptFaultCapa("EQPT_DSP_TEMP_OORL_WARN",                  "DC-DSP-TEMP-OORL-WARN",                    0,     36,     3,      3,      2)
        },
        {"DC-DSP-TEMP-OORH-SHUTDOWN",
        dcoEqptFaultCapa("EQPT_DSP_TEMP_OORH_SHUTDOWN",              "DC-DSP-TEMP-OORH-SHUTDOWN",                1,     37,     3,      3,      2)
        },
        {"DC-DSP-PCIE-ACCESS-FAULT",
        dcoEqptFaultCapa("DSP_PCIE_ACCESS_FAILURE",                  "DC-DSP-PCIE-ACCESS-FAULT",                 1,     38,     3,      3,      2)
        },
        {"DC-ICELAND-LOAD-FAULT",
        dcoEqptFaultCapa("DC_ICELAND_LOAD_FAULT",                    "DC-ICELAND-LOAD-FAULT",                    1,     39,     3,      3,      2)
        },
        {"FRAME-SYNC-FAILURE",
        dcoEqptFaultCapa("EQPT_FRAME_SYNC_FAILURE",                  "FRAME-SYNC-FAILURE",                       1,     40,     3,      3,      2)
        },
        {"MC-UBLAZE-ACCESS-FAILURE",
        dcoEqptFaultCapa("MC_UBLAZE_ACCESS_FAILURE",                 "MC-UBLAZE-ACCESS-FAILURE",                 1,     41,     3,      3,      2)
        },
    };

typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacEquipmentFaults_Direction dacEqptDir;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacEquipmentFaults_Location dacEqptLoc;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacEquipmentFaults_Severity dacEqptSeverity;

struct dcoDacEqptFaultCapa
{
    dcoDacEqptFaultCapa(std::string k, std::string n, int sa, int b, int d, int l, int s)
    : key(k)
    , name(n)
    , serviceAffecting(static_cast<bool>(sa))
    , bitPos(b)
    , direction(static_cast<dacEqptDir>(d))
    , location(static_cast<dacEqptLoc>(l))
    , severity(static_cast<dacEqptSeverity>(s))
    {}

    std::string     key;
    std::string     name;
    bool            serviceAffecting;
    int             bitPos;
    dacEqptDir      direction;
    dacEqptLoc      location;
    dacEqptSeverity severity;
};

static std::map<std::string, dcoDacEqptFaultCapa> dacEqptFaultCapa =
    {
        {"MC-TX-SOA-IDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("TX_SOA_IDAC_TEMP_OORH_WARN",               "MC-TX-SOA-IDAC-TEMP-OORH-WARN",            0,  0,      3,      3,      1)
        },
        {"MC-RX-SHSOA-IDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("RX_SHSOA_IDAC_TEMP_OORH_WARN",             "MC-RX-SHSOA-IDAC-TEMP-OORH-WARN",          0,  1,      3,      3,      1)
        },
        {"MC-WTL-GAIN-IDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("WTL_GAIN_IDAC_TEMP_OORH_WARN",             "MC-WTL-GAIN-IDAC-TEMP-OORH-WARN",          0,  2,      3,      3,      1)
        },
        {"MC-MZM-VOA-VDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("MZM_VOA_VDAC_TEMP_OORH_WARN",              "MC-MZM-VOA-VDAC-TEMP-OORH-WARN",           0,  3,      3,      3,      1)
        },
        {"MC-MZM-PA-VDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("MZM_PA_VDAC_TEMP_OORH_WARN",               "MC-MZM-PA-VDAC-TEMP-OORH-WARN",            0,  4,      3,      3,      1)
        },
        {"MC-MZM-VCAT-VDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("MZM_VCAT_VDAC_TEMP_OORH_WARN",             "MC-MZM-VCAT-VDAC-TEMP-OORH-WARN",          0,  5,      3,      3,      1)
        },
        {"MC-MZM-PHTR-VDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("MZM_PHTR_VDAC_TEMP_OORH_WARN",             "MC-MZM-PHTR-VDAC-TEMP-OORH-WARN",          0,  6,      3,      3,      1)
        },
        {"MC-MR-HTR-VDAC-TEMP-OORH-WARN",
        dcoDacEqptFaultCapa("MR_HTR_VDAC_TEMP_OORH_WARN",               "MC-MR-HTR-VDAC-TEMP-OORH-WARN",            0,  7,      3,      3,      1)
        },
    };

typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsEquipmentFaults_Direction psEqptDir;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsEquipmentFaults_Location PsEqptLoc;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsEquipmentFaults_Severity psEqptSeverity;

struct dcoPsEqptFaultCapa
{
    dcoPsEqptFaultCapa(std::string k, std::string n, int sa, int b, int d, int l, int s)
    : key(k)
    , name(n)
    , serviceAffecting(static_cast<bool>(sa))
    , bitPos(b)
    , direction(static_cast<psEqptDir>(d))
    , location(static_cast<PsEqptLoc>(l))
    , severity(static_cast<psEqptSeverity>(s))
    {}

    std::string    key;
    std::string    name;
    bool           serviceAffecting;
    int            bitPos;
    psEqptDir      direction;
    PsEqptLoc      location;
    psEqptSeverity severity;
};

static std::map<std::string, dcoPsEqptFaultCapa> psEqptFaultCapa =
    {
        {"MC-ATL-TVDD-0-75V-FAULT-HIGH",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_ATL_TVDD_0P75V_HIGH",   "MC-ATL-TVDD-0-75V-FAULT-HIGH",             1,   0,      3,      3,      2)
        },
        {"MC-ATL-TVDD-0-75V-FAULT-LOW",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_ATL_TVDD_0P75V_LOW",    "MC-ATL-TVDD-0-75V-FAULT-LOW",              1,   1,      3,      3,      2)
        },
        {"MC-ATL-RVDD-0-75V-FAULT-HIGH",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_ATL_RVDD_0P75V_HIGH",   "MC-ATL-RVDD-0-75V-FAULT-HIGH",             1,   2,      3,      3,      2)
        },
        {"MC-ATL-RVDD-0-75V-FAULT-LOW",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_ATL_RVDD_0P75V_LOW",    "MC-ATL-RVDD-0-75V-FAULT-LOW",              1,   3,      3,      3,      2)
        },
        {"TX-VCC-LOW",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_TX_VCC_LOW",            "TX-VCC-LOW",                               1,   4,      3,      3,      2)
        },
        {"TX-VCC-HIGH",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_TX_VCC_HIGH",           "TX-VCC-HIGH",                              1,   5,      3,      3,      2)
        },
        {"TX-VTT-LOW",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_TX_VTT_LOW",            "TX-VTT-LOW",                               1,   6,      3,      3,      2)
        },
        {"TX-VTT-HIGH",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_TX_VTT_HIGH",           "TX-VTT-HIGH",                              1,   7,      3,      3,      2)
        },
        {"RX-VCC-LOW",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_RX_VCC_LOW",            "RX-VCC-LOW",                               1,   8,      3,      3,      2)
        },
        {"RX-VCC-HIGH",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_RX_VCC_HIGH",           "RX-VCC-HIGH",                              1,   9,      3,      3,      2)
        },
        {"RX-VTT-LOW",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_RX_VTT_LOW",            "RX-VTT-LOW",                               1,   10,     3,      3,      2)
        },
        {"RX-VTT-HIGH",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_RX_VTT_HIGH",           "RX-VTT-HIGH",                              1,   11,     3,      3,      2)
        },
        {"MC-ATL-WAVE0-0-9V-LDO-FAULT-LOW",
        dcoPsEqptFaultCapa("DCO_ASIC_WAVE0_0_9V_PWR_OOR_LOW",          "MC-ATL-WAVE0-0-9V-LDO-FAULT-LOW",          1,   12,     3,      3,      2)
        },
        {"MC-ATL-WAVE0-0-9V-LDO-FAULT-HIGH",
        dcoPsEqptFaultCapa("DCO_ASIC_WAVE0_0_9V_PWR_OOR_HIGH",         "MC-ATL-WAVE0-0-9V-LDO-FAULT-HIGH",         1,   13,     3,      3,      2)
        },
        {"MC-ATL-WAVE1-0-9V-LDO-FAULT-LOW",
        dcoPsEqptFaultCapa("DCO_ASIC_WAVE1_0_9V_PWR_OOR_LOW",          "MC-ATL-WAVE1-0-9V-LDO-FAULT-LOW",          1,   14,     3,      3,      2)
        },
        {"MC-ATL-WAVE1-0-9V-LDO-FAULT-HIGH",
        dcoPsEqptFaultCapa("DCO_ASIC_WAVE1_0_9V_PWR_OOR_HIGH",         "MC-ATL-WAVE1-0-9V-LDO-FAULT-HIGH",         1,   15,     3,      3,      2)
        },
        {"ATOM-PS-3-3V-FAULT",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_ATOM_PS_3_3V",          "ATOM-PS-3-3V-FAULT",                       1,   16,     3,      3,      2)
        },
        {"ATOM-PS-5V-FAULT",
        dcoPsEqptFaultCapa("DCO_FACILITY_FAULT_ATOM_PS_5V",            "ATOM-PS-5V-FAULT",                         1,   17,     3,      3,      2)
        },
    };

typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults_Direction postDir;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults_Location  postLoc;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PostFaults_Severity  postSeverity;

struct dcoPostFaultCapa
{
    dcoPostFaultCapa(std::string k, std::string n, int sa, int b, int d, int l, int s)
    : key(k)
    , name(n)
    , serviceAffecting(static_cast<bool>(sa))
    , bitPos(b)
    , direction(static_cast<postDir>(d))
    , location(static_cast<postLoc>(l))
    , severity(static_cast<postSeverity>(s))
    {}
    std::string  key;
    std::string  name;
    bool         serviceAffecting;
    int          bitPos;
    postDir      direction;
    postLoc      location;
    postSeverity severity;
};
static std::map<std::string, dcoPostFaultCapa> postFaultCapa =
    {
        {"MC-POST-LINE-ADC-PLL-LOL-FAULT",
        dcoPostFaultCapa("MC_POST_LINE_ADC_PLL_LOL_FAULT",           "MC-POST-LINE-ADC-PLL-LOL-FAULT",           0,     0,      3,      3,      2)
        },
        {"MC-POST-LINE-DAC-PLL-LOL-FAULT",
        dcoPostFaultCapa("MC_POST_LINE_DAC_PLL_LOL_FAULT",           "MC-POST-LINE-DAC-PLL-LOL-FAULT",           0,     1,      3,      3,      2)
        },
        {"MC-POST-CLEANUP-PLL-OCXO-LOS-FAULT",
        dcoPostFaultCapa("MC_POST_CLEANUP_PLL_OCXO_LOS_FAULT",       "MC-POST-CLEANUP-PLL-OCXO-LOS-FAULT",       0,     2,      3,      3,      2)
        },
        {"MC-POST-CLEANUP-PLL-XTAL-LOS-FAULT",
        dcoPostFaultCapa("MC_POST_CLEANUP_PLL_XTAL_LOS_FAULT",       "MC-POST-CLEANUP-PLL-XTAL-LOS-FAULT",       0,     3,      3,      3,      2)
        },
        {"MC-POST-CLEANUP-PLL-OCXO-OOF-FAULT",
        dcoPostFaultCapa("MC_POST_CLEANUP_PLL_OCXO_OOF_FAULT",       "MC-POST-CLEANUP-PLL-OCXO-OOF-FAULT",       0,     4,      3,      3,      2)
        },
        {"MC-POST-CLEANUP-PLL-INPUT-LOL-FAULT",
        dcoPostFaultCapa("MC_POST_CLEANUP_PLL_INPUT_LOL_FAULT",      "MC-POST-CLEANUP-PLL-INPUT-LOL-FAULT",      0,     5,      3,      3,      2)
        },
        {"MC-POST-CLEANUP-PLL-INPUT-LOS-FAULT",
        dcoPostFaultCapa("MC_POST_CLEANUP_PLL_INPUT_LOS_FAULT",      "MC-POST-CLEANUP-PLL-INPUT-LOS-FAULT",      0,     6,      3,      3,      2)
        },
        {"MC-POST-CLEANUP-PLL-I2C-FAULT",
        dcoPostFaultCapa("MC_POST_CLEANUP_PLL_I2C_FAULT",            "MC-POST-CLEANUP-PLL-I2C-FAULT",            0,     7,      3,      3,      2)
        },
        {"MC-POST-TPLANE-PLL-LOL-FAULT",
        dcoPostFaultCapa("MC_POST_TPLANE_PLL_LOL_FAULT",             "MC-POST-TPLANE-PLL-LOL-FAULT",             0,     8,      3,      3,      2)
        },
        {"MC-POST-DAC-TONE-PLL-LOL-FAULT",
        dcoPostFaultCapa("MC_POST_DAC_TONE_PLL_LOL_FAULT",           "MC-POST-DAC-TONE-PLL-LOL-FAULT",           0,     9,      3,      3,      2)
        },
        {"MC-POST-ATOM-MISSING-FAULT",
        dcoPostFaultCapa("MC_POST_ATOM_NOT_PRESENTING_FAULT",        "MC-POST-ATOM-MISSING-FAULT",               0,     10,     3,      3,      2)
        },
        {"MC-POST-MISSING-CALCMN-FAULT",
        dcoPostFaultCapa("MC_POST_MISSING_CALCMN_FAULT",             "MC-POST-MISSING-CALCMN-FAULT",             0,     11,     3,      3,      2)
        },
        {"MC-POST-MISSING-CALBRD-FAULT",
        dcoPostFaultCapa("MC_POST_MISSING_CALBRD_FAULT",             "MC-POST-MISSING-CALBRD-FAULT",             0,     12,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-0-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_0_FAULT",                  "MC-POST-I2C-BUS-0-FAULT",                  0,     13,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-1-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_1_FAULT",                  "MC-POST-I2C-BUS-1-FAULT",                  0,     14,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-2-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_2_FAULT",                  "MC-POST-I2C-BUS-2-FAULT",                  0,     15,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-3-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_3_FAULT",                  "MC-POST-I2C-BUS-3-FAULT",                  0,     16,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-4-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_4_FAULT",                  "MC-POST-I2C-BUS-4-FAULT",                  0,     17,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-5-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_5_FAULT",                  "MC-POST-I2C-BUS-5-FAULT",                  0,     18,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-6-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_6_FAULT",                  "MC-POST-I2C-BUS-6-FAULT",                  0,     19,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-7-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_7_FAULT",                  "MC-POST-I2C-BUS-7-FAULT",                  0,     20,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-8-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_8_FAULT",                  "MC-POST-I2C-BUS-8-FAULT",                  0,     21,     3,      3,      2)
        },
        {"MC-POST-I2C-BUS-9-FAULT",
        dcoPostFaultCapa("MC_POST_I2C_BUS_9_FAULT",                  "MC-POST-I2C-BUS-9-FAULT",                  0,     22,     3,      3,      2)
        },
        {"MC-POST-uBLAZE-ACCESS-FAULT",
        dcoPostFaultCapa("MC_POST_uBLAZE_ACCESS_FAULT",              "MC-POST-uBLAZE-ACCESS-FAULT",              0,     23,     3,      3,      2)
        },
        {"MC-POST-SAC-BUS-RX-FAULT",
        dcoPostFaultCapa("MC_POST_SAC_BUS_RX_FAULT",                 "MC-POST-SAC-BUS-RX-FAULT",                 0,     24,     3,      3,      1)
        },
        {"UMGR-POST-NVRAM-ACCESS-FAULT",
        dcoPostFaultCapa("UMGR_POST_NVRAM_ACCESS_FAULT",             "UMGR-POST-NVRAM-ACCESS-FAULT",             0,     25,     3,      3,      2)
        },
        {"UMGR-POST-NOR-FLASH-ACCESS-FAULT",
        dcoPostFaultCapa("UMGR_POST_NOR_FLASH_ACCESS_FAULT",         "UMGR-POST-NOR-FLASH-ACCESS-FAULT",         0,     26,     3,      3,      2)
        },
        {"UMGR-POST-UNEXPECTED-PARTITION-FAULT",
        dcoPostFaultCapa("UMGR_POST_UNEXPECTED_PARTITION_FAULT",     "UMGR-POST-UNEXPECTED-PARTITION-FAULT",     0,     27,     3,      3,      2)
        },
        {"UMGR-POST-APPLICATION-SIGN-FAULT",
        dcoPostFaultCapa("UMGR_POST_APPLICATION_SIGN_FAULT",         "UMGR-POST-APPLICATION-SIGN-FAULT",         0,     28,     3,      3,      2)
        },
        {"UMGR-POST-GECKO-COM-FAULT",
        dcoPostFaultCapa("DCO_GMCU_INTERNAL_ACCESS_FAULT",           "UMGR-POST-GECKO-COM-FAULT",                0,     29,     3,      3,      2)
        },
        {"UMGR-POST-FPGA-LOAD-FAULT",
        dcoPostFaultCapa("UMGR_POST_FPGA_LOAD_FAULT",                "UMGR-POST-FPGA-LOAD-FAULT",                0,     30,     3,      3,      2)
        },
        {"UMGR-POST-FPGA-ACCESS-FAULT",
        dcoPostFaultCapa("UMGR_POST_FPGA_ACCESS_FAULT",              "UMGR-POST-FPGA-ACCESS-FAULT",              0,     31,     3,      3,      2)
        },
        {"DC-POST-ICELAND-LOAD-FAULT",
        dcoPostFaultCapa("DC_POST_ICELAND_LOAD_FAULT",               "DC-POST-ICELAND-LOAD-FAULT",               0,     32,     3,      3,      2)
        },
        {"DC-POST-MISSING-DSP-CALCMN-FAULT",
        dcoPostFaultCapa("DC_POST_MISSING_DSP_CALCMN_FAULT",         "DC-POST-MISSING-DSP-CALCMN-FAULT",         0,     33,     3,      3,      2)
        },
        {"DC-POST-MISSING-DSP-CALBRD-FAULT",
        dcoPostFaultCapa("DC_POST_MISSING_DSP_CALBRD_FAULT",         "DC-POST-MISSING-DSP-CALBRD-FAULT",         0,     34,     3,      3,      2)
        },
        {"DC-POST-DSP-TEMP-OORH-WARN",
        dcoPostFaultCapa("DC_POST_DSP_TEMP_OORH_WARN",               "DC-POST-DSP-TEMP-OORH-WARN",               0,     35,     3,      3,      2)
        },
        {"DC-POST-DSP-TEMP-OORH-SHUTDOWN",
        dcoPostFaultCapa("DC_POST_DSP_TEMP_OORH_SHUTDOWN",           "DC-POST-DSP-TEMP-OORH-SHUTDOWN",           0,     36,     3,      3,      2)
        },
        {"DC-POST-DSP-PCIE-ACCESS-FAULT",
        dcoPostFaultCapa("DC_POST_DSP_PCIE_ACCESS_FAULT",            "DC-POST-DSP-PCIE-ACCESS-FAULT",            0,     37,     3,      3,      2)
        },
        {"DC-POST-FRAME-SYNC-FAILURE",
        dcoPostFaultCapa("DC_POST_FRAME_SYNC_FAILURE",               "DC-POST-FRAME-SYNC-FAILURE",               0,     38,     3,      3,      2)
        },
        {"MC-POST-TEMP1-OOR-FAULT",
        dcoPostFaultCapa("MC_POST_TEMP1_OOR_FAULT",                  "MC-POST-TEMP1-OOR-FAULT",                  0,     39,     3,      3,      2)
        },
        {"MC-POST-TEMP2-OOR-FAULT",
        dcoPostFaultCapa("MC_POST_TEMP2_OOR_FAULT",                  "MC-POST-TEMP2-OOR-FAULT",                  0,     40,     3,      3,      2)
        },
        {"MC-POST-TEMP3-OOR-FAULT",
        dcoPostFaultCapa("MC_POST_TEMP3_OOR_FAULT",                  "MC-POST-TEMP3-OOR-FAULT",                  0,     41,     3,      3,      2)
        },
        {"MC-POST-TEMP4-OOR-FAULT",
        dcoPostFaultCapa("MC_POST_TEMP4_OOR_FAULT",                  "MC-POST-TEMP4-OOR-FAULT",                  0,     42,     3,      3,      2)
        },
        {"MC-POST-MIRAGE-REVISION-FAULT",
        dcoPostFaultCapa("DCO_MIRAGE_REVISION_MISMATCH",             "MC-POST-MIRAGE-REVISION-FAULT",            0,     43,     3,      3,      2)
        },
    };

typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacPostFaults_Direction dacPostDir;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacPostFaults_Location  dacPostLoc;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_DacPostFaults_Severity  dacPostSeverity;

struct dcoDacPostFaultCapa
{
    dcoDacPostFaultCapa(std::string k, std::string n, int sa, int b, int d, int l, int s)
    : key(k)
    , name(n)
    , serviceAffecting(static_cast<bool>(sa))
    , bitPos(b)
    , direction(static_cast<dacPostDir>(d))
    , location(static_cast<dacPostLoc>(l))
    , severity(static_cast<dacPostSeverity>(s))
    {}
    std::string     key;
    std::string     name;
    bool            serviceAffecting;
    int             bitPos;
    dacPostDir      direction;
    dacPostLoc      location;
    dacPostSeverity severity;
};
static std::map<std::string, dcoDacPostFaultCapa> dacPostFaultCapa =
    {
        {"MC-POST-TX-SOA-IDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_TX_SOA_IDAC_TEMP_OORH_WARN",       "MC-POST-TX-SOA-IDAC-TEMP-OORH-WARN",       0,  0,      3,      3,      1)
        },
        {"MC-POST-WTL-GAIN-IDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_WTL_GAIN_IDAC_TEMP_OORH_WARN",     "MC-POST-WTL-GAIN-IDAC-TEMP-OORH-WARN",     0,  1,      3,      3,      1)
        },
        {"MC-POST-RX-SHSOA-IDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_RX_SHSOA_IDAC_TEMP_OORH_WARN",     "MC-POST-RX-SHSOA-IDAC-TEMP-OORH-WARN",     0,  2,      3,      3,      1)
        },
        {"MC-POST-MZM-VOA-VDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_MZM_VOA_VDAC_TEMP_OORH_WARN",      "MC-POST-MZM-VOA-VDAC-TEMP-OORH-WARN",      0,  3,      3,      3,      1)
        },
        {"MC-POST-MZM-PA-VDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_MZM_PA_VDAC_TEMP_OORH_WARN",       "MC-POST-MZM-PA-VDAC-TEMP-OORH-WARN",       0,  4,      3,      3,      1)
        },
        {"MC-POST-MR-HTR-VDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_MR_HTR_VDAC_TEMP_OORH_WARN",       "MC-POST-MR-HTR-VDAC-TEMP-OORH-WARN",       0,  5,      3,      3,      1)
        },
        {"MC-POST-MZM-PHTR-VDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_MZM_PHTR_VDAC_TEMP_OORH_WARN",     "MC-POST-MZM-PHTR-VDAC-TEMP-OORH-WARN",     0,  6,      3,      3,      1)
        },
        {"MC-POST-MZM-VCAT-VDAC-TEMP-OORH-WARN",
        dcoDacPostFaultCapa("MC_POST_MZM_VCAT_VDAC_TEMP_OORH_WARN",     "MC-POST-MZM-VCAT-VDAC-TEMP-OORH-WARN",     0,  7,      3,      3,      1)
        },
    };

typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsPostFaults_Direction psPostDir;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsPostFaults_Location  psPostLoc;
typedef ::dcoyang::infinera_dco_card_fault::DcoCardFault_Capabilities_PsPostFaults_Severity  psPostSeverity;

struct dcoPsPostFaultCapa
{
    dcoPsPostFaultCapa(std::string k, std::string n, int sa, int b, int d, int l, int s)
    : key(k)
    , name(n)
    , serviceAffecting(static_cast<bool>(sa))
    , bitPos(b)
    , direction(static_cast<psPostDir>(d))
    , location(static_cast<psPostLoc>(l))
    , severity(static_cast<psPostSeverity>(s))
    {}
    std::string    key;
    std::string    name;
    bool           serviceAffecting;
    int            bitPos;
    psPostDir      direction;
    psPostLoc      location;
    psPostSeverity severity;
};
static std::map<std::string, dcoPsPostFaultCapa> psPostFaultCapa =
    {
        {"MC-POST-ATL-WAVE0-0-9V-LDO-FAULT-LOW",
        dcoPsPostFaultCapa("DCO_ASIC_WAVE0_0_9V_PWR_OOR_LOW",          "MC-POST-ATL-WAVE0-0-9V-LDO-FAULT-LOW",     0,   0,      3,      3,      2)
        },
        {"MC-POST-ATL-WAVE0-0-9V-LDO-FAULT-HIGH",
        dcoPsPostFaultCapa("DCO_ASIC_WAVE0_0_9V_PWR_OOR_HIGH",         "MC-POST-ATL-WAVE0-0-9V-LDO-FAULT-HIGH",    0,   1,      3,      3,      2)
        },
        {"MC-POST-ATL-WAVE1-0-9V-LDO-FAULT-LOW",
        dcoPsPostFaultCapa("DCO_ASIC_WAVE1_0_9V_PWR_OOR_LOW",          "MC-POST-ATL-WAVE1-0-9V-LDO-FAULT-LOW",     0,   2,      3,      3,      2)
        },
        {"MC-POST-ATL-WAVE1-0-9V-LDO-FAULT-HIGH",
        dcoPsPostFaultCapa("DCO_ASIC_WAVE1_0_9V_PWR_OOR_HIGH",         "MC-POST-ATL-WAVE1-0-9V-LDO-FAULT-HIGH",    0,   3,      3,      3,      2)
        },
        {"MC-POST-ATL-TVDD-0-75V-FAULT-HIGH",
        dcoPsPostFaultCapa("MC_POST_ATL_TVDD_0P75V_FAULT_HIGH",        "MC-POST-ATL-TVDD-0-75V-FAULT-HIGH",        0,   4,      3,      3,      2)
        },
        {"MC-POST-ATL-TVDD-0-75V-FAULT-LOW",
        dcoPsPostFaultCapa("MC_POST_ATL_TVDD_0P75V_FAULT_LOW",         "MC-POST-ATL-TVDD-0-75V-FAULT-LOW",         0,   5,      3,      3,      2)
        },
        {"MC-POST-ATL-RVDD-0-75V-FAULT-HIGH",
        dcoPsPostFaultCapa("MC_POST_ATL_RVDD_0P75V_FAULT_HIGH",        "MC-POST-ATL-RVDD-0-75V-FAULT-HIGH",        0,   6,      3,      3,      2)
        },
        {"MC-POST-ATL-RVDD-0-75V-FAULT-LOW",
        dcoPsPostFaultCapa("MC_POST_ATL_RVDD_0P75V_FAULT_LOW",         "MC-POST-ATL-RVDD-0-75V-FAULT-LOW",         0,   7,      3,      3,      2)
        },
        {"ATLC-CORE-HW-ERR",
        dcoPsPostFaultCapa("MC_POST_PS_ATLC_CORE_HW_FAULT",            "ATLC-CORE-HW-ERR",                         0,   8,      3,      3,      2)
        },
        {"MC-POST-ATOM-PS-3-3V-FAULT",
        dcoPsPostFaultCapa("MC_POST_ATOM_PS_3_3V_FAULT",               "MC-POST-ATOM-PS-3-3V-FAULT",               0,   9,      3,      3,      2)
        },
        {"MC-POST-ATOM-PS-5V-FAULT",
        dcoPsPostFaultCapa("MC_POST_ATOM_PS_5V_FAULT",                 "MC-POST-ATOM-PS-5V-FAULT",                 0,   10,     3,      3,      2)
        },
    };

}

#endif /* CHM6_DATAPLANE_MS_SRC_SIM_SIMDCOCARDFAULTS_H_ */
