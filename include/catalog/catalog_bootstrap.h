#pragma once

#include <cstdint>
#include "catalog/catalog_types.h"
#include "util/uuid.h"

namespace db::catalog {

// FILE ID
constexpr file_id_t DB_DATABASES_FILE_ID = 
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1);
constexpr file_id_t DB_TABLES_FILE_ID = 
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,2);
constexpr file_id_t DB_ATTRIBUTES_FILE_ID = 
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,3);
constexpr file_id_t DB_TYPES_FILE_ID = 
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,4);

// TABLE NAMES
constexpr std::string DB_DATABASES_TABLE = "db_databases";
constexpr std::string DB_TABLES_TABLE = "db_tables";
constexpr std::string DB_ATTRIBUTES_TABLE = "db_attributes";
constexpr std::string DB_TYPES_TABLE = "db_types";

// TABLE ID
constexpr table_id_t DB_DATABASES_TABLE_ID =
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1);
constexpr table_id_t DB_TABLES_TABLE_ID =
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,2);
constexpr table_id_t DB_ATTRIBUTES_TABLE_ID =
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,3);
constexpr table_id_t DB_TYPES_TABLE_ID =
    util::MakeUUID(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,4);

// PAGE ID
constexpr page_id_t ROOT_PAGE_ID = 0;
constexpr page_id_t DB_TABLES_ROOT_PAGE_ID = 1;
constexpr page_id_t DB_ATTRIBUTES_ROOT_PAGE_ID = 2;
constexpr page_id_t DB_TYPES_ROOT_PAGE_ID = 3;

// TYPE IDS and data
constexpr type_id_t INT_TYPE = 1;
constexpr uint16_t INT_SIZE = 4;
constexpr type_id_t TEXT_TYPE = 2;
constexpr uint16_t TEXT_SIZE = -1; // varlen

// ALIGNMENTS
constexpr uint32_t INT_ALIGNMENT = 4;
constexpr uint32_t TEXT_ALIGNMENT = 4;

// CATALOG SCHEMAS 
const std::vector<ColumnInfo> TABLES_CATALOG_SCHEMA = {
    {DB_TABLES_TABLE_ID, "table_id", INT_TYPE, 1},
    {DB_TABLES_TABLE_ID, "table_name", TEXT_TYPE, 2},
    {DB_TABLES_TABLE_ID, "heap_file_id", INT_TYPE, 3},
    {DB_TABLES_TABLE_ID, "first_page_id", INT_TYPE, 4}
};

const std::vector<ColumnInfo> ATTR_CATALOG_SCHEMA = {
    {DB_ATTRIBUTES_TABLE_ID, "table_id", INT_TYPE, 1},
    {DB_ATTRIBUTES_TABLE_ID, "col_id", INT_TYPE, 2},
    {DB_ATTRIBUTES_TABLE_ID, "col_name", TEXT_TYPE, 3},
    {DB_ATTRIBUTES_TABLE_ID, "type_id", INT_TYPE, 4},
    {DB_ATTRIBUTES_TABLE_ID, "ordinal_position", INT_TYPE, 5}
};

const std::vector<ColumnInfo> TYPES_CATALOG_SCHEMA = {
    {DB_TYPES_TABLE_ID, "type_id", INT_TYPE, 1},
    {DB_TYPES_TABLE_ID, "size", INT_TYPE, 2}
};
}
