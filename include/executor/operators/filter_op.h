#pragma once

#include "executor/operator.h"
#include "executor/predicate.h"
#include <memory>

namespace db::executor {
class FilterOp : public Operator {
public:
    explicit FilterOp(std::unique_ptr<Operator> child,
                        Predicate pred);
    
    template <typename F>
    requires std::invocable<F, const common::Tuple&>
    FilterOp(std::unique_ptr<Operator> child, F&& f)
        : _child{std::move(child)},
          _pred{Predicate{std::forward<F>(f)}} {}
    
    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;

private:
    std::unique_ptr<Operator> _child;
    Predicate _pred;
};
}