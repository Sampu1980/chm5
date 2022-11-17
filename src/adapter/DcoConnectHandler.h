/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef DCO_CONNECT_HANDLER_H
#define DCO_CONNECT_HANDLER_H

#include <boost/thread.hpp>

#include <SingletonService.h>

#include "CrudService.h"

namespace DataPlane
{

enum DcoConnectStateType
{
    DCS_UNKNOWN = 0,
    DCS_BOOT_IN_PROGRESS,
    DCS_INITIALIZING,
    DCS_CONNECTED,
    DCS_FAILURE,
    NUM_DCO_CONN_STATES
};

enum DcoConnectFailureType
{
    DCF_UNKNOWN = 0,
    DCF_OK,
    DCF_GECKO_SELFTEST_FAIL,
    DCF_GECKO_NOR_VERIFY_FAIL,
    DCF_GECKO_INT_FLASH_VERIFY_FAIL,
    DCF_BOOT_INIT_FAIL,
    DCF_BCM_LINK_FAIL,
    DCF_GRPC_NO_CONNECT,
    NUM_DCO_CONN_FAILURES
};

enum DcoCpuConnectionsType
{
    DCC_ZYNQ = 0,
    DCC_NXP,
    NUM_CPU_CONN_TYPES
};

extern std::string ToString(DcoConnectStateType state);
extern std::string ToString(DcoConnectFailureType error);
extern std::string ToString(DcoCpuConnectionsType connType);
extern std::string ToString(grpc_connectivity_state grpcState);

struct DcoConnectionStatus
{
    DcoConnectStateType   mConnState;
    DcoConnectFailureType mConnFailure;

    DcoConnectionStatus()
        : mConnState(DCS_UNKNOWN)
        , mConnFailure(DCF_UNKNOWN)
    {
    }
};

class DcoConnectMonitor
{
public:

    DcoConnectMonitor(std::string sockAddr,
                      bool isSim,
                      DcoCpuConnectionsType connId,
                      std::string envSimConn = "");
    virtual ~DcoConnectMonitor();

    virtual void initialize();

    DcoConnectionStatus getConnectStatus();
    void dumpConnectStatus(std::ostream& out, bool doPrtHeader = true);

    void setSimStatus(bool isSimEn,
                      DcoConnectionStatus dcoConnStat);
    bool isSimEn() { return mIsSimEn; }
    DcoConnectionStatus getSimConnectStatus();
    bool isSimDcoConnect();
    void dumpSimStatus(std::ostream& out, bool doPrtHeader = true);

    void setSockAddr(std::string sockAddr);
    std::string getSockAddr() { return mSocketAddr; }
    std::shared_ptr<GnmiClient::GnmiCrudService> getCrudPtr() { return mspCrud; }

    void setPollEn(bool isEnable) { mIsPollEn = isEnable; }
    bool getPollEn()              { return mIsPollEn; }

    void dumpGrpcConnectStatus(std::ostream& out, bool doPrtHeader = true);

    void Poll();

protected:

    std::shared_ptr<GnmiClient::GnmiCrudService> mspCrud;
    std::string mSocketAddr;

    boost::thread mThrPoller;

    std::mutex          mStatusMtx;
    DcoConnectionStatus mConnStatus;

    bool mIsSimEn;
    DcoConnectionStatus mSimConnStatus;

    bool mIsPollEn;

    DcoCpuConnectionsType mConnId;
};

class DcoZynqConnectMonitor : public DcoConnectMonitor
{
public:

    DcoZynqConnectMonitor(std::string sockAddr, bool isSim);
    virtual ~DcoZynqConnectMonitor() {};

    virtual void initialize();
};

class DcoNxpConnectMonitor : public DcoConnectMonitor
{
public:

    DcoNxpConnectMonitor(std::string sockAddr, bool isSim);
    virtual ~DcoNxpConnectMonitor() {};

    virtual void initialize();
};

class DcoConnectHandler
{
public:

    DcoConnectHandler() {}
    ~DcoConnectHandler() {}

    void initialize();

    std::shared_ptr<GnmiClient::GnmiCrudService> getCrudPtr(DcoCpuConnectionsType connId);

    DcoConnectMonitor& getDcoConnMon(DcoCpuConnectionsType connId);

    DcoConnectionStatus getDcoConnStat(DcoCpuConnectionsType connId);

    bool isDcoCommReady(DcoCpuConnectionsType connId);

    void dumpConnectStates(std::ostream& os);
    void dumpConnectErrors(std::ostream& os);
    void dumpDcoConnectIds(std::ostream& os);

private:

    std::vector<DcoConnectMonitor*> mvpDcoConnMon;

};

typedef SingletonService<DcoConnectHandler> DcoConnectHandlerSingleton;

} // namespace DataPlane

#endif // DCO_CONNECT_HANDLER_H



