/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "GearBoxCreator.h"
#include "DpEnv.h"
#include "InfnLogger.h"


namespace gearbox {

void GearBoxCreator::CreateGearBox(GearBoxAdIf*& pNewGearBox)
{
    bool isSimMode;

    if (GearBoxAdapterSingleton::IsInstanceInstalled())
    {
        INFN_LOG(SeverityLevel::info) << "GearBox already created" << endl;
        return;
    }

#ifdef ARCH_x86
    // Create Sim Version of GB Adapter
        INFN_LOG(SeverityLevel::info) << "*** x86 Env - Creating Sim Version of Gearbox Adapter" << endl;
        pNewGearBox = new GearBoxAdapterSim();
#else
    if (! dp_env::DpEnvSingleton::Instance()->isSimEnv())
    {
        // HW Create real GearBoxAdapter

        INFN_LOG(SeverityLevel::info) << "HW Platform - Creating GearBoxAdapter" << endl;
        pNewGearBox = new GearBoxAdapter();
        //mpGearBoxAdp->init();
    }
    else
    {
        // Create Sim Version of GB Adapter
        INFN_LOG(SeverityLevel::info) << "*** Sim Mode - Creating Sim Version of Gearbox Adapter" << endl;
        pNewGearBox = new GearBoxAdapterSim();
    }
#endif
    GearBoxAdapterSingleton::InstallInstance((GearBoxAdIf*)pNewGearBox);

}


} // namespace gearbox

