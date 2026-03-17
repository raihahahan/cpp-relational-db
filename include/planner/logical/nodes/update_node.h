#pragma once

#include "planner/logical/logical_plan.h"
#include "parser/analyzer.h"
#include <string>
#include <vector>

namespace db::planner::logical {

class LogicalUpdate : public LogicalPlan {
public:
    LogicalUpdate(LogicalPlanPtr child,
                  std::string table_name,
                  std::vector<std::pair<catalog::ColumnInfo, std::unique_ptr<parser::AnalyzedExpr>>> assignments);

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;

    const std::string& TableName() const;
    LogicalPlan& Child() const;
    const std::vector<std::pair<catalog::ColumnInfo, std::unique_ptr<parser::AnalyzedExpr>>>& Assignments() const;

private:
    LogicalPlanPtr _child;
    std::string _table_name;
    std::vector<std::pair<catalog::ColumnInfo, std::unique_ptr<parser::AnalyzedExpr>>> _assignments;
    mutable std::vector<LogicalPlan*> _children_cache;
};

}
