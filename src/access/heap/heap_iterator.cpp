#include "access/heap/heap_iterator.h"
#include "access/heap/heap_file.h"
#include "storage/page/slotted_page.h"
#include "storage/buffer_manager/frame.h"

using SlottedPage = db::storage::SlottedPage;

namespace db::access {
HeapIterator::HeapIterator(HeapFile* heap,
                 page_id_t page,
                 uint16_t slot,
                 bool _has_next)
    : _heap(heap),
      _curr_page{page},
      _curr_slot{slot},
      _has_next{_has_next} {
    
    if (_has_next && _curr_page != INVALID_PAGE_ID) {
        Advance();
    }
}

bool HeapIterator::HasNext() {
    return _has_next;
}

Record HeapIterator::Next() {
    if (!_has_next) {
        throw std::runtime_error("HeapIterator::Next called with no next");
    }

    RID rid{_curr_page, _curr_slot};
    auto rec = _heap->Get(rid).value();

    _curr_slot++;
    Advance();
    return rec;
}

void HeapIterator::Advance() {
    _has_next = false;

    while (_curr_page != INVALID_PAGE_ID) {
        db::storage::Frame* frame = _heap->GetBm()->request(_curr_page);
        auto sp = SlottedPage::FromBuffer(frame->data, sizeof(HeapPageHeader));

        while (_curr_slot < sp.GetNumSlots()) {
            if (sp.Get(_curr_slot).has_value()) {
                _has_next = true;
                _heap->GetBm()->release(_curr_page);
                return;
            }
            _curr_slot++;
        }

        auto* hdr = reinterpret_cast<HeapPageHeader*>(frame->data);
        page_id_t next = hdr->next_page_id;

        _heap->GetBm()->release(_curr_page);
        _curr_page = next;
        _curr_slot = 0;
    }
}

bool HeapIterator::operator==(const HeapIterator& other) const {
    return _heap == other._heap &&
           _curr_page == other._curr_page &&
           _curr_slot == other._curr_slot &&
           _has_next == other._has_next;
}

bool HeapIterator::operator!=(const HeapIterator& other) const {
    return !(*this == other);
}
}
