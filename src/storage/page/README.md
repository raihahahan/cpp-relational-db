# Slotted Page

## Overview

`SlottedPage` implements a **slotted page organisation**, a layout to store **variable-length records** inside a fixed-size page.

It provides a **zero-copy view** over raw page bytes (`char[PAGE_SIZE]`) and supports inserting, reading, updating, and deleting records using a slot directory.

## Page Layout

Each page is a fixed-size byte array (`PAGE_SIZE`) with the following layout:

```
+------------------------+  <- page start (low address)
| PageHeader             |
| - num_slots            |
| - free_space_offset    |  grows downward
+------------------------+
| SlotDirectory[]        |  grows upward
| (offset, length)       |
+------------------------+
|        FREE SPACE      |
+------------------------+
| Record Data            |  grows downward
+------------------------+  <- page end (high address)
```

### Growth Directions

- **Slot directory grows upward**
  Slots are appended sequentially after the page header.

- **Record data grows downward**
  Records are written from the end of the page backward.

The page is full when the slot directory and record data regions collide.

## Core Data Structures

### PageHeader

Stored at the beginning of the page.

```cpp
struct PageHeader {
    uint16_t num_slots;
    uint16_t free_space_offset;
};
```

- `num_slots`: number of slots in the slot directory
- `free_space_offset`: byte offset to the start of record data (from page start)

All metadata is stored as **offsets**, not pointers, to ensure correctness across disk persistence.

### Slot

Each slot describes one record.

```cpp
struct Slot {
    uint16_t offset;
    uint16_t length;
};
```

- `offset`: byte offset of record data from page start
- `length`: length of the record in bytes

  - `length == 0` indicates a logically deleted record

## Design Principles

- **View-based design**
  `SlottedPage` does not own memory. It wraps a `char*` pointing to page data.

- **Zero-copy access**
  Reads return `std::span<const char>` pointing directly into the page buffer.

- **Stable slot IDs**
  Slots are never physically removed. Deletion is logical.

- **Append-only allocation**
  Free space is consumed monotonically. Fragmentation is allowed.

- **No compaction / reuse (yet)**
  Space from deleted or moved records is not reclaimed.

## API Overview

### Initialisation

```cpp
static void Init(char* page_data);
```

Initializes a **new page**:

- `num_slots = 0`
- `free_space_offset = PAGE_SIZE`

Called exactly once when a page is first allocated.

### View Creation

```cpp
static SlottedPage FromBuffer(char* page_data);
```

- Creates a lightweight view over an existing page buffer.
- Used whenever a page is fetched from the buffer manager.

### Insert

```cpp
std::optional<uint16_t> Insert(const char* data, size_t len);
```

- Appends a new record to the page
- Allocates space for:

  - record bytes
  - one slot entry

- Returns the allocated `slot_id` on success
- Returns `nullopt` if insufficient space

### Get (Read)

```cpp
std::optional<std::span<const char>> Get(uint16_t slot_id);
```

- Returns a zero-copy view of the record data
- Returns `nullopt` if:

  - slot ID is invalid
  - slot is logically deleted

The returned span is valid **only while the page remains pinned**.

### Update

```cpp
void Update(uint16_t slot_id, const char* new_data, size_t len);
```

- If new data fits in existing space:

  - overwrite in place

- Otherwise:

  - allocate new space
  - update slot to point to new location
  - old data becomes garbage

No compaction or space reuse is performed.

### Delete

```cpp
bool Delete(uint16_t slot_id);
```

- Performs logical deletion
- Marks slot as deleted by setting `length = 0`
- Record data is not removed or reclaimed

### Free Space Query

```cpp
size_t FreeSpace() const;
```

Returns the number of free bytes between:

- the end of the slot directory
- the start of record data

Used by higher layers to decide whether insertion is possible.

## Invariants

The following must always hold:

```text
sizeof(PageHeader)
+ num_slots * sizeof(Slot)
<= free_space_offset
<= PAGE_SIZE
```

Violating this invariant indicates page corruption.

## What this component does not do

- Disk I/O
- Page pinning or eviction
- Page allocation
- Record IDs (RID)
- Table or heap semantics
- MVCC or visibility rules
- Compaction or vacuuming

All of the above are responsibilities of higher layers.

## Usage Context

`SlottedPage` is used by:

- HeapFile (heap access method)
- Future index leaf pages (optional reuse)

It assumes:

- Page memory is valid and pinned
- Caller manages concurrency and lifetime

## Status

Current implementation supports:

- Variable-length records
- Insert / read / update / delete
- Stable slot identifiers

Planned extensions:

- Free space reuse
- Page compaction
- MVCC-aware layouts
