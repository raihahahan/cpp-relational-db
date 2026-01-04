#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include "catalog/catalog_types.h"
#include "util/data.h"
#include "common/value.h"
#include "common/tuple.h"

namespace db::catalog::codec {
using Value = common::Value;
using Schema = common::Schema;
struct TableInfoCodec {
    static std::vector<uint8_t> Encode(const TableInfo& row);
    static TableInfo Decode(std::span<const uint8_t> bytes);
    static std::vector<Value> ToValues(const TableInfo& row);
    static Schema GetSchema();
};

struct ColumnInfoCodec {
    static std::vector<uint8_t> Encode(const ColumnInfo& row);
    static ColumnInfo Decode(std::span<const uint8_t> bytes);
    static std::vector<Value> ToValues(const ColumnInfo& row);
    static Schema GetSchema();
};

struct TypeInfoCodec {
    static std::vector<uint8_t> Encode(const TypeInfo& row);
    static TypeInfo Decode(std::span<const uint8_t> bytes);
    static std::vector<Value> ToValues(const TypeInfo& row);
    static Schema GetSchema();
};

}
