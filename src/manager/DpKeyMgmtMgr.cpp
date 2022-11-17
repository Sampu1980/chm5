#include "DpKeyMgmtMgr.h"
#include "InfnLogger.h"
#include "DataPlaneMgr.h"

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

namespace hal_common  = infinera::hal::common::v2;

namespace DataPlane
{

DpKeyMgmtMgr::DpKeyMgmtMgr()
    : DpObjectMgr("KeyMgmt", ((1 << DCC_ZYNQ) | (1 << DCC_NXP)) )
{
    
}

DpKeyMgmtMgr::~DpKeyMgmtMgr()
{
}

void DpKeyMgmtMgr::initialize()
{
    mpDcoSecProcKeyMgmtAdapter = std::make_unique<DpAdapter::DcoSecProcKeyMgmtAdapter>();
    mpHostKeyMgmtAdapter = std::make_unique<DpAdapter::HostKeyMgmtAdapter>();
    mpDcoZynqKeyMgmtAdapter = std::make_unique<DpAdapter::DcoZynqKeyMgmtAdapter>();

    if(mpDcoSecProcKeyMgmtAdapter)
    {
        //Register callback function to get callback on icdp updates
        mpDcoSecProcKeyMgmtAdapter->RegisterStateCreateCallback( &DpKeyMgmtMgr::CreateKeyMgmtState );
        mpDcoSecProcKeyMgmtAdapter->RegisterStateUpdateCallback( &DpKeyMgmtMgr::UpdateKeyMgmtState );
        mpDcoSecProcKeyMgmtAdapter->RegisterStateDeleteCallback( &DpKeyMgmtMgr::DeleteKeyMgmtState );
    }

    if(mpHostKeyMgmtAdapter)
    {
        //Register callback function to get callback on icdp updates
        mpHostKeyMgmtAdapter->RegisterStateCreateCallback( &DpKeyMgmtMgr::CreateKeyMgmtState );
        mpHostKeyMgmtAdapter->RegisterStateUpdateCallback( &DpKeyMgmtMgr::UpdateKeyMgmtState );
        mpHostKeyMgmtAdapter->RegisterStateDeleteCallback( &DpKeyMgmtMgr::DeleteKeyMgmtState );
    }

    mIsInitDone = true;
}

void DpKeyMgmtMgr::start()
{
    if(mpDcoSecProcKeyMgmtAdapter)
    {
        mpDcoSecProcKeyMgmtAdapter->Initialize();
    }

    if(mpHostKeyMgmtAdapter)
    {
        mpHostKeyMgmtAdapter->Start();
    }
}

void DpKeyMgmtMgr::connectionUpdate()
{
    INFN_LOG(SeverityLevel::info) << "Updating with new connection state isConnectionUp: " << mIsConnectionUp;
    mpDcoSecProcKeyMgmtAdapter->SetConnectionState(mIsConnectionUp);
    mpDcoZynqKeyMgmtAdapter->SetConnectionState(mIsConnectionUp);
}

bool DpKeyMgmtMgr::checkAndCreate(google::protobuf::Message* objMsg)
{
    Chm6KeyMgmtConfig *pKeyMgmtCfg = dynamic_cast<Chm6KeyMgmtConfig*>(objMsg);
    if (pKeyMgmtCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for KeyMgmt Config";
            return true;
        }

        hal_keymgmt::IskCpu cpu = pKeyMgmtCfg->mutable_keymgmt_config()->mutable_isk()->cpu();
        INFN_LOG(SeverityLevel::info) << " cpu=" << cpu;

        if (cpu == hal_keymgmt::ISK_CPU_DCO)
        {
            mpDcoZynqKeyMgmtAdapter->CreateOrUpdateKeyMgmtConfigForZynq(pKeyMgmtCfg);
            return true;
        }
        else if (cpu == hal_keymgmt::ISK_CPU_NXP)
        {
            mpDcoSecProcKeyMgmtAdapter->UpdateKeyMgmtConfig(pKeyMgmtCfg);
            return true;
        }
        else if (cpu == hal_keymgmt::ISK_CPU_HOST)
        {
            //Update chm6
            mpHostKeyMgmtAdapter->UpdateKeyMgmtConfig(pKeyMgmtCfg);
            return true;
        }
    }

    return false;
}

bool DpKeyMgmtMgr::checkAndUpdate(google::protobuf::Message* objMsg)
{
    Chm6KeyMgmtConfig *pKeyMgmtCfg = dynamic_cast<Chm6KeyMgmtConfig*>(objMsg);
    if (pKeyMgmtCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for KeyMgmt Config";
            return true;
        }

        hal_keymgmt::IskCpu cpu = pKeyMgmtCfg->mutable_keymgmt_config()->mutable_isk()->cpu();

        INFN_LOG(SeverityLevel::info) << " cpu=" << cpu;

        if (cpu == hal_keymgmt::ISK_CPU_DCO)
        {
            mpDcoZynqKeyMgmtAdapter->CreateOrUpdateKeyMgmtConfigForZynq(pKeyMgmtCfg);
            return true;
        }
        else if (cpu == hal_keymgmt::ISK_CPU_NXP)
        {
            mpDcoSecProcKeyMgmtAdapter->UpdateKeyMgmtConfig(pKeyMgmtCfg);
            return true;
        }
        else if (cpu == hal_keymgmt::ISK_CPU_HOST)
        {
            //Update chm6
            mpHostKeyMgmtAdapter->UpdateKeyMgmtConfig(pKeyMgmtCfg);
            return true;
        }
    }

    return false;
}

bool DpKeyMgmtMgr::checkAndDelete(google::protobuf::Message* objMsg)
{
    //Check for mark_for_delete as true
    Chm6KeyMgmtConfig *pKeyMgmtCfg = dynamic_cast<Chm6KeyMgmtConfig*>(objMsg);
    if (pKeyMgmtCfg)
    {
        if (!mIsInitDone)
        {
            INFN_LOG(SeverityLevel::error) << "Error: Init is not completed. Cannot process callback for KeyMgmt Config";
            return true;
        }

        hal_keymgmt::IskCpu cpu = pKeyMgmtCfg->mutable_keymgmt_config()->mutable_isk()->cpu();

        INFN_LOG(SeverityLevel::info) << " cpu=" << cpu;

        if (cpu == hal_keymgmt::ISK_CPU_DCO)
        {
            mpDcoZynqKeyMgmtAdapter->CreateOrUpdateKeyMgmtConfigForZynq(pKeyMgmtCfg);
            return true;
        }
        else if (cpu == hal_keymgmt::ISK_CPU_NXP)
        {
            mpDcoSecProcKeyMgmtAdapter->UpdateKeyMgmtConfig(pKeyMgmtCfg);
            return true;
        }
        else if (cpu == hal_keymgmt::ISK_CPU_HOST)
        {
            //Update chm6
            mpHostKeyMgmtAdapter->UpdateKeyMgmtConfig(pKeyMgmtCfg);
            return true;
        }
    }

    return false;
}

bool DpKeyMgmtMgr::checkOnResync (google::protobuf::Message* objMsg)
{
    return false;
}


bool DpKeyMgmtMgr::CreateKeyMgmtState(Chm6KeyMgmtState& keyState)
{
    try
    {        
        string stateDataStr;
        MessageToJsonString(keyState, &stateDataStr);
        INFN_LOG(SeverityLevel::debug) << "Key mgmt state create : " << stateDataStr;

        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);
        keyState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        keyState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectCreate(keyState);
        
    }
    catch(const std::exception& e)
    {
        INFN_LOG(SeverityLevel::info) << "Exception key mgmt state create : " << e.what();
        return false;
    }
    return true;
}

bool DpKeyMgmtMgr::UpdateKeyMgmtState(Chm6KeyMgmtState& keyState)
{
    try{

        string stateDataStr;
        MessageToJsonString(keyState, &stateDataStr);
        INFN_LOG(SeverityLevel::debug) << "Key mgmt state update : " << stateDataStr;

        // Retrieve timestamp
        struct timeval tv;
        gettimeofday(&tv, NULL);
        keyState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
        keyState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectUpdate(keyState);
        
        }
        catch(const std::exception& e)
        {
            INFN_LOG(SeverityLevel::info) << "Exception key mgmt state update : " << e.what();
            return false;
        }
        return true;

}

bool DpKeyMgmtMgr::DeleteKeyMgmtState(Chm6KeyMgmtState& keyState)
{ 
    string stateDataStr;
    MessageToJsonString(keyState, &stateDataStr);
    INFN_LOG(SeverityLevel::debug) << "Key mgmt state delete : " << stateDataStr;

    std::vector<std::reference_wrapper<google::protobuf::Message>> vDelObjects;

    // Retrieve timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    keyState.mutable_base_state()->mutable_timestamp()->set_seconds(tv.tv_sec);
    keyState.mutable_base_state()->mutable_timestamp()->set_nanos(tv.tv_usec * 1000);

    vDelObjects.push_back(keyState);
    try
    {
        DataPlaneMgrSingleton::Instance()->DpMgrRedisObjectDelete(vDelObjects); 
    }
    catch(const std::exception& e)
    {
        INFN_LOG(SeverityLevel::info) << "Exception key mgmt state delete : " << e.what();
        return false;
    }
    return true;

}

} //namespace DataPlane
