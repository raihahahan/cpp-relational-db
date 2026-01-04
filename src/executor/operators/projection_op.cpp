#include "executor/operators/projection_op.h"

namespace db::executor {
ProjectionOp::ProjectionOp(std::unique_ptr<Operator> child,
                            std::unordered_set<uint16_t> col_pos,
                            std::shared_ptr<const common::Schema> out_schema) : 
                                _child{std::move(child)}, 
                                _col_pos{std::move(col_pos)},
                                _out_schema{std::move(out_schema)} {};

void ProjectionOp::Open() {
    _child->Open();
}

std::optional<Tuple> ProjectionOp::Next() {
    auto tup = _child->Next();
    if (!tup) return std::nullopt;
    return Project(*tup);
}

void ProjectionOp::Close() {
    _child->Close();
}

Tuple ProjectionOp::Project(const Tuple& tup) {
    std::vector<common::Value> projected;
    auto vals = tup.GetValues();
    projected.reserve(_col_pos.size());
    for (size_t idx = 0; idx < vals.size(); ++idx) {
        if (_col_pos.contains(idx + 1)) {
            projected.push_back(vals[idx]);
        }
    }
    return Tuple{vals, _out_schema};
}
}