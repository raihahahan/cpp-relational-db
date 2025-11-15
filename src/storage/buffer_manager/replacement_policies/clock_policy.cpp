#include "storage/buffer_manager/replacement_policies/clock_policy.h"

namespace db::storage {
ClockPolicy::ClockPolicy(std::vector<Frame*>& frames) : frames_{frames}, hand_{0} {
    N_ = frames.size();
    ref_bits_.resize(N_, 0);
    for (size_t i = 0; i < N_; ++i) {
        frame_idx_[frames[i]] = i;
    }
}   

Frame* ClockPolicy::choose_victim() {
    for (size_t scanned = 0; scanned < 2 * N_; ++scanned) {
        Frame* f = frames_[hand_];
        size_t idx = hand_;

        // only consider unpinned
        if (f->pin_count == 0) {
            if (ref_bits_[idx] == 0) {
                return f;
            }

            // second chance, clear ref bit
            ref_bits_[idx] = 0;
        }

        advance_hand();
    }

    // throw error if can't find frame
    return nullptr;
}

void ClockPolicy::advance_hand() {
    hand_ = (hand_ + 1) % N_;
}

void ClockPolicy::record_access(Frame* f) {
    // page HIT
    // recently accessed
    size_t idx = frame_idx_[f];
    ref_bits_[idx] = 1;
}

void ClockPolicy::record_load(Frame* f) {
    // page LOAD into frame
    // most recently used
    size_t idx = frame_idx_[f];
    ref_bits_[idx] = 1;

}

void ClockPolicy::record_unpin(Frame* f) {
    // when the pin count drops to 0,
	// page has recently been unpinned. 
    // try not to replace it immediately (i.e second chance)
    size_t idx = frame_idx_[f];
    ref_bits_[idx] = 0;
}
}