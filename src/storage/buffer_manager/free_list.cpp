#include "storage/buffer_manager/free_list.h"
#include "storage/buffer_manager/frame.h"
#include "config/config.h"

namespace db::storage {
FreeList::FreeList() = default;

void FreeList::add(Frame* frame) {
    list_.push_back(frame);
};

Frame* FreeList::get() {
    if (empty()) {
        return nullptr;
    }

    Frame* frame = list_.back();
    list_.pop_back();
    return frame;
};

size_t FreeList::size() const noexcept {
    return list_.size();
};

bool FreeList::empty() const noexcept {
    return list_.empty();
};
}