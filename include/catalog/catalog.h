#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include "access/heap/heap_file.h"
#include "access/record.h"

/*
catalogs:
1. db_databases: database info
2. db_tables: table info (maps table to heap file and page id)
3. db_attributes: table column info
4. db_types: types info

schema:
1. db_databases:
   (int db_id, str db_name)

2. db_tables:
   (int table_id, int db_id, str table_name,
    int heap_file_id, int first_page_id)

3. db_attributes:
   (int table_id, int col_id, str col_name,
    int type_id, int ordinal_position)

4. db_types:
   (int type_id, int size)

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
    int db_id;
    std::string db_name;
};

struct TableInfo {
    int table_id;
    int db_id;
    std::string table_name;
    int heap_file_id;
    int first_page_id;
};

struct ColumnInfo {
    int table_id;
    int col_id;
    std::string col_name;
    int type_id;
    int ordinal_position;
};

struct TypeInfo {
    int type_id;
    int size;
};

using HeapFile = db::access::HeapFile;

class Catalog {
public:
    explicit Catalog(BufferManager* bm);
    // create catalog heap files and insert catalog metadata
    void Init();

    // db operations
    int CreateDatabase(const std::string& db_name);
    std::optional<DatabaseInfo> GetDatabase(const std::string& db_name) const;

    // table operations
    int CreateTable(
        int db_id,
        const std::string& table_name,
        const std::vector<ColumnInfo>& columns
    );

    std::optional<TableInfo> GetTable(
        int db_id,
        const std::string& table_name
    ) const;

    std::vector<ColumnInfo> GetTableColumns(int table_id) const;

    // type operations
    std::optional<TypeInfo> GetType(int type_id) const;

private:
    int AllocateDatabaseId();
    int AllocateTableId();
    int AllocateTypeId();

private:
    BufferManager* _bm;

    // catalog heap files
    HeapFile _db_databases;
    HeapFile _db_tables;
    HeapFile _db_attributes;
    HeapFile _db_types;
};

}
