#include "access/heap/heap_file.h"
#include "storage/page/slotted_page.h"
#include "storage/buffer_manager/frame.h"

using SlottedPage = db::storage::SlottedPage;
using Frame = db::storage::Frame;
namespace db::access {

HeapFile::HeapFile(BufferManager& bm, 
                    DiskManager& dm,
                    file_id_t file_id,
                    page_id_t first_page_id) :
                    _bm{bm}, 
                    _dm{dm},
                    _file_id{file_id}, 
                    _first_page_id{first_page_id} {};

void HeapFile::InitHeapPage(char* raw_page) {
    auto* heap_hdr = reinterpret_cast<HeapPageHeader*>(raw_page);
    heap_hdr->next_page_id = INVALID_PAGE_ID;
    SlottedPage::Init(raw_page + sizeof(HeapPageHeader));
};

std::optional<Record> HeapFile::Get(const RID& rid) {
    auto frame = _bm.request(rid.page_id);
    if (frame == nullptr) return std::nullopt;

    auto sp = SlottedPage::FromBuffer(frame->data + sizeof(HeapPageHeader));
    auto data = sp.Get(rid.slot_id);

    if (data.has_value()) {
        return Record{rid, (*data).data()};
    }
    
    return std::nullopt;
};

std::optional<RID> HeapFile::Insert(const char* data, size_t len) {
    page_id_t page_id = _first_page_id;

    while (page_id != INVALID_PAGE_ID) {
        Frame* frame = _bm.request(page_id);
        auto sp = SlottedPage::FromBuffer(frame->data + sizeof(HeapPageHeader));
        auto slot_id = sp.Insert(data, len);
        if (slot_id.has_value()) {
            return RID{page_id, *slot_id};
        }

        auto* hdr = reinterpret_cast<HeapPageHeader*>(frame->data);
        page_id = hdr->next_page_id;
    }

    // no page fits
    page_id = _dm.AllocatePage();
    auto frame = _bm.request(page_id);
    
    HeapFile::InitHeapPage(frame->data);
    auto sp = SlottedPage::FromBuffer(frame->data + sizeof(HeapPageHeader));
    auto slot_id = sp.Insert(data, len);

    if (slot_id.has_value()) {
        return RID{page_id, *slot_id};
    }

    // at this point, still can't fit into an empty page
    // return std::nullopt first
    // future work: implement TOAST
    return std::nullopt;
};

bool HeapFile::Update(const char* new_data, size_t len, const RID& rid) {
    auto frame = _bm.request(rid.page_id);
    if (frame == nullptr) return false;

    auto sp = SlottedPage::FromBuffer(frame->data + sizeof(HeapPageHeader));

    return sp.Update(rid.slot_id, new_data, len);
}

bool HeapFile::Delete(const RID& rid) {
    auto frame = _bm.request(rid.page_id);
    if (frame == nullptr) return false;

    auto sp = SlottedPage::FromBuffer(frame->data + sizeof(HeapPageHeader));

    return sp.Delete(rid.slot_id);
};
}