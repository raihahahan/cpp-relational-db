#include "access/heap/heap_file.h"
#include "storage/buffer_manager/replacement_policies/replacement.h"
#include <gtest/gtest.h>

namespace db::access {

TEST(HeapFileTest, InsertAndGet) {
    DiskManager dm("heap_file_insert_get.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    auto rid = hf.Insert("hello", 6);
    ASSERT_TRUE(rid.has_value());

    auto rec = hf.Get(*rid);
    ASSERT_TRUE(rec.has_value());
    ASSERT_STREQ(rec->data, "hello");
}

TEST(HeapFileTest, UpdateRecord) {
    DiskManager dm("heap_file_update.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    auto rid = hf.Insert("foo", 4).value();
    ASSERT_TRUE(hf.Update("bar", 4, rid));

    auto rec = hf.Get(rid);
    ASSERT_TRUE(rec.has_value());
    ASSERT_STREQ(rec->data, "bar");
}

TEST(HeapFileTest, DeleteRecord) {
    DiskManager dm("heap_file_delete.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    auto rid = hf.Insert("foo", 4).value();
    ASSERT_TRUE(hf.Delete(rid));

    auto rec = hf.Get(rid);
    ASSERT_FALSE(rec.has_value());
}

TEST(HeapFileTest, MultiPageInsert) {
    DiskManager dm("heap_file_multipage.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    const int N = 1000;
    for (int i = 0; i < N; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "rec-%d", i);
        ASSERT_TRUE(hf.Insert(buf, strlen(buf) + 1).has_value());
    }

    // spot-check
    for (int i = 0; i < 10; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "rec-%d", i);
        auto rec = hf.Get({0, static_cast<uint16_t>(i)});
        if (rec.has_value()) {
            ASSERT_STREQ(rec->data, buf);
        }
    }
}
};
