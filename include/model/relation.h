#pragma once

#include <optional>
#include <span>
#include "access/record.h"
#include "access/heap/heap_file.h"

using RID = db::access::RID;
using HeapFile = db::access::HeapFile;

namespace db::model {
class Relation {
public:
    explicit Relation(HeapFile hf);
    std::optional<RID> InsertRaw(std::span<const uint8_t> bytes, size_t len);

private:
    HeapFile _hf;
};
}