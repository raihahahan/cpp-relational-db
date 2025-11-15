#pragma once
#include "storage/buffer_manager/frame.h"

namespace db::storage {
class IReplacementPolicy {
public:
    virtual ~IReplacementPolicy() = default;

    virtual void record_access(Frame*) = 0; // page hit
    virtual void record_load(Frame* f) = 0; // page loaded into a frame
    virtual void record_unpin(Frame* f) = 0; // unpinned, may become candidate
    virtual Frame* choose_victim() = 0;
};
}