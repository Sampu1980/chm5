#ifndef KEY_MGMT_CONFIG_H
#define KEY_MGMT_CONFIG_H

#include "DpProtoDefs.h"
#include <mutex>

namespace DpAdapter 
{

    class KeyMgmtConfigData
    {
        private:
            bool mIsDeleted;
            bool mIsConfigSuccess;
            hal_keymgmt::ImageSigningKey mIsk;

        public: 
            KeyMgmtConfigData() {
                mIsDeleted = false;
                mIsConfigSuccess = false;
                mIsk.Clear();
            }

            KeyMgmtConfigData(hal_keymgmt::ImageSigningKey key, bool isDeleted) : mIsk(key), mIsDeleted(isDeleted)
        {               
        }

            ~KeyMgmtConfigData()
            {

            }

            bool IsKeyDeleted() {return mIsDeleted; }
            void SetKeyDeleted(bool val) { mIsDeleted = val; }
            void ClearKey() { mIsk.Clear();
                SetKeyDeleted(true); }
            void SetKey(const hal_keymgmt::ImageSigningKey& key) {
                mIsk.CopyFrom(key);
                SetKeyDeleted(false);
            }
            
            hal_keymgmt::ImageSigningKey& GetKey()
            {
                return mIsk;
            }

    };

}
#endif