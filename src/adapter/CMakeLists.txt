cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

add_library(adapter "")

add_definitions(-DCHM6_GECKOINTF)

target_sources(
    adapter
    PRIVATE
        DpPm.cpp
        DcoSecProcActor.cpp
        DcoCardAdapter.cpp
        DcoGigeClientAdapter.cpp
        DcoCarrierAdapter.cpp
        DcoOtuAdapter.cpp
        DcoOduAdapter.cpp
        DcoXconAdapter.cpp
        DcoGccControlAdapter.cpp
        DcoPmSBAdapter.cpp
        DcoSecProcAdapter.cpp
        DcoConnectHandler.cpp
        DcoSecProcCarrierAdapter.cpp
        DcoSecProcVlanAdapter.cpp
        RetryHelperIntf.cpp
        DcoSecProcIcdpAdapter.cpp
        AdapterCommon.cpp
        DcoSecProcKeyMgmtAdapter.cpp
        HostKeyMgmtAdapter.cpp
        DcoZynqKeyMgmtAdapter.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/InfnDeQueue.h
        ${CMAKE_CURRENT_LIST_DIR}/InfnDeQueueActor.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoSecProcEvent.h
        ${CMAKE_CURRENT_LIST_DIR}/DpPm.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoSecProcActor.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoCardAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoGigeClientAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoCarrierAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoOtuAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoOduAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoXconAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoGccControlAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoPmSBAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoSecProcAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoConnectHandler.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoSecProcCarrierAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoSecProcVlanAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/RetryHelperIntf.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoSecProcIcdpAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/AdapterCommon.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoSecProcKeyMgmtAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/HostKeyMgmtAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/DcoZynqKeyMgmtAdapter.h
    )

target_include_directories(
    adapter
    PUBLIC
        ${DP_INC_DIR}
        ${CMAKE_CURRENT_LIST_DIR}
        ${DCO_INTF_INC_DIR}
        ${DCO_INTF_SRC_DIR}
	${DP_MANAGER_SRC_DIR}
        ${YGOT_INC_DIR}
        ${GRPC_ADAPTER_SRC_DIR}
        ${DP_GEARBOX_SRC_DIR}
        ${BCM_EPDM_DIR}
        ${BCM_MILB_DIR}
        ${SERDES_TNR_MGR_DIR}
        ${SERDES_INCLUDE_DIR}
        ${DP_PROTOBUF_INC_DIR}
        ${GRPC_SRC_DIR}
        ${DP_COMMON_DIR}
        ${DP_SIM_GNMI_INC_DIR}
        ${UTIL_INCLUDE_DIR}
        ${LOGGER_INCLUDE_DIR}
        ${REDIS_ADAPTER_INC_DIR}
        ${DP_NORFLASHUTIL_DIR}
    )

target_link_libraries(
    adapter
    PUBLIC
        norflashutil 
)