#include "executor/operators/limit_op.h"

namespace db::executor {
LimitOp::LimitOp(std::unique_ptr<Operator> child, size_t limit) 
                : _child{std::move(child)}, _limit{limit}, _produced{0} {};
            
void LimitOp::Open() {
    _produced = 0;
    _child->Open();
}

std::optional<Tuple> LimitOp::Next() {
    if (_produced >= _limit) return std::nullopt;
  
    auto tup = _child->Next();
    if (!tup) return std::nullopt;

    ++_produced;
    return tup;
}

void LimitOp::Close() {
    _child->Close();
}
}