/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef _DP_ENV_H_
#define _DP_ENV_H_

#include <string>
#include <vector>

#include <SingletonService.h>

namespace dp_env {

class DpEnv
{
public:
    DpEnv();
    ~DpEnv() {}

    bool        isSimEnv()           { return mIsSimEnv;       }
    bool        isPmEnable()         { return mIsPmEnable;     }  // Temporary
    bool        isPmDebug()          { return mIsPmDebug;      }
    std::string getServerNmDcoZynq() { return mServerNmDcoZynq;}
    std::string getServerNmDcoNxp()  { return mServerNmDcoNxp; }
    std::string getSlotNum()         { return mSlotNum;        }
    std::string getChassisId()       { return mChassisId;      }
    std::string getDcoSimConnZynq()  { return mDcoSimConnZynq; }
    std::string getDcoSimConnNxp()   { return mDcoSimConnNxp; }
    bool        isForceSyncReady()   { return mIsForceSyncRdy; }
    bool        isBlockSyncReady()   { return mIsBlockSyncRdy; }
    bool        isCardSigned()       { return mIsCardSigned; }

    void updateEnv();

private:

    void updateSimEnv(std::string strBrdInfo);
    void updatePmEnable(std::string strPmEnable);  // Temporary
    void updatePmDebug(std::string strPmDebug);

    bool mIsSimEnv;
    bool mIsPmEnable;  // Temporary
    bool mIsPmDebug;
    std::string mServerNmDcoZynq;
    std::string mServerNmDcoNxp;
    std::string mSlotNum;
    std::string mChassisId;
    std::string mDcoSimConnZynq;
    std::string mDcoSimConnNxp;
    bool mIsForceSyncRdy;
    bool mIsBlockSyncRdy;
    bool mIsCardSigned;

    static const std::string cEnvStrBoardInfo;
    static const std::string cEnvStrServerNmDcoZynq;
    static const std::string cEnvStrServerNmDcoNxp;
    static const std::string cEnvStrSlotNum;
    static const std::string cEnvStrChassisId;
    static const std::string cEnvStrPmEnable;  // Temporary
    static const std::string cEnvStrPmDebug;
    static const std::string cEnvDcoSimConnStateZ;
    static const std::string cEnvDcoSimConnStateN;
    static const std::string cEnvStrForceSyncReady;
    static const std::string cEnvStrBlockSyncReady;
    static const std::string cEnvStrChm6Signed;

};

typedef SingletonService<DpEnv> DpEnvSingleton;

}  // namespace dp_env



#endif // _DP_ENV_H_
