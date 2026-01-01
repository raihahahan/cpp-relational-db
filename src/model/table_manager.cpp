#include "model/table_manager.h"
#include "access/heap/heap_file.h"

namespace db::model {
TableManager::TableManager(catalog::TablesCatalog* tables_catalog,
                            catalog::AttributesCatalog* attr_catalog,
                            storage::BufferManager* bm,
                            storage::DiskManager* dm) :
                        _tables_catalog{tables_catalog},
                        _attr_catalog{attr_catalog},
                        _bm{bm},
                        _dm{dm} {};

std::shared_ptr<UserTable> TableManager::OpenTable(std::string_view name) {
        if (_cache.contains(name)) return _cache[name];

        auto table_info = _tables_catalog->Lookup(name);
        if (!table_info.has_value()) throw std::runtime_error("Invalid table name.");
        
        auto cols = _attr_catalog->GetColumns(table_info->table_id);
        auto hf = access::HeapFile::Open(_bm, _dm, table_info->heap_file_id, table_info->first_page_id);

        auto rel = std::make_shared<UserTable>(std::move(hf), std::move(cols), table_info->table_id);
        _cache[name] = rel;
        return rel;
}   
}