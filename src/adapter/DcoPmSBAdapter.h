/***********************************************************************
   Copyright (c) 2020 Infinera
***********************************************************************/
#ifndef DCO_PM_SB_ADAPTER_H
#define DCO_PM_SB_ADAPTER_H

#include <iostream>
#include <cmath>
#include "CrudService.h"
#include "dp_common.h"
#include "dcoyang/infinera_dco_card_pm/infinera_dco_card_pm.pb.h"
#include "dcoyang/infinera_dco_carrier_pm/infinera_dco_carrier_pm.pb.h"
#include "dcoyang/infinera_dco_client_gige_pm/infinera_dco_client_gige_pm.pb.h"
#include "dcoyang/infinera_dco_otu_pm/infinera_dco_otu_pm.pb.h"
#include "dcoyang/infinera_dco_odu_pm/infinera_dco_odu_pm.pb.h"
#include "dcoyang/infinera_dco_gcc_control_pm/infinera_dco_gcc_control_pm.pb.h"
#include "dcoyang/infinera_dco_client_gige/infinera_dco_client_gige.pb.h"
#include "dcoyang/infinera_dco_odu/infinera_dco_odu.pb.h"
#include "dcoyang/infinera_dco_otu/infinera_dco_otu.pb.h"
#include "dcoyang/infinera_dco_carrier/infinera_dco_carrier.pb.h"
#include "dcoyang/infinera_dco_card/infinera_dco_card.pb.h"
#include "dcoyang/infinera_dco_xcon/infinera_dco_xcon.pb.h"

using dcoCardPm = dcoyang::infinera_dco_card_pm::CardPm;
using dcoCarrierPm = dcoyang::infinera_dco_carrier_pm::CarrierPms;
using dcoGigePm = dcoyang::infinera_dco_client_gige_pm::ClientGigePms;
using dcoOtuPm = dcoyang::infinera_dco_otu_pm::OtuPms;
using dcoOduPm = dcoyang::infinera_dco_odu_pm::OduPms;
using dcoGccPm = dcoyang::infinera_dco_gcc_control_pm::GccControlPms;

using dcoGigeFault = dcoyang::infinera_dco_client_gige::ClientGigeFms;
using dcoOduFault = dcoyang::infinera_dco_odu::OduFms;
using dcoOtuFault = dcoyang::infinera_dco_otu::OtuFms;
using dcoCarrierFault = dcoyang::infinera_dco_carrier::CarrierFms;
using dcoCardFault = dcoyang::infinera_dco_card::CardFms;

using dcoOduState = dcoyang::infinera_dco_odu::OduState;
using dcoOtuState = dcoyang::infinera_dco_otu::OtuState;
using dcoCarrierState = dcoyang::infinera_dco_carrier::CarrierState;
using dcoCardState = dcoyang::infinera_dco_card::CardState;
using dcoCardIsk = dcoyang::infinera_dco_card::CardIsk;

using dcoOduOpStatus = dcoyang::infinera_dco_odu::OpStatus;
using dcoOtuOpStatus = dcoyang::infinera_dco_otu::OpStatus;
using dcoGigeOpStatus = dcoyang::infinera_dco_client_gige::OpStatus;
using dcoXconOpStatus = dcoyang::infinera_dco_xcon::OpStatus;

namespace DpAdapter {

static const int AMPS_TO_MA = 1000; //dco yang defined current in A, but SRD is in mA
static const int HZ_TO_MHZ = 1000000; //dco yang defined current in Hz, but SRD is in MHz

static const int BERSER_CORRECTION = 1e6;

//trigger cold boot condition(E <-> M):
//1. configured mode is M, previous mode is E and dco notify new mode is M
//2. configured mode is E, previous mode is M and dco notify new mode is E
inline bool IsReadyTriggerDcoColdBoot(ClientMode cfgMode, ClientMode prevMode, ClientMode newMode)
{
	return ( (cfgMode == CLIENTMODE_LXTP_M && prevMode == CLIENTMODE_LXTP_E && newMode == CLIENTMODE_LXTP_M) ||
		(cfgMode == CLIENTMODE_LXTP_E && prevMode == CLIENTMODE_LXTP_M && newMode == CLIENTMODE_LXTP_E) );
}

//card pm callback context
class CardPmResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~CardPmResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//carrier pm callback context
class CarrierPmResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~CarrierPmResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//gige pm callback context
class GigePmResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~GigePmResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//otu pm callback context
class OtuPmResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OtuPmResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//odu pm callback context
class OduPmResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OduPmResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//gcc pm callback context
class GccPmResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~GccPmResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};


//gige fault callback context
class GigeFaultResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~GigeFaultResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//odu fault callback context
class OduFaultResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OduFaultResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};


//otu fault callback context
class OtuFaultResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OtuFaultResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//carrier fault callback context
class CarrierFaultResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~CarrierFaultResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//card fault callback context
class CardFaultResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~CardFaultResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//odu state callback context
class OduStateResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OduStateResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//otu state callback context
class OtuStateResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OtuStateResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//carrier state callback context
class CarrierStateResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~CarrierStateResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//card state callback context
class CardStateResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~CardStateResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//card ISK callback context
class CardIskResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~CardIskResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//odu op status callback context
class OduOpStatusResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OduOpStatusResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//otu op status callback context
class OtuOpStatusResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~OtuOpStatusResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//gige op status callback context
class GigeOpStatusResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~GigeOpStatusResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

//xcon op status callback context
class XconOpStatusResponseContext: public GnmiClient::AsyncSubCallback
{
public:
	~XconOpStatusResponseContext() {}

	void HandleSubResponse(google::protobuf::Message& obj, grpc::Status status, std::string cbId);

};

}
#endif // DCO_PM_SB_ADAPTER_H
