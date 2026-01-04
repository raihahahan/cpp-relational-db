#include <gtest/gtest.h>
#include "model/user_table.h"
#include "catalog/catalog_bootstrap.h"
#include "catalog/catalog.h"
#include "storage/buffer_manager/replacement_policies/clock_policy.h"

namespace db::model {

TEST(UserTableTest, BasicInsertFlow) {
    DiskManager* dm = new DiskManager{"test.db"};
    BufferManager* bm = new BufferManager{storage::ReplacementPolicyType::CLOCK, dm};
    catalog::Catalog* cat = new catalog::Catalog{bm, dm};

    cat->Init();
    std::vector<catalog::RawColumnInfo> cols = {
        { "id", catalog::INT_TYPE, 1 },
        { "name", catalog::TEXT_TYPE, 2 }
    };

    auto table_id = cat->CreateTable("users", cols);
    auto table = cat->LookupTable("users");
    auto hf = access::HeapFile::Open(bm, dm, table->heap_file_id, table->first_page_id);

    std::vector<catalog::ColumnInfo> schema = {
        { table_id, "id", catalog::INT_TYPE, 1 },
        { table_id, "name", catalog::TEXT_TYPE, 2 }
    };
    UserTable user_table{hf, schema, table_id};
    std::string name = "Raihan";
    std::vector<common::Value> row = { uint32_t{1}, name };
    
    auto rid = user_table.Insert(row);
    EXPECT_TRUE(rid.has_value());
};

}