/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#include <iostream>
#include <sstream>
#include <string>

// turn off the specific warning. Can also use "-Wall"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"

#include "CardPbTranslator.h"
#include "CardFaultIdToPbId.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include <google/protobuf/util/message_differencer.h>

// turn the warnings back on
#pragma GCC diagnostic pop

using google::protobuf::util::MessageDifferencer;

using chm6_common::Chm6DcoConfig;
using chm6_common::Chm6DcoCardState;
using chm6_common::Chm6DcoCardFault;
using chm6_common::Chm6DcoCardPm;
using chm6_dataplane::Chm6GccPm;

using namespace ::std;

#define ELOG INFN_LOG(SeverityLevel::error)
#define ILOG INFN_LOG(SeverityLevel::info)
#define DBG 0 // Set to 1 to enable.
#if DBG
#define DLOG ILOG
#else
#define DLOG INFN_LOG(SeverityLevel::debug)
#endif

namespace DataPlane {

void CardPbTranslator::configPbToAd(const Chm6DcoConfig& pbCfg, DpAdapter::CardConfig& adCfg) const
{
    std::string cardAid = pbCfg.base_state().config_id().value();
    INFN_LOG(SeverityLevel::info) << "Called for AID " << cardAid;
}

void CardPbTranslator::statusAdToPb(const DpAdapter::CardStatus& adStat, Chm6DcoCardState& pbStat) const
{
    // Add DCO Zynq and components as upgradable devices
    if (!adStat.fwVersion.empty())
    {
        pbStat.mutable_dco_zynq_version()->set_value(adStat.fwVersion);

        google::protobuf::Map< std::string, hal_common::UpgradableDeviceType_UpgradableDevice >* deviceMap
              = pbStat.mutable_upgradable_devices()->mutable_upgradable_devices();

        std::string key("DCO-OEC");

        if ((*deviceMap)[key].mutable_sw_version()->value() != adStat.fwVersion)
        {
            (*deviceMap)[key].mutable_device_name()->set_value(key);
            (*deviceMap)[key].mutable_sw_version()->set_value(adStat.fwVersion);
            (*deviceMap)[key].set_fw_update_status(hal_common::FW_UPDATE_STATUS_CURRENT);
        }
#if 0 // Remove "DCO-MCU-TEC" because this component is not field upgradable; and cannot find expected version
        key = "DCO-MCU-TEC";

        std::string strVers = std::to_string(adStat.mcuVer);
        if ((*deviceMap)[key].mutable_sw_version()->value() != strVers)
        {
            (*deviceMap)[key].mutable_device_name()->set_value(key);
            (*deviceMap)[key].mutable_sw_version()->set_value(strVers);
            (*deviceMap)[key].set_fw_update_status(hal_common::FW_UPDATE_STATUS_CURRENT);
        }
#endif
        key = "DCO-OEC-Bootloader";

        if ((*deviceMap)[key].mutable_sw_version()->value() != adStat.bootLoaderVer )
        {
            (*deviceMap)[key].mutable_device_name()->set_value(key);
            (*deviceMap)[key].mutable_sw_version()->set_value(adStat.bootLoaderVer);
            (*deviceMap)[key].set_fw_update_status(hal_common::FW_UPDATE_STATUS_CURRENT);
        }

        key = "DCO-OEC-FPGA";

        std::string strVers = std::to_string(adStat.fpgaVer);
        if ((*deviceMap)[key].mutable_sw_version()->value() != strVers )
        {
            (*deviceMap)[key].mutable_device_name()->set_value(key);
            (*deviceMap)[key].mutable_sw_version()->set_value(strVers);
            (*deviceMap)[key].set_fw_update_status(hal_common::FW_UPDATE_STATUS_CURRENT);
        }
    }

    // Fan control temperature flags
    pbStat.set_alt_temp_oorh((true == adStat.tempHiFanIncrease) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);
    pbStat.set_alt_temp_oorl((true == adStat.tempStableFanDecrease) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE);

    // Card Max Packet Length
    pbStat.mutable_max_packet_length()->set_value(adStat.maxPacketLength);

    // Dco Card client Mode
    hal_common::ClientMode clientMode;
    switch (adStat.activeMode)
    {
        case DataPlane::CLIENTMODE_LXTP_E:
            clientMode = hal_common::CLIENT_MODE_LXTP_E;
            break;
        case DataPlane::CLIENTMODE_LXTP_M:
            clientMode = hal_common::CLIENT_MODE_LXTP_M;
            break;
        case DataPlane::CLIENTMODE_LXTP:
            clientMode = hal_common::CLIENT_MODE_LXTP;
            break;
        default:
            clientMode = hal_common::CLIENT_MODE_UNSPECIFIED;
            break;
    }
    pbStat.set_dco_client_mode(clientMode);
}

void CardPbTranslator::capaAdToPb(const DpAdapter::CardCapability& adCapa, chm6_dataplane::DcoCapabilityState& pbCapa) const
{
    if ( (adCapa.supportedClientRate.size() != 0)
      || (adCapa.supportedLineMode.size() != 0)
      || (adCapa.supportedAp.size() != 0) )
    {
        hal_chm6::DcoCapabilities* capa = pbCapa.mutable_hal()->mutable_dco_capabilities();

        if (adCapa.supportedClientRate.size() != 0)
        {
            capa->clear_supported_clients_map();

            for (auto& adClient : adCapa.supportedClientRate)
            {
                std::string name("UNSPECIFIED");
                hal_common::BandWidth bw = hal_common::BAND_WIDTH_UNSPECIFIED;

                switch (adClient)
                {
                    case DataPlane::PORT_RATE_ETH100G:
                        bw = hal_common::BAND_WIDTH_100GB_ELAN;
                        name = std::string("100GB_ELAN");
                        break;
                    case DataPlane::PORT_RATE_ETH400G:
                        bw = hal_common::BAND_WIDTH_400GB_ELAN;
                        name = std::string("400GB_ELAN");
                        break;
                    default:
                        break;
                }

                google::protobuf::Map< std::string, hal_chm6::DcoCapabilities_SupportedClient >* pClientMap =
                        capa->mutable_supported_clients_map();

                (*pClientMap)[name].mutable_name()->set_value(name);
                (*pClientMap)[name].set_band_width(bw);
            }
        }

        if (adCapa.supportedLineMode.size() != 0)
        {
            capa->clear_supported_line_modes_map();

            for (auto& adLine : adCapa.supportedLineMode)
            {
                hal_common::ClientMode cm = hal_common::CLIENT_MODE_UNSPECIFIED;

                switch(adLine.clientMode)
                {
                    case DataPlane::CLIENTMODE_LXTP_E:
                        cm = hal_common::CLIENT_MODE_LXTP_E;
                        break;
                    case DataPlane::CLIENTMODE_LXTP_M:
                        cm = hal_common::CLIENT_MODE_LXTP_M;
                        break;
                    case DataPlane::CLIENTMODE_LXTP:
                        cm = hal_common::CLIENT_MODE_LXTP;
                        break;
                    default:
                        break;
                }

                hal_common::CarrierModeStatus cms = hal_common::CARRIER_MODE_STATUS_UNSPECIFIED;

                switch(adLine.carrierModeStatus)
                {
                case DataPlane::CARRIERMODESTATUS_SUPPORTED:
                    cms = hal_common::CARRIER_MODE_STATUS_SUPPORTED;
                    break;

                case DataPlane::CARRIERMODESTATUS_CANDIDATE:
                    cms = hal_common::CARRIER_MODE_STATUS_CANDIDATE;
                    break;

                case DataPlane::CARRIERMODESTATUS_EXPERIMENTAL:
                    cms = hal_common::CARRIER_MODE_STATUS_EXPERIMENTAL;
                    break;

                case DataPlane::CARRIERMODESTATUS_DEPRECATED:
                    cms = hal_common::CARRIER_MODE_STATUS_DEPRECATED;
                    break;

                default:
                    break;
                }

                google::protobuf::Map< std::string, hal_chm6::DcoCapabilities_SupportedLineMode >* pLineMap
                = capa->mutable_supported_line_modes_map();

                std::string key = adLine.carrierModeDesignation;

                (*pLineMap)[key].mutable_application_code()->set_value(adLine.primaryAppCode);
                (*pLineMap)[key].set_client_mode(cm);
                (*pLineMap)[key].mutable_capacity()->set_value(adLine.dataRate);
                (*pLineMap)[key].mutable_line_baud_rate()->set_value(adLine.baudRate);
                (*pLineMap)[key].mutable_carrier_mode()->set_value(adLine.carrierModeDesignation);
                (*pLineMap)[key].mutable_compatibility_id()->set_value(adLine.compatabilityId);
                (*pLineMap)[key].mutable_max_dco_power()->set_value(adLine.maxDcoPower);
                (*pLineMap)[key].set_carrier_mode_status(cms);
            }
        }

        if (adCapa.supportedAp.size() != 0)
        {
            capa->clear_supported_adv_parameters_map();

            for (auto& adAdvp : adCapa.supportedAp)
            {
                hal_common::AdvParmDirection dir = hal_common::ADV_PARM_DIRECTION_UNSPECIFIED;

                switch (adAdvp.dir)
                {
                    case DataPlane::ADVPARMDIRECTION_TX:
                        dir = hal_common::ADV_PARM_DIRECTION_TX;
                        break;

                    case DataPlane::ADVPARMDIRECTION_RX:
                        dir = hal_common::ADV_PARM_DIRECTION_RX;
                        break;

                    case DataPlane::ADVPARMDIRECTION_BI_DIR:
                        dir = hal_common::ADV_PARM_DIRECTION_BI_DIR;
                        break;

                    default:
                        break;
                }

                hal_common::AdvParmConfigImpact cfi = hal_common::ADV_PARM_CONFIG_IMPACT_UNSPECIFIED;

                switch (adAdvp.cfgImpact)
                {
                    case DataPlane::ADVPARMCONFIGIMPACT_NO_CHANGE:
                        cfi = hal_common::ADV_PARM_CONFIG_IMPACT_NO_CHANGE;
                        break;

                    case DataPlane::ADVPARMCONFIGIMPACT_NO_RE_ACQUIRE:
                        cfi = hal_common::ADV_PARM_CONFIG_IMPACT_NO_RE_ACQUIRE;
                        break;

                    case DataPlane::ADVPARMCONFIGIMPACT_RE_ACQUIRE:
                        cfi = hal_common::ADV_PARM_CONFIG_IMPACT_RE_ACQUIRE;
                        break;

                    case DataPlane::ADVPARMCONFIGIMPACT_FULL_CONFIG_PLL_CHANGE:
                        cfi = hal_common::ADV_PARM_CONFIG_IMPACT_FULL_CONFIG_PLL_CHANGE;
                        break;

                    case DataPlane::ADVPARMCONFIGIMPACT_FULL_CONFIG_NO_PLL_CHANGE:
                        cfi = hal_common::ADV_PARM_CONFIG_IMPACT_FULL_CONFIG_NO_PLL_CHANGE;
                        break;

                    default:
                        break;
                }

                hal_common::AdvParmServiceImpact si = hal_common::ADV_PARM_SERVICE_IMPACT_UNSPECIFIED;

                switch (adAdvp.servImpact)
                {
                    case DataPlane::ADVPARMSERVICEIMPACT_SERVICE_EFFECTING:
                        si = hal_common::ADV_PARM_SERVICE_IMPACT_SERVICE_EFFECTING;
                        break;

                    case DataPlane::ADVPARMSERVICEIMPACT_NON_SERVICE_EFFECTING:
                        si = hal_common::ADV_PARM_SERVICE_IMPACT_NON_SERVICE_EFFECTING;
                        break;

                    default:
                        break;
                }

                google::protobuf::Map< std::string, hal_chm6::DcoCapabilities_SupportedAdvancedParameter >* pAdvpMap =
                        capa->mutable_supported_adv_parameters_map();

                (*pAdvpMap)[adAdvp.name].mutable_name()->set_value(adAdvp.name);
                (*pAdvpMap)[adAdvp.name].mutable_data_type()->set_value(adAdvp.dataType);
                (*pAdvpMap)[adAdvp.name].mutable_value()->set_value(adAdvp.value);
                (*pAdvpMap)[adAdvp.name].set_direction(dir);
                (*pAdvpMap)[adAdvp.name].mutable_multiplicity()->set_value(adAdvp.multiplicity);
                (*pAdvpMap)[adAdvp.name].set_cfg_impact(cfi);
                (*pAdvpMap)[adAdvp.name].set_serv_impact(si);
                (*pAdvpMap)[adAdvp.name].mutable_description()->set_value(adAdvp.description);
            }
        }
    }
}

// Macro adds a BCM Link Fault to the Card.
//
//
#define ADD_BCM_FAULT( BCMPORT, FAULT, CRCERR, PBFAULT )            \
  for( auto bcmFault : BCMPORT.second )                             \
  {                                                                 \
      const string fault = bcmFault.first;                          \
      const bool isFaulted = bcmFault.second;                       \
                                                                    \
      if( fault == "BCM_FAULT_LINK_DOWN" )                          \
      {                                                             \
          addCardFaultPairPCP( cardFaultParamsToString.at( FAULT ),    \
                            FAULTDIRECTION_NA, FAULTLOCATION_NA,    \
                            isFaulted, PBFAULT );                   \
          DLOG << "Added Fault = "                                  \
               << fault << ": "                                     \
               << FAULT << " = " << isFaulted;                      \
      }                                                             \
      else                                                          \
      {                                                             \
          addCardFaultPairPCP( cardFaultParamsToString.at( CRCERR ),   \
                            FAULTDIRECTION_NA, FAULTLOCATION_NA,    \
                            isFaulted, PBFAULT );                   \
          DLOG << "Added Fault = "                                  \
               << fault << ": "                                     \
               << CRCERR << " = " << isFaulted;                     \
      }                                                             \
  }                                                                 \

void
CardPbTranslator::faultPCPAdToPb( chm6_common::Chm6DcoCardFault& pbFault,
                                  const FaultCacheMap& faultMap ) const
{
#if DBG
    for( auto e : faultMap )
    {
        ILOG << e.first;
        for( auto ee : e.second )
            ILOG << "  " << ee.first << ", " << ee.second;
    }
#endif

    // Process BCM Link Faults
    //
    for( auto bcmPort : faultMap )
    {
        const string strPort = bcmPort.first;

        DLOG << "LinkPort: " << strPort;

        // dco_zynq_pl = 9;
        // dco_zynq_ps = 10;
        // dco_nxp = 11;
        //
        // 0 = CARD_FAULT_DCO_LINE_ADC_PLL_LOL,
        // 1 = CARD_FAULT_DCO_LINE_DAC_PLL_LOL,
        // 2 = CARD_FAULT_DCO_CLEANUP_PLL_INPUT_LOL,
        // 3 = CARD_FAULT_DCO_PROC_UNRESPONSIVE,
        // 4 = CARD_FAULT_DCO_REBOOT_WARM,
        // 5 = CARD_FAULT_DCO_REBOOT_COLD,
        // 6 = CARD_FAULT_DCO_ZYNQ_PS_LINK_DOWN,
        // 7 = CARD_FAULT_DCO_ZYNQ_PL_LINK_DOWN,
        // 8 = CARD_FAULT_DCO_NXP_PL_LINK_DOWN,
        // 9 = CARD_FAULT_DCO_ZYNQ_PS_LINK_CRC_ERRORS,
        // 10 = CARD_FAULT_DCO_ZYNQ_PL_LINK_CRC_ERRORS,
        // 11 = CARD_FAULT_DCO_NXP_PL_LINK_CRC_ERRORS,
        //
        if( "9" == strPort )
        {
            // ZYNQ PL Type
            ADD_BCM_FAULT( bcmPort,
                           CARD_FAULT_DCO_ZYNQ_PL_LINK_DOWN,
                           CARD_FAULT_DCO_ZYNQ_PL_LINK_CRC_ERRORS,
                           pbFault )
        }
        else if ( "10" == strPort )
        {
            // ZYNQ PS Type
            ADD_BCM_FAULT( bcmPort,
                           CARD_FAULT_DCO_ZYNQ_PS_LINK_DOWN,
                           CARD_FAULT_DCO_ZYNQ_PS_LINK_CRC_ERRORS,
                           pbFault )
        }
        else // ( "11" == strPort )
        {
            // NXP DCO Type
            ADD_BCM_FAULT( bcmPort,
                           CARD_FAULT_DCO_NXP_PL_LINK_DOWN,
                           CARD_FAULT_DCO_NXP_PL_LINK_CRC_ERRORS,
                           pbFault )
        }
    }

    // Suppress CARD_FAULT_DCO_ZYNQ_PS_LINK_DOWN - Dco Connection Fault (comm failure) during DCO reboot
    google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* pbMap = pbFault.mutable_hal()->mutable_fault();

    std::string warmRebootKey = std::string(cardFaultParamsToString.at(CARD_FAULT_DCO_REBOOT_WARM))
                                + "-" + toString(FAULTDIRECTION_NA)
                                + "-" + toString(FAULTLOCATION_NA);

    std::string coldRebootKey = std::string(cardFaultParamsToString.at(CARD_FAULT_DCO_REBOOT_COLD))
                                + "-" + toString(FAULTDIRECTION_NA)
                                + "-" + toString(FAULTLOCATION_NA);

    if ( ((*pbMap)[warmRebootKey].fault_value() == wrapper::BOOL_TRUE)
      || ((*pbMap)[coldRebootKey].fault_value() == wrapper::BOOL_TRUE) )
    {
        std::string ZynqPsLinkDownKey = std::string(cardFaultParamsToString.at(CARD_FAULT_DCO_ZYNQ_PS_LINK_DOWN))
                                        + "-" + toString(FAULTDIRECTION_NA)
                                        + "-" + toString(FAULTLOCATION_NA);

        std::string ZynqPlLinkDownKey = std::string(cardFaultParamsToString.at(CARD_FAULT_DCO_ZYNQ_PL_LINK_DOWN))
                                        + "-" + toString(FAULTDIRECTION_NA)
                                        + "-" + toString(FAULTLOCATION_NA);

        std::string NxpPlLinkDownKey = std::string(cardFaultParamsToString.at(CARD_FAULT_DCO_NXP_PL_LINK_DOWN))
                                        + "-" + toString(FAULTDIRECTION_NA)
                                        + "-" + toString(FAULTLOCATION_NA);

        (*pbMap)[ZynqPsLinkDownKey].set_fault_value(wrapper::BOOL_FALSE);
        (*pbMap)[ZynqPsLinkDownKey].mutable_value()->set_value(false);
        (*pbMap)[ZynqPlLinkDownKey].set_fault_value(wrapper::BOOL_FALSE);
        (*pbMap)[ZynqPlLinkDownKey].mutable_value()->set_value(false);
        (*pbMap)[NxpPlLinkDownKey].set_fault_value(wrapper::BOOL_FALSE);
        (*pbMap)[NxpPlLinkDownKey].mutable_value()->set_value(false);
    }
}

void
CardPbTranslator::faultAdToPb( const DpAdapter::CardFault& adFault,
                               chm6_common::Chm6DcoCardFault& pbFault,
                               bool isConnectFault ) const
{
    // Clear all faults
    pbFault.clear_hal();

    eqptFaultAdToPb(adFault.eqpt,    pbFault);
    eqptFaultAdToPb(adFault.dacEqpt, pbFault);
    eqptFaultAdToPb(adFault.psEqpt,  pbFault);

    postFaultAdToPb(adFault.post,    pbFault);
    postFaultAdToPb(adFault.dacPost, pbFault);
    postFaultAdToPb(adFault.psPost,  pbFault);

    bool isWarmReboot = false;
    bool isColdReboot = false;
    bool isConnectFlt = false;

    // Dco reboot alarm
    if (adFault.rebootState == DpAdapter::DCO_CARD_REBOOT_WARM)
    {
        isWarmReboot = true;
    }
    else if (adFault.rebootState == DpAdapter::DCO_CARD_REBOOT_COLD)
    {
        isColdReboot = true;
    }
    else
    {
        isConnectFlt = isConnectFault;
    }

    addCardFaultPair(cardFaultParamsToString.at(CARD_FAULT_DCO_REBOOT_WARM), FAULTDIRECTION_NA, FAULTLOCATION_NA, isWarmReboot, pbFault);
    addCardFaultPair(cardFaultParamsToString.at(CARD_FAULT_DCO_REBOOT_COLD), FAULTDIRECTION_NA, FAULTLOCATION_NA, isColdReboot, pbFault);

    // Suppress Dco Connection Fault (comm failure)
    if (isWarmReboot || isColdReboot)
    {
        addCardFaultPair(cardFaultParamsToString.at(CARD_FAULT_DCO_ZYNQ_PS_LINK_DOWN), FAULTDIRECTION_NA, FAULTLOCATION_NA, false, pbFault);
        addCardFaultPair(cardFaultParamsToString.at(CARD_FAULT_DCO_ZYNQ_PL_LINK_DOWN), FAULTDIRECTION_NA, FAULTLOCATION_NA, false, pbFault);
        addCardFaultPair(cardFaultParamsToString.at(CARD_FAULT_DCO_NXP_PL_LINK_DOWN), FAULTDIRECTION_NA, FAULTLOCATION_NA, false, pbFault);
    }

    addCardFaultPair(
        cardFaultParamsToString.at(CARD_FAULT_DCO_PROC_UNRESPONSIVE),
        FAULTDIRECTION_NA,
        FAULTLOCATION_NA,
        isConnectFlt,
        pbFault);
}

void CardPbTranslator::addCardFaultPairPCP( std::string faultName,
                                            FaultDirection faultDirection,
                                            FaultLocation faultLocation,
                                            bool value,
                                            Chm6DcoCardFault& pbFault) const
{
    DLOG << "faultName = " << faultName << ", value = " << value;

    google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* faultMap
        = pbFault.mutable_hal()->mutable_fault();

    std::string faultKey(faultName);
    faultKey.append("-");
    faultKey.append(toString(faultDirection));
    faultKey.append("-");
    faultKey.append(toString(faultLocation));

    DLOG << "faultKey = " << faultKey;

    hal_common::FaultType_FaultDataType faultData;
    faultData.mutable_fault_name()->set_value(faultName);
    faultData.mutable_value()->set_value(value);

    wrapper::Bool faultValue = (value == true) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE;
    faultData.set_fault_value(faultValue);

    DLOG << "faultValue = " << faultValue;

    switch (faultDirection)
    {
        case FAULTDIRECTION_NA:
            faultData.set_direction(hal_common::DIRECTION_NA);
            break;
        case FAULTDIRECTION_RX:
            faultData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case FAULTDIRECTION_TX:
            faultData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            faultData.set_direction(hal_common::DIRECTION_UNSPECIFIED);
            break;
    }

    switch (faultLocation)
    {
        case FAULTLOCATION_NA:
            faultData.set_location(hal_common::LOCATION_NA);
            break;
        case FAULTLOCATION_NEAR_END:
            faultData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case FAULTLOCATION_FAR_END:
            faultData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            faultData.set_location(hal_common::LOCATION_UNSPECIFIED);
            break;
    }

    (*faultMap)[faultKey] = faultData;
}

void CardPbTranslator::addCardFaultPair(std::string faultName, FaultDirection faultDirection, FaultLocation faultLocation, bool value, Chm6DcoCardFault& pbFault) const
{
    google::protobuf::Map<std::string, hal_common::FaultType_FaultDataType>* faultMap = pbFault.mutable_hal()->mutable_fault();

    std::string faultKey(faultName);
    faultKey.append("-");
    faultKey.append(toString(faultDirection));
    faultKey.append("-");
    faultKey.append(toString(faultLocation));


    if ( (*faultMap)[faultKey].has_fault_name()
      && ((*faultMap)[faultKey].fault_value() == wrapper::BOOL_TRUE) )
    {
        return;
    }

    hal_common::FaultType_FaultDataType faultData;
    faultData.mutable_fault_name()->set_value(faultName);
    faultData.mutable_value()->set_value(value);

    wrapper::Bool faultValue = (value == true) ? wrapper::BOOL_TRUE : wrapper::BOOL_FALSE;
    faultData.set_fault_value(faultValue);

    switch (faultDirection)
    {
        case FAULTDIRECTION_NA:
            faultData.set_direction(hal_common::DIRECTION_NA);
            break;
        case FAULTDIRECTION_RX:
            faultData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case FAULTDIRECTION_TX:
            faultData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            faultData.set_direction(hal_common::DIRECTION_UNSPECIFIED);
            break;
    }

    switch (faultLocation)
    {
        case FAULTLOCATION_NA:
            faultData.set_location(hal_common::LOCATION_NA);
            break;
        case FAULTLOCATION_NEAR_END:
            faultData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case FAULTLOCATION_FAR_END:
            faultData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            faultData.set_location(hal_common::LOCATION_UNSPECIFIED);
            break;
    }

    (*faultMap)[faultKey] = faultData;
}

void CardPbTranslator::pmAdToPb(const DataPlane::DpMsCardPm& adPm, Chm6DcoCardPm& pbPm) const
{
    pbPm.clear_dco_pm();

    addCardPmPair(CARD_PM_DSP_TEMPERATURE , PMDIRECTION_NA , PMLOCATION_NA, adPm.SystemState.DspTemperature       , pbPm);
    addCardPmPair(CARD_PM_PIC_TEMPERATURE , PMDIRECTION_NA , PMLOCATION_NA, adPm.SystemState.PicTemperature       , pbPm);
    addCardPmPair(CARD_PM_CASE_TEMPERATURE, PMDIRECTION_NA , PMLOCATION_NA, adPm.SystemState.ModuleCaseTemperature, pbPm);
}

void CardPbTranslator::addCardPmPair(CARD_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, double value, Chm6DcoCardPm& pbPm) const
{
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pbPm.mutable_dco_pm()->mutable_pm();

    std::string pmKey(cardPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(cardPmParamsToString.at(pmParam));

    hal_common::PmType_PmDataType pmData;
    pmData.mutable_pm_data_name()->set_value(pmName);
    pmData.mutable_double_val()->set_value(value);

    switch (pmDirection)
    {
        case PMDIRECTION_NA:
            pmData.set_direction(hal_common::DIRECTION_NA);
            break;
        case PMDIRECTION_RX:
            pmData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case PMDIRECTION_TX:
            pmData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (pmLocation)
    {
        case PMLOCATION_NA:
            pmData.set_location(hal_common::LOCATION_NA);
            break;
        case PMLOCATION_NEAR_END:
            pmData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case PMLOCATION_FAR_END:
            pmData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    google::protobuf::MapPair<std::string, hal_common::PmType_PmDataType> pmPair(pmKey, pmData);
    pmMap->insert(pmPair);
}

void CardPbTranslator::pmAdToPb(const DataPlane::DpMsCardPm& adPm, Chm6DcoCardState& pbStat) const
{
    pbStat.mutable_aid()->set_value(std::to_string(adPm.name));
    pbStat.mutable_pm()->mutable_dsp_temperature()->set_value(adPm.SystemState.DspTemperature);
    pbStat.mutable_pm()->mutable_pic_temperature()->set_value(adPm.SystemState.PicTemperature);
    pbStat.mutable_pm()->mutable_module_case_temperature()->set_value(adPm.SystemState.ModuleCaseTemperature);
}

void CardPbTranslator::addGccPmPair(GCC_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, uint64_t value, Chm6GccPm& pbPm) const
{
    hal_dataplane::GccPm_OperationalPm* pmHal = pbPm.mutable_hal();
    google::protobuf::Map<std::string, hal_common::PmType_PmDataType>* pmMap = pmHal->mutable_pm()->mutable_pm();

    std::string pmKey(gccPmParamsToString.at(pmParam));
    pmKey.append("-");
    pmKey.append(toString(pmDirection));
    pmKey.append("-");
    pmKey.append(toString(pmLocation));

    std::string pmName(gccPmParamsToString.at(pmParam));

    hal_common::PmType_PmDataType pmData;
    pmData.mutable_pm_data_name()->set_value(pmName);
    pmData.mutable_uint64_val()->set_value(value);

    switch (pmDirection)
    {
        case PMDIRECTION_NA:
            pmData.set_direction(hal_common::DIRECTION_NA);
            break;
        case PMDIRECTION_RX:
            pmData.set_direction(hal_common::DIRECTION_INGRESS);
            break;
        case PMDIRECTION_TX:
            pmData.set_direction(hal_common::DIRECTION_EGRESS);
            break;
        default:
            break;
    }

    switch (pmLocation)
    {
        case PMLOCATION_NA:
            pmData.set_location(hal_common::LOCATION_NA);
            break;
        case PMLOCATION_NEAR_END:
            pmData.set_location(hal_common::LOCATION_NEAR_END);
            break;
        case PMLOCATION_FAR_END:
            pmData.set_location(hal_common::LOCATION_FAR_END);
            break;
        default:
            break;
    }

    google::protobuf::MapPair<std::string, hal_common::PmType_PmDataType> pmPair(pmKey, pmData);
    pmMap->insert(pmPair);
}

bool CardPbTranslator::isPbChanged(const chm6_common::Chm6DcoCardState& pbStatPrev, const chm6_common::Chm6DcoCardState& pbStatCur)
{
    if(false == MessageDifferencer::Equals(pbStatPrev, pbStatCur))
    {
        return true;
    }

    if (  (pbStatPrev.dco_zynq_version().value() != pbStatCur.dco_zynq_version().value())
       || (pbStatPrev.sync_ready()               != pbStatCur.sync_ready())
       || (pbStatPrev.alt_temp_oorh()            != pbStatCur.alt_temp_oorh())
       || (pbStatPrev.alt_temp_oorl()            != pbStatCur.alt_temp_oorl())          )
    {
        return true;
    }

    return false;
}

bool CardPbTranslator::isAdChanged(const DpAdapter::CardFault& adFaultPrev, const DpAdapter::CardFault& adFaultCur)
{
    if (  (adFaultPrev.eqptBmp != adFaultCur.eqptBmp)
       || (adFaultPrev.dacEqptBmp != adFaultCur.dacEqptBmp)
       || (adFaultPrev.psEqptBmp != adFaultCur.psEqptBmp)
       || (adFaultPrev.postBmp != adFaultCur.postBmp)
       || (adFaultPrev.dacPostBmp != adFaultCur.dacPostBmp)
       || (adFaultPrev.psPostBmp != adFaultCur.psPostBmp)
       || (adFaultPrev.rebootState != adFaultCur.rebootState) )
    {
        return true;
    }

    return false;
}


bool CardPbTranslator::isDcoStateReady(const DcoState dcoState) const
{
    bool ready;

    switch (dcoState)
    {
        case DCOSTATE_UNSPECIFIED:
        case DCOSTATE_BRD_INIT:
        // case DCOSTATE_FAULTED:
        // TODO  Do we want to send down config if DCO is faulted?
            ready = false;
            break;

        case DCOSTATE_DSP_CONFIG:
        case DCOSTATE_LOW_POWER:
        case DCOSTATE_POWER_UP:
        case DCOSTATE_POWER_DOWN:
        case DCOSTATE_FAULTED:
        default:
            ready = true;
            break;
    }

    return ready;
}

bool CardPbTranslator::isDcoStateLowPower(const DcoState dcoState) const
{
    if (dcoState == DCOSTATE_LOW_POWER)
    {
        return true;
    }

    return false;
}

void CardPbTranslator::eqptFaultAdToPb( const std::vector<DpAdapter::AdapterFault>& adFault,
                                        chm6_common::Chm6DcoCardFault& pbFault) const
{
    for ( auto vFault = adFault.begin();
          vFault != adFault.end();
          vFault++ )
    {
        auto it = dcoCardEqptFaultIdToPb.find(vFault->faultName);
        if (it == dcoCardEqptFaultIdToPb.end())
        {
            DLOG << "DCO eqpt fault " << vFault->faultName
                 << " CANNOT be found in ID matching table.";
        }
        else
        {
            addCardFaultPair( (*it).second,
                              vFault->direction,
                              vFault->location,
                              vFault->faulted,
                              pbFault);
        }
    }
}

void CardPbTranslator::postFaultAdToPb( const std::vector<DpAdapter::AdapterFault>& adFault,
                                        chm6_common::Chm6DcoCardFault& pbFault) const
{
    for ( auto vFault = adFault.begin();
          vFault != adFault.end();
          vFault++ )
    {
        auto it = dcoCardPostFaultIdToPb.find(vFault->faultName);
        if (it == dcoCardPostFaultIdToPb.end())
        {
            DLOG << "DCO post fault " << vFault->faultName
                 << " CANNOT be found in ID matching table.";
        }
        else
        {
            addCardFaultPair( (*it).second,
                              vFault->direction,
                              vFault->location,
                              vFault->faulted,
                              pbFault );
        }
    }
}

}; // namespace DataPlane
