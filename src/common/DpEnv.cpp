/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#include <string>
#include <vector>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include "DpEnv.h"
#include "InfnLogger.h"

namespace dp_env {

const std::string DpEnv::cEnvStrBoardInfo       = "BoardInfo";
const std::string DpEnv::cEnvStrServerNmDcoZynq = "DcoOecIp";
const std::string DpEnv::cEnvStrServerNmDcoNxp  = "DcoSecProcIp";
const std::string DpEnv::cEnvStrSlotNum         = "SlotNo";
const std::string DpEnv::cEnvStrChassisId       = "ChassisId";
const std::string DpEnv::cEnvStrPmEnable        = "PmEnable";  // Temporary
const std::string DpEnv::cEnvStrPmDebug         = "PmDebug";
const std::string DpEnv::cEnvDcoSimConnStateZ   = "DcoSimConnZynq";
const std::string DpEnv::cEnvDcoSimConnStateN   = "DcoSimConnNxp";
const std::string DpEnv::cEnvStrForceSyncReady  = "DpMsForceSyncReady";
const std::string DpEnv::cEnvStrBlockSyncReady  = "DpMsBlockSyncReady";
const std::string DpEnv::cEnvStrChm6Signed = "chm6CardSigned";

DpEnv::DpEnv()
    : mIsSimEnv(false)
    , mIsPmEnable(false)  // Temporary
    , mIsPmDebug(false)
    , mServerNmDcoZynq("gnxi")
    , mServerNmDcoNxp("chm6_grpc_server:50051")
    , mSlotNum("4")
    , mChassisId("1")
    , mDcoSimConnZynq("")
    , mDcoSimConnNxp("")
    , mIsForceSyncRdy(false)
    , mIsBlockSyncRdy(false)
    , mIsCardSigned(false)
{
    updateEnv();
}

void DpEnv::updateEnv()
{
    char *envVal = std::getenv(cEnvStrBoardInfo.c_str());

    INFN_LOG(SeverityLevel::info) << __func__ << " Reading environment variables" << std::endl;

    // Sim Mode Check
    if (envVal)
    {
        std::string tempStr(envVal);
        INFN_LOG(SeverityLevel::info) << "BoardInfo = " << tempStr;
        updateSimEnv(tempStr);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Board info env is EMPTY. Enabling HwMode by default" << std::endl;
        mIsSimEnv =  false; // No env var, assume this is HW platform
    }

    // Temporary - Pm Enable Check
    envVal = std::getenv(cEnvStrPmEnable.c_str());
    if (envVal)
    {
        std::string tempStr(envVal);
        INFN_LOG(SeverityLevel::info) << "PmEnable = " << tempStr;
        updatePmEnable(tempStr);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "PmEnable env is EMPTY. Disabling Pm by default" << std::endl;
        mIsPmEnable =  false;
    }

    // Pm Debug enable
    envVal = std::getenv(cEnvStrPmDebug.c_str());
    if (envVal)
    {
        std::string tempStr(envVal);
        INFN_LOG(SeverityLevel::info) << "PmDebug = " << tempStr;
        updatePmDebug(tempStr);
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "PmDebug env is EMPTY. Disabling Pm by default" << std::endl;
        mIsPmDebug =  false;
    }

    // Dco Zynq Server Name Check
    envVal = std::getenv(cEnvStrServerNmDcoZynq.c_str());
    if (envVal)
    {
        mServerNmDcoZynq = envVal;

        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrServerNmDcoZynq
                  << " = " << mServerNmDcoZynq << std::endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvStrServerNmDcoZynq
                  << " is Missing. Using Default: " << mServerNmDcoZynq << std::endl;
    }

    // Dco Nxp Server Name Check
    envVal = std::getenv(cEnvStrServerNmDcoNxp.c_str());
    if (envVal)
    {
        mServerNmDcoNxp = envVal;

        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrServerNmDcoNxp
                  << " = " << mServerNmDcoNxp << std::endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvStrServerNmDcoNxp
                  << " is Missing. Using Default: " << mServerNmDcoNxp << std::endl;
    }

    // Slot Number Check
    envVal = std::getenv(cEnvStrSlotNum.c_str());
    if (envVal)
    {
        mSlotNum = envVal;

        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrSlotNum
                  << " = " << mSlotNum << std::endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvStrSlotNum
                  << " is Missing. Using Default: " << mSlotNum << std::endl;
    }

    // Chassis Id Check
    envVal = std::getenv(cEnvStrChassisId.c_str());
    if (envVal)
    {
        mChassisId = envVal;

        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrChassisId
                  << " = " << mChassisId << std::endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvStrChassisId
                  << " is Missing. Using Default: " << mChassisId << std::endl;
    }

    // Dco Zynq Connect State (debugging purposes)
    envVal = std::getenv(cEnvDcoSimConnStateZ.c_str());
    if (envVal)
    {
        mDcoSimConnZynq = envVal;

        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvDcoSimConnStateZ
                  << " = " << mDcoSimConnZynq << std::endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvDcoSimConnStateZ
                  << " is Missing. Using Default: " << mDcoSimConnZynq << std::endl;
    }

    // Dco Nxp Connect State (debugging purposes)
    envVal = std::getenv(cEnvDcoSimConnStateN.c_str());
    if (envVal)
    {
        mDcoSimConnNxp = envVal;

        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvDcoSimConnStateN
                  << " = " << mDcoSimConnNxp << std::endl;
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvDcoSimConnStateN
                  << " is Missing. Using Default: " << mDcoSimConnNxp << std::endl;
    }

    // Force Sync Ready Check
    envVal = std::getenv(cEnvStrForceSyncReady.c_str());
    if (envVal)
    {
        std::string tempStr(envVal);
        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrForceSyncReady
                  << " = " << tempStr << std::endl;

        boost::algorithm::to_lower(tempStr);
        if (tempStr == "true")
        {
            mIsForceSyncRdy = true;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvStrForceSyncReady << " is EMPTY. Flag defaults to false" << std::endl;
    }

    // Block Sync Ready Check
    envVal = std::getenv(cEnvStrBlockSyncReady.c_str());
    if (envVal)
    {
        std::string tempStr(envVal);
        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrBlockSyncReady
                  << " = " << tempStr << std::endl;

        boost::algorithm::to_lower(tempStr);
        if (tempStr == "true")
        {
            mIsBlockSyncRdy = true;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvStrBlockSyncReady << " is EMPTY. Flag defaults to false" << std::endl;
    }

    //Check if system is signed
    envVal = std::getenv(cEnvStrChm6Signed.c_str());
     if (envVal)
    {
        std::string tempStr(envVal);
        INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrChm6Signed
                  << " = " << tempStr << std::endl;

        if (tempStr == "1")
        {
            mIsCardSigned = true;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Env Variable " << cEnvStrChm6Signed << " is EMPTY. Flag defaults to false" << std::endl;
    }

}

void DpEnv::updateSimEnv(std::string strBrdInfo)
{
#ifdef ARCH_x86
    // Forcing default for x86 and ignoring env variable
    INFN_LOG(SeverityLevel::info) << "Arch is x86. Setting Sim Mode Env = true" << std::endl;
    mIsSimEnv = true;
#else
    INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrBoardInfo << " = " << strBrdInfo << std::endl;
    if (strBrdInfo != "")
    {
        if((strBrdInfo == "sim") || (strBrdInfo == "eval"))
        {
            INFN_LOG(SeverityLevel::info) << "Sim Mode Env is Enabled" << std::endl;
            mIsSimEnv =  true;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "Hw Mode Env is Enabled" << std::endl;
            mIsSimEnv =  false;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "Board info env is EMPTY. Enabling HwMode for Drivers by default" << std::endl;
        mIsSimEnv =  false; // No env var, assume this is HW platform
    }
#endif
}

// Temporary
void DpEnv::updatePmEnable(std::string strPmEnable)
{
    INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrPmEnable << " = " << strPmEnable;
    if (strPmEnable != "")
    {
        if (strPmEnable == "true")
        {
            INFN_LOG(SeverityLevel::info) << "PmEnable env is Enabled" << std::endl;
            mIsPmEnable =  true;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "PmEnable env is Disabled" << std::endl;
            mIsPmEnable =  false;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "PmEnable env is EMPTY. Disabling Pm by default" << std::endl;
        mIsPmEnable = false;
    }
}

void DpEnv::updatePmDebug(std::string strPmDebug)
{
    INFN_LOG(SeverityLevel::info) << "Found Env Variable " << cEnvStrPmDebug << " = " << strPmDebug;
    if (strPmDebug != "")
    {
        if (strPmDebug == "true")
        {
            INFN_LOG(SeverityLevel::info) << "PmDebug env is Enabled" << std::endl;
            mIsPmDebug =  true;
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << "PmDebug env is Disabled" << std::endl;
            mIsPmDebug =  false;
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "PmEnable env is EMPTY. Disabling Pm by default" << std::endl;
        mIsPmDebug = false;
    }
}


}  // namespace dp_env

