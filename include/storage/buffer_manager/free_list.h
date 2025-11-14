#include <vector>
#include "buffer_manager/frame.h"

namespace db::storage {
class FreeList {
public:
    // adds an unused frame back to the freelist.
    // prerequisite: frame has pin_count == 0 and no page is assigned.
    void add(Frame* frame);

    // retrieves an unused frame for allocation
    // returns nullptr if freelist is empty.
    Frame* get();

    // number of available free frames
    size_t size() const noexcept;

    // true if no free frames left
    bool empty() const noexcept;

private:
    std::vector<Frame*> list_;
};
}