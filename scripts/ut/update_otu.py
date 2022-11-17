import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import otu_config_pb2
from infinera.hal.common.v2 import sys_constants_pb2
from infinera.wrappers.v1 import infn_datatypes_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def update_otu_object(otu_id):
    otu_config = otu_config_pb2.OtuConfig()
    otu_config.base_config.aid.value = "1-4-L" + str(otu_id) + "-1"
    otu_config.config.config_svc_type = sys_constants_pb2.BandWidth.BAND_WIDTH_400GB_ELAN
    otu_config.config.service_mode = sys_constants_pb2.ServiceMode.SERVICE_MODE_SWITCHING
    otu_config.config.otu_ms_config.fac_ms.forced_ms = sys_constants_pb2.MAINTENANCE_ACTION_SENDOCI
    otu_config.config.otu_ms_config.fac_ms.auto_ms = sys_constants_pb2.MAINTENANCE_ACTION_SENDLCK
    otu_config.config.otu_ms_config.term_ms.forced_ms = sys_constants_pb2.MAINTENANCE_ACTION_PRBS_X31
    otu_config.config.otu_ms_config.term_ms.auto_ms = sys_constants_pb2.MAINTENANCE_ACTION_PRBS_X31_INV
    otu_config.config.loopback = sys_constants_pb2.LoopbackType.LOOPBACK_TYPE_FACILITY
    otu_config.config.loopback_behavior = sys_constants_pb2.TERM_LOOPBACK_BEHAVIOR_TURNOFFLASER
    #otu_config.config.otu_prbs_config.fac_prbs_gen.value = False
    #otu_config.config.otu_prbs_config.fac_prbs_mon.value = True
    #otu_config.config.otu_prbs_config.term_prbs_gen.value = False
    #otu_config.config.otu_prbs_config.term_prbs_mon.value = True
    otu_config.config.otu_prbs_config.fac_prbs_gen_bool = infn_datatypes_pb2.BOOL_FALSE
    otu_config.config.otu_prbs_config.fac_prbs_mon_bool = infn_datatypes_pb2.BOOL_TRUE
    otu_config.config.otu_prbs_config.term_prbs_gen_bool = infn_datatypes_pb2.BOOL_FALSE
    otu_config.config.otu_prbs_config.term_prbs_mon_bool = infn_datatypes_pb2.BOOL_TRUE
    otu_config.config.carrier_channel = sys_constants_pb2.CARRIER_CHANNEL_BOTH
    otu_config.config.alarm_threshold.signal_degrade_interval.value = 8
    otu_config.config.alarm_threshold.signal_degrade_threshold.value = 40
    return otu_config

def update(otu_id):
    try:
        crud = CrudService()
        otu = update_otu_object(otu_id)
        logger.debug(crud.create(otu))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    otu_id = sys.argv[1]
    update(otu_id)
