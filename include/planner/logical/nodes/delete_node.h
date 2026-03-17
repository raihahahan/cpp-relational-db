#pragma once
#include "planner/logical/logical_plan.h"
#include <string>
#include <vector>

namespace db::planner::logical {

class LogicalDelete : public LogicalPlan {
public:
    LogicalDelete(LogicalPlanPtr child, std::string table_name);

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;

    const std::string& TableName() const;
    LogicalPlan& Child() const;

private:
    LogicalPlanPtr _child;
    std::string _table_name;
    mutable std::vector<LogicalPlan*> _children_cache;
};

}
