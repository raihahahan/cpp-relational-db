#include <gtest/gtest.h>
#include "executor/operators/seq_scan_op.h"
#include "executor/operators/filter_op.h"
#include "executor/operators/projection_op.h"
#include "executor/test_db_helper.h"
#include "catalog/catalog_bootstrap.h"

using namespace db;
using common::Value;
using catalog::ColumnInfo;

class DMLFoundationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<TestDB>("dml_foundation_test.db");

        std::vector<catalog::RawColumnInfo> cols = {
            {"id", catalog::INT_TYPE, 1},
            {"name", catalog::TEXT_TYPE, 2},
            {"age", catalog::INT_TYPE, 3}
        };

        table_id = db->table_mgr->CreateTable("users", cols);
        table = db->table_mgr->OpenTable("users");

        table->Insert({Value{uint32_t{1}}, Value{"Alice"}, Value{uint32_t{25}}});
        table->Insert({Value{uint32_t{2}}, Value{"Bob"}, Value{uint32_t{30}}});
        table->Insert({Value{uint32_t{3}}, Value{"Carol"}, Value{uint32_t{35}}});
    }

    std::unique_ptr<TestDB> db;
    catalog::table_id_t table_id;
    std::shared_ptr<model::UserTable> table;
};

TEST_F(DMLFoundationTest, SeqScanPopulatesRID) {
    executor::SeqScanOp scan{*table};
    scan.Open();

    int count = 0;
    while (auto tup = scan.Next()) {
        ASSERT_TRUE(tup->GetRID().has_value())
            << "Row " << count << " should have a RID";
        auto rid = tup->GetRID().value();
        EXPECT_GE(rid.page_id, 0);
        count++;
    }
    EXPECT_EQ(count, 3);

    scan.Close();
}

TEST_F(DMLFoundationTest, FilterPreservesRID) {
    auto scan = std::make_unique<executor::SeqScanOp>(*table);

    executor::Predicate pred{[](const common::Tuple& t) {
        return std::get<uint32_t>(t.GetValues()[0]) >= 2;
    }};

    executor::FilterOp filter{std::move(scan), pred};
    filter.Open();

    int count = 0;
    while (auto tup = filter.Next()) {
        ASSERT_TRUE(tup->GetRID().has_value())
            << "Filtered row " << count << " should have a RID";
        count++;
    }
    EXPECT_EQ(count, 2);

    filter.Close();
}

TEST_F(DMLFoundationTest, ProjectionPreservesRID) {
    auto scan = std::make_unique<executor::SeqScanOp>(*table);

    std::unordered_set<uint16_t> cols = {1, 2};
    common::Schema out_schema = {
        ColumnInfo{table_id, "id", catalog::INT_TYPE, 1},
        ColumnInfo{table_id, "name", catalog::TEXT_TYPE, 2}
    };

    executor::ProjectionOp proj{
        std::move(scan),
        cols,
        std::make_shared<const common::Schema>(out_schema)
    };

    proj.Open();

    int count = 0;
    while (auto tup = proj.Next()) {
        ASSERT_TRUE(tup->GetRID().has_value())
            << "Projected row " << count << " should have a RID";
        EXPECT_EQ(tup->GetValues().size(), 2);
        count++;
    }
    EXPECT_EQ(count, 3);

    proj.Close();
}

TEST_F(DMLFoundationTest, DeleteRemovesRow) {
    executor::SeqScanOp scan{*table};
    scan.Open();

    auto first = scan.Next();
    ASSERT_TRUE(first.has_value());
    auto rid = first->GetRID().value();
    scan.Close();

    EXPECT_TRUE(table->Delete(rid));

    executor::SeqScanOp rescan{*table};
    rescan.Open();

    int count = 0;
    while (auto tup = rescan.Next()) {
        EXPECT_NE(std::get<uint32_t>(tup->GetValues()[0]), uint32_t{1})
            << "Deleted row should not appear";
        count++;
    }
    EXPECT_EQ(count, 2);

    rescan.Close();
}

TEST_F(DMLFoundationTest, UpdateModifiesRow) {
    executor::SeqScanOp scan{*table};
    scan.Open();

    access::RID target_rid{};
    while (auto tup = scan.Next()) {
        if (std::get<uint32_t>(tup->GetValues()[0]) == 2) {
            target_rid = tup->GetRID().value();
            break;
        }
    }
    scan.Close();

    std::vector<Value> new_values = {
        Value{uint32_t{2}}, Value{"Bobby"}, Value{uint32_t{31}}
    };
    EXPECT_TRUE(table->Update(target_rid, new_values));

    executor::SeqScanOp rescan{*table};
    rescan.Open();

    bool found_updated = false;
    while (auto tup = rescan.Next()) {
        if (std::get<uint32_t>(tup->GetValues()[0]) == 2) {
            EXPECT_EQ(std::get<std::string>(tup->GetValues()[1]), "Bobby");
            EXPECT_EQ(std::get<uint32_t>(tup->GetValues()[2]), uint32_t{31});
            found_updated = true;
        }
    }
    EXPECT_TRUE(found_updated) << "Updated row should appear with new values";

    rescan.Close();
}

TEST_F(DMLFoundationTest, DeleteAllRows) {
    executor::SeqScanOp scan{*table};
    scan.Open();

    std::vector<access::RID> rids;
    while (auto tup = scan.Next()) {
        rids.push_back(tup->GetRID().value());
    }
    scan.Close();

    EXPECT_EQ(rids.size(), 3);
    for (const auto& rid : rids) {
        EXPECT_TRUE(table->Delete(rid));
    }

    executor::SeqScanOp rescan{*table};
    rescan.Open();
    EXPECT_FALSE(rescan.Next().has_value()) << "All rows should be deleted";
    rescan.Close();
}
