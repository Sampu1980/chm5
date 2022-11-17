/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include <boost/bind.hpp>
#include <iostream>
#include <string>
#include <sstream>

#include "DpGccMgr.h"
#include "DataPlaneMgrLog.h"
#include "DpPmHandler.h"
#include "InfnLogger.h"
#include "DpProtoDefs.h"
#include "DataPlaneMgr.h"
#include "DcoConnectHandler.h"

#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/socket.h>
#include <linux/if_packet.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

using namespace ::std;
using google::protobuf::util::MessageToJsonString;


namespace DataPlane
{

#define ETH_P_EXPB5              (0x88B5)
#define COLL_PKT_INTVL           (1)
#define LLDP_IF_NAME                  "eth2"

DpGccMgr::DpGccMgr()
    : DpObjectMgr("Gcc", (1 << DCC_ZYNQ))
    , mpGccAd(NULL)
    , mLldpFramePoolIdx (0)
    , mCollectPktInterval(COLL_PKT_INTVL)
    , mTotFrameCnt(0)
    , mSmallFrameCnt(0)
    , mRxGoodFrameCnt(0)
    , mTxGoodFrameCnt(0)
    , mDupFrameCnt(0)
{

}

DpGccMgr::~DpGccMgr()
{
    if (mpGccAd)
    {
        delete mpGccAd;
    }

    unregisterGccPm();
}

void DpGccMgr::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Creating DcoGccControlAdapter";

    mpGccAd = new DpAdapter::DcoGccControlAdapter();
    createGccConfigHandler();

    createLldpCollector();

    mIsInitDone = true;
}

void DpGccMgr::start()
{
    INFN_LOG(SeverityLevel::info) << "Initializing DcoGccControlAdapter";

    if (mpGccAd->initializeCcVlan() == false)
    {
        INFN_LOG(SeverityLevel::error) << "DcoGccControlAdapter Initialize Failed";
    }
    registerGccPm();
}

void DpGccMgr::registerGccPm()
{
    INFN_LOG(SeverityLevel::info) << "Registering GCC Control PM Callback";

    DpMsGCCPm gccPm;
    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpmsCbHandler *cbHandler = new DpmsGccCbHandler();
    dpMsPmCallbacksSingleton->RegisterCallBack(CLASS_NAME(gccPm), cbHandler);
}

void DpGccMgr::unregisterGccPm()
{
    INFN_LOG(SeverityLevel::info) << "Unregistering GCC PM Callback";

    DPMSPmCallbacks * dpMsPmCallbacksSingleton = DPMSPmCallbacks::getInstance();
    DpMsGCCPm  gccPm;
    dpMsPmCallbacksSingleton->UnregisterCallBack(CLASS_NAME(gccPm));
}

void DpGccMgr::reSubscribe()
{
    INFN_LOG(SeverityLevel::info) << "";

    mpGccAd->setLldpVlan();
    mpGccAd->subGccPm();
}

void DpGccMgr::createLldpCollector()
{
    struct sockaddr_ll sll;

    INFN_LOG(SeverityLevel::info) << "Creating lldp collector";
    mSd =socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (mSd < 0) {
	INFN_LOG(SeverityLevel::error) << " cannot create raw socket ";
        return;
    }

    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex(LLDP_IF_NAME );
    if ( sll.sll_ifindex == 0 )
    {
        INFN_LOG(SeverityLevel::error) << " invalid interface index ";
        return;
    }

    sll.sll_protocol = htons(ETH_P_ALL);
    if (bind(mSd, (struct sockaddr *)&sll, sizeof(sll)) < 0)
    {
	INFN_LOG(SeverityLevel::error) << " bind to " << LLDP_IF_NAME << " failed " << strerror(errno);
        close(mSd);
        return ;
    }


    mThrLldp = boost::thread(boost::bind(
                              &DpGccMgr::collectLldpFrameInt, this ));

    mThrLldp.detach();
}

void DpGccMgr::collectLldpFrameInt()
{
    unsigned char *ethBuf = NULL;
    int retval;
    int runCnt = 1;
    INFN_LOG(SeverityLevel::error) << " socket fd for  " <<  LLDP_IF_NAME  " " << mSd ;
    uint8_t seqNum = 1;
    boost::posix_time::seconds workTime(mCollectPktInterval);

    while (true)
    {
        //last allocated frame
        if  ( ethBuf == NULL )
        {
            ethBuf = new unsigned char [ETH_FRAME_LEN];
            INFN_LOG(SeverityLevel::debug) << " new buf " << hex << ethBuf <<dec;
        }
       retval = recvfrom(mSd, ethBuf, ETH_FRAME_LEN, 0,NULL, NULL );
       if (retval < 0) {
           INFN_LOG(SeverityLevel::error) <<  " Failed to lldp frame " <<  errno <<  strerror(errno);
           continue;
       }
       int i=0;

       struct iphdr *iph = (struct iphdr *)(ethBuf  + sizeof(struct ethhdr) );
       struct lldp_ethhdr * lldpEth = (struct  lldp_ethhdr*)ethBuf;
       struct ethhdr *eth = (struct ethhdr *)(ethBuf);

       if (lldpEth->enc_h_proto == htons(ETH_P_EXPB5))
       {
           mTotFrameCnt++;
           if ( retval < (ETH_ZLEN+ETH_FCS_LEN) )
           {
               mSmallFrameCnt++;
               INFN_LOG(SeverityLevel::debug) << " frame too small; ignoire ";
               continue;
           }

           {
               INFN_LOG(SeverityLevel::debug) << " encapped lldp frame : mdata seq # " << (uint16_t)lldpEth->enc_seq_num
                               << " slice " << (uint16_t)( ( (lldpEth->enc_key) & 0x20 ) >> 5)
                               << " dir " << (uint16_t)( ( (lldpEth->enc_key) & 0x10 ) >> 4)
                               << " flow id " << (uint16_t) ((lldpEth->enc_key) & 0xf)
                               << " frame length " << retval;
               LldpFrame  frame;
               LldpDir dir = LLDP_DIR_RX;
               uint16_t portId = (lldpEth->enc_key) & 0xf;
               uint16_t slice = ((lldpEth->enc_key) & 0x20) >> 5;
               if (1 == slice)
               {
                   INFN_LOG(SeverityLevel::debug) << " Change flowId for slice1 from " << portId << " to " << portId+16;
                   portId += 16;
               }
               //get dir
               if ( ( (lldpEth->enc_key) & 0x10 ) >> 4 )
               {
                   dir = LLDP_DIR_TX;
                   mTxGoodFrameCnt++;

               } else
               {
                   dir = LLDP_DIR_RX;
                   mRxGoodFrameCnt++;

               }
               LldpKey frameKey = make_pair( portId, dir);
                 //store the frame
               frame.StoreEthFrame(ethBuf, retval,  portId, dir);

               std::lock_guard<std::mutex> lock(mMutex);
               LldpFramePool &actFramePool = mLldpFramePools[mLldpFramePoolIdx];
               std::pair<LldpFramePool::iterator, bool> ret;
               INFN_LOG(SeverityLevel::debug) << " pool index/size " << mLldpFramePoolIdx << "/" << actFramePool.size();

               ret = actFramePool.insert(make_pair(frameKey, frame));
               if (ret.second == false)
               {   //lldp key exist ; copy in place; free old buf
                   LldpFramePool::iterator it = ret.first;
                   INFN_LOG(SeverityLevel::debug) << " key exist, Port Id " << portId << " dir " << dir;
                   it->second.FreeFrameBuf();
                   it->second.StoreEthFrame(ethBuf, retval,  portId, dir);
                   mDupFrameCnt++;
               }
               //ethBuf in use
               ethBuf = NULL;
           }
       }//if (eth->h_proto == htons(ETH_P_8021Q))
//test only
#if 0
       if (!(mGoodFrameCnt % 5 ))
       {
           LldpFramePool pool;
           collectLldpFrame(pool);
       }
#endif

    //boost::this_thread::sleep(workTime);
     }//while

    delete [] ethBuf;
    ethBuf = NULL;
}

void DpGccMgr::dumpActLldpPool( uint16_t portId, std::ostream& os )
{

    LldpDir dir = LLDP_DIR_RX;
    std::lock_guard<std::mutex> lock(mMutex);
    LldpFramePool &actFramePool = mLldpFramePools[mLldpFramePoolIdx];
    LldpFramePool::iterator it = actFramePool.find ( make_pair( portId, dir)  );
    bool found = false;

    os << " frames in pool " << actFramePool.size() << endl << endl;

    if ( it != actFramePool.end() )
    {
        os <<  " portId " << portId << " Dir RX " << dir << endl;
        it->second.Dump(os);
        found = true;
    }

    dir = LLDP_DIR_TX;
    it = actFramePool.find ( make_pair( portId, dir)  );
    if ( it != actFramePool.end() )
    {
        os << " portId " << portId << " Dir TX " << dir << endl;
        it->second.Dump(os);
        found = true;
    }

    if (found == false)
    {
        os << " no lldp frame from port id " << portId << endl;
    }

}

void DpGccMgr::createGccConfigHandler()
{
    INFN_LOG(SeverityLevel::info) << "run ...";

    mspGccCfgHndlr = std::make_shared<GccConfigHandler>(mpGccAd);

    mspConfigHandler = mspGccCfgHndlr;
}

bool DpGccMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    VlanConfig *pVlanCfg = dynamic_cast<VlanConfig*>(objMsg);
    if (pVlanCfg)
    {
        if (NULL == mspGccCfgHndlr)
        {
            INFN_LOG(SeverityLevel::error) << "GccConfigHandler not initialized";
            return false;
        }

        mspGccCfgHndlr->sendTransactionStateToInProgress(pVlanCfg);

        std::string aid = pVlanCfg->base_config().config_id().value();
        string cfgdata;
        MessageToJsonString(*pVlanCfg, &cfgdata);
        INFN_LOG(SeverityLevel::info) << "vlan aid [" << aid << "] GccConfig create data [" << cfgdata << "]";

        handleConfig(objMsg, pVlanCfg->base_config().config_id().value(), ConfigType::CREATE);

        return true;
    }

    return false;
}

bool DpGccMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    VlanConfig *pVlanCfg = dynamic_cast<VlanConfig*>(objMsg);
    if (pVlanCfg)
    {
        if (NULL == mspGccCfgHndlr)
        {
            INFN_LOG(SeverityLevel::error) << "GccConfigHandler not initialized";
            return false;
        }

        mspGccCfgHndlr->sendTransactionStateToInProgress(pVlanCfg);

        std::string aid = pVlanCfg->base_config().config_id().value();
        string cfgdata;
        MessageToJsonString(*pVlanCfg, &cfgdata);
        INFN_LOG(SeverityLevel::info) << "vlan aid [" << aid << "] GccConfig update data [" << cfgdata << "]";

        handleConfig(objMsg, pVlanCfg->base_config().config_id().value(), ConfigType::MODIFY);

        return true;
    }

    return false;
}

bool DpGccMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    VlanConfig *pVlanCfg = dynamic_cast<VlanConfig*>(objMsg);
    if (pVlanCfg)
    {
        if (NULL == mspGccCfgHndlr)
        {
            INFN_LOG(SeverityLevel::error) << "GccConfigHandler not initialized";
            return false;
        }

        mspGccCfgHndlr->sendTransactionStateToInProgress(pVlanCfg);

        std::string aid = pVlanCfg->base_config().config_id().value();
        string cfgdata;
        MessageToJsonString(*pVlanCfg, &cfgdata);
        INFN_LOG(SeverityLevel::info) << "vlan aid [" << aid << "] GccConfig delete data [" << cfgdata << "]";

        handleConfig(objMsg, pVlanCfg->base_config().config_id().value(), ConfigType::DELETE);

        return true;
    }

    return false;
}

bool DpGccMgr::checkOnResync(google::protobuf::Message* objMsg)
{
    VlanConfig *pVlanCfg = dynamic_cast<VlanConfig*>(objMsg);
    if (pVlanCfg)
    {
        if (NULL == mspGccCfgHndlr)
        {
            INFN_LOG(SeverityLevel::error) << "GccConfigHandler not initialized";
            return false;
        }

        std::string aid = pVlanCfg->base_config().config_id().value();
        string cfgdata;
        MessageToJsonString(*pVlanCfg, &cfgdata);
        INFN_LOG(SeverityLevel::info) << "vlan aid [" << aid << "] GccConfig Resync data [" << cfgdata << "]";

        mspGccCfgHndlr->createCacheObj(*pVlanCfg);

        return true;
    }

    return false;
}


void DpGccMgr::dumpLldp(std::ostream& os)
{
    os << " Socket   " << endl;
    os << "    intf name " << LLDP_IF_NAME << endl;
    os << "    fd " <<  mSd << endl;
    os << " Rx'ed Frame Counters " << endl;
    os << "    Tx Good  " << mTxGoodFrameCnt << endl;
    os << "    Rx Good  " << mRxGoodFrameCnt << endl;
    os << "    Error " << mSmallFrameCnt << endl;
    os << "    Total " << mTotFrameCnt << endl;
    os << "    Duplicate " << mDupFrameCnt << endl;

}
//pass lldp frames to upper layer
int DpGccMgr::collectLldpFrame( LldpFramePool &pool )
{
    INFN_LOG(SeverityLevel::debug) << " current pool idx " << mLldpFramePoolIdx ;
    pool = mLldpFramePools[mLldpFramePoolIdx];
    //prepare to switch to next pool
    uint32_t nextPoolIdx = (~mLldpFramePoolIdx) & 1;
    LldpFramePool &nextFramePool = mLldpFramePools[nextPoolIdx];
    for (LldpFramePool::iterator it=nextFramePool.begin(); it != nextFramePool.end(); ++it)
    {
        INFN_LOG(SeverityLevel::debug) << " port id " << it->first.first << " dir " << it->first.second;
        it->second.FreeFrameBuf();
    }
    nextFramePool.clear();
    //new current pool index
    std::lock_guard<std::mutex> lock(mMutex);
    mLldpFramePoolIdx = nextPoolIdx;
    INFN_LOG(SeverityLevel::debug) << " new current pool idx " << mLldpFramePoolIdx
                    << " pool size " << nextFramePool.size();
    return 0;
}

ConfigStatus DpGccMgr::processConfig(google::protobuf::Message* objMsg, ConfigType cfgType)
{
    // Check for Carrier ..............................................................................
    VlanConfig *pCfg = static_cast<VlanConfig*>(objMsg);

    ConfigStatus status = ConfigStatus::SUCCESS;

    switch(cfgType)
    {
        case ConfigType::CREATE:
            status = mspGccCfgHndlr->onCreate(pCfg);

            break;

        case ConfigType::MODIFY:
            status = mspGccCfgHndlr->onModify(pCfg);
            break;

        case ConfigType::DELETE:
            status = mspGccCfgHndlr->onDelete(pCfg);
            break;
    }

    return status;
}

ConfigStatus DpGccMgr::completeConfig(google::protobuf::Message* objMsg,
                                 ConfigType cfgType,
                                 ConfigStatus status,
                                 std::ostringstream& log)
{
    INFN_LOG(SeverityLevel::info) << "cfgType " << toString(cfgType);

    VlanConfig *pCfg = static_cast<VlanConfig*>(objMsg);

    STATUS transactionStat = STATUS::STATUS_SUCCESS;
    if (ConfigStatus::SUCCESS != status)
    {
        transactionStat = STATUS::STATUS_FAIL;

        INFN_LOG(SeverityLevel::info) << "Fall-back failure handling: Caching Config although it has failed.";

        switch(cfgType)
        {
            case ConfigType::CREATE:
                mspGccCfgHndlr->createCacheObj(*pCfg);
                break;

            case ConfigType::MODIFY:
                mspGccCfgHndlr->updateCacheObj(*pCfg);
                break;

            case ConfigType::DELETE:
                // what todo here?  delete failures require redis cleanup. Not sure how best to handle this

                INFN_LOG(SeverityLevel::error) << "FAIL. CANNOT handle delete failures";

                break;

        }

        mspGccCfgHndlr->setTransactionErrorString(log.str());
        mspGccCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);
        return ConfigStatus::SUCCESS;
    }

    switch(cfgType)
    {
        case ConfigType::CREATE:
            mspGccCfgHndlr->createCacheObj(*pCfg);

            break;

        case ConfigType::MODIFY:
            //@todo : Do we need to notify modifications in carrier to DCO Secproc?

            mspGccCfgHndlr->updateCacheObj(*pCfg);

            break;

        case ConfigType::DELETE:
            mspGccCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

            STATUS tStat = mspGccCfgHndlr->deleteGccConfigCleanup(pCfg);
            if (STATUS::STATUS_FAIL == tStat)
            {
                INFN_LOG(SeverityLevel::error) << " Failed to delete in Redis for configId " << pCfg->base_config().config_id().value();

                return ConfigStatus::FAIL_OTHER;
            }

            mspGccCfgHndlr->deleteCacheObj(*pCfg);

            // return here; transaction response already sent above in this case
            return ConfigStatus::SUCCESS;

            break;
    }

    mspGccCfgHndlr->sendTransactionStateToComplete(pCfg, transactionStat);

    return ConfigStatus::SUCCESS;
}

LldpFrame::LldpFrame()
    : mLen(0)
    , mEthBuf(NULL)
{
}

LldpFrame::~LldpFrame()
{

}

void LldpFrame::StoreEthFrame(unsigned char *frame , uint16_t len, uint16_t portId, LldpDir dir)
{
    mLen =len;
    mSerdeLaneNum = portId + 1;
    mDir = dir;
    mEthBuf = frame;
    INFN_LOG(SeverityLevel::debug) << " storeframe buf " << hex << mEthBuf << dec << endl;
}

 unsigned char *LldpFrame::GetLldpFrame( uint16_t &len, uint8_t &serdeLaneNum, LldpDir &dir)
{
    len = mLen - sizeof(struct encap_ethhdr );
    serdeLaneNum = mSerdeLaneNum;
    dir = mDir;
    return (mEthBuf + sizeof(struct encap_ethhdr));
}

void LldpFrame::Dump(std::ostream& os)
{
    struct lldp_ethhdr * lldpEth = (struct  lldp_ethhdr*)mEthBuf;

    os << " Frame len " << mLen << " Frame Header: " << endl;
    os << " Encap Header " << endl;
    os << hex << "     dmac (hex) " << (uint16_t)(lldpEth->enc_h_dest[0]) << "." << (uint16_t)(lldpEth->enc_h_dest[1]) << "."
       << (uint16_t)lldpEth->enc_h_dest[2] << "." << (uint16_t)(lldpEth->enc_h_dest[3]) << "."
       << (uint16_t)(lldpEth->enc_h_dest[4]) << "." << (uint16_t)(lldpEth->enc_h_dest[5]) << endl;

    os << hex << "     smac (hex) " << (uint16_t)(lldpEth->enc_h_source[0]) << "." << (uint16_t)(lldpEth->enc_h_source[1]) << "."
       << (uint16_t)lldpEth->enc_h_source[2] << "." << (uint16_t)(lldpEth->enc_h_source[3]) << "."
       << (uint16_t)(lldpEth->enc_h_source[4]) << "." << (uint16_t)(lldpEth->enc_h_source[5]) << endl;

    os << "     ethertype (hex) " << (uint16_t)ntohs(lldpEth->enc_h_proto) << dec << endl;

    os << "     seq # (dec) " << (uint16_t)(lldpEth->enc_seq_num) << endl;
    os << "     key (hex) raw : " << hex << (uint16_t)(lldpEth->enc_key) << dec << endl ;
    os << "                     port id : "
       << (uint16_t)(lldpEth->enc_key & 0xf) ;

    if ( ( ( (lldpEth->enc_key) & 0x10 ) >> 4 ) == LLDP_DIR_RX )
    {

        os << "                    dir: RX " << endl;
    }
    else
    {
        os << "                    dir: TX " << endl;
    }
    os << " Lldp Header " << endl;
    os << hex << "     dmac (hex) " << (uint16_t)(lldpEth->h_dest[0]) << "." << (uint16_t)(lldpEth->h_dest[1]) << "."
       << (uint16_t)lldpEth->h_dest[2] << "." << (uint16_t)(lldpEth->h_dest[3]) << "."
       << (uint16_t)(lldpEth->h_dest[4]) << "." << (uint16_t)(lldpEth->h_dest[5]) << endl;

    os << hex << "     smac (hex) " << (uint16_t)(lldpEth->h_source[0]) << "." << (uint16_t)(lldpEth->h_source[1]) << "."
       << (uint16_t)lldpEth->h_source[2] << "." << (uint16_t)(lldpEth->h_source[3]) << "."
       << (uint16_t)(lldpEth->h_source[4]) << "." << (uint16_t)(lldpEth->h_source[5]) << endl;

    os << "     ethertype (hex) " << (uint16_t)ntohs(lldpEth->h_proto) << dec << endl;


}

void LldpFrame::FreeFrameBuf()
{
    if  ( mEthBuf != NULL )
    {
        INFN_LOG(SeverityLevel::debug) << " free frame buf " << hex << mEthBuf << dec << endl;
        delete [] mEthBuf;
    }
    else
    {
     INFN_LOG(SeverityLevel::error) << " free failed:NULL pointer " << endl;
    }
    mEthBuf = NULL;
}

} // namespace DataPlane
