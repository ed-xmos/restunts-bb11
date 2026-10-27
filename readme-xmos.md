# Embedded and XMOS Ports

## Embedded C99 Stub Profile

Build the dependency-free embedded stub library with the local C compiler:

```sh
tools/scripts/build-embedded-stubs.sh
```

To use an embedded compiler, pass its CMake toolchain file and use a separate build directory:

```sh
tools/scripts/build-embedded-stubs-toolchain.sh /path/to/your-toolchain.cmake
```

Both commands produce `librestunts_embedded_stubs.a` in the selected build directory. This strict C99 static library does not include SDL; firmware provides the final entry point and links the archive. See [the embedded profile guide](docs/embedded-stubs.md) for its framebuffer, input, timer, sound-event, and filesystem interfaces.

## XCORE-AI Explorer Build

For the XCORE-AI Explorer xcommon build, run the root-level helper after `x15` has sourced the XMOS environment:

```sh
./build-xmos.sh
```

Alternatively, pass the directory containing `xcommon.cmake`:

```sh
./build-xmos.sh /path/to/xmos/cmake
```

Add `--clean` to clean generated outputs before building:

```sh
./build-xmos.sh --clean
```

This profile uses the same game/stub sources and the `XCORE-AI-EXPLORER-800.xn` target. It is a compile/link starting point; board-specific graphics, audio, timer, controls, and FFS wiring still need to be supplied.

## XMOS ExtMem Car-Rendering Investigation

### Symptom

On XMOS, a red quadrilateral can extend from the car body. It is absent in the native embedded build. Changing XMOS optimization flags, holding wheel steering and suspension neutral, skipping wheel-face rendering, and skipping triangle polygons did not remove the artifact.

### Finding

The strongest evidence points to corruption while reading compressed car resources into the XMOS legacy-memory arena, which is placed in external memory. The on-disk `STCOUN.P3S` is 5,338 bytes and hashes to `43802eec`; native `fread` and decompression produce the expected 6,994-byte resource archive with seven entries. XMOS runs were intermittent: some produced that valid archive, while others produced an invalid header and failed to locate `car0`.

In a failing XMOS run, bytes read directly into the legacy-memory destination differed from a reference read into ordinary internal memory at offsets 2482, 2706, 4242, and 5106. At each offset, bit 7 was cleared. Chunk-level checks showed staged reads matched the reference, while copying the same bytes into the legacy-memory destination could reproduce the differences. The issue persisted without the timer and graphics jobs. Byte, halfword, word, `memcpy`, and fence experiments did not resolve it, so the exact hardware or address-lane mechanism remains undetermined.

When a malformed package was used, car model vertex counts could be wrong; for example, native reported `car0/car1/car2` counts of 191/118/18, while an XMOS run reported 191/190/71. This provides a plausible path from resource corruption to malformed car geometry, but the ExtMem write/read behavior still needs a minimal hardware test before calling the underlying mechanism fixed.

### Next Check

Use a small board test that writes and reads known byte patterns in `.ExtMem.bss` at each alignment and access width. Compare those results with internal-memory buffers and inspect the generated XMOS accesses and external-memory configuration.
