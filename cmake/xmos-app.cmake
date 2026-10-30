include("${CMAKE_CURRENT_LIST_DIR}/embedded-source-list.cmake")

set(RESTUNTS_XMOS_HW_TARGET "XK-EVK-XU416-800.xn" CACHE STRING "XMOS hardware target for the embedded application")
# set(RESTUNTS_XMOS_HW_TARGET "XCORE-AI-EXPLORER-800.xn" CACHE STRING "XMOS hardware target for the embedded application")
set_property(CACHE RESTUNTS_XMOS_HW_TARGET PROPERTY STRINGS
    XCORE-AI-EXPLORER-800.xn XK-EVK-XU416-800.xn)
set(APP_HW_TARGET "${RESTUNTS_XMOS_HW_TARGET}")
set(restunts_ffs_default OFF)
if(RESTUNTS_XMOS_HW_TARGET STREQUAL "XK-EVK-XU416-800.xn")
    set(restunts_ffs_default ON)
endif()
option(RESTUNTS_XMOS_USE_FFS "Read embedded game files from flash instead of host stdio" ${restunts_ffs_default})
set(RESTUNTS_XMOS_FILESYSTEM_DIR "${CMAKE_CURRENT_LIST_DIR}/../../lib_filesystem/lib_filesystem"
    CACHE PATH "Path to the lib_filesystem library")
if(RESTUNTS_XMOS_USE_FFS)
    if(NOT RESTUNTS_XMOS_HW_TARGET STREQUAL "XK-EVK-XU416-800.xn")
        message(FATAL_ERROR "RESTUNTS_XMOS_USE_FFS currently supports XK-EVK-XU416-800.xn only")
    endif()
    set(restunts_petit_fs "${RESTUNTS_XMOS_FILESYSTEM_DIR}/src/file_systems/petit_fatfs")
    if(NOT EXISTS "${restunts_petit_fs}/pff.c")
        message(FATAL_ERROR "Set RESTUNTS_XMOS_FILESYSTEM_DIR to the lib_filesystem library directory")
    endif()
endif()
set(APP_PCA_ENABLE OFF)
set(APP_COMPILER_FLAGS
    -g
    -report
    -std=gnu99
    -fsigned-char
    -DPATH_MAX=64
    -D__XMOS__
    -DRESTUNTS_FULL
    -DRESTUNTS_EMBEDDED_STUBS
    -DRESTUNTS_GIT_HASH=\"embedded\"
    -Wno-unknown-pragmas)
if(RESTUNTS_XMOS_USE_FFS)
    list(APPEND APP_COMPILER_FLAGS -DRESTUNTS_XMOS_USE_FFS=1)
endif()
if(RESTUNTS_XMOS_HW_TARGET STREQUAL "XK-EVK-XU416-800.xn")
    list(PREPEND APP_COMPILER_FLAGS -Os -DRESTUNTS_LEGACY_MEMORY_IN_SOC_RAM=1)
else()
    list(PREPEND APP_COMPILER_FLAGS -O3 -mno-dual-issue -mcmodel=large -DUSE_EXTMEM=1)
endif()


set(APP_XSCOPE_SRCS         src/restunts/platform/embedded/config.xscope)


    option(RESTUNTS_XMOS_DEBUG "Enable embedded callback trace output" OFF)
if(RESTUNTS_XMOS_DEBUG)
    list(APPEND APP_COMPILER_FLAGS -DRESTUNTS_EMBEDDED_DEBUG=1)
endif()

option(RESTUNTS_XMOS_SAVE_FRAMES "Save presented frames as BMP files" ON)
if(RESTUNTS_XMOS_SAVE_FRAMES)
    list(APPEND APP_COMPILER_FLAGS -DRESTUNTS_EMBEDDED_SAVE_FRAMES=1)
endif()

set(APP_C_SRCS)
foreach(source IN LISTS restunts_embedded_game_sources)
    list(APPEND APP_C_SRCS "src/restunts/c/${source}")
endforeach()

list(APPEND APP_C_SRCS
    src/restunts/platform/sdl3/memory.c
    src/restunts/platform/embedded/stubs.c
    src/restunts/platform/embedded/xmos_main.c)

    set(APP_INCLUDES
    src/restunts/c
    src/restunts/platform/embedded)

    set(XMOS_SANDBOX_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

XMOS_REGISTER_APP()
if(RESTUNTS_XMOS_USE_FFS)
    target_sources(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../src/restunts/platform/embedded/flash_filesystem.c"
        "${restunts_petit_fs}/pff.c")
    target_include_directories(${PROJECT_NAME} PRIVATE "${restunts_petit_fs}")
    target_link_libraries(${PROJECT_NAME} PRIVATE flashdata flash)
endif()