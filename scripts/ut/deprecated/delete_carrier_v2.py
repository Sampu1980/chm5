import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import carrier_config_pb2

from grpc_client_adaptor import CrudService

def delete_carrier_object(car_id):
    car_config = carrier_config_pb2.Chm6CarrierConfig()
    car_config.base_config.config_id.value = "1-4-L" + str(car_id) + "-1"

    return car_config


def delete(car_id):
    try:
        crud = CrudService()
        car = delete_carrier_object(car_id)
        print(crud.delete())
    except (grpc.RpcError) as err:
        print("Set failed")
        print(err.details)

if __name__ == '__main__':
    car_id = sys.argv[1]
    delete(car_id)
