# Heap File & Heap Iterator

This module implements a **heap-organised file** and a **sequential heap scan iterator** on top of the storage layer.

It is responsible for record-level access on top of pages managed by the buffer manager.

The design follows classic DBMS architecture (Postgres-style heap + scan), with strict pin/unpin discipline and explicit dirty tracking.

## Overview

```
+----------------------+
| HeapIterator         |
|----------------------|
| Sequential scan:     |
| 1. Slot-by-slot      |
| 2. Page-by-page      |
+---------+------------+
          |
          v
+-------------------+
| HeapFile          |
|-------------------|
| Insert / Get      |
| Update / Delete   |
| Page chaining     |
+---------+---------+
          |
          v
+-------------------+
| SlottedPage       |
|-------------------|
| Slot directory    |
| Variable records  |
+---------+---------+
          |
          v
+-------------------+
| BufferManager     |
| DiskManager       |
+-------------------+
```

## HeapFile

### Responsibility

`HeapFile` provides **record-oriented access** on top of heap pages:

- Inserts records into the first page with available space
- Chains pages via `next_page_id`
- Retrieves records by `(page_id, slot_id)`
- Updates and deletes records in-place
- Manages heap bootstrap (first page creation)

### Page Layout

Each heap page is a slotted page with a small header:

```cpp
struct HeapPageHeader {
    page_id_t next_page_id;
};
```

The remaining space is managed by `SlottedPage`.

### Key Invariants

- Every call to `BufferManager::request()` has exactly one matching `release()`
- Pages modified by heap operations are explicitly marked dirty
- No page pointers survive an unpin
- Page chaining is updated atomically (link old => new)

### Public API

```cpp
class HeapFile {
public:
    std::optional<Record> Get(const RID& rid);
    std::optional<RID> Insert(const char* data, size_t len);
    bool Update(const char* new_data, size_t len, const RID& rid);
    bool Delete(const RID& rid);

    HeapIterator begin();
    HeapIterator end();
};
```

### Insert Semantics

1. If heap is empty, allocate and initialise the first page
2. Traverse pages via `next_page_id`
3. Attempt insertion into each page
4. If no page fits:

   - Allocate a new page
   - Link it from the last page
   - Insert into the new page

All structural changes are marked dirty.

## HeapIterator

### Responsibility

`HeapIterator` implements a **sequential scan** over a heap file:

- Traverses pages in chain order
- Iterates slots within each page
- Skips deleted slots
- Is eviction-safe and buffer-safe
- Provides snapshot semantics for slot iteration

### Iterator Semantics

- `HasNext()` returns whether a next record exists
- `Next()` returns the next record and advances
- `begin()` returns a scan starting at the first page
- `end()` returns a sentinel iterator with no remaining records

### Buffer Manager Interaction

- Pages are pinned only during slot inspection
- Pages are always released before advancing
- No long-lived pins
- Safe under aggressive eviction

### Public API

```cpp
class HeapIterator {
public:
    bool HasNext();
    Record Next();

    Record operator*();
    HeapIterator& operator++();

    bool operator==(const HeapIterator& other) const;
    bool operator!=(const HeapIterator& other) const;
};
```

## Correctness Guarantees

The combined `HeapFile` + `HeapIterator` implementation guarantees:

- No buffer pin leaks
- No use-after-unpin
- No duplicate records during scan
- Correct behavior under eviction pressure
- Deterministic scan order

These guarantees are enforced by unit tests.

## Tests

The following test categories exist:

### HeapFile Tests

- Insert + Get
- Update
- Delete
- Multi-page insertion
- Eviction safety

### HeapIterator Tests

- Empty heap scan
- Single-page scan
- Multi-page scan
- Skipping deleted records
- Eviction stress test
- Snapshot correctness

Failing any of these indicates a violation of buffer or iterator invariants.

## Design Notes

- Explicit dirty marking is required; modifying page memory does not imply persistence
- Iterator logic does not hold pins across calls
- HeapFile and HeapIterator are intentionally decoupled
- This module is designed to plug directly into a sequential scan executor

## Future Work

- Free Space Map (FSM)
- Visibility metadata (MVCC)
- Concurrent scans
- Index scans
- Predicate pushdown
