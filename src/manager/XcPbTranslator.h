/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef XC_PB_TRANSLATOR_H_
#define XC_PB_TRANSLATOR_H_

#include <string>

#include <DcoXconAdapter.h>

#include "DpProtoDefs.h"
#include "DpCommonPbTranslator.h"


namespace DataPlane {

class XcPbTranslator : public DpCommonPbTranslator
{
public:
    XcPbTranslator() {}

    ~XcPbTranslator() {}

    bool isXconDirectionIngress(std::string aid) const;

    void configPbToAd(const chm6_dataplane::Chm6XCConfig& pbCfg, DpAdapter::XconCommon& adCfg) const;
    void configAdToPb(const DpAdapter::XconCommon& adCfg, chm6_dataplane::Chm6XCState& pbStat) const;

    static std::string idSwapSrcDest(std::string id);

private:

};


}; // namespace DataPlane

#endif /* XC_PB_TRANSLATOR_H_ */
