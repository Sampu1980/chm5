import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import odu_config_pb2
from infinera.hal.common.v2 import sys_constants_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def delete_odu_object(odu_id):
    odu_config = odu_config_pb2.OduConfig()
    odu_config.base_config.aid.value = "1-4-L" + str(odu_id) + "-1"
    odu_config.config.odu_index.value = int(odu_id)
    return odu_config

def delete(odu_id):
    try:
        crud = CrudService()
        odu = delete_odu_object(odu_id)
        logger.debug(crud.delete(odu))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    odu_id = sys.argv[1]
    delete(odu_id)
