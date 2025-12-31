#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include "catalog/catalog_types.h"

namespace db::catalog::codec::utilities {
template <typename T>
concept FixedWidthSerializable = 
    std::is_trivially_copyable_v<T> &&
    std::is_standard_layout_v<T>;

template <typename T>
concept VariableWidthSerializable =
    requires (T v) {
        { v.data() } -> std::convertible_to<const char*>;
        { v.size() } -> std::convertible_to<size_t>;
    };


template <FixedWidthSerializable T>
inline void WriteData(std::vector<uint8_t>& buf, T v) {
    uint8_t bytes[sizeof(T)];
    std::memcpy(bytes, &v, sizeof(v));
    buf.insert(buf.end(), bytes, bytes + sizeof(v));
};

template <VariableWidthSerializable T>
inline void WriteData(std::vector<uint8_t>& buf, T v) {
    WriteData(buf, static_cast<uint32_t>(v.size()));
    buf.insert(buf.end(),
                reinterpret_cast<const uint8_t*>(v.data()),
                reinterpret_cast<const uint8_t*>(v.data() + v.size()));
};

template <FixedWidthSerializable T>
inline T ReadData(std::span<const uint8_t> buf, size_t& off) {
    T res;
    std::memcpy(&res, buf.data() + off, sizeof(res));
    off += sizeof(res);
    return res;
};

template <VariableWidthSerializable T>
inline T ReadData(std::span<const uint8_t> buf, 
                                size_t& off) {
    auto len = ReadData<uint32_t>(buf, off);
    std::string s(reinterpret_cast<const char*>(buf.data() + off), len);
    off += len;
    return s;
};
}

namespace db::catalog::codec {
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
