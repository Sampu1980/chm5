cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
project(ms_main_app)

if (${ARCH} STREQUAL "arm64")
    message("*** Build Target Set For ARM 64")
elseif(${ARCH} STREQUAL "x86")
    message("*** Build Target Set For x86 64")
    add_definitions(-DARCH_x86)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

#option(CROSS_COMPILE_ARM64 "Enable or disable cross compiling for ARM64" ON)
#option(CROSS_COMPILE_ARM64 "Enable or disable cross compiling for ARM64" OFF)
option(GCOV "enable or disable gcov flags" OFF)
include(../../cmake/cpp_flags.cmake)
include(../../cmake/coverity_build.cmake)

set (DCO_INTF_SRC_DIR /home/infinera/src_gen/dco/cpp/dcoyang)
set (DCO_INTF_INC_DIR /home/infinera/src_gen/dco/cpp)
set (YGOT_INC_DIR /root/go/src)
set (YGOT_SRC_DIR /root/go/src/github.com/openconfig/ygot/proto)
set (GRPC_ADAPTER_SRC_DIR /home/infinera/grpc_client_adapter/cpp)
set (GRPC_SRC_DIR /home/infinera/grpc/include)
set (DP_COMMON_DIR /home/infinera/src/common)
set (DP_PROTOBUF_SRC_DIR /home/infinera/src_gen/cpp)
set (DP_PROTOBUF_INC_DIR /home/infinera/src_gen/cpp)
set (DP_MANAGER_INC_DIR /home/infinera/src/manager/include)
set (DP_MANAGER_SRC_DIR /home/infinera/src/manager)
set (DP_ADAPTER_SRC_DIR /home/infinera/src/adapter)
set (DP_GEARBOX_SRC_DIR /home/infinera/src/gearbox)
set (DP_SIM_GNMI_INC_DIR /home/infinera/src/sim)
set (DP_NORFLASHUTIL_DIR /home/infinera/src/norflashutil)
set(CLI_SRC_DIR ${CMAKE_SOURCE_DIR}/external-repos/cli)
set(CLI_INCLUDE_DIR ${CLI_SRC_DIR}/include)
set(CLI_LIB_DIR ${CMAKE_BINARY_DIR}/lib)

set(CLI_SRC_DIR ${CMAKE_SOURCE_DIR}/external-repos/cli)
set(CLI_INCLUDE_DIR ${CLI_SRC_DIR}/include)
set(CLI_LIB_DIR ${CMAKE_BINARY_DIR}/lib)

set(SERDES_INCLUDE_DIR /home/src/include)
set(SERDES_LIB_DIR /usr/local/lib)
set(LOGGER_INCLUDE_DIR /home/infinera/src_art/logging/include)

set(SERDES_TNR_MGR_DIR /home/infinera/src/serdesTunerManager)



# Pulled from other Infinera/CHM6 Artifactories
set(REDIS_ADAPTER_LIB_DIR /usr/local/lib)
set(REDIS_ADAPTER_INC_DIR /usr/local/include)

# setting for gearbox
add_definitions(-fPIC -DZ_LINUX  -DLINUX_COMPILE)
set(CMAKE_CXX_FLAGS "-pthread")
if(${ARCH} STREQUAL "x86")
   set(USR_LIB_DIR_ARCH /usr/lib/x86_64-linux-gnu)
   set(USR_LOCAL_LIB_DIR_ARCH /usr/local/lib/x86_64-linux-gnu)
   set(USR_LOCAL_INC_DIR_ARCH /usr/local/include/x86_64-linux-gnu)
elseif (${ARCH} STREQUAL "arm64")
   set(USR_LIB_DIR_ARCH /usr/lib/aarch64-linux-gnu)
   set(USR_LOCAL_LIB_DIR_ARCH /usr/local/lib/aarch64-linux-gnu)
   set(USR_LOCAL_INC_DIR_ARCH /usr/local/include/aarch64-linux-gnu)
endif()
set(BOOST_INCLUDE_DIR /usr/include/boost)
set(USR_LIB_DIR /usr/lib)

set(BCM_SDK_DIR ${USR_LOCAL_INC_DIR_ARCH})
set(BCM_SDK_LIB_DIR ${USR_LOCAL_LIB_DIR_ARCH})

# Set epdm include and lib dir
set(BCM_EPDM_DIR_PREV_REL ${BCM_SDK_DIR}/epdm_prev_rel)
set(BCM_EPDM_LIB_DIR_PREV_REL ${BCM_SDK_LIB_DIR}/epdm_prev_rel)
set(BCM_EPDM_DIR ${BCM_SDK_DIR}/epdm)
set(BCM_EPDM_LIB_DIR ${BCM_SDK_LIB_DIR}/epdm)

# Set milb include and lib dir
set(BCM_MILB_DIR_PREV_REL ${BCM_SDK_DIR}/milb_prev_rel)
set(BCM_MILB_LIB_DIR_PREV_REL ${BCM_SDK_LIB_DIR}/milb_prev_rel)
set(BCM_MILB_DIR ${BCM_SDK_DIR}/milb)
set(BCM_MILB_LIB_DIR ${BCM_SDK_LIB_DIR}/milb)


set(UTIL_INCLUDE_DIR /home/src/include)
set(UTIL_LIB_DIR /usr/local/lib)


# Build lib from auto generated DCO proto cc files
if(TRUE)
add_library(dco-intf STATIC "")

target_sources(
  dco-intf
  PRIVATE
    ${YGOT_SRC_DIR}/ywrapper/ywrapper.pb.cc
    ${YGOT_SRC_DIR}/yext/yext.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_card/infinera_dco_card.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_carrier/infinera_dco_carrier.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_client_gige/infinera_dco_client_gige.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_otu/infinera_dco_otu.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_odu/infinera_dco_odu.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_xcon/infinera_dco_xcon.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_gcc_control/infinera_dco_gcc_control.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_card_pm/infinera_dco_card_pm.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_carrier_pm/infinera_dco_carrier_pm.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_client_gige_pm/infinera_dco_client_gige_pm.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_gcc_control_pm/infinera_dco_gcc_control_pm.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_odu_pm/infinera_dco_odu_pm.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_otu_pm/infinera_dco_otu_pm.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_card_fault/infinera_dco_card_fault.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_carrier_fault/infinera_dco_carrier_fault.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_client_gige_fault/infinera_dco_client_gige_fault.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_gcc_control_fault/infinera_dco_gcc_control_fault.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_odu_fault/infinera_dco_odu_fault.pb.cc
    ${DCO_INTF_SRC_DIR}/infinera_dco_otu_fault/infinera_dco_otu_fault.pb.cc
    ${DCO_INTF_SRC_DIR}/enums/enums.pb.cc

  PUBLIC
    ${YGOT_SRC_DIR}/ywrapper/ywrapper.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_card/infinera_dco_card.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_carrier/infinera_dco_carrier.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_client_gige/infinera_dco_client_gige.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_otu/infinera_dco_otu.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_odu/infinera_dco_odu.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_xcon/infinera_dco_xcon.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_gcc_control/infinera_dco_gcc_control.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_card_pm/infinera_dco_card_pm.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_carrier_pm/infinera_dco_carrier_pm.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_client_gige_pm/infinera_dco_client_gige_pm.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_gcc_control_pm/infinera_dco_gcc_control_pm.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_odu_pm/infinera_dco_odu_pm.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_otu_pm/infinera_dco_otu_pm.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_card_fault/infinera_dco_card_fault.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_carrier_fault/infinera_dco_carrier_fault.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_client_gige_fault/infinera_dco_client_gige_fault.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_gcc_control_fault/infinera_dco_gcc_control_fault.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_odu_fault/infinera_dco_odu_fault.pb.h
    ${DCO_INTF_SRC_DIR}/infinera_dco_otu_fault/infinera_dco_otu_fault.pb.h
    ${DCO_INTF_SRC_DIR}/enums/enums.pb.h
    )

target_include_directories(
  dco-intf
  PUBLIC
    ${DCO_INTF_INC_DIR}
    ${YGOT_INC_DIR}
    )

add_library( dp-protobuf STATIC "")

file(GLOB_RECURSE SRC_PROTO_WRAPPERS        "${DP_PROTOBUF_SRC_DIR}/infinera/wrappers/v1/*.cc")
file(GLOB_RECURSE SRC_PROTO_CHM6_CMN        "${DP_PROTOBUF_SRC_DIR}/infinera/chm6/common/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_LCCMN_CMN       "${DP_PROTOBUF_SRC_DIR}/infinera/lccmn/common/v1/*.cc")
file(GLOB_RECURSE SRC_PROTO_CHM6_TOM        "${DP_PROTOBUF_SRC_DIR}/infinera/chm6/tom/v3/*.cc")
file(GLOB_RECURSE SRC_PROTO_CHM6_DP         "${DP_PROTOBUF_SRC_DIR}/infinera/chm6/dataplane/v3/*.cc")
file(GLOB_RECURSE SRC_PROTO_LCCMN_TOM       "${DP_PROTOBUF_SRC_DIR}/infinera/lccmn/tom/v1/*.cc")
file(GLOB_RECURSE SRC_PROTO_LCCMN_DP        "${DP_PROTOBUF_SRC_DIR}/infinera/lccmn/dataplane/v1/*.cc")
file(GLOB_RECURSE SRC_PROTO_CHM6_PCP        "${DP_PROTOBUF_SRC_DIR}/infinera/chm6/pcp/v5/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_TOM         "${DP_PROTOBUF_SRC_DIR}/infinera/hal/tom/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_CMN         "${DP_PROTOBUF_SRC_DIR}/infinera/hal/common/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_CHM6        "${DP_PROTOBUF_SRC_DIR}/infinera/hal/chm6/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_EQPT        "${DP_PROTOBUF_SRC_DIR}/infinera/hal/equipment/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_DP          "${DP_PROTOBUF_SRC_DIR}/infinera/hal/dataplane/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_PCP         "${DP_PROTOBUF_SRC_DIR}/infinera/hal/pcp/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_KEYMGMT_V1  "${DP_PROTOBUF_SRC_DIR}/infinera/hal/keymgmt/v1/*.cc")
file(GLOB_RECURSE SRC_PROTO_HAL_KEYMGMT_V2  "${DP_PROTOBUF_SRC_DIR}/infinera/hal/keymgmt/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_SECPROC_BRD     "${DP_PROTOBUF_SRC_DIR}/infinera/dco6secproc/board/v1/*.cc")
file(GLOB_RECURSE SRC_PROTO_SECPROC_DP      "${DP_PROTOBUF_SRC_DIR}/infinera/dco6secproc/dataplane/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_SECPROC_CP      "${DP_PROTOBUF_SRC_DIR}/infinera/dco6secproc/ctrlplane/v1/*.cc")
file(GLOB_RECURSE SRC_PROTO_SECPROC_KM      "${DP_PROTOBUF_SRC_DIR}/infinera/dco6secproc/keymgmt/v2/*.cc")
file(GLOB_RECURSE SRC_PROTO_CHM6_KM          "${DP_PROTOBUF_SRC_DIR}/infinera/chm6/keymgmt/v2/*.cc")

target_sources(
  dp-protobuf
  PRIVATE
    ${SRC_PROTO_WRAPPERS}
    ${SRC_PROTO_CHM6_CMN}
    ${SRC_PROTO_LCCMN_CMN}
    ${SRC_PROTO_HAL_TOM}
    ${SRC_PROTO_HAL_CMN}
    ${SRC_PROTO_CHM6_TOM}
    ${SRC_PROTO_CHM6_DP}
    ${SRC_PROTO_LCCMN_TOM}
    ${SRC_PROTO_LCCMN_DP}
    ${SRC_PROTO_CHM6_PCP}
    ${SRC_PROTO_HAL_CHM6}
    ${SRC_PROTO_HAL_EQPT}
    ${SRC_PROTO_HAL_DP}
    ${SRC_PROTO_SECPROC_BRD}
    ${SRC_PROTO_SECPROC_DP}
    ${SRC_PROTO_SECPROC_CP}
    ${SRC_PROTO_SECPROC_KM}
    ${SRC_PROTO_HAL_PCP}
    ${SRC_PROTO_HAL_KEYMGMT_V1}
    ${SRC_PROTO_HAL_KEYMGMT_V2}
    ${SRC_PROTO_CHM6_KM}
    ${DP_PROTOBUF_SRC_DIR}/compiled_generation.cc
    )

target_include_directories(
  dp-protobuf
  PUBLIC
    ${DP_PROTOBUF_INC_DIR}
    )

endif()

add_subdirectory(../../external-repos/cli ${CMAKE_BINARY_DIR}/external-repos/cli)
add_subdirectory(../manager             ${CMAKE_BINARY_DIR}/src/manager)
add_subdirectory(../norflashutil        ${CMAKE_BINARY_DIR}/src/norflashutil)
add_subdirectory(../serdesTunerManager  ${CMAKE_BINARY_DIR}/src/serdesTunerManager)
add_subdirectory(../adapter             ${CMAKE_BINARY_DIR}/src/adapter)
add_subdirectory(../gearbox             ${CMAKE_BINARY_DIR}/src/gearbox)
add_subdirectory(../dbgcmds             ${CMAKE_BINARY_DIR}/src/dbgcmds)
add_subdirectory(../sim                 ${CMAKE_BINARY_DIR}/src/sim)
add_subdirectory(../common              ${CMAKE_BINARY_DIR}/src/common)

add_executable(
    chm6_dp_prev_rel_ms
    dpms_main.cpp
)

target_include_directories(
    chm6_dp_prev_rel_ms
    PUBLIC
        ${LOGGER_INCLUDE_DIR}
        ${UTIL_INCLUDE_DIR}
        $(SERDES_INCLUDE_DIR)
)

add_executable(
    chm6_dp_ms
    dpms_main.cpp
)

target_include_directories(
    chm6_dp_ms
    PUBLIC
        ${LOGGER_INCLUDE_DIR}
        ${UTIL_INCLUDE_DIR}
        $(SERDES_INCLUDE_DIR)
)

add_library(chm6grpcadapter STATIC IMPORTED)
set_target_properties(chm6grpcadapter PROPERTIES
    IMPORTED_LOCATION "/usr/local/lib/libchm6grpcadapter.a"
    )

add_library(gnmi STATIC IMPORTED)
set_target_properties(gnmi PROPERTIES
    IMPORTED_LOCATION "/usr/local/lib/libgnmi.a"
    )

link_directories(
    ${CLI_LIB_DIR}
    $(SERDES_LIB_DIR)
)

add_library(jsoncpp STATIC IMPORTED)
set_target_properties(jsoncpp PROPERTIES IMPORTED_LOCATION "/usr/local/lib/libjsoncpp.a"
    )
add_library(crypto STATIC IMPORTED)
add_library(ssl STATIC IMPORTED)
if(${ARCH} STREQUAL "x86")
   set_property(TARGET crypto PROPERTY
                IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libcrypto.a")
   set_property(TARGET ssl PROPERTY
                IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libssl.a")
elseif (${ARCH} STREQUAL "arm64")
   set_property(TARGET crypto PROPERTY
                IMPORTED_LOCATION "/usr/lib/aarch64-linux-gnu/libcrypto.a")
   set_property(TARGET ssl PROPERTY
                IMPORTED_LOCATION "/usr/lib/aarch64-linux-gnu/libssl.a")
endif()

add_library(regif STATIC IMPORTED)
set_target_properties(regif PROPERTIES IMPORTED_LOCATION "/usr/local/lib/libRegIf.a")

add_library(InfnLogger INTERFACE)
target_include_directories(InfnLogger INTERFACE "/home/infinera/src_art/logging/include")

set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

# add add_dependencies to build the generated code last.
#add_dependencies( dco-intf dp-protobuf dbgcmds adapter manager )
add_dependencies( dco-intf dp-protobuf dbgcmds manager adapter )
add_dependencies( dbgcmds manager adapter )

if(${ARCH} STREQUAL "x86")
    target_link_libraries(
        chm6_dp_prev_rel_ms
        manager
        adapter
        dbgcmds
        dco-intf
        dp-protobuf
        serdes_manager
        gearbox
        dp_sim
        grpc++
        protobuf
        pthread
        dl
        gpr
        crypto
        pthread
        ssl
    	grpc++_reflection
        chm6grpcadapter
        gnmi
        cli::cli
        jsoncpp
        cli::cli
	${USR_LIB_DIR_ARCH}/libInfnTracer.a
        ${USR_LIB_DIR_ARCH}/libboost_system.a
        ${USR_LIB_DIR_ARCH}/libboost_thread.a
        ${USR_LIB_DIR_ARCH}/libboost_log.a
        ${USR_LIB_DIR_ARCH}/libboost_log_setup.a
        ${USR_LIB_DIR_ARCH}/libboost_filesystem.a
        ${USR_LIB_DIR_ARCH}/liblightstep_tracer.a
        ${USR_LIB_DIR_ARCH}/libopentracing.a
        ${USR_LIB_DIR_ARCH}/libopentracing_mocktracer.a
        regif
        common
        libInfnLogger.a
        libSerdes.a
        rt
        libshmmutexaccess.a
        libgeckointf.a
        libgecko_access.a
	#-Wl,-whole-archive /usr/local/lib/libfdradapter.a -Wl,-no-whole-archive
    )

    target_link_libraries(
        chm6_dp_ms
        manager
        adapter
        dbgcmds
        dco-intf
        dp-protobuf
        serdes_manager
        gearbox
        dp_sim
        grpc++
        protobuf
        pthread
        dl
        gpr
        crypto
        pthread
        ssl
    	grpc++_reflection
        chm6grpcadapter
        gnmi
        cli::cli
        jsoncpp
        cli::cli
	${USR_LIB_DIR_ARCH}/libInfnTracer.a
        ${USR_LIB_DIR_ARCH}/libboost_system.a
        ${USR_LIB_DIR_ARCH}/libboost_thread.a
        ${USR_LIB_DIR_ARCH}/libboost_log.a
        ${USR_LIB_DIR_ARCH}/libboost_log_setup.a
        ${USR_LIB_DIR_ARCH}/libboost_filesystem.a
        ${USR_LIB_DIR_ARCH}/liblightstep_tracer.a
        ${USR_LIB_DIR_ARCH}/libopentracing.a
        ${USR_LIB_DIR_ARCH}/libopentracing_mocktracer.a
        regif
        common
        libInfnLogger.a
        libSerdes.a
        rt
        libshmmutexaccess.a
        libgeckointf.a
        libgecko_access.a
	#-Wl,-whole-archive /usr/local/lib/libfdradapter.a -Wl,-no-whole-archive
    )
elseif (${ARCH} STREQUAL "arm64")
    target_link_libraries(
        chm6_dp_prev_rel_ms
        manager
        adapter
        dbgcmds
        dco-intf
        dp-protobuf
        serdes_manager
        gearbox
        gearbox_hw_prev_rel
        dp_sim
        grpc++
        protobuf
        pthread
        dl
        gpr
        crypto
        pthread
        ssl
        grpc++_reflection
        chm6grpcadapter
        gnmi
        cli::cli
        jsoncpp
        cli::cli
	${USR_LIB_DIR_ARCH}/libInfnTracer.a
        ${USR_LIB_DIR_ARCH}/libboost_system.a
        ${USR_LIB_DIR_ARCH}/libboost_thread.a
        ${USR_LIB_DIR_ARCH}/libboost_log.a
        ${USR_LIB_DIR_ARCH}/libboost_log_setup.a
        ${USR_LIB_DIR_ARCH}/libboost_filesystem.a
        ${USR_LIB_DIR_ARCH}/liblightstep_tracer.a
        ${USR_LIB_DIR_ARCH}/libopentracing.a
        ${USR_LIB_DIR_ARCH}/libopentracing_mocktracer.a
        regif
        common
        libInfnLogger.a
        libSerdes.a
        rt
        libshmmutexaccess.a
        libgeckointf.a
        libgecko_access.a
    )
    target_link_libraries(
        chm6_dp_ms
        manager
        adapter
        dbgcmds
        dco-intf
        dp-protobuf
        serdes_manager
        gearbox
        gearbox_hw
        dp_sim
        grpc++
        protobuf
        pthread
        dl
        gpr
        crypto
        pthread
        ssl
        grpc++_reflection
        chm6grpcadapter
        gnmi
        cli::cli
        jsoncpp
        cli::cli
	${USR_LIB_DIR_ARCH}/libInfnTracer.a
        ${USR_LIB_DIR_ARCH}/libboost_system.a
        ${USR_LIB_DIR_ARCH}/libboost_thread.a
        ${USR_LIB_DIR_ARCH}/libboost_log.a
        ${USR_LIB_DIR_ARCH}/libboost_log_setup.a
        ${USR_LIB_DIR_ARCH}/libboost_filesystem.a
        ${USR_LIB_DIR_ARCH}/liblightstep_tracer.a
        ${USR_LIB_DIR_ARCH}/libopentracing.a
        ${USR_LIB_DIR_ARCH}/libopentracing_mocktracer.a
        regif
        common
        libInfnLogger.a
        libSerdes.a
        rt
        libshmmutexaccess.a
        libgeckointf.a
        libgecko_access.a
    )
endif()
