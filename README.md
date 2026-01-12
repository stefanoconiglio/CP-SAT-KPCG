# CP-SAT (C++ Port)

Solve knapsack instances with conflicts using OR-Tools CP-SAT.
Includes a Python reference script (`CP-SAT.py`) and the C++ port (`CP-SAT.cpp`).

## Requirements

- Linux
- CMake, g++, make, git
- C++17 (required by OR-Tools)

Install build tools on Ubuntu:

```sh
sudo apt-get install -y git cmake g++ make curl unzip
```

## Build OR-Tools locally (recommended)

This repo includes a script that downloads and builds OR-Tools into
`./.local/or-tools`.

```sh
make ortools
```

You can pin a version:

```sh
ORTOOLS_GIT_REF=v9.9 make ortools
```

## Build the solver

```sh
make build
```

## Download instances

```sh
make download
make unzip
```

## Run

```sh
make run
```

To skip downloads when you already have the data:

```sh
./build/CP-SAT --no-download
```

## Using a system OR-Tools install

If you already have OR-Tools installed, set `ORTOOLS_ROOT` to the install
prefix before building:

```sh
export ORTOOLS_ROOT=/path/to/or-tools
make build
```

## Python script

The original Python version is available at `CP-SAT.py`. It can download data,
set up a local venv for OR-Tools, and solve the instances.
