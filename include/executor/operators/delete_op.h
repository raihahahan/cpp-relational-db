#pragma once

#include "executor/operator.h"
#include "model/user_table.h"
#include <memory>

namespace db::executor {

class DeleteOp : public Operator {
public:
    DeleteOp(std::unique_ptr<Operator> child,
             std::shared_ptr<model::UserTable> table);

    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;

private:
    std::unique_ptr<Operator> _child;
    std::shared_ptr<model::UserTable> _table;
    bool _done = false;
    uint32_t _count = 0;
};

}
