#include "storage/buffer_manager/free_list.h"
#include "storage/buffer_manager/frame.h"
#include "config/config.h"

#include <gtest/gtest.h>

namespace db::storage {

class FreeListTest : public ::testing::Test {
protected:
    FreeList fl;
    Frame f1;
    Frame f2;
    Frame f3;

    void SetUp() override {
        // initialise frames with sane defaults
        f1.page_id = INVALID_PAGE_ID;
        f1.pin_count = 0;
        f1.dirty = false;

        f2.page_id = INVALID_PAGE_ID;
        f2.pin_count = 0;
        f2.dirty = false;

        f3.page_id = INVALID_PAGE_ID;
        f3.pin_count = 0;
        f3.dirty = false;
    }
};

TEST_F(FreeListTest, InitiallyEmpty) {
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);
}

TEST_F(FreeListTest, AddSingleFrame) {
    fl.add(&f1);
    EXPECT_FALSE(fl.empty());
    EXPECT_EQ(fl.size(), 1);

    Frame* retrieved = fl.get();
    EXPECT_EQ(retrieved, &f1);
    EXPECT_TRUE(fl.empty());
}

TEST_F(FreeListTest, AddMultipleFramesLIFOOrder) {
    fl.add(&f1);
    fl.add(&f2);
    fl.add(&f3);

    EXPECT_EQ(fl.size(), 3);

    // LIFO: last added = first returned
    EXPECT_EQ(fl.get(), &f3);
    EXPECT_EQ(fl.get(), &f2);
    EXPECT_EQ(fl.get(), &f1);

    EXPECT_TRUE(fl.empty());
}

TEST_F(FreeListTest, GetOnEmptyReturnsNullptr) {
    EXPECT_EQ(fl.get(), nullptr);

    fl.add(&f1);
    EXPECT_NE(fl.get(), nullptr);

    // now empty again
    EXPECT_EQ(fl.get(), nullptr);
}

TEST_F(FreeListTest, SizeReflectsAddAndGet) {
    fl.add(&f1);
    fl.add(&f2);

    EXPECT_EQ(fl.size(), 2);
    fl.get();
    EXPECT_EQ(fl.size(), 1);
    fl.get();
    EXPECT_EQ(fl.size(), 0);
}

}
