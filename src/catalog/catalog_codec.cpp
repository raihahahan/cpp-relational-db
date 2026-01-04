#include "catalog/catalog_codec.h"
#include "util/data.h"
#include "catalog/catalog_bootstrap.h"

namespace db::catalog::codec {
std::vector<uint8_t> TableInfoCodec::Encode(const TableInfo& row) {
    std::vector<uint8_t> buf;
    util::data::WriteData(buf, row.first_page_id);
    util::data::WriteData(buf, row.heap_file_id);
    util::data::WriteData(buf, row.table_id);
    util::data::WriteData(buf, row.table_name);
    return buf;
}

TableInfo TableInfoCodec::Decode(std::span<const uint8_t> bytes) {
    TableInfo t;
    size_t off = 0;
    t.first_page_id = util::data::ReadData<page_id_t>(bytes, off);
    t.heap_file_id = util::data::ReadData<file_id_t>(bytes, off);
    t.table_id = util::data::ReadData<table_id_t>(bytes, off);
    t.table_name = util::data::ReadData<std::string>(bytes, off);
    return t;
}

std::vector<Value> TableInfoCodec::ToValues(const TableInfo& row) {
    std::vector<Value> values;
    values.push_back(row.first_page_id);
    values.push_back(row.heap_file_id);
    values.push_back(row.table_id);
    values.push_back(row.table_name);
    return values;
}

common::Schema TableInfoCodec::GetSchema() { 
   return TABLES_CATALOG_SCHEMA;
}


std::vector<uint8_t> ColumnInfoCodec::Encode(const ColumnInfo& row) {
    std::vector<uint8_t> buf;
    util::data::WriteData(buf, row.col_name);
    util::data::WriteData(buf, row.ordinal_position);
    util::data::WriteData(buf, row.table_id);
    util::data::WriteData(buf, row.type_id);
    return buf;
}

ColumnInfo ColumnInfoCodec::Decode(std::span<const uint8_t> bytes) {
    ColumnInfo c;
    size_t off = 0;
    c.col_name = util::data::ReadData<std::string>(bytes, off);
    c.ordinal_position = util::data::ReadData<uint16_t>(bytes, off);
    c.table_id = util::data::ReadData<table_id_t>(bytes, off);
    c.type_id = util::data::ReadData<type_id_t>(bytes, off);
    return c;
}

std::vector<Value> ColumnInfoCodec::ToValues(const ColumnInfo& row) {
    std::vector<Value> values;
    values.push_back(row.col_name);
    values.push_back(row.ordinal_position);
    values.push_back(row.table_id);
    values.push_back(row.type_id);
    return values;
}

common::Schema ColumnInfoCodec::GetSchema() { 
   return ATTR_CATALOG_SCHEMA;
}


std::vector<uint8_t> TypeInfoCodec::Encode(const TypeInfo& row) {
    std::vector<uint8_t> buf;
    util::data::WriteData(buf, row.type_id);
    util::data::WriteData(buf, row.size);
    util::data::WriteData(buf, row.type_name);
    return buf;
}

TypeInfo TypeInfoCodec::Decode(std::span<const uint8_t> bytes) {
    TypeInfo t;
    size_t off = 0;
    t.type_id = util::data::ReadData<type_id_t>(bytes, off);
    t.size = util::data::ReadData<uint16_t>(bytes, off);
    t.type_name = util::data::ReadData<std::string>(bytes, off);
    return t;
}

std::vector<Value> TypeInfoCodec::ToValues(const TypeInfo& row) {
    std::vector<Value> values;
    values.push_back(row.size);
    values.push_back(row.type_id);
    values.push_back(row.type_name);
    return values;
}

common::Schema TypeInfoCodec::GetSchema() { 
   return TYPES_CATALOG_SCHEMA;
}

}