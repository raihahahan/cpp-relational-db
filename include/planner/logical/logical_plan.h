#pragma once
#include <memory>
#include "parser/ast.h"
namespace db::planner {
class LogicalPlan {
public:
    virtual ~LogicalPlan() = default;
};

using LogicalPlanPtr = std::unique_ptr<LogicalPlan>;

class LogicalPlanner {
public:
    static LogicalPlanPtr
    Build(const parser::AstNode& ast);
};

}