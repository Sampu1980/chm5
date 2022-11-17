import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import gige_client_config_pb2
from infinera.hal.dataplane.v2 import client_base_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def delete_gige_client_object(gigeclient_id):
    gigeclient_config = gige_client_config_pb2.GigeClientConfig()
    gigeclient_config.base_config.aid.value = "1-4-T" + str(gigeclient_id);
    return gigeclient_config

def delete(gigeclient_id):
    try:
        crud = CrudService()
        gigeclient = delete_gige_client_object(gigeclient_id)
        logger.debug(crud.delete(gigeclient))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    gigeclient_id = sys.argv[1]
    delete(gigeclient_id)
