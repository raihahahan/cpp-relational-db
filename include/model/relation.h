#pragma once

#include <optional>
#include <span>
#include "access/record.h"
#include "access/heap/heap_file.h"
#include "access/heap/heap_iterator.h"
#include "common/tuple.h"

using RID = db::access::RID;
using Record = db::access::Record;
using HeapFile = db::access::HeapFile;
using HeapIterator = db::access::HeapIterator;
using Tuple = db::common::Tuple;

namespace db::model {
class Relation {
public:
    explicit Relation(HeapFile hf);
    std::optional<RID> InsertRaw(std::span<const uint8_t> bytes, size_t len);
    HeapIterator Begin();
    virtual Tuple Decode(const Record& rec) const  = 0;
protected:
    HeapFile _hf;
};
}