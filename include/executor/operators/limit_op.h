#pragma once

#include "executor/operator.h"
#include <memory>

namespace db::executor {
class LimitOp : public Operator {
public: 
    explicit LimitOp(std::unique_ptr<Operator> child, size_t limit);
    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;

private:
    std::unique_ptr<Operator> _child;
    size_t _limit;
    size_t _produced;
};
}