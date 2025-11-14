#include "buffer_manager/frame.h"

namespace db::storage {
class IReplacementPolicy {
public:
    virtual ~IReplacementPolicy() = default;

    virtual void recordAccess(Frame*) = 0; // page hit
    virtual void recordLoad(Frame* f) = 0; // page loaded into a frame
    virtual void recordUnpin(Frame* f) = 0; // unpinned, may become candidate
    virtual Frame* chooseVictim(const std::vector<Frame*>& frames) = 0;
};
}