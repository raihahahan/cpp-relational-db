#include "model/dynamic_codec.h"
#include "util/data.h"
#include <assert.h>
#include "catalog/catalog_bootstrap.h"
#include <cstdint>

namespace db::model {
uint32_t getAlignment(catalog::type_id_t type_id) {
    size_t alignment = 0;
    switch (type_id)
    {
    case catalog::INT_TYPE:
        alignment = catalog::INT_ALIGNMENT;
        break;
    
    case catalog::TEXT_TYPE:
        alignment = catalog::TEXT_ALIGNMENT;
    default:
        break;
    }
    return alignment;
}

void applyPadding(std::vector<uint8_t>& buffer, catalog::type_id_t type_id) {
    size_t alignment = getAlignment(type_id);

    size_t current_size = buffer.size();
    size_t remainder = current_size % alignment;
    size_t padding_needed = (remainder == 0) ? 0 : (alignment - remainder);
    buffer.resize(buffer.size() + padding_needed, 0);
}

void skipPadding(size_t& offset, catalog::type_id_t type_id) {
    size_t alignment = getAlignment(type_id);
    size_t remainder = offset % alignment;
    if (remainder != 0) offset += (alignment - remainder);
}

std::vector<uint8_t> DynamicCodec::Encode(
                    const std::vector<Value>& values,
                    const std::vector<ColumnInfo> schema) {
    assert(values.size() == schema.size());
    std::vector<uint8_t> buffer;
    for (size_t i = 0; i < values.size(); ++i) {
        const auto& val = values[i];
        const auto& col = schema[i];

        applyPadding(buffer, col.type_id);

        std::visit([&](auto&& arg) {
            util::data::WriteData(buffer, arg);
        }, val);
    }

    return buffer;
}

std::vector<Value> DynamicCodec::Decode(
    std::span<const uint8_t> bytes, 
    const std::vector<ColumnInfo>& schema) {
    std::vector<Value> result;
    size_t offset = 0;

    for (const auto& col : schema) {
        skipPadding(offset, col.type_id);

        if (col.type_id == catalog::INT_TYPE) {
            result.push_back(util::data::ReadData<uint32_t>(bytes, offset));
        } else if (col.type_id == catalog::TEXT_TYPE) {
            result.push_back(util::data::ReadData<std::string>(bytes, offset));
        }
    }
    return result;
}
};
