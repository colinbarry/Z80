# Zilog Z80 Emulator in C99

The Zilog Z80 was a popular 8-bit microprocessor based on the Intel 8080 chip.
It is backwards compatible with the Intel chip, but adds a large number of
additional opcodes and features.

This project is an attempt to accurately emulate the processor, which it does,
including undocumented features (which some programs do rely upon). 

## Installation

The recommended way to build against the library is with CMake 3.14 or later:


```cmake
cmake_minimum_required(VERSION 3.14)

project(Spectrum)

include(FetchContent)
FetchContent_Declare(
    z80
    GIT_REPOSITORY https://github.com/colinbarry/z80 
    GIT_TAG main)
FetchContent_MakeAvailable(z80)

add_executable(Spectrum spectrum.c)
target_link_libraries(Spectrum z80)
```

## Testing

Z80 comes with two test suites:

- Frank Cringle's Z80 Instruction Set Exerciser (zexdoc/zexall) computes the
  checksum of the CPU's flags after executing each opcode, and compares the
  results to values taken from real Z80 hardware.

- The FUSE emulation project (https://fuse-emulator.sourceforge.net) supplies
  over 1000 short assembly programs, and compares the expected with the actual
  result. This uses Lua to convert the FUSE tests file into a .c source file.

Run the tests using `ctest`:

```bash
cmake -B ./build
cmake --build ./build
ctest --test-dir ./build
```

