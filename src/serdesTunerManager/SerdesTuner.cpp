#include <iostream>
#include <stdio.h>
#include <cstring>
#include "types.h"
#include "SerdesTuner.h"
#include "SerdesConnMap.h"
#include "InfnLogger.h"
#include "GearBoxAdapter.h"
#include "GearBoxUtils.h"
#include "GearBoxAdIf.h"
#include "GearBoxIf.h"
#include <time.h>

#include "chm6/redis_adapter/application_servicer.h"
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

using google::protobuf::util::MessageToJsonString;

namespace tuning
{

const std::string SerdesTuner::cTxMain        = "TxMain";
const std::string SerdesTuner::cTxPost1       = "TxPost1";
const std::string SerdesTuner::cTxPost2       = "TxPost2";
const std::string SerdesTuner::cTxPost3       = "TxPost3";
const std::string SerdesTuner::cTxPre1        = "TxPre1";
const std::string SerdesTuner::cTxPre2        = "TxPre2";
const std::string SerdesTuner::cTx_InEQ       = "Tx_InEQ";
const std::string SerdesTuner::cAutPeaking_En = "AutPeaking_EN";

const std::string SerdesTuner::cCaui4         = "CAUI4";
const std::string SerdesTuner::cGaui4         = "GAUI4";
const std::string SerdesTuner::cGaui8         = "GAUI8";
const std::string SerdesTuner::cOtu4          = "OTU4_4";

const uint16      SerdesTuner::cAtlNumLanes      = 32;

const std::string SerdesTuner::c100gPon       = "QSFP28-Generic";
const std::string SerdesTuner::c400gPon       = "QSFPDD-Generic";
const std::string SerdesTuner::c400gZrPon     = "QSFPDD-ZR-Generic";

//TODO: update connection map with correct linkid
SerdesTuner::SerdesTuner()
: mpCalData(NULL)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    const char *csv_lut_file;
    const char *csv_loss_file;

    const char *bin_lut_file;
    const char *bin_tables_file;

    uint32 cal_bin_size = 0;
    csv_lut_file = "/var/local/CHM6_LUT.csv" ;
    csv_loss_file = "/var/local/CHM6_Loss.csv" ;

    bin_tables_file = "/opt/infinera/chm6/configs/msapps/eep_loss_lut.bin";
    ifstream eepbin(bin_tables_file, ios::binary);
    uint8* cal_buf_ptr = nullptr;
    if (eepbin)
    {
        eepbin.seekg (0, eepbin.end);
        cal_bin_size = eepbin.tellg();
        eepbin.seekg (0, eepbin.beg);
        INFN_LOG(SeverityLevel::info) << "Using cal bin file: " << bin_tables_file << " size = " << cal_bin_size;
        cal_buf_ptr = new uint8[cal_bin_size];
        eepbin.read((char *) cal_buf_ptr, cal_bin_size);
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "CalEEProm read failed!Fallback to Serdes from software build!\n";
    }

    INFN_LOG(SeverityLevel::info) << "Init using loss csv file " <<  csv_loss_file << endl;
    INFN_LOG(SeverityLevel::info) << "Init using LUT csv file " << csv_lut_file << endl;

    mpCalData = new serdesCalDataGen6::SerdesCalDataGen6("CHM6", csv_loss_file, csv_lut_file,
                                                                    cal_buf_ptr, cal_bin_size);
    if (!mpCalData)
    {
        std::cout<< "Error: SerdesCalDataGen6 create failed\n";
        //TODO: throw exception here
        return;
    }
    bool status = Initialize();

    INFN_LOG(SeverityLevel::info) << "Serdes Init=" << status;

    for (unsigned int i = 0; i < 16; i++)
    {
        mTomSerdesMap.push_back(new lccmn_dataplane::TomSerdesMapState());
        mTomState.push_back(new lccmn_tom::TomState());
    }
    if (cal_buf_ptr)
    {
        delete [] cal_buf_ptr;
    }
}

SerdesTuner::~SerdesTuner()
{
    if (mpCalData)
    {
        delete mpCalData;
    }
    mpCalData = NULL;
}

bool SerdesTuner::Initialize()
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    serdesCalDataGen6::DataStatus importStatus;
    mpCalData->Initialize(importStatus);

    if (importStatus != serdesCalDataGen6::OK)
    {
        std::cout << "Error: Loss/LUT loading failed! importStatus: " << (uint32) importStatus << "\n";
        INFN_LOG(SeverityLevel::info) << "Error: Loss/LUT loading failed! importStatus: " << (uint32) importStatus << endl;
        return false;
    }
    return true;
}

void SerdesTuner::GetGearboxBcmParams(uint16 bcmNum, bool host, uint16& bus, uint16& bcmUnit, uint16& side)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);
    bus = gearbox::GetBus(bcmNum);
    bcmUnit = gearbox::GetBcmUnit(bcmNum);

    if (host == true)
    {
        side = gearbox::cHostSide;
    }
    else
    {
        side = gearbox::cLineSide;
    }
}

void SerdesTuner::GetTxParamsFromMap(std::map<std::string,sint32> TxParams, int& pre2, int& pre,
                                     int& main, int & post, int& post2, int & post3)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    std::string str;


    for (std::map<std::string,sint32>::iterator it=TxParams.begin(); it!=TxParams.end(); ++it)
    {
        INFN_LOG(SeverityLevel::debug) << it->first << " : " << it->second << endl;

        str = it->first;

        if (str.find(cTxMain) != std::string::npos)
        {
            main = (int)it->second;
            INFN_LOG(SeverityLevel::debug) << " main=" << main << " it->second=" << it->second;
        }
        else if (str.find(cTxPost1) != std::string::npos)
        {
            post = (int)it->second;
            INFN_LOG(SeverityLevel::debug) << " post=" << post << " elem.second=" << it->second;
        }
        else if (str.find(cTxPost2) != std::string::npos)
        {
            post2 = (int)it->second;
            INFN_LOG(SeverityLevel::debug) << " post2=" << post2 << " it->second=" << it->second;
        }
        else if (str.find(cTxPost3) != std::string::npos)
        {
            post3 = (int)it->second;
            INFN_LOG(SeverityLevel::debug) << " post3=" << post3 << " it->second=" << it->second;
        }
        else if (str.find(cTxPre1) != std::string::npos)
        {
            pre = (int)it->second;
            INFN_LOG(SeverityLevel::debug) << " pre1=" << pre << " it->second=" << it->second;
        }
        else if (str.find(cTxPre2) != std::string::npos)
        {
            pre2 = (int)it->second;
            INFN_LOG(SeverityLevel::debug) << " pre2=" << pre2 << " it->second=" << it->second;
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "Unknown txParam=" << it->first << " value=" << it->second << endl;
        }
    }
    INFN_LOG(SeverityLevel::info) << "  GetTxParamsFromMap: Bcm#: " << "pre2=" << pre2 << " pre=" << pre << " main=" << main << " post=" << post << " post2=" << post2 << " post3=" << post3 << endl;
}

void SerdesTuner::GetRxParamsFromMap(std::map<std::string,sint32> RxParams, std::map<unsigned int, int64_t>& rxValue)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    unsigned int rxConfig = 0;
    int64_t rxParam       = 0;
    std::string str;


    for (std::map<std::string,sint32>::iterator it=RxParams.begin(); it!=RxParams.end(); ++it)
    {
        INFN_LOG(SeverityLevel::debug) << it->first << " : " << it->second << endl;

        str = it->first;
        // Auto peaking is host only

        if (str.find(cAutPeaking_En) != std::string::npos)
        {
            rxParam = (int64_t)it->second;
            rxConfig = gearbox::DS_AutoPeakingEnable;
            rxValue.insert(std::make_pair(rxConfig, rxParam));
            INFN_LOG(SeverityLevel::debug) << " AutoPeaking_En=" << rxParam;
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "Unknown rxParam=" << it->first << " value=" << it->second << endl;
        }
    }


}



// Tune on BCM Tx to TOM direction
int SerdesTuner::SetBcmSerdesTxParams(uint16 bcmNum, bool host, uint16 laneNum, std::map<std::string,sint32> TxParams, bool force)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    uint16 bus;
    uint16 bcmUnit;
    uint16 side;
    uint16 lane;
    int pre2 = 0, pre = 0, main = 0, post = 0, post2 = 0, post3 = 0;
    int getPre2 = 0, getPre = 0, getMain = 0, getPost = 0, getPost2 = 0, getPost3 = 0;
    int rc = 0;
    bool tuneTxFir = true;

    GetGearboxBcmParams(bcmNum, host, bus, bcmUnit, side);
    GetTxParamsFromMap(TxParams, pre2, pre, main, post, post2, post3);
    lane = 1 << laneNum;
    gearbox::Bcm81725Lane param = {bus, bcmUnit, side, lane};

    INFN_LOG(SeverityLevel::info) << "  SetBcmSerdesTxParams: Bus: " << std::hex << bus << " bcmUnit=" << bcmUnit << " side=" << side << " Lane: " << lane << std::dec << endl;
    INFN_LOG(SeverityLevel::info) << "  SetBcmSerdesTxParams: Bcm#: " << "setTxFir pre2=" << pre2 << " pre=" << pre << " main=" << main << " post=" << post << " post2=" << post2 << " post3=" << post3 << endl;

    gearbox::GearBoxAdIf* adapter = gearbox::GearBoxAdapterSingleton::Instance();

    if (force == false)
    {
        rc = adapter->getTxFir(param, getPre2, getPre, getMain, getPost, getPost2, getPost3);

        if (getPre2  == pre2  &&
            getPre   == pre   &&
            getMain  == main  &&
            getPost  == post  &&
            getPost2 == post2 &&
            getPost3 == post3)
        {
            INFN_LOG(SeverityLevel::info) << "  SetBcmSerdesTxParams: SetTxFir already programmed!";
            tuneTxFir = false;
        }
    }


    if (force     == true ||
        tuneTxFir == true)
    {
        INFN_LOG(SeverityLevel::info) << "  SetBcmSerdesTxParams: Force=" << force << " tuneTxFir=" << tuneTxFir << " ...setting TxFir!";
        rc = adapter->setTxFir(param, pre2, pre, main, post, post2, post3);
    }

    INFN_LOG(SeverityLevel::info) << " SetTxFir=" << rc << endl;
    return rc;

}

void SerdesTuner::SetAtlSerdesTxParams(uint16 laneNum, std::map<std::string,sint32> TxParams)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    INFN_LOG(SeverityLevel::info) << "Saving Atlantic Tx Serdes Params to map.";
    int pre2 = 0, pre = 0, main = 0, post = 0, post2 = 0, post3 = 0;

    if (laneNum >= 0 && laneNum < cAtlNumLanes)
    {
        // Loss table starts lanes at 0
        laneNum++;
    }

    GetTxParamsFromMap(TxParams, pre2, pre, main, post, post2, post3);

    AtlBcmSerdesTxCfg atlBcmSerdesTxCfg = {polarityTbl[laneNum].txPolarity, polarityTbl[laneNum].rxPolarity, pre2,
                                           pre, main, post, post2, post3};

    if (mAtlBcmSerdesTxCfg.find(laneNum) != mAtlBcmSerdesTxCfg.end())
    {
        INFN_LOG(SeverityLevel::info) << "Existing Atlantic Tx param, erasing and updating.";
        mAtlBcmSerdesTxCfg.erase(laneNum);
    }

    INFN_LOG(SeverityLevel::info) << "Insert map value laneNum=" << laneNum << " (1 based) " <<
            " txPol=" << polarityTbl[laneNum].txPolarity << " rxPol=" << polarityTbl[laneNum].rxPolarity <<
            " pre2=" << pre2 << " pre=" << pre << " main=" << main << " post=" << post << " post2=" << post2 << " post3=" << post3;
    mAtlBcmSerdesTxCfg.insert(make_pair(laneNum, atlBcmSerdesTxCfg));


}



void SerdesTuner::SetAtlSerdesRxParams(uint16 laneNum, std::map<std::string,sint32> RxParams)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    // Atlantic chip has no rx serdes params?
    if (RxParams.empty() == true)
    {
        INFN_LOG(SeverityLevel::info) << "No Atlantic Rx Serdes params to set/configure.";
    }
    else
    {
        for (auto elem : RxParams)
        {
            INFN_LOG(SeverityLevel::info) << "elem.first=" << elem.first << " elem.second=" << elem.second;
        }
        INFN_LOG(SeverityLevel::info) << "!!!! SW BUG - LUT UPDATED, need to implement missing Atlantic Rx serdes params !!!";
    }
}


int SerdesTuner::SetBcmSerdesRxParams(uint16 bcmNum, bool host, uint16 laneNum, std::map<std::string,sint32> RxParams, bool force)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    uint16 bus;
    uint16 bcmUnit;
    uint16 side;
    uint16 lane;
    int rc = 0;
    bool found = false;
    std::string str;
    std::map<unsigned int, int64_t> rxValue;


    GetGearboxBcmParams(bcmNum, host, bus, bcmUnit, side);
    GetRxParamsFromMap(RxParams, rxValue);


    lane = 1 << laneNum;
    INFN_LOG(SeverityLevel::info) << "  SetBcmSerdesRxParams: Bus: " << std::hex << bus << " bcmUnit=0x" << bcmUnit << " side=" << side << " LaneMask: 0x" << lane << std::dec << endl;

    gearbox::Bcm81725Lane param = {(unsigned int)bus, (unsigned int)bcmUnit, (unsigned int)side, (unsigned int)lane};

    gearbox::GearBoxAdIf* adapter = gearbox::GearBoxAdapterSingleton::Instance();


    bool tuneAutoPeakEn = true;
    if (force == false)
    {
        bcm_plp_dsrds_firmware_lane_config_t lnConf;
        memset(&lnConf, 0, sizeof(bcm_plp_dsrds_firmware_lane_config_t));

        if (!adapter->getLaneConfigDs(param, lnConf))
        {
            // Check if serdes value is the same, if same, then no need to tune
            if (rxValue.find(gearbox::DS_AutoPeakingEnable) != rxValue.end() &&
                lnConf.AutoPeakingEnable == rxValue[gearbox::DS_AutoPeakingEnable])
            {
                tuneAutoPeakEn = false;
                INFN_LOG(SeverityLevel::info) << "  SetBcmSerdesRxParams: SetAutoPeakEn already programmed!";
            }
        }
    }

    if (force          == true ||
        tuneAutoPeakEn == true)
    {
        INFN_LOG(SeverityLevel::info) << "  SetBcmSerdesRxParams: Force=" << force << " tuneAutoPeakEn=" << tuneAutoPeakEn << " ...setting autoPeakEn!";
        for (auto elem : rxValue)
        {
            rc += adapter->setLaneConfigDs(param, elem.first, elem.second);
        }
    }
    return rc;
}




// Tune on TOM, Rx from BCM direction
void SerdesTuner::SetTomSerdesRxParams(uint16 tomId, uint16 laneNum,
                                       std::map<std::string,sint32> RxParams)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    INFN_LOG(SeverityLevel::info) << "  SetTomSerdesRxParams: TomId: " << tomId << " Lane: " << laneNum;

    uint16 tomIdx = tomId - 1;

    // Declares top level message
    lccmn_dataplane::TomSerdesLaneNum tomSerdesLaneNum;

    // Create google map to point to message.map variable
    google::protobuf::Map<std::string, sint32> * tomSerdesLaneNumMap =
            tomSerdesLaneNum.mutable_param_lane_num();


    for(auto elem : RxParams)
    {
        INFN_LOG(SeverityLevel::info) << " " << elem.first << ":" << elem.second << "," << endl;


        google::protobuf::MapPair<std::string, sint32>paramLanePair(elem.first, elem.second);
        tomSerdesLaneNumMap->insert(paramLanePair);

        string paramLaneStr;
        MessageToJsonString(tomSerdesLaneNum, &paramLaneStr);
        INFN_LOG(SeverityLevel::info) << "TomSerdesLaneNum=" << paramLaneStr;

        //serdesPair[elem.first] = elem.second;
    }
    google::protobuf::MapPair<uint32, lccmn_dataplane::TomSerdesLaneNum>tomSerdesLanePair(laneNum, tomSerdesLaneNum);
    mTomSerdesMap[tomIdx]->mutable_rx_params()->insert(tomSerdesLanePair);


    string tomSerdesStr;
    MessageToJsonString(*mTomSerdesMap[tomIdx], &tomSerdesStr);
    INFN_LOG(SeverityLevel::info) << "TomSerdesMap=" << tomSerdesStr;
}

// Tune on TOM, Tx to BCM direction
void SerdesTuner::SetTomSerdesTxParams(uint16 tomId, uint16 laneNum,
                                       std::map<std::string,sint32> TxParams)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    INFN_LOG(SeverityLevel::info) << "  SetTomSerdesTxParams: TomId: " << tomId << " Lane: " << laneNum;

    uint16 tomIdx = tomId - 1;

    // Declares top level message
    lccmn_dataplane::TomSerdesLaneNum tomSerdesLaneNum;

    // Create google map to point to message.map variable
    google::protobuf::Map<std::string, sint32> * tomSerdesLaneNumMap =
            tomSerdesLaneNum.mutable_param_lane_num();



    for(auto elem : TxParams)
    {
        INFN_LOG(SeverityLevel::info) << " " << elem.first << ":" << elem.second << "," << endl;


        google::protobuf::MapPair<std::string, sint32>paramLanePair(elem.first, elem.second);
        tomSerdesLaneNumMap->insert(paramLanePair);

        string paramLaneStr;
        MessageToJsonString(tomSerdesLaneNum, &paramLaneStr);
        INFN_LOG(SeverityLevel::info) << "TomSerdesLaneNum=" << paramLaneStr;
    }

    google::protobuf::MapPair<uint32, lccmn_dataplane::TomSerdesLaneNum>tomSerdesLanePair(laneNum, tomSerdesLaneNum);
    mTomSerdesMap[tomIdx]->mutable_tx_params()->insert(tomSerdesLanePair);


    string tomSerdesStr;
    MessageToJsonString(*mTomSerdesMap[tomIdx], &tomSerdesStr);
    INFN_LOG(SeverityLevel::info) << "TomSerdesMap=" << tomSerdesStr;
}



int SerdesTuner::TuneBcmToTomLinks(std::string aId, gearbox::Bcm81725DataRate_t rate, bool force)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int rc = 0;
    std::string rateStr = rate2SerdesCalDataString(rate);
    const tuning::LinkData *conn_map = BCM_to_qsfp28_connections;
    uint16 connMapSize = kLinksFromBcmToQsfp28;
    uint16 numLanes = c100gNumLanes;
    uint16 numOptLanes = 0;

    uint16 tomId = gearbox::aid2PortNum(aId);
    uint16 tomIdx = tomId - 1;
    uint32 startingLane = 0;

    // Configure for default TOM type/vendor
    // Default constructor requires data
    dp::DeviceInfo src {"BCM", "PORT" , "BRCM"};
    dp::DeviceInfo dest {"QSFP28", c100gPon, "Generic"};

    std::vector<SerdesDumpData> serdesDump;

    INFN_LOG(SeverityLevel::info) << "===Tune BCM to TOM links for AID=" << aId << " rate=" << rate;
    INFN_LOG(SeverityLevel::info) << "TomId=" << tomId << endl;


    if (GetRedisTomState(aId) == false)
    {
        INFN_LOG(SeverityLevel::error) << "Failed to read redis Chm6::TomState." << endl;
        return -1;
    }

    if (rate == gearbox::RATE_400GE)
    {
        conn_map = BCM_to_qsfpdd_connections_400GE;
        connMapSize = kLinksFromBcmToQsfpdd_400GE;
        numLanes = c400gNumLanes;
    }
    else if (rate == gearbox::RATE_4_100GE)
    {
        conn_map = BCM_to_qsfpdd_connections_4_100GE;
        connMapSize = kLinksFromBcmToQsfpdd_4_100GE;
        numLanes = c4_100gNumLanes;
        startingLane = GetBcmToTomLaneOffset(tomId, rate);
    }

    auto elem = mQsfpState.find(tomId);
    if (elem != mQsfpState.end())
    {
        numOptLanes   = elem->second.mNumOpticalLanes;
        dest.mDevice  = elem->second.mDevInfo.mDevice;
        dest.mPonType = elem->second.mDevInfo.mPonType;
        dest.mVendor  = elem->second.mDevInfo.mVendor;
        INFN_LOG(SeverityLevel::info) << "Installed TOM, tomId=" << tomId << " device=" << dest.mDevice << " pon=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Did not find - Installed TOM, tomId=" << tomId << " device=" << dest.mDevice << " pon=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
    }


    INFN_LOG(SeverityLevel::info) << "ConnMapSize=" << connMapSize << " numLanes=" << numLanes << " numOpticalLanes=" << numOptLanes;

    tuning::LinkData BcmToQsfp[numLanes];
    memset(BcmToQsfp, 0, (sizeof(tuning::LinkData)*numLanes));

    //mTomSerdesMap[tomId]->clear_rx_params();



    if (mTomSerdesMap[tomIdx]->has_aid() == false)
    {
        mTomSerdesMap[tomIdx]->mutable_base_state()->mutable_config_id()->set_value(aId);
        mTomSerdesMap[tomIdx]->mutable_base_state()->mutable_timestamp()->set_seconds(time(NULL));
        mTomSerdesMap[tomIdx]->mutable_base_state()->mutable_mark_for_delete()->set_value(false);
        mTomSerdesMap[tomIdx]->mutable_aid()->set_value(aId);

        AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectCreate(*mTomSerdesMap[tomIdx]);
        INFN_LOG(SeverityLevel::info) << "Create redis object.";
    }

    // Clear proto maps
    mTomSerdesMap[tomIdx]->mutable_rx_params()->clear();

    // First iterate through all BCM to TOM lanes
    for (uint32 lane = 0; lane < numLanes; ++lane)
    {
        GetLinkData(conn_map, connMapSize, tomId, lane + startingLane, false/*Qsfp is Rx*/, &BcmToQsfp[lane]);
        mpCalData->GetLinkLoss(BcmToQsfp[lane].link_id, BcmToQsfp[lane].link_loss);
        DumpLinkData(&BcmToQsfp[lane]);



        std::map<std::string,sint32> txparams;
        std::map<std::string,sint32> rxparams;

        std::string tomType = dest.mDevice;

        INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToQsfp[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;


        ZStatus status = mpCalData->GetTxTuningParams(src, dest, BcmToQsfp[lane].link_loss, rateStr, txparams);
        if(status != ZOK)
        {
            INFN_LOG(SeverityLevel::info) << "Error: Can not find TxTuningParams, will use generic." << endl;

            if (rate == gearbox::RATE_100GE)
            {
                dest.mDevice = "QSFP28";
                dest.mPonType = c100gPon; // LUT is using PON where PonType was now
            }
            else if (rate == gearbox::RATE_400GE)
            {
            	if(tomType == "QSFPDD-ZR")
            	{
            		dest.mDevice = "QSFPDD-ZR";
            		dest.mPonType = c400gZrPon;
            	}
            	else
            	{
            		dest.mDevice = "QSFPDD";
            		dest.mPonType = c400gPon;
            	}
            }
            else if (rate == gearbox::RATE_OTU4)
            {
                dest.mDevice = "QSFP28";
                dest.mPonType = c100gPon;
            }
            else if (rate == gearbox::RATE_4_100GE)
            {
                if(tomType == "QSFPDD-ZR")
                {
                    dest.mDevice = "QSFPDD-ZR";
                    dest.mPonType = c400gZrPon;
                }
            }

            dest.mVendor = "Generic";

            if (mpCalData->GetTxTuningParams(src, dest, BcmToQsfp[lane].link_loss, rateStr, txparams) != ZOK)
            {
                INFN_LOG(SeverityLevel::error) << "Cannot find tom type generic in GetTxTuningParams, returning!" << endl;
                INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
                INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
                INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToQsfp[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

                return -1;
            }
        }

        // Now tune BCM Tx (on BCM) and tune TOM Rx (on TOM)
        // Tune on BCM, Tx --> TOM direction (call gearbox)
        rc += SetBcmSerdesTxParams(BcmToQsfp[lane].tx_dev_id, false /*line*/, BcmToQsfp[lane].tx_lane_num, txparams, force);


        status = mpCalData->GetRxTuningParams(src, dest, BcmToQsfp[lane].link_loss, rateStr, rxparams);
        if(status != ZOK)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Can not find RxTuningParams!" << endl;
            INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToQsfp[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

            return -1;
        }


        SerdesDumpData serdesData(true, BcmToQsfp[lane].tx_dev_id, BcmToQsfp[lane].tx_lane_num,
                                  BcmToQsfp[lane].rx_dev_id, BcmToQsfp[lane].rx_lane_num, src, dest, txparams, rxparams,
                                  BcmToQsfp[lane].link_id, BcmToQsfp[lane].link_loss);
        serdesDump.push_back(serdesData);


        INFN_LOG(SeverityLevel::info) << "Port=" << tomId << " Rx Lane=" << BcmToQsfp[lane].rx_lane_num << endl;

        if (lane < numOptLanes)
        {
            // Tune on TOM, TOM Rx from BCM (call protos)
            SetTomSerdesRxParams(BcmToQsfp[lane].rx_dev_id, BcmToQsfp[lane].rx_lane_num,
                    rxparams);

        }

    }


    auto serdesDumpElem = mSerdesDumpBcmToTom.find(tomId);

    if (serdesDumpElem != mSerdesDumpBcmToTom.end())
    {
        mSerdesDumpBcmToTom.erase(serdesDumpElem);
    }

    INFN_LOG(SeverityLevel::info) << "finish block ok!"<< endl;
    mSerdesDumpBcmToTom.insert(make_pair(tomId, serdesDump));




    // Make sure we call this below, but only do it after LAST update to avoid multiple onModifies in TOM ms.
   // AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectUpdate(*mTomSerdesMap[tomIdx]);
    INFN_LOG(SeverityLevel::info) << "Not updating redis object, will update when last link is tuned, see TuneTomToBcmLinks()";

    INFN_LOG(SeverityLevel::info) << "===End tune BCM to TOM links for AID=" << aId << " rate=" << rate;
    return rc;
}

unsigned int SerdesTuner::GetBcmToTomLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    unsigned int startingLane = 0;

    if (rate == gearbox::RATE_4_100GE)
    {
        switch (tomId)
        {
        case gearbox::PORT1:
            startingLane = 2;
            break;
        case gearbox::PORT2:
            startingLane = 0;
            break;
        case gearbox::PORT3:
            startingLane = 4;
            break;
        case gearbox::PORT4:
            startingLane = 6;
            break;

        case gearbox::PORT5:
            startingLane = 4;
            break;
        case gearbox::PORT6:
            startingLane = 6;
            break;
        case gearbox::PORT7:
            startingLane = 0;
            break;
        case gearbox::PORT8:
            startingLane = 2;
            break;

        case gearbox::PORT9:
            startingLane = 2;
            break;
        case gearbox::PORT10:
            startingLane = 0;
            break;
        case gearbox::PORT11:
            startingLane = 4;
            break;
        case gearbox::PORT12:
            startingLane = 6;
            break;

        case gearbox::PORT13:
            startingLane = 4;
            break;
        case gearbox::PORT14:
            startingLane = 6;
            break;
        case gearbox::PORT15:
            startingLane = 0;
            break;
        case gearbox::PORT16:
            startingLane = 2;
            break;

        }
    }
    return startingLane;
}

unsigned int SerdesTuner::GetTomToBcmLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    unsigned int startingLane = 0;

    if (rate == gearbox::RATE_4_100GE)
    {
        switch (tomId)
        {
        case gearbox::PORT1:
            startingLane = 2;
            break;
        case gearbox::PORT2:
            startingLane = 0;
            break;
        case gearbox::PORT3:
            startingLane = 4;
            break;
        case gearbox::PORT4:
            startingLane = 6;
            break;

        case gearbox::PORT5:
            startingLane = 4;
            break;
        case gearbox::PORT6:
            startingLane = 6;
            break;
        case gearbox::PORT7:
            startingLane = 0;
            break;
        case gearbox::PORT8:
            startingLane = 2;
            break;

        case gearbox::PORT9:
            startingLane = 2;
            break;
        case gearbox::PORT10:
            startingLane = 0;
            break;
        case gearbox::PORT11:
            startingLane = 4;
            break;
        case gearbox::PORT12:
            startingLane = 6;
            break;

        case gearbox::PORT13:
            startingLane = 4;
            break;
        case gearbox::PORT14:
            startingLane = 6;
            break;
        case gearbox::PORT15:
            startingLane = 0;
            break;
        case gearbox::PORT16:
            startingLane = 2;
            break;

        }
    }

    return startingLane;
}

unsigned int SerdesTuner::GetBcmToAtlLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    // Starting lane is derived from Bcm_To_Atl table
    // 2 lanes are used for 100G
    // 4 lanes are used for 200G
    // 8 lanes are used for 400G
    unsigned int startingLane = 0;
    unsigned int laneOffset = 0;


    // AF0 todo - Add check for invalid ports for 200GE and 400GE, no port2, 4, etc...
    if (rate == gearbox::RATE_200GE)
    {
        laneOffset = 2;
    }
    else if (rate == gearbox::RATE_400GE)
    {
        laneOffset = 6;
    }
    else if (rate == gearbox::RATE_4_100GE)
    {
        laneOffset = 0;
    }

    switch (tomId)
    {
    case gearbox::PORT1:
        startingLane = (6 - laneOffset);
        break;
    case gearbox::PORT2:
        startingLane = 4;
        break;
    case gearbox::PORT3:
        startingLane = 0;
        break;
    case gearbox::PORT4:
        startingLane = 2;
        break;
    case gearbox::PORT5:
        startingLane = 0;
        break;
    case gearbox::PORT6:
        startingLane = (2 - laneOffset);
        break;
    case gearbox::PORT7:
        startingLane = 4;
        break;
    case gearbox::PORT8:
        startingLane = (6 - laneOffset);
        break;
    case gearbox::PORT9:
        startingLane = (6 - laneOffset);
        break;
    case gearbox::PORT10:
        startingLane = 4;
        break;
    case gearbox::PORT11:
        startingLane = 0;
        break;
    case gearbox::PORT12:
        startingLane = 2;
        break;
    case gearbox::PORT13:
        startingLane = 0;
        break;
    case gearbox::PORT14:
        startingLane = (2 - laneOffset);
        break;
    case gearbox::PORT15:
        startingLane = 4;
        break;
    case gearbox::PORT16:
        startingLane = (6 - laneOffset);
        break;

    }

    return startingLane;
}

unsigned int SerdesTuner::GetAtlToBcmLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    // Starting lane is derived from Atl_to_Bcm_connection table
    // 2 lanes are used for 100G
    // 4 lanes are used for 200G
    // 8 lanes are used for 400G
    unsigned int startingLane = 0;
    unsigned int laneOffset = 0;

    // AF0 todo - Add check for invalid ports for 200GE and 400GE, no port2, 4, etc...
    if (rate == gearbox::RATE_200GE)
    {
        laneOffset = 2;
    }
    else if (rate == gearbox::RATE_400GE)
    {
        laneOffset = 6;
    }
    else if (rate == gearbox::RATE_4_100GE)
    {
        laneOffset = 0;
    }

    switch (tomId)
    {
    case gearbox::PORT1:
        startingLane = (6 - laneOffset);
        break;
    case gearbox::PORT2:
        startingLane = 4;
        break;
    case gearbox::PORT3:
        startingLane = 0;
        break;
    case gearbox::PORT4:
        startingLane = 2;
        break;
    case gearbox::PORT5:
        startingLane = 0;
        break;
    case gearbox::PORT6:
        startingLane = (2 - laneOffset);
        break;
    case gearbox::PORT7:
        startingLane = 4;
        break;
    case gearbox::PORT8:
        startingLane = (6 - laneOffset);
        break;
    case gearbox::PORT9:
        startingLane = (6 - laneOffset);
        break;
    case gearbox::PORT10:
        startingLane = 4;
        break;
    case gearbox::PORT11:
        startingLane = 0;
        break;
    case gearbox::PORT12:
        startingLane = 2;
        break;
    case gearbox::PORT13:
        startingLane = 0;
        break;
    case gearbox::PORT14:
        startingLane = (2 - laneOffset);
        break;
    case gearbox::PORT15:
        startingLane = 4;
        break;
    case gearbox::PORT16:
        startingLane = (6 - laneOffset);;
        break;

    }

    return startingLane;
}

// Returns the Host side (BCM 3) lane offset for each TOM port
unsigned int SerdesTuner::GetBcmToBcmHostLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    unsigned int startingLane = 0;
    unsigned int laneOffset = 0;

    // AF0 todo - Add check for invalid ports for 200GE and 400GE, no port2, 4, etc...

    if (rate == gearbox::RATE_200GE)
    {
        laneOffset = 2;
    }

    switch (tomId)
    {
    case gearbox::PORT3:
        startingLane = 0;
        break;
    case gearbox::PORT4:
        startingLane = 2;
        break;
    case gearbox::PORT5:
        startingLane = 0;
        break;
    case gearbox::PORT6:
        startingLane = (2 - laneOffset);
        break;
    case gearbox::PORT11:
        startingLane = 0;
        break;
    case gearbox::PORT12:
        startingLane = 2;
        break;
    case gearbox::PORT13:
        startingLane = 0;
        break;
    case gearbox::PORT14:
        startingLane = (2 - laneOffset);
        break;
    }

    return startingLane;
}

unsigned int SerdesTuner::GetBcmToAtlNumHost(unsigned int tomId)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    unsigned int bcmNum = 2;

    switch (tomId)
    {
    case gearbox::PORT1:
    case gearbox::PORT2:
    case gearbox::PORT3:
    case gearbox::PORT4:
        bcmNum = 2;
        break;
    case gearbox::PORT5:
    case gearbox::PORT6:
    case gearbox::PORT7:
    case gearbox::PORT8:
        bcmNum = 1;
        break;
    case gearbox::PORT9:
    case gearbox::PORT10:
    case gearbox::PORT11:
    case gearbox::PORT12:
        bcmNum = 5;
        break;
    case gearbox::PORT13:
    case gearbox::PORT14:
    case gearbox::PORT15:
    case gearbox::PORT16:
        bcmNum = 4;
        break;
    default:
        INFN_LOG(SeverityLevel::info) << "TomID=" << tomId << "  - port not valid for BCM to BCM.";
        break;
    }
    return bcmNum;

}

// Gets the BCM num for BCM to BCM for the Atl side BCMs (1, 2, (top) or 4 and 5 (bottom)
unsigned int SerdesTuner::GetBcmToBcmLineToHostNum(unsigned int tomId)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    unsigned int bcmNum = 2;

    switch (tomId)
    {
    case gearbox::PORT3:
    case gearbox::PORT4:
        bcmNum = 2;
        break;
    case gearbox::PORT5:
    case gearbox::PORT6:
        bcmNum = 1;
        break;
    case gearbox::PORT11:
    case gearbox::PORT12:
        bcmNum = 5;
        break;
    case gearbox::PORT13:
    case gearbox::PORT14:
        bcmNum = 4;
        break;
    default:
        INFN_LOG(SeverityLevel::info) << "TomID=" << tomId << "  - port not valid for BCM to BCM.";
        break;
    }
    return bcmNum;
}



int SerdesTuner::TuneBcmHostToBcmLineLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int rc = 0;
    std::vector<SerdesDumpData> serdesDump;
    INFN_LOG(SeverityLevel::info) << "===Tune BCM Host to BCM Line links for tomId=" << tomId << " rate=" << (unsigned int)rate << " ====" << endl;
    if (IsBcmToBcmValidPort(tomId) == false)
    {
        INFN_LOG(SeverityLevel::info) << "===Not required for tomId=" << tomId << "====";
        return 0;
    }

    unsigned int numLanes = 2;
    if (rate == gearbox::RATE_200GE)
    {
        numLanes = 4;
    }
    else if (rate == gearbox::RATE_400GE ||
             rate == gearbox::RATE_4_100GE)
    {
        return 0;
    }

    LinkData BcmToBcm[numLanes];
    memset(BcmToBcm, 0, (sizeof(tuning::LinkData) * numLanes));
    std::string rateStr = cGaui8;
    unsigned int startingLane = GetBcmToBcmHostLaneOffset(tomId, rate);
    unsigned int bcmNum       = GetBcmToBcmLineToHostNum(tomId);

    for (uint32 lane = 0; lane < numLanes; ++lane)
    {
        INFN_LOG(SeverityLevel::info) << "TomID=" << tomId << " Lane=" << lane + startingLane << endl;
        GetLinkData(BcmHost_to_BcmLine_connections, kLinksFromBcmHostToBcmLine, bcmNum, (lane + startingLane), false /*Rx*/, &BcmToBcm[lane]);
        mpCalData->GetLinkLoss(BcmToBcm[lane].link_id, BcmToBcm[lane].link_loss);
        DumpLinkData(&BcmToBcm[lane]);

        dp::DeviceInfo src { "BCM", "HOST" , "BRCM" };
        dp::DeviceInfo dest{ "BCM", "PORT" , "BRCM" };

        std::map<std::string, sint32> txparams;
        std::map<std::string, sint32> rxparams;

        INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Searching for LUT data for BCM " << BcmToBcm[lane].tx_dev_id << " to " << BcmToBcm[lane].rx_dev_id << " in GetTxTuningParams." << endl;
        INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToBcm[lane].link_loss << " lane=" << lane << endl;

        ZStatus status = mpCalData->GetTxTuningParams(src, dest, BcmToBcm[lane].link_loss, rateStr, txparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::info) << "Cannot find BCM " << BcmToBcm[lane].tx_dev_id << " to " << BcmToBcm[lane].rx_dev_id << " in GetTxTuningParams." << endl;
            return -1;
        }

        // Tune BCM Tx (host) to BCM Rx (line)
        rc += SetBcmSerdesTxParams(BcmToBcm[lane].tx_dev_id, true /*host Tx*/, BcmToBcm[lane].tx_lane_num, txparams);

        status = mpCalData->GetRxTuningParams(src, dest, BcmToBcm[lane].link_loss, rateStr, rxparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Can not find RxTuningParams!" << endl;
            INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToBcm[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

            return -1;
        }

        SerdesDumpData serdesData(true, BcmToBcm[lane].tx_dev_id, BcmToBcm[lane].tx_lane_num,
                BcmToBcm[lane].rx_dev_id, BcmToBcm[lane].rx_lane_num, src, dest, txparams, rxparams,
                BcmToBcm[lane].link_id, BcmToBcm[lane].link_loss);
        serdesDump.push_back(serdesData);
        INFN_LOG(SeverityLevel::info) << "Dump ok!"<< endl;


        rc += SetBcmSerdesRxParams(BcmToBcm[lane].rx_dev_id, false /*line*/, BcmToBcm[lane].rx_lane_num, rxparams);
    }

    auto elem = mSerdesBcmHostToBcmLine.find(tomId);
    INFN_LOG(SeverityLevel::info) << "Find ok!"<< endl;

    if (elem != mSerdesBcmHostToBcmLine.end())
    {
        INFN_LOG(SeverityLevel::info) << "if block ok!"<< endl;
        mSerdesBcmHostToBcmLine.erase(elem);
    }

    INFN_LOG(SeverityLevel::info) << "finish block ok!"<< endl;
    mSerdesBcmHostToBcmLine.insert(make_pair(tomId, serdesDump));


    return rc;
}


bool SerdesTuner::IsBcmToBcmValidPort(unsigned int tomId)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    if (tomId == gearbox::PORT3  ||
        tomId == gearbox::PORT4  ||
        tomId == gearbox::PORT5  ||
        tomId == gearbox::PORT6  ||
        tomId == gearbox::PORT11 ||
        tomId == gearbox::PORT12 ||
        tomId == gearbox::PORT13 ||
        tomId == gearbox::PORT14)
    {
        return true;
    }
    else
    {
        return false;
    }
}




int SerdesTuner::TuneBcmLineToBcmHostLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int rc = 0;
    std::vector<SerdesDumpData> serdesDump;
    INFN_LOG(SeverityLevel::info) << "===Tune BCM Line to BCM Host for tomId=" << tomId << " rate=" << (unsigned int)rate << "====";
    if (IsBcmToBcmValidPort(tomId) == false)
    {
        INFN_LOG(SeverityLevel::info) << "===Not required for tomId=" << tomId << "====";
        return 0;
    }

    unsigned int numLanes = 2;
    if (rate == gearbox::RATE_200GE)
    {
        numLanes = 4;
    }
    else if (rate == gearbox::RATE_400GE ||
             rate == gearbox::RATE_4_100GE)
    {
        return 0;
    }

    LinkData BcmToBcm[numLanes];
    memset(BcmToBcm, 0, (sizeof(tuning::LinkData) * numLanes));
    std::string rateStr = cGaui8;
    unsigned int startingLane = GetBcmToBcmHostLaneOffset(tomId, rate);
    unsigned int bcmNum       = GetBcmToBcmLineToHostNum(tomId);

    for (uint32 lane=0; lane < numLanes; ++lane)
    {
        INFN_LOG(SeverityLevel::info) << "TomID=" << tomId << " Lane=" << lane + startingLane << endl;
        GetLinkData(BcmLine_to_BcmHost_connections, kLinksFromBcmLineToBcmHost, bcmNum, lane + startingLane, true /*Tx*/, &BcmToBcm[lane]);
        mpCalData->GetLinkLoss(BcmToBcm[lane].link_id, BcmToBcm[lane].link_loss);
        DumpLinkData(&BcmToBcm[lane]);

        dp::DeviceInfo src { "BCM", "PORT" , "BRCM" };
        dp::DeviceInfo dest{ "BCM", "HOST" , "BRCM" };

        std::map<std::string, sint32> txparams;
        std::map<std::string, sint32> rxparams;

        INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Searching for LUT data for BCM " << BcmToBcm[lane].rx_dev_id << " to " << BcmToBcm[lane].tx_dev_id << " in GetTxTuningParams." << endl;
        INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToBcm[lane].link_loss << " lane=" << lane << endl;


        ZStatus status = mpCalData->GetTxTuningParams(src, dest, BcmToBcm[lane].link_loss, rateStr, txparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::info) << "Cannot find BCM " << BcmToBcm[lane].rx_dev_id << " to " << BcmToBcm[lane].tx_dev_id << " in GetTxTuningParams." << endl;
            return -1;
        }

        // Tune BCM Tx (line) to BCM Rx (Host)
        rc += SetBcmSerdesTxParams(BcmToBcm[lane].tx_dev_id, false /*line Tx*/, BcmToBcm[lane].tx_lane_num, txparams);

        status = mpCalData->GetRxTuningParams(src, dest, BcmToBcm[lane].link_loss, rateStr, rxparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Can not find RxTuningParams!" << endl;
            INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToBcm[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

            return -1;
        }

        SerdesDumpData serdesData(true, BcmToBcm[lane].tx_dev_id, BcmToBcm[lane].tx_lane_num,
                BcmToBcm[lane].rx_dev_id, BcmToBcm[lane].rx_lane_num, src, dest, txparams, rxparams,
                BcmToBcm[lane].link_id, BcmToBcm[lane].link_loss);
        serdesDump.push_back(serdesData);
        INFN_LOG(SeverityLevel::info) << "Dump ok!"<< endl;


        rc += SetBcmSerdesRxParams(BcmToBcm[lane].rx_dev_id, true /*host*/, BcmToBcm[lane].rx_lane_num, rxparams);
    }


    auto elem = mSerdesBcmLineToBcmHost.find(tomId);
    INFN_LOG(SeverityLevel::info) << "Find ok!"<< endl;

    if (elem != mSerdesBcmLineToBcmHost.end())
    {
        INFN_LOG(SeverityLevel::info) << "if block ok!"<< endl;
        mSerdesBcmLineToBcmHost.erase(elem);
    }

    INFN_LOG(SeverityLevel::info) << "finish block ok!"<< endl;
    mSerdesBcmLineToBcmHost.insert(make_pair(tomId, serdesDump));




    return rc;
}



std::string SerdesTuner::rate2SerdesCalDataString(unsigned int rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    // Example G = 50G, 4 lanes 50x4 = 200G
    // C = 25G
    std::string strRate;

    switch(rate)
    {
    case gearbox::RATE_100GE:
        strRate = cCaui4;
        break;
    case gearbox::RATE_200GE:
        strRate = cGaui4;
        break;
    case gearbox::RATE_400GE:
        strRate = cGaui8;
        break;
    case gearbox::RATE_OTU4:
        strRate = cOtu4;
        break;
    case gearbox::RATE_4_100GE:
        strRate = cCaui4;
        break;
    default:
        break;
    }

   return strRate;
}

int SerdesTuner::TuneAtlToBcmLinks(std::string aId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int result = 0;

    INFN_LOG(SeverityLevel::info) << "===Get Atlantic Serdes ONLY for AID=" << aId << " ====" << endl;
    unsigned int tomId  = gearbox::aid2PortNum(aId);

    result += TuneAtlToBcmLinks(tomId, rate);


    return result;
}

int SerdesTuner::TuneLinks(std::string aId, bool onCreate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int result = 0;
    gearbox::Bcm81725DataRate_t rate;
    unsigned int tomId  = gearbox::aid2PortNum(aId);

    gearbox::GearBoxAdIf* adapter = gearbox::GearBoxAdapterSingleton::Instance();

    if (adapter->getCachedPortRate(tomId, rate) == 0)
    {
        result = TuneLinks(aId, rate, onCreate);
    }
    else
    {
        result = -1;
    }



    return result;
}

int SerdesTuner::TuneLinks(std::string aId, gearbox::Bcm81725DataRate_t rate, bool onCreate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int result    = 0;


    if (onCreate == true)
    {
        INFN_LOG(SeverityLevel::info) << "===Tune Serdes for AID=" << aId << " ====" << endl;
        unsigned int tomId  = gearbox::aid2PortNum(aId);

        result += TuneBcmLineToBcmHostLinks(tomId, rate);
        result += TuneBcmHostToBcmLineLinks(tomId, rate);
        result += TuneBcmToTomLinks(aId, rate);
        result += TuneTomToBcmLinks(aId, rate);

        result += TuneBcmToAtlLinks(tomId, rate);

        // Atlantic to BCM is handled separately before DCO is brought up.
        result += PushAtlToBcmRxParamsToHw(tomId, rate);

        INFN_LOG(SeverityLevel::info) << "===Serdes Tuning Complete result= " << result << " ====" << endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "===Warm reboot or onModify triggered, no serdes tuning necessary for AID=" << aId << "===" << endl;
    }
    return result;
}


int SerdesTuner::TuneTomToBcmLinks(std::string aId, gearbox::Bcm81725DataRate_t rate, bool force)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int rc = 0;
    std::vector<SerdesDumpData> serdesDump;
    std::string rateStr = rate2SerdesCalDataString(rate);
    const tuning::LinkData *conn_map = Qsfp28_to_BCM_connections;
    uint16 connMapSize = kLinksFromQsfp28ToBcm;
    uint16 tomId       = gearbox::aid2PortNum(aId);
    uint16 tomIdx      = tomId - 1;
    uint16 numLanes    = c100gNumLanes;
    uint16 numOptLanes = 0;
    uint32 startingLane = 0;

    INFN_LOG(SeverityLevel::info) << "===Tune TOM to BCM links for AID=" << aId << " rate=" << rate;

    if (rate == gearbox::RATE_400GE)
    {
        conn_map    = Qsfpdd_to_BCM_connections_400GE;
        connMapSize = kLinksFromQsfpddToBcm_400GE;
        numLanes    = c400gNumLanes;
    }
    else if  (rate == gearbox::RATE_4_100GE)
    {
        conn_map    = Qsfpdd_to_BCM_connections_4_100GE;
        connMapSize = kLinksFromQsfpddToBcm_4_100GE;
        numLanes    = c4_100gNumLanes;
        startingLane = GetTomToBcmLaneOffset(tomId, rate);
    }

    auto elem = mQsfpState.find(tomId);
    if (elem != mQsfpState.end())
    {
        numOptLanes = elem->second.mNumOpticalLanes;
    }

    INFN_LOG(SeverityLevel::info) << "ConnMapSize=" << connMapSize << " numLanes=" << numLanes << " numOpticalLanes=" << numOptLanes;


    tuning::LinkData QsfpToBcm[numLanes];
    memset(QsfpToBcm, 0, (sizeof(tuning::LinkData)*numLanes));


    if (mTomSerdesMap[tomIdx]->has_aid() == false)
    {
        mTomSerdesMap[tomIdx]->mutable_base_state()->mutable_config_id()->set_value(aId);
        mTomSerdesMap[tomIdx]->mutable_base_state()->mutable_timestamp()->set_seconds(time(NULL));
        mTomSerdesMap[tomIdx]->mutable_base_state()->mutable_mark_for_delete()->set_value(false);
        mTomSerdesMap[tomIdx]->mutable_aid()->set_value(aId);

        AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectCreate(*mTomSerdesMap[tomIdx]);
        INFN_LOG(SeverityLevel::info) << "Create redis object.";
    }

    // Clear proto maps
    mTomSerdesMap[tomIdx]->mutable_tx_params()->clear();




    // Default constructor requires data
    dp::DeviceInfo src{ "QSFP28", c100gPon, "Generic" };

    INFN_LOG(SeverityLevel::info) << "TomId=" << tomId << endl;

    elem = mQsfpState.find(tomId);

    if (elem != mQsfpState.end())
    {
        src.mDevice = elem->second.mDevInfo.mDevice;
        src.mPonType = elem->second.mDevInfo.mPonType;
        src.mVendor = elem->second.mDevInfo.mVendor;
        INFN_LOG(SeverityLevel::info) << "Installed TOM, tomId=" << tomId << " device=" << src.mDevice << " pon=" << src.mPonType << " vendor=" << src.mVendor << endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Did not find - Installed TOM, tomId=" << tomId << " device=" << src.mDevice << " pon=" << src.mPonType << " vendor=" << src.mVendor << endl;
    }


    //TODO: we don't need BCM1/2/3 in LUT, this is help reduce # segments required as well
    dp::DeviceInfo dest{ "BCM", "PORT" , "BRCM" };


    for (uint32 lane = 0; lane < numLanes; ++lane)
    {
        GetLinkData(conn_map, connMapSize, tomId, lane + startingLane, true/*Qsfp is Tx*/, &QsfpToBcm[lane]);
        mpCalData->GetLinkLoss(QsfpToBcm[lane].link_id, QsfpToBcm[lane].link_loss);
        DumpLinkData(&QsfpToBcm[lane]);


        std::map<std::string, sint32> txparams;
        std::map<std::string, sint32> rxparams;

        INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "LinkLoss=" << QsfpToBcm[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

        std::string tomType = src.mDevice;

        ZStatus status = mpCalData->GetTxTuningParams(src, dest, QsfpToBcm[lane].link_loss, rateStr, txparams);
        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::info) << "Cannot find tom type in GetTxTuningParams, will use generic." << endl;


            if (rate == gearbox::RATE_100GE)
            {
                src.mDevice = "QSFP28";
                src.mPonType = c100gPon;
            }
            else if (rate == gearbox::RATE_400GE)
            {
                if(tomType == "QSFPDD-ZR")
                {
                    src.mDevice = "QSFPDD-ZR";
                    src.mPonType = c400gZrPon;
                }
                else
                {
                    src.mDevice = "QSFPDD";
                    src.mPonType = c400gPon;
                }
            }
            else if (rate == gearbox::RATE_OTU4)
            {
                src.mDevice = "QSFP28";
                src.mPonType = c100gPon;
            }
            else if (rate == gearbox::RATE_4_100GE)
            {
                if(tomType == "QSFPDD-ZR")
                {
                    src.mDevice = "QSFPDD-ZR";
                    src.mPonType = c400gZrPon;
                }
            }


            src.mVendor = "Generic";


            if (mpCalData->GetTxTuningParams(src, dest, QsfpToBcm[lane].link_loss, rateStr, txparams) != ZOK)
            {
                INFN_LOG(SeverityLevel::error) << "Cannot find tom type generic in GetRxTuningParams, returning!" << endl;
                INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
                INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
                INFN_LOG(SeverityLevel::info) << "LinkLoss=" << QsfpToBcm[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

                return -1;
            }

        }


        if (lane < numOptLanes)
        {
            SetTomSerdesTxParams(QsfpToBcm[lane].tx_dev_id, QsfpToBcm[lane].tx_lane_num, txparams);

        }


        // Now tune Qsfp Tx (on TOM by sending proto) and tune BCM Rx (on BCM)
        // Tune on BCM, Rx <--- TOM direction (call gearbox)


        status = mpCalData->GetRxTuningParams(src, dest, QsfpToBcm[lane].link_loss, rateStr, rxparams);
        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::error) << "Error: GetRxTuningParams failed!\n";
            INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "LinkLoss=" << QsfpToBcm[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

            return -1;
        }

        SerdesDumpData serdesData(true, QsfpToBcm[lane].tx_dev_id, QsfpToBcm[lane].tx_lane_num,
                QsfpToBcm[lane].rx_dev_id, QsfpToBcm[lane].rx_lane_num, src, dest, txparams, rxparams,
                QsfpToBcm[lane].link_id, QsfpToBcm[lane].link_loss);
        serdesDump.push_back(serdesData);


        // AF0 todo - do we need this? there's nothing to tune on BCM line side facing TOM
        INFN_LOG(SeverityLevel::info) << "===Tune TOM to BCM links, is this needed for BCM line side? For AID=" << aId << " rate=" << rate;
        rc += SetBcmSerdesRxParams(QsfpToBcm[lane].rx_dev_id, false /*line*/, QsfpToBcm[lane].rx_lane_num, rxparams, force);
    }

    AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectUpdate(*mTomSerdesMap[tomIdx]);

    INFN_LOG(SeverityLevel::info) << "Updated redis object.";
    INFN_LOG(SeverityLevel::info) << "===End tune TOM to BCM links for AID=" << aId << " rate=" << rate;



    auto dumpElem = mSerdesDumpTomToBcm.find(tomId);


    if (dumpElem != mSerdesDumpTomToBcm.end())
    {
        mSerdesDumpTomToBcm.erase(dumpElem);
    }


    mSerdesDumpTomToBcm.insert(make_pair(tomId, serdesDump));





    return rc;
}

int SerdesTuner::TuneBcmToAtlLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int rc = 0;
    std::vector<SerdesDumpData> serdesDump;
    INFN_LOG(SeverityLevel::info) << "===Tune BCM Host to Atl for tomId=" << tomId << " rate=" << (unsigned int)rate << "====";

    unsigned int numLanes = 2;
    if (rate == gearbox::RATE_200GE)
    {
        numLanes = 4;
    }
    else if (rate == gearbox::RATE_400GE)
    {
        numLanes = 8;
    }
    else if (rate == gearbox::RATE_4_100GE)
    {
        numLanes = 2;
    }

    LinkData BcmToAtl[numLanes];
    memset(BcmToAtl, 0, (sizeof(tuning::LinkData) * numLanes));

    // LUT only has values for rate of 400GE
    std::string rateStr = cGaui8;
    unsigned int startingLane = GetBcmToAtlLaneOffset(tomId, rate);
    unsigned int bcmNum       = GetBcmToAtlNumHost(tomId);

    for (uint32 lane = 0; lane < numLanes; ++lane)
    {
        INFN_LOG(SeverityLevel::info) << "TomID=" << tomId << " Table Lane offset=" << lane + startingLane << endl;
        GetLinkData(Bcm_to_Atl_connections, kLinksFromBcmHostToAtlHost, bcmNum, (lane + startingLane), true /*Tx*/, &BcmToAtl[lane]);
        mpCalData->GetLinkLoss(BcmToAtl[lane].link_id, BcmToAtl[lane].link_loss);
        DumpLinkData(&BcmToAtl[lane]);

        dp::DeviceInfo src  { "BCM", "HOST" , "BRCM" };
        dp::DeviceInfo dest { "ATL", "HOST" , "BRCM" };

        std::map<std::string, sint32> txparams;
        std::map<std::string, sint32> rxparams;

        INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Searching for LUT data for BCM " << BcmToAtl[lane].tx_dev_id << " to Atlantic in GetTxTuningParams." << endl;
        INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToAtl[lane].link_loss << " lane=" << lane << endl;

        ZStatus status = mpCalData->GetTxTuningParams(src, dest, BcmToAtl[lane].link_loss, rateStr, txparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::info) << "Cannot find BCM " << BcmToAtl[lane].tx_dev_id << " to Atlantic in GetTxTuningParams." << endl;
            return -1;
        }

        // Tune BCM Tx (host) to Atlantic (host)
        rc += SetBcmSerdesTxParams(BcmToAtl[lane].tx_dev_id, true /*host Tx*/, BcmToAtl[lane].tx_lane_num, txparams);

        status = mpCalData->GetRxTuningParams(src, dest, BcmToAtl[lane].link_loss, rateStr, rxparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Can not find RxTuningParams!" << endl;
            INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "LinkLoss=" << BcmToAtl[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

            return -1;
        }

        // Save serdes params programmed into vector for each lane
        SerdesDumpData serdesData(true, BcmToAtl[lane].tx_dev_id, BcmToAtl[lane].tx_lane_num,
                BcmToAtl[lane].rx_dev_id, BcmToAtl[lane].rx_lane_num, src, dest, txparams, rxparams,
                BcmToAtl[lane].link_id, BcmToAtl[lane].link_loss);

        serdesDump.push_back(serdesData);

        SetAtlSerdesRxParams(BcmToAtl[lane].rx_lane_num, rxparams);
    }

    // Save serdes params into map for dump
    auto elem = mSerdesBcmHostToAtl.find(tomId);


    if (elem != mSerdesBcmHostToAtl.end())
    {
        mSerdesBcmHostToAtl.erase(elem);
    }

    INFN_LOG(SeverityLevel::info) << "Bcm to Atl serdes size=" << serdesDump.size();
    mSerdesBcmHostToAtl.insert(make_pair(tomId, serdesDump));

    return rc;
}

int SerdesTuner::TuneAtlToBcmLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int rc = 0;
    std::vector<SerdesDumpData> serdesDump;
    INFN_LOG(SeverityLevel::info) << "===Tune Atlantic Host to BCM Host for tomId=" << tomId << " rate=" << (unsigned int)rate << "====";

    // Default 100GE lanes
    unsigned int numLanes = cAtlNumLanes100Ge;
    if (rate == gearbox::RATE_200GE)
    {
        numLanes = cAtlNumLanes200Ge;
    }
    else if (rate == gearbox::RATE_400GE)
    {
        numLanes = cAtlNumLanes400Ge;
    }
    else if (rate == gearbox::RATE_OTU4)
    {
        numLanes = cAtlNumLanesOtu4;
    }
    else if (rate == gearbox::RATE_4_100GE)
    {
        numLanes = cAtlNumLanes100Ge;
    }

    LinkData AtlToBcm[numLanes];
    memset(AtlToBcm, 0, (sizeof(tuning::LinkData) * numLanes));

    // LUT only has values for rate of 400GE
    std::string rateStr = cGaui8;
    unsigned int startingLane = GetAtlToBcmLaneOffset(tomId, rate);
    unsigned int bcmNum       = GetBcmToAtlNumHost(tomId);

    for (uint32 lane = 0; lane < numLanes; ++lane)
    {
        INFN_LOG(SeverityLevel::info) << "TomID=" << tomId << " Lane=" << lane + startingLane << " bcmNum=" << bcmNum << " tableSize=" << kLinksFromAtlHostToBcmHost << endl;
        GetLinkData(Atl_to_Bcm_connections, kLinksFromAtlHostToBcmHost, bcmNum, (lane + startingLane), false /*Rx*/, &AtlToBcm[lane]);
        mpCalData->GetLinkLoss(AtlToBcm[lane].link_id, AtlToBcm[lane].link_loss);
        DumpLinkData(&AtlToBcm[lane]);

        dp::DeviceInfo src  { "ATL", "HOST" , "BRCM" };
        dp::DeviceInfo dest { "BCM", "HOST" , "BRCM" };

        std::map<std::string, sint32> txparams;
        std::map<std::string, sint32> rxparams;

        INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
        INFN_LOG(SeverityLevel::info) << "Searching for LUT data for BCM " << AtlToBcm[lane].tx_dev_id << " to Atlantic in GetTxTuningParams." << endl;
        INFN_LOG(SeverityLevel::info) << "LinkLoss=" << AtlToBcm[lane].link_loss << " lane=" << lane << endl;

        ZStatus status = mpCalData->GetTxTuningParams(src, dest, AtlToBcm[lane].link_loss, rateStr, txparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::info) << "Cannot find BCM " << AtlToBcm[lane].tx_dev_id << " to Atlantic in GetTxTuningParams." << endl;
            return -1;
        }

        // Tune Atlantic Tx to BCM Rx serdes
        SetAtlSerdesTxParams(AtlToBcm[lane].tx_lane_num, txparams);

        status = mpCalData->GetRxTuningParams(src, dest, AtlToBcm[lane].link_loss, rateStr, rxparams);

        if (status != ZOK)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Can not find RxTuningParams!" << endl;
            INFN_LOG(SeverityLevel::info) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;
            INFN_LOG(SeverityLevel::info) << "LinkLoss=" << AtlToBcm[lane].link_loss << " lane=" << lane << " rateStr=" << rateStr << endl;

            return -1;
        }

        SerdesDumpData serdesData(true, AtlToBcm[lane].tx_dev_id, AtlToBcm[lane].tx_lane_num,
                AtlToBcm[lane].rx_dev_id, AtlToBcm[lane].rx_lane_num, src, dest, txparams, rxparams,
                AtlToBcm[lane].link_id, AtlToBcm[lane].link_loss);
        serdesDump.push_back(serdesData);


        // AF0 remove
        //rc += SetBcmSerdesRxParams(AtlToBcm[lane].rx_dev_id, true /*host*/, AtlToBcm[lane].rx_lane_num, rxparams);

    }


    auto elem = mSerdesAtlToBcmHost.find(tomId);

    if (elem != mSerdesAtlToBcmHost.end())
    {
        mSerdesAtlToBcmHost.erase(elem);
    }
    INFN_LOG(SeverityLevel::info) << "AtlToBcm sedesDump size=" << serdesDump.size();
    mSerdesAtlToBcmHost.insert(make_pair(tomId, serdesDump));

    return rc;
}

int SerdesTuner::PushAtlToBcmRxParamsToHw(unsigned int port, gearbox::Bcm81725DataRate_t rate)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    int rc = 0;
    std::map<unsigned int, std::vector<SerdesDumpData>> dump;
    GetSerdesDumpData(dump, AtlToBcm);
    auto it = dump.find(port);

    if (it != dump.end() && it->second.size() > 0)
    {
        for (unsigned int i = 0; i < it->second.size(); i++)
        {
            rc += SetBcmSerdesRxParams(it->second[i].mDestDeviceNum, true, it->second[i].mDestLaneNum, it->second[i].mRxParams);
        }
    }
    return rc;
}

unsigned int SerdesTuner::GetAtlBcmSerdesTxOffset(unsigned int portId, gearbox::Bcm81725DataRate_t rate, unsigned int lane)
{
    if ((rate == gearbox::RATE_100GE) ||
        (rate == gearbox::RATE_OTU4)  ||
        (rate == gearbox::RATE_4_100GE))
    {
        return atlPortLanes100Ge[portId].laneNum[lane];
    }
    else if (rate == gearbox::RATE_200GE)
    {
        return atlPortLanes200Ge[portId].laneNum[lane];
    }
    else if (rate == gearbox::RATE_400GE)
    {
        return atlPortLanes400Ge[portId].laneNum[lane];
    }
    else
    {
        // default 100GE
        return atlPortLanes100Ge[portId].laneNum[lane];
    }
}


bool SerdesTuner::GetAtlBcmMapParam(std::string aId, gearbox::Bcm81725DataRate_t portRate, std::vector<AtlBcmSerdesTxCfg> & atlBcmTbl)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    unsigned int tomId;
    // Convert AID to laneNum
    tomId = gearbox::aid2PortNum(aId);
    unsigned int totalLanes = 0;
    unsigned int laneNum = 0;

    if (portRate == gearbox::RATE_100GE ||
        portRate == gearbox::RATE_4_100GE)
    {
        totalLanes = cAtlNumLanes100Ge;
    }
    else if (portRate == gearbox::RATE_400GE)
    {
        totalLanes = cAtlNumLanes400Ge;
    }
    else if (portRate == gearbox::RATE_200GE)
    {
        totalLanes = cAtlNumLanes200Ge;
    }
    else if (portRate == gearbox::RATE_OTU4)
    {
        totalLanes = cAtlNumLanesOtu4;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "Cannot get atlBcmParam num lanes - unknown port rate=" << portRate << endl;
        return false;
    }

    // Offset the tomId which is the index for the AtlPortLanes100GE table
    // AtlPortLanes100GE is written for 100G, so for other rates which use more lanes and occupy
    //  other ports for 100G, we will offset our starting index back

    for (unsigned int i = 0; i < totalLanes; i++)
    {
        laneNum = GetAtlBcmSerdesTxOffset(tomId, portRate, i);

        auto elem = mAtlBcmSerdesTxCfg.find(laneNum);

        if (elem == mAtlBcmSerdesTxCfg.end())
        {
            INFN_LOG(SeverityLevel::error) << "Can not find Atl serdes values for tomId=" << tomId << " rate=" << portRate << " laneNum=" << laneNum;
            return false;
        }

        INFN_LOG(SeverityLevel::info) << "TomId=" << tomId << " pushing back..." << elem->second.txPre1 << " " << elem->second.txPre2 << " " << elem->second.txMain <<
                elem->second.txPost1 << " " << elem->second.txPost2 << " " << elem->second.txPost3;
        atlBcmTbl.push_back(elem->second);
    }

    return true;
}

void SerdesTuner::GetAtlBcmMapParams(std::map<unsigned int, AtlBcmSerdesTxCfg> & atlBcmMap)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    atlBcmMap = mAtlBcmSerdesTxCfg;
}

void SerdesTuner::GetSerdesDumpData(std::map<unsigned int, std::vector<SerdesDumpData>> & dump, LinkType_t link)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    std::map<unsigned int, std::vector<SerdesDumpData>>  emptyMap;

    switch (link)
    {
    case BcmLineToBcmHost:
        dump = mSerdesBcmLineToBcmHost;
        break;
    case BcmHostToBcmLine:
        dump = mSerdesBcmHostToBcmLine;
        break;
    case BcmToTom:
        dump = mSerdesDumpBcmToTom;
        break;
    case TomToBcm:
        dump = mSerdesDumpTomToBcm;
        break;
    case BcmToAtl:
        dump = mSerdesBcmHostToAtl;
        break;
    case AtlToBcm:
        dump = mSerdesAtlToBcmHost;
        break;
    default:
        dump = emptyMap;
        break;
    }

}



void SerdesTuner::GetLinkData(const tuning::LinkData* allLinkData, const size_t totalSize,
                              uint32 qsfpId, uint32 laneNum, bool tx, tuning::LinkData* fillLinkData)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    for (uint32 i=0; i<totalSize; ++i)
    {
        if (tx)
        {
            if (allLinkData[i].tx_dev_id == qsfpId && allLinkData[i].tx_lane_num == laneNum)
            {
                fillLinkData->link_id = allLinkData[i].link_id;
                fillLinkData->tx_dev_id = allLinkData[i].tx_dev_id;
                fillLinkData->tx_lane_num = allLinkData[i].tx_lane_num;
                fillLinkData->rx_dev_id = allLinkData[i].rx_dev_id;
                fillLinkData->rx_lane_num = allLinkData[i].rx_lane_num;
                fillLinkData->link_loss = 0;

            }
        }
        else
        {
            if (allLinkData[i].rx_dev_id == qsfpId && allLinkData[i].rx_lane_num == laneNum)
            {
                fillLinkData->link_id = allLinkData[i].link_id;
                fillLinkData->tx_dev_id = allLinkData[i].tx_dev_id;
                fillLinkData->tx_lane_num = allLinkData[i].tx_lane_num;
                fillLinkData->rx_dev_id = allLinkData[i].rx_dev_id;
                fillLinkData->rx_lane_num = allLinkData[i].rx_lane_num;
                fillLinkData->link_loss = 0;
            }
        }
    }
}

void SerdesTuner::DumpLinkData(tuning::LinkData* linkData)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    INFN_LOG(SeverityLevel::info) << "LinkId: "  << linkData->link_id << " Loss: " << linkData->link_loss;
    INFN_LOG(SeverityLevel::info) << " TxDevId: " << linkData->tx_dev_id << " TxLane: " << linkData->tx_lane_num;
    INFN_LOG(SeverityLevel::info) << " RxDevId: " << linkData->rx_dev_id << " RxLane: " << linkData->rx_lane_num << endl;
}


unsigned int SerdesTuner::TomAid2PortNum(const std::string & aidStr)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    // Example: aidStr = 1-6-1 .. 1-6-16
    // This has no T in the AID

    if (aidStr.empty())
    {
        return -1;
    }

    std::string sId;
    std::size_t pos = aidStr.rfind('-');
    if (pos != std::string::npos)
    {
        sId = aidStr.substr(pos+1);
    }
    else
    {
        return -1;
    }

    return (stoi(sId, nullptr));
}

void SerdesTuner::TomType2Lut(std::string boardType, std::string boardPonType, std::string& vendor,
                              std::string &boardTypeLut, std::string& boardPonTypeLut, std::string& vendorLut)
{
    // Lock
    //std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);


    boardPonTypeLut = boardPonType;
    boardTypeLut = boardType;


    if (vendor.find("Sumitomo") != std::string::npos)
    {
        vendorLut = "Sumitomo";
    }
    else if (vendor.find("TIMBERCON") != std::string::npos)
    {
        vendorLut = "Timbercon";
    }
    else if (vendor.find("FINISAR") != std::string::npos)
   {
        vendorLut = "Finisar";
    }
    else if (vendor.find("Oclaro") != std::string::npos)
    {
        vendorLut = "Oclaro";
    }
    else if (vendor.find("KAIAM") != std::string::npos)
    {
        vendorLut = "Kaiam";
    }
    else if (vendor.find("LUMENTUM") != std::string::npos)
    {
        vendorLut = "Lumentum";
    }
    else if (vendor.find("INNOLIGHT") != std::string::npos)
    {
        vendorLut = "Innolight";
    }
    else if (vendor.find("TE Connectivity") != std::string::npos)
    {
        vendorLut = "TE-Connectivity";
    }
    else if (vendor.find("AOI") != std::string::npos)
    {
        vendorLut = "AOI";
    }
    else if (vendor.find("CIG") != std::string::npos)
    {
        vendorLut = "CIG";
    }
    else if (vendor.find("LINKTEL") != std::string::npos)
    {
        vendorLut = "LinkTel";
    }
    else if (vendor.find("Hisense") != std::string::npos)
    {
        vendorLut = "Hisense";
    }
    else if (vendor.find("ACCELINK") != std::string::npos)
    {
        vendorLut = "Accelink";
    }
    else if (vendor.find("Acacia Comm Inc.") != std::string::npos)
    {
        vendorLut = "Acacia";
    }
    else if (vendor.find("INPHI") != std::string::npos)
    {
        vendorLut = "Inphi";
    }
    else if (vendor.find("OPLINK") != std::string::npos)
    {
        vendorLut = "Oplink";
    }
    else
    {
        vendorLut = vendor;
    }
}

bool SerdesTuner::GetRedisTomState(std::string aId)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    lccmn_tom::TomState stateData;
    int tomType;
    string boardType;
    string boardPonType;
    string boardSubType;
    string ponType;
    string vendor;
    unsigned int numOptLanes = 0;
    unsigned int infoGood = 0;

    INFN_LOG(SeverityLevel::info) << "Getting Chm6::TomState for aid=" << aId;

    unsigned int tomId = gearbox::aid2PortNum(aId);
    // Convert manager AID (1-6-T5) to TomState AID (1-6-5)
    size_t pos = aId.find(".");
    if(pos != std::string::npos)
    {
        aId = aId.substr(0,pos);
    }
    aId.erase(remove(aId.begin(), aId.end(), 'T'), aId.end());


    stateData.mutable_base_state()->mutable_config_id()->set_value(aId);

    INFN_LOG(SeverityLevel::info) << "tomId=" << tomId;

    auto elem = mQsfpState.find(tomId);

    if (elem != mQsfpState.end())
    {
        // erase old value
        mQsfpState.erase(elem);
    }

    try
    {
        AppServicerIntfSingleton::getInstance()->getRedisInstance()->RedisObjectRead(stateData);


        string tomState;
        MessageToJsonString(stateData, &tomState);

        INFN_LOG(SeverityLevel::info) << "stateData=" << tomState << endl;

        aId = stateData.base_state().config_id().value();



        if (stateData.has_hal())
        {
            INFN_LOG(SeverityLevel::info) << "has hal!" << endl;

            tomType = (int)stateData.hal().tom_type();

            if (stateData.hal().has_tom_board_type())
            {
                boardType = stateData.hal().tom_board_type().value();
                infoGood++;
                INFN_LOG(SeverityLevel::info) << "has board_type!" << endl;
            }


            if (stateData.hal().has_tom_board_subtype())
            {
                boardSubType = stateData.hal().tom_board_subtype().value();
                infoGood++;
                INFN_LOG(SeverityLevel::info) << "has subType" << endl;
            }

            if (stateData.hal().has_tom_vendor_info())
            {
                ponType = stateData.hal().tom_vendor_info().pon().value();
                infoGood++;
                INFN_LOG(SeverityLevel::info) << "has ponType" << endl;
            }

            if (stateData.hal().has_tom_vendor_info())
            {
                vendor = stateData.hal().tom_vendor_info().vendor_name().value();
                infoGood++;
                INFN_LOG(SeverityLevel::info) << "has vendor info" << endl;
            }

            if (stateData.hal().has_num_optical_lanes())
            {
                numOptLanes = stateData.hal().num_optical_lanes().value();
                infoGood++;
                INFN_LOG(SeverityLevel::info) << "has num optical lanes" << endl;
            }
        }

        // infoGood = 5 means we have board type, subType and vendor info
        if (infoGood == 5)
        {
            string boardTypeLut, boardPonTypeLut, vendorLut;

            TomType2Lut(boardType, boardPonType, vendor, boardTypeLut, boardPonTypeLut, vendorLut);
            dp::DeviceInfo devInfo{ boardTypeLut, ponType , vendorLut };
            TomState ts(devInfo, numOptLanes);
            mQsfpState.insert(make_pair(tomId, ts));
            INFN_LOG(SeverityLevel::info) << "aid=" << aId << " infoGood!" << " tomId=" << tomId << endl;


            INFN_LOG(SeverityLevel::info) << "Processed info - aid=" << aId << " tomType=" << (int)tomType << " boardType=" << boardTypeLut << " boardPonType=" << boardPonTypeLut <<
                    " vendor=" << vendorLut << endl;
        }

        INFN_LOG(SeverityLevel::info) << " Raw info - aid=" << aId << " tomType=" << (int)tomType << " boardType=" << boardType << " boardPonType=" << boardPonType <<
                " vendor=" << vendor << endl;
    }
    catch(std::exception const &excp)
    {
        INFN_LOG(SeverityLevel::info) << "Exception caught in ProcessBoardConfigInDb(): " << excp.what()
                       << " - Redis DB may NOT have Chm6TomState message.";
        return false;
    }
    catch(...)
    {
        INFN_LOG(SeverityLevel::info) << "Redis DB may NOT have Chm6TomState message.";

        return false;
    }


    return true;
}

bool SerdesTuner::GetBcmHostToBcmLineSerdesParams(unsigned int port, unsigned int rate, const LinkData ** linkData, unsigned int& totalLanes)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    bool found = false;

    if ( ((rate == gearbox::RATE_100GE) ||
          (rate == gearbox::RATE_OTU4))
            &&
         (port == gearbox::PORT3 ||
          port == gearbox::PORT4  ||
          port == gearbox::PORT5  ||
          port == gearbox::PORT6  ||
          port == gearbox::PORT11 ||
          port == gearbox::PORT12 ||
          port == gearbox::PORT13 ||
          port == gearbox::PORT14) )
    {
        for (unsigned int i = 0; i < kLinksFromBcmHostToBcmLine; i++)
        {
            if ( (port == gearbox::PORT3 && i == 0) ||
                 (port == gearbox::PORT4 && i == 2) ||
                 (port == gearbox::PORT5 && i == 4) ||
                 (port == gearbox::PORT6 && i == 6) ||
                 (port == gearbox::PORT11 && i == 8) ||
                 (port == gearbox::PORT12 && i == 10) ||
                 (port == gearbox::PORT13 && i == 12) ||
                 (port == gearbox::PORT14 && i == 14) )
            {
                *linkData = &BcmHost_to_BcmLine_connections[i];
                totalLanes = c100gNumBcmToBcmLanes;
                found = true;
                break;
            }
        }
    }
    else if (rate == gearbox::RATE_400GE)
    {
       // NA
    }
    return found;
}


bool SerdesTuner::GetBcmToTomSerdesParams(unsigned int port, unsigned int rate, const LinkData ** linkData, unsigned int& totalLanes)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    bool found = false;
    if ((rate == gearbox::RATE_100GE) ||
        (rate == gearbox::RATE_OTU4))
    {
        for (unsigned int i = 0; i < kLinksFromBcmToQsfp28; i++)
        {
            if (BCM_to_qsfp28_connections[i].rx_dev_id == port)
            {
                *linkData = &BCM_to_qsfp28_connections[i];
                totalLanes = c100gNumLanes;
                found = true;
                break;
            }
        }
    }
    else if (rate == gearbox::RATE_400GE)
    {
        for (unsigned int i = 0; i < kLinksFromBcmToQsfpdd_400GE; i++)
        {
            if (BCM_to_qsfpdd_connections_400GE[i].rx_dev_id == port)
            {
                *linkData = &BCM_to_qsfpdd_connections_400GE[i];
                totalLanes = c400gNumLanes;
                found = true;
                break;
            }
        }
    }
    else if (rate == gearbox::RATE_4_100GE)
    {
        for (unsigned int i = 0; i < kLinksFromBcmToQsfpdd_4_100GE; i++)
        {
            if (BCM_to_qsfpdd_connections_4_100GE[i].rx_dev_id == port)
            {
                *linkData = &BCM_to_qsfpdd_connections_4_100GE[i];
                totalLanes = c4_100gNumLanes;
                found = true;
                break;
            }
        }
    }
    return found;
}

bool SerdesTuner::GetTomToBcmSerdesParams(unsigned int port, unsigned int rate, const LinkData ** linkData, unsigned int& totalLanes)
{
    // Lock
    std::lock_guard<std::recursive_mutex> lock(mSerdesTunerPtrMtx);

    bool found = false;
    if ((rate == gearbox::RATE_100GE) ||
        (rate == gearbox::RATE_OTU4))
    {
        for (unsigned int i = 0; i < kLinksFromQsfp28ToBcm; i++)
        {
            if (Qsfp28_to_BCM_connections[i].tx_dev_id == port)
            {
                *linkData = &Qsfp28_to_BCM_connections[i];
                totalLanes = c100gNumLanes;
                found = true;
                break;
            }
        }
    }
    else if (rate == gearbox::RATE_400GE)
    {
        for (unsigned int i = 0; i < kLinksFromQsfpddToBcm_400GE; i++)
        {
            if (Qsfpdd_to_BCM_connections_400GE[i].tx_dev_id == port)
            {
                *linkData = &Qsfpdd_to_BCM_connections_400GE[i];
                totalLanes = c400gNumLanes;
                found = true;
                break;
            }
        }
    }
    else if (rate == gearbox::RATE_4_100GE)
    {
        for (unsigned int i = 0; i < kLinksFromQsfpddToBcm_4_100GE; i++)
        {
            if (Qsfpdd_to_BCM_connections_4_100GE[i].tx_dev_id == port)
            {
                *linkData = &Qsfpdd_to_BCM_connections_4_100GE[i];
                totalLanes = c4_100gNumLanes;
                found = true;
                break;
            }
        }
    }
    return found;
}

unsigned int SerdesTuner::GetNumOpticalLanes(unsigned int port)
{
    unsigned int numOptLanes = 0;

    auto elem = mQsfpState.find(port);
    if (elem != mQsfpState.end())
    {
        numOptLanes   = elem->second.mNumOpticalLanes;
    }

    return numOptLanes;
}


} // end namespace tuning
