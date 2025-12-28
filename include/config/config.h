#pragma once
#include <string>

namespace db::config {
    inline constexpr size_t PAGE_SIZE = 8192; // 8kB
    inline constexpr size_t BUFFER_POOL_SIZE = 100;
    inline constexpr std::string DATA_PATH = "data/";
    inline uint32_t DB_MAGIC = 0xDBDBDBDB;
    inline uint32_t ROOT_PAGE_ID = 0;
    constexpr uint32_t DB_TABLES_ROOT_PAGE_ID = 1;
    constexpr uint32_t DB_ATTRIBUTES_ROOT_PAGE_ID = 2;
    constexpr uint32_t DB_TYPES_ROOT_PAGE_ID = 3;
}

   