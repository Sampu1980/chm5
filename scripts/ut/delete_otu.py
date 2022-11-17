import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import otu_config_pb2
from infinera.hal.common.v2 import sys_constants_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def delete_otu_object(otu_id):
    otu_config = otu_config_pb2.OtuConfig()
    otu_config.base_config.aid.value = "1-4-L" + str(otu_id) + "-1"
    return otu_config

def delete(otu_id):
    try:
        crud = CrudService()
        otu = delete_otu_object(otu_id)
        logger.debug(crud.delete(otu))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    otu_id = sys.argv[1]
    delete(otu_id)
