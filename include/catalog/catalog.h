#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include "access/heap/heap_file.h"
#include "access/record.h"
#include "catalog/catalog_bootstrap.h"

/*
catalogs:
1. db_databases: database info
2. db_tables: table info (maps table to heap file and page id)
3. db_attributes: table column info
4. db_types: types info

schema:
1. db_databases:
- (db_id_t db_id, str db_name)

2. db_tables:
- (int table_id, db_id_t db_id, str table_name,
    int heap_file_id, int first_page_id)

3. db_attributes:
- (int table_id, int col_id, str col_name,
    int type_id, int ordinal_position)

4. db_types:
- (int type_id, int size)

note:
- (db_id, table_name) is unique
- catalogs are stored as regular heap files
- catalog lookup maps logical names -> heap file + page id

on build db program:
- no catalogs created

on init db program:
1. create four heap files for the above catalogs
2. insert hardcoded types into db_types
3. insert rows into db_tables for the four catalog tables
4. insert column metadata into db_attributes for the four catalog tables

on create database:
1. insert row into db_databases

on create table:
1. create heap file for table
2. insert row into db_tables
3. insert rows into db_attributes
*/

namespace db::catalog {

// catalog metadata
struct DatabaseInfo {
    db_id_t db_id;
    std::string db_name;
};

struct TableInfo {
    table_id_t table_id;
    db_id_t db_id;
    std::string table_name;
    file_id_t heap_file_id;
    page_id_t first_page_id;
};

struct ColumnInfo {
    table_id_t table_id;
    col_id_t col_id;
    std::string col_name;
    type_id_t type_id;
    int ordinal_position;
};

struct TypeInfo {
    type_id_t type_id;
    int size;
};

using HeapFile = db::access::HeapFile;

class Catalog {
public:
    explicit Catalog(BufferManager* bm, DiskManager* dm);
    // create catalog heap files and insert catalog metadata
    void Init();

    // db operations
    db_id_t CreateDatabase(const std::string& db_name);
    std::optional<DatabaseInfo> GetDatabase(const std::string& db_name) const;
    bool DeleteDatabase(const std::string& db_name);

    // table operations
    table_id_t CreateTable(
        db_id_t db_id,
        const std::string& table_name,
        const std::vector<ColumnInfo>& columns
    );

    std::optional<TableInfo> LookupTable(
        db_id_t db_id,
        const std::string& table_name
    ) const;

    std::vector<ColumnInfo> GetTableColumns(table_id_t table_id) const;

    bool AlterTable(db_id_t db_id, const std::string& table_name);

    bool DeleteTable(db_id_t db_id, const std::string& table_name);

    // row operations
    bool InsertRow(db_id_t db_id, const std::string& table_name,
                    const std::vector<ColumnInfo>& columns);

    bool UpdateRow(db_id_t db_id, const std::string& table_name,
                    const std::vector<ColumnInfo>& columns);

    // type operations
    std::optional<TypeInfo> GetType(type_id_t type_id) const;

    // bootstrapping
    bool IsInitialised();
    void LoadCatalogs(db_id_t db_id = DEFAULT_DB_ID);
    void BootstrapCatalogs();
    void InsertBuiltinTypes();
    void InsertCatalogMetadata();
    
private:
    db_id_t AllocateDatabaseId();
    table_id_t AllocateTableId();
    type_id_t AllocateTypeId();
    col_id_t AllocateColId();

private:
    BufferManager* _bm;
    DiskManager* _dm;

    // catalog heap files
    std::optional<HeapFile> _db_databases;
    std::optional<HeapFile> _db_tables;
    std::optional<HeapFile> _db_attributes;
    std::optional<HeapFile> _db_types;
};

}
