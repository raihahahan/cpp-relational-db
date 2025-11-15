#pragma once

#include <cstdint>

namespace db::storage {

using page_id_t = int32_t;

class IDiskManager {
public:
    virtual ~IDiskManager() = default;

    virtual void ReadPage(page_id_t page_id, char* page_data) = 0;
    virtual void WritePage(page_id_t page_id, const char* page_data) = 0;
    virtual page_id_t AllocatePage() = 0;
    virtual void DeallocatePage(page_id_t page_id) = 0;
};

}
