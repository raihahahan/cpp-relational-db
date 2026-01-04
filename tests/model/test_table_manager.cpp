#include <gtest/gtest.h>
#include <filesystem>
#include "model/table_manager.h"
#include "storage/disk_manager/disk_manager.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "catalog/catalog.h"

namespace db::model {

class TableManagerTest : public ::testing::Test {
protected:
    const std::string db_name = "test_manager.db";
    storage::DiskManager* dm;
    storage::BufferManager* bm;
    catalog::Catalog* cat;

    void SetUp() override {
        if (std::filesystem::exists(db_name)) {
            std::filesystem::remove(db_name);
        }
        dm = new storage::DiskManager{db_name};
        bm = new storage::BufferManager{storage::ReplacementPolicyType::CLOCK, dm};
        cat = new catalog::Catalog{bm, dm};
        cat->Init();
    }

    void TearDown() override {
        delete cat;
        delete bm;
        delete dm;
        std::filesystem::remove(db_name);
    }
};

TEST_F(TableManagerTest, OpenTableAndCacheTest) {
    std::vector<catalog::RawColumnInfo> cols = {
        { "id", catalog::INT_TYPE, 1 }
    };
    TableManager manager{cat};
    manager.CreateTable("students", cols);

    // test first open (cache miss)
    auto table1 = manager.OpenTable("students");
    ASSERT_NE(table1, nullptr);
    
    // test functionality of the opened table
    std::vector<common::Value> row = { uint32_t{42} };
    auto rid = table1->Insert(row);
    EXPECT_TRUE(rid.has_value());

    // test second open (cache hit)
    auto table2 = manager.OpenTable("students");
    
    // verify it is the exact same instance (shared_ptr points to same memory)
    EXPECT_EQ(table1.get(), table2.get());
}

TEST_F(TableManagerTest, OpenNonExistentTable) {
    TableManager manager{cat};
    EXPECT_ANY_THROW(manager.OpenTable("ghost_table"));
}

}