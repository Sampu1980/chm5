import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import carrier_config_pb2
from infinera.hal.common.v2 import sys_constants_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def delete_carrier_object(car_id):
    car_config = carrier_config_pb2.CarrierConfig()
    car_config.base_config.aid.value = "1-4-L" + str(car_id) + "-1"
    return car_config

def delete(car_id):
    try:
        crud = CrudService()
        car = delete_carrier_object(car_id)
        logger.debug(crud.delete(car))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    car_id = sys.argv[1]
    delete(car_id)
