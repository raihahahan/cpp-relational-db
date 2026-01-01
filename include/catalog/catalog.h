#pragma once

#include <optional>
#include <vector>
#include <string>
#include "catalog/catalog_types.h"
#include "catalog/catalog_tables.h"
#include "catalog/catalog_bootstrap.h"
#include "util/uuid.h"

/*
catalogs:
1. db_tables: table info (maps table to heap file and page id)
2. db_attributes: table column info
3. db_types: types info

schema:
1. db_tables:
- (int table_id, str table_name,
    int heap_file_id, int first_page_id)

2. db_attributes:
- (int table_id, int col_id, str col_name,
    int type_id, int ordinal_position)

3. db_types:
- (int type_id, int size)

note:
- catalogs are stored as regular heap files
- catalog lookup maps logical names -> heap file + page id

on build db program:
- no catalogs created

on init db program:
1. create heap files for the above catalogs
2. insert hardcoded types into db_types
3. insert rows into db_tables for the four catalog tables
4. insert column metadata into db_attributes for the four catalog tables

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
    
    // table operations
    table_id_t CreateTable(
        const std::string& table_name,
        const std::vector<RawColumnInfo>& columns
    );

    std::optional<TableInfo> LookupTable(
        const std::string& table_name
    );

    std::vector<ColumnInfo> GetTableColumns(table_id_t table_id);
    
    // bootstrap
    bool IsInitialised() const;
    void LoadCatalogs();
    void BootstrapCatalogs();
    void InsertBuiltinTypes();
    void InsertCatalogMetadata();

    // catalog accessor
    TablesCatalog* GetTablesCatalog() const;
    AttributesCatalog* GetAttributesCatalog() const;
    TypesCatalog* GetTypesCatalog() const;
    db::storage::BufferManager* GetBm() const;
    db::storage::DiskManager* GetDm() const;
    
private:
    db::storage::BufferManager* _bm;
    db::storage::DiskManager* _dm;

    std::optional<TablesCatalog> _tables;
    std::optional<AttributesCatalog> _attributes;
    std::optional<TypesCatalog> _types;
};
}
