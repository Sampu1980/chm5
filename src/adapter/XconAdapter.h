/************************************************************************
   Copyright (c) 2020 Infinera
************************************************************************/
#ifndef XCON_ADAPTER_H
#define XCON_ADAPTER_H

#include "dp_common.h"
#include "AdapterCommon.h"
#include <string>


using namespace std;
using namespace DataPlane;

namespace DpAdapter {

struct XconCommon
{
    XconCommon()
    : name(""), source(""), destination(""), clientId(1), loOduId(1), hoOtuId(1), sourceClient(""), destinationClient(""), transactionId(0)
    {
        direction = XCONDIRECTION_BI_DIR;
        payload = XCONPAYLOADTYPE_100GBE;
        payloadTreatment = XCONPAYLOADTREATMENT_TRANSPORT;
    }
    string name;        // AID
    string source;
    string destination;
    uint32_t clientId;
    uint32_t loOduId;
    uint32_t hoOtuId;   // Point to which OTUCni it belong to
    XconDirection direction;
    string sourceClient;        // XPress XCon
    string destinationClient;   // XPress XCon
    XconPayLoadType payload;    // Xpress Xcon
    XconPayLoadTreatment payloadTreatment;    // Xpress Xcon
    uint64_t transactionId;
};

typedef struct
{
    XconCommon xcon;
} XconStatus;


// XconAdapter Base class
class XconAdapter
{
public:
    XconAdapter() {}
    virtual ~XconAdapter() {}

    virtual bool createXcon(string aid, XconCommon *cfg) = 0;
    virtual bool createXpressXcon(string aid, XconCommon *cfg, uint32_t *srcLoOduId, uint32_t *dstLoOduId) = 0;
    virtual bool deleteXcon(string aid) = 0;
    virtual bool deleteXpressXcon(string aid, string *srcClientId, string *dstClientId, XconPayLoadType *payload, uint32_t *srcLoOduId, uint32_t *dstLoOduId) = 0;
    virtual bool initializeXcon() = 0;

    virtual bool getXconConfig(string aid, XconCommon *cfg) = 0;
    virtual bool getXconStatus(string aid, XconStatus *stat) = 0;

    virtual bool xconConfigInfo(ostream& out, string aid) = 0;
    virtual bool xconStatusInfo(ostream& out, string aid) = 0;
};

}
#endif // XCON_ADAPTER_H
