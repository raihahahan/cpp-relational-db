#pragma once
#include "storage/buffer_manager/replacement_policies/replacement.h"

namespace db::storage {
class ClockPolicy : public IReplacementPolicy {
public:
    explicit ClockPolicy(std::vector<Frame*>& frames);

    void record_access(Frame* f) override;
    void record_load(Frame* f) override;
    void record_unpin(Frame* f) override;
    void advance_hand();

    Frame* choose_victim() override;

private:
    size_t hand_;
    size_t N_;
    std::vector<int> ref_bits_;
    std::unordered_map<Frame*, size_t> frame_idx_;
    std::vector<Frame*>& frames_;
};
}
