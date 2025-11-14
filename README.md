# C++ Relational Database

This repository implements a mini relational database system in modern C++. It is inspired by PostgreSQL and upon taking [CS3223 (Database Implementations)](https://nusmods.com/courses/CS3223/database-systems-implementation)

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Getting Started](#getting-started)
4. [Build & Test](#build--test)
5. [Project Roadmap](#project-roadmap)
6. [Repository Structure](#repository-structure)

## Overview

This database system is built as a layered architecture, where each layer abstracts a specific level of data management:

1. Query Layer: Parses, optimizes, and executes SQL statements.
2. Execution Layer: Handles query operators (scan, join, aggregate).
3. Buffer Manager: Caches and manages in-memory pages.
4. Storage Layer: Reads/writes pages to disk (raw I/O).

Each layer interacts only with the one directly below it, following clean modular boundaries.

Below is a simplified high-level view of the entire system stack:

![architecture.png](docs/architecture.png)

## Getting Started

### Prerequisites

- **C++20 or later** (tested with GCC 12+ / Clang 15+)
- **CMake 3.14+**
- **GoogleTest** (fetched automatically via CMake)
- **Make** for simplified build commands

## Build & Test

You can use either CMake directly or the provided Makefile shortcuts.

### Using Makefile

```bash
# Build the project
make build

# Run all tests
make test

# Run DiskManager-specific tests
make test_disk_manager

# Clean build artifacts
make clean
```

### Manual CMake Workflow

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
ctest --output-on-failure
```

## Repository Structure

```

📁 cpp-relational-db
├── 📁 include
│ ├── 📁 common
│ ├── 📁 concurrency
│ ├── 📁 config
│ │ └── config.h
│ ├── 📁 execution
│ ├── 📁 network
│ ├── 📁 parser
│ ├── 📁 recovery
│ └── 📁 storage
│ └── disk_manager.h
│
├── 📁 src
│ ├── 📁 concurrency
│ ├── 📁 execution
│ ├── 📁 network
│ ├── 📁 parser
│ ├── 📁 recovery
│ └── 📁 storage
│ ├── disk_manager.cpp
│ └── README.md
│
├── 📁 tests
│ └── 📁 storage
│ └── test_disk_manager.cpp
│
├── .gitignore
├── CMakeLists.txt
├── LICENSE
├── Makefile
└── README.md

```

## Storage Layer Overview

The **storage subsystem** forms the foundation of the database.
It is divided into three layers:

| Component                  | Description                                         |
| -------------------------- | --------------------------------------------------- |
| **DiskManager**            | Handles raw page I/O and file operations.           |
| **BufferManager**          | Manages in-memory pages and replacement policy.     |
| **FilesLayer (Heap File)** | Stores tuples inside pages using Record IDs (RIDs). |

For detailed documentation, see the [storage/README.md](src/storage/README.md).

## Project Roadmap

| Phase | Layer                       | Description                                                       | Status      |
| ----- | --------------------------- | ----------------------------------------------------------------- | ----------- |
| 1     | **Storage**                 | Implement DiskManager, BufferManager, and Heap File organization. | In Progress |
| 2     | **Execution**               | Add query operators and execution plans (scan, join, aggregate).  | Planned     |
| 3     | **Concurrency**             | Introduce locking, transaction management, and isolation.         | Planned     |
| 4     | **Recovery**                | Implement write-ahead logging and crash recovery.                 | Planned     |
| 5     | **Networking & SQL Parser** | Add a client-server interface and query parsing layer.            | Planned     |

## License

This project is released under the [MIT License](LICENSE).
