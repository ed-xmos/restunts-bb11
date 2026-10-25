set(core "${CMAKE_SOURCE_DIR}/src/restunts/c")
include("${CMAKE_CURRENT_LIST_DIR}/embedded-source-list.cmake")
set(embedded_sources ${restunts_embedded_game_sources})
list(TRANSFORM embedded_sources PREPEND "${core}/")

add_library(restunts_embedded_stubs STATIC
    ${embedded_sources}
    "${CMAKE_SOURCE_DIR}/src/restunts/platform/sdl3/memory.c"
    "${CMAKE_SOURCE_DIR}/src/restunts/platform/embedded/stubs.c")
set_target_properties(restunts_embedded_stubs PROPERTIES
    C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_definitions(restunts_embedded_stubs PRIVATE RESTUNTS_FULL RESTUNTS_EMBEDDED_STUBS
    RESTUNTS_GIT_HASH="embedded")
target_include_directories(restunts_embedded_stubs PRIVATE "${core}")
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(restunts_embedded_stubs PRIVATE
        -Os -ffunction-sections -fdata-sections -fsigned-char -fno-strict-aliasing
        -Wall -Wextra -Wno-pointer-sign -Wno-missing-field-initializers -Wno-missing-braces)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    find_package(Threads REQUIRED)
    add_executable(restunts_embedded_runner
        "${CMAKE_SOURCE_DIR}/src/restunts/platform/embedded/native_main.c")
    set_target_properties(restunts_embedded_runner PROPERTIES
        C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    target_include_directories(restunts_embedded_runner PRIVATE
        "${CMAKE_SOURCE_DIR}/src/restunts/platform/embedded")
    target_link_libraries(restunts_embedded_runner PRIVATE
        restunts_embedded_stubs Threads::Threads)
endif()

message(STATUS "Building dependency-free C99 embedded stub library: restunts_embedded_stubs")