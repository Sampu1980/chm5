#ifndef CHM6_DP_MS_NORFLASHUTIL_H 
#define CHM6_DP_MS_NORFLASHUTIL_H 

#include <mutex>
#include <string>
#include <vector>

#include "ImageKeyHolder.h"

namespace NORDriver
{

    class NORFlashUtil
    {
        public:
            /* Singleton Pattern single object creation */
            static NORFlashUtil& GetInstance();

            /* Public Key Management APIs */
            int AddToNor(imageKeyList& myImageKeyList);
            int ReadFromNor(imageKeyList& myImageKeyList);
            int DeleteFromNor(std::string keyname);
            int GetKRKListFromGecko(imageKeyList& myImageKeyList);
            void DumpImageKeyList(imageKeyList& myImageKeyList, std::ostream &out);
            std::string GetIdevIdCert();
            const std::vector<std::string>& GetInUseList();

            /* Delete implicit creations */
            NORFlashUtil(NORFlashUtil const&) = delete;
            NORFlashUtil(NORFlashUtil &&) = delete;
            NORFlashUtil& operator=(NORFlashUtil const&) = delete;
            NORFlashUtil& operator=(NORFlashUtil &&) = delete;

        private:
            NORFlashUtil();
            ~NORFlashUtil();

            void UpdateInUseList();
            void DumpImageKeyList(imageKeyList& myImageKeyList);
            void InitializeGeckoService();

            std::string myShelf;
            std::string mySlot;
            std::vector<std::string> mInUseList;
            std::mutex mNorAccessMutex;
    };

} //End Namespace NORDriver

#endif /* CHM6_DP_MS_NORFLASHUTIL_H */