import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import odu_config_pb2
from infinera.hal.common.v2 import sys_constants_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def create_odu_object(odu_id):
    odu_config = odu_config_pb2.OduConfig()
    odu_config.base_config.aid.value = "1-4-L" + str(odu_id) + "-1"
    odu_config.config.odu_index.value = int(odu_id)
    odu_config.config.config_svc_type = sys_constants_pb2.BandWidth.BAND_WIDTH_100GB_ELAN
    odu_config.config.odu_prbs_config.fac_prbs_gen.value = True
    odu_config.config.odu_prbs_config.fac_prbs_mon.value = False
    odu_config.config.odu_prbs_config.term_prbs_gen.value = True
    odu_config.config.odu_prbs_config.term_prbs_mon.value = False
    odu_config.config.service_mode = sys_constants_pb2.ServiceMode.SERVICE_MODE_SWITCHING
    odu_config.config.odu_ms_config.fac_forceaction = sys_constants_pb2.MAINTENANCE_ACTION_SENDAIS
    odu_config.config.odu_ms_config.fac_autoaction = sys_constants_pb2.MAINTENANCE_ACTION_SENDOCI
    odu_config.config.odu_ms_config.term_forceaction = sys_constants_pb2.MAINTENANCE_ACTION_SENDLCK
    odu_config.config.odu_ms_config.term_autoaction = sys_constants_pb2.MAINTENANCE_ACTION_PRBS_X31
    #odu_config.config.tx_tti_config.transmit_tti(0).value = 1
    #odu_config.config.tx_tti_config.transmit_tti[0].value = 1
    #odu_config.config.ts(0).value = 2
    #odu_config.config.ts[0].value = 2
    odu_config.config.ts_granularity = odu_config_pb2.TSGranularity.TS_GRANULARITY_25G
    odu_config.config.alarm_threshold.signal_degrade_interval.value = 7
    odu_config.config.alarm_threshold.signal_degrade_threshold.value = 30
    return odu_config

def create(odu_id):
    try:
        crud = CrudService()
        odu = create_odu_object(odu_id)
        logger.debug(crud.create(odu))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    odu_id = sys.argv[1]
    create(odu_id)
