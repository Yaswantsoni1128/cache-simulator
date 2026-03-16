# Cache Simulator

A small, readable CPU cache simulator written in C++ with a Python reference implementation.

## Overview

`cache-simulator` models how a CPU cache responds to memory accesses and reports hits, misses, and a miss classification breakdown. It supports configurable cache size, block size, associativity, and replacement policies (LRU and FIFO). A Python version is included so you can run the simulator without a C++ toolchain.

## Repository layout

- `include/` - public headers for the C++ simulator
- `src/` - C++ source files (driver and implementation)
- `py/` - Python simulator (`cache_simulator.py`) — runnable without compilation
- `examples/` - example input traces
- `Makefile` / `build.bat` - build helpers for Unix/Windows
- `README.md` - this file

## Features

- Configurable cache size and block size
- Set associativity (n-way set-associative)
- Replacement policies: LRU (mandatory) and FIFO
- Miss classification: compulsory, conflict, capacity (uses an ideal fully-associative LRU for classification)
- Optional compare mode to run two policies on the same trace
- Step-by-step mode to inspect state per-access

## Input format

Each non-empty line in the trace file should contain an operation and an address. Examples:

```
R 0x1000
W 4096
```

Addresses may be hex (`0x...`) or decimal.

## Quick start — Python (no build required)

Run the included Python simulator (requires Python 3.7+):

```powershell
python py\cache_simulator.py -i examples\input.txt -c 16384 -b 64 -a 4 -r lru
```

Options:
- `-i <file>` : input trace file (required)
- `-c <bytes>` : cache size in bytes (default 16384)
- `-b <bytes>` : block size in bytes (default 64)
- `-a <n>` : associativity (default 4)
- `-r <lru|fifo>` : replacement policy (default lru)
- `-s` : step-by-step mode
- `--compare` : run both policies and compare results

## Quick start — C++ (optional)

If you have a C++ toolchain (g++, clang++, or MSVC), build and run the C++ simulator:

PowerShell (g++):

```powershell
cd D:\cache-simulator
g++ -std=c++17 src\*.cpp -Iinclude -O2 -o cache_simulator.exe
.\cache_simulator.exe -i examples\input.txt -c 16384 -b 64 -a 4 -r lru
```

Or use the provided helper:

```powershell
.\build.bat
```

## Example output (short)

```
=== Results ===
Hits: 6
Misses: 10
Hit rate: 37.5000%
Miss classification: compulsory=10, conflict=0, capacity=0

Final cache state:
Cache state: (sets=64, associativity=4)
Set 0: [tag=0x0,t=8] [tag=0x1,t=13] [tag=0x2,t=7] [tag=0x3,t=10]
... (truncated)
```

## Development notes

- The miss classification uses a separate fully-associative LRU of equal capacity to distinguish compulsory vs capacity vs conflict misses.
- The simulator currently models read/write accesses identically (no dirty bits / write-back modelling).
- The code is designed to be readable and easy to extend (add policies, write buffering, or cache coherence later).

## License

MIT

## Contributing

Create a branch, add tests or improvements, and open a pull request.