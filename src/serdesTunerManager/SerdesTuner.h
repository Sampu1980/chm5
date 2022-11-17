#ifndef SerdesTuner_h
#define SerdesTuner_h

#include <map>
#include <infinera/chm6/common/v2/tom_serdes.pb.h>
#include <infinera/lccmn/dataplane/v1/tom_serdes.pb.h>
#include <infinera/chm6/tom/v3/tom_state.pb.h>
#include "SerdesCalDataGen6.h"
#include "SingletonService.h"
#include "types.h"
#include "chm6/redis_adapter/application_servicer.h"
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include "GearBoxUtils.h"
#include "DpProtoDefs.h"
#include "AtlanticSerdesTable.h"

//using namespace serdesCalDataGen6;
//namespace chm6_tom    = infinera::chm6::tom::v3;
namespace tuning
{
    struct LinkData
    {
        uint16 tx_dev_id;
        uint16 tx_lane_num;
        uint16 rx_dev_id;
        uint16 rx_lane_num;
        uint16 link_id;
        uint32 link_loss;
    };

    typedef enum TomTypes
    {
        TOM_TYPE_UNSPECIFIED = 0,
        TOM_TYPE_QSFP28      = 1,
        TOM_TYPE_QSFPDD      = 2,
        TOM_TYPE_QSFP56      = 3
    } TomTypes_t;

    typedef enum BcmNum
    {
        // Top Mz
        BcmNum1 = 1,
        BcmNum2,
        BcmNum3,

        // Bottom Mz (Bcm 1, 2, 3)
        BcmNum4,
        BcmNum5,
        BcmNum6
    } BcmNum_t;

    // 3 BCMs per mezz
    const unsigned int cBcmNumTotal  = 3;

    typedef enum LinkTypes
    {
        BcmLineToBcmHost,
        BcmHostToBcmLine,
        BcmToTom,
        TomToBcm,
        BcmToAtl,
        AtlToBcm,
        NumLinks
    }LinkType_t;



   struct SerdesDumpData
   {
        bool mValid;
        unsigned int mSrcDeviceNum;
        unsigned int mSrcLaneNum;
        unsigned int mDestDeviceNum;
        unsigned int mDestLaneNum;
        dp::DeviceInfo mSrc;
        dp::DeviceInfo mDest;
        std::map<std::string,sint32> mTxParams;
        std::map<std::string,sint32> mRxParams;
        unsigned int mLinkId;
        unsigned int mLinkLoss;
        SerdesDumpData() : mValid(false), mSrcDeviceNum(0), mSrcLaneNum(0),
                mDestDeviceNum(0), mDestLaneNum(0), mLinkId(0), mLinkLoss(0),
                mSrc("", "", ""), mDest("", "", "")
        {

        }
        SerdesDumpData(bool valid, unsigned int srcDeviceNum,  unsigned int srcLaneNum,
                       unsigned int destDeviceNum, unsigned int destLaneNum,
                       dp::DeviceInfo src, dp::DeviceInfo dest,
                       std::map<std::string,sint32> txParams, std::map<std::string,sint32> rxParams,
                       unsigned int linkId, unsigned int linkLoss) :
                           mValid(valid), mSrcDeviceNum(srcDeviceNum), mSrcLaneNum(srcLaneNum),
                           mDestDeviceNum(destDeviceNum), mDestLaneNum(destLaneNum),
                           mSrc(src.mDevice, src.mPonType, src.mVendor), mDest(dest.mDevice, dest.mPonType, dest.mVendor),
                           mTxParams(txParams), mRxParams(rxParams), mLinkId(linkId), mLinkLoss(linkLoss)
        {

        }
   };

   struct TomState
   {
       dp::DeviceInfo mDevInfo;
       unsigned int mNumOpticalLanes;

       TomState() : mDevInfo("", "", ""), mNumOpticalLanes(0)
       {

       }

       TomState(dp::DeviceInfo devInfo, unsigned int numOpticalLanes) :
                mDevInfo(devInfo.mDevice, devInfo.mPonType, devInfo.mVendor), mNumOpticalLanes(numOpticalLanes)
       {

       }


   };




class SerdesTuner
{
public:
	SerdesTuner();
	virtual ~SerdesTuner();

	bool Initialize();

	int TuneAtlToBcmLinks(std::string aId, gearbox::Bcm81725DataRate_t rate);
	int TuneLinks(std::string aId, bool onCreate);
	int TuneLinks(std::string aId, gearbox::Bcm81725DataRate_t rate, bool onCreate);

    //Segment BCM->TOM and TOM->BCM
	int TuneBcmToTomLinks(std::string aId, gearbox::Bcm81725DataRate_t rate, bool force=false);
	int TuneTomToBcmLinks(std::string aId, gearbox::Bcm81725DataRate_t rate, bool force=false);

    //Segment BCM <-> BCM
	int TuneBcmHostToBcmLineLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    int TuneBcmLineToBcmHostLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);


    //Segment BCM->ATL and ATL->BCM
    int TuneBcmToAtlLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    int TuneAtlToBcmLinks(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    int PushAtlToBcmRxParamsToHw(unsigned int port, gearbox::Bcm81725DataRate_t rate);

    void GetAtlParams(){}

    void GetLinkData(const tuning::LinkData* allLinkData, const size_t totalSize,
                     uint32 qsfpId, uint32 laneNum, bool tx, tuning::LinkData* fillLinkData);

    unsigned int TomAid2PortNum(const std::string & aidStr);
    void TomType2Lut(std::string boardType, std::string boardSubType, std::string& vendor,
                     std::string& boardTypeLut, std::string& boardSubTypeLut, std::string& vendorLut);

    std::string rate2SerdesCalDataString(unsigned int rate);

    void GetAtlBcmMapParams(std::map<unsigned int, AtlBcmSerdesTxCfg> & bcmAtlMap);
    bool GetAtlBcmMapParam(std::string aId, gearbox::Bcm81725DataRate_t portRate, std::vector<AtlBcmSerdesTxCfg> & atlBcmTbl);

    // Dumps
    void GetSerdesDumpData(std::map<unsigned int, std::vector<SerdesDumpData>> & dump, LinkType_t link);

    void GetTxParamsFromMap(std::map<std::string,sint32> TxParams, int& pre2, int& pre,
                                         int& main, int & post, int& post2, int & post3);
    void GetRxParamsFromMap(std::map<std::string,sint32> RxParams, std::map<unsigned int, int64_t>& rxValue);


    bool GetBcmHostToBcmLineSerdesParams(unsigned int port, unsigned int rate, const LinkData ** linkData, unsigned int& totalLanes);
    bool GetBcmToTomSerdesParams(unsigned int port, unsigned int rate, const LinkData ** linkData, unsigned int& totalLanes);
    bool GetTomToBcmSerdesParams(unsigned int port, unsigned int rate, const LinkData ** linkData, unsigned int& totalLanes);

    unsigned int GetAtlBcmSerdesTxOffset(unsigned int portId, gearbox::Bcm81725DataRate_t rate, unsigned int lane);
    unsigned int GetNumOpticalLanes(unsigned int port);
private:
    //Segment BCM <-> BCM
    bool IsBcmToBcmValidPort(unsigned int tomId);
    unsigned int GetBcmToBcmHostLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    unsigned int GetBcmToBcmLineToHostNum(unsigned int tomId);


    //Segment BCM->ATL and ATL->BCM, get lane offsets in table
    unsigned int GetBcmToTomLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    unsigned int GetTomToBcmLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    unsigned int GetBcmToAtlLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    unsigned int GetAtlToBcmLaneOffset(unsigned int tomId, gearbox::Bcm81725DataRate_t rate);
    unsigned int GetBcmToAtlNumHost(unsigned int tomId);


    bool GetRedisTomState(std::string aId);
    int SetBcmSerdesTxParams(uint16 bcmNum, bool host, uint16 laneNum, std::map<std::string,sint32> TxParams, bool force=false);
    void SetTomSerdesRxParams(uint16 tomId,  uint16 laneNum, std::map<std::string,sint32> RxParams);

    void SetTomSerdesTxParams(uint16 tomId, uint16 laneNum,
                                           std::map<std::string,sint32> TxParams);


    int SetBcmSerdesRxParams(uint16 bcmNum, bool host, uint16 laneNum, std::map<std::string, sint32> RxParams, bool force=false);

    void SetAtlSerdesTxParams(uint16 laneNum, std::map<std::string,sint32> TxParams);
    void SetAtlSerdesRxParams(uint16 laneNum, std::map<std::string,sint32> RxParams);

    void GetGearboxBcmParams(uint16 bcmNum, bool host, uint16& bus, uint16& bcmUnit, uint16& side);
	void DumpLinkData(tuning::LinkData* linkData);



	serdesCalDataGen6::SerdesCalDataGen6 * mpCalData;

	std::vector<lccmn_dataplane::TomSerdesMapState *>  mTomSerdesMap;
	std::vector<lccmn_tom::TomState *> mTomState;


	static const std::string cTxMain;
	static const std::string cTxPost1;
	static const std::string cTxPost2;
	static const std::string cTxPost3;
	static const std::string cTxPre1;
	static const std::string cTxPre2;
	static const std::string cTx_InEQ;
	static const std::string cAutPeaking_En;
	static const std::string cCaui4;
	static const std::string cGaui4;
	static const std::string cGaui8;
	static const std::string cOtu4;
	static const uint16      cAtlNumLanes;
	std::map<unsigned int, TomState> mQsfpState;
	std::map<unsigned int, AtlBcmSerdesTxCfg> mAtlBcmSerdesTxCfg;
	std::map<unsigned int, std::vector<SerdesDumpData>> mSerdesDumpBcmToTom;
	std::map<unsigned int, std::vector<SerdesDumpData>> mSerdesDumpTomToBcm;
	std::map<unsigned int, std::vector<SerdesDumpData>> mSerdesBcmHostToBcmLine;
	std::map<unsigned int, std::vector<SerdesDumpData>> mSerdesBcmLineToBcmHost;

	std::map<unsigned int, std::vector<SerdesDumpData>> mSerdesBcmHostToAtl;
	std::map<unsigned int, std::vector<SerdesDumpData>> mSerdesAtlToBcmHost;

	std::recursive_mutex       mSerdesTunerPtrMtx;
	static const std::string c100gPon;
	static const std::string c400gPon;
	static const std::string c400gZrPon;
	//std::map<unsigned int, SerdesDataDevice> mSerdesDump;

};

typedef SingletonService<SerdesTuner> SerdesTunerSingleton;

} // tuning

#endif //SerdesTuner_h
