#include "executor/operators/delete_op.h"
#include "catalog/catalog_bootstrap.h"

namespace db::executor {

DeleteOp::DeleteOp(std::unique_ptr<Operator> child,
                   std::shared_ptr<model::UserTable> table)
    : _child(std::move(child)), _table(std::move(table)) {}

void DeleteOp::Open() { _child->Open(); }

std::optional<Tuple> DeleteOp::Next() {
    if (_done) return std::nullopt;

    while (auto tup = _child->Next()) {
        auto rid = tup->GetRID();
        if (rid.has_value()) {
            _table->Delete(rid.value());
            ++_count;
        }
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

void DeleteOp::Close() { _child->Close(); }

}
