import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import gige_client_config_pb2

from grpc_client_adaptor import CrudService

def delete_gige_client_object(gigeclient_id):
    gigeclient_config = gige_client_config_pb2.Chm6GigeClientConfig()
    gigeclient_config.base_config.config_id.value = "1-4-T" + str(gigeclient_id)
    return gigeclient_config


def delete(gigeclient_id):
    try:
        crud = CrudService()
        '''
        Ideally only aid can be passed to server for deletion
        Passing the empty object for consistency and timestamp
        '''
        gigeclient = delete_gige_client_object(gigeclient_id)
        print(crud.delete(gigeclient))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    gigeclient_id = sys.argv[1]
    delete(gigeclient_id)
