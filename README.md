# CP-SAT

Solve knapsack instances with conflicts using OR-Tools CP-SAT. Includes a Python
script (`CP-SAT.py`) and a ported C++ code (`CP-SAT.cpp`).

## Requirements

### Python

- Python 3.8+ (compatible with OR-Tools Python)

### C++

- Linux
- CMake, g++, make, git
- C++17 (required by OR-Tools)

Install build tools on Ubuntu:

```sh
sudo apt-get install -y git cmake g++ make curl unzip
```

## Python (quick start)

The Python script can download the instances, install OR-Tools into a local
venv, and run the solver.

```sh
python3 CP-SAT.py
```

Skip downloads and/or the venv install if you already have them:

```sh
python3 CP-SAT.py --no-download --no-install
```

## C++ build (Linux)

### Build OR-Tools locally (recommended)

This repo includes a script that downloads and builds OR-Tools into
`./.local/or-tools`.

```sh
make ortools
```

You can pin a version:

```sh
ORTOOLS_GIT_REF=v9.9 make ortools
```

### Build the solver

```sh
make build
```

### Download instances

```sh
make download
make unzip
```

### Run

```sh
make run
```

To skip downloads when you already have the data:

```sh
./build/CP-SAT --no-download
```

### Using a system OR-Tools install

If you already have OR-Tools installed, set `ORTOOLS_ROOT` to the install
prefix before building:

```sh
export ORTOOLS_ROOT=/path/to/or-tools
make build
```
