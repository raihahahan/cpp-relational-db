#include "storage/page/slotted_page.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "access/record.h"

namespace db::access {
class HeapIterator {
public:
    bool hasNext();
    Record next();
};
}