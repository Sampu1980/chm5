import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import gige_client_config_pb2
from infinera.hal.dataplane.v2 import client_base_pb2
from infinera.wrappers.v1 import infn_datatypes_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def create_gige_client_object(gigeclient_id):
    gigeclient_config = gige_client_config_pb2.GigeClientConfig()
    gigeclient_config.base_config.aid.value = "1-4-T" + str(gigeclient_id);
    gigeclient_config.config.rate = client_base_pb2.PortRate.PORT_RATE_ETH100G
    gigeclient_config.config.loopback = client_base_pb2.LoopBackType.LOOP_BACK_TYPE_FACILITY
    #gigeclient_config.config.fec_enable.value = True
    gigeclient_config.config.fec_enable_bool = infn_datatypes_pb2.BOOL_TRUE
    gigeclient_config.config.mtu.value = 5;
    #gigeclient_config.config.mode.rx_snoop_enable.value = True
    #gigeclient_config.config.mode.tx_snoop_enable.value = False
    #gigeclient_config.config.mode.rx_drop_enable.value = True
    #gigeclient_config.config.mode.tx_drop_enable.value = False
    gigeclient_config.config.mode.rx_snoop_enable_bool = infn_datatypes_pb2.BOOL_TRUE
    gigeclient_config.config.mode.tx_snoop_enable_bool = infn_datatypes_pb2.BOOL_FALSE
    gigeclient_config.config.mode.rx_drop_enable_bool = infn_datatypes_pb2.BOOL_TRUE
    gigeclient_config.config.mode.tx_drop_enable_bool = infn_datatypes_pb2.BOOL_FALSE
    gigeclient_config.config.auto_rx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_IDLE
    gigeclient_config.config.auto_tx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_LF
    gigeclient_config.config.force_rx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_RF
    gigeclient_config.config.force_tx = client_base_pb2.MaintenanceSignal.MAINTENANCE_SIGNAL_IDLE
    return gigeclient_config

def create(gigeclient_id):
    try:
        crud = CrudService()
        gigeclient = create_gige_client_object(gigeclient_id)
        logger.debug(crud.create(gigeclient))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    gigeclient_id = sys.argv[1]
    create(gigeclient_id)
