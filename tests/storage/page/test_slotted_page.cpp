#include "storage/page/slotted_page.h"
#include "config/config.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace db::storage {

class SlottedPageTest : public ::testing::Test {
protected:
    // raw page buffer, aligned to avoid UB with reinterpret_cast
    alignas(8) char page[config::PAGE_SIZE];
    SlottedPage sp;

    void SetUp() override {
        std::memset(page, 0, sizeof(page));
        SlottedPage::Init(page, 0);
        sp = SlottedPage::FromBuffer(page, 0);
    }
};

// ----------------------------
// Init
// ----------------------------

TEST_F(SlottedPageTest, InitSetsHeaderCorrectly) {
    auto* header = reinterpret_cast<PageHeader*>(page);

    EXPECT_EQ(header->num_slots, 0);
    EXPECT_EQ(header->free_space_offset, config::PAGE_SIZE);
    EXPECT_GT(sp.FreeSpace(), 0u);
}

// ----------------------------
// Insert / Get
// ----------------------------

TEST_F(SlottedPageTest, InsertAndGetSingleRecord) {
    const char data[] = "hello";

    auto slot = sp.Insert(data, sizeof(data));
    ASSERT_TRUE(slot.has_value());

    auto result = sp.Get(*slot);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result.value().second, sizeof(data));
    EXPECT_EQ(std::memcmp(result.value().first.data(), data, sizeof(data)), 0);
}

TEST_F(SlottedPageTest, MultipleInsertsWork) {
    const char a[] = "a";
    const char b[] = "bb";
    const char c[] = "ccc";

    auto s1 = sp.Insert(a, sizeof(a));
    auto s2 = sp.Insert(b, sizeof(b));
    auto s3 = sp.Insert(c, sizeof(c));

    ASSERT_TRUE(s1.has_value());
    ASSERT_TRUE(s2.has_value());
    ASSERT_TRUE(s3.has_value());

    EXPECT_EQ(std::memcmp(sp.Get(*s1)->first.data(), a, sizeof(a)), 0);
    EXPECT_EQ(std::memcmp(sp.Get(*s2)->first.data(), b, sizeof(b)), 0);
    EXPECT_EQ(std::memcmp(sp.Get(*s3)->first.data(), c, sizeof(c)), 0);
}

// ----------------------------
// Delete (tombstone)
// ----------------------------

TEST_F(SlottedPageTest, DeleteCreatesTombstone) {
    const char data[] = "delete me";

    auto slot = sp.Insert(data, sizeof(data));
    ASSERT_TRUE(slot.has_value());

    EXPECT_TRUE(sp.Delete(*slot));

    auto result = sp.Get(*slot);
    EXPECT_FALSE(result.has_value());
}

// ----------------------------
// Update
// ----------------------------

TEST_F(SlottedPageTest, UpdateInPlaceWhenSmaller) {
    const char old_data[] = "hello world";
    const char new_data[] = "hi";

    auto slot = sp.Insert(old_data, sizeof(old_data));
    ASSERT_TRUE(slot.has_value());

    sp.Update(*slot, new_data, sizeof(new_data));

    auto result = sp.Get(*slot);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->second, sizeof(new_data));
    EXPECT_EQ(std::memcmp(result->first.data(), new_data, sizeof(new_data)), 0);
}

TEST_F(SlottedPageTest, UpdateRelocatesWhenLarger) {
    const char small[] = "a";
    const char large[] = "this is a much larger record";

    auto slot = sp.Insert(small, sizeof(small));
    ASSERT_TRUE(slot.has_value());

    sp.Update(*slot, large, sizeof(large));

    auto result = sp.Get(*slot);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->second, sizeof(large));
    EXPECT_EQ(std::memcmp(result->first.data(), large, sizeof(large)), 0);
}

// ----------------------------
// Free space / failure cases
// ----------------------------

TEST_F(SlottedPageTest, InsertFailsWhenOutOfSpace) {
    std::vector<char> big(config::PAGE_SIZE);

    auto slot = sp.Insert(big.data(), big.size());
    EXPECT_FALSE(slot.has_value());
}

TEST_F(SlottedPageTest, GetInvalidSlotReturnsNullopt) {
    auto result = sp.Get(1234);
    EXPECT_FALSE(result.has_value());
}

} 
