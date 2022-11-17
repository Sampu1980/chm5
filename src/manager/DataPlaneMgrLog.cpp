/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/


#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <time.h>

#include "DataPlaneMgrLog.h"

using namespace std;

namespace DataPlane
{

DataPlaneMgrLog::DataPlaneMgrLog(string logFilePath)
{
    mFile.open(logFilePath, ios::binary | ios::out | ios::trunc);
    mFile.clear();
    if (mFile.is_open())
    {
        cout << "LogFile Open: " << logFilePath << endl;
        mFile << "Begin Log .......... " << endl;
    }
    else
    {
        bool result = false;
        boost::filesystem::path dirPath(logFilePath);

        if (boost::filesystem::create_directories(dirPath.parent_path()))
        {
            mFile.open(logFilePath, ios::app | ios::binary | ios::out);
            mFile.clear();
            if (mFile.is_open())
            {
                cout << "LogFile Open: " << logFilePath;
                mFile << "Begin Log .......... " << endl;
                result = true;
            }
        }
        if (!result)
        {
            string exStr = "Bad File Path: " + logFilePath;
            throw(exStr);
        }
    }
}

void DataPlaneMgrLog::AddLog(ostringstream& aLine)
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mFileMtx);

    time_t curTime = time(0);

    if (mFile.is_open())
    {
        mFile << ctime(&curTime);
        // Move back 1 position to remove cr
        long pos = mFile.tellp();
        mFile.seekp(pos-1);
        mFile << " " << aLine.str() << endl;
    }
    else
    {
        cout << "ERROR: LogFile Closed" << endl;
    }
}

void DataPlaneMgrLog::ResetLog()
{
    // Take Lock
    std::lock_guard<std::mutex> lock(mFileMtx);

    if (mFile.is_open())
    {
        mFile.clear();
        mFile << "Reset Log ......." << endl;
    }
    else
    {
        cout << "ERROR: LogFile Closed" << endl;
    }
}

} //namespace DataPlane



