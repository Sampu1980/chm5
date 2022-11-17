//
//
// Copyright(c) Infinera 2021-2022
//
#ifndef DcoSecProcEvent_h
#define DcoSecProcEvent_h

//#include "SingletonService.h"
#include <vector>
#include <map>
#include <google/protobuf/message.h>

using namespace std;

//Events types for sending data to NXP
enum DcoSecProcEventType
{
    DCO_SECPROC_INIT = 0,
    DCO_SECPROC_OFEC_STATUS_CREATE,
    DCO_SECPROC_OFEC_STATUS_UPDATE,
    DCO_SECPROC_OFEC_STATUS_DELETE,
    DCO_SECPROC_RX_STATUS,
    DCO_SECPROC_MAX
};

typedef DcoSecProcEventType DcoSecProcEventType_t;

class DcoSecProcEvent 
{
  public:
    DcoSecProcEvent()
    {
      mEventType = DCO_SECPROC_INIT;
      mEventValue = false;
    }
    ~DcoSecProcEvent() {}
    DcoSecProcEvent(DcoSecProcEventType_t eventType, bool eventvalue, std::string aid)//google::protobuf::Message *protobuf)
    {
      mEventType = eventType;
      mEventValue = eventvalue;
      mAid = aid;
      //mProtobuf = protobuf;
    }
    DcoSecProcEventType_t GetEventType()
    {
      return mEventType;
    }
    void SetEventType(DcoSecProcEventType_t event)
    {
      mEventType = event;
    }

    bool GetEventValue()
    {
      return mEventValue;
    }
    void SetEventValue(bool eventvalue)
    {
      mEventValue = eventvalue;
    }

    std::string GetAid()
    {
      return mAid;
    }
    void SetAid(std::string aid)
    {
      mAid = aid;
    }
    /*
    google::protobuf::Message* GetProto()
    {
      return mProtobuf;
    }

    void SetProto(google::protobuf::Message *protobuf)
    {
      mProtobuf = protobuf;
    }
    */

    private:
      DcoSecProcEventType_t mEventType;
      bool mEventValue;
      std::string mAid;
      //google::protobuf::Message *mProtobuf;
};

#endif /* DcoSecProcEvent_h */



