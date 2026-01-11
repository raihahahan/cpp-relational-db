#pragma once

#include "planner/logical/logical_plan.h"
#include "executor/operator.h"
#include "model/table_manager.h"
#include "planner/logical/logical_planner.h"
#include <memory>

namespace db::planner {

class PhysicalPlanner {
public:
    static std::unique_ptr<executor::Operator> 
    Build(const LogicalPlan& plan, PlanningContext& ctx);
};

}

/*
note: 
- in the future, when there are multiple physical plans to a logical plan (e.g. joins: nested loop, hash, merge), 
    will need to create separate physical plan classes (e.g physical_join.h)
- there, cost-base decision making will happen
- for now, use simple mappings: (4 jan 2026)
    - scan: seq_scan (future: index scan)
    - filter: filter
    - projection: projection
    - limit: limit
*/