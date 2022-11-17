/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#ifndef _DATAPLANEMGRLOG_H_
#define _DATAPLANEMGRLOG_H_

#include <fstream>
#include <mutex>

#include <SingletonService.h>

namespace DataPlane
{

class DataPlaneMgrLog
{
public:

    DataPlaneMgrLog(std::string logFilePath);
    ~DataPlaneMgrLog() { mFile.close(); }

    void AddLog(std::ostringstream& aLine);
    void ResetLog();

private:

    std::mutex mFileMtx;

    std::ofstream mFile;

};

typedef SingletonService<DataPlaneMgrLog> DpMgrLogSingleton;

} // namespace DataPlane

#endif /* _DATAPLANEMGRLOG_H_ */
