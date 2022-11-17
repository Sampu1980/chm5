cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
add_library(dp_sim "")

target_sources(
    dp_sim
    PRIVATE
        SimCrudService.cpp
        SimObjMgr.cpp
        SimDcoCarrierMgr.cpp
        SimDcoCardMgr.cpp
        SimDcoGigeMgr.cpp
        SimDcoOtuMgr.cpp
        SimDcoOduMgr.cpp
        SimDcoXconMgr.cpp
        SimDcoGccMgr.cpp
        SimCmds.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/SimCrudService.h
        ${CMAKE_CURRENT_LIST_DIR}/SimObjMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimDcoCarrierMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimDcoCardMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimDcoGigeMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimDcoOtuMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimDcoOduMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimDcoXconMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimDcoGccMgr.h
        ${CMAKE_CURRENT_LIST_DIR}/SimCmds.h
    )

target_include_directories(
    dp_sim
    PUBLIC
        ${DP_INC_DIR}
        ${CMAKE_CURRENT_LIST_DIR}
        ${GRPC_ADAPTER_SRC_DIR}
        ${GRPC_SRC_DIR}
        ${DCO_INTF_INC_DIR}
        ${DCO_INTF_SRC_DIR}
        ${UTIL_INCLUDE_DIR}
        ${DP_MANAGER_SRC_DIR}
        ${CLI_INCLUDE_DIR}
        ${DP_ADAPTER_SRC_DIR}
        ${DP_COMMON_DIR}
        ${DP_PROTOBUF_INC_DIR}
        ${LOGGER_INCLUDE_DIR}
    )
