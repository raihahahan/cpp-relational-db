#pragma once
#include "planner/logical/logical_plan.h"
#include <string>
#include <vector>

namespace db::planner::logical {
class LogicalProject : public LogicalPlan {
public:
    explicit LogicalProject(LogicalPlanPtr child,
                            std::vector<std::string> cols);

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;
    const std::vector<std::string>& Columns() const;

private:
    std::unique_ptr<LogicalPlan> _child;
    std::vector<std::string> _columns;
    mutable std::vector<LogicalPlan*> _children_cache;
};
}