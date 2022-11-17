import grpc
import threading
import sys

from infinera.chm6.dataplane.v2 import otu_config_pb2
from infinera.hal.common.v1 import sys_constants_pb2

from grpc_client_adaptor import CrudService

def create_otu_object(otu_id):
    otu_config = otu_config_pb2.Chm6OtuConfig()
    otu_config.base_config.config_id.value = "1-4-L" + str(otu_id) + "-1"
    otu_config.hal.config_svc_type = sys_constants_pb2.BandWidth.BAND_WIDTH_100GB_ELAN
    otu_config.hal.service_mode = sys_constants_pb2.ServiceMode.SERVICE_MODE_SWITCHING
    otu_config.hal.otu_ms_config.fac_ms.forced_ms = sys_constants_pb2.MAINTENANCE_ACTION_SENDAIS
    otu_config.hal.otu_ms_config.fac_ms.auto_ms = sys_constants_pb2.MAINTENANCE_ACTION_SENDOCI
    otu_config.hal.otu_ms_config.fac_prbs.value = True
    otu_config.hal.loopback = sys_constants_pb2.LoopbackType.LOOPBACK_TYPE_TERMINAL
    otu_config.hal.loopback_behavior = sys_constants_pb2.TERM_LOOPBACK_BEHAVIOR_BRIDGESIGNAL
    otu_config.hal.prbs_gen_mon.fac_test_sig_gen.value = True
    otu_config.hal.prbs_gen_mon.fac_test_sig_mon.value = False
    return otu_config

def create(otu_id):
    try:
        crud = CrudService()
        otu = create_otu_object(otu_id)
        print(crud.create(otu))
    except grpc.RpcError as err:
        print("Set failed")
        print(err.details)


if __name__ == '__main__':
    otu_id = sys.argv[1]
    create(otu_id)
