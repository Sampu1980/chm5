#include "DcoZynqKeyMgmtAdapter.h"
#include "InfnLogger.h"
#include <google/protobuf/util/json_util.h>
#include <string>
#include "DcoCardAdapter.h"
#include <fstream>
#include"DpCardMgr.h"

namespace DpAdapter
{

DcoZynqKeyMgmtAdapter::~DcoZynqKeyMgmtAdapter()
{
    for(auto iter:mZynqKeyMgmtConfigMap)
    {
        if(iter.second)
        {
            delete iter.second;
        }
    }
}

void DcoZynqKeyMgmtAdapter::CreateOrUpdateKeyMgmtConfigForZynq(Chm6KeyMgmtConfig *pKeyMgmtCfg)
{

    DpAdapter::DcoCardAdapter* adapter = DpCardMgrSingleton::Instance()->getAdPtr();

    if(pKeyMgmtCfg->has_keymgmt_config())
    {
        hal_keymgmt::KeyMgmtOperation operation = pKeyMgmtCfg->mutable_keymgmt_config()->operation();

        if (operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY ||
                operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY)
        {
            hal_keymgmt::ImageSigningKey isk = pKeyMgmtCfg->keymgmt_config().isk();

            //Check if key exists in the local cache
            auto iter = mZynqKeyMgmtConfigMap.find(isk.key_name().value());
            if(iter != mZynqKeyMgmtConfigMap.end())
            {
                KeyMgmtConfigData* pData = iter->second;
                if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY) 
                {
                    if(pData && pData->IsKeyDeleted())
                    {
                        INFN_LOG(SeverityLevel::info) << __func__ << " Duplicate DELETE request, dropping " << isk.key_name().value();
                        //Duplicate delete config - drop and return
                        return;
                    }

                    if(pData)
                    {                       
                        pData->ClearKey();
                    }

                }
                if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY)
                {
                    if( pData && (false == pData->IsKeyDeleted()) )
                    {
                        INFN_LOG(SeverityLevel::info) << __func__ << " Duplicate add request, dropping " << isk.key_name().value();
                        //Duplicate add operation - drop request
                        return;
                    }

                    if( pData )
                    {
                        //add key
                        pData->SetKey(isk);
                    }
                }
            }
            else
            {
                //Key not found in cache
                bool isDeleted = false;
                if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY)
                {
                    isDeleted = true;
                }
                else if(operation == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY)
                {
                    isDeleted = false;
                }

                KeyMgmtConfigData* pNewData = new KeyMgmtConfigData(isk, isDeleted);
                mZynqKeyMgmtConfigMap[isk.key_name().value()] = pNewData;
                INFN_LOG(SeverityLevel::info) << __func__ << " Added to cache " << isk.key_name().value();
            }

            if (operation == hal_keymgmt::KEY_MGMT_OPERATION_ADD_KEY)
            {
                std::string keyName = pKeyMgmtCfg->mutable_keymgmt_config()->mutable_isk()->mutable_key_name()->value();
                INFN_LOG(SeverityLevel::info) << "ADD_KEY " << keyName;
#ifndef ARCH_x86
                CreateISKRing(pKeyMgmtCfg);
                std::string keyPath = "127.8.0.1:/opt/infinera/chm6/configs/msapps/ISKRing";
                adapter->addIsk(keyPath);
#endif
            }
            else if (operation == hal_keymgmt::KEY_MGMT_OPERATION_DELETE_KEY)
            {
                std::string keyName = pKeyMgmtCfg->mutable_keymgmt_config()->mutable_isk()->mutable_key_name()->value();
                INFN_LOG(SeverityLevel::info) << "DELETE_KEY " << keyName;

#ifndef ARCH_x86
                adapter->removeIsk(keyName);
#endif
            }
        }
    }

}

bool DcoZynqKeyMgmtAdapter::CreateISKRing(Chm6KeyMgmtConfig *pKeyMgmtCfg)
{

    hal_keymgmt::ImageSigningKey* mutable_isk
        = pKeyMgmtCfg->mutable_keymgmt_config()->mutable_isk();

    ofstream iskRingFile("/opt/infinera/chm6/configs/msapps/ISKRing");

    //Key_type, key_algo and signature_length are constants and wont be picked from incoming config

    iskRingFile
        << "<Key_data_start>\n"
        << "Key_Name:"              << mutable_isk->mutable_key_name()->value()           << "\\0:64\n"
        << "Key_SerialNum:"         << mutable_isk->mutable_key_serial_number()->value()  << "\\0:8\n"
        << "Issuer_Name:"           << mutable_isk->mutable_issuer_name()->value()        << "\\0:20\n"
        << "Key_length:"            << mutable_isk->mutable_key_length()->value()         << "\\0:4\n"
        << "Key_Payload:"           << mutable_isk->mutable_key_payload()->value()        << "\\0:512\n"
        << "Key_Type:"              << "hex"                                              << "\\0:12\n"
        << "Key_Algo:"              << "ECC-P521"                                         << "\\0:12\n"
        << "<Key_data_end>\n";

    iskRingFile
        << "<Sig_data_start>\n"
        << "Signature_length:"      << "512"                                              << "\\0:4\n"
        << "Signature_Payload:"     << mutable_isk->mutable_signature_payload()->value()  << "\\0:512\n"
        << "Signature_Hash_Scheme:" << mutable_isk->mutable_signature_hash_scheme()->value()  <<"\\0:12\n"
        << "Signature_Algo:"        << "ECDSA-SHA512"                                     << "\\0:16\n"
        << "Signature_gen_time:"    << mutable_isk->mutable_signature_gen_time()->value()  << "\\0:8\n"
        << "Signing_Key_ID:"        << mutable_isk->mutable_krk_name()->value()    << "\\0:64\n"
        << "<Sig_data_end>\n";

    iskRingFile.close();

    return true;
}

void DcoZynqKeyMgmtAdapter::SetConnectionState(bool isConnectUp)
{
    if( mIsConnectUp != isConnectUp )
    {
        if(isConnectUp)
        {
            //If connection is up; sync key config from cache
            INFN_LOG(SeverityLevel::info) << " sync config from cache " << mZynqKeyMgmtConfigMap.size();
            SyncFromCache();
        }
    }
    mIsConnectUp =  isConnectUp;
}


void DcoZynqKeyMgmtAdapter::SyncFromCache()
{
    DpAdapter::DcoCardAdapter* adapter = DpCardMgrSingleton::Instance()->getAdPtr();

    std::map<std::string, KeyMgmtConfigData*>::iterator iter;
    for(iter = mZynqKeyMgmtConfigMap.begin(); iter != mZynqKeyMgmtConfigMap.end(); iter++)
    {
        KeyMgmtConfigData* pData = iter->second;
        if(pData)
        {
            hal_keymgmt::ImageSigningKey isk = pData->GetKey();
            if( true ==  pData->IsKeyDeleted() )
            {
                //Call delete key operation
                std::string keyName(iter->first);
#ifndef ARCH_x86
                adapter->removeIsk(keyName);
#endif
 
            }
            else
            {
                Chm6KeyMgmtConfig keyMgmtCfg;
                keyMgmtCfg.mutable_base_config()->mutable_config_id()->set_value(iter->first);
                keyMgmtCfg.mutable_keymgmt_config()->mutable_isk()->CopyFrom(isk);

                //Add key operation
#ifndef ARCH_x86
                CreateISKRing(&keyMgmtCfg);
                std::string keyPath = "127.8.0.1:/opt/infinera/chm6/configs/msapps/ISKRing";
                adapter->addIsk(keyPath);
#endif
 
            }
        }       
    }
}


}//end namespace
