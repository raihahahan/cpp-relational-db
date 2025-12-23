#pragma once

#include <string>
#include <unordered_map>
#include "access/heap/heap_file.h"
namespace db::catalog {

struct TableMetadata {
    db::access::file_id_t file_id;
    db::access::page_id_t first_page_id;
};

class Catalog {
public:
    explicit Catalog(db::storage::BufferManager& bm);

    // lookup
    const TableMetadata& GetTable(const std::string& name) const;

    // DDL
    void CreateTable(const std::string& name /* schema later */);

    // access
    db::access::HeapFile OpenHeapFile(const std::string& name);

private:
    db::storage::BufferManager& bm_;
    // in-memory cache (backed by catalog heap tables)
    std::unordered_map<std::string, TableMetadata> tables_;
};

}
