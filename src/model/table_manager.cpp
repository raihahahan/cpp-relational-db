#include "model/table_manager.h"
#include "access/heap/heap_file.h"

namespace db::model {
TableManager::TableManager(catalog::Catalog* catalog) :
                        _catalog{catalog} {};

std::shared_ptr<UserTable> TableManager::OpenTable(std::string_view name) {
    if (_cache.contains(name)) return _cache[name];

    auto table_info = _catalog->GetTablesCatalog()->Lookup(name);
    if (!table_info.has_value()) throw std::runtime_error("Invalid table name.");

    auto cols = _catalog->GetAttributesCatalog()->GetColumns(table_info->table_id);
    auto hf = access::HeapFile::Open(_catalog->GetBm(), _catalog->GetDm(), table_info->heap_file_id, table_info->first_page_id);

    auto rel = std::make_shared<UserTable>(std::move(hf), std::move(cols), table_info->table_id);
    _cache[name] = rel;
    return rel;
} 

table_id_t TableManager::CreateTable(const std::string& table_name, const std::vector<catalog::RawColumnInfo>& columns) {
    return _catalog->CreateTable(table_name, columns);
}
}