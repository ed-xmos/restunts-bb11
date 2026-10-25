include("${CMAKE_CURRENT_LIST_DIR}/embedded-source-list.cmake")

set(APP_HW_TARGET XCORE-AI-EXPLORER-800.xn)
set(APP_PCA_ENABLE OFF)
set(APP_COMPILER_FLAGS
    -O3
    -mno-dual-issue
    -g
    -report
    -std=gnu99
    -fsigned-char
    -mcmodel=large
    -DPATH_MAX=64
    -D__XMOS__
    -DRESTUNTS_FULL
    -DRESTUNTS_EMBEDDED_STUBS
    -DRESTUNTS_GIT_HASH=\"embedded\"
    -DUSE_EXTMEM=1
    -Wno-unknown-pragmas)
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