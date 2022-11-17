/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef GEAR_BOX_CREATOR_H
#define GEAR_BOX_CREATOR_H

#include "GearBoxAdIf.h"
#include "GearBoxAdapter.h"
#include "GearBoxAdapterSim.h"

#include <SingletonService.h>


namespace gearbox {

class GearBoxCreator
{
public:
    GearBoxCreator()
    {
    }

    ~GearBoxCreator() {}

    void CreateGearBox(GearBoxAdIf*& pNewGearBox);

protected:

};

typedef SingletonService<GearBoxCreator> GearBoxCreatorSingleton;

} // namespace gearbox

#endif  // GEAR_BOX_CREATOR_H
