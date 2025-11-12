# x64emulator

A toy x86-64 userspace emulator, for educational purposes.

## Features

* Emulation of most common x86-64 instructions (ring-3 and 64bit context only), including SSE2, (S)SSE3 and SSE4.1.
* A simple optimizing JIT compiler.
* A thin (linux) kernel/filesystem emulation layer, with a focus on keeping the host machine clean.
* A prototype timeline profiler.

## How to compile
Clone this repository and build the emulator, profileviewer and tests with

    $ git clone --recursive https://github.com/n-stott/x64emulator.git
    $ cd x64emulator
    $ cmake -B build
    $ cmake --build build

Here are the options that can be given to cmake:
* `-DMULTIPROCESSING=ON` to enable multi-core support
* `-DSSE3=ON`, `-DSSSE3=ON`, `-DSSE41=ON` to enable SSE3, SSSE3 and SSE4.1 support respectively

## How to use
To run a program `program`, you just need to use `emulator program`. More generally, running `emulator program arg0 arg1` runs `program` invoked with arguments `arg0`, `arg1` in the emulator.

The emulator executable provides several runtime options:
* `emulator --syscalls command` to display all syscalls that are performed
* `emulator --nojit command` to disable the JIT compiler
* `emulator --nojitchaining command` to disable jumping between basic blocks within jitted code
* `emulator -O0 command` to disable JIT optimizations
* `emulator -O1 command` to enable all JIT optimizations (default)
* `emulator --jitstats level command` to retrieve some statistics from the jit (higher level means more information)
* `emulator --shm command` to enable shared memory syscalls (may be required by some programs)
* `emulator --mem X command` to provide X MB of virtual memory to the emulator (minimum of 256MB require)
* `emulator --profile command` to save a profile of the command (for short running programs only). This disables the JIT automatically
* `emulator -j N command` to provide N cores to the emulator when compiled in MULTIPROCESSING mode.

## Profile viewer
After running the emulator with `--profile`, a json file `output.json` which stores profiling events is created. This file can be opened with `profileviewer` (or `profileviewer newname.json` if the file has been renamed) to show the timeline.

The profileviewer currently only supports displaying the timeline for the main thread.