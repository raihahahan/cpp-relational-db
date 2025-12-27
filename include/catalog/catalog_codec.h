#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include "catalog/catalog_types.h"

namespace db::catalog::codec::util {
template <typename T>
concept FixedWidthSerializable = 
    std::is_trivially_copyable_v<T> &&
    std::is_standard_layout_v<T>;

template <FixedWidthSerializable T>
inline void WriteInt(std::vector<uint8_t>& buf, T v) {
    uint8_t bytes[sizeof(T)];
    std::memcpy(bytes, &v, sizeof(v));
    buf.insert(buf.end(), bytes, bytes + sizeof(v));
};

inline void WriteString(std::vector<uint8_t>& buf, std::string_view s) {
    WriteInt(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(),
                reinterpret_cast<const uint8_t*>(s.data()),
                reinterpret_cast<const uint8_t*>(s.data() + s.size()));
};

template <FixedWidthSerializable T>
inline T ReadInt(std::span<const uint8_t> buf, size_t& off) {
    T res;
    std::memcpy(&res, buf.data() + off, sizeof(res));
    off += sizeof(res);
    return res;
};

inline std::string ReadString(std::span<const uint8_t> buf, 
                                size_t& off) {
    auto len = ReadInt<uint32_t>(buf, off);
    std::string s(reinterpret_cast<const char*>(buf.data()), len);
    off += len;
    return s;
};
}

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
