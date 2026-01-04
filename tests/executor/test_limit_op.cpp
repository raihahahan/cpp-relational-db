#include <gtest/gtest.h>

#include "executor/operators/limit_op.h"
#include "executor/operators/seq_scan_op.h"
#include "executor/operators/filter_op.h"
#include "executor/test_db_helper.h"

using namespace db;
using common::Value;

TEST(LimitOpTest, LimitsRows) {
    TestDB db{"limit_test.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1},
        {"name", catalog::TEXT_TYPE, 2}
    };

    db.table_mgr->CreateTable("students", schema);
    auto table = db.table_mgr->OpenTable("students");

    table->Insert({Value{1}, Value{"Alice"}});
    table->Insert({Value{2}, Value{"Bob"}});
    table->Insert({Value{3}, Value{"Carol"}});

    auto scan = std::make_unique<executor::SeqScanOp>(*table);
    executor::LimitOp limit{std::move(scan), 2};

    limit.Open();

    auto t1 = limit.Next();
    ASSERT_TRUE(t1.has_value());
    EXPECT_EQ(std::get<uint32_t>(t1->GetValues()[0]), 1);

    auto t2 = limit.Next();
    ASSERT_TRUE(t2.has_value());
    EXPECT_EQ(std::get<uint32_t>(t2->GetValues()[0]), 2);

    EXPECT_FALSE(limit.Next().has_value());

    limit.Close();
}

TEST(LimitOpTest, ZeroLimitReturnsNothing) {
    TestDB db{"limit_zero.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1}
    };

    db.table_mgr->CreateTable("nums", schema);
    auto table = db.table_mgr->OpenTable("nums");

    table->Insert({Value{1}});
    table->Insert({Value{2}});

    auto scan = std::make_unique<executor::SeqScanOp>(*table);
    executor::LimitOp limit{std::move(scan), 0};

    limit.Open();
    EXPECT_FALSE(limit.Next().has_value());
    limit.Close();
}

TEST(LimitOpTest, EmptyInput) {
    TestDB db{"limit_empty.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1}
    };

    db.table_mgr->CreateTable("empty", schema);
    auto table = db.table_mgr->OpenTable("empty");

    auto scan = std::make_unique<executor::SeqScanOp>(*table);
    executor::LimitOp limit{std::move(scan), 10};

    limit.Open();
    EXPECT_FALSE(limit.Next().has_value());
    limit.Close();
}

TEST(LimitOpTest, LimitAfterFilter) {
    TestDB db{"limit_filter.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1}
    };

    db.table_mgr->CreateTable("nums", schema);
    auto table = db.table_mgr->OpenTable("nums");

    for (uint32_t i = 1; i <= 10; i++) {
        table->Insert({Value{i}});
    }

    auto scan = std::make_unique<executor::SeqScanOp>(*table);

    executor::Predicate pred{
        [](const common::Tuple& t) {
            return std::get<uint32_t>(t.GetValues()[0]) % 2 == 0;
        }
    };

    auto filter = std::make_unique<executor::FilterOp>(std::move(scan), pred);
    executor::LimitOp limit{std::move(filter), 3};

    limit.Open();

    for (int i = 0; i < 3; i++) {
        auto tup = limit.Next();
        ASSERT_TRUE(tup.has_value());
    }

    EXPECT_FALSE(limit.Next().has_value());
    limit.Close();
}


