#pragma once
#include "planner/logical/logical_plan.h"
#include <string>
namespace db::planner::logical {
class LogicalScan : public LogicalPlan {
public:
    explicit LogicalScan(std::string table_name);
    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;

    std::string TableName() const;
    
private:
    std::string _table_name;
};
}