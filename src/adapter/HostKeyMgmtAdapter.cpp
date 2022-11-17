#include "HostKeyMgmtAdapter.h"
#include "NORFlashUtil.h"
#include "InfnLogger.h"
#include <google/protobuf/util/json_util.h>
#include <string>
#include "DpEnv.h"
#include "InfnLogger.h"
#include "DataPlaneMgr.h"

using google::protobuf::util::MessageToJsonString;
using google::protobuf::Message;

using namespace NORDriver;

namespace DpAdapter 
{
    HostKeyMgmtAdapter::HostKeyMgmtAdapter()
    {
        //@todo : Add specific initialization
    }

    HostKeyMgmtAdapter::~HostKeyMgmtAdapter()
    {
        for(auto iter:mHostKeyMgmtConfigMap)
        {
            if(iter.second)
            {
                delete iter.second;
            }
        }
    }

    void HostKeyMgmtAdapter::Start()
    {
        Chm6KeyMgmtState state;
        state.mutable_base_state()->mutable_config_id()->set_value(GetChm6StateObjConfigId());
        FillKeyMgmtState(state);
        mCreateKeyStCb(state);
    }

    std::string HostKeyMgmtAdapter::GetChm6StateObjConfigId()
    {
        return ( DataPlaneMgrSingleton::Instance()->getChassisIdStr() + "-" + dp_env::DpEnvSingleton::Instance()->getSlotNum() + "-" + "HOST");
    }

    void HostKeyMgmtAdapter::UpdateKeyMgmtConfig(Chm6KeyMgmtConfig* keyConfig)
    {
        std::string configStr;
        MessageToJsonString(*keyConfig, &configStr);
        INFN_LOG(SeverityLevel::debug) << __func__ << " Host KeyMgmt config " << 
                " " << configStr << std::endl;

        std::vector<std::string> keysToDelete;

        if(keyConfig->has_keymgmt_config())
        {
            hal_keymgmt::KeyMgmtOperation op = keyConfig->keymgmt_config().operation();

            //Update NORFlash in case of add and delete
            if (op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY ||
                    op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY)
            {
                hal_keymgmt::ImageSigningKey isk = keyConfig->keymgmt_config().isk();

                //Check if key exists in the local cache
                auto iter = mHostKeyMgmtConfigMap.find(isk.key_name().value());
                if(iter != mHostKeyMgmtConfigMap.end())
                {
                    KeyMgmtConfigData* pData = iter->second;
                    if(op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY) 
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
                    if(op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY)
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
                    if(op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY)
                    {
                        isDeleted = true;
                    }
                    else if(op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY)
                    {
                        isDeleted = false;
                    }

                    KeyMgmtConfigData* pNewData = new KeyMgmtConfigData(isk, isDeleted);
                    mHostKeyMgmtConfigMap[isk.key_name().value()] = pNewData;
                    INFN_LOG(SeverityLevel::info) << __func__ << " Added to cache " << isk.key_name().value();
                }

                imageKeyList myImageKeyList;
                imageKey tmpKey;
                tmpKey.key_name = isk.key_name().value();
                tmpKey.key_serial_number = isk.key_serial_number().value();
                tmpKey.issuer_name = isk.issuer_name().value();
                tmpKey.key_length = isk.key_length().value();
                tmpKey.key_payload = isk.key_payload().value();
                tmpKey.signature_hash_scheme = isk.signature_hash_scheme().value();
                tmpKey.signature_algorithm = isk.signature_algorithm().value();
                //krk_name attribute is used to represent the signature key id
                tmpKey.signature_key_id = isk.krk_name().value();
                tmpKey.signature_payload = isk.signature_payload().value();
                tmpKey.signature_gen_time = isk.signature_gen_time().value();
                myImageKeyList.insert(std::pair<string,imageKey>(tmpKey.key_name, tmpKey));
                if (op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_ADD_KEY)
                {

#ifndef ARCH_x86
                    //Adding key to NOR flash
                    NORFlashUtil::GetInstance().AddToNor(myImageKeyList);
#endif
                }
                else if( op == hal_keymgmt::KeyMgmtOperation::KEY_MGMT_OPERATION_DELETE_KEY)
                {
#ifndef ARCH_x86
                    //Removing key from NOR flash
                    NORFlashUtil::GetInstance().DeleteFromNor(tmpKey.key_name);
#endif
                    //Add the key being deleted to state object as key with NA data fields, beingdeleted = true
                    keysToDelete.push_back(tmpKey.key_name);
                }
            }
        }

        Chm6KeyMgmtState state;
        state.mutable_base_state()->mutable_config_id()->set_value(GetChm6StateObjConfigId());

        UpdateKeyMgmtState(state, keysToDelete);
       
    }

    namespace
{
    BoolWrapper ConvertBoolToBoolWrapper(bool input)
    {
        BoolWrapper op = BoolWrapper::BOOL_UNSPECIFIED;
        switch(input)
        {
            case 0:
                op = BoolWrapper::BOOL_FALSE;
                break;
            case 1:
                op = BoolWrapper::BOOL_TRUE;
                break;
        }
        return op;

    }
}

    void HostKeyMgmtAdapter::FillKeyMgmtState(Chm6KeyMgmtState &stateObj)
    {
        // Read KRK
        imageKeyList myKRKList;

#ifndef ARCH_x86
        NORFlashUtil::GetInstance().GetKRKListFromGecko(myKRKList);
#endif

        for(auto imagekey:myKRKList)
        {
            stateObj.mutable_keymgmt_state()->mutable_krk()->mutable_key_name()->set_value(imagekey.second.key_name);
            stateObj.mutable_keymgmt_state()->mutable_krk()->mutable_key_serial_number()->set_value(imagekey.second.key_serial_number);
            stateObj.mutable_keymgmt_state()->mutable_krk()->mutable_issuer_name()->set_value(imagekey.second.issuer_name);
            stateObj.mutable_keymgmt_state()->mutable_krk()->mutable_key_length()->set_value(imagekey.second.key_length);
            stateObj.mutable_keymgmt_state()->mutable_krk()->mutable_key_payload()->set_value(imagekey.second.key_payload);
        } 


#ifndef ARCH_x86
        // Read DevId cert
        std::string devIdCert = NORFlashUtil::GetInstance().GetIdevIdCert();
        stateObj.mutable_keymgmt_state()->mutable_idevid()->mutable_cert()->set_value(devIdCert);
        INFN_LOG(SeverityLevel::debug) << __func__ << " DevId certificate: " << devIdCert;
#endif

        // Read ISKs
        imageKeyList myImageKeyList;
        auto isk_map = stateObj.mutable_keymgmt_state()->mutable_isk_map();


#ifndef ARCH_x86
        NORFlashUtil::GetInstance().ReadFromNor(myImageKeyList);
#endif

        for(auto imageKey:myImageKeyList)
        {
            hal_keymgmt::ImageSigningKey isk;
            isk.set_cpu(hal_keymgmt::IskCpu::ISK_CPU_HOST);
            isk.mutable_key_name()->set_value(imageKey.second.key_name);
            isk.mutable_key_serial_number()->set_value(imageKey.second.key_serial_number);
            isk.mutable_issuer_name()->set_value(imageKey.second.issuer_name);
            isk.mutable_key_length()->set_value(imageKey.second.key_length);
            isk.mutable_key_payload()->set_value(imageKey.second.key_payload);
            isk.mutable_signature_hash_scheme()->set_value(imageKey.second.signature_hash_scheme);
            isk.mutable_signature_algorithm()->set_value(imageKey.second.signature_algorithm);
            //isk.mutable_signature_key_id()->set_value(imageKey.second.signature_key_id);
            //krk_name used to represent signature key id
            isk.mutable_krk_name()->set_value(imageKey.second.signature_key_id);
            isk.mutable_signature_payload()->set_value(imageKey.second.signature_payload);
            isk.mutable_signature_gen_time()->set_value(imageKey.second.signature_gen_time);
            isk.set_being_deleted(BoolWrapper::BOOL_FALSE);
            isk.set_is_key_in_use(ConvertBoolToBoolWrapper(imageKey.second.is_key_in_use));
            isk.set_is_key_verified(ConvertBoolToBoolWrapper(imageKey.second.is_key_verified));
            
            (*isk_map)[imageKey.second.key_name] = isk;
        }
    }

    void HostKeyMgmtAdapter::UpdateKeyMgmtState(Chm6KeyMgmtState &stateObj, std::vector<std::string> keysToDelete)
    {
        if( true == stateObj.base_state().mark_for_delete().value() )
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " delete";
            mDeleteKeyStCb(stateObj);
        }
        else
        {
            INFN_LOG(SeverityLevel::info) << __func__ << " redis update/create";
            FillKeyMgmtState(stateObj);

            if(keysToDelete.size() != 0)
            {
                auto isk_map = stateObj.mutable_keymgmt_state()->mutable_isk_map();
                for(auto key:keysToDelete)
                {
                    auto isk_map_iter = isk_map->find(key);
                    if(isk_map_iter == isk_map->end())
                    {
                        //Key was successfully deleted from NOR
                        //Fill up NA data and send update
                        hal_keymgmt::ImageSigningKey isk;
                        isk.set_cpu(hal_keymgmt::IskCpu::ISK_CPU_HOST);
                        isk.mutable_key_name()->set_value(key);
                        isk.mutable_key_serial_number()->set_value("NA");
                        isk.mutable_issuer_name()->set_value("NA");
                        isk.mutable_key_length()->set_value(1);
                        isk.mutable_key_payload()->set_value("NA");
                        isk.mutable_signature_hash_scheme()->set_value("NA");
                        isk.mutable_signature_algorithm()->set_value("NA");
                        isk.mutable_krk_name()->set_value("NA");
                        //isk.mutable_signature_key_id()->set_value("NA");
                        isk.mutable_signature_payload()->set_value("NA");
                        isk.mutable_signature_gen_time()->set_value("NA");
                        isk.set_being_deleted(BoolWrapper::BOOL_TRUE);
                        isk.set_is_key_in_use(BoolWrapper::BOOL_FALSE);
                        isk.set_is_key_verified(BoolWrapper::BOOL_FALSE);

                        (*isk_map)[key] = isk;

                    }
                    
                }

            }
            mUpdateKeyStCb(stateObj);
        }     
    }

}
