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

## XMOS Hardware Targets

The default hardware target is `XK-EVK-XU416`; it uses XTC 0.3.6. Source that
installation's `SetEnv`, then run the root-level helper:

```sh
source /path/to/XTC/0.3.6/SetEnv
./build-xmos.sh
```

The helper passes the selected hardware target to CMake on every run and uses a
default build directory named for both the target and XTC installation. This
keeps CMake caches separate when switching either one. Set
`RESTUNTS_XMOS_BUILD_DIR` to override that directory.

Alternatively, pass the directory containing `xcommon.cmake`:

```sh
./build-xmos.sh /path/to/xmos/cmake
```

Add `--clean` to clean generated outputs before building:

```sh
./build-xmos.sh --clean
```

To build the `XCORE-AI-EXPLORER-800.xn` target, use its XTC 15.3.1 environment
and a separate build directory:

```sh
RESTUNTS_XMOS_HW_TARGET=XCORE-AI-EXPLORER-800.xn \\
./build-xmos.sh --clean /path/to/XTC/15.3.1/build/xcommon_cmake
```

`RESTUNTS_XMOS_HW_TARGET` selects the xcommon hardware target; it does not select
the compiler. The compiler/toolchain comes from the sourced XMOS environment or
the directory containing `xcommon.cmake`. This profile uses the same game/stub
sources and is a compile/link starting point; board-specific graphics, audio,
timer, and controls still need to be supplied.

The XU416 target uses XTC 0.3.6-compatible flags and does not place the
legacy-memory arena in external DDR. The image compiles and links with the
supplied toolchain; runtime behavior on the board has not yet been validated.

## Read-Only Flash Filesystem

`RESTUNTS_XMOS_USE_FFS` defaults to `ON` for `XK-EVK-XU416-800.xn`.
It uses the plain-C Petit FatFs core from the sibling `lib_filesystem` checkout
and the XTC 0.3.6 flash libraries. Other targets retain host stdio by default;
the flash backend currently supports the XU416 target only.

The flash must contain a FAT12 image in its xflash data partition. After building
the game, run `./flash-ffs-xmos.sh` to create `cmake/bin/ffs_image.img` and flash
it alongside `cmake/bin/restunts.xe`. This includes the `stunts` files except
BMPs and `frames.ffconcat`, storing their basenames in the FAT root. The script
defaults to a 1 MiB boot partition; set `RESTUNTS_XMOS_BOOT_PARTITION_SIZE` to
override its size in bytes. This replaces the board's factory image and data
partition. The game discovers the data partition address rather
than assuming a fixed boot partition size. An image already validated with the
VX4 filesystem comparison test can be reused when loading the game with xrun.

The backend mounts lazily on the first file access and allows one open file.
It supports reads, DOS seek/tell, case-insensitive filenames, and exact-name
file discovery. A second open is rejected with a diagnostic without replacing
the active file. Wildcard discovery remains unsupported, as in the stdio stub.
Game saves are skipped, and ghost replay selection is disabled because it
requires a writable temporary file. Direct create/write/remove calls fail;
they never modify flash. Diagnostic frame capture still uses host stdio.

Build with the default flash backend:

```sh
./build-xmos.sh
```

Restore the host-stdio backend for comparison:

```sh
RESTUNTS_XMOS_USE_FFS=OFF ./build-xmos.sh
```

Re-enable it with `RESTUNTS_XMOS_USE_FFS=ON`. The helper forwards these settings
to the CMake cache; omitting an override retains a previously cached value.
Set `RESTUNTS_XMOS_FILESYSTEM_DIR` to the library directory containing
`src/file_systems/petit_fatfs` if the checkout is elsewhere.

## XMOS ExtMem Car-Rendering Investigation

### Symptom

On XMOS, a red quadrilateral can extend from the car body. It is absent in the native embedded build. Changing XMOS optimization flags, holding wheel steering and suspension neutral, skipping wheel-face rendering, and skipping triangle polygons did not remove the artifact.

### Finding

The strongest evidence points to corruption while reading compressed car resources into the XMOS legacy-memory arena, which is placed in external memory. The on-disk `STCOUN.P3S` is 5,338 bytes and hashes to `43802eec`; native `fread` and decompression produce the expected 6,994-byte resource archive with seven entries. XMOS runs were intermittent: some produced that valid archive, while others produced an invalid header and failed to locate `car0`.

In a failing XMOS run, bytes read directly into the legacy-memory destination differed from a reference read into ordinary internal memory at offsets 2482, 2706, 4242, and 5106. At each offset, bit 7 was cleared. Chunk-level checks showed staged reads matched the reference, while copying the same bytes into the legacy-memory destination could reproduce the differences. The issue persisted without the timer and graphics jobs. Byte, halfword, word, `memcpy`, and fence experiments did not resolve it, so the exact hardware or address-lane mechanism remains undetermined.

When a malformed package was used, car model vertex counts could be wrong; for example, native reported `car0/car1/car2` counts of 191/118/18, while an XMOS run reported 191/190/71. This provides a plausible path from resource corruption to malformed car geometry, but the ExtMem write/read behavior still needs a minimal hardware test before calling the underlying mechanism fixed.

A later reproduction fails after frame 392 while loading `SDCRED.PES`. A debug trace saw an impossible declared resource size of 2,164,260,609 bytes and a shape count of 32,896 before reading the first shape header; the trace then faulted while accessing that header. This is consistent with the same resource corruption, though the SD CRED bytes have not yet been compared against a native reference.

### Next Check

Use a small board test that writes and reads known byte patterns in `.ExtMem.bss` at each alignment and access width. Compare those results with internal-memory buffers and inspect the generated XMOS accesses and external-memory configuration.
