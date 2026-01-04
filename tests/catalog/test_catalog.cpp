#include <gtest/gtest.h>

#include "util/uuid.h"
#include "storage/disk_manager/disk_manager.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "config/config.h"
#include "catalog/catalog.h"
#include <vector>

#include <filesystem>

using namespace db;
using namespace db::catalog;

class CatalogTest : public ::testing::Test {
protected:
    void SetUp() override {
        // isolate each test
        db_file = "test_catalog.db";
        std::filesystem::remove(db_file);

        dm = std::make_unique<storage::DiskManager>(db_file);
        bm = std::make_unique<storage::BufferManager>(
            storage::ReplacementPolicyType::CLOCK,
            dm.get()
        );

        catalog = std::make_unique<Catalog>(bm.get(), dm.get());
    }

    void TearDown() override {
        catalog.reset();
        bm.reset();
        dm.reset();
        std::filesystem::remove(db_file);
    }

    std::string db_file;
    std::unique_ptr<storage::DiskManager> dm;
    std::unique_ptr<storage::BufferManager> bm;
    std::unique_ptr<Catalog> catalog;
};

TEST_F(CatalogTest, BootstrapInitialisesCatalog) {
    EXPECT_FALSE(catalog->IsInitialised());

    catalog->Init();

    EXPECT_TRUE(catalog->IsInitialised());

    // catalog tables should exist
    auto tables = catalog->LookupTable(DB_TABLES_TABLE);
    auto attrs = catalog->LookupTable(DB_ATTRIBUTES_TABLE);
    auto types = catalog->LookupTable(DB_TYPES_TABLE);

    EXPECT_TRUE(tables.has_value());
    EXPECT_TRUE(attrs.has_value());
    EXPECT_TRUE(types.has_value());
}

TEST_F(CatalogTest, RestartLoadsCatalog) {
    catalog->Init();
    ASSERT_TRUE(catalog->IsInitialised());

    // simulate restart
    catalog.reset();
    bm.reset();
    dm.reset();

    dm = std::make_unique<storage::DiskManager>(db_file);
    bm = std::make_unique<storage::BufferManager>(
        storage::ReplacementPolicyType::CLOCK,
        dm.get()
    );
    catalog = std::make_unique<Catalog>(bm.get(), dm.get());

    EXPECT_TRUE(catalog->IsInitialised());

    catalog->Init();

    auto tables = catalog->LookupTable(DB_TABLES_TABLE);
    EXPECT_TRUE(tables.has_value());
}

TEST_F(CatalogTest, CreateTableInsertsMetadata) {
    catalog->Init();

    std::vector<RawColumnInfo> cols = {
        { "id", INT_TYPE, 1 },
        { "name", TEXT_TYPE, 2 }
    };

    auto table_id = catalog->CreateTable("users", cols);
    auto table = catalog->LookupTable("users");
    ASSERT_TRUE(table.has_value());

    EXPECT_EQ(table->table_id, table_id);
    EXPECT_EQ(table->table_name, "users");
}

TEST_F(CatalogTest, TableColumnsArePersisted) {
    catalog->Init();

    std::vector<RawColumnInfo> cols = {
        { "id", INT_TYPE, 1 },
        { "name", TEXT_TYPE, 2 },
        { "age", INT_TYPE, 3 }
    };

    auto table_id = catalog->CreateTable("people", cols);
    auto stored_cols = catalog->GetTableColumns(table_id);

    ASSERT_EQ(stored_cols.size(), 3);

    EXPECT_EQ(stored_cols[0].col_name, "id");
    EXPECT_EQ(stored_cols[1].col_name, "name");
    EXPECT_EQ(stored_cols[2].col_name, "age");
}

