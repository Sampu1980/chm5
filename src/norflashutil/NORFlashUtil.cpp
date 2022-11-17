/**
 **  @file
 **
 **
 ** Copyright (c) 2020, Infinera.
 ** */

#include <set>

#include "keymgmt_include/KeyMgmtUtils.h"
#include "keymgmt_include/FlashAccessMtdioWrapper.h"
#include <gecko_include/CpuGeckoIntf.h>
#include <gecko_include/RaStub.h>

#include "NORFlashUtil.h"
#include <InfnLogger.h>

using namespace NORDriver;

#ifndef ARCH_x86
#include <NorISKUtil.h>

extern "C"
{
    int infn_keyctl_add(const char* iskbuff, const char* keyName, char* errorMsg);
    int infn_keyctl_revoke(const char* keyName, char* errorMsg);
}
#endif

NORFlashUtil& NORFlashUtil::GetInstance()
{
    static NORFlashUtil instance;
    return instance;
}

void NORFlashUtil::InitializeGeckoService()
{
    int initValue = 0;
    void *pShmMutex   = NULL;
    INFN_LOG(SeverityLevel::info)<<"Get Gecko Flag";
    initValue = ShmMutex::Initialize();
    if (0 != initValue)
    {
        INFN_LOG(SeverityLevel::error) << "Error: shm object not found";
        return;
    }

    pShmMutex = ShmMutex::Get((char *)"gecko");
    INFN_LOG(SeverityLevel::debug) << "Created shmmutex object";
    if (NULL == pShmMutex)
    {
        INFN_LOG(SeverityLevel::error) << "shmmutex for Gecko not found";
        return;
    }

    CpuGeckoIntf *gckIntf = (new CpuGeckoIntf(pShmMutex));
    CpuGeckoIntfService::InstallInstance(gckIntf);

    geckoStubRa *raStub = new geckoStubRa();
    RaStubService::InstallInstance(raStub);
}

NORFlashUtil::NORFlashUtil()
{
#ifndef ARCH_x86
    NorISKUtilService::InstallInstance(new NorISKUtil());
    InitializeGeckoService();
#endif
    myShelf = "";
    mySlot = "";
}

NORFlashUtil::~NORFlashUtil()
{
#ifndef ARCH_x86
    auto norisk = NorISKUtilService::UninstallInstance();
    if(norisk)
    {
        delete norisk;
        norisk = NULL;
    }
#endif
}

namespace
{
    const uint32_t ISK_BIN_SZ = 4096;
    const char* MTD_DEV_ISK1 = "/dev/mtd1";
    const char* MTD_DEV_ISK2 = "/dev/mtd3";
    const std::string KERNEL_SIG_TLV_FILE  = "/boot/image.ub.sig.tlv.bin";

#ifndef ARCH_x86
    void PopulateImageKey(EccNorEntry_t* _norKPtr, imageKey& imageKeyInst, bool isKrk)
    {
        char name[64] = {0};
        if(_norKPtr == NULL)
        {
            INFN_LOG(SeverityLevel::error) << "_norKPtr = NULL";
            return;
        }

        if (_norKPtr->EcKey->keyName.len == 0)
        {
            INFN_LOG(SeverityLevel::error) << "Keyname length = " << _norKPtr->EcKey->keyName.len;
            return;
        }

        memcpy(name, _norKPtr->EcKey->keyName.data, _norKPtr->EcKey->keyName.len);
        imageKeyInst.key_name = std::string(name);

        uint64_t ser_num = 0;
        memcpy(&ser_num, _norKPtr->EcKey->keySerialNum.data, sizeof(ser_num));
        imageKeyInst.key_serial_number = to_string(ser_num);

        memset(name, 0, 64);
        memcpy(name, _norKPtr->EcKey->issuerName.data, _norKPtr->EcKey->issuerName.len);
        imageKeyInst.issuer_name = std::string(name);

        imageKeyInst.key_payload = KeyMgmtUtil::getBytesInHexStr(_norKPtr->EcKey->keyPayload.data, _norKPtr->EcKey->keyPayload.len);
        imageKeyInst.key_length = imageKeyInst.key_payload.length();

        if (isKrk)
        {
            return;
        }

        if( *(_norKPtr->EcKey->enable.data) )
        {
            imageKeyInst.is_key_verified = true;
        }
        else
        {
            imageKeyInst.is_key_verified = false;
        }

        if (_norKPtr->sigCount == 0)
        {
            INFN_LOG(SeverityLevel::error) << "No Signatures found";
            return;
        }

        imageKeyInst.signature_payload = KeyMgmtUtil::getBytesInHexStr(_norKPtr->EcKeySigs[0]->sigPayload.data,
                _norKPtr->EcKeySigs[0]->sigPayload.len);

        uint64_t sig_gen_time = 0;
        memcpy(&sig_gen_time, _norKPtr->EcKeySigs[0]->sigGenTime.data, sizeof(sig_gen_time));
        imageKeyInst.signature_gen_time = std::to_string(sig_gen_time);
        char sig_id[64] = {0};
        memcpy(sig_id, _norKPtr->EcKeySigs[0]->sigKeyId.data, _norKPtr->EcKeySigs[0]->sigKeyId.len);
        imageKeyInst.signature_key_id = std::string(sig_id);
        
        if (*(_norKPtr->EcKeySigs[0]->sigHashScheme.data) == 0)
        {
            imageKeyInst.signature_hash_scheme = "SHA2_256";
        }
        else if (*(_norKPtr->EcKeySigs[0]->sigHashScheme.data) == 1)
        {
            imageKeyInst.signature_hash_scheme = "SHA2_384";
        }
        else if (*(_norKPtr->EcKeySigs[0]->sigHashScheme.data) == 2)
        {
            imageKeyInst.signature_hash_scheme = "SHA2_512";
        }

        if (*(_norKPtr->EcKeySigs[0]->sigAlgo.data) == 0)
        {
            imageKeyInst.signature_algorithm = "ECDSA";
        }
        else if (*(_norKPtr->EcKeySigs[0]->sigAlgo.data) == 1)
        {
            imageKeyInst.signature_algorithm = "RSA";
        }

    }
#endif

    void CleanUpBuffer(uint8_t*& buff)
    {
        if (buff)
        {
            free(buff);
            buff = NULL;
        }
    }

} //End Empty namespace

void NORFlashUtil::DumpImageKeyList(imageKeyList& myImageKeyList)
{
    for (const auto& image: myImageKeyList)
    {
        INFN_LOG(SeverityLevel::info) << "shelf_id                  = " << image.second.shelf_id;
        INFN_LOG(SeverityLevel::info) << "slot_id                   = " << image.second.slot_id;
        INFN_LOG(SeverityLevel::info) << "cpu                       = " << image.second.cpu;
        INFN_LOG(SeverityLevel::info) << "key_name                  = " << image.second.key_name;
        INFN_LOG(SeverityLevel::info) << "key_serial_number         = " << image.second.key_serial_number;
        INFN_LOG(SeverityLevel::info) << "issuer_name               = " << image.second.issuer_name;
        INFN_LOG(SeverityLevel::info) << "key_length                = " << image.second.key_length;
        INFN_LOG(SeverityLevel::info) << "key_payload               = " << image.second.key_payload;
        INFN_LOG(SeverityLevel::info) << "krk_name                  = " << image.second.krk_name;
        INFN_LOG(SeverityLevel::info) << "is_key_in_use             = " << image.second.is_key_in_use;
        INFN_LOG(SeverityLevel::info) << "is_key_verified           = " << image.second.is_key_verified;
        INFN_LOG(SeverityLevel::info) << "signature_hash_scheme     = " << image.second.signature_hash_scheme;
        INFN_LOG(SeverityLevel::info) << "signature_algorithm       = " << image.second.signature_algorithm;
        INFN_LOG(SeverityLevel::info) << "signature_key_id          = " << image.second.signature_key_id;
        INFN_LOG(SeverityLevel::info) << "signature_payload         = " << image.second.signature_payload;
        INFN_LOG(SeverityLevel::info) << "signature_gen_time        = " << image.second.signature_gen_time;
    }
}

void NORFlashUtil::DumpImageKeyList(imageKeyList& myImageKeyList, std::ostream &out)
{
    for (const auto& image: myImageKeyList)
    {
        out << "shelf_id                  = " << image.second.shelf_id << endl;
        out << "slot_id                   = " << image.second.slot_id << endl;
        out << "cpu                       = " << image.second.cpu << endl;
        out << "key_name                  = " << image.second.key_name << endl;
        out << "key_serial_number         = " << image.second.key_serial_number << endl;
        out << "issuer_name               = " << image.second.issuer_name << endl;
        out << "key_length                = " << image.second.key_length << endl;
        out << "key_payload               = " << image.second.key_payload << endl;
        out << "krk_name                  = " << image.second.krk_name << endl;
        out << "is_key_in_use             = " << image.second.is_key_in_use << endl;
        out << "is_key_verified           = " << image.second.is_key_verified << endl;
        out << "signature_hash_scheme     = " << image.second.signature_hash_scheme << endl;
        out << "signature_algorithm       = " << image.second.signature_algorithm << endl;
        out << "signature_key_id          = " << image.second.signature_key_id << endl;
        out << "signature_payload         = " << image.second.signature_payload << endl;
        out << "signature_gen_time        = " << image.second.signature_gen_time << endl;
    }
}

int NORFlashUtil::AddToNor(imageKeyList& myImageKeyList)
{
    std::lock_guard<std::mutex> lock(mNorAccessMutex);

    FlashAccessMtdioWrapper gFlashObjIsk1(MTD_DEV_ISK1);
    FlashAccessMtdioWrapper gFlashObjIsk2(MTD_DEV_ISK2);

    DumpImageKeyList(myImageKeyList);

    int ret = 0;
#ifndef ARCH_x86
    NorISKUtil* norutil = NorISKUtilService::Instance();
    if(norutil == NULL)
    {
        INFN_LOG(SeverityLevel::error) << "NorIskUtil instance is not created";
        return -1;
    }

    uint8_t* new_isk1_buff = NULL;
    size_t new_isk1_buff_len = 0;
    size_t isk1_buff_len = ISK_BIN_SZ;
    uint8_t* isk1_buff = NULL;
    isk1_buff = (uint8_t*)malloc(ISK_BIN_SZ);

    if(isk1_buff == NULL)
    {
        INFN_LOG(SeverityLevel::error) << "NORFlashUtil::AddToNor - malloc failure";
        return -2;
    }

    for (auto myImageKey: myImageKeyList)
    {
        if (gFlashObjIsk1.ReadFlash(0,ISK_BIN_SZ,isk1_buff) != 0)
        {
            INFN_LOG(SeverityLevel::error) << "Read from NOR Flash Failed";
            CleanUpBuffer(isk1_buff);
            return -3;
        }

        EccNorEntry_t* isk = NULL;

        norutil->InitEccNorIntf(&isk);

        norutil->AddEccValueIntf("Key_Name",const_cast<char*>(myImageKey.second.key_name.c_str()),isk);
        norutil->AddEccValueIntf("Key_SerialNum",const_cast<char*>(myImageKey.second.key_serial_number.c_str()),isk);
        norutil->AddEccValueIntf("Issuer_Name",const_cast<char*>(myImageKey.second.issuer_name.c_str()),isk);
        norutil->AddEccValueIntf("Key_Payload",const_cast<char*>(myImageKey.second.key_payload.c_str()),isk);
        norutil->AddEccValueIntf("Signature_Payload",
                const_cast<char*>(myImageKey.second.signature_payload.c_str()),isk);
        norutil->AddEccValueIntf("Signature_Hash_Scheme",
                const_cast<char*>(myImageKey.second.signature_hash_scheme.c_str()),isk);
        norutil->AddEccValueIntf("Signature_Algo",
                const_cast<char*>(myImageKey.second.signature_algorithm.c_str()),isk);
        norutil->AddEccValueIntf("Signature_gen_time",
                const_cast<char*>(myImageKey.second.signature_gen_time.c_str()),isk);
        norutil->AddEccValueIntf("Signing_Key_ID",
                const_cast<char*>(myImageKey.second.signature_key_id.c_str()),isk);
        auto retAdd = norutil->AddSecKeyIntf(isk,isk1_buff,isk1_buff_len,&new_isk1_buff,&new_isk1_buff_len);

        norutil->FreeIsklistIntf(isk);

        if(new_isk1_buff && (new_isk1_buff_len == ISK_BIN_SZ) && (retAdd == 0) )
        {
            if (0 != gFlashObjIsk1.WriteToFlash(0,new_isk1_buff_len,new_isk1_buff))
            {
                //write to nor failed
                INFN_LOG(SeverityLevel::error) << "WriteToFlash Primary Sector Failure";
                CleanUpBuffer(isk1_buff);
                CleanUpBuffer(new_isk1_buff);
                return -4;
            }
        }
        else
        {
            INFN_LOG(SeverityLevel::error) << "ISK Buffer is not valid to write onto the flash";
            CleanUpBuffer(isk1_buff);
            CleanUpBuffer(new_isk1_buff);
            return -5;
        }
		char errorMsg[64];
		std::string keyring = "<Key_data_start>\nKey_Name:";
		keyring+= myImageKey.second.key_name;
		keyring+= "\\0:64\nKey_SerialNum:";
		keyring+= myImageKey.second.key_serial_number;
        keyring+= "\\0:8\nIssuer_Name:";
        keyring+= myImageKey.second.issuer_name;
        keyring+= "\\0:20\nKey_length:";
        keyring+= myImageKey.second.key_length;
        keyring+= "\\0:4\nKey_Payload:";
		keyring+= myImageKey.second.key_payload;
        keyring+= "\\0:512\nKey_Type:hex\\0:12\nKey_Algo:ECC-P521\\0:12\n<Key_data_end>\n<Sig_data_start>\nSignature_length:139\\0:4\nSignature_Payload:";
        keyring+= myImageKey.second.signature_payload;
        keyring+= "\\0:512\nSignature_Hash_Scheme:";
        keyring+= myImageKey.second.signature_hash_scheme;
        keyring+= "\\0:12\nSignature_Algo:";
        keyring+= myImageKey.second.signature_algorithm;
        keyring+= "\\0:16\nSignature_gen_time:";
        keyring+= myImageKey.second.signature_gen_time;
        keyring+= "\\0:8\nSigning_Key_ID:";
        keyring+= myImageKey.second.signature_key_id;
        keyring+= "\\0:64\n<Sig_data_end>";

		INFN_LOG(SeverityLevel::info) << keyring;


        if (infn_keyctl_add(keyring.c_str(), myImageKey.second.key_name.c_str(), errorMsg) < 0)
        {
            INFN_LOG(SeverityLevel::error) << std::string(errorMsg);
        }

    }

    CleanUpBuffer(isk1_buff);
    CleanUpBuffer(new_isk1_buff);
#endif
    return ret;
}

int NORFlashUtil::ReadFromNor(imageKeyList& myImageKeyList)
{
    std::lock_guard<std::mutex> lock(mNorAccessMutex);
#ifndef ARCH_x86
    NorISKUtil* ptrNorIskUtil = NorISKUtilService::Instance();
    FlashAccessMtdioWrapper gFlashObjIsk1(MTD_DEV_ISK1);
    FlashAccessMtdioWrapper gFlashObjIsk2(MTD_DEV_ISK2);
    if( !ptrNorIskUtil )
    {
        INFN_LOG(SeverityLevel::info) << "NorIskUtil instance is not created";
        return -1;
    }

    size_t isk1_buff_len = ISK_BIN_SZ;
    uint8_t* isk1_buff = NULL;
    isk1_buff = (uint8_t*)malloc(ISK_BIN_SZ);
    if(isk1_buff == NULL)
    {
        INFN_LOG(SeverityLevel::error) << "malloc failure";
        return -2;
    }


    if (gFlashObjIsk1.ReadFlash(0,ISK_BIN_SZ,isk1_buff) == -1)
    {
        INFN_LOG(SeverityLevel::error) << "Read flash primary failed";
        CleanUpBuffer(isk1_buff);
        return -3;
    }    

    
    int numofkeys = 0;

    EccNorEntry_t* isklist = NULL;

    UpdateInUseList();

    ptrNorIskUtil->ReadISKNorIntf(&isklist,isk1_buff,isk1_buff_len,&numofkeys,0);

    INFN_LOG(SeverityLevel::info) << "Number of keys = " << numofkeys;

    if (numofkeys == 0)
    {
        CleanUpBuffer(isk1_buff);
        ptrNorIskUtil->FreeIsklistIntf(isklist);
        return -4;
    }

    for(int i=0; i<numofkeys; i++)
    {
        imageKey imageKeyInst;
        PopulateImageKey(isklist+i, imageKeyInst, false);
        if( true == KeyMgmtUtil::isKeyPresentInList(mInUseList, imageKeyInst.key_name) )
        {
            imageKeyInst.is_key_in_use = true;
        }
        else
        {
            imageKeyInst.is_key_in_use = false;
        }
        myImageKeyList.emplace(imageKeyInst.key_name, imageKeyInst);
    }

    ptrNorIskUtil->FreeIsklistIntf(isklist);
    CleanUpBuffer(isk1_buff);
    DumpImageKeyList(myImageKeyList);
#endif

    return 0;
}

int NORFlashUtil::DeleteFromNor(string keyname)
{
    std::lock_guard<std::mutex> lock(mNorAccessMutex);
    FlashAccessMtdioWrapper gFlashObjIsk1(MTD_DEV_ISK1);

    UpdateInUseList();

    if( true == KeyMgmtUtil::isKeyPresentInList(mInUseList, keyname) )
    {
        INFN_LOG(SeverityLevel::error) << "Failed to delete in-use key:" << keyname;
        return -1;
    }

    uint8_t* isk1_buff = NULL;
    uint8_t* new_isk1_buff = NULL;
    size_t new_isk1_buff_len = 0;
    isk1_buff = (uint8_t*)malloc(ISK_BIN_SZ);

    if(isk1_buff == NULL)
    {
        INFN_LOG(SeverityLevel::error) << "malloc failure";
        return -2;
    }

    size_t isk1_buff_len = ISK_BIN_SZ;
    gFlashObjIsk1.ReadFlash(0,ISK_BIN_SZ,isk1_buff);

#ifndef ARCH_x86
    NorISKUtil* ptrNorIskUtil = NorISKUtilService::Instance();
    if(ptrNorIskUtil)
    {
        if (ptrNorIskUtil->DeleteSecKeyIntf(keyname.c_str(),isk1_buff, isk1_buff_len, &new_isk1_buff, &new_isk1_buff_len) != 0)
        {
            INFN_LOG(SeverityLevel::error) << "Delete ISK from Buffer Error";
            CleanUpBuffer(isk1_buff);
            CleanUpBuffer(new_isk1_buff);
            return -3;
        }
        if(new_isk1_buff && (new_isk1_buff_len == ISK_BIN_SZ) )
        {
            if( gFlashObjIsk1.WriteToFlash(0,new_isk1_buff_len, new_isk1_buff ) != 0)
            {
                //error in updating flash
                INFN_LOG(SeverityLevel::error) << "Delete ISK: error in updating flash";
                CleanUpBuffer(isk1_buff);
                CleanUpBuffer(new_isk1_buff);
                return -4;
            }
        }
    }
    else
    {
        INFN_LOG(SeverityLevel::info) << "NorIskUtil instance is not created";
    }
#endif
    CleanUpBuffer(isk1_buff);
    CleanUpBuffer(new_isk1_buff);

#ifndef ARCH_x86
    char errorMsg[64];
    if (infn_keyctl_revoke(keyname.c_str(), errorMsg) < 0)
    {
        INFN_LOG(SeverityLevel::error) << std::string(errorMsg);
        return -5;
    }
#endif

    return 0;
}

int NORFlashUtil::GetKRKListFromGecko(imageKeyList& myImageKeyList)
{
#ifndef ARCH_x86
    EccNorEntry_t* krklist = NULL;
    int readkrk = 1;
    int numofkeys = 0;
    uint8_t* krk_bin = NULL;
    uint32_t krk_bin_len = 0;
    krk_bin = CpuGeckoIntfService::Instance()->SD_GetKRKList();
    krk_bin_len = CpuGeckoIntfService::Instance()->SD_GetKrkKeyLen();


    NorISKUtil* ptrNorIskUtil = NorISKUtilService::Instance();
    if(ptrNorIskUtil && krk_bin)
    {
        INFN_LOG(SeverityLevel::info) << "KRK List read from Gecko";
        ptrNorIskUtil->ReadISKNorIntf(&krklist,krk_bin,krk_bin_len,&numofkeys, readkrk);
        for(int i=0; i<numofkeys; i++)
        {
            imageKey imageKeyInst;
            PopulateImageKey(krklist+i, imageKeyInst, true);
            myImageKeyList.emplace(imageKeyInst.key_name, imageKeyInst);
        }
        ptrNorIskUtil->FreeIsklistIntf(krklist);
        free(krk_bin);
        krk_bin = NULL;
    }
    else
    {
        INFN_LOG(SeverityLevel::error) << "KRK not read from Gecko";
    }
#endif
    return 0;
}

std::string NORFlashUtil::GetIdevIdCert()
{
    uint32_t der_cert_len = 0;
    uint8_t* der_cert = NULL;
    CpuGeckoIntfService::Instance()->SD_GetCertificate(&der_cert, &der_cert_len);
    if( !der_cert || !der_cert_len )
    {
        INFN_LOG(SeverityLevel::error) << "Unable to get der cert file from Gecko";
        return "";
    }

    std::string iDevID = KeyMgmtUtil::getBytesInHexStr(der_cert, der_cert_len);

    if(der_cert)
    {
        free(der_cert);
        der_cert = NULL;
    }

    return std::move(iDevID);
}

void NORFlashUtil::UpdateInUseList()
{
    mInUseList.clear();
    //Update in-use list from gecko.
    CpuGeckoIntfService::Instance()->SD_GetInUseList(mInUseList);

    //Update in-use from /proc/keys
    KeyMgmtUtil::GetInUseFromProcFile(mInUseList);
#ifndef ARCH_x86
    //Update InUse from kernel sig.tlv
    NorISKUtil* ptrNorIskUtil = NorISKUtilService::Instance();
    if(ptrNorIskUtil)
    {
        char sig_key_id[64+1] = {0};
        ptrNorIskUtil->ParseSigTlvBinIntf(KERNEL_SIG_TLV_FILE.c_str(), sig_key_id, 65);
        mInUseList.push_back(std::string(sig_key_id));
    }
#endif

    //make unique in-use list
    std::set<std::string> inUseList(mInUseList.begin(), mInUseList.end());
    mInUseList.clear();
    std::copy(inUseList.begin(), inUseList.end(), std::back_inserter(mInUseList));
}

const std::vector<std::string>& NORFlashUtil::GetInUseList()
{
    UpdateInUseList();
    return mInUseList;
}
