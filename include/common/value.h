#pragma once
#include <string>
#include <cstdint>
#include <variant>

namespace db::common {
using Value = 
    std::variant<uint32_t, std::string, config::uuid_t, catalog::page_id_t>;
}
