/**************************************************************************
   Copyright (c) 2021 Infinera
**************************************************************************/
#pragma once
#ifndef FAULT_CACHE_MAP_H
#define FAULT_CACHE_MAP_H

#include <map>
#include <string>

namespace DataPlane
{

// Fault Cache Map
typedef std::map< std::string,
        std::map< std::string,
                  bool > > FaultCacheMap;

}; // namespace DataPlane

#endif /*FAULT_CACHE_MAP_H */
