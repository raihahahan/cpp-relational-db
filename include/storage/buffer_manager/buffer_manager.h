#include "buffer_manager/free_list.h"
#include "buffer_manager/frame.h"
#include "buffer_manager/policy/replacement.h"
#include "disk_manager/disk_manager.h"


namespace db::storage {
class BufferManager {
public:
    BufferManager(size_t pool_size, std::unique_ptr<IReplacementPolicy> policy);

    Frame* request(page_id_t pid);
    void release(page_id_t pid);
    void markDirty(page_id_t pid);
    void flushAll();

private:
    Frame* evict();
    void read(page_id_t pid, Frame* f);
    void flush(Frame* f);
    void pin(Frame* f);
    void unpin(Frame* f);

    std::unordered_map<page_id_t, Frame*> page_table;
    std::vector<Frame> frames;
    std::vector<Frame*> frame_ptrs;   // used by replacement policy
    FreeList free_list;
    std::unique_ptr<IReplacementPolicy> policy;
    DiskManager* disk;
};
}