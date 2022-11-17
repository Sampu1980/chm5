#ifndef HOST_KEY_MGMT_ADAPTER_H
#define HOST_KEY_MGMT_ADAPTER_H

#include "KeyMgmtConfigData.h"
#include "DpProtoDefs.h"
#include <mutex>

namespace DpAdapter 
{
    using KeyMgrCallback = std::function<bool(Chm6KeyMgmtState&)>;

    class HostKeyMgmtAdapter
    {
        public:
            
            HostKeyMgmtAdapter();

	        ~HostKeyMgmtAdapter();

            void Start();

            std::string GetChm6StateObjConfigId();

            void UpdateKeyMgmtConfig(Chm6KeyMgmtConfig* keyConfig);
            
            void UpdateKeyMgmtState(Chm6KeyMgmtState &stateObj, std::vector<std::string> keysToDelete);

            void FillKeyMgmtState(Chm6KeyMgmtState &stateObj);
       	    
            void RegisterStateCreateCallback(KeyMgrCallback f) { mCreateKeyStCb = f; };
            void RegisterStateUpdateCallback(KeyMgrCallback f) { mUpdateKeyStCb = f; };
            void RegisterStateDeleteCallback(KeyMgrCallback f) { mDeleteKeyStCb = f; };
	 
	     private:
            std::map<std::string, KeyMgmtConfigData*> mHostKeyMgmtConfigMap;
            KeyMgrCallback mCreateKeyStCb;
            KeyMgrCallback mUpdateKeyStCb;
            KeyMgrCallback mDeleteKeyStCb;
     };

}//end namespace

#endif