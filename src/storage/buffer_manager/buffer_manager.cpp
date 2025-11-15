#include "storage/buffer_manager/buffer_manager.h"
#include "storage/buffer_manager/replacement_policies/clock_policy.h"
#include "config/config.h"

namespace db::storage {
BufferManager::BufferManager(ReplacementPolicyType type, IDiskManager* dm)
    : disk_(dm) {
    pool_.resize(config::BUFFER_POOL_SIZE);
    frame_ptrs_.reserve(config::BUFFER_POOL_SIZE);

    for (size_t i = 0; i < config::BUFFER_POOL_SIZE; ++i) {
        Frame* f = &pool_[i];
        frame_ptrs_.push_back(f);

        // init metadata
        f->page_id = INVALID_PAGE_ID;
        f->pin_count = 0;
        f->dirty = 0;
        f->data = new char[config::PAGE_SIZE];

        free_list_.add(f);
    }

    if (type == ReplacementPolicyType::CLOCK) {
        policy_ = std::make_unique<ClockPolicy>(frame_ptrs_);
    } else {
        throw std::runtime_error("BufferManager: Unknown replacement policy type.");
    }
};

Frame* BufferManager::request(page_id_t pid) {
    auto it = page_table_.find(pid);

    // case 1: p is in some frame. page hit
    if (it != page_table_.end()) {
        Frame* frame = it->second;
        pin(frame);
        policy_->record_access(frame);
        return frame;
    }

    Frame* frame = nullptr;

    // case 2: p is not in some frame
    // check if free list has available frames
    if (!free_list_.empty()) {
        // case 2a: free list is not empty
        // 1. move some frame f' from freelist to pool
        // 2. increment pin count of f'
        // 3. read p into f'
        // 4. return f'
        frame = free_list_.get();
    } else {
        frame = evict();

    }

    pin(frame);
    read(pid, frame); // loads page + sets page_table_[pid]
    policy_->record_load(frame);
    return frame;
}

void BufferManager::release(page_id_t pid) {
    auto it = page_table_.find(pid);

    if (it == page_table_.end()) {
        throw std::runtime_error("BufferManager::release(): page not found");
    }

    Frame* f = it->second;
    if (f->pin_count <= 0) {
        throw std::runtime_error("BufferManager::release(): pin count already 0.");
    }

    unpin(f);
    if (f->pin_count == 0) {
        policy_->record_unpin(f);
    }
}

void BufferManager::mark_dirty(Frame* frame) {
    frame->dirty = 1;
}

void BufferManager::flush_all() {
    for (Frame& f : pool_) {
        if (f.dirty) flush(&f);
    }
}

// private methods
Frame* BufferManager::evict() {
    auto victim = policy_->choose_victim();

    if (victim == nullptr) {
        throw std::runtime_error("BufferManager::evict(): no eviction candidates (all frames pinned)");
    }

    if (victim->pin_count != 0) {
        throw std::runtime_error("BufferManager::evict(): policy selected a pinned frame.");
    }

    if (victim->dirty) {
        flush(victim);
    }

    // remove old mapping
    if (victim->page_id != INVALID_PAGE_ID) {
        page_table_.erase(victim->page_id);
    }

    // reset frame metadata
    victim->page_id = INVALID_PAGE_ID;
    victim->dirty = 0;

    return victim;
}

void BufferManager::read(page_id_t pid, Frame* f) {
    disk_->ReadPage(pid, f->data);
    page_table_[pid] = f;
    f->page_id = pid;
    f->dirty = 0;
}

void BufferManager::flush(Frame* f) {
    disk_->WritePage(f->page_id, f->data);
    f->dirty = 0;
}

void BufferManager::pin(Frame* f) {
    ++f->pin_count;
}

void BufferManager::unpin(Frame* f) {
    --f->pin_count;
}
}