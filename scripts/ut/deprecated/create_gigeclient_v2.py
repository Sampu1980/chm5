import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import gige_client_config_pb2
from infinera.hal.dataplane.v1 import client_base_pb2

from grpc_client_adaptor import CrudService

def create_gige_client_object(gigeclient_id):
    gigeclient_config = gige_client_config_pb2.Chm6GigeClientConfig()
    gigeclient_config.base_config.config_id.value = "1-4-T" + str(gigeclient_id);
    gigeclient_config.hal.rate = client_base_pb2.PortRate.PORT_RATE_ETH100G
    gigeclient_config.hal.loopback = client_base_pb2.LoopBackType.LOOP_BACK_TYPE_FACILITY
    gigeclient_config.hal.fec_enable.value = True
    gigeclient_config.hal.mtu.value = 5;
    gigeclient_config.hal.mode.rx_snoop_enable.value = True
    gigeclient_config.hal.mode.tx_snoop_enable.value = False
    gigeclient_config.hal.mode.rx_drop_enable.value = True
    gigeclient_config.hal.mode.tx_drop_enable.value = False
    gigeclient_config.hal.auto_rx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_LF
    gigeclient_config.hal.auto_tx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_IDLE
    gigeclient_config.hal.force_rx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_LF
    gigeclient_config.hal.force_tx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_IDLE
    return gigeclient_config


def create(gigeclient_id):
    try:
        crud = CrudService()
        gigeclient = create_gige_client_object(gigeclient_id)
        print(crud.create(gigeclient))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    gigeclient_id = sys.argv[1]
    create(gigeclient_id)
