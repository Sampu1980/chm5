#include <DpPm.h>

using namespace DataPlane;

DPMSPmCallbacks* DPMSPmCallbacks::instance = NULL;
map<string, DpmsCbHandler*> DPMSPmCallbacks::cbhMap;

DPMSPmCallbacks::DPMSPmCallbacks()
{
    cbhMap.clear();
}

DPMSPmCallbacks *
DPMSPmCallbacks::getInstance()
{
    if (instance == NULL) {
        instance = new DPMSPmCallbacks;
    }
    return instance;
}

bool DPMSPmCallbacks::RegisterCallBack(string className, DpmsCbHandler* callbackHandler)
{
    if (className.empty() || (callbackHandler == NULL)) return false;

    cbhMap[className] = callbackHandler;

    return true;
}

void DPMSPmCallbacks::UnregisterCallBack(string className)
{
    if (className.empty()) return;

	auto itr = cbhMap.find(className);
	if (itr != cbhMap.end())
	{
		if (itr->second) delete itr->second;
		cbhMap.erase(itr);
	}
}

bool DPMSPmCallbacks::CallCallback(string className, DcoBasePM *basePMp)
{
    if (className.empty()) return false;
    if (cbhMap.find(className) == cbhMap.end()) return false;
    //Make call to callback handler
    DpmsCbHandler* callbackHandler = cbhMap[className];
	INFN_LOG(SeverityLevel::debug) << __func__ << " trigger OnRcvPm for className: " << className << endl;
    return callbackHandler->OnRcvPm(className, *basePMp);
}
