#pragma once

#include "storage/page/slotted_page.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "access/record.h"

namespace db::access {

class HeapFile;

class HeapIterator {
public:
    HeapIterator(HeapFile* heap,
                 page_id_t page,
                 uint16_t slot,
                 bool has_next);
    HeapIterator() = default;
    bool HasNext();
    Record Next();

    Record operator*();
    HeapIterator& operator++();

    bool operator==(const HeapIterator& other) const;
    bool operator!=(const HeapIterator& other) const;

private:
    HeapFile* _heap;
    page_id_t _curr_page;
    uint16_t _curr_slot;
    bool _has_next;

    void Advance();
};
}