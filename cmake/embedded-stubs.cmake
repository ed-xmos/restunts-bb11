set(core "${CMAKE_SOURCE_DIR}/src/restunts/c")
set(embedded_sources
    fileio.c legacy.c resource.c math.c memmgr.c strlib.c numlib.c kevinrnd.c
    replinit.c gameinit.c gamestep.c simd.c carsetup.c trkutil.c trackres.c
    headless_data.c headless_trackdata.c state.c physics_grip.c opponent.c
    track_setup.c stateply.c physics_collision.c statecrs.c statecar.c trackobj.c
    owoot.c owoot_wheels.c owoot_road.c owoot_route.c replay.c stateio.c
    restunts.c command_line.c fatal.c timing.c game_input.c ui_text.c ui_input.c
    ui_dialog.c intro.c highscore.c replay_record.c replay_viewer.c race.c ghost.c
    race_resources.c dashboard.c menu_common.c menu_main.c menu_options.c menu_track.c
    menu_track_editor.c menu_opponent.c menu_car.c audio_engine.c audio_resources.c
    audio_car.c audio_sequence.c shape3d.c shape3d_resources.c shape3d_car.c
    shape3d_prerender.c shape3d_lines.c shape2d.c shape2d_blit.c video_pages.c
    hires.c hypervision.c frame_interpolation.c phantom_physics.c shape3d_hires.c
    shape2d_resources.c heapsort.c frame.c frame_adaptive.c skybox.c frame_overlay.c
    intro_render.c track_preview.c full_data.c full_strings.c full_tables.c)
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

message(STATUS "Building dependency-free C99 embedded stub library: restunts_embedded_stubs")