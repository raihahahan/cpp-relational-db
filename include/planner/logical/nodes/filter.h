#pragma once
#include "planner/logical/logical_plan.h"
#include <string>
#include <vector>

namespace db::planner::logical {

class LogicalFilter : public LogicalPlan {
public:
    LogicalFilter(LogicalPlanPtr child,
                    parser::Expr pred);

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;

    const parser::Expr& Predicate() const;
    LogicalPlan& Child() const;

private:
    LogicalPlanPtr _child;
    parser::Expr _pred;
    mutable std::vector<LogicalPlan*> _children_cache;
    
};
}