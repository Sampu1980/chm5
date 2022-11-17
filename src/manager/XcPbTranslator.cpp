/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "XcPbTranslator.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"

#include <iostream>
#include <sstream>

using ::chm6_dataplane::Chm6XCConfig;
using ::chm6_dataplane::Chm6XCState;

using namespace ::std;


namespace DataPlane {

void XcPbTranslator::configPbToAd(const Chm6XCConfig& pbCfg, DpAdapter::XconCommon& adCfg) const
{
    std::string xconAid = pbCfg.base_config().config_id().value();
    INFN_LOG(SeverityLevel::info) << "AID: " << xconAid;

    // Source
    if (pbCfg.hal().has_source())
    {
        std::string source = pbCfg.hal().source().value();
        INFN_LOG(SeverityLevel::info) << "Source: " << source;
        adCfg.source = source;
    }

    if (pbCfg.hal().has_destination())
    {
        std::string destination = pbCfg.hal().destination().value();
        INFN_LOG(SeverityLevel::info) << "Destination: " << destination;
        adCfg.destination = destination;
    }

    if (hal_dataplane::DIRECTION_UNSPECIFIED != pbCfg.hal().xc_direction())
    {
        hal_dataplane::Direction xcDirection = pbCfg.hal().xc_direction();
        INFN_LOG(SeverityLevel::info) << "XCDirection: " << xcDirection;
        switch (xcDirection)
        {
            case hal_dataplane::DIRECTION_ONE_WAY:
                if (isXconDirectionIngress(xconAid))
                {
                    adCfg.direction = DataPlane::XCONDIRECTION_INGRESS;
                }
                else
                {
                    adCfg.direction = DataPlane::XCONDIRECTION_EGRESS;
                }
                break;
            case hal_dataplane::DIRECTION_TWO_WAY:
                adCfg.direction = DataPlane::XCONDIRECTION_BI_DIR;
                break;
            default:
                break;
        }
    }
    else
    {
        // Required by DCO XCON Adapter to create
        if (isXconDirectionIngress(xconAid))
        {
            adCfg.direction = DataPlane::XCONDIRECTION_INGRESS;
        }
        else
        {
            adCfg.direction = DataPlane::XCONDIRECTION_EGRESS;
        }
    }

    if (pbCfg.hal().has_regen_source_client())
    {
        std::string regen_source_client = pbCfg.hal().regen_source_client().value();
        INFN_LOG(SeverityLevel::info) << "regen_source_client: " << regen_source_client;
        adCfg.sourceClient = regen_source_client;
    }

    if (pbCfg.hal().has_regen_destination_client())
    {
        std::string regen_destination_client = pbCfg.hal().regen_destination_client().value();
        INFN_LOG(SeverityLevel::info) << "regen_destination_client: " << regen_destination_client;
        adCfg.destinationClient = regen_destination_client;
    }

    INFN_LOG(SeverityLevel::info) << "payload_treatment: " << pbCfg.hal().payload_treatment();
    switch (pbCfg.hal().payload_treatment())
    {
        case hal_dataplane::PAYLOAD_TREATMENT_REGEN:
            adCfg.payloadTreatment = DataPlane::XCONPAYLOADTREATMENT_REGEN;
            break;
        case hal_dataplane::PAYLOAD_TREATMENT_REGEN_SWITCHING:
            adCfg.payloadTreatment = DataPlane::XCONPAYLOADTREATMENT_REGEN_SWITCHING;
            break;
        case hal_dataplane::PAYLOAD_TREATMENT_TRANSPORT:
            adCfg.payloadTreatment = DataPlane::XCONPAYLOADTREATMENT_TRANSPORT;
            break;
        case hal_dataplane::PAYLOAD_TREATMENT_TRANSPORT_WO_FEC:
            adCfg.payloadTreatment = DataPlane::XCONPAYLOADTREATMENT_TRANSPORT_WO_FEC;
            break;
        default:
            adCfg.payloadTreatment = DataPlane::XCONPAYLOADTREATMENT_TRANSPORT;
            break;
    }

    INFN_LOG(SeverityLevel::info) << "payload type: " << pbCfg.hal().payload_type();
    switch (pbCfg.hal().payload_type())
    {
        case hal_dataplane::XCON_PAYLOAD_TYPE_OTU4:
            adCfg.payload = DataPlane::XCONPAYLOADTYPE_OTU4;
            break;
        case hal_dataplane::XCON_PAYLOAD_TYPE_100GBE_LAN:
            adCfg.payload = DataPlane::XCONPAYLOADTYPE_100GBE;
            break;
        case hal_dataplane::XCON_PAYLOAD_TYPE_400GBE_LAN:
            adCfg.payload = DataPlane::XCONPAYLOADTYPE_400GBE;
            break;
        case hal_dataplane::XCON_PAYLOAD_TYPE_100G:
            adCfg.payload = DataPlane::XCONPAYLOADTYPE_100G;
            break;
        default:
            adCfg.payload = DataPlane::XCONPAYLOADTYPE_100GBE;
            break;
    }
}

void XcPbTranslator::configAdToPb(const DpAdapter::XconCommon& adCfg, Chm6XCState& pbStat) const
{
    std::string configId = pbStat.base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << configId;

    ::infinera::hal::dataplane::v2::XCConfig_Config* pInstallCfg = pbStat.mutable_hal()->mutable_installed_config();

    pInstallCfg->mutable_source()->set_value(adCfg.source);
    pInstallCfg->mutable_destination()->set_value(adCfg.destination);
    pInstallCfg->mutable_regen_source_client()->set_value(adCfg.sourceClient);
    pInstallCfg->mutable_regen_destination_client()->set_value(adCfg.destinationClient);

    if ((adCfg.direction == DataPlane::XCONDIRECTION_INGRESS) ||
        (adCfg.direction == DataPlane::XCONDIRECTION_EGRESS))
    {
        pInstallCfg->set_xc_direction(hal_dataplane::DIRECTION_ONE_WAY);
    }
    else if (adCfg.direction == DataPlane::XCONDIRECTION_BI_DIR)
    {
        pInstallCfg->set_xc_direction(hal_dataplane::DIRECTION_TWO_WAY);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "Unknown direction: " << adCfg.direction;
    }

    switch (adCfg.payloadTreatment)
    {
        case DataPlane::XCONPAYLOADTREATMENT_REGEN:
            pInstallCfg->set_payload_treatment(hal_dataplane::PAYLOAD_TREATMENT_REGEN);
            break;
        case DataPlane::XCONPAYLOADTREATMENT_REGEN_SWITCHING:
            pInstallCfg->set_payload_treatment(hal_dataplane::PAYLOAD_TREATMENT_REGEN_SWITCHING);
            break;
        case DataPlane::XCONPAYLOADTREATMENT_TRANSPORT:
            pInstallCfg->set_payload_treatment(hal_dataplane::PAYLOAD_TREATMENT_TRANSPORT);
            break;
        case DataPlane::XCONPAYLOADTREATMENT_TRANSPORT_WO_FEC:
            pInstallCfg->set_payload_treatment(hal_dataplane::PAYLOAD_TREATMENT_TRANSPORT_WO_FEC);
            break;
        case DataPlane::XCONPAYLOADTREATMENT_SWITCHING:
            pInstallCfg->set_payload_treatment(hal_dataplane::PAYLOAD_TREATMENT_SWITCHING);
            break;
        default:
            pInstallCfg->set_payload_treatment(hal_dataplane::PAYLOAD_TREATMENT_UNSPECIFIED);
            break;
    }

    switch (adCfg.payload)
    {
        case DataPlane::XCONPAYLOADTYPE_OTU4:
            pInstallCfg->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_OTU4);
            break;
        case DataPlane::XCONPAYLOADTYPE_100GBE:
            pInstallCfg->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_100GBE_LAN);
            break;
        case DataPlane::XCONPAYLOADTYPE_400GBE:
            pInstallCfg->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_400GBE_LAN);
            break;
        case DataPlane::XCONPAYLOADTYPE_100G:
            pInstallCfg->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_100G);
            break;
        default:
            pInstallCfg->set_payload_type(hal_dataplane::XCON_PAYLOAD_TYPE_UNSPECIFIED);
            break;
    }
}

bool XcPbTranslator::isXconDirectionIngress(std::string aidStr) const
{
    // Example: aid   1-4-T3,1-4-L1-1            ingress
    //                1-4-T3,1-4-L1-1-ODU4i-8    ingress
    //                1-4-L1-1,1-4-T3            egress
    //                1-4-L1-1-ODU4i-8,1-4-T3    egress

    bool isIngress = true;

    std::string sId;
    std::string aidPort = "-L";
    std::size_t found = aidStr.find(aidPort);
    if (found != std::string::npos)
    {
        std::string commaStr = ",";
        std::string tempStr = aidStr.substr(found + aidPort.length());
        std::size_t commaFound = tempStr.find(commaStr);
        if (commaFound != std::string::npos)
        {
            isIngress = false;
        }
    }

    return (isIngress);
}

std::string XcPbTranslator::idSwapSrcDest(std::string strId)
{
    std::string newId;
    std::size_t found = strId.find(",");
    if (found != std::string::npos)
    {
        string idPreComma = strId.substr(0, found);
        string idPostComma = strId.substr(found + 1);

        newId = idPostComma + "," + idPreComma;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "Invalid ID Format " << strId;

        newId = "BAD_ID";
    }

    return newId;
}

}; // namespace DataPlane
