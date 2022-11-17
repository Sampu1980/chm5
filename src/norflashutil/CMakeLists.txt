cmake_minimum_required (VERSION 3.13.4)

add_library (norflashutil "")

add_definitions(-DCHM6_GECKOINTF)


if (${ARCH} STREQUAL "arm64")
    target_sources(
        norflashutil
        PRIVATE
            NORFlashUtil.cpp
            keyCtl.c
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/NORFlashUtil.h
    )
elseif (${ARCH} STREQUAL "x86")
    target_sources(
        norflashutil
        PRIVATE
            NORFlashUtil.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/NORFlashUtil.h
    )
endif()

target_include_directories(
    norflashutil
    PUBLIC
    ${DP_INC_DIR}
    ${UTIL_INCLUDE_DIR}
    ${LOGGER_INCLUDE_DIR} 
    ${DP_NORFLASHUTIL_DIR}
)

if (${ARCH} STREQUAL "arm64")
    target_link_libraries(
        norflashutil
        PUBLIC 
        InfnLogger
        keyutils
        ssl
        crypto
        noriskutil
        keymgmtutil
        shmmutexaccess
        libgeckointf.a
    )                                     
elseif (${ARCH} STREQUAL "x86")
    target_link_libraries(
        norflashutil
        PUBLIC 
        InfnLogger
        ssl
        crypto
        noriskutil
        keymgmtutil
        shmmutexaccess
        libgeckointf.a
    )                                     
endif()
