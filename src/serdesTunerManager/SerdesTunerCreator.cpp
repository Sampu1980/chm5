/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/

#include "SerdesTunerCreator.h"
#include "SerdesTuner.h"
#include "InfnLogger.h"


namespace tuning
{

void SerdesTunerCreator::CreateSerdesTuner(SerdesTuner*& pNewSerdesTuner)
{
    bool isSimMode;

    if (SerdesTunerSingleton::IsInstanceInstalled())
    {
        INFN_LOG(SeverityLevel::info) << "SerdesTuner already created" << endl;
        return;
    }


    // HW Create real SerdesTuning

    INFN_LOG(SeverityLevel::info) << "HW Platform - Creating SerdesTuner" << endl;
    pNewSerdesTuner = new SerdesTuner();

    SerdesTunerSingleton::InstallInstance((SerdesTuner*)pNewSerdesTuner);
}



} // namespace tuning

