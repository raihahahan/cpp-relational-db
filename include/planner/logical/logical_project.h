#pragma once
#include "planner/logical/logical_plan.h"

#include <string>
#include <vector>

namespace db::planner::logical {
class LogicalProject : public LogicalPlan {
public:
    explicit LogicalProject(LogicalPlanPtr child,
                            std::vector<uint16_t> cols);

    LogicalPlanPtr child;
    std::vector<uint16_t> cols;
};
}