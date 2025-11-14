#include "disk_manager/disk_manager.h"
#define INVALID_PAGE_ID -1

namespace db::storage {
struct Frame {
    page_id_t page_id = INVALID_PAGE_ID;
    int pin_count = 0;
    int dirty = 0; // 0 = not dirty, 1 = dirty
    char* data; // memory region for page content
};
}