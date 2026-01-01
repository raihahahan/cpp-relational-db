#pragma once
#include <vector>
#include "model/user_table.h"

namespace db::model {
struct DynamicCodec {
    static std::vector<uint8_t> Encode(
                            const std::vector<Value>& values,
                            const std::vector<ColumnInfo> schema);
    
    static std::vector<Value> Decode(std::span<const uint8_t> bytes, const std::vector<ColumnInfo>& schema);
};
}