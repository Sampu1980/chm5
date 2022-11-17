#include <string>
#include <chrono>
#include <thread>
#include <math.h>

#include "infinera/chm6/examples/v1/odu_config.pb.h"
#include "CrudService.h"

using namespace std;
using namespace chrono;
using namespace google::protobuf;


const uint64_t TIME_INTERVAL = pow(10, 9);

std::unique_ptr<infinera::chm6::examples::v1::OduConfig> createOduObj(uint32_t oduId)
{
	std::unique_ptr<infinera::chm6::examples::v1::OduConfig> oduObj(new infinera::chm6::examples::v1::OduConfig());
	const string oduIdStr = "odu-" + to_string(oduId);	
	oduObj->mutable_config()->mutable_odu_id()->set_value(oduIdStr);
	oduObj->mutable_base_config()->mutable_config_id()->set_value(oduIdStr);
	return oduObj;
}

int main()
{
	uint32_t oduId = 1;
    auto objMsg = createOduObj(oduId);
	::GnmiClient::CrudService crud;
	crud.Set(objMsg);
	this_thread::sleep_for(seconds(2));
	crud.Get(objMsg);
	this_thread::sleep_for(seconds(2));
	thread getThread = thread(&::GnmiClient::CrudService::Get<std::unique_ptr<infinera::chm6::examples::v1::OduConfig>>, &crud, ref(objMsg));
	getThread.join();	
	//this_thread::sleep_for(seconds(2));
	//crud.Subscribe(key, objMsg);	
	//crud.Stream(key, TIME_INTERVAL, objMsg);
    return 0;
}
