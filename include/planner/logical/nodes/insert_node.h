#pragma once

#include "planner/logical/logical_plan.h"
#include "parser/analyzer.h"
#include <string>
#include <vector>

namespace db::planner::logical {

class LogicalInsert : public LogicalPlan {
public:
    LogicalInsert(std::string table_name,
                  std::vector<catalog::ColumnInfo> target_columns,
                  std::vector<std::vector<std::unique_ptr<parser::AnalyzedExpr>>> values);

    LogicalPlanType Type() const override;
    const std::vector<LogicalPlan*>& Children() const override;

    const std::string& TableName() const;
    const std::vector<catalog::ColumnInfo>& TargetColumns() const;
    const std::vector<std::vector<std::unique_ptr<parser::AnalyzedExpr>>>& Values() const;

private:
    std::string _table_name;
    std::vector<catalog::ColumnInfo> _target_columns;
    std::vector<std::vector<std::unique_ptr<parser::AnalyzedExpr>>> _values;
};

}
