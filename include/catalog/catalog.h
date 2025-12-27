#pragma once

#include <optional>
#include <vector>
#include <string>
#include "catalog/catalog_types.h"
#include "catalog/catalog_tables.h"
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

class Catalog {
public:
    explicit Catalog(db::storage::BufferManager* bm,
                     db::storage::DiskManager* dm);

    void Init();
    
    // database operations
    db_id_t CreateDatabase(const std::string& db_name);
    std::optional<DatabaseInfo> GetDatabase(const std::string& db_name) const;

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
    
private:
    // bootstrap
    bool IsInitialised() const;
    void LoadCatalogs(db_id_t db_id = DEFAULT_DB_ID);
    void BootstrapCatalogs();
    void InsertBuiltinTypes();
    void InsertCatalogMetadata();

    // id allocators
    db_id_t AllocateDatabaseId();
    table_id_t AllocateTableId();
    col_id_t AllocateColId();
    type_id_t AllocateTypeId();
    file_id_t AllocateFileId();

private:
    db::storage::BufferManager* _bm;
    db::storage::DiskManager* _dm;

    std::optional<DatabasesCatalog> _databases;
    std::optional<TablesCatalog> _tables;
    std::optional<AttributesCatalog> _attributes;
    std::optional<TypesCatalog> _types;
};
}
