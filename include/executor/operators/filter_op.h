#pragma once

#include "executor/operator.h"
#include "executor/predicate.h"
#include <memory>

namespace db::executor {
class FilterOp : public Operator {
public:
    explicit FilterOp(std::unique_ptr<Operator> child,
                        Predicate pred);
    
    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;

private:
    std::unique_ptr<Operator> _child;
    Predicate _pred;
};
}