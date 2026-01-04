#include <gtest/gtest.h>
#include "executor/operators/seq_scan_op.h"
#include "executor/test_db_helper.h"
#include "catalog/catalog_bootstrap.h"

using namespace db;
using common::Value;

TEST(SeqScanOpTest, ScansAllRows) {
    TestDB db{"seq_scan_test.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1},
        {"name", catalog::TEXT_TYPE, 2}
    };

    db.table_mgr->CreateTable("students", schema);
    auto table = db.table_mgr->OpenTable("students");

    table->Insert({Value{1}, Value{"Alice"}});
    table->Insert({Value{2}, Value{"Bob"}});

    executor::SeqScanOp scan{*table};

    scan.Open();

    auto t1 = scan.Next();
    ASSERT_TRUE(t1.has_value());
    EXPECT_EQ(std::get<uint32_t>(t1->GetValues()[0]), 1);

    auto t2 = scan.Next();
    ASSERT_TRUE(t2.has_value());
    EXPECT_EQ(std::get<uint32_t>(t2->GetValues()[0]), 2);

    EXPECT_FALSE(scan.Next().has_value());

    scan.Close();
}

TEST(CatalogSeqScanTest, ScanAllCatalogTables) {
    TestDB db{"catalog_seq_scan.db"};

    auto tables_rel = db.catalog->GetTablesCatalog();
    auto attrs_rel = db.catalog->GetAttributesCatalog();
    auto types_rel = db.catalog->GetTypesCatalog();

    auto scan_and_count = [](model::Relation& rel) {
        executor::SeqScanOp scan{rel};
        scan.Open();

        size_t count = 0;
        while (true) {
            auto tup = scan.Next();
            if (!tup) break;
            count++;
        }

        scan.Close();
        return count;
    };

    size_t tables_cnt = scan_and_count(*tables_rel);
    size_t attrs_cnt  = scan_and_count(*attrs_rel);
    size_t types_cnt  = scan_and_count(*tables_rel);

    EXPECT_GT(tables_cnt, 0); // system tables exist
    EXPECT_GT(attrs_cnt, 0);  // columns exist
    EXPECT_GT(types_cnt, 0);  // builtin types exist
}



