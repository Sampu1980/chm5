/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include <boost/format.hpp>

#include "DcoConnectHandler.h"
#include "InfnLogger.h"
#include "DpEnv.h"
#include "dp_common.h"

#include <InfnTracer/InfnTracer.h>

// #include "ShmMutexGuard.h"
// #include "CpuGeckoIntf.h" // Gecko Intf
// #include "RaStub.h"       // Gecko's RaStub

namespace DataPlane
{

std::string ToString(DcoConnectStateType state)
{
    switch (state)
    {
        case DCS_UNKNOWN:
            return "DCS_UNKNOWN";
            break;
        case DCS_BOOT_IN_PROGRESS:
            return "DCS_BOOT_IN_PROGRESS";
            break;
        case DCS_INITIALIZING:
            return "DCS_INITIALIZING";
            break;
        case DCS_CONNECTED:
            return "DCS_CONNECTED";
            break;
        case DCS_FAILURE:
            return "DCS_FAILURE";
            break;
        default:
            return "UNSUPPORTED TYPE";
            break;
    }
}

DcoConnectStateType FromString(std::string inStr)
{
    DcoConnectStateType retState = DCS_UNKNOWN;

    if (inStr == "DCS_UNKNOWN")
    {
        retState = DCS_UNKNOWN;
    }
    else if (inStr == "DCS_BOOT_IN_PROGRESS")
    {
        retState = DCS_BOOT_IN_PROGRESS;
    }
    else if (inStr == "DCS_INITIALIZING")
    {
        retState = DCS_INITIALIZING;
    }
    else if (inStr == "DCS_CONNECTED")
    {
        retState = DCS_CONNECTED;
    }
    else if (inStr == "DCS_FAILURE")
    {
        retState = DCS_FAILURE;
    }

    return retState;
}

std::string ToString(DcoConnectFailureType error)
{
    switch (error)
    {
        case DCF_UNKNOWN:
            return "DCF_UNKNOWN";
            break;
        case DCF_OK:
            return "DCF_OK";
            break;
        case DCF_GECKO_SELFTEST_FAIL:
            return "DCF_GECKO_SELFTEST_FAIL";
            break;
        case DCF_GECKO_NOR_VERIFY_FAIL:
            return "DCF_GECKO_NOR_VERIFY_FAIL";
            break;
        case DCF_GECKO_INT_FLASH_VERIFY_FAIL:
            return "DCF_GECKO_INT_FLASH_VERIFY_FAIL";
            break;
        case DCF_BOOT_INIT_FAIL:
            return "DCF_BOOT_INIT_FAIL";
            break;
        case DCF_BCM_LINK_FAIL:
            return "DCF_BCM_LINK_FAIL";
            break;
        case DCF_GRPC_NO_CONNECT:
            return "DCF_GRPC_NO_CONNECT";
            break;
        default:
            return "UNSUPPORTED TYPE";
            break;
    }
}

std::string ToString(DcoCpuConnectionsType connType)
{
    switch (connType)
    {
        case DCC_ZYNQ:
            return "DCC_ZYNQ";
            break;
        case DCC_NXP:
            return "DCC_NXP";
            break;
        default:
            return "UNSUPPORTED TYPE";
            break;
    }
}

std::string ToString(grpc_connectivity_state grpcState)
{
    switch (grpcState)
    {
        case GRPC_CHANNEL_IDLE:
            return "GRPC_CHANNEL_IDLE";
            break;
        case GRPC_CHANNEL_CONNECTING:
            return "GRPC_CHANNEL_CONNECTING";
            break;
        case GRPC_CHANNEL_READY:
            return "GRPC_CHANNEL_READY";
            break;
        case GRPC_CHANNEL_TRANSIENT_FAILURE:
            return "GRPC_CHANNEL_TRANSIENT_FAILURE";
            break;
        case GRPC_CHANNEL_SHUTDOWN:
            return "GRPC_CHANNEL_SHUTDOWN";
            break;
        default:
            return "UNSUPPORTED TYPE";
            break;
    }
}

void initGecko()
{
    // int returnValue = 0;
    // void* pShmMutex   = NULL;

    // returnValue = ShmMutex::Initialize();
    // if (0 != returnValue)
    // {
    //     INFN_LOG(SeverityLevel::error) << "Error: shm object not found";
    //     return;
    // }
    // else
    // {
    //     pShmMutex = ShmMutex::Get((char *)"gecko");
    //     INFN_LOG(SeverityLevel::info) << "Created shmmutex object";
    //     if (NULL == pShmMutex)
    //     {
    //         INFN_LOG(SeverityLevel::info) << "shmmutex for Gecko not found";
    //         return;
    //     }
    // }

    // CpuGeckoIntf *pIntf = new CpuGeckoIntf(pShmMutex);
    // CpuGeckoIntfService::InstallInstance(pIntf);

    // geckoStubRa *raStub = new geckoStubRa();
    // RaStubService::InstallInstance(raStub);
}

void DcoConnectHandler::initialize()
{
    initGecko();

    // Add Zynq
    DcoZynqConnectMonitor *pZynqConnMon =
            new DcoZynqConnectMonitor(GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoZynq()),
                                      dp_env::DpEnvSingleton::Instance()->isSimEnv());

    // Add NXP
    DcoNxpConnectMonitor *pNxpConnMon =
            new DcoNxpConnectMonitor(GEN_GNMI_ADDR(dp_env::DpEnvSingleton::Instance()->getServerNmDcoNxp()),
                                     dp_env::DpEnvSingleton::Instance()->isSimEnv());

    mvpDcoConnMon.push_back((DcoConnectMonitor*)pZynqConnMon);
    mvpDcoConnMon.push_back((DcoConnectMonitor*)pNxpConnMon);

    pZynqConnMon->initialize();
    pNxpConnMon->initialize();
}

std::shared_ptr<GnmiClient::GnmiCrudService> DcoConnectHandler::getCrudPtr(DcoCpuConnectionsType connId)
{
    return mvpDcoConnMon[connId]->getCrudPtr();
}

DcoConnectMonitor& DcoConnectHandler::getDcoConnMon(DcoCpuConnectionsType connId)
{
    if (connId == NUM_CPU_CONN_TYPES)
    {
        uint32_t intId = connId;
        intId--;
        connId = (DcoCpuConnectionsType)intId;
    }

    return *mvpDcoConnMon[connId];
}

DcoConnectionStatus DcoConnectHandler::getDcoConnStat(DcoCpuConnectionsType connId)
{
    if (connId == NUM_CPU_CONN_TYPES)
    {
        INFN_LOG(SeverityLevel::error) << "Error: connId must be less than " << (uint32_t)NUM_CPU_CONN_TYPES;

        DcoConnectionStatus connStat;
        return connStat;    // return defaults
    }

    return mvpDcoConnMon[connId]->getConnectStatus();
}

bool DcoConnectHandler::isDcoCommReady(DcoCpuConnectionsType connId)
{
    DcoConnectionStatus connStat = getDcoConnStat(connId);

    return (connStat.mConnState == DCS_CONNECTED);
}

void DcoConnectHandler::dumpConnectStates(std::ostream& os)
{
    os << "********** Dco Connection States **********" << std::endl << std::endl;

    uint32_t type;
    for(type = DCS_UNKNOWN; type < NUM_DCO_CONN_STATES; type++)
    {
        os << boost::format("%-3d : %-30d") % (int)type % ToString((DcoConnectStateType)type) << std::endl;
    }
}

void DcoConnectHandler::dumpConnectErrors(std::ostream& os)
{
    os << "********** Dco Connection Errors **********" << std::endl << std::endl;

    uint32_t type;
    for(type = DCF_UNKNOWN; type < NUM_DCO_CONN_FAILURES; type++)
    {
        os << boost::format("%-3d : %-30d") % (int)type % ToString((DcoConnectFailureType)type) << std::endl;
    }
}

void DcoConnectHandler::dumpDcoConnectIds(std::ostream& os)
{
    os << "********** Dco Connection IDs **********" << std::endl << std::endl;

    uint32_t type;
    for(type = DCC_ZYNQ; type < NUM_CPU_CONN_TYPES; type++)
    {
        os << boost::format("%-3d : %-30d") % (int)type % ToString((DcoCpuConnectionsType)type) << std::endl;
    }
}

DcoConnectMonitor::DcoConnectMonitor(std::string sockAddr,
                                     bool isSim,
                                     DcoCpuConnectionsType connId,
                                     std::string envSimConn )
    : mSocketAddr(sockAddr)
    , mIsSimEn(isSim)
    , mIsPollEn(true)
    , mConnId(connId)
{
    INFN_LOG(SeverityLevel::info) << "Socket Address: " << mSocketAddr << " isSim: " << mIsSimEn;

    if (isSim)
    {
        mSimConnStatus.mConnState   = DCS_CONNECTED;
        mSimConnStatus.mConnFailure = DCF_OK;

        mIsPollEn = false;  // no need to poll on sim
    }

    // simulating connection state from env variable
    if (envSimConn != "")
    {
        mIsSimEn  = true;
        mIsPollEn = false;

        DcoConnectStateType simState = FromString(envSimConn);

        INFN_LOG(SeverityLevel::info) << "Connection: " << ToString(connId) << " Env DcoConnectState: " << envSimConn
                << " Mapped to Enum: " << ToString(simState);

        mSimConnStatus.mConnState = simState;
        if (simState == DCS_CONNECTED)
        {
            mSimConnStatus.mConnFailure = DCF_OK;
        }
        else
        {
            mSimConnStatus.mConnFailure = DCF_GRPC_NO_CONNECT;
        }
    }
}

DcoConnectMonitor::~DcoConnectMonitor()
{

}

void DcoConnectMonitor::initialize()
{
    INFN_LOG(SeverityLevel::info) << "Starting Polling Thread...";

    mThrPoller = boost::thread(boost::bind(&DcoConnectMonitor::Poll, this));

    mThrPoller.detach();
}

DcoConnectionStatus DcoConnectMonitor::getConnectStatus()
{
    const std::lock_guard<std::mutex> lock(mStatusMtx);

    if (mIsSimEn)
    {
        return mSimConnStatus;
    }
    else
    {
        return mConnStatus;
    }
}

void DcoConnectMonitor::dumpConnectStatus(std::ostream& os, bool doPrtHeader)
{
    DcoConnectionStatus connStat = getConnectStatus();

    if (doPrtHeader)
    {
    os << boost::format("%-10d : %-25d : %-30d") % "*** Cpu"         % "*** DcoConnectState"         % "*** DcoConnectFailure"         << std::endl;
    }
    os << boost::format("%-10d : %-25d : %-30d") % ToString(mConnId) % ToString(connStat.mConnState) % ToString(connStat.mConnFailure) << std::endl;
}

void DcoConnectMonitor::setSimStatus(bool isSimEn,
                                     DcoConnectionStatus dcoConnStat)
{
    INFN_LOG(SeverityLevel::info) << " sim enable: " << (uint32_t)isSimEn
                   << " mConnState: " << (uint32_t)dcoConnStat.mConnState
                   << " mConnFailure: " << (uint32_t)dcoConnStat.mConnFailure;

    const std::lock_guard<std::mutex> lock(mStatusMtx);

    mIsSimEn    = isSimEn;
    mSimConnStatus = dcoConnStat;
}

DcoConnectionStatus DcoConnectMonitor::getSimConnectStatus()
{
    const std::lock_guard<std::mutex> lock(mStatusMtx);

    return mSimConnStatus;
}

bool DcoConnectMonitor::isSimDcoConnect()
{
    const std::lock_guard<std::mutex> lock(mStatusMtx);

    bool isConnect = (mIsSimEn == true) ? (mSimConnStatus.mConnState == DCS_CONNECTED) : true;

    return isConnect;
}

void DcoConnectMonitor::dumpSimStatus(std::ostream& os, bool doPrtHeader)
{
    DcoConnectionStatus connStat = getSimConnectStatus();

    if (doPrtHeader)
    {
    os << boost::format("%-10d : %-10d : %-25d : %-30d") % "*** Cpu"         % "*** SimEn" % "*** SimDcoConnectState"      % "*** SimDcoConnectFailure"      << std::endl;
    }
    os << boost::format("%-10d : %-10d : %-25d : %-30d") % ToString(mConnId) % mIsSimEn    % ToString(connStat.mConnState) % ToString(connStat.mConnFailure) << std::endl;
}

void DcoConnectMonitor::setSockAddr(std::string sockAddr)
{
    INFN_LOG(SeverityLevel::info) << "socket address: " << sockAddr;

    // todo: separate mutex for socket??

    const std::lock_guard<std::mutex> lock(mStatusMtx);

    mSocketAddr = sockAddr;
}

void DcoConnectMonitor::dumpGrpcConnectStatus(std::ostream& os, bool doPrtHeader)
{
    if (mspCrud == nullptr)
    {
        os << " Crud Pointer not initialized" << std::endl;
        return;
    }

    grpc_connectivity_state notifyState = mspCrud->GetNotifyChannelState();
    grpc_connectivity_state streamState = mspCrud->GetStreamChannelState();
    if (doPrtHeader)
    {
    os << boost::format("%-10d : %-35d : %-35d") % "*** Cpu"         % "*** GrpcNotifyState" % "*** GrpcStreamState" << std::endl;
    }
    os << boost::format("%-10d : %-35d : %-35d") % ToString(mConnId) % ToString(notifyState) % ToString(streamState) << std::endl;
}

void DcoConnectMonitor::Poll()
{
    INFN_LOG(SeverityLevel::info) << "Start monitoring of DcoConnection for socket address " << mSocketAddr;

    grpc_connectivity_state grpcState;
    bool isFirst = true;
    while (true)
    {
        if (mIsPollEn)
        {
            INFN_LOG(SeverityLevel::debug) << "Polling Socket Address " << mSocketAddr;

            {
                const std::lock_guard<std::mutex> lock(mStatusMtx);

                // Check Grpc Status
                if (mspCrud != nullptr)
                {
                    grpc_connectivity_state newGrpcState = mspCrud->GetNotifyChannelState();

                    if (isFirst || (grpcState != newGrpcState))
                    {
                        if ( isFirst ||
                            (grpcState    == GRPC_CHANNEL_READY) ||
                            (newGrpcState == GRPC_CHANNEL_READY))
                        {
                            if (isFirst)
                            {
                                INFN_LOG(SeverityLevel::info) << "grpc_connectivity_state for connId: " << ToString(mConnId) <<
                                                  " state: " << ToString(newGrpcState);
                            }
                            else
                            {
                                INFN_LOG(SeverityLevel::info) << "Change in grpc_connectivity_state for connId: " << ToString(mConnId) <<
                                                  " old state: " << ToString(grpcState) <<
                                                  " new state: " << ToString(newGrpcState);
                            }
                        }
                        else
                        {
                            INFN_LOG(SeverityLevel::debug) << "Change in grpc_connectivity_state for connId: " << ToString(mConnId) <<
                                               " old state: " << ToString(grpcState) <<
                                               " new state: " << ToString(newGrpcState);
                        }

                        grpcState = newGrpcState;

                        isFirst = false;
                    }
                }

                // Update Connection State
                //   todo: Increase this logic and add gecko and bcm monitoring
                if (grpcState == GRPC_CHANNEL_READY)
                {
                    mConnStatus.mConnState   = DCS_CONNECTED;
                    mConnStatus.mConnFailure = DCF_OK;
                }
                else
                {
                    mConnStatus.mConnState   = DCS_FAILURE;
                    mConnStatus.mConnFailure = DCF_GRPC_NO_CONNECT;
                }
            }
        }

        // todo: make this sleep configurable ???
        boost::posix_time::seconds workTime(1);
        boost::this_thread::sleep(workTime);

        // todo: handle thread exiting ???
    }

    INFN_LOG(SeverityLevel::info) << "Finished Polling. Done ***";
}

DcoZynqConnectMonitor::DcoZynqConnectMonitor(std::string sockAddr, bool isSim)
    : DcoConnectMonitor(sockAddr, isSim, DCC_ZYNQ, dp_env::DpEnvSingleton::Instance()->getDcoSimConnZynq())
{

}

void DcoZynqConnectMonitor::initialize()
{
    mspCrud = std::make_shared<::GnmiClient::SBGnmiClient>(mSocketAddr);

    mspCrud->SetTracer(
             std::static_pointer_cast<InfnTracer::InfnTracer>(
             InfnTracer::Global()
             )
    );
    DcoConnectMonitor::initialize();
}

DcoNxpConnectMonitor::DcoNxpConnectMonitor(std::string sockAddr, bool isSim)
    : DcoConnectMonitor(sockAddr, isSim, DCC_NXP, dp_env::DpEnvSingleton::Instance()->getDcoSimConnNxp())
{
}

void DcoNxpConnectMonitor::initialize()
{
    mspCrud = std::make_shared<::GnmiClient::NBGnmiClient>(mSocketAddr);

    mspCrud->SetTracer(
             std::static_pointer_cast<InfnTracer::InfnTracer>(
             InfnTracer::Global()
             )
    );
    DcoConnectMonitor::initialize();
}

} // namespace DataPlane

