#include "executor/operators/insert_op.h"
#include "catalog/catalog_bootstrap.h"

namespace db::executor {

InsertOp::InsertOp(std::shared_ptr<model::UserTable> table,
                   std::vector<std::vector<common::Value>> rows)
    : _table(std::move(table)), _rows(std::move(rows)) {}

void InsertOp::Open() {}

std::optional<Tuple> InsertOp::Next() {
    if (_done) return std::nullopt;

    for (auto& row : _rows) {
        _table->Insert(row);
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

void InsertOp::Close() {}

}
