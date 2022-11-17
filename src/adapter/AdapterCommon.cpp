
/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "AdapterCommon.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include <thread>
#include "DataPlaneMgr.h"

using namespace std;
using namespace DataPlane;

namespace DpAdapter {

const string IdGen::cOdu4AidStr   = "ODU4";
const string IdGen::cOdu4iAidStr   = "ODU4i";
const string IdGen::cOduFlexAidStr = "ODUflexi";


std::string IdGen::genLineAid(uint32 lineId)
{
    // Example: Carrier Aid = 1-4-L2-1

    std::string aid =        DataPlaneMgrSingleton::Instance()->getChassisIdStr() +
                      "-"  + dp_env::DpEnvSingleton::Instance()->getSlotNum()   +
                      "-L" + to_string(lineId) + "-1";

    return aid;
}

std::string IdGen::genOtuAid(uint32 otuId)
{
    // Example: OtuCni Aid = 1-4-L2-1
    // OTU client id is from 1-16.  17-18 is for HO-OTUCni.
    // AID format: 1-5-T1, map to otu id 1; 1-5-T16 map to otu id 16
    std::string aid;
    if ( ( otuId  == HO_OTU_ID_1 ) || ( otuId  == HO_OTU_ID_2 ) )
    {
        otuId -= HO_OTU_ID_OFFSET;
        aid =        DataPlaneMgrSingleton::Instance()->getChassisIdStr() +
                      "-"  + dp_env::DpEnvSingleton::Instance()->getSlotNum()   +
                      "-L" + to_string(otuId) + "-1";
    }
    else
    {
        aid =        DataPlaneMgrSingleton::Instance()->getChassisIdStr() +
                      "-"  + dp_env::DpEnvSingleton::Instance()->getSlotNum()   +
                      "-T" + to_string(otuId);

    }
    return aid;
}

std::string IdGen::genOduAid(uint32 oduId)
{
    // Example: 1-4-L2-1-ODU4i-161

    uint32 lineId = (oduId / 16) + 1;
    uint32 tsNum  = (((oduId % 16) - 1) * 80) + 1;

    std::string strAid = genLineAid(lineId) + "-ODU4i-" + to_string(tsNum);

    return strAid;
}

std::string IdGen::genOduAid(uint32 otuId, const std::vector<uint32_t>& tS, OduPayLoad payLoad)
{
    std::vector<uint32_t>::const_iterator it = std::min_element(tS.begin(), tS.end());
    uint32_t startTs = *it;
    if (otuId > 1)
    {
        startTs = startTs - 32;
    }
    uint32_t tsNum = (startTs - 1) * 20 + 1;

    std::string oduTypeStr;
    if (payLoad == ODUPAYLOAD_LO_FLEXI)
    {
        oduTypeStr = "-ODUflexi-";
    }
    else
    {
        oduTypeStr = "-ODU4i-";
    }

    std::string strAid = genLineAid(otuId) + oduTypeStr + to_string(tsNum);

    return strAid;
}

std::string IdGen::genOduAid(uint32 oduId, uint32 otuId, OduPayLoad payLoad)
{
    // Example 4i:   1-4-L2-1-ODU4i-1
    // Example Flex: 1-4-L2-1-ODUflexi-1

    std::string oduStr;
    uint32 lineIdOffset;
    uint32 baseOffset;
    if (payLoad == ODUPAYLOAD_LO_FLEXI)
    {
        oduStr = cOduFlexAidStr;

        baseOffset   = cOffOduFlex_1;
        lineIdOffset = cOffOduFlex_2;
    }
    else
    {
        oduStr = cOdu4iAidStr;

        baseOffset   = cOffOdu4i_1;
        lineIdOffset = cOffOdu4i_2;
    }

    uint32 offset;
    if (otuId == 1)
    {
        offset = baseOffset;
    }
    else
    {
        offset = lineIdOffset;
    }

    uint32 instId;
    if (oduId < offset)
    {
        INFN_LOG(SeverityLevel::error) << "ERROR. Invalid OduId " << oduId << " for Payload " << payLoad;
        instId = oduId + 99;    // what do do here?
    }
    else
    {
        instId = oduId - offset;
    }

    std::string strAid = genLineAid(otuId) + "-" + oduStr + "-" + to_string(instId);

    return strAid;
}

std::string IdGen::genGigeAid(uint32 portId)
{
    // Example: 1-4-T1

    std::string strAid =        DataPlaneMgrSingleton::Instance()->getChassisIdStr() +
                         "-"  + dp_env::DpEnvSingleton::Instance()->getSlotNum() +
                         "-T" + to_string(portId);

    INFN_LOG(SeverityLevel::debug) << "aid " << strAid;

    return strAid;
}

std::string IdGen::genXconAid(uint32 loOduId, uint32 portId)
{
    // Example: 1-6-L1-1-ODU4i-161,1-6-T4

    // Note: Only Bi-Directional AID support currently
    // Todo: Support Uni-Directional AID formats (how to know the direction and order??)

    std::string strAid = genOduAid(loOduId) + "," + genGigeAid(portId);

    return strAid;
}

std::string IdGen::genXconAid(std::string oduAid, uint32 portId, XconDirection dir)
{
    std::string aid;
    std::string gigeAid(genGigeAid(portId));
    if ((dir == XCONDIRECTION_EGRESS) ||
        (dir == XCONDIRECTION_BI_DIR))
    {
        aid = oduAid + "," + gigeAid;
    }
    else
    {
        aid = gigeAid + "," + oduAid;
    }

    return aid;
}

std::string EnumTranslate::toString(const bool boolVal)
{
    std::string retString = (true == boolVal) ? "TRUE" : "FALSE";
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::OtuSubType type)
{
    std::string retString;
    switch (type)
    {
        case OTUSUBTYPE_LINE:   retString = "LINE";    break;
        case OTUSUBTYPE_CLIENT: retString = "CLIENT";  break;
        default:                retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::OtuPayLoad payLoad)
{
    std::string retString;
    switch (payLoad)
    {
        case OTUPAYLOAD_OTUCNI: retString = "OTUCNI";  break;
        case OTUPAYLOAD_OTU4:   retString = "OTU4";    break;
        default:                retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::OduSubType type)
{
    std::string retString;
    switch (type)
    {
        case ODUSUBTYPE_LINE:   retString = "LINE";    break;
        case ODUSUBTYPE_CLIENT: retString = "CLIENT";  break;
        default:                retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::OduPayLoad payLoad)
{
    std::string retString;
    switch (payLoad)
    {
        case ODUPAYLOAD_ODU4: retString = "ODU4";  break;
        case ODUPAYLOAD_LO_ODU4I:   retString = "LO-ODU4I";    break;
        case ODUPAYLOAD_LO_FLEXI:   retString = "LO-FLEXI";    break;
        case ODUPAYLOAD_HO_ODUCNI:   retString = "HO-ODUCNI";    break;
        case ODUPAYLOAD_LO_ODU4:   retString = "LO-ODU4";    break;
        default:                retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::OduMsiPayLoad Msi)
{
    std::string retString;
    switch (Msi)
    {
        case ODUMSIPAYLOAD_ODU4I_OTU4: retString = "ODU4I-OTU4";  break;
        case ODUMSIPAYLOAD_ODU4I_ODU4: retString = "ODU4I-ODU4";  break;
        case ODUMSIPAYLOAD_ODU4: retString = "ODU4";  break;
        case ODUMSIPAYLOAD_ODU4I_100GE: retString = "ODU4I_100GE";  break;
        case ODUMSIPAYLOAD_ODUFLEXI_25GE: retString = "ODUFLEXI-25GE";  break;
        case ODUMSIPAYLOAD_ODUFLEXI_50GE: retString = "ODUFLEXI-50GE";  break;
        case ODUMSIPAYLOAD_ODUFLEXI_75GE: retString = "ODUFLEXI-75GE";  break;
        case ODUMSIPAYLOAD_ODUFLEXI_100GE: retString = "ODUFLEXI-100GE";  break;
        case ODUMSIPAYLOAD_ODUFLEXI_200GE: retString = "ODUFLEXI-200GE";  break;
        case ODUMSIPAYLOAD_ODUFLEXI_400GE: retString = "ODUFLEXI-400GE";  break;
        default:                retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::LoopBackType lpbk)
{
    std::string retString;
    switch (lpbk)
    {
        case LOOP_BACK_TYPE_UNSPECIFIED: retString = "UNSPECIFIED"; break;
        case LOOP_BACK_TYPE_OFF:         retString = "OFF";         break;
        case LOOP_BACK_TYPE_FACILITY:    retString = "FACILITY";    break;
        case LOOP_BACK_TYPE_TERMINAL:    retString = "TERMINAL";    break;
        default:                         retString = "UNKNOWN";     break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::CarrierChannel channel)
{
    std::string retString;
    switch (channel)
    {
        case CARRIERCHANNEL_UNSPECIFIED: retString = "UNSPECIFIED"; break;
        case CARRIERCHANNEL_ONE:         retString = "ONE";         break;
        case CARRIERCHANNEL_TWO:         retString = "TWO";         break;
        case CARRIERCHANNEL_BOTH:        retString = "BOTH";        break;
        default:                         retString = "UNKNOWN";     break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::ServiceMode srvMode)
{
    std::string retString;
    switch (srvMode)
    {
        case SERVICEMODE_UNSPECIFIED: retString = "UNSPECIFIED"; break;
        case SERVICEMODE_NONE:        retString = "NONE";        break;
        case SERVICEMODE_TRANSPORT:   retString = "TRANSPORT";   break;
        case SERVICEMODE_SWITCHING:   retString = "SWITCHING";   break;
        case SERVICEMODE_ADAPTATION:  retString = "ADAPTATION";  break;
        default:                      retString = "UNKNOWN";     break;
    }
    return retString;
}


std::string EnumTranslate::toString(const DataPlane::InterfaceType intfType)
{
    std::string retString;
    switch (intfType)
    {
        case INTERFACETYPE_UNSPECIFIED: retString = "UNSPECIFIED"; break;
        case INTERFACETYPE_GAUI:        retString = "GAUI";        break;
        case INTERFACETYPE_CAUI_4:      retString = "CAUI4";       break;
        case INTERFACETYPE_LAUI_2:      retString = "LAUI2";       break;
        case INTERFACETYPE_OTL_4_2:     retString = "OTL4.2";      break;
        default:                        retString = "UNKNOWN";     break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::MaintenanceSignal ms)
{
    std::string retString;
    switch (ms)
    {
        case MAINTENANCE_SIGNAL_UNSPECIFIED:  retString = "UNSPECIFIED";  break;
        case MAINTENANCE_SIGNAL_NOREPLACE:    retString = "NOREPLACE";    break;
        case MAINTENANCE_SIGNAL_LASEROFF:     retString = "LASEROFF";     break;
        case MAINTENANCE_SIGNAL_RF:           retString = "RF";           break;
        case MAINTENANCE_SIGNAL_IDLE:         retString = "IDLE";         break;
        case MAINTENANCE_SIGNAL_LF:           retString = "LF";           break;
        case MAINTENANCE_SIGNAL_OCI:          retString = "OCI";          break;
        case MAINTENANCE_SIGNAL_LCK:          retString = "LCK";          break;
        case MAINTENANCE_SIGNAL_AIS:          retString = "AIS";          break;
        case MAINTENANCE_SIGNAL_PRBS_X31:     retString = "PRBS_X31";     break;
        case MAINTENANCE_SIGNAL_PRBS_X31_INV: retString = "PRBS_X31_INV"; break;
        default:                              retString = "UNKNOWN";      break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::PrbsMode prbs)
{
    std::string retString;
    switch (prbs)
    {
        case PRBSMODE_UNSPECIFIED:  retString = "UNSPECIFIED"; break;
        case PRBSMODE_PRBS_NONE:    retString = "NONE";        break;
        case PRBSMODE_PRBS_X31:     retString = "X31";         break;
        case PRBSMODE_PRBS_X31_INV: retString = "X31-INV";     break;
        default:                    retString = "UNKNOWN";     break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::Direction dir)
{
    std::string retString;
    switch (dir)
    {
        case DIRECTION_UNSET: retString = "UNSET";   break;
        case DIRECTION_TX:    retString = "TX";      break;
        case DIRECTION_RX:    retString = "RX";      break;
        default:              retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::EthernetMode ethMode)
{
    std::string retString;
    switch (ethMode)
    {
        case ETHERNETMODE_TRANSPARENT: retString = "TRANSPARENT"; break;
        case ETHERNETMODE_RETIMED:     retString = "RETIMED";     break;
        default:                       retString = "UNKNOWN";     break;
    }
    return retString;
};

std::string EnumTranslate::toString(const DataPlane::ServiceModeQualifier srvModeQual)
{
    std::string retString;
    switch (srvModeQual)
    {
        case SERVICEMODEQUALIFIER_NONE: retString = "NONE";    break;
        default:                        retString = "UNKNOWN"; break;
    }
    return retString;
};

std::string EnumTranslate::toString(const DataPlane::PortRate rate)
{
    std::string retString;
    switch (rate)
    {
        case PORT_RATE_ETH100G: retString = "100GE";   break;
        case PORT_RATE_ETH200G: retString = "200GE";   break;
        case PORT_RATE_ETH400G: retString = "400GE";   break;
        default:                retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::ClientStatus clientStatus)
{
    std::string retString;
    switch (clientStatus)
    {
        case CLIENTSTATUS_UP:      retString = "UP";      break;
        case CLIENTSTATUS_DOWN:    retString = "DOWN";    break;
        case CLIENTSTATUS_FAULTED: retString = "FAULTED"; break;
        default:                   retString = "UNKNOWN"; break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::CarrierState carrierState)
{
    std::string retString;
    switch (carrierState)
    {
        case CARRIERSTATE_ACTIVE:        retString = "CARRIERSTATE_ACTIVE";        break;
        case CARRIERSTATE_STANDBY:       retString = "CARRIERSTATE_STANDBY";       break;
        case CARRIERSTATE_AUTODISCOVERY: retString = "CARRIERSTATE_AUTODISCOVERY"; break;
        default:                         retString = "UNKNOWN";                    break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::ClientMode clientMode)
{
    std::string retString;
    switch (clientMode)
    {
        case CLIENTMODE_LXTP_E: retString = "CLIENTMODE_LXTP_E"; break;
        case CLIENTMODE_LXTP_M: retString = "CLIENTMODE_LXTP_M"; break;
        case CLIENTMODE_LXTP:   retString = "CLIENTMODE_LXTP";   break;
        default:                retString = "UNKNOWN";           break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::CarrierAdvParmStatus state)
{
    std::string retString;
    switch (state)
    {
        case DataPlane::CARRIERADVPARMSTATUS_SET:           retString = "CARRIERADVPARMSTATUS_SET";           break;
        case DataPlane::CARRIERADVPARMSTATUS_UNKNOWN:       retString = "CARRIERADVPARMSTATUS_UNKNOWN";       break;
        case DataPlane::CARRIERADVPARMSTATUS_INPROGRESS:    retString = "CARRIERADVPARMSTATUS_INPROGRESS";    break;
        case DataPlane::CARRIERADVPARMSTATUS_FAILED:        retString = "CARRIERADVPARMSTATUS_FAILED";        break;
        case DataPlane::CARRIERADVPARMSTATUS_NOT_SUPPORTED: retString = "CARRIERADVPARMSTATUS_NOT_SUPPORTED"; break;
        default:                                            retString = "CARRIERSTATE_UNSPECIFIED";           break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::DcoState state)
{
    std::string retString;
    switch(state)
    {
        case DCOSTATE_BRD_INIT:   retString = "DCOSTATE_BRD_INIT";    break;
        case DCOSTATE_DSP_CONFIG: retString = "DCOSTATE_DSP_CONFIG";  break;
        case DCOSTATE_LOW_POWER:  retString = "DCOSTATE_LOW_POWER";   break;
        case DCOSTATE_POWER_UP:   retString = "DCOSTATE_POWER_UP";    break;
        case DCOSTATE_POWER_DOWN: retString = "DCOSTATE_POWER_DOWN";  break;
        case DCOSTATE_FAULTED:    retString = "DCOSTATE_FAULTED";     break;
        default:                  retString = "DCOSTATE_UNSPECIFIED"; break;
    }
    return retString;
};

std::string EnumTranslate::toString(const DataPlane::FwUpgradeState state)
{
    std::string retString;
    switch(state)
    {
        case FWUPGRADESTATE_IDLE:                 retString = "FWUPGRADESTATE_IDLE";                 break;
        case FWUPGRADESTATE_DOWNLOAD_IN_PROGESS:  retString = "FWUPGRADESTATE_DOWNLOAD_IN_PROGESS";  break;
        case FWUPGRADESTATE_DOWNLOAD_COMPLETE:    retString = "FWUPGRADESTATE_DOWNLOAD_COMPLETE";    break;
        case FWUPGRADESTATE_INSTALL_IN_PROGRESS:  retString = "FWUPGRADESTATE_INSTALL_IN_PROGRESS";  break;
        case FWUPGRADESTATE_INSTALL_COMPLETE:     retString = "FWUPGRADESTATE_INSTALL_COMPLETE";     break;
        case FWUPGRADESTATE_ACTIVATE_IN_PROGRESS: retString = "FWUPGRADESTATE_ACTIVATE_IN_PROGRESS"; break;
        case FWUPGRADESTATE_ACTIVATE_COMPLETE:    retString = "FWUPGRADESTATE_ACTIVATE_COMPLETE";    break;
        case FWUPGRADESTATE_ISK_OP_IN_PROGRESS:   retString = "FWUPGRADESTATE_ISK_OP_IN_PROGRESS";   break;
        case FWUPGRADESTATE_ISK_OP_COMPLETE:      retString = "FWUPGRADESTATE_ISK_OP_COMPLETE";      break;
        case FWUPGRADESTATE_DOWNLOAD_FAILED:      retString = "FWUPGRADESTATE_DOWNLOAD_FAILED";      break;
        case FWUPGRADESTATE_INSTALL_FAILED:       retString = "FWUPGRADESTATE_INSTALL_FAILED";       break;
        case FWUPGRADESTATE_ACTIVATE_FAILED:      retString = "FWUPGRADESTATE_ACTIVATE_FAILED";      break;
        case FWUPGRADESTATE_ISK_OP_FAILED:        retString = "FWUPGRADESTATE_ISK_OP_FAILED";        break;
        default:                                  retString = "FWUPGRADESTATE_UNSPECIFIED";          break;
    }
    return retString;
};

std::string EnumTranslate::toString(const BootState state)
{
    std::string retString;
    switch(state)
    {
        case BOOTSTATE_CARD_UP:     retString = "BOOTSTATE_CARD_UP";     break;
        case BOOTSTATE_IN_PROGRESS: retString = "BOOTSTATE_IN_PROGRESS"; break;
        case BOOTSTATE_REJECTED:    retString = "BOOTSTATE_REJECTED";    break;
        default:                    retString = "BOOTSTATE_UNSPECIFIED"; break;
    }
    return retString;
};

std::string EnumTranslate::toString(const DataPlane::XconDirection dir)
{
    std::string retString;
    switch (dir)
    {
        case XCONDIRECTION_UNSPECIFIED: retString = "UNSPECIFIED";   break;
        case XCONDIRECTION_EGRESS:      retString = "EGRESS";        break;
        case XCONDIRECTION_INGRESS:     retString = "INGRESS";       break;
        case XCONDIRECTION_BI_DIR:      retString = "BI_DIR";        break;
        default:                        retString = "UNKNOWN";       break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::XconPayLoadType payload)
{
    std::string retString;
    switch (payload)
    {
        case XCONPAYLOADTYPE_UNSPECIFIED: retString = "UNSPECIFIED";   break;
        case XCONPAYLOADTYPE_100GBE:      retString = "100GBE";        break;
        case XCONPAYLOADTYPE_400GBE:      retString = "400GBE";        break;
        case XCONPAYLOADTYPE_OTU4:        retString = "OTU4";          break;
        case XCONPAYLOADTYPE_100G:        retString = "100G";          break;
        default:                          retString = "UNKNOWN";       break;
    }
    return retString;
}
std::string EnumTranslate::toString(const DataPlane::XconPayLoadTreatment ptr)
{
    std::string retString;
    switch (ptr)
    {
        case XCONPAYLOADTREATMENT_UNSPECIFIED:      retString = "UNSPECIFIED";      break;
        case XCONPAYLOADTREATMENT_TRANSPORT:        retString = "TRANSPORT";        break;
        case XCONPAYLOADTREATMENT_SWITCHING:        retString = "SWITCHING";        break;
        case XCONPAYLOADTREATMENT_TRANSPORT_WO_FEC: retString = "TRANSPORT_WO_FEC"; break;
        case XCONPAYLOADTREATMENT_REGEN:            retString = "REGEN";            break;
        case XCONPAYLOADTREATMENT_REGEN_SWITCHING:  retString = "REGEN_SWITCHING";  break;
        default:                                    retString = "UNKNOWN";          break;
    }
    return retString;
}

std::string EnumTranslate::toString(const DataPlane::OpStatus opstatus)
{
    std::string retString;
    switch (opstatus)
    {
        case OPSTATUS_UNSPECIFIED :       retString = "UNSPECIFIED";       break;
        case OPSTATUS_SUCCESS :           retString = "SUCCESS";           break;
        case OPSTATUS_SDK_EXCEPTION :     retString = "SDK_EXCEPTION";     break;
        case OPSTATUS_INVALID_PARAM :     retString = "INVALID_PARAM";     break;
        case OPSTATUS_OPERATIONAL_ERROR : retString = "OPERATIONAL_ERROR"; break;
        case OPSTATUS_UNDEFINED :         retString = "UNDEFINED";         break;
        default:                          retString = "UNKNOWN";           break;
    }
    return retString;
}

bool AdapterCacheOpStatus::waitForDcoAck(uint64_t transId)
{
    if ( transId == 0 )
    {
        // If we don't want to wait for ACK just set transId to 0
        return true;
    }

    int timeOut = 0;

    while (timeOut < DCO_TRANS_ID_TIMEOUT_MSEC)
    {
        if (transId == transactionId)
        {
            if (opStatus == OPSTATUS_SUCCESS)
            {
                INFN_LOG(SeverityLevel::info) << __func__ << " SUCCESS transId: 0x" << hex << transId << dec << endl;
                return true;
            }
            else
            {
                INFN_LOG(SeverityLevel::error) << __func__ << " DCO Transaction ERROR: " <<  EnumTranslate::toString(opStatus) << "TransID: 0x" << hex  << transId << dec << endl;
                    return false;
            }
        }
        this_thread::sleep_for(std::chrono::milliseconds(DCO_TRANS_ID_DELAY_MSEC));
        timeOut += DCO_TRANS_ID_DELAY_MSEC;
    }
    INFN_LOG(SeverityLevel::error) << __func__ << " ERROR DCO TimeOut on transId: 0x" << hex << transId << dec << endl;
    return false;
}

} /* DpAdapter */
