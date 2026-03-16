#pragma once
#include "planner/logical/logical_plan.h"
#include <cstdint>
#include <string>
#include <vector>

namespace db::planner::logical {
class LogicalProject : public LogicalPlan {
public:
    explicit LogicalProject(LogicalPlanPtr child,
                            std::vector<std::string> cols,
                            std::vector<uint16_t> positions);

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;
    const std::vector<std::string>& Columns() const;
    const std::vector<uint16_t>& Positions() const;

private:
    std::unique_ptr<LogicalPlan> _child;
    std::vector<std::string> _columns;
    std::vector<uint16_t> _positions;
    mutable std::vector<LogicalPlan*> _children_cache;
};
}