import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import xcon_config_pb2

from grpc_client_adaptor import CrudService

def update_xcon_object(xcon_id):
    xcon_config = xcon_config_pb2.Chm6XCConfig()
    xcon_config.base_config.config_id.value = "xcon-" + str(xcon_id);
    xcon_config.hal.source.value = "NewSrc";
    xcon_config.hal.destination.value = "NewDst";
    #xcon_config.hal.xc_direction = xcon_config_pb2.DIRECTION_ONE_WAY
    return xcon_config

def update(xcon_id):
    try:
        crud = CrudService()
        xcon = update_xcon_object(xcon_id)
        print(crud.create(xcon))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    xcon_id = sys.argv[1]
    update(xcon_id)
