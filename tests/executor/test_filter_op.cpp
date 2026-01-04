#include <gtest/gtest.h>
#include "executor/operators/filter_op.h"
#include "executor/operators/seq_scan_op.h"
#include "executor/test_db_helper.h"

using namespace db;
using common::Value;

TEST(FilterOpTest, FiltersRows) {
    TestDB db{"filter_test.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1},
        {"name", catalog::TEXT_TYPE, 2}
    };

    db.table_mgr->CreateTable("students", schema);
    auto table = db.table_mgr->OpenTable("students");

    table->Insert({Value{1}, Value{"Alice"}});
    table->Insert({Value{2}, Value{"Bob"}});
    table->Insert({Value{3}, Value{"Carol"}});

    auto scan = executor::SeqScanOp(*table);
    
    auto lambd = [](const common::Tuple& t) {
        return std::get<uint32_t>(t.GetValues()[0]) >= 2;
    };

    executor::Predicate pred{lambd};
    executor::FilterOp filter{std::make_unique<executor::SeqScanOp>(scan), pred};

    filter.Open();

    auto t1 = filter.Next();
    ASSERT_TRUE(t1.has_value());
    EXPECT_EQ(std::get<uint32_t>(t1->GetValues()[0]), 2);

    auto t2 = filter.Next();
    ASSERT_TRUE(t2.has_value());
    EXPECT_EQ(std::get<uint32_t>(t2->GetValues()[0]), 3);

    EXPECT_FALSE(filter.Next().has_value());

    filter.Close();
}

TEST(CatalogFilterTest, FilterDbTablesByName) {
    TestDB db{"catalog_filter_tables.db"};

    auto tables_rel = db.catalog->GetTablesCatalog();
    auto scan = std::make_unique<executor::SeqScanOp>(*tables_rel);

    executor::Predicate pred{[](const common::Tuple& tup) {
        // schema: table_id, table_name, heap_file_id, first_page_id
        auto name = std::get<std::string>(tup.GetValues()[1]);
        return name == catalog::DB_TABLES_TABLE;
    }};

    executor::FilterOp filter{std::move(scan), pred};
    filter.Open();

    auto result = filter.Next();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
        std::get<std::string>(result->GetValues()[1]),
        catalog::DB_TABLES_TABLE
    );

    EXPECT_FALSE(filter.Next().has_value());
    filter.Close();
}

TEST(CatalogFilterTest, FilterAttributesByColumnName) {
    TestDB db{"catalog_filter_attrs.db"};

    auto attrs_rel = db.catalog->GetAttributesCatalog();
    auto scan = std::make_unique<executor::SeqScanOp>(*attrs_rel);

    executor::Predicate pred{[](const common::Tuple& tup) {
        // schema: table_id, col_name, type_id, ordinal_position
        const auto& values = tup.GetValues();
        auto col_name = std::get<std::string>(values[1]); // pos = 2, index 1
        return col_name == "col_name";
    }};

    executor::FilterOp filter{std::move(scan), pred};
    filter.Open();

    auto result = filter.Next();
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(
        std::get<std::string>(result->GetValues()[1]),
        "col_name"
    );

    EXPECT_FALSE(filter.Next().has_value());
    filter.Close();
}

TEST(CatalogFilterTest, FilterTypesById) {
    TestDB db{"catalog_filter_types.db"};

    auto types_rel = db.catalog->GetTypesCatalog();
    auto scan = std::make_unique<executor::SeqScanOp>(*types_rel);

    executor::Predicate pred{[](const common::Tuple& tup) {
        // schema: type_id, size
        auto type_id = std::get<uint32_t>(tup.GetValues()[0]);
        return type_id > 0;
    }};

    executor::FilterOp filter{std::move(scan), pred};
    filter.Open();

    auto result = filter.Next();
    EXPECT_TRUE(result.has_value());

    filter.Close();
}
