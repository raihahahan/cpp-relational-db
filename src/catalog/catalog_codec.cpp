#include "catalog/catalog_codec.h"

namespace db::catalog::codec {
std::vector<uint8_t> TableInfoCodec::Encode(const TableInfo& row) {
    std::vector<uint8_t> buf;
    utilities::WriteData(buf, row.first_page_id);
    utilities::WriteData(buf, row.heap_file_id);
    utilities::WriteData(buf, row.table_id);
    utilities::WriteData(buf, row.table_name);
    return buf;
}

TableInfo TableInfoCodec::Decode(std::span<const uint8_t> bytes) {
    TableInfo t;
    size_t off = 0;
    t.first_page_id = utilities::ReadData<page_id_t>(bytes, off);
    t.heap_file_id = utilities::ReadData<file_id_t>(bytes, off);
    t.table_id = utilities::ReadData<table_id_t>(bytes, off);
    t.table_name = utilities::ReadData<std::string>(bytes, off);
    return t;
}

std::vector<uint8_t> ColumnInfoCodec::Encode(const ColumnInfo& row) {
    std::vector<uint8_t> buf;
    utilities::WriteData(buf, row.col_id);
    utilities::WriteData(buf, row.col_name);
    utilities::WriteData(buf, row.ordinal_position);
    utilities::WriteData(buf, row.table_id);
    utilities::WriteData(buf, row.type_id);
    return buf;
}

ColumnInfo ColumnInfoCodec::Decode(std::span<const uint8_t> bytes) {
    ColumnInfo c;
    size_t off = 0;
    c.col_id = utilities::ReadData<col_id_t>(bytes, off);
    c.col_name = utilities::ReadData<std::string>(bytes, off);
    c.ordinal_position = utilities::ReadData<uint16_t>(bytes, off);
    c.table_id = utilities::ReadData<table_id_t>(bytes, off);
    c.type_id = utilities::ReadData<type_id_t>(bytes, off);
    return c;
}

std::vector<uint8_t> TypeInfoCodec::Encode(const TypeInfo& row) {
    std::vector<uint8_t> buf;
    utilities::WriteData(buf, row.type_id);
    utilities::WriteData(buf, row.size);
    utilities::WriteData(buf, row.type_name);
    return buf;
}

TypeInfo TypeInfoCodec::Decode(std::span<const uint8_t> bytes) {
    TypeInfo t;
    size_t off = 0;
    t.type_id = utilities::ReadData<type_id_t>(bytes, off);
    t.size = utilities::ReadData<uint16_t>(bytes, off);
    t.type_name = utilities::ReadData<std::string>(bytes, off);
    return t;
}
}