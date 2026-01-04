#include <gtest/gtest.h>

#include "executor/operators/seq_scan_op.h"
#include "executor/operators/filter_op.h"
#include "executor/operators/projection_op.h"
#include "executor/operators/limit_op.h"
#include "executor/test_db_helper.h"
#include "executor/executor.h"

using namespace db;
using common::Value;

TEST(ExecutorTest, ExecuteSeqScan) {
    TestDB db{"exec_execute_seq_scan.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1}
    };

    db.table_mgr->CreateTable("nums", schema);
    auto table = db.table_mgr->OpenTable("nums");

    for (uint32_t i = 0; i < 5; i++) {
        table->Insert({Value{i}});
    }

    auto plan = std::make_unique<executor::SeqScanOp>(*table);
    executor::Executor exec{std::move(plan)};

    // should fully drain without crashing
    exec.Execute();

    SUCCEED(); // reaching here is success
}

TEST(ExecutorTest, ExecuteAndCollectSeqScan) {
    TestDB db{"exec_collect_seq_scan.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1}
    };

    db.table_mgr->CreateTable("nums", schema);
    auto table = db.table_mgr->OpenTable("nums");

    for (uint32_t i = 1; i <= 3; i++) {
        table->Insert({Value{i}});
    }

    auto plan = std::make_unique<executor::SeqScanOp>(*table);
    executor::Executor exec{std::move(plan)};

    auto res = exec.ExecuteAndCollect();

    ASSERT_EQ(res.size(), 3);
    EXPECT_EQ(std::get<uint32_t>(res[0].GetValues()[0]), 1);
    EXPECT_EQ(std::get<uint32_t>(res[1].GetValues()[0]), 2);
    EXPECT_EQ(std::get<uint32_t>(res[2].GetValues()[0]), 3);
}

TEST(ExecutorTest, ExecuteAndCollectWithFilter) {
    TestDB db{"exec_collect_filter.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1}
    };

    db.table_mgr->CreateTable("nums", schema);
    auto table = db.table_mgr->OpenTable("nums");

    for (uint32_t i = 1; i <= 5; i++) {
        table->Insert({Value{i}});
    }

    auto scan = std::make_unique<executor::SeqScanOp>(*table);
    auto filter = std::make_unique<executor::FilterOp>(
        std::move(scan), [](const common::Tuple& t) {
            return std::get<uint32_t>(t.GetValues()[0]) > 3;
        }
    );

    executor::Executor exec{std::move(filter)};
    auto res = exec.ExecuteAndCollect();

    ASSERT_EQ(res.size(), 2);
    EXPECT_EQ(std::get<uint32_t>(res[0].GetValues()[0]), 4);
    EXPECT_EQ(std::get<uint32_t>(res[1].GetValues()[0]), 5);
}

TEST(ExecutorTest, ExecuteAndCollectFullPipeline) {
    TestDB db{"exec_collect_pipeline.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1},
        {"name", catalog::TEXT_TYPE, 2}
    };

    auto table_id = db.table_mgr->CreateTable("students", schema);
    auto table = db.table_mgr->OpenTable("students");

    table->Insert({Value{1}, Value{"Alice"}});
    table->Insert({Value{2}, Value{"Bob"}});
    table->Insert({Value{3}, Value{"Carol"}});
    table->Insert({Value{4}, Value{"Dave"}});

    auto seq_scan = std::make_unique<executor::SeqScanOp>(*table);
    auto filter = std::make_unique<executor::FilterOp>(
        std::move(seq_scan), [](const common::Tuple& t) {
            return std::get<uint32_t>(t.GetValues()[0]) >= 2;
        }
    );
    std::unordered_set<uint16_t> cols_to_project = {2};
    common::Schema out_schema = {
        {table_id, "name", catalog::TEXT_TYPE, 2}
    };
    auto proj = std::make_unique<executor::ProjectionOp>(
        std::move(filter),
        cols_to_project,
        std::make_shared<const common::Schema>(out_schema)
    );
    auto limit = std::make_unique<executor::LimitOp>(
        std::move(proj), 2
    );

    executor::Executor exec{std::move(limit)};
    auto res = exec.ExecuteAndCollect();

    ASSERT_EQ(res.size(), 2);
    EXPECT_EQ(std::get<std::string>(res[0].GetValues()[0]), "Bob");
    EXPECT_EQ(std::get<std::string>(res[1].GetValues()[0]), "Carol");
}
