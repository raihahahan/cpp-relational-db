#include "executor/operators/seq_scan_op.h"

namespace db::executor {
SeqScanOp::SeqScanOp(model::Relation& rel)
        : _rel{rel} {};

void SeqScanOp::Open() {
    _iter = _rel.Begin();
}

std::optional<Tuple> SeqScanOp::Next() {
    if (!_iter.HasNext()) return std::nullopt;
    auto record = _iter.Next();
    auto tuple = _rel.Decode(record);
    return Tuple{tuple.GetValues(), tuple.GetSchema(), record.rid};
}

void SeqScanOp::Close() {}
}