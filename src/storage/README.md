# Storage Layer Overview

The storage subsystem handles all persistent data management in the database. It abstracts away raw file operations and provides a layered interface for higher-level components.

The `/storage` folder contains the following layers:

```
+-----------------------------+
|        File Layer           |
|   (Heap files, indexes)     |
+-------------+---------------+
              |
              v
+-----------------------------+
|       Buffer Manager        | in-memory page cache (Clock replacement)
|    - pin/unpin pages        |
|    - manage dirty pages     |
|    - evict when full        |
+-------------+---------------+
              |
              v
+-----------------------------+
|        Disk Manager         |
|    - read/write pages       |
|    - manage free space      |
|    - append new pages       |
+-------------+---------------+
              |
              v
+-----------------------------+
|       OS / File System      |
|     (actual .db file)       |
+-----------------------------+
```

Each layer builds on the one below it.

## 1. Architecture Overview

| Layer                      | Description                       | Responsibility                                                 |
| -------------------------- | --------------------------------- | -------------------------------------------------------------- |
| **DiskManager**            | Lowest-level storage abstraction. | Reads and writes fixed-size pages to disk files.               |
| **BufferManager**          | Caches pages in main memory.      | Manages pinned pages, dirty flags, and eviction.               |
| **FilesLayer (Heap File)** | Logical record storage layer.     | Organizes tuples inside pages using RIDs and slot directories. |

Each page is fixed-size (4 KB), representing a block of the database file on disk.

### File Layout Example

```
Database file: mydb.db
-----------------------------------------------
| Page 0 | Page 1 | Page 2 | Page 3 | Page 4 |
-----------------------------------------------
  0KB     4KB      8KB     12KB     16KB
```

Example operations:

- `ReadPage(3, buf)`: reads bytes [12KB, 16KB)
- `WritePage(4, buf)`: writes bytes [16KB, 20KB)
- `AllocatePage()`: returns next free page ID (e.g., 5)
- `DeallocatePage(1)`: marks page 1 as free

## 2. DiskManager

`DiskManager` is the lowest-level storage component
It is responsible for moving data between disk and memory, allocating new pages, and tracking free space.

### Responsibilities

| Method                              | Description                                                                                                                                                                                    |
| ----------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Constructor / Destructor**        | Opens (or creates) the database file in binary mode. Initializes internal state such as `next_page_id_`. Closes the file on destruction.                                                       |
| **`ReadPage(page_id, page_data)`**  | Reads one fixed-size page (4 KB) from disk into the given memory buffer. The offset is computed as `page_id * PAGE_SIZE`. If the file is shorter than expected, only partial data may be read. |
| **`WritePage(page_id, page_data)`** | Writes exactly one fixed-size page from memory to disk. The offset is computed as `page_id * PAGE_SIZE`. Flushes the stream to ensure data persistence.                                        |
| **`AllocatePage()`**                | Returns a new `page_id` for use. If there are free pages in `free_list`, it reuses one; otherwise, increments `next_page_id_`.                                                                 |
| **`DeallocatePage(page_id)`**       | Adds the specified page ID to the `free_list`, allowing it to be reused later. This does not physically remove the page from disk.                                                             |
| **`GetNumPages()`**                 | Returns the total number of pages currently stored in the file, computed as `file_size / PAGE_SIZE`. Used to initialize `next_page_id_`.                                                       |

### Notes

- The DiskManager operates on raw pages only. It does not understand tuples, schemas, or indices.
- It is the only layer that directly performs file I/O through `std::fstream`.
- Each page ID corresponds to one contiguous 4 KB region on disk.
- Higher layers (BufferManager, FilesLayer) should never directly access files — only through DiskManager.

### Testing Instructions

Unit tests for `DiskManager` are implemented using GoogleTest.
Each test creates a temporary file, allocates and writes pages, then verifies correctness.

Example tests:

- `AllocatePageTest`: checks initial allocation returns `page_id = 0`.
- `WriteBufferTest`: writes a 4 KB buffer to a page and verifies that the data matches after reading.
- `IdIncrementTest`: verifies that page IDs increment correctly on repeated allocations.
- `DeallocatePageTest`: ensures that freed page IDs are reused.

To run tests, use the root-level Makefile:

```bash
# Build project
make build

# Run all tests
make test

# Run DiskManager-specific tests
make test_disk_manager

# Clean build directory
make clean
```

## 3. BufferManager (TODO)

The BufferManager sits above the DiskManager. It maintains an in-memory cache of database pages to minimize disk I/O.

### Responsibilities

- Maintain a fixed number of in-memory **frames**, each storing a page.
- Keep a page table mapping `page_id -> frame index`.
- Manage pin counts and dirty flags for pages.
- Implement a replacement policy (Clock).
- Interface with `DiskManager` for page reads/writes.

## 4. FilesLayer (TODO)

The FilesLayer manages logical records inside physical pages. It builds on top of the BufferManager and implements tuple-level operations.

### Responsibilities

- Manage heap pages that store multiple tuples.
- Use line pointers (slot directory) to locate records within a page.
- Provide operations for inserting, reading, updating, and deleting tuples.
- Use Record IDs (RIDs) composed of `(page_id, slot_id)` to locate tuples.
- Work through BufferManager for all page access.
