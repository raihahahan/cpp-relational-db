#include "catalog/catalog_codec.h"

namespace db::catalog::codec {
std::vector<uint8_t> TableInfoCodec::Encode(const TableInfo& row) {
    std::vector<uint8_t> buf;
    util::WriteInt(buf, row.first_page_id);
    util::WriteInt(buf, row.heap_file_id);
    util::WriteInt(buf, row.table_id);
    util::WriteString(buf, row.table_name);
    return buf;
}

TableInfo TableInfoCodec::Decode(std::span<const uint8_t> bytes) {
    TableInfo t;
    size_t off = 0;
    t.first_page_id = util::ReadInt<page_id_t>(bytes, off);
    t.heap_file_id = util::ReadInt<file_id_t>(bytes, off);
    t.table_id = util::ReadInt<table_id_t>(bytes, off);
    t.table_name = util::ReadString(bytes, off);
    return t;
}

std::vector<uint8_t> ColumnInfoCodec::Encode(const ColumnInfo& row) {
    std::vector<uint8_t> buf;
    util::WriteInt(buf, row.col_id);
    util::WriteString(buf, row.col_name);
    util::WriteInt(buf, row.ordinal_position);
    util::WriteInt(buf, row.table_id);
    util::WriteInt(buf, row.type_id);
    return buf;
}

ColumnInfo ColumnInfoCodec::Decode(std::span<const uint8_t> bytes) {
    ColumnInfo c;
    size_t off = 0;
    c.col_id = util::ReadInt<col_id_t>(bytes, off);
    c.col_name = util::ReadString(bytes, off);
    c.ordinal_position = util::ReadInt<uint16_t>(bytes, off);
    c.table_id = util::ReadInt<table_id_t>(bytes, off);
    c.type_id = util::ReadInt<type_id_t>(bytes, off);
    return c;
}

std::vector<uint8_t> TypeInfoCodec::Encode(const TypeInfo& row) {
    std::vector<uint8_t> buf;
    util::WriteInt(buf, row.type_id);
    util::WriteInt(buf, row.size);
    util::WriteString(buf, row.type_name);
    return buf;
}

TypeInfo TypeInfoCodec::Decode(std::span<const uint8_t> bytes) {
    TypeInfo t;
    size_t off = 0;
    t.type_id = util::ReadInt<type_id_t>(bytes, off);
    t.size = util::ReadInt<uint16_t>(bytes, off);
    t.type_name = util::ReadString(bytes, off);
    return t;
}
}