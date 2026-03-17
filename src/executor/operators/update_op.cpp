#include "executor/operators/update_op.h"
#include "catalog/catalog_bootstrap.h"

namespace db::executor {

UpdateOp::UpdateOp(std::unique_ptr<Operator> child,
                   std::shared_ptr<model::UserTable> table,
                   std::vector<std::pair<uint16_t, common::Value>> assignments)
    : _child(std::move(child)),
      _table(std::move(table)),
      _assignments(std::move(assignments)) {}

void UpdateOp::Open() { _child->Open(); }

std::optional<Tuple> UpdateOp::Next() {
    if (_done) return std::nullopt;

    while (auto tup = _child->Next()) {
        auto rid = tup->GetRID();
        if (!rid.has_value()) continue;

        auto values = tup->GetValues();
        for (const auto& [ordinal, new_val] : _assignments) {
            values[ordinal - 1] = new_val;
        }

        _table->Update(rid.value(), values);
        ++_count;
    }

    _done = true;

    common::Schema schema = {
        catalog::ColumnInfo{{}, "affected_rows", catalog::INT_TYPE, 1}
    };
    return Tuple{
        {common::Value{_count}},
        std::make_shared<const common::Schema>(std::move(schema))
    };
}

void UpdateOp::Close() { _child->Close(); }

}
