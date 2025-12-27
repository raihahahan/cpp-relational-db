#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include "catalog/catalog_types.h"

namespace db::catalog::codec {

struct DatabaseInfoCodec {
    static std::vector<uint8_t> Encode(const DatabaseInfo& row);
    static DatabaseInfo Decode(std::span<const uint8_t> bytes);
};

struct TableInfoCodec {
    static std::vector<uint8_t> Encode(const TableInfo& row);
    static TableInfo Decode(std::span<const uint8_t> bytes);
};

struct ColumnInfoCodec {
    static std::vector<uint8_t> Encode(const ColumnInfo& row);
    static ColumnInfo Decode(std::span<const uint8_t> bytes);
};

struct TypeInfoCodec {
    static std::vector<uint8_t> Encode(const TypeInfo& row);
    static TypeInfo Decode(std::span<const uint8_t> bytes);
};

}
