#pragma once

#include <string>
#include <cstdint>
#include "access/heap/heap_file.h"
#include "storage/page/slotted_page.h"
#include "config/config.h"

namespace db::catalog {

// ids
using table_id_t = config::uuid_t;
using type_id_t = uint32_t;
using file_id_t = db::access::file_id_t;
using page_id_t = db::storage::page_id_t;

struct TableInfo {
    table_id_t table_id;
    std::string table_name;
    file_id_t heap_file_id;
    page_id_t first_page_id;
};

struct RawColumnInfo {
    std::string col_name;
    type_id_t type_id;
    uint16_t ordinal_position;
};

struct ColumnInfo {
    table_id_t table_id;
    std::string col_name;
    type_id_t type_id;
    uint16_t ordinal_position;
};

struct TypeInfo {
    type_id_t type_id;
    uint16_t size;
    std::string type_name;
};

}
