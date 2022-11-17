/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef CHM6_DATAPLANE_MS_SRC_MANAGER_CARDFAULTIDTOPBID_H_
#define CHM6_DATAPLANE_MS_SRC_MANAGER_CARDFAULTIDTOPBID_H_

#include <string>

namespace DataPlane
{
// R1.0
const std::map<std::string, std::string> dcoCardEqptFaultIdToPb =
{
        /*
         * Defined in sheet "DCO and Power"
         */
        { "TDCO-LOW",                            "DCO-TEMP-OORL-WARN"              }, // row 22
        { "TDCO-HIGH",                           "DCO-TEMP-OORH-WARN"              }, // row 23

        { "DC-DSP-TEMP-OORH-WARN",               "DCO-DSP-TEMP-OORH-WARN"          }, // row 24
        { "DC-DSP-TEMP-OORL-WARN",               "DCO-DSP-TEMP-OORL-WARN"          }, // row 25
        { "DC-DSP-TEMP-OORH-SHUTDOWN",           "DCO-DSP-TEMP-OORH-SHUTDOWN"      }, // row 26

        { "MC-ATL-WAVE1-0-9V-LDO-FAULT-LOW",     "DCO-ASIC-WAVE1-0-9V-PWR-OORL"    }, // row 36
        { "MC-ATL-WAVE1-0-9V-LDO-FAULT-HIGH",    "DCO-ASIC-WAVE1-0-9V-PWR-OORH"    }, // row 37
        { "MC-ATL-WAVE0-0-9V-LDO-FAULT-LOW",     "DCO-ASIC-WAVE0-0-9V-PWR-OORL"    }, // row 38
        { "MC-ATL-WAVE0-0-9V-LDO-FAULT-HIGH",    "DCO-ASIC-WAVE0-0-9V-PWR-OORH"    }, // row 39

        { "MC-ATL-TVDD-0-75V-FAULT-LOW",         "DCO-TVDD-0-75V-VOUT-FAULT-LOW"   }, // row 40
        { "MC-ATL-TVDD-0-75V-FAULT-HIGH",        "DCO-TVDD-0-75V-VOUT-FAULT-HIGH"  }, // row 41
        { "MC-ATL-RVDD-0-75V-FAULT-LOW",         "DCO-RVDD-0-75V-VOUT-FAULT-LOW"   }, // row 42
        { "MC-ATL-RVDD-0-75V-FAULT-HIGH",        "DCO-RVDD-0-75V-VOUT-FAULT-HIGH"  }, // row 43

        { "MC-TX-SOA-IDAC-TEMP-OORH-WARN",       "DCO-TX-SOA-IDAC-TEMP-OORH-WARN"  }, // row 80
        { "MC-RX-SHSOA-IDAC-TEMP-OORH-WARN",     "DCO-RX-SHSOA-IDAC-TEMP-OORH-WARN"}, // row 87
        { "MC-WTL-GAIN-IDAC-TEMP-OORH-WARN",     "DCO-WTL-GAIN-IDAC-TEMP-OORH-WARN"}, // row 94
        { "MC-MZM-PA-VDAC-TEMP-OORH-WARN",       "DCO-MZM-PA-VDAC-TEMP-OORH-WARN"  }, // row 99
        { "MC-MZM-VCAT-VDAC-TEMP-OORH-WARN",     "DCO-MZM-VCAT-VDAC-TEMP-OORH-WARN"}, // row 105
        { "MC-MZM-PHTR-VDAC-TEMP-OORH-WARN",     "DCO-PH-HTR-VDAC-TEMP-OORH-WARN"  }, // row 109
        { "MC-MR-HTR-VDAC-TEMP-OORH-WARN",       "DCO-MR-HTR-VDAC-TEMP-OORH-WARN"  }, // row 112

        { "DC-DSP-PCIE-ACCESS-FAULT",            "DCO-DSP-PCIE-ACCESS-FAILURE"     }, // row 115

        { "MC-SAC-BUS-RX-FAULT",                 "DCO-STATUS-CONTROL-BUS-FAULT"    }, // row 116

        { "MC-MCU-TEC-I2C-FAULT",                "DCO-TEC-ACCESS-FAILURE"          }, // row 126

        { "MC-MCU-TEC-CONTROL-WARNING",          "DCO-TEC-CONTROL-LOOP-WARN"       }, // row 128
        { "MC-FAULT-FTPIC-HIGH",                 "DCO-TEC-CONTROL-LOOP-FAULT-HIGH" }, // row 129
        { "MC-FAULT-FTPIC-LOW",                  "DCO-TEC-CONTROL-LOOP-FAULT-LOW"  }, // row 130

        { "MC-TEC-FAULT-NON-SPECIFIC",           "DCO-TEC-FAULT-NON-SPECIFIC"      }, // row 131

        { "MC-TEC-TEMP-READ-FAILURE",            "DCO-TEC-TEMP-READ-FAILURE"       }, // row 132

        { "MC-UBLAZE-ACCESS-FAILURE",            "DCO-UBLAZE-ACCESS-FAILURE"       }, // row 138

        { "DC-ICELAND-LOAD-FAULT",               "DCO-DSP-FW-LOAD-FAILURE"         }, // row 145

        { "DC-ICELAND-HEARTBEAT-FAILURE",        "DCO-DSP-FW-HEARTBEAT-FAILURE"    }, // row 147

        { "DCO-SECUREBOOT-SIGNATURE-FAULT",      "DCO-SECUREBOOT-SIGNATURE-FAULT"  }, // row 150

        { "MC-I2C-BUS-0-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-1-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-2-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-3-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-4-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-5-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-6-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-7-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-8-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154
        { "MC-I2C-BUS-9-FAULT",                  "DCO-I2C-DEV-FAILURE"             }, // row 154

        { "MC-CLEANUP-PLL-OCXO-LOS-FAULT",       "DCO-CLEANUP-PLL-OCXO-LOS"        }, // row 155
        { "MC-CLEANUP-PLL-XTAL-LOS-FAULT",       "DCO-CLEANUP-PLL-XTAL-LOS"        }, // row 156
        { "MC-CLEANUP-PLL-OCXO-OOF-FAULT",       "DCO-CLEANUP-PLL-OCXO-FREQ-OOR"   }, // row 157
        { "MC-CLEANUP-PLL-INPUT-LOL-FAULT",      "DCO-CLEANUP-PLL-INPUT-LOL"       }, // row 158
        { "MC-CLEANUP-PLL-INPUT-LOS-FAULT",      "DCO-CLEANUP-PLL-INPUT-LOS"       }, // row 159
        { "MC-CLEANUP-PLL-I2C-FAULT",            "DCO-CLEANUP-PLL-I2C-READ-FAULT"  }, // row 160
        { "MC-CLEANUP-PLL-GEN-FAULT",            "DCO-CLEANUP-PLL-I2C-FAULT"       }, // row 161
        { "MC-TPLANE-PLL-LOL-FAULT",             "DCO-TPLANE-PLL-LOL"              }, // row 162
        { "MC-DAC-TONE-PLL-LOL-FAULT",           "DCO-DAC-TONE-PLL-LOL"            }, // row 163
        { "MC-LINE-ADC-PLL-LOL-FAULT",           "DCO-LINE-ADC-PLL-LOL"            }, // row 164
        { "MC-LINE-DAC-PLL-LOL-FAULT",           "DCO-LINE-DAC-PLL-LOL"            }, // row 165

        { "VDCO-HIGH",                           "DCO-VOLTAGE-HIGH-FAULT"          }, // row 175
        { "VDCO-LOW",                            "DCO-VOLTAGE-LOW-FAULT"           }, // row 176
        { "IDCO-HIGH",                           "DCO-CURRENT-HIGH-FAULT"          }, // row 177
        { "IDCO-LOW",                            "DCO-CURRENT-LOW-FAULT"           }, // row 178
        { "MC-FPGA-ECC-FAULT",                   "DCO-FPGA-ECC-FAULT"              }, // row 179

        /*
         * defined in sheet "ACO incl ATOM"
         */
        { "ATOM-PS-3-3V-FAULT",                  "DCO-ATOM-PS-3-3V-FAULT"          }, // row 6
        { "ATOM-PS-5V-FAULT",                    "DCO-ATOM-PS-5V-FAULT"            }, // row 7

        { "OPT-OORH",                            "DCO-TX-POWER-OORH"               }, // row 10
        { "OPT-OORL",                            "DCO-TX-POWER-OORL"               }, // row 11
        { "MC-TX-PUMP-FAULT",                    "DCO-TX-PUMP-FAULT"               }, // row 12

        { "MC-TONE-POWER-OOR-FAULT",             "DCO-TONE-PWR-OOR-FAULT"          }, // row 21

        { "TX-VCC-LOW",                          "DCO-TX-VCC-OORL-FAULT"           }, // row 27
        { "TX-VCC-HIGH",                         "DCO-TX-VCC-OORH-FAULT"           }, // row 28
        { "TX-VTT-LOW",                          "DCO-TX-VTT-OORL-FAULT"           }, // row 29
        { "TX-VTT-HIGH",                         "DCO-TX-VTT-OORH-FAULT"           }, // row 30
        { "RX-VCC-LOW",                          "DCO-RX-VCC-OORL-FAULT"           }, // row 31
        { "RX-VCC-HIGH",                         "DCO-RX-VCC-OORH-FAULT"           }, // row 32
        { "RX-VTT-LOW",                          "DCO-RX-VTT-OORL-FAULT"           }, // row 33
        { "RX-VTT-HIGH",                         "DCO-RX-VTT-OORH-FAULT"           }, // row 34
};

const std::map<std::string, std::string> dcoCardPostFaultIdToPb =
{
        /*
         * Defined in sheet "DCO and Power"
         */
        { "MC-POST-TEMP1-OOR-FAULT",                "DCO-TEMP1-OORH-WARN"               }, // row 21
        { "MC-POST-TEMP2-OOR-FAULT",                "DCO-TEMP2-OORH-WARN"               }, // row 21
        { "MC-POST-TEMP3-OOR-FAULT",                "DCO-TEMP3-OORH-WARN"               }, // row 21
        { "MC-POST-TEMP4-OOR-FAULT",                "DCO-TEMP4-OORH-WARN"               }, // row 21

        { "DC-POST-DSP-TEMP-OORH-WARN",             "DCO-DSP-TEMP-OORH-WARN"            }, // row 24
        { "DC-POST-DSP-TEMP-OORH-SHUTDOWN",         "DCO-DSP-TEMP-OORH-SHUTDOWN"        }, // row 26

        { "MC-POST-ATL-WAVE1-0-9V-LDO-FAULT-LOW",   "DCO-ASIC-WAVE1-0-9V-PWR-OORL"      }, // row 36
        { "MC-POST-ATL-WAVE1-0-9V-LDO-FAULT-HIGH",  "DCO-ASIC-WAVE1-0-9V-PWR-OORH"      }, // row 37
        { "MC-POST-ATL-WAVE0-0-9V-LDO-FAULT-LOW",   "DCO-ASIC-WAVE0-0-9V-PWR-OORL"      }, // row 38
        { "MC-POST-ATL-WAVE0-0-9V-LDO-FAULT-HIGH",  "DCO-ASIC-WAVE0-0-9V-PWR-OORH"      }, // row 39

        { "MC-POST-ATL-TVDD-0-75V-FAULT-LOW",       "DCO-TVDD-0-75V-VOUT-FAULT-LOW"     }, // row 40
        { "MC-POST-ATL-TVDD-0-75V-FAULT-HIGH",      "DCO-TVDD-0-75V-VOUT-FAULT-HIGH"    }, // row 41
        { "MC-POST-ATL-RVDD-0-75V-FAULT-LOW",       "DCO-RVDD-0-75V-VOUT-FAULT-LOW"     }, // row 42
        { "MC-POST-ATL-RVDD-0-75V-FAULT-HIGH",      "DCO-RVDD-0-75V-VOUT-FAULT-HIGH"    }, // row 43

        { "MC-POST-TX-SOA-IDAC-TEMP-OORH-WARN",     "DCO-TX-SOA-IDAC-TEMP-OORH-WARN"    }, // row 80
        { "MC-POST-RX-SHSOA-IDAC-TEMP-OORH-WARN",   "DCO-RX-SHSOA-IDAC-TEMP-OORH-WARN"  }, // row 87
        { "MC-POST-WTL-GAIN-IDAC-TEMP-OORH-WARN",   "DCO-WTL-GAIN-IDAC-TEMP-OORH-WARN"  }, // row 94
        { "MC-POST-MZM-PA-VDAC-TEMP-OORH-WARN",     "DCO-MZM-PA-VDAC-TEMP-OORH-WARN"    }, // row 99
        { "MC-POST-MZM-VCAT-VDAC-TEMP-OORH-WARN",   "DCO-MZM-VCAT-VDAC-TEMP-OORH-WARN"  }, // row 105
        { "MC-POST-MZM-PHTR-VDAC-TEMP-OORH-WARN",   "DCO-PH-HTR-VDAC-TEMP-OORH-WARN"    }, // row 109
        { "MC-POST-MR-HTR-VDAC-TEMP-OORH-WARN",     "DCO-MR-HTR-VDAC-TEMP-OORH-WARN"    }, // row 112

        { "DC-POST-DSP-PCIE-ACCESS-FAULT",          "DCO-DSP-PCIE-ACCESS-FAILURE"       }, // row 115

        { "MC-POST-SAC-BUS-RX-FAULT",               "DCO-STATUS-CONTROL-BUS-FAULT"      }, // row 116

        { "UMGR-POST-GECKO-COM-FAULT",              "DCO-GMCU-INTERNAL-ACCESS-FAULT"    }, // row 118

        { "MC-POST-MIRAGE-REVISION-FAULT",          "DCO-MIRAGE-REVISION-MISMATCH"      }, // row 125

        { "MC-POST-MCU-TEC-I2C-FAULT",              "DCO-TEC-ACCESS-FAILURE"            }, // row 126

        { "MC-POST-MCU-TEC-ALRT-FAULT",             "DCO-TEC-FAULT-NON-SPECIFIC"        }, // row 131

        { "MC-POST-MCU-TEC-TEMP-READ-FAILURE-FAULT","DCO-TEC-TEMP-READ-FAILURE"         }, // row 132

        { "UMGR-POST-FPGA-LOAD-FAULT",              "DCO-PL-LOAD-FAILURE"               }, // row 133
        { "UMGR-POST-FPGA-ACCESS-FAULT",            "DCO-PL-ACCESS-FAILURE"             }, // row 134

        { "MC-POST-PIC-SIGE-ASIC-VERSION-FAULT",    "DCO-PIC-ASIC-REVISION-MISMATCH"    }, // row 136

        { "MC-POST-UBLAZE-ACCESS-FAULT",            "DCO-UBLAZE-ACCESS-FAILURE"         }, // row 138

        { "MC-POST-MISSING-CALCMN-FAULT",           "DCO-COMMON-CAL-MISSING"            }, // row 139
        { "DC-POST-MISSING-DSP-CALCMN-FAULT",       "DCO-DSP-COMMON-CAL-MISSING"        }, // row 140
        { "MC-POST-MISSING-CALBRD-FAULT",           "DCO-BOARD-CAL-MISSING"             }, // row 141
        { "DC-POST-MISSING-DSP-CALBRD-FAULT",       "DCO-DSP-BOARD-CAL-MISSING"         }, // row 142

        { "DC-POST-ICELAND-LOAD-FAULT",             "DCO-DSP-FW-LOAD-FAILURE"           }, // row 145

        { "DC-POST-DSP-CAL-VERSION-FAULT",          "DCO-DSP-REVISION-MISMATCH"         }, // row 146

        { "UMGR-POST-NOR-FLASH-ACCESS-FAULT",       "DCO-NOR-FLASH-ACCESS-FAILURE"      }, // row 148
        { "UMGR-POST-UNEXPECTED-PARTITION-FAULT",   "DCO-ZYNQ-UNEXPECTED-PARTITION"     }, // row 149
        { "UMGR-POST-APPLICATION-SIGN-FAULT",       "DCO-SECUREBOOT-SIGNATURE-FAULT"    }, // row 150

        { "MC-POST-I2C-BUS-0-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-1-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-2-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-3-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-4-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-5-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-6-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-7-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-8-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154
        { "MC-POST-I2C-BUS-9-FAULT",                "DCO-I2C-DEV-FAILURE"               }, // row 154

        { "MC-POST-CLEANUP-PLL-OCXO-LOS-FAULT",     "DCO-CLEANUP-PLL-OCXO-LOS"          }, // row 155
        { "MC-POST-CLEANUP-PLL-XTAL-LOS-FAULT",     "DCO-CLEANUP-PLL-XTAL-LOS"          }, // row 156
        { "MC-POST-CLEANUP-PLL-OCXO-OOF-FAULT",     "DCO-CLEANUP-PLL-OCXO-FREQ-OOR"     }, // row 157
        { "MC-POST-CLEANUP-PLL-INPUT-LOL-FAULT",    "DCO-CLEANUP-PLL-INPUT-LOL"         }, // row 158
        { "MC-POST-CLEANUP-PLL-INPUT-LOS-FAULT",    "DCO-CLEANUP-PLL-INPUT-LOS"         }, // row 159
        { "MC-POST-CLEANUP-PLL-I2C-FAULT",          "DCO-CLEANUP-PLL-I2C-READ-FAULT"    }, // row 160

        { "MC-POST-TPLANE-PLL-LOL-FAULT",           "DCO-TPLANE-PLL-LOL"                }, // row 162
        { "MC-POST-DAC-TONE-PLL-LOL-FAULT",         "DCO-DAC-TONE-PLL-LOL"              }, // row 163
        { "MC-POST-LINE-ADC-PLL-LOL-FAULT",         "DCO-LINE-ADC-PLL-LOL"              }, // row 164
        { "MC-POST-LINE-DAC-PLL-LOL-FAULT",         "DCO-LINE-DAC-PLL-LOL"              }, // row 165

        { "UMGR-POST-NVRAM-ACCESS-FAULT",           "DCO-NVRAM-ACCESS-FAILURE"          }, // row 168

        /*
         * defined in sheet "ACO incl ATOM"
         */
        { "MC-POST-ATOM-PS-3-3V-FAULT",             "DCO-ATOM-PS-3-3V-FAULT"            }, // row 6
        { "MC-POST-ATOM-PS-5V-FAULT",               "DCO-ATOM-PS-5V-FAULT"              }, // row 7
        { "MC-POST-ATOM-MISSING-FAULT",             "DCO-ATOM-MISSING-FAULT"            }, // row 20

};

}; // namespace DataPlane

#endif /* CHM6_DATAPLANE_MS_SRC_MANAGER_CARDFAULTIDTOPBID_H_ */
