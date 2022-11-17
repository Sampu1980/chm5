import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import xcon_config_pb2

from grpc_client_adaptor import CrudService

def delete_xcon_object(xcon_id):
    xcon_config = xcon_config_pb2.Chm6XCConfig()
    xcon_config.base_config.config_id.value = "xcon-" + str(xcon_id)
    return xcon_config


def delete(xcon_id):
    try:
        crud = CrudService()
        '''
        Ideally only aid can be passed to server for deletion
        Passing the empty object for consistency and timestamp
        '''
        xcon = delete_xcon_object(xcon_id)
        print(crud.delete(xcon))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    xcon_id = sys.argv[1]
    delete(xcon_id)
