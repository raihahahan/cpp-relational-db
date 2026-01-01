#pragma once
#include <unordered_map>
#include "catalog/catalog_types.h"
#include "catalog/catalog_tables.h"
#include "model/user_table.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "storage/disk_manager/disk_manager.h"
#include <memory>

namespace db::model {
class TableManager {
public:
    explicit TableManager(catalog::TablesCatalog* tables_catalog,
                            catalog::AttributesCatalog* attr_catalog,
                            storage::BufferManager* bm,
                            storage::DiskManager* dm);
    std::shared_ptr<UserTable> OpenTable(std::string_view name);

private:
    std::unordered_map<std::string_view, std::shared_ptr<UserTable>> _cache;
    catalog::TablesCatalog* _tables_catalog;
    catalog::AttributesCatalog* _attr_catalog;
    storage::BufferManager* _bm;
    storage::DiskManager* _dm;
};
}