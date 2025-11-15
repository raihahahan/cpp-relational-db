#include "storage/buffer_manager/replacement_policies/clock_policy.h"
#include "storage/buffer_manager/frame.h"
#include "config/config.h"

#include <gtest/gtest.h>

namespace db::storage {

class ClockPolicyTest : public ::testing::Test {
protected:
    std::vector<Frame> pool;
    std::vector<Frame*> frames;
    ClockPolicy* policy;

    ClockPolicyTest() {
        // allocate 4 frames for easier testing
        pool.resize(4);
        for (int i = 0; i < 4; i++) {
            pool[i].pin_count = 0;
            pool[i].page_id = INVALID_PAGE_ID;
            pool[i].dirty = false;

            frames.push_back(&pool[i]);
        }
        policy = new ClockPolicy(frames);
    }

    ~ClockPolicyTest() override {
        delete policy;
    }
};

// ----------------------------
// Basic victim selection
// ----------------------------
TEST_F(ClockPolicyTest, ChoosesFirstUnpinnedFrame) {
    Frame* victim = policy->choose_victim();
    EXPECT_EQ(victim, frames[0]);
}

// ----------------------------
// Second chance behavior
// ----------------------------
TEST_F(ClockPolicyTest, UsesSecondChanceWhenRefBitIsSet) {
    // mark frame 0 as recently accessed
    policy->record_access(frames[0]);

    Frame* victim = policy->choose_victim();
    // should skip f0 because its refbit=1, clear it, and return f1
    EXPECT_EQ(victim, frames[1]);
}

// ----------------------------
// Hand rotates correctly
// ----------------------------
TEST_F(ClockPolicyTest, HandRotationSkipsPinnedFrames) {
    frames[0]->pin_count = 1; // pinned
    frames[1]->pin_count = 1; // pinned

    // so victim should be f2
    Frame* victim = policy->choose_victim();
    EXPECT_EQ(victim, frames[2]);
}

// ----------------------------
// record_unpin gives second chance
// ----------------------------
TEST_F(ClockPolicyTest, UnpinClearsRefBit) {
    policy->advance_hand();
    // mark accessed
    policy->record_access(frames[1]);
    // unpin => should clear its refbit
    policy->record_unpin(frames[1]);

    // so frame 1 is now a candidate again
    Frame* victim = policy->choose_victim();
    EXPECT_EQ(victim, frames[1]);
}

// ----------------------------
// Full-cycle second chance sweep
// ----------------------------
TEST_F(ClockPolicyTest, FullSweepSecondChance) {
    // mark all frames as accessed (refbit = 1)
    for (auto f : frames) {
        policy->record_access(f);
    }

    // now first CLOCK cycle: all bits get cleared
    Frame* first_try = policy->choose_victim();
    EXPECT_EQ(first_try, frames[0]) 
        << "After clearing bits, frame0 should be selected";

    // mark frame0 accessed again
    policy->record_access(frames[0]);

    // next victim should be frame1
    Frame* second_try = policy->choose_victim();
    EXPECT_EQ(second_try, frames[1]);
}

// ----------------------------
// All pinned => return nullptr
// ----------------------------
TEST_F(ClockPolicyTest, NoVictimWhenAllPinned) {
    for (auto f : frames) {
        f->pin_count = 1;
    }

    Frame* victim = policy->choose_victim();
    EXPECT_EQ(victim, nullptr);
}

}
