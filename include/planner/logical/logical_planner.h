#pragma once

#include "parser/analyzer.h"
#include "planner/logical/logical_plan.h"
#include "model/table_manager.h"

namespace db::planner {

class LogicalPlanner {
public:
    static LogicalPlanPtr Build(const parser::Query& query);
};

struct PlanningContext {
    model::TableManager* table_mgr;
};
}