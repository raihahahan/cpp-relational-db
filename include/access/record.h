#pragma once
#include "storage/page/slotted_page.h"
#include <vector>

namespace db::access {
using page_id_t = db::storage::page_id_t;
struct RID {
    page_id_t page_id;
    uint16_t slot_id;
};

struct Record {
    RID rid;
    const char* data;
};
}
