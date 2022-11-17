import grpc
import threading
import sys

from infinera.hal.dataplane.v2 import carrier_config_pb2
from infinera.hal.common.v2 import sys_constants_pb2
from infinera.wrappers.v1 import infn_datatypes_pb2

from chm6.grpc_adapter.crud_service import CrudService
from logger_util import setup_log

logger = setup_log()

def create_carrier_object(car_id):
    car_config = carrier_config_pb2.CarrierConfig()
    car_config.base_config.aid.value = "1-4-L" + str(car_id) + "-1"
    car_config.config.mode.application_code.value = "appCode"
    car_config.config.mode.baud_rate.value = 3
    car_config.config.mode.capacity.value = 2
    car_config.config.mode.client = sys_constants_pb2.CLIENT_MODE_LXTP_E
    car_config.config.enable_bool = infn_datatypes_pb2.BOOL_TRUE 
    car_config.config.frequency.value = 2020.10
    car_config.config.frequency_offset.value = 2384.70
    car_config.config.target_power.value = 20.5
    car_config.config.tx_cd_pre_comp.value = 1234.56
    #car_config.config.advanced_param?
    car_config.config.alarm_threshold.pmd_oorh_threshold.value = 5
    car_config.config.alarm_threshold.post_fec_q_sf_threshold.value = 6.1
    car_config.config.alarm_threshold.pre_fec_q_signal_degrade_threshold.value = 7.2
    return car_config

def create(car_id):
    try:
        crud = CrudService()
        car = create_carrier_object(car_id)
        logger.debug(crud.create(car))
    except grpc.RpcError as err:
        logger.error("Set failed")
        logger.error(err.details)

if __name__ == '__main__':
    car_id = sys.argv[1]
    create(car_id)
