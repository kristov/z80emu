# z80emu - graphical emulator for a Z80 CPU

I wanted a terminal/ncurses based interface to a Z80 chip emulator without being attached to any specific microcomputer implementation (ie: ZX Spectrum). This is so I can see at the register and instruction level what the chip is doing and use it as a pausable and modifiable interpreter.

There are three panels:

1. Left register panel - this shows the values of the registers, program counter and various other chip internals in decimal, hex and binary.
2. Middle panel dissasembly - this shows a dissasembly of the instruction just executed, and the instruction about to be executed.
3. Right panel memory - This is the entire 64K addressable memory as visible by the CPU. I want to add features where you can bank in and out binary files to sections of memory while the CPU is running. The address of the *next* instruction in the program counter is highlighted.

## Features to add

1. Be able to load binary ROM files at runtime and specify where in memory space to mount them.
2. Full dissasembly window scrollable.
3. Scrollable memory space.
4. Insert or delete binary or assembly instructions into memory, shifting other instructions up or down.
5. Full history of all registers for debugging.

## Building

Probably only compiles on Linux. Depends on libncursesw. Need a clone of the z80ex github repository:

```sh
make clone-emulator
make
./z80emu
```
