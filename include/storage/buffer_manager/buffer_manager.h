#pragma once
#include "storage/buffer_manager/free_list.h"
#include "storage/buffer_manager/frame.h"
#include "storage/buffer_manager/replacement_policies/replacement.h"
#include "storage/disk_manager/idisk_manager.h"


namespace db::storage {

enum class ReplacementPolicyType { CLOCK };
class BufferManager {
public:
    BufferManager(ReplacementPolicyType type, IDiskManager* dm);

    Frame* request(page_id_t pid);
    void release(page_id_t pid);
    void mark_dirty(Frame* frame);
    void flush_all();

private:
    Frame* evict();
    void read(page_id_t pid, Frame* f);
    void flush(Frame* f);
    void pin(Frame* f);
    void unpin(Frame* f);

    std::unordered_map<page_id_t, Frame*> page_table_;
    std::vector<Frame> pool_;
    std::vector<Frame*> frame_ptrs_;   // used by replacement policy
    FreeList free_list_;
    std::unique_ptr<IReplacementPolicy> policy_;
    IDiskManager* disk_;
};
}