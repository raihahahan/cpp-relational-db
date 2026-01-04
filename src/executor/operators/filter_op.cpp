#include "executor/operators/filter_op.h"

namespace db::executor {
FilterOp::FilterOp(std::unique_ptr<Operator> child, Predicate pred) :
                    _child{std::move(child)}, _pred{std::move(pred)} {};

void FilterOp::Open() {
    _child->Open();
}

std::optional<Tuple> FilterOp::Next() {
    while (true) {
        auto tup = _child->Next();
        if (!tup) return std::nullopt;
        if (_pred.Evaluate(*tup)) return tup;
    }
}

void FilterOp::Close() {
    _child->Close();
}
}