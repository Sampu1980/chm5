/*
 * serdes_tuner_cmds.cpp
 *
 *  Created on: Sep 9, 2020
 *      Author: aforward
 */
#include <cli/clilocalsession.h> // include boost asio
#include <cli/remotecli.h>
#include <cli/cli.h>
#include <boost/function.hpp>
#include <chrono>
#include "dp_common.h"
#include "GearBoxAdapter.h"
#include "GearBoxUtils.h"
#include <vector>
#include "epdm_bcm_common_defines.h"
#include "InfnLogger.h"
#include "types.h"
#include <iomanip>
#include "serdes_tuner_cmds.h"
#include <map>
#include "AtlanticSerdesTable.h"
#include "GearBoxUtils.h"



using namespace cli;
using namespace std;
using namespace boost;
using namespace gearbox;
using namespace tuning;

const unsigned int cFailTomTx              = (1 << 0);
const unsigned int cFailTomRx              = (1 << 1);
const unsigned int cFailBcmTxLineToTom     = (1 << 2);
const unsigned int cFailBcmTxHostToBcmLine = (1 << 4);
const unsigned int cFailBcmLineToRxHost    = (1 << 5);
const unsigned int cFailBcmTxLineToBcmHost = (1 << 6);
const unsigned int cFailBcmTxToAtl         = (1 << 7);
const unsigned int cFailAtlTxToBcm         = (1 << 8);
const unsigned int cFailAtlToBcmRx         = (1 << 9);

const unsigned int cNaSerdes               = (1 << 10);


void dumpParams(ostream& out, std::map<std::string,sint32> serdesParam)
{
    for( map<string,sint32>::iterator it = serdesParam.begin(); it != serdesParam.end(); it++)
    {
        string paramName = it->first;
        sint32 param = it->second;

        out << paramName << ":" << param << endl;
    }
}

void cmdSetBcmTomParams(ostream& out, unsigned int slot, unsigned int port, unsigned int rate)
{
    INFN_LOG(SeverityLevel::info) << "SetBcmTomParams port=" << port << endl;
    int result = 0;

    std::string aId = "1-" + std::to_string(slot) + "-T" + std::to_string(port);



    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();

    if (serdesTuner)
    {
        result = serdesTuner->TuneBcmToTomLinks(aId, (gearbox::Bcm81725DataRate_t)rate, true);
        if  (result)
        {
            out << "Error tuning BCM to TOM links." << endl;
        }
        else
        {
            out << "Successfully tuned BCM to TOM links." << endl;
        }

        result = serdesTuner->TuneTomToBcmLinks(aId, (gearbox::Bcm81725DataRate_t)rate, true);
        if (result)
        {
            out << "Error tuning TOM to BCM links." << endl;
        }
        else
        {
            out << "Successfully tuned TOM to BCM links." << endl;
        }
    }
    else
    {
        out << "SerdesTuner is NULL!" << endl;
    }
}

void cmdSetBcmBcmParams(ostream& out, unsigned int port, unsigned int rate)
{
    if (port == gearbox::PORT3  ||
        port == gearbox::PORT4  ||
        port == gearbox::PORT5  ||
        port == gearbox::PORT6  ||
        port == gearbox::PORT11 ||
        port == gearbox::PORT12 ||
        port == gearbox::PORT13 ||
        port == gearbox::PORT14)
    {
        SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();

        serdesTuner->TuneBcmLineToBcmHostLinks(port, (gearbox::Bcm81725DataRate_t)rate);
        serdesTuner->TuneBcmHostToBcmLineLinks(port, (gearbox::Bcm81725DataRate_t)rate);
    }
    else
    {
        out << "BCM to BCM is not needed on this port." << endl;
    }
}

void cmdSetBcmAtlParams(ostream& out, unsigned int port, unsigned int rate)
{
    int result = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();

    result = serdesTuner->TuneBcmToAtlLinks(port, (gearbox::Bcm81725DataRate_t)rate);
    if  (result)
    {
        out << "Error tuning BCM to Atlantic links." << endl;
    }
    else
    {
        out << "Successfully tuned BCM to Atlantic links." << endl;
    }

    result = serdesTuner->TuneAtlToBcmLinks(port, (gearbox::Bcm81725DataRate_t)rate);
    if  (result)
    {
        out << "Error tuning Atlantic to BCM links." << endl;
    }
    else
    {
        out << "Successfully tuned Atlantic to BCM links." << endl;
    }
    result = serdesTuner->PushAtlToBcmRxParamsToHw(port, (gearbox::Bcm81725DataRate_t)rate);

}


std::string GetFoundTitle(bool found)
{
    if (found)
    {
        return "==Set Value==";
    }
    else
    {
        return "==Not Set==";
    }
}


bool GetBcmRxDump(std::map<unsigned int, std::vector<SerdesDumpData>> dump, unsigned int port, unsigned int lane,
                  std::map<unsigned int, int64_t>& rxValue, unsigned int& srcDevNum, unsigned int& srcLaneNum,
                  dp::DeviceInfo& src, dp::DeviceInfo& dest, unsigned int& linkId, unsigned int& linkLoss)
{
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    bool found = false;
    std::vector<std::string> str;
    auto it = dump.find(port);

    if (it != dump.end() && it->second.size() > 0)
    {
        for (unsigned int i = 0; i < it->second.size(); i++)
        {
            if (it->second[i].mDestLaneNum == lane)
            {
                // found lane
                serdesTuner->GetRxParamsFromMap(it->second[i].mRxParams, rxValue);
                srcDevNum = it->second[i].mSrcDeviceNum;
                srcLaneNum = it->second[i].mSrcLaneNum;
                src = it->second[i].mSrc;
                dest = it->second[i].mDest;
                linkId = it->second[i].mLinkId;
                linkLoss = it->second[i].mLinkLoss;
                found = true;
                break;
            }
        }
    }
    return found;
}




unsigned int DumpTomTx(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    std::map<unsigned int, std::vector<SerdesDumpData>> dump;
    unsigned int laneNum = 0;
    unsigned int totalLanes = 0;
    const LinkData * pLinkData = NULL;
    std::map<std::string,sint32> txParams;
    SerdesDumpData laneDump;

    serdesTuner->GetSerdesDumpData(dump, TomToBcm);

    out << endl << "=======Dumping Tom Tx for port " << port << "=======" << endl;

    // Get laneNum based on port
    if (serdesTuner->GetTomToBcmSerdesParams(port, rate, &pLinkData, totalLanes) == false)
    {
        out << "Problem finding port=" << port << " in serdesTable!" << endl;
        status |= cFailTomTx;
        return status;
    }

    bool found = false;
    unsigned int numOptLanes = serdesTuner->GetNumOpticalLanes(port);

    for (unsigned int i = 0; i < totalLanes; i++)
    {
        found = GetTomTxDump(dump, port, pLinkData[i].tx_lane_num, laneDump);

        out << "     Src Lane : " << setw(19) << pLinkData[i].tx_lane_num << endl;
        out << "      Src TOM : " << setw(19) << laneDump.mSrcDeviceNum << endl;
        out << "       SrcDev : " << setw(19) << laneDump.mSrc.mDevice << endl;
        out << "   SrcPonType : " << setw(19) << laneDump.mSrc.mPonType << endl;
        out << "    SrcVendor : " << setw(19) << laneDump.mSrc.mVendor << endl;
        out << "   *Dest Lane : " << setw(19) << pLinkData[i].rx_lane_num << endl;
        out << "  *Dest DevId : " << setw(19) << pLinkData[i].rx_dev_id << endl;
        out << "     *DestDev : " << setw(19) << laneDump.mDest.mDevice << endl;
        out << " *DestPonType : " << setw(19) << laneDump.mDest.mPonType << endl;
        out << "  *DestVendor : " << setw(19) << laneDump.mDest.mVendor << endl;
        out << "       LinkId : " << setw(19) << laneDump.mLinkId << endl;
        out << "     LinkLoss : " << setw(19) << laneDump.mLinkLoss << endl;
        out << "        Found : " << setw(19) <<  found << endl << endl;

        if (found == false)
        {
            status |= cFailTomTx;
        }

        if (i < numOptLanes)
        {
            out << setw(13) << " Optical Lane : " << setw(19) << i << endl;
            txParams = laneDump.mTxParams;
            for (auto elem : txParams)
            {
                std::string str = elem.first;
                str = "  " + str;

                // Remove garbage carriage return from LUT string
                std::size_t found = str.rfind('\r');

                if (found != std::string::npos)
                {
                    str = str.substr(0, found);
                }
                out << setw(13) << str << setw(2) << " : " << setw(19) << elem.second << endl;
            }
        }
        out << endl;
    }
    out << endl;


    return status;
}

bool GetTomTxDump(std::map<unsigned int, std::vector<SerdesDumpData>> dump, unsigned int port, unsigned int lane, SerdesDumpData & laneDump)
{
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    bool found = false;
    std::vector<std::string> str;
    auto it = dump.find(port);

    if (it != dump.end() && it->second.size() > 0)
    {
        for (unsigned int i = 0; i < it->second.size(); i++)
        {
            if (it->second[i].mSrcLaneNum == lane)
            {
                // found lane
                laneDump = it->second[i];
                found = true;
                break;
            }
        }
    }
    return found;
}

unsigned int DumpTomRx(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    std::map<unsigned int, std::vector<SerdesDumpData>> dump;
    unsigned int laneNum = 0;
    unsigned int totalLanes = 0;
    const LinkData * pLinkData = NULL;
    std::map<std::string,sint32> rxParams;
    SerdesDumpData laneDump;

    serdesTuner->GetSerdesDumpData(dump, BcmToTom);

    out << endl << "=======Dumping Tom Rx for port " << port << "=======" << endl;

    // Get laneNum based on port
    if (serdesTuner->GetBcmToTomSerdesParams(port, rate, &pLinkData, totalLanes) == false)
    {
        out << "Problem finding port=" << port << " in serdesTable!" << endl;
        status |= cFailTomRx;
        return status;
    }

    bool found = false;
    unsigned int numOptLanes = serdesTuner->GetNumOpticalLanes(port);

    for (unsigned int i = 0; i < totalLanes; i++)
    {
        found = GetTomRxDump(dump, port, pLinkData[i].rx_lane_num, laneDump);


        out << setw(13) << "Dest Lane"   << setw(2) << ":" << setw(19) << pLinkData[i].rx_lane_num << endl;
        out << setw(13) << "Dest TOM"    << setw(2) << ":" << setw(19) << laneDump.mDestDeviceNum << endl;
        out << setw(13) << "DestDev"     << setw(2) << ":" << setw(19) << laneDump.mDest.mDevice << endl;
        out << setw(13) << "DestPonType" << setw(2) << ":" << setw(19) << laneDump.mDest.mPonType << endl;
        out << setw(13) << "DestVendor"  << setw(2) << ":" << setw(19) << laneDump.mDest.mVendor << endl;


        out << setw(13) << "*Src Lane"   << setw(2) << ":" << setw(19) << pLinkData[i].tx_lane_num << endl;
        out << setw(13) << "*Src DevId"  << setw(2) << ":" << setw(19) << pLinkData[i].tx_dev_id << endl;
        out << setw(13) << "*SrcDev"     << setw(2) << ":" << setw(19) << laneDump.mSrc.mDevice << endl;
        out << setw(13) << "*SrcPonType" << setw(2) << ":" << setw(19) << laneDump.mSrc.mPonType << endl;
        out << setw(13) << "*SrcVendor"  << setw(2) << ":" << setw(19) << laneDump.mSrc.mVendor << endl;

        out << setw(13) << "LinkId"      << setw(2) << ":" << setw(19) << laneDump.mLinkId << endl;
        out << setw(13) << "LinkLoss"    << setw(2) << ":" << setw(19) << laneDump.mLinkLoss << endl;
        out << setw(13) << "Found"       << setw(2) << ":" << setw(19) << found << endl << endl;

        if (found == false)
        {
            status |= cFailTomRx;
        }

        if (i < numOptLanes)
        {
            out << setw(13) << "Optical Lane :" << setw(19) <<  i << endl;
            rxParams = laneDump.mRxParams;
            for (auto elem : rxParams)
            {
                out << setw(13) << elem.first << setw(2) << ":" << setw(19) << elem.second << endl;
            }
        }
        out << endl;
    }
    out << endl;


    return status;
}

bool GetTomRxDump(std::map<unsigned int, std::vector<SerdesDumpData>> dump, unsigned int port, unsigned int lane, SerdesDumpData & laneDump)
{

    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    bool found = false;
    std::vector<std::string> str;
    auto it = dump.find(port);

    if (it != dump.end() && it->second.size() > 0)
    {
        for (unsigned int i = 0; i < it->second.size(); i++)
        {
            if (it->second[i].mDestLaneNum == lane)
            {
                INFN_LOG(SeverityLevel::debug) << "Lane=" << lane;
                // found lane
                laneDump = it->second[i];
                found = true;
                break;
            }
        }
    }
    return found;
}

unsigned int DumpBcmTxHostToBcmLine(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    Bcm81725Lane param;
     int pre2  = 0;
     int pre   = 0;
     int main  = 0;
     int post  = 0;
     int post2 = 0;
     int post3 = 0;

     int dumpPre2 = 0;
     int dumpPre   = 0;
     int dumpMain  = 0;
     int dumpPost  = 0;
     int dumpPost2 = 0;
     int dumpPost3 = 0;
     bool found = false;
     dp::DeviceInfo src {"", "" , ""};
     dp::DeviceInfo dest {"", "" , ""};
     unsigned int linkId = 0;
     unsigned int linkLoss = 0;
     unsigned int destDev = 0;
     unsigned int destLaneNum = 0;
     unsigned int srcDevNum = 0;


     if ( (rate == gearbox::RATE_4_100GE)
           ||
         (port != gearbox::PORT3  &&
          port != gearbox::PORT4  &&
          port != gearbox::PORT5  &&
          port != gearbox::PORT6  &&
          port != gearbox::PORT11 &&
          port != gearbox::PORT12 &&
          port != gearbox::PORT13 &&
          port != gearbox::PORT14) )
     {
         out << " No BCM Tx Host to BCM Line Serdes for port=" << port << endl;
         return cNaSerdes;
     }

     std::vector<PortConfig> portConfig;
     gearbox::GetPortConfigEntry(portConfig, port, rate);

     out << "=======Dumping Bcm Tx Host to Bcm Line=======" << endl;
     for (unsigned int i = 0; i < portConfig.size(); i++)
     {
         gearbox::GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
         if (!adapter)
         {
             out << "gearbox init failed" << endl;
             return cFailBcmTxHostToBcmLine;
         }

         for (uint16 laneNum = 0; laneNum < (sizeof(uint16) * 8 /*num bits*/); laneNum++)
         {
             unsigned int bitMask = (1 << laneNum);

             if ( (portConfig[i].bcmUnit == 0xc) &&
                  (portConfig[i].host.laneMap & bitMask) )
             {
                 // Bcm Host Tx to Bcm Line
                 out << endl;


                 std::map<unsigned int, std::vector<SerdesDumpData>> dump;
                 serdesTuner->GetSerdesDumpData(dump, BcmHostToBcmLine);

                 // Host side
                 param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, bitMask};

                 found = GetBcmTxDump(dump, port, srcDevNum, laneNum, dumpPre2, dumpPre, dumpMain, dumpPost, dumpPost2, dumpPost3,
                                      destDev, destLaneNum, src, dest, linkId, linkLoss);


                 out <<             "  SrcBus         : "   << gearbox::bus2String(portConfig[i].bus) << endl
                     << std::hex << "  SrcUnit        : 0x" << portConfig[i].bcmUnit << endl << std::dec
                     <<             "  SrcDevNum      : "   << srcDevNum << endl
                     <<             "  SrcDev         : "   << src.mDevice << endl
                     <<             "  SrcPonType     : "   << src.mPonType << endl
                     <<             "  SrcVendor      : "   << src.mVendor << endl
                     <<             "  SrcSide        : "   << side2String(cHostSide) << endl
                     <<             "  SrcLaneNum     : "   << laneNum << endl
                     << std::hex << "  SrcLane Mask   : 0x" << portConfig[i].host.laneMap  << std::dec << endl
                     <<             "  LinkId         : "   << linkId << endl
                     <<             "  LinkLoss       : "   << linkLoss << endl
                     <<             " *DestUnit       : "   << destDev <<  endl
                     <<             " *DestDestLane   : "   << destLaneNum << endl
                     <<             " *DestDev        : "   << dest.mDevice << endl
                     <<             " *DestPonType    : "   << dest.mPonType << endl
                     <<             " *DestVendor     : "   << dest.mVendor << endl
                     <<             "  Found          : "   << found << endl << endl;

                 if (found == false)
                 {
                     status |= cFailBcmTxHostToBcmLine;
                 }

                 std::string title = GetFoundTitle(found);

                 if (!adapter->getTxFir(param, pre2, pre, main, post, post2, post3))
                 {
                     out << "         ==BCM Value==" << " " << title << endl;
                     out << "  Pre2  :" << setw(7) << pre2  << setw(15) <<  dumpPre2  << endl;
                     out << "  Pre   :" << setw(7) << pre   << setw(15) <<  dumpPre   << endl;
                     out << "  Main  :" << setw(7) << main  << setw(15) <<  dumpMain  << endl;
                     out << "  Post  :" << setw(7) << post  << setw(15) <<  dumpPost  << endl;
                     out << "  Post2 :" << setw(7) << post2 << setw(15) <<  dumpPost2 << endl;
                     out << "  Post3 :" << setw(7) << post3 << setw(15) <<  dumpPost3 << endl << endl;

                     if (pre2  != dumpPre2  ||
                         pre   != dumpPre   ||
                         main  != dumpMain  ||
                         post  != dumpPost  ||
                         post2 != dumpPost2 ||
                         post3 != dumpPost3)
                     {
                         status |= cFailBcmTxHostToBcmLine;
                     }
                 }
                 else
                 {
                     out << "Error trying to get Tx FIR for port=" << port << endl;
                     status |= cFailBcmTxHostToBcmLine;
                 }
             }
         }
     }
     return status;
}

unsigned int DumpBcmLineToRxHost(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    Bcm81725Lane param;
    unsigned int autoPeakingEn = 0;
    bool found = false;
    unsigned int srcDevNum;
    unsigned int srcLaneNum;
    dp::DeviceInfo src {"", "" , ""};
    dp::DeviceInfo dest {"", "" , ""};
    unsigned int linkId;
    unsigned int linkLoss;


    if ( (rate == gearbox::RATE_4_100GE)
          ||
        (port != gearbox::PORT3  &&
         port != gearbox::PORT4  &&
         port != gearbox::PORT5  &&
         port != gearbox::PORT6  &&
         port != gearbox::PORT11 &&
         port != gearbox::PORT12 &&
         port != gearbox::PORT13 &&
         port != gearbox::PORT14) )
    {
        out << " No BCM Line to BCM Rx Host Serdes for port=" << port << endl;
        return cNaSerdes;
    }

    std::vector<PortConfig> portConfig;
    gearbox::GetPortConfigEntry(portConfig, port, rate);

    out << endl << "=======Dumping Bcm Line to Bcm Rx Host=======" << endl;
    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        gearbox::GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
        if (!adapter)
        {
            out << "gearbox init failed" << endl;
            return cFailBcmLineToRxHost;
        }

        for (uint16 laneNum = 0; laneNum < (sizeof(uint16) * 8 /*num bits*/); laneNum++)
        {
            unsigned int bitMask = (1 << laneNum);

            if ( (portConfig[i].bcmUnit == 0xc) &&
                 (portConfig[i].host.laneMap & bitMask) )
            {


                std::map<unsigned int, int64_t> rxValue;
                unsigned int dumpAutoPeakingEn = 0;

                std::map<unsigned int, std::vector<SerdesDumpData>> dump;
                serdesTuner->GetSerdesDumpData(dump, BcmLineToBcmHost);

                param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, bitMask};
                found = GetBcmRxDump(dump, port, laneNum, rxValue, srcDevNum, srcLaneNum, src, dest, linkId, linkLoss);

                // Bcm Line to Host Rx
                out <<             "  DestBus      : "   << gearbox::bus2String(portConfig[i].bus) << endl
                    << std::hex << "  DestUnit     : 0x" << portConfig[i].bcmUnit  << endl << std::dec
                    <<             "  DestSide     : "   << side2String(cHostSide) << endl
                    <<             "  DestLaneNum  : "   << laneNum  << endl
                    << std::hex << "  DestLane Mask: 0x" << portConfig[i].host.laneMap << endl << std::dec
                    <<             "  LinkId       : "   << linkId << endl
                    <<             "  LinkLoss     : "   << linkLoss << endl
                    << std::hex << " *SrcUnit      : 0x" << srcDevNum << endl << std::dec
                    <<             " *SrcLaneNum   : "   << srcLaneNum << endl
                    <<             " *SrcVendor    : "   << src.mVendor << endl
                    <<             " *SrcDev       : "   << src.mDevice << endl
                    <<             " *SrcPonType   : "   << src.mPonType << endl
                    <<             "  Found        : "   << found << endl << endl;

                if (found == false)
                {
                    status |= cFailBcmLineToRxHost;
                }

                auto elem = rxValue.find(gearbox::DS_AutoPeakingEnable);

                if (elem == rxValue.end())
                {
                    out << "Can not find Rx AutoPeakingEn params." << endl;
                }
                else
                {
                    dumpAutoPeakingEn = (unsigned int)elem->second;
                }

                bcm_plp_dsrds_firmware_lane_config_t lnConf;
                memset(&lnConf, 0, sizeof(bcm_plp_dsrds_firmware_lane_config_t));

                string title = GetFoundTitle(found);

                // BCM Host Rx
                if (!adapter->getLaneConfigDs(param, lnConf))
                {
                    out << "                  ==BCM Value==" << " " << title << endl;
                    out << " AutoPeakingEn:       " << lnConf.AutoPeakingEnable << setw(15) << dumpAutoPeakingEn << endl;

                    if (lnConf.AutoPeakingEnable != dumpAutoPeakingEn)
                    {
                        status |= cFailBcmLineToRxHost;
                    }
                }
                else
                {
                    out << "cmdGetRxAutoPeakingEn failed" << endl;
                    status |= cFailBcmLineToRxHost;
                }
                out << endl;
            }
        }
    }
    return status;
}



bool GetBcmTxDump(std::map<unsigned int, std::vector<SerdesDumpData>> dump, unsigned int port, unsigned int& srcDevNum,
                  unsigned int lane, int& pre2, int& pre, int& main, int & post, int& post2, int & post3,
                  unsigned int& destDeviceNum, unsigned int& destLaneNum,
                  dp::DeviceInfo& src, dp::DeviceInfo& dest, unsigned int& linkId, unsigned int& linkLoss)
{
    pre2 = 0;
    pre = 0;
    main = 0;
    post = 0;
    post2 = 0;
    post3 = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    bool found = false;
    std::vector<std::string> str;
    auto it = dump.find(port);


    if (it != dump.end() && it->second.size() > 0)
    {
        INFN_LOG(SeverityLevel::debug) << " Port found port=" << port << " size=" << it->second.size();
        for (unsigned int i = 0; i < it->second.size(); i++)
        {
            INFN_LOG(SeverityLevel::debug) << " Port found=" << port << " itSrcLaneNum[" << i << "]=" << it->second[i].mSrcLaneNum << " lane=" << lane;
            if (it->second[i].mSrcLaneNum == lane)
            {
                // found lane
                serdesTuner->GetTxParamsFromMap(it->second[i].mTxParams, pre2, pre, main, post, post2, post3);
                srcDevNum = it->second[i].mSrcDeviceNum;
                src = it->second[i].mSrc;
                dest = it->second[i].mDest;
                linkId = it->second[i].mLinkId;
                linkLoss = it->second[i].mLinkLoss;
                destDeviceNum = it->second[i].mDestDeviceNum;
                destLaneNum = it->second[i].mDestLaneNum;

                INFN_LOG(SeverityLevel::debug) << "Src Device=" << src.mDevice << " PonType=" << src.mPonType << " vendor=" << src.mVendor << endl;
                INFN_LOG(SeverityLevel::debug) << "Dest Device=" << dest.mDevice << " PonType=" << dest.mPonType << " vendor=" << dest.mVendor << endl;

                found = true;
                break;
            }
        }
    }

    return found;
}

unsigned int DumpBcmTxLineToBcmHost(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    Bcm81725Lane param;
     int pre2  = 0;
     int pre   = 0;
     int main  = 0;
     int post  = 0;
     int post2 = 0;
     int post3 = 0;

     int dumpPre2 = 0;
     int dumpPre   = 0;
     int dumpMain  = 0;
     int dumpPost  = 0;
     int dumpPost2 = 0;
     int dumpPost3 = 0;
     bool found = false;
     dp::DeviceInfo src {"", "" , ""};
     dp::DeviceInfo dest {"", "" , ""};
     unsigned int linkId = 0;
     unsigned int linkLoss = 0;
     unsigned int destDev = 0;
     unsigned int destLaneNum = 0;
     unsigned int srcDevNum = 0;


     if ( (rate == gearbox::RATE_4_100GE)
           ||
         (port != gearbox::PORT3        &&
          port != gearbox::PORT4        &&
          port != gearbox::PORT5        &&
          port != gearbox::PORT6        &&
          port != gearbox::PORT11       &&
          port != gearbox::PORT12       &&
          port != gearbox::PORT13       &&
          port != gearbox::PORT14) )
     {
         out << " No BCM Tx Line to BCM Host Serdes for port=" << port << endl;
         return cNaSerdes;
     }


     out << endl<< "=======Dumping Bcm Tx Line to Bcm Host=======" << endl;
     std::vector<PortConfig> portConfig;
     gearbox::GetPortConfigEntry(portConfig, port, rate);

     for (unsigned int i = 0; i < portConfig.size(); i++)
     {
         gearbox::GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
         if (!adapter)
         {
             out << "gearbox init failed" << endl;
             return cFailBcmTxLineToBcmHost;
         }

         for (uint16 laneNum = 0; laneNum < (sizeof(uint16) * 8 /*num bits*/); laneNum++)
         {
             unsigned int bitMask = (1 << laneNum);

             if ( (portConfig[i].bcmUnit == 0x8 ||
                   portConfig[i].bcmUnit == 0x4)
                     &&
                  (portConfig[i].line.laneMap & bitMask) )
             {


                 std::map<unsigned int, std::vector<SerdesDumpData>> dump;
                 serdesTuner->GetSerdesDumpData(dump, BcmLineToBcmHost);

                 // Line side
                 param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, bitMask};

                 found = GetBcmTxDump(dump, port, srcDevNum, laneNum, dumpPre2, dumpPre, dumpMain, dumpPost, dumpPost2, dumpPost3,
                                      destDev, destLaneNum, src, dest, linkId, linkLoss);

                 // Bcm Line Tx to Bcm Host
                 out <<             "  SrcBus         : "   << gearbox::bus2String(portConfig[i].bus) << endl
                     << std::hex << "  SrcUnit        : 0x" << portConfig[i].bcmUnit << std::dec << endl
                     <<             "  SrcDevNum      : "   << srcDevNum << endl
                     <<             "  SrcDev         : "   << src.mDevice << endl
                     <<             "  SrcPonType     : "   << src.mPonType << endl
                     <<             "  SrcVendor      : "   << src.mVendor << endl
                     <<             "  SrcSide        : "   << side2String(cLineSide) << endl
                     <<             "  SrcLaneNum     : "   << laneNum << endl
                     << std::hex << "  SrcLane Mask   : 0x" << portConfig[i].line.laneMap  << std::dec << endl
                     <<             "  LinkId         : "   << linkId << endl
                     <<             "  LinkLoss       : "   << linkLoss << endl
                     <<             " *DestUnit       : "   << destDev << endl
                     <<             " *DestDestLane   : "   << destLaneNum << endl
                     <<             " *DestDev        : "   << dest.mDevice << endl
                     <<             " *DestPonType    : "   << dest.mPonType << endl
                     <<             " *DestVendor     : "   << dest.mVendor << endl
                     <<             "  Found          : "   << found << endl << endl;

                 if (found == false)
                 {
                     status |= cFailBcmTxLineToBcmHost;
                 }

                 std::string title = GetFoundTitle(found);

                 if (!adapter->getTxFir(param, pre2, pre, main, post, post2, post3))
                 {
                     out << "         ==BCM Value==" << " " << title << endl;
                     out << "  Pre2  :" << setw(7) << pre2  << setw(15) <<  dumpPre2 << endl;
                     out << "  Pre   :" << setw(7) << pre   << setw(15) <<  dumpPre << endl;
                     out << "  Main  :" << setw(7) << main  << setw(15) <<  dumpMain << endl;
                     out << "  Post  :" << setw(7) << post  << setw(15) <<  dumpPost << endl;
                     out << "  Post2 :" << setw(7) << post2 << setw(15) <<  dumpPost2 << endl;
                     out << "  Post3 :" << setw(7) << post3 << setw(15) <<  dumpPost3 << endl << endl;

                     if (pre2  != dumpPre2  ||
                         pre   != dumpPre   ||
                         main  != dumpMain  ||
                         post  != dumpPost  ||
                         post2 != dumpPost2 ||
                         post3 != dumpPost3)
                     {
                         status |= cFailBcmTxLineToBcmHost;
                     }
                 }
                 else
                 {
                     out << "Error trying to get Tx FIR for port=" << port << endl;
                     status |= cFailBcmTxLineToBcmHost;
                 }
             }
         }
     }
     return status;
}

bool IsBcmLineValid(unsigned int port, unsigned int bcmNum)
{
    bool valid = false;

    if ( (port == gearbox::PORT1  ||
          port == gearbox::PORT2  ||
          port == gearbox::PORT9  ||
          port == gearbox::PORT10)
            &&
          bcmNum == 0x8)
    {
        valid = true;
    }

    else if ( (port == gearbox::PORT3  ||
               port == gearbox::PORT4  ||
               port == gearbox::PORT5  ||
               port == gearbox::PORT6  ||
               port == gearbox::PORT11 ||
               port == gearbox::PORT12 ||
               port == gearbox::PORT13 ||
               port == gearbox::PORT14)
                 &&
               bcmNum == 0xc)
    {
        valid = true;
    }
    else if ( (port == gearbox::PORT7  ||
               port == gearbox::PORT8  ||
               port == gearbox::PORT15 ||
               port == gearbox::PORT16)
                 &&
               bcmNum == 0x4)
    {
        valid = true;
    }

    return valid;
}

unsigned int DumpBcmTxLineToTom(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    Bcm81725Lane param;
     int pre2  = 0;
     int pre   = 0;
     int main  = 0;
     int post  = 0;
     int post2 = 0;
     int post3 = 0;

     int dumpPre2 = 0;
     int dumpPre   = 0;
     int dumpMain  = 0;
     int dumpPost  = 0;
     int dumpPost2 = 0;
     int dumpPost3 = 0;
     bool found = false;

     dp::DeviceInfo src {"", "" , ""};
     dp::DeviceInfo dest {"", "" , ""};
     unsigned int linkId = 0;
     unsigned int linkLoss = 0;
     unsigned int destDev = 0;
     unsigned int destLaneNum = 0;
     unsigned int srcDevNum = 0;

     out << endl<< "=======Dumping Bcm Tx Line to TOM=======" << endl;
     std::vector<PortConfig> portConfig;
     gearbox::GetPortConfigEntry(portConfig, port, rate);

     gearbox::GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();

     for (unsigned int i = 0; i < portConfig.size(); i++)
     {
         if (!adapter)
         {
             out << "gearbox init failed" << endl;
             return cFailBcmTxLineToTom;
         }

         for (uint16 laneNum = 0; laneNum < (sizeof(uint16) * 8 /*num bits*/); laneNum++)
         {
             unsigned int bitMask = (1 << laneNum);

             if (IsBcmLineValid(port, portConfig[i].bcmUnit) == true &&
                 portConfig[i].line.laneMap & bitMask)
             {
                 std::map<unsigned int, std::vector<SerdesDumpData>> dump;
                 serdesTuner->GetSerdesDumpData(dump, BcmToTom);

                 // Line side
                 param = {portConfig[i].bus, portConfig[i].bcmUnit, cLineSide, bitMask};

                 found = GetBcmTxDump(dump, port, srcDevNum, laneNum, dumpPre2, dumpPre, dumpMain, dumpPost, dumpPost2, dumpPost3,
                                      destDev, destLaneNum, src, dest, linkId, linkLoss);


                 // Bcm Line Tx to Bcm Host
                 out <<             "  SrcBus         : "   << gearbox::bus2String(portConfig[i].bus) << endl
                     << std::hex << "  SrcUnit        : 0x" << portConfig[i].bcmUnit << endl << std::dec
                     <<             "  SrcDevNum      : "   << srcDevNum << endl
                     <<             "  SrcDev         : "   << src.mDevice << endl
                     <<             "  SrcPonType     : "   << src.mPonType << endl
                     <<             "  SrcVendor      : "   << src.mVendor << endl
                     <<             "  SrcSide        : "   << side2String(cLineSide) << endl
                     <<             "  SrcLaneNum     : "   << laneNum << endl
                     << std::hex << "  SrcLane Mask   : 0x" << portConfig[i].line.laneMap << std::dec << endl
                     <<             "  LinkId         : "   << linkId << endl
                     <<             "  LinkLoss       : "   << linkLoss << endl
                     <<             " *DestUnit       : "   << destDev << endl
                     <<             " *DestDestLane   : "   << destLaneNum << endl
                     <<             " *DestDev        : "   << dest.mDevice << endl
                     <<             " *DestPonType    : "   << dest.mPonType << endl
                     <<             " *DestVendor     : "   << dest.mVendor << endl
                     <<             "  Found          : "   << found << endl << endl;

                 if (found == false)
                 {
                     status |= cFailBcmTxLineToTom;
                 }


                 std::string title = GetFoundTitle(found);

                 if (!adapter->getTxFir(param, pre2, pre, main, post, post2, post3))
                 {
                     out << "         ==BCM Value==" << " " << title << endl;
                     out << "  Pre2  :" << setw(7) << pre2  << setw(15) <<  dumpPre2 << endl;
                     out << "  Pre   :" << setw(7) << pre   << setw(15) <<  dumpPre << endl;
                     out << "  Main  :" << setw(7) << main  << setw(15) <<  dumpMain << endl;
                     out << "  Post  :" << setw(7) << post  << setw(15) <<  dumpPost << endl;
                     out << "  Post2 :" << setw(7) << post2 << setw(15) <<  dumpPost2 << endl;
                     out << "  Post3 :" << setw(7) << post3 << setw(15) <<  dumpPost3 << endl << endl;

                     if (pre2  != dumpPre2  ||
                         pre   != dumpPre   ||
                         main  != dumpMain  ||
                         post  != dumpPost  ||
                         post2 != dumpPost2 ||
                         post3 != dumpPost3)
                     {
                         status |= cFailBcmTxLineToTom;
                     }

                 }
                 else
                 {
                     out << "Error trying to get Tx FIR for port=" << port << endl;
                     status |= cFailBcmTxLineToTom;
                 }
             }
         }
     }
     return status;
}


unsigned int DumpAtlTxToBcm(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    int dumpPre2 = 0;
    int dumpPre   = 0;
    int dumpMain  = 0;
    int dumpPost  = 0;
    int dumpPost2 = 0;
    int dumpPost3 = 0;
    bool found = false;
    dp::DeviceInfo src {"", "" , ""};
    dp::DeviceInfo dest {"", "" , ""};
    unsigned int linkId = 0;
    unsigned int linkLoss = 0;
    unsigned int destDev = 0;
    unsigned int destLaneNum = 0;
    unsigned int srcDevNum = 0;

    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();

    std::map<unsigned int, AtlBcmSerdesTxCfg> atlBcmMap;

    serdesTuner->GetAtlBcmMapParams(atlBcmMap);



    if (rate >= gearbox::NUM_RATES ||
        port > gearbox::NUM_PORTS  ||
        port < gearbox::PORT1)
    {
        out << "Invalid rate or port, rate=" << rate << " port=" << port << endl;
        return cFailAtlTxToBcm;
    }

    out << "===Printing Programmed Atlantic Tx Params to BCM Host===" << endl;


    out << "  Port: " << port <<  endl;
    out << "  Rate:" << gearbox::rateEnum2String((Bcm81725DataRate_t)rate) << endl << endl;

    unsigned int atlPortLane = 0;


    for (unsigned int lane = 0; lane < tuning::cAtlNumLanes[rate]; lane++)
    {
        // Get lane number for atlantic
        atlPortLane = serdesTuner->GetAtlBcmSerdesTxOffset(port, (gearbox::Bcm81725DataRate)rate, lane);

        auto elem = atlBcmMap.find(atlPortLane);

        std::map<unsigned int, std::vector<SerdesDumpData>> dump;
        serdesTuner->GetSerdesDumpData(dump, AtlToBcm);
        // We are actually retrieving values for Atl Tx, but GetBcmTxDump does the same.
        found = GetBcmTxDump(dump, port, srcDevNum, atlPortLane-1, dumpPre2, dumpPre, dumpMain, dumpPost, dumpPost2, dumpPost3,
                                              destDev, destLaneNum, src, dest, linkId, linkLoss);
        if (found == false)
        {
            status |= cFailAtlTxToBcm;
        }

        if (elem != atlBcmMap.end())
        {
            out << "  SrcLane    : " << setw(6) << elem->first-1 << endl;
            out << "  SrcDev     : " << setw(6) << src.mDevice << endl;
            out << "  SrcPonType : " << setw(6) << src.mPonType << endl;
            out << "  SrcVendor  : " << setw(6) << src.mVendor << endl;
            out << " *DestLane   : " << setw(6) << destLaneNum << endl;
            out << " *DestDevNum : " << setw(6) << destDev << endl;
            out << " *DestDev    : " << setw(6) << dest.mDevice << endl;
            out << " *DestPonType: " << setw(6) << dest.mPonType << endl;
            out << " *DestVendor : " << setw(6) << dest.mVendor << endl;
            out << "  LinkId     : " << setw(6) << linkId << endl;
            out << "  LinkLoss   : " << setw(6) << linkLoss << endl;
            out << "  Found      : " << setw(6) << found << endl << endl;
            out << "  txPolarity : " << setw(6) << elem->second.txPolarity << endl;
            out << "  rxPolarity : " << setw(6) << elem->second.rxPolarity << endl;
            out << "  Tx Pre 2   : " << setw(6) << elem->second.txPre2 << endl;
            out << "  Tx Pre 1   : " << setw(6) << elem->second.txPre1 << endl;
            out << "  Tx Main    : " << setw(6) << elem->second.txMain << endl;
            out << "  Tx Post 1  : " << setw(6) << elem->second.txPost1 << endl;
            out << "  Tx Post 2  : " << setw(6) << elem->second.txPost2 << endl;
            out << "  Tx Post 3  : " << setw(6) << elem->second.txPost3 << endl << endl;
        }
        else
        {
            out << "No serdes tuning on this Atlantic to BCM lane. Lane=" << atlPortLane << endl;
        }
    }

    return status;
}

unsigned int DumpAtlToBcmRx(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    Bcm81725Lane param;
    unsigned int autoPeakingEn = 0;
    bool found = false;
    unsigned int srcDevNum;
    unsigned int srcLaneNum;
    dp::DeviceInfo src {"", "" , ""};
    dp::DeviceInfo dest {"", "" , ""};
    unsigned int linkId;
    unsigned int linkLoss;

    std::vector<PortConfig> portConfig;
    gearbox::GetPortConfigEntry(portConfig, port, rate);

    out << endl << "=======Dumping Atlantic to BCM Rx Host=======" << endl;
    for (unsigned int i = 0; i < portConfig.size(); i++)
    {
        gearbox::GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
        if (!adapter)
        {
            out << "gearbox init failed" << endl;
            return cFailAtlToBcmRx;
        }

        for (uint16 laneNum = 0; laneNum < (sizeof(uint16) * 8 /*num bits*/); laneNum++)
        {
            unsigned int bitMask = (1 << laneNum);

            if ( (portConfig[i].bcmUnit == 0x4 ||
                  portConfig[i].bcmUnit == 0x8)
                   &&
                 (portConfig[i].host.laneMap & bitMask) )
            {


                std::map<unsigned int, int64_t> rxValue;
                unsigned int dumpAutoPeakingEn = 0;

                std::map<unsigned int, std::vector<SerdesDumpData>> dump;
                serdesTuner->GetSerdesDumpData(dump, AtlToBcm);

                param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, bitMask};
                found = GetBcmRxDump(dump, port, laneNum, rxValue, srcDevNum, srcLaneNum, src, dest, linkId, linkLoss);

                // Bcm Line to Host Rx
                out <<             "  DestBus      : "   << gearbox::bus2String(portConfig[i].bus) << endl
                    << std::hex << "  DestUnit     : 0x" << portConfig[i].bcmUnit  << endl << std::dec
                    <<             "  DestSide     : "   << side2String(cHostSide) << endl
                    <<             "  DestLaneNum  : "   << laneNum  << endl
                    << std::hex << "  DestLane Mask: 0x" << portConfig[i].host.laneMap << std::dec << endl
                    <<             "  LinkId       : "   << linkId << endl
                    <<             "  LinkLoss     : "   << linkLoss << endl
                    <<             " *SrcLaneNum   : "   << srcLaneNum << endl
                    <<             " *SrcVendor    : "   << src.mVendor << endl
                    <<             " *SrcDev       : "   << src.mDevice << endl
                    <<             " *SrcPonType   : "   << src.mPonType << endl
                    <<             "  Found        : "   << found << endl << endl;

                if (found == false)
                {
                    status |= cFailAtlToBcmRx;
                }

                auto elem = rxValue.find(gearbox::DS_AutoPeakingEnable);

                if (elem == rxValue.end())
                {
                    out << "Can not find Rx AutoPeakingEn params." << endl;
                }
                else
                {
                    dumpAutoPeakingEn = (unsigned int)elem->second;
                }

                bcm_plp_dsrds_firmware_lane_config_t lnConf;
                memset(&lnConf, 0, sizeof(bcm_plp_dsrds_firmware_lane_config_t));

                string title = GetFoundTitle(found);

                // BCM Host Rx
                if (!adapter->getLaneConfigDs(param, lnConf))
                {
                    out << "                  ==BCM Value==" << " " << title << endl;
                    out << " AutoPeakingEn:       " << lnConf.AutoPeakingEnable << setw(15) << dumpAutoPeakingEn << endl;

                    if (lnConf.AutoPeakingEnable != dumpAutoPeakingEn)
                    {
                        status |= cFailAtlToBcmRx;
                    }
                }
                else
                {
                    out << "cmdGetRxAutoPeakingEn failed" << endl;
                    status |= cFailAtlToBcmRx;
                }
                out << endl;
            }
        }
    }
    return status;
}

unsigned int DumpBcmTxToAtl(ostream& out, unsigned int port, unsigned int rate)
{
    unsigned int status = 0;
    SerdesTuner* serdesTuner = SerdesTunerSingleton::Instance();
    Bcm81725Lane param;
     int pre2  = 0;
     int pre   = 0;
     int main  = 0;
     int post  = 0;
     int post2 = 0;
     int post3 = 0;

     int dumpPre2 = 0;
     int dumpPre   = 0;
     int dumpMain  = 0;
     int dumpPost  = 0;
     int dumpPost2 = 0;
     int dumpPost3 = 0;
     bool found = false;
     dp::DeviceInfo src {"", "" , ""};
     dp::DeviceInfo dest {"", "" , ""};
     unsigned int linkId = 0;
     unsigned int linkLoss = 0;
     unsigned int destDev = 0;
     unsigned int destLaneNum = 0;
     unsigned int srcDevNum = 0;


     std::vector<PortConfig> portConfig;
     gearbox::GetPortConfigEntry(portConfig, port, rate);

     out << "=======Dumping Bcm Tx Host to Atlantic=======" << endl;
     for (unsigned int i = 0; i < portConfig.size(); i++)
     {
         gearbox::GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
         if (!adapter)
         {
             out << "gearbox init failed" << endl;
             return cFailBcmTxToAtl;
         }

         for (uint16 laneNum = 0; laneNum < (sizeof(uint16) * 8 /*num bits*/); laneNum++)
         {
             unsigned int bitMask = (1 << laneNum);

             if ( (portConfig[i].bcmUnit == 0x4 ||
                   portConfig[i].bcmUnit == 0x8)
                     &&
                  (portConfig[i].host.laneMap & bitMask) )
             {
                 // Bcm Host Tx to Bcm Line
                 out << endl;


                 std::map<unsigned int, std::vector<SerdesDumpData>> dump;
                 serdesTuner->GetSerdesDumpData(dump, BcmToAtl);

                 // Host side
                 param = {portConfig[i].bus, portConfig[i].bcmUnit, cHostSide, bitMask};

                 found = GetBcmTxDump(dump, port, srcDevNum, laneNum, dumpPre2, dumpPre, dumpMain, dumpPost, dumpPost2, dumpPost3,
                                      destDev, destLaneNum, src, dest, linkId, linkLoss);


                 out <<             "  SrcBus         : "   << gearbox::bus2String(portConfig[i].bus) << endl
                     << std::hex << "  SrcUnit        : 0x" << portConfig[i].bcmUnit << endl << std::dec
                     <<             "  SrcDevNum      : "   << srcDevNum << endl
                     <<             "  SrcDev         : "   << src.mDevice << endl
                     <<             "  SrcPonType     : "   << src.mPonType << endl
                     <<             "  SrcVendor      : "   << src.mVendor << endl
                     <<             "  SrcSide        : "   << side2String(cHostSide) << endl
                     <<             "  SrcLaneNum     : "   << laneNum << endl
                     << std::hex << "  SrcLane Mask   : 0x" << portConfig[i].host.laneMap  << std::dec << endl
                     <<             "  LinkId         : "   << linkId << endl
                     <<             "  LinkLoss       : "   << linkLoss << endl
                     <<             " *DestUnit       : "   << destDev <<  endl
                     <<             " *DestDestLane   : "   << destLaneNum << endl
                     <<             " *DestDev        : "   << dest.mDevice << endl
                     <<             " *DestPonType    : "   << dest.mPonType << endl
                     <<             " *DestVendor     : "   << dest.mVendor << endl
                     <<             "  Found          : "   << found << endl << endl;

                 if (found == false)
                 {
                     status |= cFailBcmTxToAtl;
                 }

                 std::string title = GetFoundTitle(found);

                 if (!adapter->getTxFir(param, pre2, pre, main, post, post2, post3))
                 {
                     out << "         ==BCM Value==" << " " << title << endl;
                     out << "  Pre2  :" << setw(7) << pre2  << setw(15) <<  dumpPre2  << endl;
                     out << "  Pre   :" << setw(7) << pre   << setw(15) <<  dumpPre   << endl;
                     out << "  Main  :" << setw(7) << main  << setw(15) <<  dumpMain  << endl;
                     out << "  Post  :" << setw(7) << post  << setw(15) <<  dumpPost  << endl;
                     out << "  Post2 :" << setw(7) << post2 << setw(15) <<  dumpPost2 << endl;
                     out << "  Post3 :" << setw(7) << post3 << setw(15) <<  dumpPost3 << endl << endl;

                     if (pre2  != dumpPre2  ||
                         pre   != dumpPre   ||
                         main  != dumpMain  ||
                         post  != dumpPost  ||
                         post2 != dumpPost2 ||
                         post3 != dumpPost3)
                     {
                         status |= cFailBcmTxToAtl;
                     }
                 }
                 else
                 {
                     out << "Error trying to get Tx FIR for port=" << port << endl;
                     status |= cFailBcmTxToAtl;
                 }
             }
         }
     }
     return status;
}

void ShowResults(unsigned int status, ostream& out)
{
    out << "===Serdes Results===" << endl;
    if (status & cFailTomTx)
    {
        out << "TomTx              : Fail" << endl;
    }
    else
    {
        out << "TomTx              : Pass" << endl;
    }

    if (status & cFailTomRx)
    {
        out << "TomRx              : Fail" << endl;
    }
    else
    {
        out << "TomRx              : Pass" << endl;
    }

    if (status & cFailBcmTxLineToTom)
    {
        out << "BcmTxLineToTom     : Fail" << endl;
    }
    else
    {
        out << "BcmTxLineToTom     : Pass" << endl;
    }

    if (status & cFailBcmTxHostToBcmLine)
    {
        out << "BcmTxHostToBcmLine : Fail" << endl;
    }
    else if (status & cNaSerdes)
    {
        out << "BcmTxHostToBcmLine : NA" << endl;
    }
    else
    {
        out << "BcmTxHostToBcmLine : Pass" << endl;
    }

    if (status & cFailBcmLineToRxHost)
    {
        out << "BcmLineToRxHost    : Fail" << endl;
    }
    else if (status & cNaSerdes)
    {
        out << "BcmLineToRxHost    : NA" << endl;
    }
    else
    {
        out << "BcmLineToRxHost    : Pass" << endl;
    }

    if (status & cFailBcmTxLineToBcmHost)
    {
        out << "BcmTxLineToBcmHost : Fail" << endl;
    }
    else if (status & cNaSerdes)
    {
        out << "BcmTxLineToBcmHost : NA" << endl;
    }
    else
    {
        out << "BcmTxLineToBcmHost : Pass" << endl;
    }

    if (status & cFailBcmTxToAtl)
    {
        out << "BcmTxToAtl         : Fail" << endl;
    }
    else
    {
        out << "BcmTxToAtl         : Pass" << endl;
    }

    if (status & cFailAtlTxToBcm)
    {
        out << "AtlTxToBcm         : Fail" << endl;
    }
    else
    {
        out << "AtlTxToBcm         : Pass" << endl;
    }

    if (status & cFailAtlToBcmRx)
    {
        out << "AtlToBcmRx         : Fail" << endl;
    }
    else
    {
        out << "AtlToBcmRx         : Pass" << endl;
    }
}

void cmdDumpSerdesPortValues(ostream& out, unsigned int port)
{
    unsigned int status = 0;
    gearbox::Bcm81725DataRate rate;
    bool rateFound = true;
    gearbox::GearBoxAdIf* adapter = GearBoxAdapterSingleton::Instance();
    if (adapter->getCachedPortRate(port, rate) != 0)
    {
        rateFound = false;
        // Assume 100G lanes
        rate = RATE_100GE;
    }
    out << "Rate: " << gearbox::rateEnum2String(rate) << endl;
    status |= DumpTomTx(out, port, rate);
    status |= DumpTomRx(out, port, rate);
    status |= DumpBcmTxLineToTom(out, port, rate);
    // No RxLineToTom
    status |= DumpBcmTxHostToBcmLine(out, port, rate);
    out << endl << "=======BCM Rx Line Params are N/A=======" << endl;
    status |= DumpBcmTxLineToBcmHost(out, port, rate);
    status |= DumpBcmLineToRxHost(out, port, rate);
    status |= DumpBcmTxToAtl(out, port, rate);
    status |= DumpAtlToBcmRx(out, port, rate);
    status |= DumpAtlTxToBcm(out, port, rate);
    // No BcmToAtlRx

    if (rateFound == false)
    {
        out << "****Port=" << port << " might not be configured, defaulting to 100G lanes.****" << endl;
    }
    out << endl << "* means connected to" << endl;
    out << "Lanes are 0 based." << endl << endl;
    out << "Rate: " << gearbox::rateEnum2String(rate) << endl;;
    ShowResults(status, out);
}


void cmdDumpSrdsTnrMgrAll(ostream& out)
{
    INFN_LOG(SeverityLevel::info) << "";

    auto startTime = std::chrono::steady_clock::now();

    out << endl << "===srds_tnr_mgr===" << endl;
    for (unsigned int port = gearbox::PORT1; port <= gearbox::PORT16; port++)
    {
        out << "DumpSerdesPortValues " << port << endl;
        cmdDumpSerdesPortValues(out, port);
    }

    auto endTime = std::chrono::steady_clock::now();

    std::chrono::seconds elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    out << "**** Serdes Cli Collection Time Duration: " << elapsedSec.count() << " seconds" << endl;

    INFN_LOG(SeverityLevel::info) << "Serdes  Cli Collection Time Duration: " << elapsedSec.count() << " seconds" ;
}
