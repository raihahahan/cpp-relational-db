# Storage Layer Overview

The storage subsystem handles all persistent data management in the database. It abstracts away raw file operations and provides a layered interface for higher-level components.

The `/storage` folder contains the following layers:

```
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

| Layer             | Description                       | Responsibility                                   |
| ----------------- | --------------------------------- | ------------------------------------------------ |
| **DiskManager**   | Lowest-level storage abstraction. | Reads and writes fixed-size pages to disk files. |
| **BufferManager** | Caches pages in main memory.      | Manages pinned pages, dirty flags, and eviction. |

Each page is fixed-size (4 KB), representing a block of the database file on disk.

### File Layout Example

```
Database file: mydb.db
-----------------------------------------------
| Page 0 | Page 1 | Page 2 | Page 3 | Page 4 |
-----------------------------------------------
  0KB     8KB      16KB     24KB     32KB
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

## 3. BufferManager

The BufferManager sits above the DiskManager and maintains an in-memory cache of database pages (frames) to reduce direct disk I/O. It exposes a minimal API for fetching, releasing, marking, and flushing pages. Higher layers such as the file access (heap/index) and catalog systems interact with pages only through this interface.

### Responsibilities

- Cache a fixed number of pages in memory (`pool_`).
- Maintain a **page table** mapping `page_id => Frame*` for fast lookup.
- Manage **pin/unpin counts** to track page usage.
- Mark pages as **dirty** when modified, ensuring they are flushed before eviction.
- Manage a **free list** for unused frames and a **replacement policy** (Clock).
- Coordinate all physical reads/writes via `IDiskManager`.

### Public Interface

| Method                   | Description                                                                                                                                                                                                                                        |
| ------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`request(page_id)`**   | Fetches the page with the given `page_id`. If the page is already cached, returns its frame and increments its pin count. If not, evicts a victim (if needed), reads the page from disk into a free frame, pins it, and returns the frame pointer. |
| **`release(page_id)`**   | Unpins the page, decreasing its pin count. When the pin count reaches zero, the page becomes eligible for eviction.                                                                                                                                |
| **`mark_dirty(Frame*)`** | Marks a frame as dirty, indicating it must be written back to disk before eviction.                                                                                                                                                                |
| **`flush_all()`**        | Writes all dirty frames currently in the buffer pool back to disk.                                                                                                                                                                                 |

### Internal Components

| Member            | Purpose                                                                                                 |
| ----------------- | ------------------------------------------------------------------------------------------------------- |
| **`page_table_`** | Maps `page_id` to in-memory `Frame*` for O(1) lookups.                                                  |
| **`pool_`**       | Vector of preallocated frames representing the in-memory page pool.                                     |
| **`frame_ptrs_`** | Collection of raw pointers to frames, passed to the replacement policy for tracking.                    |
| **`free_list_`**  | Holds unallocated frames available for immediate use.                                                   |
| **`policy_`**     | Clock replacement policy implementing `IReplacementPolicy`. Responsible for selecting eviction victims. |
| **`disk_`**       | Pointer to the underlying `IDiskManager`, used for all disk reads/writes.                               |

### Private Helpers

| Method                            | Description                                                                                     |
| --------------------------------- | ----------------------------------------------------------------------------------------------- |
| **`evict()`**                     | Chooses a victim frame to evict using the replacement policy. Flushes it if dirty before reuse. |
| **`read(page_id, frame)`**        | Reads the specified page from disk into the given frame’s memory buffer.                        |
| **`flush(frame)`**                | Writes a dirty frame’s contents back to disk via `IDiskManager`.                                |
| **`pin(frame)` / `unpin(frame)`** | Increments or decrements a frame’s pin count. Pinned frames cannot be evicted.                  |

### Typical Workflow

```

User → request(page_id)
↓
If cached → return Frame* and pin
Else:
• Get frame from FreeList or evict victim via Clock
• disk_->ReadPage(page_id, frame.data)
• Insert (page_id → frame) into page_table_
• Pin frame and return pointer

User modifies frame
↓
mark_dirty(frame)
↓
release(page_id)
↓
If pin_count == 0 → frame eligible for replacement

Shutdown
↓
flush_all() → writes all dirty frames to disk

```

### Testing Guidelines

- **Page Fetch Test:** Request the same page twice; verify pin count increments and no duplicate load from disk.
- **Eviction Test:** Fill the pool, then request a new page to trigger Clock replacement. Confirm dirty victim is flushed.
- **Dirty Flag Test:** Modify a frame, call `mark_dirty()`, and ensure `flush_all()` persists changes to disk.
- **Free List Test:** Ensure that newly allocated frames are first drawn from `FreeList` before eviction occurs.

### Notes

- The BufferManager is the _only_ layer that directly interacts with the DiskManager.
- Higher layers (HeapFile, IndexFile, Catalog) should always acquire pages via `request()` and release them via `release()`.
- Correct pin/unpin discipline is critical: pinned pages must never be evicted.
- Replacement policy is pluggable — currently supports Clock via `ReplacementPolicyType::CLOCK`.
