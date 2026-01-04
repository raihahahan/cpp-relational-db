#include <gtest/gtest.h>

#include "catalog/catalog_codec.h"
#include "catalog/catalog.h"
#include "config/config.h"
#include "util/uuid.h"

namespace db::catalog::codec {
TEST(TableInfoCodecTest, RoundTripSingle) {
    TableInfo in{
        .table_id = 42,
        .table_name = "users",
        .heap_file_id = 7,
        .first_page_id = 123
    };

    auto buf = TableInfoCodec::Encode(in);
    auto out = TableInfoCodec::Decode(buf);

    EXPECT_EQ(out.table_id, in.table_id);
    EXPECT_EQ(out.table_name, in.table_name);
    EXPECT_EQ(out.heap_file_id, in.heap_file_id);
    EXPECT_EQ(out.first_page_id, in.first_page_id);
}

TEST(TableInfoCodecTest, RoundTripMultipleDifferentNames) {
    std::vector<TableInfo> tables = {
        {util::GenerateUUID(), "short", 10, 100},
        {util::GenerateUUID(), "medium_length_name", 11, 101},
        {util::GenerateUUID(), "this_is_a_very_long_table_name_to_catch_offset_bugs", 12, 102}
    };

    for (const auto& t : tables) {
        auto buf = TableInfoCodec::Encode(t);
        auto out = TableInfoCodec::Decode(buf);

        EXPECT_EQ(out.table_id, t.table_id);
        EXPECT_EQ(out.table_name, t.table_name);
        EXPECT_EQ(out.heap_file_id, t.heap_file_id);
        EXPECT_EQ(out.first_page_id, t.first_page_id);
    }
}

TEST(ColumnInfoCodecTest, RoundTripSingle) {
    ColumnInfo in{
        .table_id = 1,
        .col_name = "email",
        .type_id = 2,
        .ordinal_position = 3
    };

    auto buf = ColumnInfoCodec::Encode(in);
    auto out = ColumnInfoCodec::Decode(buf);

    EXPECT_EQ(out.table_id, in.table_id);
    EXPECT_EQ(out.col_name, in.col_name);
    EXPECT_EQ(out.type_id, in.type_id);
    EXPECT_EQ(out.ordinal_position, in.ordinal_position);
}

TEST(ColumnInfoCodecTest, MultipleColumnsSameTable) {
    using namespace util;
    std::vector<ColumnInfo> cols = {
        {util::GenerateUUID(), "id", 1, 1},
        {util::GenerateUUID(), "username", 2, 2},
        {util::GenerateUUID(), "created_at", 1, 3}
    };

    for (const auto& c : cols) {
        auto buf = ColumnInfoCodec::Encode(c);
        auto out = ColumnInfoCodec::Decode(buf);

        EXPECT_EQ(out.table_id, c.table_id);
        EXPECT_EQ(out.col_name, c.col_name);
        EXPECT_EQ(out.type_id, c.type_id);
        EXPECT_EQ(out.ordinal_position, c.ordinal_position);
    }
}

TEST(TypeInfoCodecTest, RoundTripSingle) {
    TypeInfo in{
        .type_id = 1,
        .size = 4,
        .type_name = "int"
    };

    auto buf = TypeInfoCodec::Encode(in);
    auto out = TypeInfoCodec::Decode(buf);

    EXPECT_EQ(out.type_id, in.type_id);
    EXPECT_EQ(out.size, in.size);
    EXPECT_EQ(out.type_name, in.type_name);
}

TEST(TypeInfoCodecTest, VariableLengthType) {
    TypeInfo in{
        .type_id = 2,
        .size = 0,
        .type_name = "text"
    };

    auto buf = TypeInfoCodec::Encode(in);
    auto out = TypeInfoCodec::Decode(buf);

    EXPECT_EQ(out.type_id, in.type_id);
    EXPECT_EQ(out.size, in.size);
    EXPECT_EQ(out.type_name, in.type_name);
}

}
