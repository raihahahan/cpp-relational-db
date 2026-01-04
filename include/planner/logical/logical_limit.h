#pragma once
#include "planner/logical/logical_plan.h"

#include <string>
#include <vector>

namespace db::planner::logical {
class LogicalLimit : public LogicalPlan {
public:
    LogicalLimit(LogicalPlanPtr child, size_t limit);

    LogicalPlanPtr child;
    size_t limit;
};
}
