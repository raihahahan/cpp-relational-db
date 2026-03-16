#include "planner/logical/logical_planner.h"
#include "planner/logical/nodes/scan.h"
#include "planner/logical/nodes/filter.h"
#include "planner/logical/nodes/project.h"
#include "planner/logical/nodes/limit.h"

namespace db::planner {


// LogicalScan
namespace logical {

LogicalScan::LogicalScan(std::string table_name)
    : _table_name(std::move(table_name)) {}

LogicalPlanType LogicalScan::Type() const { return LogicalPlanType::Scan; }

const std::vector<LogicalPlan *> &LogicalScan::Children() const {
    static const std::vector<LogicalPlan *> empty;
    return empty;
}

std::string LogicalScan::TableName() const { return _table_name; }

// LogicalFilter
LogicalFilter::LogicalFilter(LogicalPlanPtr child,
                             std::unique_ptr<parser::AnalyzedExpr> pred)
    : _child(std::move(child)), _pred(std::move(pred)) {}

LogicalPlanType LogicalFilter::Type() const { return LogicalPlanType::Filter; }

const std::vector<LogicalPlan *> &LogicalFilter::Children() const {
    _children_cache = {_child.get()};
    return _children_cache;
}

const parser::AnalyzedExpr &LogicalFilter::Predicate() const { return *_pred; }

LogicalPlan &LogicalFilter::Child() const { return *_child; }

// LogicalProject
LogicalProject::LogicalProject(LogicalPlanPtr child,
                               std::vector<std::string> cols,
                               std::vector<uint16_t> positions)
    : _child(std::move(child)),
      _columns(std::move(cols)),
      _positions(std::move(positions)) {}

LogicalPlanType LogicalProject::Type() const {
    return LogicalPlanType::Project;
}

const std::vector<LogicalPlan *> &LogicalProject::Children() const {
    _children_cache = {_child.get()};
    return _children_cache;
}

const std::vector<std::string> &LogicalProject::Columns() const {
    return _columns;
}

const std::vector<uint16_t> &LogicalProject::Positions() const {
    return _positions;
}


// LogicalLimit
LogicalPlanType LogicalLimit::Type() const { return LogicalPlanType::Limit; }

const std::vector<LogicalPlan *> &LogicalLimit::Children() const {
    _children_cache = {_child.get()};
    return _children_cache;
}

size_t LogicalLimit::Limit() const { return _limit; }

}

LogicalPlanPtr LogicalPlanner::Build(const parser::Query &query) {
    // 1. Scan
    LogicalPlanPtr plan =
        std::make_unique<logical::LogicalScan>(query.range_table.table_name);

    // 2. Filter (if WHERE clause present)
    if (query.where_clause) {
        auto pred_copy = parser::clone(*query.where_clause);
        plan = std::make_unique<logical::LogicalFilter>(std::move(plan),
                                                        std::move(pred_copy));
    }

    // 3. Project. Collect column names and ordinal positions from target list
    std::vector<std::string> col_names;
    std::vector<uint16_t> col_positions;
    col_names.reserve(query.target_list.size());
    col_positions.reserve(query.target_list.size());
    for (const auto &te : query.target_list) {
        col_names.push_back(te.name);
        col_positions.push_back(te.column.ordinal_position);
    }
    plan = std::make_unique<logical::LogicalProject>(
        std::move(plan), std::move(col_names), std::move(col_positions));

    // 4. Limit (if present)
    if (query.limit_count) {
        plan = std::make_unique<logical::LogicalLimit>(std::move(plan),
                                                       *query.limit_count);
    }

    return plan;
}
}
