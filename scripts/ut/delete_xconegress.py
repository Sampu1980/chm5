import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import xcon_config_pb2
#from infinera.hal.dataplane.v2 import xcon_config_base_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def delete_xcon_object(xcon_id):
    xcon_config = xcon_config_pb2.XCConfig()
    xcon_config.base_config.aid.value = "1-4-L1-1-ODU4i-" + str(xcon_id) + "," + "1-4-T3"
    return xcon_config

def delete(xcon_id):
    try:
        crud = CrudService()
        xcon = delete_xcon_object(xcon_id)
        logger.debug(crud.delete(xcon))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    xcon_id = sys.argv[1]
    delete(xcon_id)
