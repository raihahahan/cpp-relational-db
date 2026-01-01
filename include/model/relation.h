#include <optional>
#include "access/record.h"
#include "access/heap/heap_file.h"

using RID = db::access::RID;
using HeapFile = db::access::HeapFile;

namespace db::model {
class Relation {
public:
    explicit Relation(HeapFile hf);
    std::optional<RID> InsertRaw(char* bytes, size_t len);

private:
    HeapFile _hf;
};
}