#pragma once

#include "executor/operator.h"
#include "model/user_table.h"
#include <memory>
#include <vector>

namespace db::executor {

class UpdateOp : public Operator {
public:
    UpdateOp(std::unique_ptr<Operator> child,
             std::shared_ptr<model::UserTable> table,
             std::vector<std::pair<uint16_t, common::Value>> assignments);

    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;

private:
    std::unique_ptr<Operator> _child;
    std::shared_ptr<model::UserTable> _table;
    std::vector<std::pair<uint16_t, common::Value>> _assignments; // ordinal_position -> new value
    bool _done = false;
    uint32_t _count = 0;
};

}
