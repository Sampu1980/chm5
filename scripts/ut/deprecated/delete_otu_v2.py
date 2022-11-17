import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import otu_config_pb2

from grpc_client_adaptor import CrudService

def delete_otu_object(otu_id):
    otu_config = otu_config_pb2.Chm6OtuConfig()
    otu_config.base_config.config_id.value = "1-4-L" + str(otu_id) + "-1"
    return otu_config


def delete(otu_id):
    try:
        crud = CrudService()
        '''
        Ideally only aid can be passed to server for deletion
        Passing the empty object for consistency and timestamp
        '''
        otu = delete_otu_object(otu_id)
        print(crud.delete(otu))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    otu_id = sys.argv[1]
    delete(otu_id)
