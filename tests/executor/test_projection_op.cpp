#include <gtest/gtest.h>
#include "executor/operators/projection_op.h"
#include "executor/operators/seq_scan_op.h"
#include "executor/test_db_helper.h"

using namespace db;
using common::Value;

TEST(ProjectionOpTest, ProjectsColumns) {
    TestDB db{"projection_test.db"};

    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1},
        {"name", catalog::TEXT_TYPE, 2}
    };

    auto table_id = db.table_mgr->CreateTable("students", schema);
    auto table = db.table_mgr->OpenTable("students");

    table->Insert({Value{1}, Value{"Alice"}});

    auto scan = executor::SeqScanOp(
        *table
    );

    std::unordered_set<uint16_t> cols = {2}; // project name only
    std::vector<ColumnInfo> out_schema;
    out_schema.emplace_back(table_id, 
                    schema[1].col_name, 
                    schema[1].type_id, 
                    schema[1].ordinal_position);

    executor::ProjectionOp proj{
        std::make_unique<executor::SeqScanOp>(scan),
        cols,
        std::make_shared<const common::Schema>(out_schema)
    };

    proj.Open();

    auto t = proj.Next();
    ASSERT_TRUE(t.has_value());
    ASSERT_EQ(t->GetValues().size(), 1);
    EXPECT_EQ(std::get<std::string>(t->GetValues()[0]), "Alice");

    EXPECT_FALSE(proj.Next().has_value());
    proj.Close();
}

TEST(CatalogProjectionTest, ProjectDbTableNames) {
    TestDB db{"catalog_proj_tables.db"};

    auto tables_rel = db.catalog->GetTablesCatalog();
    auto scan = executor::SeqScanOp(*tables_rel);

    // project table_name (pos = 2, 1-indexed)
    std::unordered_set<uint16_t> cols = {2};

    common::Schema out_schema = {
        catalog::TABLES_CATALOG_SCHEMA[1]
    };

    executor::ProjectionOp proj{
        std::make_unique<executor::SeqScanOp>(scan),
        cols,
        std::make_shared<const common::Schema>(out_schema)
    };

    proj.Open();

    bool saw_tables = false;

    while (true) {
        auto tup = proj.Next();
        if (!tup) break;

        ASSERT_EQ(tup->GetValues().size(), 1);
        auto name = std::get<std::string>(tup->GetValues()[0]);

        if (name == catalog::DB_TABLES_TABLE) {
            saw_tables = true;
        }
    }

    proj.Close();
    EXPECT_TRUE(saw_tables);
}

TEST(CatalogProjectionTest, ProjectAttributeNames) {
    TestDB db{"catalog_proj_attrs.db"};

    auto attrs_rel = db.catalog->GetAttributesCatalog();
    auto scan = executor::SeqScanOp(*attrs_rel);

    // project col_name (pos = 3)
    std::unordered_set<uint16_t> cols = {3};

    common::Schema out_schema = {
        catalog::ATTR_CATALOG_SCHEMA[2]
    };

    executor::ProjectionOp proj{
        std::make_unique<executor::SeqScanOp>(scan),
        cols,
        std::make_shared<const common::Schema>(out_schema)
    };

    proj.Open();

    auto tup = proj.Next();
    ASSERT_TRUE(tup.has_value());
    ASSERT_EQ(tup->GetValues().size(), 1);

    proj.Close();
}

TEST(CatalogProjectionTest, ProjectTypeSizes) {
    TestDB db{"catalog_proj_types.db"};

    auto types_rel = db.catalog->GetTypesCatalog();
    auto scan = std::make_unique<executor::SeqScanOp>(*types_rel);

    // project size (pos = 2)
    std::unordered_set<uint16_t> cols = {2};

    common::Schema out_schema = {
        catalog::TYPES_CATALOG_SCHEMA[1]
    };

    executor::ProjectionOp proj{
        std::move(scan),
        cols,
        std::make_shared<const common::Schema>(out_schema)
    };

    proj.Open();

    auto tup = proj.Next();
    ASSERT_TRUE(tup.has_value());
    ASSERT_EQ(tup->GetValues().size(), 1);

    proj.Close();
}

