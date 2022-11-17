
/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

#include <google/protobuf/text_format.h>
#include "DcoOduAdapter.h"
#include "DcoXconAdapter.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "DcoConnectHandler.h"
#include "dp_common.h"
#include "DcoPmSBAdapter.h"

using namespace std;
using std::make_shared;
using std::make_unique;

using namespace chrono;
using namespace google::protobuf;
using namespace GnmiClient;
using namespace dcoyang::infinera_dco_xcon;

using xconConfig = Xcons_Xcon_Config;
using xconState = Xcons_Xcon_State;


namespace DpAdapter {

AdapterCacheOpStatus DcoXconAdapter::mOpStatus[MAX_XCON_ID];
bool DcoXconAdapter::mXconOpStatusSubEnable = false;

DcoXconAdapter::DcoXconAdapter()
{
    mspSimCrud = DpSim::SimSbGnmiClient::getInstance();

    std::string serverAddrAndPort = GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoZynq());
    INFN_LOG(SeverityLevel::info) << "Gnmi Server Address = " << serverAddrAndPort;
    mspSbCrud = std::dynamic_pointer_cast<GnmiClient::SBGnmiClient>(DcoConnectHandlerSingleton::Instance()->getCrudPtr(DCC_ZYNQ));

    setSimModeEn(dp_env::DpEnvSingleton::Instance()->isSimEnv());

    if (mspCrud == NULL)
    {
        INFN_LOG(SeverityLevel::error) <<  __func__ << "Crud is NULL" << '\n';
    }
}

DcoXconAdapter::~DcoXconAdapter()
 {

 }

bool DcoXconAdapter::createXcon(string aid, XconCommon *cfg)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	bool status = false;

    int xconId = aidToXconId(aid);

    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon Id: " << xconId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Create xconId: " << xconId << endl;

    Xcon xcon;

    if (setKey(&xcon, xconId) == false)
    {
        return false;
    }

    uint32_t clientId, hoOtuId, loOduId;
    // XCON command come from DP Mgr else it is from CLI command
    if (cfg->source.empty() == false && cfg->destination.empty() == false)
    {
        string sId = cfg->source;
        if (getClientId(cfg->source, &clientId) == false)
        {
            if (getClientId(cfg->destination, &clientId) == false)
            {
                return false;
            }
        }
        else
        {
            sId = cfg->destination;
        }

        if (getOtnId(sId, &hoOtuId, &loOduId) == false)
        {
            return false;
        }
    }
    else
    {
        // From CLI
        clientId = cfg->clientId;
        hoOtuId = cfg->hoOtuId;
        loOduId = cfg->loOduId;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " clientId: " << clientId << " hoOtuId: " << hoOtuId << ", loOduId: " << loOduId << endl;

    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_name()->set_value(aid);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_ho_otu_id()->set_value(hoOtuId);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_lo_odu_id()->set_value(loOduId);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_client_id()->set_value(clientId);

    Xcons_Xcon_Config_Direction dir;
    switch(cfg->direction)
    {
    case XCONDIRECTION_INGRESS:
        dir = xconConfig::DIRECTION_INGRESS;
        break;
    case XCONDIRECTION_EGRESS:
        dir = xconConfig::DIRECTION_EGRESS;
        break;
    case XCONDIRECTION_BI_DIR:
        dir = xconConfig::DIRECTION_BIDIRECTIONAL;
        break;
    default:
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon Direction: " << cfg->direction << '\n';
        return false;
    }
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->set_direction(dir);

    Xcons_Xcon_Config_PayloadType payload;
    switch (cfg->payload)
    {
        case XCONPAYLOADTYPE_100GBE:
            payload = xconConfig::PAYLOADTYPE_100GB_ELAN;
            break;
        case XCONPAYLOADTYPE_400GBE:
            payload = xconConfig::PAYLOADTYPE_400GB_ELAN;
            break;
        case XCONPAYLOADTYPE_OTU4:
            payload = xconConfig::PAYLOADTYPE_OTU4;
            break;
        case XCONPAYLOADTYPE_100G:
            payload = xconConfig::PAYLOADTYPE_100G;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon payload: " << cfg->payload << '\n';
            payload = xconConfig::PAYLOADTYPE_NOTSET;
            // return false;
            break;
    }

    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->set_payload_type(payload);

    Xcons_Xcon_Config_PayloadTreatment ptr;
    switch (cfg->payloadTreatment)
    {
        case XCONPAYLOADTREATMENT_TRANSPORT:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_TRANSPORT;
            break;
        case XCONPAYLOADTREATMENT_SWITCHING:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_SWITCHING;
            break;
        case XCONPAYLOADTREATMENT_TRANSPORT_WO_FEC:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_TRANSPORT_WO_FEC;
            break;
        case XCONPAYLOADTREATMENT_REGEN:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_REGEN;
            break;
        case XCONPAYLOADTREATMENT_REGEN_SWITCHING:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_REGEN_SWITCHING;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon payload treatment: " << cfg->payloadTreatment << '\n';
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_NONE;
            // return false;
            break;
    }

    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->set_payload_treatment(ptr);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&xcon);
    grpc::Status gStatus = mspCrud->Create( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Create FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    return status;
}

// We need to create 4 uni-dir Xcon for 1 Xpress Xcon
bool DcoXconAdapter::createXpressXcon(string aid, XconCommon *cfg, uint32_t *srcLoOduId, uint32_t *dstLoOduId)
{
    uint32_t srcClientId, dstClientId, srcOtuCniId, dstOtuCniId;

    if (cfg->payloadTreatment != XCONPAYLOADTREATMENT_REGEN && cfg->payloadTreatment != XCONPAYLOADTREATMENT_REGEN_SWITCHING )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad PTR for Xpress Xcon: " << cfg->payloadTreatment << '\n';
        return false;
    }

    // Xpress XCON command come from DP Mgr, Adapter CLI does not have xpress XCON command
    if (cfg->source.empty() == false && cfg->destination.empty() == false)
    {
        if ( cfg->sourceClient.empty() == false )
        {
            if (getClientId(cfg->sourceClient, &srcClientId) == false)
            {
                INFN_LOG(SeverityLevel::error) << __func__ << "Bad src client Id: " << cfg->sourceClient << '\n';
                return false;
            }
        }

        if ( cfg->destinationClient.empty() == false )
        {
            if (getClientId(cfg->destinationClient, &dstClientId) == false)
            {
                INFN_LOG(SeverityLevel::error) << __func__ << "Bad dst client Id: " << cfg->destinationClient << '\n';
                return false;
            }
        }

        if (getOtnId(cfg->source, &srcOtuCniId, srcLoOduId) == false)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad src LoOdu Id: " << cfg->source << '\n';
            return false;
        }

        if (getOtnId(cfg->destination, &dstOtuCniId, dstLoOduId) == false)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad dest LoOdu Id: " << cfg->destination << '\n';
            return false;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon Cfg: " << aid << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " SRC clientId: " << srcClientId << " srcOtuCniId: " << srcOtuCniId << ", srcLoOduId: " << *srcLoOduId << endl;

    INFN_LOG(SeverityLevel::info) << __func__ << " DST clientId: " << dstClientId << " dstOtuCniId: " << dstOtuCniId << ", dstLoOduId: " << *dstLoOduId << endl;


    // Our XCON ID is based on the LO-ODU ID.  XCON ID = LO-ODU ID
    // XPRESS XCON would need 4 uni-directional XCONs.
    // Which mean for one LO-ODU connection we need two uni Xcon:
    // INGRESS and EGRESS to connect two diffent clients.
    // INGRESS XCON ID = LO-ODU ID;
    // EGRESS XCON ID =  on L1 and 100G => LO-ODU ID + 8
    //                =  on L1 and 400G => LO-ODU ID + 12
    //                =  on L2 => LO-ODU ID + 12
    // XCON mapping table is in:
    // https://confluence/display/SOFTWARE/CHM6-DCO%3A+ODU+ID+and+XCON+ID+mapping

    uint32_t srcXconId = *srcLoOduId;
    uint32_t srcXconId2 = srcXconId + 12;  // L2 or L1&400GE
    // L1
    if ( srcOtuCniId == HO_OTU_ID_1 )
    {
        if ( cfg->payload != XCONPAYLOADTYPE_400GBE )
        {
            srcXconId2 = srcXconId + 8;
        }
    }

    uint32_t dstXconId = *dstLoOduId;
    uint32_t dstXconId2 = dstXconId + 12;  // L2 or L1&400GE
    // L1
    if ( dstOtuCniId == HO_OTU_ID_1 )
    {
        if ( cfg->payload != XCONPAYLOADTYPE_400GBE )
        {
            dstXconId2 = dstXconId + 8;
        }
    }

    if (dstXconId < 1 || dstXconId > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad DST Xcon Id: " << dstXconId << '\n';
        return false;
    }
    if (dstXconId2 < 1 || dstXconId2 > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad DST Xcon Id2: " << dstXconId2 << '\n';
        return false;
    }
    if (srcXconId2 < 1 || srcXconId2 > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad SRC Xcon Id2: " << srcXconId2 << '\n';
        return false;
    }
    if (srcXconId < 1 || srcXconId > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad SRC Xcon Id: " << srcXconId << '\n';
        return false;
    }

    INFN_LOG(SeverityLevel::info) << __func__ << " Create Xpress srcXconId: " << srcXconId << " srcXconId2: " << srcXconId2 << " dstXconId: " << dstXconId << " dstXconId2: " << dstXconId2 << endl;

    // Save AID, src and dst client AID in DCO so we can restore
    // regen Src/dst client on warm boot.
    // DCO XCON name field is only 32 bytes long.  We will need encoded the
    // AIDs to fit.
    cfg->name = encodeXpressXconAid(aid, cfg->sourceClient, cfg->destinationClient);

    INFN_LOG(SeverityLevel::info) << __func__ << " encoded Xpress XCON AID : " << cfg->name << '\n';

    // 1. Create UNI XCON src LO-ODU Ingress to Src Client
    if (createUniDirXpressXcon(cfg, srcXconId, srcClientId, *srcLoOduId, srcOtuCniId, xconConfig::DIRECTION_INGRESS) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR create INGRESS srcXconId : " << srcXconId << '\n';
        return false;
    }

    // 2. Create UNI XCON Src Client Egress to Dst LO-ODU
    if (createUniDirXpressXcon(cfg, srcXconId2, srcClientId, *dstLoOduId, dstOtuCniId, xconConfig::DIRECTION_EGRESS) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR create EGRESS srcXconId2 : " << srcXconId2 << '\n';
        return false;
    }

    // 3. Create UNI XCON dst LO-ODU Ingress to dst Client
    if (createUniDirXpressXcon(cfg, dstXconId, dstClientId, *dstLoOduId, dstOtuCniId, xconConfig::DIRECTION_INGRESS) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR create INGRESS dstXconId : " << dstXconId << '\n';
        return false;
    }

    // 4. Create UNI XCON dst Client Egress to src LO-ODU
    if (createUniDirXpressXcon(cfg, dstXconId2, dstClientId, *srcLoOduId, srcOtuCniId, xconConfig::DIRECTION_EGRESS) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR create EGRESS dstXconId2 : " << dstXconId2 << '\n';
        return false;
    }

    return true;
}

bool DcoXconAdapter::deleteXpressXcon(string aid, string *srcClientAid, string *dstClientAid, XconPayLoadType *payload, uint32_t *srcLoOduId, uint32_t *dstLoOduId)
{
    // Xpress XCON AID: 1-5-L1-1-ODU4i-1,1-5-L2-ODU4i-1"
    std::size_t found = aid.find(",");
    if (found == std::string::npos)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid ID. Missing comma : " << aid << '\n';
        return false;
    }

    string srcAid = aid.substr(0, found);
    string dstAid = aid.substr(found + 1);

    uint32_t srcOtuCniId;
    INFN_LOG(SeverityLevel::info) << __func__ << "Deleting SRC XCONs ODU AID: " << srcAid << '\n';

    if (getOtnId(srcAid, &srcOtuCniId, srcLoOduId) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad src LoOdu Id: " << srcAid << '\n';
        return false;
    }

    // *** XconId is from loOduId.
    uint32_t srcXconId = *srcLoOduId;

    // Get ClientId and Payload info from XCON before we delete it.
    XconCommon cfg;
    string srcXconAid = "Xcon ";
    srcXconAid += std::to_string(srcXconId);
    if ( getXconConfig(aid, &cfg) == false )
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Fail get config: " << srcXconAid << '\n';
        return false;
    }
    *payload = cfg.payload;
    *srcClientAid = cfg.sourceClient;
    *dstClientAid = cfg.destinationClient;
    // 1. delete UNI XCON src LO-ODU Ingress to Src Client
    if ( deleteXcon(srcXconAid) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Fail delete Xpress Xcon: " << srcXconAid << '\n';
        return false;
    }

    uint32_t srcXconId2 = srcXconId + 12;  // L2 or L1&400GE
    // L1
    if ( srcOtuCniId == HO_OTU_ID_1 )
    {
        if ( *payload != XCONPAYLOADTYPE_400GBE )
        {
            srcXconId2 = srcXconId + 8;
        }
    }
    // 2. Create UNI XCON Src Client Egress to Dst LO-ODU
    string srcXconAid2 = "Xcon ";
    srcXconAid2 += std::to_string(srcXconId2);
    if ( deleteXcon(srcXconAid2) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Fail delete Xpress Xcon: " << srcXconAid2 << '\n';
        return false;
    }

    uint32_t dstOtuCniId;

    INFN_LOG(SeverityLevel::info) << __func__ << "Deleting DST XCONs ODU AID: " << dstAid << '\n';

    if (getOtnId(dstAid, &dstOtuCniId, dstLoOduId) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad dst LoOdu Id: " << dstAid << '\n';
        return false;
    }

    // *** XconId is from loOduId.
    uint32_t dstXconId = *dstLoOduId;

    string dstXconAid = "Xcon ";
    dstXconAid += std::to_string(dstXconId);

    // 3. Delete UNI XCON dst LO-ODU Ingress to dst Client
    if ( deleteXcon(dstXconAid) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Fail delete Xpress Xcon: " << dstXconAid << '\n';
        return false;
    }

    uint32_t dstXconId2 = dstXconId + 12;  // L2 or L1&400GE
    // L1
    if ( dstOtuCniId == HO_OTU_ID_1 )
    {
        if ( *payload != XCONPAYLOADTYPE_400GBE )
        {
            dstXconId2 = dstXconId + 8;
        }
    }
    // 4. Delete UNI XCON dst Client Egress to src LO-ODU
    string dstXconAid2 = "Xcon ";
    dstXconAid2 += std::to_string(dstXconId2);
    if ( deleteXcon(dstXconAid2) == false)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Fail delete Xpress Xcon: " << dstXconAid2 << '\n';
        return false;
    }
    return true;
}

bool DcoXconAdapter::deleteXcon(string aid)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
	bool status = true;
    int xconId = aidToXconId(aid);
    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon Id: " << xconId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " Delete xconId: " << xconId << endl;

    Xcon xcon;
    //only set id in Xcon level, do not set id in Xcon config level for Xcon obj deletion
    xcon.add_xcon()->set_xcon_id(xconId);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&xcon);
    grpc::Status gStatus = mspCrud->Delete( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Delete FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }
    mOpStatus[xconId - 1].ClearOpStatusCache();
    return status;
}

bool DcoXconAdapter::initializeXcon()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    bool status = true;

    subscribeStreams();
    
    return status;
}

void DcoXconAdapter::subscribeStreams()
{
    //subscribe to xcon op status
    if(!mXconOpStatusSubEnable)
    {
        subXconOpStatus();
        mXconOpStatusSubEnable = true;
    }
}

//
//  Get/Read methods
//

void DcoXconAdapter::dumpAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    Xcon xconMsg;

    uint32 cfgId = 1;
    if (setKey(&xconMsg, cfgId) == false)
    {
        out << "Failed to set key" << endl;

        return;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&xconMsg);

    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);

        grpc::Status gStatus = mspCrud->Read( msg );
        if (!gStatus.ok())
        {
            INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message();

            out << "DCO Read FAIL. Grpc Error Code: " << gStatus.error_code() << ": " << gStatus.error_message() << endl;

            return;
        }
    }

    for (int idx = 0; idx < xconMsg.xcon_size(); idx++)
    {
        string xconAid;

        if (!xconMsg.xcon(idx).xcon().has_config())
        {
            out << "*** Xcon Config not present for idx: " << idx << " Skipping!!" << endl;

            continue;
        }

        uint32 objId = (uint32)xconMsg.xcon(idx).xcon().config().xcon_id().value();

        XconCommon cmn;

        translateConfig(xconMsg.xcon(idx).xcon().config(), cmn);

        if (xconMsg.xcon(idx).xcon().config().has_name())
        {
            xconAid = xconMsg.xcon(idx).xcon().config().name().value();

            INFN_LOG(SeverityLevel::debug) << "Xcon has name: " << xconAid;
        }
        else
        {
            xconAid = IdGen::genXconAid(DcoOduAdapter::getCacheOduAid(cmn.loOduId),
                                        cmn.clientId,
                                        cmn.direction);

            INFN_LOG(SeverityLevel::debug) << "Xcon has NO NAME. Using old genID created AID: " << xconAid;
        }

        out << "*** Xcon Config Info for XconId: " << objId << " Aid: " << xconAid << endl;

        dumpConfigInfo(out, cmn);

        if (xconMsg.xcon(idx).xcon().has_state())
        {
            XconStatus stat;

            translateStatus(xconMsg.xcon(idx).xcon().state(), stat);

            out << "*** Xcon State Info for XconId: " << objId << " Aid: " << xconAid << endl;

            dumpStatusInfo(out, stat);
        }
        else
        {
            out << "*** Xcon State not present for XconId: " << objId << " Aid: " << xconAid << endl;
        }
    }
}

bool DcoXconAdapter::xconConfigInfo(ostream& out, string aid)
{
    XconCommon cfg;
    bool status = getXconConfig(aid, &cfg);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }
    out << " Xcon Config Info: " << aid  << endl;

    dumpConfigInfo(out, cfg);

    return status;

}

void DcoXconAdapter::dumpConfigInfo(std::ostream& out, const XconCommon& cfg)
{
    out << " Name (AID): " << cfg.name << endl;
    out << " Client ID: " << cfg.clientId << endl;
    out << " LO ODU ID: " << cfg.loOduId << endl;
    out << " HO OTUCni ID: " << cfg.hoOtuId << endl;
    out << " Source AID: " << cfg.source << endl;
    out << " Destination AID: " << cfg.destination << endl;
    out << " Source Client AID: " << cfg.sourceClient << endl;
    out << " Destination Client AID: " << cfg.destinationClient << endl;
    out << " XCON Direction: " << EnumTranslate::toString(cfg.direction) << endl;
    out << " XCON Payload: " << EnumTranslate::toString(cfg.payload) << endl;
    out << " XCON Payload Treatment: " << EnumTranslate::toString(cfg.payloadTreatment) << endl;
    out << " TransactionId: 0x" << hex <<cfg.transactionId << dec << endl;
}

bool DcoXconAdapter::xconConfigInfo(ostream& out)
{
    TypeMapXconConfig mapCfg;

    if (!getXconConfig(mapCfg))
    {
        out << "*** FAILED to get Xcon Status!!" << endl;
        return false;
    }

    out << "*** Xcon Config Info ALL: " << endl;

    for(auto& it : mapCfg)
    {
        out << " Xcon Config Info: "  << it.first << endl;
        dumpConfigInfo(out, *it.second);
        out << endl;
    }

    return true;
}

bool DcoXconAdapter::xconStatusInfo(ostream& out, string aid)
{
    XconStatus stat;
    memset(&stat, 0, sizeof(stat));
    bool status = getXconStatus(aid, &stat);
    if ( status == false )
    {
        out << __func__ << " FAIL "  << endl;
        return status;
    }

    out << " Xcon Status Info for AID: "  << aid << endl;

    dumpStatusInfo(out, stat);

    int xconId = aidToXconId(aid);
    out << "\n mOpStatus.opStatus: " << EnumTranslate::toString(mOpStatus[xconId - 1].opStatus) << endl;
    out << "\n mOpStatus.transactionId: " << mOpStatus[xconId - 1].transactionId << endl;
    return status;
}

void DcoXconAdapter::dumpStatusInfo(std::ostream& out, const XconStatus& stat)
{
    out << " Xcon Status Info: "  << endl;
    out << " Client ID: " << stat.xcon.clientId << endl;
    out << " LO ODU ID: " << stat.xcon.loOduId << endl;
    out << " HO OTUCni ID: " << stat.xcon.hoOtuId << endl;
    out << " XCON Direction: " << EnumTranslate::toString(stat.xcon.direction) << endl;
    out << " XCON Payload: " << EnumTranslate::toString(stat.xcon.payload) << endl;
    out << " XCON Payload Treatment: " << EnumTranslate::toString(stat.xcon.payloadTreatment) << endl;
    out << " TransactionId    : 0x" << hex << stat.xcon.transactionId << dec << endl;
    out << "\n mXconOpStatusSubEnable: " << EnumTranslate::toString(mXconOpStatusSubEnable) << endl;
}

bool DcoXconAdapter::getXconConfig(string aid, XconCommon *cfg)
{
    int xconId = aidToXconId(aid);
    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon Id: " << xconId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " xconId: " << xconId << endl;
    Xcon xcon;

    if (setKey(&xcon, xconId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getXconObj( &xcon, xconId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Xcon Object  FAIL "  << endl;
        return status;
    }

    if (xcon.xcon(idx).xcon().has_config() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO Config Empty ... XconId: " <<  xconId << endl;
        return false;
    }

    translateConfig(xcon.xcon(idx).xcon().config(), *cfg);

    return status;
} // getXconConfig

void DcoXconAdapter::translateConfig(const XconConfig& dcoCfg, XconCommon& adCfg)
{
    if (dcoCfg.has_name())
    {
        adCfg.name = dcoCfg.name().value();
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Xcon has NO NAME";

        adCfg.name = "";
    }

    if (dcoCfg.has_ho_otu_id())
    {
        adCfg.hoOtuId = dcoCfg.ho_otu_id().value();
    }

    if (dcoCfg.has_lo_odu_id())
    {
        adCfg.loOduId = dcoCfg.lo_odu_id().value();
    }

    if (dcoCfg.has_client_id())
    {
        adCfg.clientId = dcoCfg.client_id().value();
    }

    if (dcoCfg.has_tid())
    {
        adCfg.transactionId = dcoCfg.tid().value();
    }
    switch (dcoCfg.direction())
    {
        case xconConfig::DIRECTION_EGRESS:
            adCfg.direction = XCONDIRECTION_EGRESS;
            break;
        case xconConfig::DIRECTION_INGRESS:
            adCfg.direction = XCONDIRECTION_INGRESS;
            break;
        case xconConfig::DIRECTION_BIDIRECTIONAL:
            adCfg.direction = XCONDIRECTION_BI_DIR;
            break;
        default:
            adCfg.direction = XCONDIRECTION_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown XCON direction: " << dcoCfg.direction() << endl;
            break;
    }

    switch (dcoCfg.payload_type())
    {
        case xconConfig::PAYLOADTYPE_100GB_ELAN:
            adCfg.payload = XCONPAYLOADTYPE_100GBE;
            break;
        case xconConfig::PAYLOADTYPE_400GB_ELAN:
            adCfg.payload = XCONPAYLOADTYPE_400GBE;
            break;
        case xconConfig::PAYLOADTYPE_OTU4:
            adCfg.payload = XCONPAYLOADTYPE_OTU4;
            break;
        case xconConfig::PAYLOADTYPE_100G:
            adCfg.payload = XCONPAYLOADTYPE_100G;
            break;
        default:
            adCfg.payload = XCONPAYLOADTYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown XCON payload: " << dcoCfg.payload_type() << endl;
            break;
    }

    switch (dcoCfg.payload_treatment())
    {
        case xconConfig::PAYLOADTREATMENT_PAYLOAD_TRANSPORT:
            adCfg.payloadTreatment = XCONPAYLOADTREATMENT_TRANSPORT;
            break;
        case xconConfig::PAYLOADTREATMENT_PAYLOAD_SWITCHING:
            adCfg.payloadTreatment = XCONPAYLOADTREATMENT_SWITCHING;
            break;
        case xconConfig::PAYLOADTREATMENT_PAYLOAD_TRANSPORT_WO_FEC:
            adCfg.payloadTreatment = XCONPAYLOADTREATMENT_TRANSPORT_WO_FEC;
            break;
        case xconConfig::PAYLOADTREATMENT_PAYLOAD_REGEN:
            adCfg.payloadTreatment = XCONPAYLOADTREATMENT_REGEN;
            break;
        case xconConfig::PAYLOADTREATMENT_PAYLOAD_REGEN_SWITCHING:
            adCfg.payloadTreatment = XCONPAYLOADTREATMENT_REGEN_SWITCHING;
            break;
        default:
            adCfg.payloadTreatment = XCONPAYLOADTREATMENT_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown XCON PTR: " << dcoCfg.payload_treatment() << endl;
            break;
    }

    // Xpress Xcon
    if (adCfg.payloadTreatment == XCONPAYLOADTREATMENT_REGEN || adCfg.payloadTreatment == XCONPAYLOADTREATMENT_REGEN_SWITCHING)
    {
        // DCO string name field is not long enough, 32 bytes,
        // we need to encode the  Xpress XCON AIDs to do warm boot restore.
        // From: "1-7-L1-1-ODUflexi-1,1-7-L2-1-ODUflexi-1,1-7-T8,1-7-T16"
        // To encoded AID: 1-7-L1-1-ODUflexi-1,1,8,16
        // Now we need to decode the encoded AID.
        // And fill in the rest of other field

        std::size_t found = adCfg.name.find(",");
        if (found == std::string::npos)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid ID. Missing comma : " << adCfg.name << '\n';
            return;
        }

        adCfg.source = adCfg.name.substr(0, found);

        // To encoded AID: 1-7-L1-1-ODUflexi-1,1,8,16
        // aid = 1,8,16
        string aid = adCfg.name.substr(found + 1);

        found = aid.find(",");
        if (found == std::string::npos)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid ID. Missing comma : " << aid << '\n';
            return;
        }
        string dstId = aid.substr(0, found);
        string aid2 = aid.substr(found+1);  // aid2 = 8,16

        found = adCfg.source.find("L");
        if (found == std::string::npos)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid Source AID : " << adCfg.source << '\n';
            return;
        }

        adCfg.destination = adCfg.source.substr(0, found);

        found = adCfg.source.find("L1");
        if (found == std::string::npos)
        {
            // Source must be L2
            adCfg.destination += "L1";
            found = adCfg.source.find("L2");
        }
        else
        {
            // Source is L1
            adCfg.destination += "L2";
            found = adCfg.source.find("L1");
        }

        if (found == std::string::npos)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid Source AID: " << adCfg.source << '\n';
            return;
        }

        aid = adCfg.source.substr(found+2);  // aid = -1-ODUflexi-1
        found = aid.find_last_of("-");
        adCfg.destination += aid.substr(0,found+1) + dstId;

        // aid2 = 8,16
        found = aid2.find(",");
        if (found == std::string::npos)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid encoded client id: " << aid2 << '\n';
            return;
        }

        string cId1 = aid2.substr(0, found);
        string cId2 = aid2.substr(found+1);

        found = adCfg.source.find("L");
        if (found == std::string::npos)
        {
            INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid source aid: " << adCfg.source << '\n';
            return;
        }

        adCfg.sourceClient = adCfg.source.substr(0, found) + "T" + cId1;
        adCfg.destinationClient = adCfg.source.substr(0, found) + "T" + cId2;

        // Now return the proper XpressXcon Aid to upper layer.
        adCfg.name = adCfg.source + "," + adCfg.destination;
        INFN_LOG(SeverityLevel::info) << __func__ << " Xpress Xcon AID : " << adCfg.name << '\n';
        INFN_LOG(SeverityLevel::info) << __func__ << " Xpress Xcon source Client AID : " << adCfg.sourceClient << '\n';
        INFN_LOG(SeverityLevel::info) << __func__ << " Xpress Xcon dest Client AID : " << adCfg.destinationClient << '\n';

    }
    else
    {
        string lineOduAid = DcoOduAdapter::getCacheOduAid(adCfg.loOduId);
        adCfg.source  = lineOduAid;
        //for gige, clientId: 1-16, for client odu4, clientId: 33-48
        adCfg.clientId = (adCfg.clientId > CLIENT_ODU_ID_OFFSET) ? (adCfg.clientId - CLIENT_ODU_ID_OFFSET) : adCfg.clientId;
        string tmpStr     = IdGen::genGigeAid(adCfg.clientId);
        adCfg.destination = tmpStr;
    }

} // translateConfig

bool DcoXconAdapter::getXconConfig(TypeMapXconConfig& mapCfg)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    Xcon xcon;

    uint32 cfgId = 1;
    if (setKey(&xcon, cfgId) == false)
    {
        return false;
    }

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&xcon);

    bool status = true;

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    INFN_LOG(SeverityLevel::info) << "size: " << xcon.xcon_size();

    for (int idx = 0; idx < xcon.xcon_size(); idx++)
    {
        std::shared_ptr<XconCommon> spXconCmn = make_shared<XconCommon>();
        translateConfig(xcon.xcon(idx).xcon().config(), *spXconCmn);

        string xconAid = spXconCmn->name;
        if (xconAid == "")
        {
            xconAid = IdGen::genXconAid(DcoOduAdapter::getCacheOduAid(spXconCmn->loOduId),
                                           spXconCmn->clientId,
                                           spXconCmn->direction);
        }

        // We only need to return one Express XCON config for 4 Uni XCONs
        if (spXconCmn->payloadTreatment == XCONPAYLOADTREATMENT_REGEN || spXconCmn->payloadTreatment == XCONPAYLOADTREATMENT_REGEN_SWITCHING)
        {
            if (xcon.xcon(idx).xcon_id() == spXconCmn->loOduId && spXconCmn->direction == XCONDIRECTION_INGRESS)
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "UNI XCON with LoOdu Id: " << spXconCmn->loOduId << '\n';
                // Alway return the Uni XCON with source LO-ODU and
                // INGRESS for Xpress XCON config
                // Xpress XCON AID: 1-5-L1-1-ODU4i-1,1-5-L2-ODU4i-1"

                std::size_t found = xconAid.find(",");
                if (found == std::string::npos)
                {
                    INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid ID. Missing comma : " << xconAid << '\n';
                    continue;
                }
                string srcAid = xconAid.substr(0, found);

                uint32_t srcOtuCniId, srcLoOduId;
                if (getOtnId(srcAid, &srcOtuCniId, &srcLoOduId) == false)
                {
                    INFN_LOG(SeverityLevel::error) << __func__ << "Bad src LoOdu Id: " << srcAid << '\n';
                    continue;
                }
                if (srcLoOduId != spXconCmn->loOduId)
                {
                    INFN_LOG(SeverityLevel::info) << __func__ << "Skip XCON with Destination LoOdu Id: " << srcAid << '\n';
                    continue;
                }
                // If we get here this is 1 of the 4 Uni-dir XCON we want to
                // use, change uni dir back to Bi dir
                spXconCmn->direction = XCONDIRECTION_BI_DIR;
            }
            else
            {
                INFN_LOG(SeverityLevel::info) << __func__ << "Skip XCON with EGRESS DIR: " << xconAid << '\n';
                continue;
            }
        }

        mapCfg[xconAid] = spXconCmn;
    }

    return status;
}

bool DcoXconAdapter::getXconStatus(string aid, XconStatus *stat)
{
    int xconId = aidToXconId(aid);
    if (xconId < 1 || xconId > MAX_XCON_ID)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon Id: " << xconId << '\n';
        return false;
    }
    INFN_LOG(SeverityLevel::info) << __func__ << " xconId: " << xconId << endl;
    Xcon xcon;

    if (setKey(&xcon, xconId) == false)
    {
        return false;
    }

    int idx = 0;
    bool status = getXconObj( &xcon, xconId, &idx);
    if ( status == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " Get Dco Xcon Object  FAIL "  << endl;
        return status;
    }

    if (xcon.xcon(idx).xcon().has_state() == false )
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " DCO State Container is Empty ... XconId: " <<  xconId << endl;
        return false;
    }

    translateStatus(xcon.xcon(idx).xcon().state(), *stat);

    return status;
} // getXconStatus

void DcoXconAdapter::translateStatus(const XconState& dcoState, XconStatus& stat)
{
    if (dcoState.has_ho_otu_id())
    {
        stat.xcon.hoOtuId = dcoState.ho_otu_id().value();
    }

    if (dcoState.has_lo_odu_id())
    {
        stat.xcon.loOduId = dcoState.lo_odu_id().value();
    }

    if (dcoState.has_lo_odu_id())
    {
        stat.xcon.clientId = dcoState.client_id().value();
    }

    if (dcoState.has_tid())
    {
        stat.xcon.transactionId = dcoState.tid().value();
    }

    switch (dcoState.direction())
    {
        case xconState::DIRECTION_EGRESS:
           stat.xcon.direction = XCONDIRECTION_EGRESS;
            break;
        case xconState::DIRECTION_INGRESS:
            stat.xcon.direction = XCONDIRECTION_INGRESS;
            break;
        case xconState::DIRECTION_BIDIRECTIONAL:
            stat.xcon.direction = XCONDIRECTION_BI_DIR;
            break;
        default:
            stat.xcon.direction = XCONDIRECTION_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown XCON direction: " << dcoState.direction() << endl;
            break;
    }

    switch (dcoState.payload_type())
    {
        case xconState::PAYLOADTYPE_100GB_ELAN:
            stat.xcon.payload = XCONPAYLOADTYPE_100GBE;
            break;
        case xconState::PAYLOADTYPE_400GB_ELAN:
            stat.xcon.payload = XCONPAYLOADTYPE_400GBE;
            break;
        case xconState::PAYLOADTYPE_OTU4:
            stat.xcon.payload = XCONPAYLOADTYPE_OTU4;
            break;
        case xconState::PAYLOADTYPE_100G:
            stat.xcon.payload = XCONPAYLOADTYPE_100G;
            break;
        default:
            stat.xcon.payload = XCONPAYLOADTYPE_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown XCON payload: " << dcoState.payload_type() << endl;
            break;
    }

    switch (dcoState.payload_treatment())
    {
        case xconState::PAYLOADTREATMENT_PAYLOAD_TRANSPORT:
            stat.xcon.payloadTreatment = XCONPAYLOADTREATMENT_TRANSPORT;
            break;
        case xconState::PAYLOADTREATMENT_PAYLOAD_SWITCHING:
            stat.xcon.payloadTreatment = XCONPAYLOADTREATMENT_SWITCHING;
            break;
        case xconState::PAYLOADTREATMENT_PAYLOAD_TRANSPORT_WO_FEC:
            stat.xcon.payloadTreatment = XCONPAYLOADTREATMENT_TRANSPORT_WO_FEC;
            break;
        case xconState::PAYLOADTREATMENT_PAYLOAD_REGEN:
            stat.xcon.payloadTreatment = XCONPAYLOADTREATMENT_REGEN;
            break;
        case xconState::PAYLOADTREATMENT_PAYLOAD_REGEN_SWITCHING:
            stat.xcon.payloadTreatment = XCONPAYLOADTREATMENT_REGEN_SWITCHING;
            break;
        default:
            stat.xcon.payloadTreatment = XCONPAYLOADTREATMENT_UNSPECIFIED;
            INFN_LOG(SeverityLevel::info) << __func__ << "Unknown XCON payload treatment: " << dcoState.payload_treatment() << endl;
            break;
    }
}

// ====== DcoClientAdapter private =========
int DcoXconAdapter::aidToXconId (string aid)
{
    if (aid == NULL)
        return -1;
    // AID #1: 1-4-L1-1-ODU4i-81,1-4-T3
    // AID #2:  1-4-T5,1-4-L1-1-ODU4i-81
    // AID 1-4-L1-1-ODUflexi-81
    // AID 1-5-L1-1-ODU4i-81
    // Just use the LO-ODU ID for XCON ID
    string sId;
    string aidOdu = "-L";
    string aidClient = "-T";
    std::size_t lpos = aid.find(aidOdu);
    std::size_t tpos = aid.find(aidClient);
    if (tpos != std::string::npos || lpos != std::string::npos)
    {
        // AID #1: 1-4-L1-1-ODU4i-81,1-4-T3
        if ( tpos > lpos)
        {
            string sTemp = ",";
            std::size_t found = aid.find(sTemp);
            if (found != std::string::npos)
            {
                string oduAid = aid.substr(0,found);
                int oduId = DcoOduAdapter::getCacheOduId(oduAid);
                INFN_LOG(SeverityLevel::info) << __func__ << " LoOduId : " << oduId << endl;
                if (oduId <= 0)
                {
                    return -1;
                }
                else
                {
                    return oduId;
                }
            }
            else
            {
                return -1;
            }
        }
        // AID #2:  1-4-T5,1-4-L1-1-ODU4i-81
        else
        {
            string oduAid;
            while (aid[tpos] != ',' && aid[tpos] != '\0' )
            {
                tpos++;
            }
            if (aid[tpos] == ',' )
            {
                tpos++;
                oduAid = aid.substr(tpos);
            }
            int oduId = DcoOduAdapter::getCacheOduId(oduAid);
            INFN_LOG(SeverityLevel::info) << __func__ << " LoOduId : " << oduId << endl;
            if (oduId <= 0)
            {
                return -1;
            }
            else
            {
                return oduId;
            }
        }
    }
    else
    {
        string cliXcon = "Xcon ";
        std::size_t found = aid.find(cliXcon);
        if (found == std::string::npos)
            return -1;
        sId = aid.substr(cliXcon.length());
        return(stoi(sId,nullptr));
    }

    return -1;
}

bool DcoXconAdapter::setKey (Xcon *xcon, int xconId)
{
    bool status = true;
	if (xcon == NULL)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Fail *xcon = NULL... "  << endl;
        return false;
    }

    Xcons_XconKey *cKey;

    // Always use the first object in the ClientXcon list, since we
    // create new xcon object every time we send.
    cKey = xcon->add_xcon();
    cKey->set_xcon_id(xconId);
    cKey->mutable_xcon()->mutable_config()->mutable_xcon_id()->set_value(xconId);

    return status;
}
//
// Fetch Xcon containter object and find the object that match with xconId in
// list
//
bool DcoXconAdapter::getXconObj( Xcon *xcon, int xconId, int *idx)
{
    // Save the XconId from setKey
    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(xcon);
    INFN_LOG(SeverityLevel::info) << __func__ << " \n\n ****Read ... \n"  << endl;
    bool status = true;

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    grpc::Status gStatus = mspCrud->Read( msg );
    if (gStatus.ok())
    {
        status = true;
        xcon = static_cast<Xcon*>(msg);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Read FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
        return status;
    }

    // Search for the correct XconId.
    bool found = false;

    INFN_LOG(SeverityLevel::info) << __func__ << " xcon size " << xcon->xcon_size() << endl;

    for (*idx = 0; *idx < xcon->xcon_size(); (*idx)++)
    {
        INFN_LOG(SeverityLevel::debug) << __func__ << " xcon id " << xcon->xcon(*idx).xcon_id() << endl;
        if (xcon->xcon(*idx).xcon_id() == xconId )
        {
            found = true;
            break;
        }
    }

    if (found == false)
    {
        INFN_LOG(SeverityLevel::info) << __func__ << " cannot find XconId in DCO  FAIL... XconId: " <<  xconId << endl;
        return false;
    }
    return status;
}

bool DcoXconAdapter::getClientId (string aid, uint32_t *clientId)
{
    if (aid == NULL)
        return false;

    // AID format: 1-4-T1
    string sId;
    string aidPort = "-T";
    std::size_t pos = aid.find(aidPort);

    if (pos == std::string::npos)
    {
	    INFN_LOG(SeverityLevel::info) << __func__ << ", not a client aid: " << aid << endl;
	    return false;
    }

    int clientOduId = DcoOduAdapter::getCacheOduId(aid);
    //client gige
    if (clientOduId == -1)
    {
	    sId = aid.substr(pos + aidPort.length());
        //For
        std::size_t temp = sId.find(".");
        if( temp != std::string::npos)
        {
            int parent = stoi(sId.substr(0,temp), nullptr);
            int child = stoi(sId.substr(temp+1));

            if (parent == 1)
            {

                *clientId = DataPlane::portLow0[child-1];
            }

            else if (parent == 8)
            {

                *clientId = DataPlane::portHigh0[child-1];
            }
            else if (parent == 9)
            {

                *clientId = DataPlane::portLow1[child-1];
            }
            else if (parent == 16)
            {

                *clientId = DataPlane::portHigh1[child-1];
            }
        }
        else
        {
            *clientId = stoi(sId,nullptr);
        }
        INFN_LOG(SeverityLevel::info) << __func__ << " ClientId sId: " << *clientId << endl;

    }
    else //client odu
    {
	    INFN_LOG(SeverityLevel::info) << __func__ << " aid: " << aid << " Client OduId : " << clientOduId << endl;
	    *clientId = clientOduId;
    }

    return true;
}

bool DcoXconAdapter::getOtnId (string aid, uint32_t *hoOtuId, uint32_t *loOduId)
{
    INFN_LOG(SeverityLevel::info) << __func__ << " AID: "  << aid << endl;
    if (aid == NULL)
        return false;
    // AID format: 1-5-L1-1-ODU4i-1 or 1-5-L1-1-ODU4-1
    string sId;
    string aidOtu = "-L";
    std::size_t found = aid.find(aidOtu);
    if (found != std::string::npos)
    {
        std::size_t pos = found + aidOtu.length();
        while (pos != std::string::npos)
        {
            if (aid[pos] == '-')
                break;
            sId.push_back(aid[pos]);
            pos++;
        }
    }
    else
    {
        return false;
    }
    //otu Id: 17-18
    *hoOtuId = stoi(sId,nullptr) + HO_OTU_ID_OFFSET;
    INFN_LOG(SeverityLevel::info) << __func__ << "HoOtuId: " << *hoOtuId << " sId: " << sId << endl;

    int oduId = DcoOduAdapter::getCacheOduId(aid);
    INFN_LOG(SeverityLevel::info) << __func__ << " LoOduId : " << oduId << endl;
    if (oduId < 0)
    {
        return false;
    }
    else
    {
        // Line side XCONID would be 17-32
        *loOduId = oduId;
        return true;
    }

    return false;
}

void DcoXconAdapter::setSimModeEn(bool enable)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (enable)
    {
        mspCrud = mspSimCrud;
        mIsGnmiSimModeEn = true;
    }
    else
    {
        mspCrud = mspSbCrud;
        mIsGnmiSimModeEn = false;
    }
}

// To create Uni Directional Xcon for Xpress Xcon
bool DcoXconAdapter::createUniDirXpressXcon(XconCommon *cfg, uint32_t xconId, uint32_t clientId, uint32_t loOduId, uint32_t otuCniId, Xcons_Xcon_Config_Direction dir )
{
	bool status = false;
    Xcon xcon;

    if (setKey(&xcon, xconId) == false)
    {
        return false;
    }
    cfg->transactionId = getTimeNanos();
    INFN_LOG(SeverityLevel::info) << __func__ << " Name : " << cfg->name << " xconId: " << xconId << " clientId: " << clientId << " loOduId: " << loOduId << " otuCniId: " << otuCniId << " dir: " << dir << " transId: 0x" << hex << cfg->transactionId <<  dec << endl;

    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_name()->set_value(cfg->name);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_ho_otu_id()->set_value(otuCniId);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_lo_odu_id()->set_value(loOduId);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_client_id()->set_value(clientId);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->set_direction(dir);
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->mutable_tid()->set_value(cfg->transactionId);

    Xcons_Xcon_Config_PayloadType payload;
    switch (cfg->payload)
    {
        case XCONPAYLOADTYPE_100GBE:
            payload = xconConfig::PAYLOADTYPE_100GB_ELAN;
            break;
        case XCONPAYLOADTYPE_400GBE:
            payload = xconConfig::PAYLOADTYPE_400GB_ELAN;
            break;
        case XCONPAYLOADTYPE_OTU4:
            payload = xconConfig::PAYLOADTYPE_OTU4;
            break;
        case XCONPAYLOADTYPE_100G:
            payload = xconConfig::PAYLOADTYPE_100G;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon payload: " << cfg->payload << '\n';
            payload = xconConfig::PAYLOADTYPE_NOTSET;
            // return false;
            break;
    }
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->set_payload_type(payload);

    Xcons_Xcon_Config_PayloadTreatment ptr;
    switch (cfg->payloadTreatment)
    {
        case XCONPAYLOADTREATMENT_TRANSPORT:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_TRANSPORT;
            break;
        case XCONPAYLOADTREATMENT_SWITCHING:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_SWITCHING;
            break;
        case XCONPAYLOADTREATMENT_TRANSPORT_WO_FEC:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_TRANSPORT_WO_FEC;
            break;
        case XCONPAYLOADTREATMENT_REGEN:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_REGEN;
            break;
        case XCONPAYLOADTREATMENT_REGEN_SWITCHING:
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_REGEN_SWITCHING;
            break;
        default:
            INFN_LOG(SeverityLevel::error) << __func__ << "Bad Xcon payload treatment: " << cfg->payloadTreatment << '\n';
            ptr = xconConfig::PAYLOADTREATMENT_PAYLOAD_NONE;
            // return false;
            break;
    }
    xcon.mutable_xcon(0)->mutable_xcon()->mutable_config()->set_payload_treatment(ptr);

    google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&xcon);
    grpc::Status gStatus = mspCrud->Create( msg );
    if (gStatus.ok())
    {
        status = true;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << __func__ << " Create FAIL "  << gStatus.error_code() << ": " << gStatus.error_message() << endl;
        status = false;
    }

    // One Xpress XCON command from host would create 10 - 12 commands
    // to DCO.  DCO could not keep up.  Dpms will wait for DCO to complete
    // status = false or TransactionId = 0 ==> don't wait
    if ( status == true && mOpStatus[xconId-1].waitForDcoAck(cfg->transactionId) == false)
    {
        return false;
    }
    return status;
}

// DCO string name field is not long enough, 32 bytes,  we need to encode the
// Xpress XCON AIDs to do warm boot restore.
// From: "1-7-L1-1-ODUflexi-1,1-7-L2-1-ODUflexi-1,1-7-T8,1-7-T16"
// To encoded AID: 1-7-L1-1-ODUflexi-1,1,8,16

string DcoXconAdapter::encodeXpressXconAid(string xconAid, string srcClientAid, string dstClientAid)
{
    std::size_t found = xconAid.find(",");
    if (found == std::string::npos)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid XCON AID. Missing comma : " << xconAid << '\n';
        // return empty string on error
        return std::string();
    }

    // Get src Aid
    string encodedAid = xconAid.substr(0, found);
    string dstAid = xconAid.substr(found + 1);

    found = dstAid.find_last_of("-");
    if (found == std::string::npos)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid XCON DST AID. Missing comma : " << dstAid << '\n';
        return std::string();
    }
    // Get Destination ODU instance ID.
    encodedAid += "," + dstAid.substr(found+1);

    found = srcClientAid.find_last_of("T");
    if (found == std::string::npos)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid XCON SRC Client AID. Missing comma : " << srcClientAid << '\n';
        return std::string();
    }
    // Get src client instance ID.
    encodedAid += "," + srcClientAid.substr(found+1);

    found = dstClientAid.find_last_of("T");
    if (found == std::string::npos)
    {
        INFN_LOG(SeverityLevel::error) << __func__ << "ERROR: Invalid XCON DST Client AID. Missing comma : " << dstClientAid << '\n';
        return std::string();
    }
    // Get dest client instance ID.
    encodedAid += "," + dstClientAid.substr(found+1);
    return encodedAid;
}

void DcoXconAdapter::subXconOpStatus()
{
	dcoXconOpStatus ops;
	google::protobuf::Message* msg = static_cast<google::protobuf::Message*>(&ops);
	::GnmiClient::AsyncSubCallback* cb = new XconOpStatusResponseContext(); //cleaned up in grpc client
	std::string cbId = mspCrud->Subscribe(msg, cb);
	INFN_LOG(SeverityLevel::info) << " xcon op status sub request sent to server.."  << endl;
}

} /* DpAdapter */
