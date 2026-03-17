#pragma once

#include "parser/analyzer.h"
#include "planner/logical/logical_plan.h"
#include "model/table_manager.h"

namespace db::planner {

class LogicalPlanner {
public:
    static LogicalPlanPtr Build(const parser::Query& query);
    static LogicalPlanPtr Build(const parser::AnalyzedInsert& ins);
    static LogicalPlanPtr Build(const parser::AnalyzedDelete& del);
};

struct PlanningContext {
    model::TableManager* table_mgr;
};
}