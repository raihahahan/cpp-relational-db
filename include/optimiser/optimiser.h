#pragma once
#include "planner/logical/logical_plan.h"

namespace db::optimiser {
class Optimiser {
public:
    static planner::LogicalPlanPtr Optimise(planner::LogicalPlanPtr plan);
};
}
