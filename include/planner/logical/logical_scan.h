#pragma once
#include "planner/logical/logical_plan.h"
#include <string>
namespace db::planner::logical {
class LogicalScan : public LogicalPlan {
public:
    explicit LogicalScan(std::string table_name);

    std::string table_name;
};
}