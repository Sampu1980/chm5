cmake_minimum_required(VERSION 3.5 FATAL_ERROR)


set(CMAKE_CXX_STANDARD 14)
add_definitions(-DZ_arm -DPLP_MILLENIOB_SUPPORT -DPLP_EPDM_MILLENIOB)
set(CMAKE_C_FLAGS "-DPLP_MILLENIOB_SUPPORT")
set(CMAKE_C_FLAGS "-DPLP_EPDM_MILLENIOB")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function") # allow unused functions for now
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-type-limits ")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function") # allow unused functions for now
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-qualifiers")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")

# We need to build two gearbox libraries with two BCM SDK to support
# NSA upgrade. See GX-31240 for more details.

# build with previous BCM SDK
add_library(gearbox_hw_prev_rel "")

target_sources(
    gearbox_hw_prev_rel
    PRIVATE
    	GearBoxAdapter.cpp
    	Bcm81725.cpp
    	GearBoxUtils.cpp
    	Bcm81725_avs_util.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/Bcm81725.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxIf.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxAdIf.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxUtils.h
        ${CMAKE_CURRENT_LIST_DIR}/Bcm81725_avs_util.h
        ${CMAKE_CURRENT_LIST_DIR}/../adapter
        ${DP_PROTOBUF_INC_DIR}/infinera/hal/board/v2/board_state.pb.h
        ${DP_PROTOBUF_INC_DIR}/infinera/hal/common/v2/common.pb.h
)

target_include_directories(gearbox_hw_prev_rel PUBLIC
                           ${UTIL_INCLUDE_DIR}
                           ${BCM_EPDM_DIR}
                           ${BCM_MILB_DIR}
                           ${LOGGER_INCLUDE_DIR}
                           ${SERDES_INCLUDE_DIR}
                           ${DP_MANAGER_SRC_DIR}
                           ${DP_ADAPTER_SRC_DIR}
                           ${DP_PROTOBUF_INC_DIR}
                           ${DP_COMMON_DIR}
                           ${BCM_MILB_DIR}/bcm_pm_if
                           ${BCM_MILB_DIR}/phymod/include
                           ${BCM_MILB_DIR}/phymod/chip/millenio/tier1/capi
)

target_link_libraries(
	gearbox_hw_prev_rel
    ${UTIL_LIB_DIR}/libRegIf.a
    ${UTIL_LIB_DIR}/libUtil.a
    ${SERDES_LIB_DIR}/libSerdes.a
)

if (${ARCH} STREQUAL "arm64")
	target_link_libraries(gearbox_hw_prev_rel "-Wl,-whole-archive" ${BCM_EPDM_LIB_DIR_PREV_REL}/libepdm.a ${BCM_MILB_LIB_DIR_PREV_REL}/libphymodepil.a ${USR_LIB_DIR_ARCH}/liblightstep_tracer.a ${USR_LIB_DIR_ARCH}/libfdradapter.a "-Wl,-no-whole-archive" -L../../lib/ -ldp-protobuf)
elseif(${ARCH} STREQUAL "x86")

endif()

# build with lastest BCM SDK

add_library(gearbox_hw "")

target_sources(
    gearbox_hw
    PRIVATE
    	GearBoxAdapter.cpp
    	Bcm81725.cpp
    	GearBoxUtils.cpp
    	Bcm81725_avs_util.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxAdapter.h
        ${CMAKE_CURRENT_LIST_DIR}/Bcm81725.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxIf.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxAdIf.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxUtils.h
        ${CMAKE_CURRENT_LIST_DIR}/Bcm81725_avs_util.h
        ${CMAKE_CURRENT_LIST_DIR}/../adapter
        ${DP_PROTOBUF_INC_DIR}/infinera/hal/board/v2/board_state.pb.h
        ${DP_PROTOBUF_INC_DIR}/infinera/hal/common/v2/common.pb.h
)

target_include_directories(gearbox_hw PUBLIC
                           ${UTIL_INCLUDE_DIR}
                           ${BCM_EPDM_DIR}
                           ${BCM_MILB_DIR}
                           ${LOGGER_INCLUDE_DIR}
                           ${SERDES_INCLUDE_DIR}
                           ${DP_MANAGER_SRC_DIR}
                           ${DP_ADAPTER_SRC_DIR}
                           ${DP_PROTOBUF_INC_DIR}
                           ${DP_COMMON_DIR}
                           ${BCM_MILB_DIR}/bcm_pm_if
                           ${BCM_MILB_DIR}/phymod/include
                           ${BCM_MILB_DIR}/phymod/chip/millenio/tier1/capi
)

target_link_libraries(
	gearbox_hw
    ${UTIL_LIB_DIR}/libRegIf.a
    ${UTIL_LIB_DIR}/libUtil.a
    ${SERDES_LIB_DIR}/libSerdes.a
)

if (${ARCH} STREQUAL "arm64")
	target_link_libraries(gearbox_hw "-Wl,-whole-archive" ${BCM_EPDM_LIB_DIR}/libepdm.a ${BCM_MILB_LIB_DIR}/libphymodepil.a ${USR_LIB_DIR_ARCH}/liblightstep_tracer.a ${USR_LIB_DIR_ARCH}/libfdradapter.a "-Wl,-no-whole-archive" -L../../lib/ -ldp-protobuf)
elseif(${ARCH} STREQUAL "x86")

endif()

#################################
# Non HW Specific
add_library(gearbox "")

target_sources(
    gearbox
    PRIVATE
      GearBoxAdapterSim.cpp
      GearBoxCreator.cpp
      GearBoxUtils.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxAdIf.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxAdapterSim.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxCreator.h
        ${CMAKE_CURRENT_LIST_DIR}/GearBoxUtils.h

)

target_include_directories(gearbox PUBLIC
                           ${UTIL_INCLUDE_DIR}
                           ${BCM_EPDM_DIR}
                           ${BCM_MILB_DIR}
                           ${DP_COMMON_DIR}
                           ${DP_MANAGER_SRC_DIR}
                           ${LOGGER_INCLUDE_DIR}
                           ${DP_ADAPTER_SRC_DIR}
                           ${DP_PROTOBUF_INC_DIR}
                           ${DP_COMMON_DIR}

)

target_link_libraries(
  gearbox
  ${UTIL_LIB_DIR}/libUtil.a
)
