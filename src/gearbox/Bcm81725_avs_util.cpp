//-------------------------------------------------------------
//
// Copyright(c) 2020 Infinera Corporation. All right reserved.
//
//-------------------------------------------------------------

#include <stdio.h>
#include <thread>
#include <iostream>
#include <iomanip>
//#include "InfnLogger.h"
#include "GearBoxIf.h"
#include "Bcm81725_avs_util.h"

#include "milleniob_reference_app/milleniob_helper.h"
#include "milleniob_reference_app/milleniob_common.h"

#ifdef PMBUS_DEBUG
#define pmbus_debug_log(expr)  \
        do  { std::cout << expr; \
        }   while(0)
#else
#define pmbus_debug_log(expr)
#endif

namespace gearbox {

const unsigned int cTopMzMdioAddrMask = 0x10;

const std::string bcmMilleniob = "milleniob";

const Bcm81725Lane cParamTbl[NUM_MEZZ_BRDS][NUM_BCMS_PER_MEZZ] =
                                {
                                  { {0, 0x4, 1, 0xff}, {0, 0x8, 1, 0xff}, {0, 0xc, 1, 0xff} },
                                  { {1, 0x4, 1, 0xff}, {1, 0x8, 1, 0xff}, {1, 0xc, 1, 0xff} }
                                };

extern int mdio_read(void *user, unsigned int mdioAddr, unsigned int regAddr, unsigned int *data);
extern int mdio_write(void *user, unsigned int mdioAddr, unsigned int regAddr, unsigned int data);

int bcm_plp_pmbus_access(const char* chip_name, bcm_plp_access_t plp_info, bcm_plp_pmbus_info_t *pmbus_info)
{
    capi_phy_info_t phy_info;
    capi_pmbus_info_t capi_pmbus_info;
    plp_milleniob_phymod_access_t phy_access;
    plp_milleniob_phymod_bus_t bus;
    int rv = 0;

    memset(&phy_access, 0, sizeof(plp_milleniob_phymod_access_t));
    memset(&bus, 0, sizeof(plp_milleniob_phymod_bus_t));
    memset(&phy_info, 0, sizeof(capi_phy_info_t));
    memset(&capi_pmbus_info, 0, sizeof(capi_pmbus_info_t));

    std::string name = "BCM-PM-IF";
    char* bus_name = new char[name.size()+1];
    strcpy(bus_name, name.c_str());
    bus.bus_name = bus_name;
    bus.read = mdio_read;
    bus.write = mdio_write;
    phy_access.bus = &bus;
    phy_access.user_acc = plp_info.platform_ctxt;
    phy_access.lane_mask = plp_info.lane_map;
    phy_access.addr = plp_info.phy_addr;
    phy_info.phy_id = plp_info.phy_addr;
    phy_info.lane_mask = plp_info.lane_map;
    if (plp_info.if_side) {
        if (CHIP_ID == 0x81725) {
            phy_info.core_ip = PHY_REVERSE_MODE ? CORE_IP_CLIENT : CORE_IP_LW;
        } else {
            phy_info.core_ip = CORE_IP_CLIENT;
        }
    } else {
        if (CHIP_ID == 0x81725) {
            phy_info.core_ip = PHY_REVERSE_MODE ? CORE_IP_LW : CORE_IP_CLIENT;
        } else {
            phy_info.core_ip = CORE_IP_LW;
        }
    }
    phy_info.ref_ptr = (void *)&phy_access;

    if (pmbus_info == NULL) {
        pmbus_debug_log("Invalid pmbus info" << std::endl);
        delete[] bus_name;
        return -1;
    }
    capi_pmbus_info.cmd = pmbus_info->pmbus_cmd;
    capi_pmbus_info.len = pmbus_info->length;
    capi_pmbus_info.i2c_address = pmbus_info->i2c_address;
    capi_pmbus_info.w_data_ptr = pmbus_info->w_data;
    capi_pmbus_info.r_data_ptr = pmbus_info->r_data;

    if (pmbus_info->pmbus_type == bcmplpPmbusInit) {
        rv = plp_milleniob_capi_pmbus_forward_init(&phy_info);
        if (rv) {
            pmbus_debug_log("plp_milleniob_capi_pmbus_forward_init failed" << std::endl);
            delete[] bus_name;
            return rv;
        }
    } else if (pmbus_info->pmbus_type == bcmplpPmbusWriteRequest) {
        rv = plp_milleniob_capi_pmbus_request_write(&phy_info, &capi_pmbus_info);
        if (rv) {
            pmbus_debug_log("plp_milleniob_capi_pmbus_request_write failed" << std::endl);
            delete[] bus_name;
            return rv;
        }
    } else if (pmbus_info->pmbus_type == bcmplpPmbusWriteStatus) {
        rv = plp_milleniob_capi_pmbus_poll_write_status(&phy_info, &capi_pmbus_info);
        if (rv == RR_PMBUS_FORWARD_WRITE_WAIT) {
            pmbus_debug_log("plp_milleniob_capi_pmbus_poll_write_status in process" << std::endl);
        } else if (rv == RR_SUCCESS) {
            pmbus_debug_log("plp_milleniob_capi_pmbus_poll_write_status success" << std::endl);
        } else {
            pmbus_debug_log("plp_milleniob_capi_pmbus_poll_write_status failed" << std::endl);
            delete[] bus_name;
            return rv;
        }
    } else if (pmbus_info->pmbus_type == bcmplpPmbusReadRequest) {
        rv = plp_milleniob_capi_pmbus_request_read(&phy_info, &capi_pmbus_info);
        if (rv) {
           pmbus_debug_log("plp_milleniob_capi_pmbus_request_read failed" << std::endl);
           delete[] bus_name;
           return rv;
        }
    } else if (pmbus_info->pmbus_type == bcmplpPmbusReadPollReply) {
        rv  = plp_milleniob_capi_pmbus_poll_read_reply(&phy_info, &capi_pmbus_info);
        if (rv == RR_PMBUS_FORWARD_READ_WAIT) {
            pmbus_debug_log("plp_milleniob_capi_pmbus_poll_read_reply in process" << std::endl);
        } else if (rv == RR_SUCCESS) {
            pmbus_debug_log("plp_milleniob_capi_pmbus_poll_read_reply success" << std::endl);
        } else {
            pmbus_debug_log("plp_milleniob_capi_pmbus_poll_read_reply failed" << std::endl);
            delete[] bus_name;
            return rv;
        }
    } else {
        pmbus_debug_log("Invalid pmbus request tye = " << pmbus_info->pmbus_type <<  std::endl);
        delete[] bus_name;
        return -1;
    }

    delete[] bus_name;
    return rv;
}

int getAvsPmbusData(const Bcm81725Lane& param, uint32_t op, uint32_t len, uint8_t *data)
{
    pmbus_debug_log( __FILE__ << ": " <<__LINE__ << ": " << __func__ << " ");
    pmbus_debug_log( "bus=" << param.bus << " mdioAddr=" << param.mdioAddr << " side="
            << param.side << " laneNum=" << param.laneNum << std::endl);

    int rv = 0;
    unsigned int phy_addr;

    if (param.bus == MEZZ_BRD_TOP)
    {
        phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
        pmbus_debug_log("-------------------------- This is TopMz ------------------------------" << std::endl);
    }
    else
    {
        phy_addr = param.mdioAddr;
        pmbus_debug_log("-------------------------- This is BtmMz -------------------------------" << std::endl);
    }

    bcm_plp_access_t phy_info;
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));

    int retry_cnt = 20;
    int i2c_addr  = 0x1E;

    bcm_plp_pmbus_info_t pmbus_info;
    memset(&pmbus_info, 0, sizeof(bcm_plp_pmbus_info_t));

    phy_info.platform_ctxt = (void*)(&param.bus);
    phy_info.phy_addr = phy_addr;
    phy_info.if_side = 0;
    phy_info.lane_map = 0xFFFF;

    pmbus_info.pmbus_type = bcmplpPmbusInit;

    pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusInit ... " << std::endl);

    pmbus_debug_log("===============bcm_plp_pmbus_access() Init parameters =====================" << std::endl);
    pmbus_debug_log("phy_info.phy_addr        : 0x" << std::hex << phy_info.phy_addr << std::dec <<  std::endl);
    pmbus_debug_log("phy_info.if_side         : " << phy_info.if_side <<  std::endl);
    pmbus_debug_log("phy_info.lane_map        : 0x" << std::hex <<  phy_info.lane_map << std::dec <<  std::endl);
    pmbus_debug_log("phy_info.data_path_dir   : " << phy_info.data_path_dir << " (1:Egress 2:Ingress 3: both)\n" << std::endl);

    pmbus_debug_log("pmbus_info.pmbus_type    : " << pmbus_info.pmbus_type << " (0:Init 1:WriteReq 2: WriteStat 3: ReadReq 4: ReadPollReply)\n" << std::endl);

    rv = bcm_plp_pmbus_access(bcmMilleniob.c_str(), phy_info, &pmbus_info);
    if (rv) {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusInit failed with rv:" << rv <<  std::endl);
        return rv;
    } else {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusInit success with rv:" << rv <<  std::endl);
    }

    pmbus_info.pmbus_cmd = op;
    pmbus_info.length = len;
    pmbus_info.i2c_address = i2c_addr;
    pmbus_info.r_data = data;
    pmbus_info.pmbus_type = bcmplpPmbusReadRequest;

    pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusReadRequest ... " << std::endl);

    pmbus_debug_log("===============bcm_plp_pmbus_access() Read parameters =====================" << std::endl);
    pmbus_debug_log("pmbus_info.pmbus_type    : " << pmbus_info.pmbus_type << " (0:Init 1:WriteReq 2: WriteStat 3: ReadReq 4: ReadPollReply)\n" << std::endl);
    pmbus_debug_log("pmbus_info.pmbus_cmd     : 0x" << std::hex << pmbus_info.pmbus_cmd << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.length        : 0x" << std::hex << pmbus_info.length << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.i2c_address   : 0x" << std::hex << pmbus_info.i2c_address << std::dec <<  std::endl);

    rv = bcm_plp_pmbus_access(bcmMilleniob.c_str(), phy_info, &pmbus_info);
    if (rv) {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusReadRequest failed with rv:" << rv <<  std::endl);
        return rv;
    } else {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusReadRequest success with rv:" << rv <<  std::endl);
    }

    /* if rv == code is BCM_PLP_PMBUS_FORWARD_READ_WAIT fof bcmplpPmbusReadPollReply
     * either application wait or do other operations */
    pmbus_info.pmbus_type = bcmplpPmbusReadPollReply;
    pmbus_debug_log("===============bcm_plp_pmbus_access() ReadPollReply parameters =====================" << std::endl);
    pmbus_debug_log("pmbus_info.pmbus_type    : " << pmbus_info.pmbus_type << " (0:Init 1:WriteReq 2: WriteStat 3: ReadReq 4: ReadPollReply)\n" << std::endl);
    pmbus_debug_log("pmbus_info.pmbus_cmd     : 0x" << std::hex << pmbus_info.pmbus_cmd << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.length        : 0x" << std::hex << pmbus_info.length << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.i2c_address   : 0x" << std::hex << pmbus_info.i2c_address << std::dec <<  std::endl);

    do {
        pmbus_info.pmbus_type = bcmplpPmbusReadPollReply;
        rv = bcm_plp_pmbus_access(bcmMilleniob.c_str(), phy_info, &pmbus_info);
        if ((rv == BCM_PLP_PMBUS_FORWARD_READ_FAILED) || (rv == 0)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        --retry_cnt;
    } while (retry_cnt > 0);

    if ((retry_cnt == 0) || rv != 0) {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusReadPollReply failed with rv:" << rv <<  std::endl);
        return rv;
    } else {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusReadPollReply success with rv:" << rv <<  std::endl);
        pmbus_debug_log("read_data = 0x" << std::hex << (unsigned int)*data << std::dec <<  std::endl);
    }
    return rv;
}

int setAvsPmbusData(const Bcm81725Lane& param, uint32_t op, uint32_t len, uint8_t *data)
{
    pmbus_debug_log(__FILE__ << ": " <<__LINE__ << ": " << __func__ << " ");
    pmbus_debug_log( "bus=" << param.bus << " mdioAddr=" << param.mdioAddr << " side="  << param.side << " laneNum=" << param.laneNum <<  std::endl);

    int rv = 0;
    unsigned int phy_addr;

    if (param.bus == MEZZ_BRD_TOP)
    {
        phy_addr = param.mdioAddr | cTopMzMdioAddrMask;
        pmbus_debug_log("-------------------------- This is TopMz ------------------------------" << std::endl);
    }
    else
    {
        phy_addr = param.mdioAddr;
        pmbus_debug_log("-------------------------- This is BtmMz -------------------------------" << std::endl);
    }

    bcm_plp_access_t phy_info;
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));

    int retry_cnt = 30;
    int i2c_addr  = 0x1e;

    bcm_plp_pmbus_info_t pmbus_info;
    memset(&pmbus_info, 0, sizeof(bcm_plp_pmbus_info_t));

    phy_info.platform_ctxt = (void*)(&param.bus);
    phy_info.phy_addr = phy_addr;
    phy_info.if_side = 0;
    phy_info.lane_map = 0xFFFF;

    pmbus_info.pmbus_type = bcmplpPmbusInit;

    pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusInit ... " << std::endl);

    pmbus_debug_log("===============bcm_plp_pmbus_access() Init parameters =====================" << std::endl);
    pmbus_debug_log("phy_info.phy_addr        : 0x" << std::hex << phy_info.phy_addr << std::dec <<  std::endl);
    pmbus_debug_log("phy_info.if_side         : " << phy_info.if_side <<  std::endl);
    pmbus_debug_log("phy_info.lane_map        : 0x" << std::hex <<  phy_info.lane_map << std::dec <<  std::endl);
    pmbus_debug_log("phy_info.data_path_dir   : " << phy_info.data_path_dir << " (1:Egress 2:Ingress 3: both)\n" << std::endl);

    pmbus_debug_log("pmbus_info.pmbus_type    : " << pmbus_info.pmbus_type << " (0:Init 1:WriteReq 2: WriteStat 3: ReadReq 4: ReadPollReply)\n" << std::endl);

    rv = bcm_plp_pmbus_access(bcmMilleniob.c_str(), phy_info, &pmbus_info);
    if (rv) {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusInit failed with rv:" << rv <<  std::endl);
        return rv;
    } else {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusInit success with rv:" << rv <<  std::endl);
    }

    pmbus_info.pmbus_cmd = op;
    pmbus_info.length = len;
    pmbus_info.i2c_address = i2c_addr;
    pmbus_info.w_data = data;

    pmbus_info.pmbus_type = bcmplpPmbusWriteRequest;

    pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusWriteRequest ... " << std::endl);

    pmbus_debug_log("===============bcm_plp_pmbus_access() Write parameters =====================" << std::endl);

    pmbus_debug_log("pmbus_info.pmbus_type    : " << pmbus_info.pmbus_type << " (0:Init 1:WriteReq 2: WriteStat 3: ReadReq 4: ReadPollReply)\n" << std::endl);
    pmbus_debug_log("pmbus_info.pmbus_cmd     : 0x" << std::hex << pmbus_info.pmbus_cmd << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.length        : 0x" << std::hex << pmbus_info.length << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.i2c_address   : 0x" << std::hex << pmbus_info.i2c_address << std::dec <<  std::endl);

    std::ios_base::fmtflags flags(std::cout.flags());
    pmbus_debug_log("pmbus_info.w_data        : 0x" << std::hex << std::setw(2) << std::setfill('0'));

    for(int i = 0; i < len; i++)
    {
        unsigned int printData = pmbus_info.w_data[i];
        pmbus_debug_log(printData);
    }

    pmbus_debug_log(std::endl);

    std::cout.flags(flags);

    rv = bcm_plp_pmbus_access(bcmMilleniob.c_str(), phy_info, &pmbus_info);
    if (rv) {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusWriteRequest failed with rv:" << rv <<  std::endl);
        return rv;
    } else {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusWriteRequest success with rv:" << rv <<  std::endl);
    }

    /* if rv == code is BCM_PLP_PMBUS_FORWARD_WRITE_WAIT fof bcmplpPmbusWriteStatus
     * either application wait or do other operations */
    pmbus_info.pmbus_type = bcmplpPmbusWriteStatus;
    pmbus_debug_log("===============bcm_plp_pmbus_access() WriteStatus parameters =====================" << std::endl);
    pmbus_debug_log("pmbus_info.pmbus_type    : " << pmbus_info.pmbus_type << " (0:Init 1:WriteReq 2: WriteStat 3: ReadReq 4: ReadPollReply)\n" << std::endl);
    pmbus_debug_log("pmbus_info.pmbus_cmd     : 0x" << std::hex << pmbus_info.pmbus_cmd << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.length        : 0x" << std::hex << pmbus_info.length << std::dec <<  std::endl);
    pmbus_debug_log("pmbus_info.i2c_address   : 0x" << std::hex << pmbus_info.i2c_address << std::dec <<  std::endl);

    do {
        pmbus_info.pmbus_type = bcmplpPmbusWriteStatus;
        //pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusWriteStatus ... " << std::endl);
        rv = bcm_plp_pmbus_access(bcmMilleniob.c_str(), phy_info, &pmbus_info);
        if ((rv == BCM_PLP_PMBUS_FORWARD_WRITE_FAILED) || (rv == 0)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        --retry_cnt;
    } while (retry_cnt > 0);
    if ((retry_cnt == 0) || rv != 0) {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusWriteStatus failed with rv:" << rv <<  std::endl);
        return rv;
    } else {
        pmbus_debug_log("bcm_plp_pmbus_access bcmplpPmbusWriteStatus success with rv:" << rv <<  std::endl);
    }
    return rv;
}

int getAvsPmbusData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData)
{
    if (mezzIdx >= NUM_MEZZ_BRDS)
    {
        //INFN_LOG(SeverityLevel::info) << "Error: mezzIdx out of range. Idx: " << mezzIdx;
        return -1;
    }

    if (bcmIdx >= NUM_BCMS_PER_MEZZ)
    {
        //INFN_LOG(SeverityLevel::info) << "Error: bcmIdx out of range. Idx: " << bcmIdx;
        return -1;
    }

    Bcm81725Lane* pBcmParam = (Bcm81725Lane*)&cParamTbl[mezzIdx][bcmIdx];

    return getAvsPmbusData(*pBcmParam, opCode, len, pData);
}

int setAvsPmbusData(uint32_t mezzIdx, uint32_t bcmIdx, uint32_t opCode, uint32_t len, uint8_t* pData)
{
    if (mezzIdx >= NUM_MEZZ_BRDS)
    {
        //INFN_LOG(SeverityLevel::info) << "Error: mezzIdx out of range. Idx: " << mezzIdx;
        return -1;
    }

    if (bcmIdx >= NUM_BCMS_PER_MEZZ)
    {
        //INFN_LOG(SeverityLevel::info) << "Error: bcmIdx out of range. Idx: " << bcmIdx;
        return -1;
    }

    Bcm81725Lane* pBcmParam = (Bcm81725Lane*)&cParamTbl[mezzIdx][bcmIdx];

    return setAvsPmbusData(*pBcmParam, opCode, len, pData);
}

}

