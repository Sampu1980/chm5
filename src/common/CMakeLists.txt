cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

set(BUILD_SHARED_LIBS OFF)

add_library(common STATIC "")

target_sources (
    common
    PRIVATE
        DpEnv.cpp
        AtlanticSerdesTable.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/DpEnv.h
        ${CMAKE_CURRENT_LIST_DIR}/AtlanticSerdesTable.h
)

target_include_directories(
    common
    PUBLIC
        ${UTIL_INCLUDE_DIR}
        ${LOGGER_INCLUDE_DIR}
        ${BCM_EPDM_DIR}
        ${BCM_MILB_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../gearbox
        
)

target_link_libraries(
    common
)
