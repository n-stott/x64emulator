# x64emulator

A toy x86-64 userspace emulator, for educational purposes.

## Features

* Emulation of most x86-64 instructions (in 64bit context), including SSE2, (S)SSE3 and SSE4.1.
* An optimizing JIT compiler
* A thin (linux) kernel/filesystem emulation layer, with a focus on keeping the host machine clean.
* A prototype timeline profiler