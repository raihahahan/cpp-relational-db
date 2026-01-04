#pragma once
#include <memory>
#include "parser/ast.h"
namespace db::planner {
enum class LogicalPlanType {
    Scan,
    Filter,
    Project,
    Limit
};

class LogicalPlan {
public:
    virtual ~LogicalPlan() = default;
    virtual LogicalPlanType Type() const =  0;
    virtual const std::vector<LogicalPlan*>& Children() const = 0;
};

using LogicalPlanPtr = std::unique_ptr<LogicalPlan>;
}