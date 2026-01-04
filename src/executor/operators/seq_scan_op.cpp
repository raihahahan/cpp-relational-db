#include "executor/operators/seq_scan_op.h"

namespace db::executor {
SeqScanOp::SeqScanOp(std::shared_ptr<model::Relation> rel)
        : _rel{std::move(rel)} {};

void SeqScanOp::Open() {
    _iter = _rel->Begin();
}

std::optional<Tuple> SeqScanOp::Next() {
    if (!_iter.HasNext()) return std::nullopt;
    return _rel->Decode(_iter.Next());
}

void SeqScanOp::Close() {}
}