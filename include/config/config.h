#pragma once
#include <string>

namespace db::config {
    using uuid_t = std::array<uint8_t, 16>;
    inline constexpr size_t PAGE_SIZE = 8192; // 8kB
    inline constexpr size_t BUFFER_POOL_SIZE = 100;
    inline constexpr std::string DATA_PATH = "data/";
    inline uint32_t DB_MAGIC = 0xDBDBDBDB;
}

   