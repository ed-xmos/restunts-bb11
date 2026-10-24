# Embedded Stub Build

This opt-in profile builds the C99 game sources as a static library without
fetching SDL or the desktop audio, graphics, and filesystem dependencies:

```sh
cmake -S . -B out/embedded-stubs -DRESTUNTS_BUILD_EMBEDDED_STUBS=ON
cmake --build out/embedded-stubs --target restunts_embedded_stubs
```

Pass the embedded compiler or its toolchain file through the normal CMake
options. The profile uses C99, `-Os`, function sections, and data sections for
GCC/Clang. Link with dead-section elimination where supported.

For the XCORE-AI Explorer xcommon application profile, run `./build-xmos.sh`
after sourcing the XMOS environment, or pass the directory containing
`xcommon.cmake` to the script. Use `./build-xmos.sh --clean` for a clean build.
The root CMake file selects
`XCORE-AI-EXPLORER-800.xn`, reuses the same game sources, and adds a minimal C
entry point. This only establishes a build target; it does not yet bind the
board's display, sound, controls, timer, or FFS implementation.

To run the simulator with the repository's game data, launch it from the data
directory so relative DOS resource names resolve there:

```sh
cd stunts
xsim ../cmake/bin/restunts.xe
```

The library includes the original 1 MiB segmented-memory compatibility map.
`stubs.h` exposes the indexed 320x200 framebuffer and VGA palette, input
injection, a manually driven 10 ms timer tick, and a callback for legacy sound
driver commands. The sound callback is not MIDI: map the command/resource data
to the target's sound engine. The profile emits no sound unless a callback is
installed.

Install callbacks before calling `restunts_embedded_run(argc, argv)`. Call
`restunts_embedded_tick_10ms()` from a hardware timer at 100 Hz while the game
runs. The frame callback receives the indexed framebuffer and six-bit VGA
palette after the legacy backbuffer is presented; copy or consume the pixels
before returning if the display hardware cannot read the game memory directly.
Sound-event resource/data pointers are callback-scoped and must be consumed or
copied before returning. Key injection takes legacy BIOS-style key values;
held-key, mouse, and joystick state can be updated between ticks.

File handles use ISO C `stdio`. Directory enumeration returns no entries because
ISO C99 has no directory API; supply a target implementation of
`dos_file_find_first` and `dos_file_find_next` if the game should discover
tracks, cars, replays, or other files dynamically. The other OS/input/video
operations are minimal stubs and should be replaced or driven by the target.

SDL-specific enhanced artwork and animation loaders are omitted. Their stubs
leave the original indexed artwork path available. This profile is an
integration starting point, not a verified 400 KB final image or a complete
device port.