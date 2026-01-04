#include "model/relation.h"

namespace db::model {
Relation::Relation(HeapFile hf) : _hf{std::move(hf)} {}
std::optional<RID> Relation::InsertRaw(std::span<const uint8_t> bytes, size_t len) {
    // TODO: in the future, to add MVCC tuple header logic into insert raw
    const char* data = reinterpret_cast<const char*>(bytes.data());
    return _hf.Insert(data, len);
};

HeapIterator Relation::Begin() {
    return _hf.begin();
}
}
