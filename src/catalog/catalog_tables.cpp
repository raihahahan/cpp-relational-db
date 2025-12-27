#include "catalog/catalog_tables.h"

namespace db::catalog {
std::optional<DatabaseInfo> DatabasesCatalog::Lookup(std::string_view db_name) const {
    for (auto it = _hf->begin(); it != _hf->end(); ++it) {
        auto rec = *it;
        auto bytes = std::span<const uint8_t>{
            reinterpret_cast<const uint8_t*>(rec.data),
            rec.size
        };
        
        auto db = codec::DatabaseInfoCodec::Decode(bytes);
        if (db.db_name == db_name) {
            return db;
        }
    }
    return std::nullopt;
}

std::optional<TableInfo> TablesCatalog::Lookup(db_id_t db_id, std::string_view table_name) const {
    for (auto it = _hf->begin(); it != _hf->end(); ++it) {
        auto rec = *it;
        auto bytes = std::span<const uint8_t>{
            reinterpret_cast<const uint8_t*>(rec.data),
            rec.size
        };
        
        auto table = codec::TableInfoCodec::Decode(bytes);
        if (table.table_name == table_name) {
            return table;
        }
    }
    return std::nullopt;
}

std::vector<ColumnInfo> AttributesCatalog::GetColumns(table_id_t table_id) const {
    std::vector<ColumnInfo> res;
    for (auto it = _hf->begin(); it != _hf->end(); ++it) {
        auto rec = *it;
        auto bytes = std::span<const uint8_t>{
            reinterpret_cast<const uint8_t*>(rec.data),
            rec.size
        };
        
        auto col = codec::ColumnInfoCodec::Decode(bytes);
        if (col.table_id == table_id) {
            res.emplace_back(col);
        }
    }
    return res;
}

std::vector<TypeInfo> TypesCatalog::GetTypes() const {
    std::vector<TypeInfo> res;
    for (auto it = _hf->begin(); it != _hf->end(); ++it) {
        auto rec = *it;
        auto bytes = std::span<const uint8_t>{
            reinterpret_cast<const uint8_t*>(rec.data),
            rec.size
        };
        
        auto type = codec::TypeInfoCodec::Decode(bytes);
        res.emplace_back(type);
    }
    return res;
}
}