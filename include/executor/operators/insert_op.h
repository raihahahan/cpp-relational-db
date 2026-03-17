#pragma once

#include "executor/operator.h"
#include "model/user_table.h"
#include <memory>
#include <vector>

namespace db::executor {

class InsertOp : public Operator {
public:
    InsertOp(std::shared_ptr<model::UserTable> table,
             std::vector<std::vector<common::Value>> rows);

    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;

private:
    std::shared_ptr<model::UserTable> _table;
    std::vector<std::vector<common::Value>> _rows;
    bool _done = false;
    uint32_t _count = 0;
};

}
