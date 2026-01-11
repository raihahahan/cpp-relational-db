#pragma once

#include "planner/logical/logical_plan.h"
#include "model/table_manager.h"

namespace db::planner {
class LogicalPlanner {
public:
    static LogicalPlanPtr Build(const parser::AstNode& ast);
};

struct PlanningContext {
    model::TableManager* table_mgr;
};
}