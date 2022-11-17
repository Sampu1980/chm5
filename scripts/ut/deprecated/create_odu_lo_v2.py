import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import odu_config_pb2
#from infinera.hal.dataplane.v1 import odu_config_pb2
from infinera.hal.common.v1 import sys_constants_pb2

from grpc_client_adaptor import CrudService

def create_odu_object(odu_id):
    odu_config = odu_config_pb2.Chm6OduConfig()
    odu_config.base_config.config_id.value = "1-4-L1-1-ODU4i-" + str(odu_id)
    odu_config.hal.config_svctype = sys_constants_pb2.BandWidth.BAND_WIDTH_100GB_ELAN
    odu_config.hal.prbs_gen_mon.fac_test_sig_gen.value = True
    odu_config.hal.prbs_gen_mon.fac_test_sig_mon.value = False
    odu_config.hal.service_mode = sys_constants_pb2.ServiceMode.SERVICE_MODE_SWITCHING
    odu_config.hal.odu_ms_config.fac_forceaction = sys_constants_pb2.MAINTENANCE_ACTION_TURNOFFLASER
    odu_config.hal.odu_ms_config.fac_autoaction = sys_constants_pb2.MAINTENANCE_ACTION_SENDAIS
    odu_config.hal.odu_ms_config.term_forceaction = sys_constants_pb2.MAINTENANCE_ACTION_SENDIDLE
    odu_config.hal.odu_ms_config.term_autoaction = sys_constants_pb2.MAINTENANCE_ACTION_DISABLETRANSMITTER
    #odu_config.hal.tx_tti_config.transmit_tti(0).value = 1
    #odu_config.hal.tx_tti_config.transmit_tti[0].value = 1
    #odu_config.hal.ts(0).value = 2
    #odu_config.hal.ts[0].value = 2
    #odu_config.hal.ts_granularity = odu_config_pb2.TSGranularity.TS_GRANULARITY_25G
    return odu_config

def create(odu_id):
    try:
        crud = CrudService()
        odu = create_odu_object(odu_id)
        print(crud.create(odu))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    odu_id = sys.argv[1]
    create(odu_id)
