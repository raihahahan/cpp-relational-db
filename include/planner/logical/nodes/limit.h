#pragma once
#include "planner/logical/logical_plan.h"

#include <string>
#include <vector>

namespace db::planner::logical {
class LogicalLimit : public LogicalPlan {
public:
    LogicalLimit(std::unique_ptr<LogicalPlan> child, size_t limit)
        : _child{std::move(child)}, _limit{limit} {}

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;
    size_t Limit() const;

private:
    std::unique_ptr<LogicalPlan> _child;
    size_t _limit;
    mutable std::vector<LogicalPlan*> _children_cache;
};
}
