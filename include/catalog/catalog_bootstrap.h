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

}
