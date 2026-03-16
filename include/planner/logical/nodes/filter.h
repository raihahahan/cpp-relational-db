#pragma once
#include "planner/logical/logical_plan.h"
#include "parser/analyzer.h"
#include <memory>
#include <vector>

namespace db::planner::logical {

class LogicalFilter : public LogicalPlan {
public:
    LogicalFilter(LogicalPlanPtr child,
                  std::unique_ptr<parser::AnalyzedExpr> pred);

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;

    const parser::AnalyzedExpr& Predicate() const;
    LogicalPlan& Child() const;

private:
    LogicalPlanPtr _child;
    std::unique_ptr<parser::AnalyzedExpr> _pred;
    mutable std::vector<LogicalPlan*> _children_cache;
};
}