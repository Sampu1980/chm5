#ifndef DCO_ZYNQ_KEY_MGMT_ADAPTER_H
#define DCO_ZYNQ_KEY_MGMT_ADAPTER_H

#include "DpProtoDefs.h"
#include <mutex>
#include "KeyMgmtConfigData.h"

namespace DpAdapter 
{

    class DcoZynqKeyMgmtAdapter
    {
        public:
            
            DcoZynqKeyMgmtAdapter() { mIsConnectUp = true; };

	        ~DcoZynqKeyMgmtAdapter();

            void CreateOrUpdateKeyMgmtConfigForZynq(Chm6KeyMgmtConfig *pKeyMgmtCfg);

            bool CreateISKRing(Chm6KeyMgmtConfig *pKeyMgmtCfg);
            void SetConnectionState(bool isConnectUp);
            void SyncFromCache();

	     private:
            std::map<std::string, KeyMgmtConfigData*> mZynqKeyMgmtConfigMap;
            bool mIsConnectUp;
            
    };

}//end namespace

#endif