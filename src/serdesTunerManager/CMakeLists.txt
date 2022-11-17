cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
add_library(
    serdes_manager
    STATIC
        SerdesTuner.cpp
    )
        
target_sources(
    serdes_manager
    PRIVATE
        SerdesTuner.cpp
        SerdesTunerCreator.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/SerdesConnMap.h
        ${CMAKE_CURRENT_LIST_DIR}/SerdesTuner.h
        ${CMAKE_CURRENT_LIST_DIR}/SerdesTunerCreator.h
        ${DP_PROTOBUF_INC_DIR}/infinera/chm6/common/v2/tom_serdes.pb.h
        ${DP_PROTOBUF_INC_DIR}/infinera/chm6/tom/v3/tom_state.pb.h
    )
        

target_include_directories(
    serdes_manager
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../adapter
        ${CMAKE_CURRENT_LIST_DIR}/../gearbox
        ${CMAKE_CURRENT_LIST_DIR}/../manager
        ${BCM_EPDM_DIR}
        ${BCM_MILB_DIR}
        ${DP_COMMON_DIR}
        ${BOOST_INCLUDE_DIR}
        ${UTIL_INCLUDE_DIR}
        ${CLI_INCLUDE_DIR}
        ${LOGGER_INCLUDE_DIR}
        ${SERDES_INCLUDE_DIR}
        ${DP_PROTOBUF_INC_DIR}
        ${REDIS_ADAPTER_INC_DIR}
        ${DP_MANAGER_SRC_DIR}
        ${DP_COMMON_DIR}
    )
    

        
target_link_libraries(
    serdes_manager
	${SERDES_LIB_DIR}/libSerdes.a
    ${USR_LIB_DIR_ARCH}/libhiredis.a
    ${USR_LIB_DIR}/libprotobuf.a
    ${REDIS_ADAPTER_LIB_DIR}/libredisadapter.a
    protobuf
    dp-protobuf
	)
