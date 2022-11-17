import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import odu_config_pb2

from grpc_client_adaptor import CrudService

def delete_odu_object(odu_id):
    odu_config = odu_config_pb2.Chm6OduConfig()
    odu_config.base_config.config_id.value = "1-4-L" + str(odu_id) + "-1"
    return odu_config


def delete(odu_id):
    try:
        crud = CrudService()
        '''
        Ideally only aid can be passed to server for deletion
        Passing the empty object for consistency and timestamp
        '''
        odu = delete_odu_object(odu_id)
        print(crud.delete(odu))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    odu_id = sys.argv[1]
    delete(odu_id)
