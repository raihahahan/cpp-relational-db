#pragma once
#include <unordered_map>
#include "model/user_table.h"
#include "catalog/catalog.h"
#include "catalog/catalog_types.h"
#include <memory>

namespace db::model {
class TableManager {
public:
    explicit TableManager(catalog::Catalog* catalog);
    std::shared_ptr<UserTable> OpenTable(std::string name);
    table_id_t CreateTable(const std::string& table_name, const std::vector<catalog::RawColumnInfo>& columns);

private:
    std::unordered_map<std::string, std::shared_ptr<UserTable>> _cache;
    catalog::Catalog* _catalog;
};
}