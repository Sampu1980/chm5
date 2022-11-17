/**************************************************************************
   Copyright (c) 2021 Infinera
**************************************************************************/
#ifndef GCC_PB_TRANSLATOR_H_
#define GCC_PB_TRANSLATOR_H_

#include <string>
#include <DcoGccControlAdapter.h>
#include <DpPm.h>
#include "DpProtoDefs.h"
#include "DpCommonPbTranslator.h"

namespace DataPlane {

class GccPbTranslator : public DpCommonPbTranslator
{
public:
    GccPbTranslator() {}

    ~GccPbTranslator() {}

    enum GCC_PM_PARAMS
    {
        TX_PACKETS       = 0,
        RX_PACKETS          ,
        TX_BYTES            ,
        RX_BYTES            ,
        TX_PACKETS_DROPPED  ,
        RX_PACKETS_DROPPED  ,
        NUM_GCC_PM_PARAMS
    };

    const boost::unordered_map<GCC_PM_PARAMS,const char*> gccPmParamsToString = boost::assign::map_list_of
        (TX_PACKETS         , "packets"              )
        (RX_PACKETS         , "packets"              )
        (TX_BYTES           , "bytes"                )
        (RX_BYTES           , "bytes"                )
        (TX_PACKETS_DROPPED , "packets-dropped"      )
        (RX_PACKETS_DROPPED , "packets-dropped"      );

	//enable when proto file been added
    void pmAdToPb(const DataPlane::DpMsGccPMContainer& adPm, bool validity, chm6_dataplane::Chm6GccPm& pbPm) const;
    void pmAdDefault(DataPlane::DpMsGccPMContainer& adPm);

    void addGccPmPair(GCC_PM_PARAMS pmParam, PmDirection pmDirection, PmLocation pmLocation, uint64_t value, bool validity, chm6_dataplane::Chm6GccPm& pbPm) const;
};


}; // namespace DataPlane

#endif /* CARRIER_PB_TRANSLATOR_H_ */
