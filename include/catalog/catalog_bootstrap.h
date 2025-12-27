#pragma once

#include <cstdint>
#include "access/heap/heap_file.h"

// IDs
using file_id_t = db::access::file_id_t;
using page_id_t = db::storage::page_id_t;
using db_id_t = uint32_t;
using table_id_t = uint32_t;
using type_id_t = uint32_t;
using col_id_t = uint32_t;

namespace db::catalog {
    
// FILE ID
constexpr file_id_t DB_DATABASES_FILE_ID = 0;
constexpr file_id_t DB_TABLES_FILE_ID = 1;
constexpr file_id_t DB_ATTRIBUTES_FILE_ID = 2;
constexpr file_id_t DB_TYPES_FILE_ID = 3;

// DB ID
constexpr db_id_t DEFAULT_DB_ID = 0;

// TABLE NAMES
constexpr std::string DB_DATABASES_TABLE = "db_databases";
constexpr std::string DB_TABLES_TABLE = "db_tables";
constexpr std::string DB_ATTRIBUTES_TABLE = "db_attributes";
constexpr std::string DB_TYPES_TABLE = "db_types";
}