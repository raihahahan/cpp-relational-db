#pragma once

#include <cstdint>
#include <optional>
#include "access/record.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "storage/page/slotted_page.h"

#define INVALID_PAGE_ID -1
using BufferManager = db::storage::BufferManager;

namespace db::access {
using file_id_t = uint16_t;
using page_id_t = db::storage::page_id_t;

struct HeapPageHeader {
    page_id_t next_page_id;
};

class HeapFile {
public:
    explicit HeapFile(BufferManager& bm, 
                        file_id_t file_id, 
                        page_id_t first_page_id);
    
    RID Insert(const char* data, size_t len);
    std::optional<Record> Get(const RID& rid);
    bool Delete(const RID& rid);

private:
    void InitHeapPage(char* raw_page_data);
    BufferManager& _bm;
    file_id_t _file_id;
    page_id_t _first_page_id;
};
}