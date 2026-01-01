# C++ Relational Database

This repository implements a mini relational database system in modern C++. It is inspired by PostgreSQL and upon taking [CS3223 (Database Implementations)](https://nusmods.com/courses/CS3223/database-systems-implementation).

Technical write-up of this project: [Link](https://mraihan.dev/blog/Implementing-a-relational-database-from-scratch---Storage-Layer)

## Table of Contents

1. [Overview](#overview)
2. [Getting Started](#getting-started)
3. [Build & Test](#build--test)
4. [Project Roadmap](#project-roadmap)

## Overview

This database system is built as a layered architecture, where each layer abstracts a specific level of data management:

1. Storage Layer: Disk Manager, Buffer Manager, Slotted Page Organisation => raw bytes stored as pages (done)
2. Access Layer: Heap Files and Heap iteration => API to access pages stored in heap files (done)
3. Catalog Layer: Creates and provides metadata information for the database (i.e info about tables, attributes, types) (done)
4. Model Layer: Schema aware layer to create and insert rows into user tables in the database (done)
5. Query Evaluation Engine: SQL Parser, Query Optimiser, Query Executor, operators
6. Concurrency Control
7. Recoverability Manager

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

## Storage Layer Overview

The **storage subsystem** forms the foundation of the database.
It is divided into three layers:

| Component         | Description                                     |
| ----------------- | ----------------------------------------------- |
| **DiskManager**   | Handles raw page I/O and file operations.       |
| **BufferManager** | Manages in-memory pages and replacement policy. |
| **Slotted Page**  | Organisation of data within each page.          |

For detailed documentation, see the [storage/README.md](src/storage/README.md).

## Access Layer Overview

The **access subsystem** provides structured access to stored data.
It sits above the storage layer and below query execution.
Currently, only heap access is implemented. (Future work: B+ trees)

| Component        | Description                                           |
| ---------------- | ----------------------------------------------------- |
| **HeapFile**     | Append-only heap storage for variable-length records. |
| **HeapIterator** | Sequential scan over heap file records.               |
| **Record**       | Logical view of a stored tuple.                       |
| **RID**          | Record identifier (page id + slot id).                |

### Responsibilities

- Insert, update, and delete records
- Provide iterator-based access for scans
- Abstract page layout from higher layers

The access layer is restart-safe and operates purely on persisted data.

For detailed documentation, see the [access/README.md](src/access/heap/README.md).

## Catalog Layer Overview

The **catalog subsystem** stores and manages database metadata.
It describes what data exists and where it is stored.

| Component             | Description                                                     |
| --------------------- | --------------------------------------------------------------- |
| **Catalog**           | Entry point for metadata access and bootstrap logic.            |
| **TablesCatalog**     | Stores table-level metadata (table name, heap file, root page). |
| **AttributesCatalog** | Stores column metadata for each table.                          |
| **TypesCatalog**      | Stores built-in and user-defined data types.                    |
| **Catalog Codecs**    | Binary encoders/decoders for catalog rows.                      |

### Responsibilities

- Bootstraps system catalogs on first startup
- Loads catalog metadata from disk on restart
- Provides lookup APIs for tables, columns, and types
- Ensures catalog durability across restarts

Catalog data is persisted using heap files and reconstructed in-memory on startup.

For detailed documentation, see the [catalog/README.md](src/catalog/README.md).

## Database Server Overview

The **database server subsystem** manages database-level lifecycle and routing.
It provides a lightweight entry point for creating, opening, and deleting databases.

| Component    | Description                                       |
| ------------ | ------------------------------------------------- |
| **DbServer** | Manages database files and DiskManager instances. |

## Model Layer Overview

### Responsibilities

- Discover existing databases on startup
- Create new database files
- Open existing databases
- Cache active DiskManager instances
- Delete databases safely

For detailed documentation, see the [server/README.md](src/server/README.md).

## Model Layer Overview

The **model layer** acts as the logical bridge between low-level physical storage and high-level database abstractions. It manages how data is structured, aligned in memory, and presented to the system as tables and rows.

| Component        | Description                                                                     |
| ---------------- | ------------------------------------------------------------------------------- |
| **TableManager** | Global registry that caches opened table instances and resolves metadata.       |
| **UserTable**    | Logical handle for a table, enforcing schema and providing a row-based API.     |
| **DynamicCodec** | Serialisation engine that handles binary encoding with proper memory alignment. |
| **Relation**     | Base class providing shared physical interaction logic for all relational data. |

### Responsibilities

- **Metadata Orchestration**: Resolves human-readable table names into physical storage IDs using the Catalog.
- **Schema Enforcement**: Ensures that inserted data matches the types and positions defined in the table schema.
- **Data Alignment**: Automatically handles padding and alignment for various data types during serialization to ensure architecture compatibility.
- **Instance Caching**: Manages a shared cache of active tables to prevent redundant file handles and ensure data consistency.

### Data Representation

The model layer uses a flexible variant-based system to represent in-memory values:
`using Value = std::variant<uint32_t, std::string>;`

When data is persisted, the **DynamicCodec** transforms these variants into a compact, aligned binary format suitable for storage on disk.

For detailed documentation, see the [model/README.md](src/model/README.md).

## Project Roadmap

| Phase | Layer                       | Description                                                  |
| ----- | --------------------------- | ------------------------------------------------------------ |
| 1     | **Storage**                 | Implement DiskManager, BufferManager                         |
| 2     | **Access**                  | Heap file, B+-tree index, Hash index                         |
| 3     | **Execution**               | Query operators and execution plans (scan, join, aggregate). |
| 4     | **Concurrency**             | Locking, transaction management, and isolation.              |
| 5     | **Recovery**                | Write-ahead logging and crash recovery.                      |
| 6     | **Networking & SQL Parser** | Client-server interface and query parsing layer.             |

## License

This project is released under the [MIT License](LICENSE).
