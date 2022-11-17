
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")

###### For ARM ARCH compilation setting ######
if(CROSS_COMPILE_ARM64)
    # set cross-compiled system type, it's better not use the type which cmake cannot recognized.
    SET( CMAKE_SYSTEM_NAME Linux )
    SET( CMAKE_SYSTEM_PROCESSOR aarch64 )
    SET(TARGET_ABI "linux-gnu")
    SET( CMAKE_C_COMPILER "/usr/bin/aarch64-${TARGET_ABI}-gcc" )
    SET( CMAKE_CXX_COMPILER "/usr/bin/aarch64-${TARGET_ABI}-g++" )

    # set searching rules for cross-compiler
    SET(CMAKE_FIND_ROOT_PATH "/usr/aarch64-${TARGET_ABI}")
    SET( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
    SET( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
    SET( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

    # set ${CMAKE_C_FLAGS} and ${CMAKE_CXX_FLAGS}flag for cross-compiled process
    SET( CMAKE_CXX_FLAGS "-march=armv8-a ${CMAKE_CXX_FLAGS}" )
    SET(CMAKE_EXE_LINKER_FLAGS "-static")

    # other settings
    add_definitions(-D__ARM_NEON)
    add_definitions(-DLINUX)
    SET( LINUX true)
endif()
###### End ARM ARCH compilation setting ######


# place binaries and libraries according to GNU standards
include(GNUInstallDirs)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

option(GCOV "enable or disable gcov flags" OFF)
# we use this to get code coverage
if(CMAKE_CXX_COMPILER_ID MATCHES GNU AND GCOV)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -g")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

