cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

set(BUILD_SHARED_LIBS OFF)

add_definitions(-fPIC)

add_library(dbgcmds STATIC "")

target_sources (
    dbgcmds
    PRIVATE
        dbgcmds.cpp
        adapter_cmds.cpp
        bcm_cmds.cpp
        serdes_tuner_cmds.cpp
)

target_include_directories(
    dbgcmds
    PUBLIC
        ${DCO_INTF_INC_DIR}
        ${YGOT_INC_DIR}
        ${CLI_INCLUDE_DIR}
        ${DP_COMMON_DIR}
        ${UTIL_INCLUDE_DIR}
        ${BCM_EPDM_DIR}
        ${BCM_MILB_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../adapter
        ${CMAKE_CURRENT_LIST_DIR}/../gearbox
        ${SERDES_TNR_MGR_DIR}
        ${SERDES_INCLUDE_DIR}
        ${GRPC_ADAPTER_SRC_DIR}
        ${DP_MANAGER_SRC_DIR}
        ${DP_COMMON_DIR}
)

target_link_libraries(
    dbgcmds
    manager
)
