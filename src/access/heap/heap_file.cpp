#include "access/heap/heap_file.h"
#include "storage/page/slotted_page.h"

namespace db::access {

HeapFile::HeapFile(BufferManager& bm, 
                    file_id_t file_id,
                    page_id_t first_page_id) :
                    _bm{bm}, 
                    _file_id{file_id}, 
                    _first_page_id{first_page_id} {};

void HeapFile::InitHeapPage(char* raw_page) {
    auto* heap_hdr = reinterpret_cast<HeapPageHeader*>(raw_page);
    heap_hdr->next_page_id = INVALID_PAGE_ID;
    db::storage::SlottedPage::Init(raw_page + sizeof(HeapPageHeader));
};

RID HeapFile::Insert(const char* data, size_t len) {

};

std::optional<Record> HeapFile::Get(const RID& rid) {

};

bool HeapFile::Delete(const RID& rid) {

};
}