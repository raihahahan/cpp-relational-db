#include <gtest/gtest.h>

#include "catalog/catalog_tables.h"
#include "catalog/catalog_codec.h"
#include "access/heap/heap_file.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "storage/disk_manager/disk_manager.h"
#include "config/config.h"
#include "util/uuid.h"

using namespace db;
using namespace db::catalog;
using namespace db::catalog::codec;
using namespace db::storage;
using namespace db::access;

class CatalogTablesTest : public ::testing::Test {
protected:
    std::string db_file = "test_catalog_tables.db";

    std::unique_ptr<DiskManager> dm;
    std::unique_ptr<BufferManager> bm;

    void SetUp() override {
        std::remove(db_file.c_str());

        dm = std::make_unique<DiskManager>(db_file);
        bm = std::make_unique<BufferManager>(
            ReplacementPolicyType::CLOCK,
            dm.get()
        );
    }

    void TearDown() override {
        bm.reset();
        dm.reset();
        std::remove(db_file.c_str());
    }
};

TEST_F(CatalogTablesTest, TablesCatalogLookupByName) {
    auto hf = HeapFile::Create(bm.get(), dm.get(), util::GenerateUUID());

    TableInfo t1{
        .table_id = util::GenerateUUID(),
        .table_name = "users",
        .heap_file_id = util::GenerateUUID(),
        .first_page_id = 42
    };

    TableInfo t2{
        .table_id = util::GenerateUUID(),
        .table_name = "orders",
        .heap_file_id = util::GenerateUUID(),
        .first_page_id = 43
    };

    auto b1 = TableInfoCodec::Encode(t1);
    auto b2 = TableInfoCodec::Encode(t2);

    hf.Insert(reinterpret_cast<const char*>(b1.data()), b1.size());
    hf.Insert(reinterpret_cast<const char*>(b2.data()), b2.size());

    bm->flush_all();

    TablesCatalog tables(hf);

    auto res1 = tables.Lookup("users");
    ASSERT_TRUE(res1.has_value());
    EXPECT_EQ(res1->table_name, "users");
    EXPECT_EQ(res1->first_page_id, 42);

    auto res2 = tables.Lookup("orders");
    ASSERT_TRUE(res2.has_value());
    EXPECT_EQ(res2->table_name, "orders");

    auto res3 = tables.Lookup("nonexistent");
    EXPECT_FALSE(res3.has_value());
}

TEST_F(CatalogTablesTest, AttributesCatalogGetColumnsForTable) {
    auto hf = HeapFile::Create(bm.get(), dm.get(), util::GenerateUUID());

    table_id_t table1 = util::GenerateUUID();
    table_id_t table2 = util::GenerateUUID();

    ColumnInfo c1{table1, "id", INT_TYPE, 1};
    ColumnInfo c2{table1, "name", TEXT_TYPE, 2};
    ColumnInfo c3{table2, "price", INT_TYPE, 1};

    for (auto& c : {c1, c2, c3}) {
        auto buf = ColumnInfoCodec::Encode(c);
        hf.Insert(reinterpret_cast<const char*>(buf.data()), buf.size());
    }

    bm->flush_all();

    AttributesCatalog attrs(hf);

    auto cols1 = attrs.GetColumns(table1);
    ASSERT_EQ(cols1.size(), 2);

    EXPECT_EQ(cols1[0].table_id, table1);
    EXPECT_EQ(cols1[1].table_id, table1);

    auto cols2 = attrs.GetColumns(table2);
    ASSERT_EQ(cols2.size(), 1);
    EXPECT_EQ(cols2[0].col_name, "price");
}

TEST_F(CatalogTablesTest, TypesCatalogReturnsAllTypes) {
    auto hf = HeapFile::Create(bm.get(), dm.get(), util::GenerateUUID());

    TypeInfo t1{INT_TYPE, 4, "int"};
    TypeInfo t2{TEXT_TYPE, 0, "text"};

    for (auto& t : {t1, t2}) {
        auto buf = TypeInfoCodec::Encode(t);
        hf.Insert(reinterpret_cast<const char*>(buf.data()), buf.size());
    }

    bm->flush_all();

    TypesCatalog types(hf);

    auto res = types.GetTypes();
    ASSERT_EQ(res.size(), 2);

    EXPECT_EQ(res[0].type_name, "int");
    EXPECT_EQ(res[1].type_name, "text");
}

TEST_F(CatalogTablesTest, RestartPreservesCatalogTables) {
    file_id_t fid = util::GenerateUUID();
    page_id_t first_page;

    {
        auto hf = HeapFile::Create(bm.get(), dm.get(), fid);
        first_page = hf.GetPageId();

        TableInfo t{
            .table_id = util::GenerateUUID(),
            .table_name = "persisted",
            .heap_file_id = fid,
            .first_page_id = first_page
        };

        auto buf = TableInfoCodec::Encode(t);
        hf.Insert(reinterpret_cast<const char*>(buf.data()), buf.size());

        bm->flush_all();
    }

    // simulate restart
    bm.reset();
    dm.reset();

    dm = std::make_unique<DiskManager>(db_file);
    bm = std::make_unique<BufferManager>(
        ReplacementPolicyType::CLOCK,
        dm.get()
    );

    auto hf = HeapFile::Open(bm.get(), dm.get(), fid, first_page);
    TablesCatalog tables(hf);

    auto res = tables.Lookup("persisted");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->table_name, "persisted");
}
