import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import scg_config_pb2
from infinera.wrappers.v1 import infn_datatypes_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def create_scg_object(scg_id):
    scg_config = scg_config_pb2.ScgConfig()
    scg_config.base_config.aid.value = "1-4-L" + str(scg_id)
    scg_config.config.supported_facilities = infn_datatypes_pb2.BOOL_FALSE 
    return scg_config

def create(scg_id):
    try:
        crud = CrudService()
        scg = create_scg_object(scg_id)
        logger.debug(crud.create(scg))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    scg_id = sys.argv[1]
    create(scg_id)
