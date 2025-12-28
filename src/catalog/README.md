# Catalog Layer

This directory implements the **system catalog** for the database.
The catalog is responsible for storing and retrieving **metadata** about tables, columns, and types, and is persisted on disk using the same heap file abstraction as user tables.

The catalog layer sits **below the query executor** and is required for:

- name resolution (`table_name => heap file`)
- schema discovery (`table_id => columns`)
- type resolution (`type_id => size / name`)
- database bootstrapping and restart correctness

## High-level Architecture

```
Catalog
 ├── TablesCatalog      (db_tables)
 ├── AttributesCatalog  (db_attributes)
 ├── TypesCatalog       (db_types)
 └── Codecs             (row <=> bytes)
```

All catalog data is stored as **regular heap files**.
There is **no special storage path** for metadata.

## Catalog Tables

### 1. `db_tables`

Stores one row per table.

Logical schema:

```
(table_id, table_name, heap_file_id, first_page_id)
```

Purpose:

- Maps logical table names to physical storage
- Enables executor to locate heap files

Access:

- Sequential scan via `TablesCatalog::Lookup(table_name)`

### 2. `db_attributes`

Stores one row per column.

Logical schema:

```
(table_id, col_id, col_name, type_id, ordinal_position)
```

Purpose:

- Describes table schemas
- Enables row decoding and projection

Access:

- Sequential scan via `AttributesCatalog::GetColumns(table_id)`

### 3. `db_types`

Stores built-in and future user-defined types.

Logical schema:

```
(type_id, size, type_name)
```

Purpose:

- Type resolution during decoding
- Fixed-width vs variable-width handling

Access:

- Sequential scan via `TypesCatalog::GetTypes()`

## Storage Model

- Each catalog table is backed by a `HeapFile`
- Records are stored in slotted pages
- All access paths are **sequential scans**
- No indexes are used at this stage

This mirrors real database bootstrapping (e.g. PostgreSQL), where catalog access must work **before** the planner or executor exists.

## Codecs (`catalog_codec.*`)

Each catalog row type has an explicit codec responsible for converting between:

```
struct Row  <=>  std::vector<uint8_t>
```

### Design rules

- Codecs are **explicit and per-table**
- No `memcpy` of structs
- Variable-length fields are length-prefixed
- Decoding uses `std::span<const uint8_t>` for safety

### Example: `TableInfoCodec`

Encoding:

```cpp
util::WriteInt(buf, row.first_page_id);
util::WriteInt(buf, row.heap_file_id);
util::WriteInt(buf, row.table_id);
util::WriteString(buf, row.table_name);
```

Decoding:

```cpp
t.first_page_id = util::ReadInt<page_id_t>(bytes, off);
t.heap_file_id  = util::ReadInt<file_id_t>(bytes, off);
t.table_id      = util::ReadInt<table_id_t>(bytes, off);
t.table_name    = util::ReadString(bytes, off);
```

This explicit layout:

- decouples physical storage from C++ struct layout
- allows schema evolution
- prevents undefined behavior from padding or alignment

## CatalogTable Base Class

Catalog tables share common behavior:

- ownership of a `HeapFile`
- insertion of encoded rows

This is factored into a template base:

```cpp
CatalogTable<Row, Codec>
```

Responsibilities:

- enforce codec correctness at compile time
- provide a generic `Insert(row)` implementation
- expose protected access to the underlying heap file

Specialised catalog tables implement **only semantic access paths** (e.g. `Lookup`, `GetColumns`).

## Bootstrapping

Catalog bootstrapping happens **once**, on an empty database.

### Initialization check

- Page 0 is treated as a superblock
- A magic value (`DB_MAGIC`) indicates initialisation

### Bootstrap sequence

1. Allocate root page and write header
2. Allocate heap root pages for each catalog table
3. Initialize each heap page
4. Open heap files
5. Insert built-in types
6. Insert catalog metadata (tables + columns)

After bootstrapping, catalog tables behave exactly like user tables.

## Guarantees Provided by the Catalog

Once initialized, the catalog guarantees:

- table name => heap file resolution
- schema availability for decoding rows
- restart correctness (metadata persists on disk)
- independence from query execution logic
