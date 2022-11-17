/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DP_GCC_MGR_H
#define DP_GCC_MGR_H

#include <memory>
#include <mutex>
#include <google/protobuf/message.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <linux/if_ether.h>

#include "DpObjectMgr.h"
#include "GccConfigHandler.h"
#include <SingletonService.h>
#include "DcoGccControlAdapter.h"
#include <SingletonService.h>

namespace DataPlane
{
    class DpGccMgr;

enum LldpDir {
    LLDP_DIR_RX = 0,
    LLDP_DIR_TX
};

struct lldp_ethhdr {
	unsigned char	enc_h_dest[ETH_ALEN];
	unsigned char	enc_h_source[ETH_ALEN];
	__be16		enc_h_proto;
        uint8_t         enc_seq_num;
        uint8_t         enc_key;
        unsigned char	h_dest[ETH_ALEN];
	unsigned char	h_source[ETH_ALEN];
	__be16		h_proto;
} __attribute__((packed));

struct encap_ethhdr {
	unsigned char	h_dest[ETH_ALEN];
	unsigned char	h_source[ETH_ALEN];
	__be16		h_proto;
        uint8_t         seq_num;
        uint8_t         key;
} __attribute__((packed));


class LldpFrame {
public:
  LldpFrame();
  virtual ~LldpFrame();
  unsigned char * GetLldpFrame(uint16_t &len, uint8_t &serdeLaneNum, LldpDir &dir);
  void Dump(std::ostream& os);
private:
  friend class DpGccMgr;
  void FreeFrameBuf();
  void StoreEthFrame(unsigned char *frame , uint16_t len, uint16_t portId, LldpDir dir);
  uint16_t      mLen ;
  uint8_t       mSerdeLaneNum;
  LldpDir       mDir;
  unsigned char * mEthBuf;

};

//LldpPktKey : portId , direction
typedef pair <uint16_t, LldpDir > LldpKey;
typedef map<LldpKey, LldpFrame> LldpFramePool;

class DpGccMgr : public DpObjectMgr
{
public:
    DpGccMgr();
    virtual ~DpGccMgr();
    virtual void initialize();
    virtual bool checkAndCreate(google::protobuf::Message* objMsg);
    virtual bool checkAndUpdate(google::protobuf::Message* objMsg);
    virtual bool checkAndDelete(google::protobuf::Message* objMsg);
    virtual bool checkOnResync (google::protobuf::Message* objMsg);

    virtual int  collectLldpFrame( LldpFramePool &pool );
    virtual void dumpActLldpPool( uint16_t portId, std::ostream& os );
    virtual void dumpLldp(std::ostream& os);
    DpAdapter::DcoGccControlAdapter* getAdPtr() { return mpGccAd; };
    std::shared_ptr<GccConfigHandler> getCfgHndlrPtr() { return mspGccCfgHndlr; };

protected:

    virtual void start();

    virtual void reSubscribe();

    virtual ConfigStatus processConfig(google::protobuf::Message* objMsg, ConfigType cfgType);

    virtual ConfigStatus completeConfig(google::protobuf::Message* objMsg,
                                        ConfigType cfgType,
                                        ConfigStatus status,
                                        std::ostringstream& log);

private:

    // Gcc PM Callback registration
    void registerGccPm();
    void unregisterGccPm();

    // Gcc Config Handler
    void createGccConfigHandler();

    std::shared_ptr<GccConfigHandler> mspGccCfgHndlr;
    DpAdapter::DcoGccControlAdapter*  mpGccAd;
    boost::thread                     mThrLldp;
    int                               mSd;
    uint32_t                          mLldpFramePoolIdx;
    LldpFramePool                     mLldpFramePools[2];
    std::mutex                        mMutex;
    int                               mCollectPktInterval;
    uint64_t                          mTotFrameCnt;
    uint64_t                          mSmallFrameCnt;
    uint64_t                          mRxGoodFrameCnt;
    uint64_t                          mTxGoodFrameCnt;
    uint64_t                          mDupFrameCnt;
    void createLldpCollector();
    void collectLldpFrameInt();
};

typedef SingletonService<DpGccMgr> DpGccMgrSingleton;

} // namespace DataPlane

#endif /* DP_GCC_MGR_H */
