# Disk Manager

The `DiskManager` is the lowest-level storage component responsible for reading and writing fixed-size database pages to a single on-disk file. It provides raw page-oriented I/O and page allocation services on top of a filesystem-backed binary file. All higher-level storage components (BufferManager, access methods, heap files, indexes) rely on the DiskManager for durable persistence.

This module abstracts away the mechanics of file I/O so that the rest of the system interacts only in terms of page IDs and fixed-size byte buffers.

## 1. Responsibilities

The `DiskManager` implements:

1. Fixed-size page reads
2. Fixed-size page writes
3. Page allocation
4. Page deallocation
5. Tracking of next available page
6. Ensuring the backing file exists and is opened in the correct mode
7. Exposing the number of pages currently stored in the file

It does not perform buffering, caching, eviction, or concurrency control. These concerns belong to higher layers, primarily the `BufferManager`.

## 2. Page Model

All physical pages have the same size:

```
db::config::PAGE_SIZE  // e.g., 4096 bytes
```

The file layout is logical and simple:

```
[offset = page_id * PAGE_SIZE] → page contents
```

Thus:

- `page_id = 0` is stored at offset `0`
- `page_id = 1` is stored at offset `PAGE_SIZE`
- `page_id = 2` is stored at offset `2 * PAGE_SIZE`
- and so on

This enables O(1) seeking and I/O for any page.

## 3. Components

### 3.1 DiskManager (`disk_manager.h` / `disk_manager.cpp`)

Example interface:

```cpp
class DiskManager {
public:
    explicit DiskManager(const std::string &db_file);
    ~DiskManager();

    void ReadPage(page_id_t page_id, char* page_data);
    void WritePage(page_id_t page_id, const char* page_data);

    page_id_t AllocatePage();
    void DeallocatePage(page_id_t page_id);

    int GetNumPages() const;

private:
    std::fstream db_io_;
    std::vector<page_id_t> free_list;
    page_id_t next_page_id_;
};
```

The DiskManager uses a `std::fstream` opened in binary read/write mode for all I/O operations.

## 4. High-Level Contracts

### 4.1 Construction

```cpp
DiskManager(const std::string &db_file)
```

Contract:

1. Attempts to open the database file in read/write binary mode.
2. If the file does not exist, it is created.
3. Determines the current number of pages via `GetNumPages()`.
4. Sets `next_page_id_` to the first unallocated page.

A newly created database file has zero pages.

### 4.2 ReadPage(page_id, buffer)

```cpp
void ReadPage(page_id_t page_id, char* page_data)
```

Contract:

- Computes offset: `page_id * PAGE_SIZE`
- Seeks to that offset using `seekg`
- Reads exactly `PAGE_SIZE` bytes into `page_data`
- Caller must ensure `page_data` points to a buffer of size ≥ PAGE_SIZE
- Does not modify `page_id`, pin counts, or metadata (handled by BufferManager)

ReadPage does not perform bounds checking; the caller—typically the BufferManager—is responsible for ensuring the page exists.

### 4.3 WritePage(page_id, buffer)

```cpp
void WritePage(page_id_t page_id, const char* page_data)
```

Contract:

- Computes offset: `page_id * PAGE_SIZE`
- Writes exactly `PAGE_SIZE` bytes at that offset
- Calls `flush()` to commit changes to disk
- Does not allocate or free pages

This operation is idempotent: overwriting an existing page is allowed.

### 4.4 AllocatePage()

```cpp
page_id_t AllocatePage()
```

Contract:

- If `free_list` is not empty, returns a recycled page ID from it.
- Otherwise, returns `next_page_id_` and increments it.
- Does **not** modify the file size or write any data; allocation is logical only.
- Physical file growth occurs only when the page is first written to by WritePage.

This is the only method that produces new valid page IDs.

### 4.5 DeallocatePage(page_id)

```cpp
void DeallocatePage(page_id_t page_id)
```

Contract:

- Pushes the page ID onto `free_list`
- The page may later be reassigned by AllocatePage()
- Does not zero or rewrite the page on disk (higher layers decide the lifecycle)
- Does not shrink the underlying file

Deallocation does not guarantee secure erasure or file compaction.

---

### 4.6 GetNumPages()

```cpp
int GetNumPages() const
```

Contract:

- Seeks to file end using `seekg(0, end)`
- Divides file size by `PAGE_SIZE`
- Returns number of pages currently stored in the file
- Does not account for free_list or deallocated pages

This value is used to initialize `next_page_id_` on startup.

## 5. Interaction with BufferManager

The DiskManager does **not** manage in-memory frames or caching.
Instead, the BufferManager:

- Chooses when to read a page from disk:

  ```
  DiskManager::ReadPage(pid, frame->data)
  ```

- Chooses when to write a dirty page back to disk:

  ```
  DiskManager::WritePage(pid, frame->data)
  ```

- Uses `AllocatePage()` to create new pages in heap or index structures
- Uses `DeallocatePage()` when higher-level structures release a page
- Does not directly access fstream or file offsets

DiskManager is purely a persistence layer; it knows nothing about:

- Pin counts
- Replacement policies
- Dirty tracking
- Frame structures
- Free frames in the buffer pool

All these belong to the BufferManager or higher layers.

## 6. Summary

The DiskManager provides:

- Low-level page I/O to a single backing file
- Page-based reads and writes
- Logical allocation and deallocation of pages
- Automatic file creation
- Tracking of the next available page ID
