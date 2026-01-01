# Model Layer Overview

The model layer serves as the logical bridge between the low-level physical storage (Heap Files, Pages) and the higher-level database abstractions (Tables, Rows, Values). It is responsible for schema enforcement, data serialisation (encoding/decoding), and managing table instances.

The `/model` folder contains the following components:

```
+---------------------------------------+
|             Table Manager             | Logic for opening and caching
|    - Resolve names to metadata        | UserTable instances.
|    - Manage table lifecycle           |
+-------------------+-------------------+
                    |
                    v
+---------------------------------------+
|               UserTable               | Logical representation of a
|    - Insert rows as Values            | table schema and its physical
|    - Interaction with Relation base   | data file.
+-------------------+-------------------+
                    |
                    v
+---------------------------------------+
|             DynamicCodec              | Serialises C++ types into
|    - Encode/Decode Values             | byte buffers with proper
|    - Handle padding/alignment         | memory alignment.
+-------------------+-------------------+
                    |
                    v
+---------------------------------------+
|            Relation (Base)            | Shared logic for physical
|    - Interaction with HeapFile        | interaction and MVCC headers.
+---------------------------------------+

```

## 1. Architecture Overview

| Component        | Description                        | Responsibility                                             |
| ---------------- | ---------------------------------- | ---------------------------------------------------------- |
| **TableManager** | Global table registry.             | Caches opened tables and resolves names via the Catalog.   |
| **UserTable**    | Schema-aware table handle.         | Provides a row-based API for inserts and lookups.          |
| **DynamicCodec** | Binary serialisation engine.       | Converts `std::variant` values to/from aligned bytes.      |
| **Relation**     | Base class for all relation types. | Encapsulates a `HeapFile` and provides raw storage access. |

### Data Representation: The `Value` Type

The system uses a flexible variant type to represent data in memory:
`using Value = std::variant<uint32_t, std::string>;`

## 2. DynamicCodec

The `DynamicCodec` is responsible for transforming logical rows into a compact binary format suitable for storage in a `SlottedPage`.

### Responsibilities

- **Alignment**: Ensures that data types (like `uint32_t`) start at memory offsets that match their architecture alignment requirements (e.g. 4-byte boundaries).
- **Padding**: Automatically inserts null bytes between fields to maintain alignment.
- **Type Dispatch**: Uses `std::visit` to handle different types within the `Value` variant during encoding.

### Interface Highlights

| Method                       | Description                                                                        |
| ---------------------------- | ---------------------------------------------------------------------------------- |
| **`Encode(values, schema)`** | Converts a vector of Values into a byte buffer (`std::vector<uint8_t>`).           |
| **`Decode(bytes, schema)`**  | Reconstructs a vector of Values from a raw byte span based on the provided schema. |

## 3. UserTable & Relation

`UserTable` inherits from `Relation` to combine physical storage capabilities with logical schema metadata.

### Responsibilities

- **Schema Enforcement**: Uses `ColumnInfo` to ensure that inserted data matches the table's defined types.
- **Row Insertion**: Orchestrates the flow from `Value` -> `DynamicCodec` -> `HeapFile`.
- **Storage Abstraction**: Wraps a `HeapFile` instance to manage the underlying pages and records.

### Typical Workflow

1. User calls UserTable::Insert(values)
2. UserTable passes values to DynamicCodec::Encode()
3. DynamicCodec returns a byte buffer with proper alignment
4. UserTable calls Relation::InsertRaw()
5. Relation passes raw bytes to HeapFile::Insert()

## 4. TableManager

The `TableManager` acts as a factory and cache for tables. It ensures that only one instance of a `UserTable` exists for any given table name to prevent conflicting file access.

### Responsibilities

- **Metadata Resolution**: Interacts with the `TablesCatalog` and `AttributesCatalog` to fetch table IDs and column schemas.
- **Caching**: Maintains an internal `std::unordered_map` to store and reuse `shared_ptr<UserTable>` instances.
- **Resource Management**: Coordinates with the `BufferManager` and `DiskManager` to open the correct `HeapFile` for a table.

### Public Interface

| Method                | Description                                                                                                                               |
| --------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| **`OpenTable(name)`** | Looks up a table by name. If not in cache, it loads metadata from the catalogs, opens the `HeapFile`, and instantiates a new `UserTable`. |

## Testing Instructions

```bash
make test_model
```
