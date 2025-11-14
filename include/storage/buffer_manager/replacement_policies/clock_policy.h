#include "replacement_policies/replacement.h"

namespace db::storage {
class ClockPolicy : public IReplacementPolicy {
public:
    explicit ClockPolicy(size_t pool_size);

    void recordAccess(Frame* f) override;
    void recordLoad(Frame* f) override;
    void recordUnpin(Frame* f) override;

    Frame* chooseVictim(const std::vector<Frame*>& frames) override;

private:
    size_t hand;
    std::vector<int> ref_bits;
};
}
