#include <gtest/gtest.h>
#include "model/dynamic_codec.h"
#include "catalog/catalog_bootstrap.h"

namespace db::model {

class DynamicCodecTest : public ::testing::Test {
protected:
    // helper to create a standard test schema
    std::vector<ColumnInfo> GetTestSchema() {
        return {
            {.col_name = "id", .type_id = catalog::INT_TYPE},
            {.col_name = "name", .type_id = catalog::TEXT_TYPE},
            {.col_name = "score", .type_id = catalog::INT_TYPE}
        };
    }
};

TEST_F(DynamicCodecTest, RoundTripBasic) {
    auto schema = GetTestSchema();
    
    // create input values matching the variant: std::variant<uint32_t, std::string>
    std::vector<Value> input;
    input.emplace_back(static_cast<uint32_t>(1));
    input.emplace_back(std::string("bAnana"));
    input.emplace_back(static_cast<uint32_t>(100));

    // test Encode
    auto buf = DynamicCodec::Encode(input, schema);
    
    // test Decode
    auto output = DynamicCodec::Decode(buf, schema);

    ASSERT_EQ(input.size(), output.size());
    EXPECT_EQ(std::get<uint32_t>(output[0]), 1);
    EXPECT_EQ(std::get<std::string>(output[1]), "bAnana");
    EXPECT_EQ(std::get<uint32_t>(output[2]), 100);
}

TEST_F(DynamicCodecTest, AlignmentAndPadding) {
    // schema designed to force padding
    // TEXT: 4 bytes (len) + 1 byte ('A') = 5 bytes. 
    // next INT needs 4-byte alignment, so 3 bytes padding required.
    std::vector<ColumnInfo> schema = {
        {.col_name = "char_col", .type_id = catalog::TEXT_TYPE},
        {.col_name = "int_col", .type_id = catalog::INT_TYPE}
    };

    std::vector<Value> input = { std::string("A"), static_cast<uint32_t>(42) };
    auto buf = DynamicCodec::Encode(input, schema);

    // calculation: 
    // [4 bytes len] + [1 byte 'A'] = 5 bytes
    // [3 bytes padding] = 8 bytes total so far
    // [4 bytes int] = 12 bytes total
    EXPECT_EQ(buf.size(), 12);

    auto output = DynamicCodec::Decode(buf, schema);
    EXPECT_EQ(std::get<uint32_t>(output[1]), 42);
}

TEST_F(DynamicCodecTest, EmptyStringHandling) {
    std::vector<ColumnInfo> schema = {
        {.col_name = "empty", .type_id = catalog::TEXT_TYPE},
        {.col_name = "suffix", .type_id = catalog::INT_TYPE}
    };

    std::vector<Value> input = { std::string(""), static_cast<uint32_t>(99) };

    auto buf = DynamicCodec::Encode(input, schema);
    auto output = DynamicCodec::Decode(buf, schema);

    EXPECT_EQ(std::get<std::string>(output[0]), "");
    EXPECT_EQ(std::get<uint32_t>(output[1]), 99);
}
}