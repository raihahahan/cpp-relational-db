# Database Server

The **database server subsystem** is responsible for managing database-level
lifecycle and routing. It provides a lightweight interface for creating, opening, deleting, and
discovering database files.

This layer intentionally stays minimal.

## Overview

The server maintains a cache of active databases and maps database names to
their corresponding on-disk files.

Each database is represented by a single `.db` file stored under `config::DATA_PATH`.

```
DATA_PATH/
├── db1.db
├── db2.db
└── system.db
```

## Components

| Component    | Description                                        |
| ------------ | -------------------------------------------------- |
| **DbServer** | Manages database files and `DiskManager` instances |

## Responsibilities

The DbServer is responsible for:

- Discovering existing databases on startup
- Creating new database files
- Opening existing databases
- Caching active `DiskManager` instances
- Deleting databases and cleaning up resources

## Design Principles

### Minimal State

The server stores only:

- Database name => `DiskManager` mappings

All persistent state lives on disk.

### Explicit Lifecycle Management

Databases are managed explicitly via API calls:

- `CreateDatabase(name)`
- `OpenDatabase(name)`
- `DeleteDatabase(name)`

This avoids implicit creation and makes database state predictable.

### Restart Safety

On startup, `DbServer::Init()` scans the data directory and loads all existing
database files into the in-memory cache.

This ensures:

- Databases survive restarts
- No metadata is lost
- Database discovery is deterministic

## Public API

### `Init()`

```cpp
void Init();
```

Scans `config::DATA_PATH` and loads all existing `.db` files into the cache.

Should be called once during server startup.

### `CreateDatabase()`

```cpp
bool CreateDatabase(const std::string& db_name);
```

Creates a new database file.

- Returns `false` if the database already exists
- Returns `true` on successful creation

### `OpenDatabase()`

```cpp
storage::DiskManager* OpenDatabase(const std::string& db_name);
```

Opens an existing database.

- Returns a cached `DiskManager` if already open
- Returns `nullptr` if the database does not exist

### `DeleteDatabase()`

```cpp
bool DeleteDatabase(const std::string& db_name);
```

Deletes an existing database.

- Removes the database from the cache
- Deletes the on-disk `.db` file
- Returns `false` if the database does not exist

## Example Usage

```cpp
DbServer server;
server.Init();

server.CreateDatabase("testdb");

auto* dm = server.OpenDatabase("testdb");
if (dm == nullptr) {
    // handle error
}

server.DeleteDatabase("testdb");
```

## Testing

The DbServer is tested using integration tests that validate:

- Database creation and deletion
- Restart behavior
- File discovery on startup
- Cache correctness

See:

```
tests/server/test_server.cpp
```
