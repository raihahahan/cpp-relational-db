#include "access/heap/heap_file.h"
#include "storage/page/slotted_page.h"
#include "storage/buffer_manager/frame.h"
#include "config/config.h"

using SlottedPage = db::storage::SlottedPage;
using Frame = db::storage::Frame;
namespace db::access {

HeapFile::HeapFile(BufferManager* bm, 
                    DiskManager* dm,
                    file_id_t file_id,
                    page_id_t first_page_id) :
                    _bm{bm}, 
                    _dm{dm},
                    _file_id{file_id}, 
                    _first_page_id{first_page_id} {};

void HeapFile::InitHeapPage(char* raw_page) {
    auto* heap_hdr = reinterpret_cast<HeapPageHeader*>(raw_page);
    heap_hdr->next_page_id = INVALID_PAGE_ID;
    SlottedPage::Init(raw_page, sizeof(HeapPageHeader));
};

std::optional<Record> HeapFile::Get(const RID& rid) {
    auto* frame = _bm->request(rid.page_id);
    if (frame == nullptr) {
        _bm->release(rid.page_id);
        return std::nullopt;
    } 

    auto sp = SlottedPage::FromBuffer(frame->data, sizeof(HeapPageHeader));
    auto data = sp.Get(rid.slot_id);
    _bm->release(rid.page_id);

    if (data.has_value()) {
        return Record{rid, (*data).data()};
    }
    
    return std::nullopt;
};

std::optional<RID> HeapFile::Insert(const char* data, size_t len) {
    // defensive check for first page id
    if (_first_page_id == INVALID_PAGE_ID) {
        _first_page_id = _dm->AllocatePage();
        Frame* first = _bm->request(_first_page_id);
        InitHeapPage(first->data);
        _bm->mark_dirty(first);
        _bm->release(_first_page_id);
    }

    page_id_t page_id = _first_page_id;
    page_id_t last_page_id = INVALID_PAGE_ID;
    HeapPageHeader* hdr = nullptr;

    while (page_id != INVALID_PAGE_ID) {
        Frame* frame = _bm->request(page_id);
        auto sp = SlottedPage::FromBuffer(frame->data, sizeof(HeapPageHeader));
        auto slot_id = sp.Insert(data, len);
        if (slot_id.has_value()) {
            _bm->mark_dirty(frame);
            _bm->release(page_id);
            return RID{page_id, *slot_id};
        }

        hdr = reinterpret_cast<HeapPageHeader*>(frame->data);
        _bm->release(page_id);
        last_page_id = page_id;
        page_id = hdr->next_page_id;
    }

    // no page fits
    page_id_t new_page_id = _dm->AllocatePage();

    // re-fetch last page
    Frame* last = _bm->request(last_page_id);
    auto* last_hdr = reinterpret_cast<HeapPageHeader*>(last->data);
    last_hdr->next_page_id = new_page_id;
    _bm->mark_dirty(last);
    _bm->release(last_page_id);

    // init new page
    Frame* frame = _bm->request(new_page_id);
    HeapFile::InitHeapPage(frame->data);
    _bm->mark_dirty(frame);

    // insert data into page
    auto sp = SlottedPage::FromBuffer(frame->data, sizeof(HeapPageHeader));
    auto slot_id = sp.Insert(data, len);

    if (slot_id.has_value()) {
        _bm->release(new_page_id);
        return RID{new_page_id, *slot_id};
    }

    _bm->release(new_page_id);

    // at this point, still can't fit into an empty page
    // return std::nullopt first
    // future work: implement TOAST
    return std::nullopt;
};

bool HeapFile::Update(const char* new_data, size_t len, const RID& rid) {
    auto frame = _bm->request(rid.page_id);
    if (frame == nullptr) {
        _bm->release(rid.page_id);
        return false;
    }

    auto sp = SlottedPage::FromBuffer(frame->data, sizeof(HeapPageHeader));
    bool res = sp.Update(rid.slot_id, new_data, len);
    if (res) {
        _bm->mark_dirty(frame);
    }

    _bm->release(rid.page_id);
    return res;
}

bool HeapFile::Delete(const RID& rid) {
    auto frame = _bm->request(rid.page_id);
    if (frame == nullptr) {
        _bm->release(rid.page_id);
        return false;
    }

    auto sp = SlottedPage::FromBuffer(frame->data, sizeof(HeapPageHeader));
    bool res = sp.Delete(rid.slot_id);

    if (res) {
        _bm->mark_dirty(frame);
    }

    _bm->release(rid.page_id);
    return res;
};

BufferManager* HeapFile::GetBm() const {
    return _bm;
}

HeapIterator HeapFile::begin() {
    if (_first_page_id == INVALID_PAGE_ID) {
        return end();
    }
    return HeapIterator(this, _first_page_id, 0, true);
}

HeapIterator HeapFile::end() {
    return HeapIterator(this, static_cast<page_id_t>(INVALID_PAGE_ID), 0, false);
}

Record HeapIterator::operator*() {
    if (!_has_next) {
        throw std::runtime_error("Dereferencing end iterator");
    }

    RID rid{_curr_page, _curr_slot};
    return _heap->Get(rid).value();
}

HeapIterator& HeapIterator::operator++() {
    if (!_has_next) return *this;
    _curr_slot++;
    Advance();
    return *this;
}

HeapFile HeapFile::Create(BufferManager* bm, 
                            DiskManager* dm, 
                            file_id_t fid) {
    page_id_t pid = dm->AllocatePage();
    Frame* frame = bm->request(pid);
    HeapFile hf{bm, dm, fid, pid};
    hf.InitHeapPage(frame->data);
    bm->mark_dirty(frame);
    bm->release(pid);

    return hf;
}

HeapFile HeapFile::Open(BufferManager* bm, 
                            DiskManager* dm, 
                            file_id_t fid,
                            page_id_t first_page_id) {
    return HeapFile{bm, dm, fid, first_page_id};
}
}