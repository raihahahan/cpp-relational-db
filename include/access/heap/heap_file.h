#pragma once

#include <cstdint>
#include <optional>
#include "access/record.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "storage/disk_manager/disk_manager.h"
#include "storage/page/slotted_page.h"
#include "access/heap/heap_iterator.h"

#define INVALID_PAGE_ID -1
using BufferManager = db::storage::BufferManager;
using DiskManager = db::storage::DiskManager;

namespace db::access {
using file_id_t = uint16_t;
using page_id_t = db::storage::page_id_t;

struct alignas(8) HeapPageHeader {
    page_id_t next_page_id;
};
static_assert(sizeof(HeapPageHeader) % 8 == 0);

class HeapFile {
public:
    explicit HeapFile(BufferManager* bm, 
                        DiskManager* dm,
                        file_id_t file_id, 
                        page_id_t first_page_id);
    HeapFile() = default;
    
    std::optional<RID> Insert(const char* data, size_t len);
    std::optional<Record> Get(const RID& rid);
    bool Update(const char* new_data, size_t len, const RID& rid);
    bool Delete(const RID& rid);

    // accessor
    BufferManager* GetBm() const;
    page_id_t GetPageId() const { return _first_page_id; };

    // iterator
    HeapIterator begin();   
    HeapIterator end();

    // factory
    static HeapFile Create(BufferManager* bm,
                            DiskManager* dm,
                            file_id_t fid);
    static HeapFile Open(BufferManager* bm,
                            DiskManager* dm,
                            file_id_t fid,
                            page_id_t first_page_id);

private:
    void InitHeapPage(char* raw_page_data);
    BufferManager* _bm;
    DiskManager* _dm;
    file_id_t _file_id;
    page_id_t _first_page_id;

    friend class HeapIterator;
};
}