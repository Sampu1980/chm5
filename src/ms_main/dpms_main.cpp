
#include <iostream>
#include <thread>
#include "DataPlaneMgr.h"
#include "GearBoxAdapter.h"
#include "GearBoxCreator.h"
#include "DataPlaneMgrLog.h"
#include "DpEnv.h"
#include "InfnLogger.h"
#include "BackTrace.h"
#include "DcoConnectHandler.h"
#include <SimCrudService.h>
#include "DcoSecProcActor.h"

#include <InfnTracer/InfnTracer.h>

using namespace std;
using namespace DataPlane;
using namespace gearbox;
using namespace InfnTracer;

extern int DbgCmds();

void initLog()
{
    InfnLogger::initLogging("chm6_dataplane_ms");
    InfnLogger::setLoggingSeverity(SeverityLevel::info);
    INFN_LOG(SeverityLevel::info) << "";
}

void initDcoConnectHandler()
{
    DcoConnectHandlerSingleton::InstallInstance(new DcoConnectHandler());
    DcoConnectHandlerSingleton::Instance()->initialize();
}

void initSignalHandlers()
{
    std::signal(SIGABRT,sig_hdlr);
    std::signal(SIGSEGV,sig_hdlr);
    std::signal(SIGHUP ,sig_hdlr);
    std::signal(SIGINT ,sig_hdlr);
    std::signal(SIGTERM,sig_hdlr);
    std::signal(SIGQUIT,sig_hdlr);
    std::signal(SIGILL ,sig_hdlr);
    std::signal(SIGTRAP,sig_hdlr);
    std::signal(SIGFPE ,sig_hdlr);
    std::signal(SIGBUS ,sig_hdlr);
    std::signal(SIGSYS ,sig_hdlr);
    std::signal(SIGXCPU,sig_hdlr);
    std::signal(SIGXFSZ,sig_hdlr);
    std::signal(SIGPIPE,sig_hdlr);
    std::signal(SIGTTOU,sig_hdlr);
    std::signal(SIGTTIN,sig_hdlr);
}

int main(int argc, char **argv, char **envp) {

    cout << "*** Starting dpms_main " << endl;

    initSignalHandlers();

    initLog();
    static const std::string component = "CHM6_DPMSMs";
    std::shared_ptr<InfnTracer::InfnTracer> tracer_ = \
        InfnTracer::MakeInfnTracerFromEnvironment(component, envp);
    InfnTracer::InitGlobal(tracer_);

    INFN_LOG(SeverityLevel::info) << "CHM6_DPMSMs Done Calling initTracer";

    INFN_LOG(SeverityLevel::info) << "*************************************************************************";
    INFN_LOG(SeverityLevel::info) << "****** Starting/Initializing Dataplane MS *******************************";

    ::dp_env::DpEnv* pEnv = new ::dp_env::DpEnv();
    ::dp_env::DpEnvSingleton::InstallInstance(pEnv);

    DcoSecProcEventListenerService::InstallInstance(new DcoSecProcEventListener(NXP_EVENT_QUEUE_SIZE));
    DcoSecProcEventListenerService::Instance()->Start();

    initDcoConnectHandler();

    std::shared_ptr<DpSim::SimSbGnmiClient> spSbCrud = DpSim::SimSbGnmiClient::getInstance();

    GearBoxCreator* pGbCreatr = new GearBoxCreator();
    GearBoxCreatorSingleton::InstallInstance(pGbCreatr);

    DataPlaneMgr * pDpMgr = new DataPlaneMgr();
    DataPlaneMgrSingleton::InstallInstance(pDpMgr);
    DataPlaneMgrSingleton::Instance()->initRedisCallbacks();

    INFN_LOG(SeverityLevel::info) << "----------- start debug cli -----------" << endl;

    int ret = DbgCmds();

    return 0;
}
