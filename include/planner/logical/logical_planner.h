#pragma once

#include "planner/logical/logical_plan.h"

namespace db::planner {
class LogicalPlanner {
public:
    static LogicalPlanPtr Build(const parser::AstNode& ast);
};
}