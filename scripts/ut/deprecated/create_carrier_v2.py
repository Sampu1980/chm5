import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import carrier_config_pb2

from grpc_client_adaptor import CrudService
from infinera.hal.common.v1 import sys_constants_pb2

def create_carrier_object(car_id):
    car_config = carrier_config_pb2.Chm6CarrierConfig()
    car_config.base_config.config_id.value = "1-4-L" + str(car_id) + "-1"
    car_config.hal.mode.application_code.value = "appCode"
    car_config.hal.mode.baud_rate.value = 3
    car_config.hal.mode.capacity.value = 2
    car_config.hal.mode.client = sys_constants_pb2.CLIENT_MODE_LXTP_E
    car_config.hal.enable.value = True 
    
    # BUG - GX-3836
    #car_config.hal.frequency.value = 2020.10
    #car_config.hal.frequency_offset.value = 2020.10
    #car_config.hal.target_power.value = 20.5

    return car_config


def create(car_id):
    try:
        crud = CrudService()
        car = create_carrier_object(car_id)
        print(crud.create(car))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)

if __name__ == '__main__':
    car_id = sys.argv[1]
    create(car_id)
