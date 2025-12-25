#include "access/heap/heap_file.h"
#include "access/heap/heap_iterator.h"
#include "storage/buffer_manager/replacement_policies/replacement.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

namespace db::access {
TEST(HeapIteratorTest, EmptyHeap) {
    DiskManager dm("heap_iter_empty.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    auto it = hf.begin();
    ASSERT_FALSE(it.HasNext());
}

TEST(HeapIteratorTest, SequentialScanSinglePage) {
    DiskManager dm("heap_iter_single.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    std::vector<std::string> expected;
    for (int i = 0; i < 10; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "v%d", i);
        expected.emplace_back(buf);
        hf.Insert(buf, strlen(buf) + 1);
    }

    auto it = hf.begin();
    std::vector<std::string> actual;

    while (it.HasNext()) {
        actual.emplace_back(it.Next().data);
    }

    ASSERT_EQ(actual.size(), expected.size());
    ASSERT_EQ(actual, expected);
}


TEST(HeapIteratorTest, SequentialScanMultiPage) {
    DiskManager dm("heap_iter_multi.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    const int N = 500;
    for (int i = 0; i < N; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "val-%d", i);
        hf.Insert(buf, strlen(buf) + 1);
    }

    auto it = hf.begin();
    int count = 0;

    while (it.HasNext()) {
        it.Next();
        count++;
    }

    ASSERT_EQ(count, N);
}

TEST(HeapIteratorTest, SkipDeletedRecords) {
    DiskManager dm("heap_iter_delete.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    auto r1 = hf.Insert("a", 2).value();
    auto r2 = hf.Insert("b", 2).value();
    auto r3 = hf.Insert("c", 2).value();

    hf.Delete(r2);

    auto it = hf.begin();
    std::vector<std::string> vals;

    while (it.HasNext()) {
        vals.emplace_back(it.Next().data);
    }

    ASSERT_EQ(vals.size(), 2);
    ASSERT_EQ(vals[0], "a");
    ASSERT_EQ(vals[1], "c");
}

TEST(HeapIteratorTest, EvictionStress) {
    DiskManager dm("heap_iter_eviction.db");
    BufferManager bm(db::storage::ReplacementPolicyType::CLOCK, &dm);

    HeapFile hf(&bm, &dm, 1, INVALID_PAGE_ID);

    const int N = 2000;
    for (int i = 0; i < N; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "x-%d", i);
        hf.Insert(buf, strlen(buf) + 1);
    }

    auto it = hf.begin();
    int count = 0;

    while (it.HasNext()) {
        it.Next();
        count++;
    }

    ASSERT_EQ(count, N);
}


}