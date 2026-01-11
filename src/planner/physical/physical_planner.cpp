#include "planner/physical/physical_planner.h"
#include "planner/logical/logical_plan.h"

// logical nodes
#include "planner/logical/nodes/scan.h"
#include "planner/logical/nodes/filter.h"
#include "planner/logical/nodes/project.h"
#include "planner/logical/nodes/limit.h"

// physical plans
#include "executor/operators/seq_scan_op.h"
#include "executor/operators/filter_op.h"
#include "executor/operators/limit_op.h"
#include "executor/operators/projection_op.h"
#include "executor/predicate.h"

namespace db::planner {
executor::Predicate CompilePredicate(parser::Expr expr);
std::shared_ptr<const common::Schema> 
BuildOutputSchema(const logical::LogicalProject& proj);
std::unordered_set<uint16_t> 
ColumnsToPositions(const logical::LogicalProject& proj);

std::unique_ptr<executor::Operator> 
PhysicalPlanner::Build(const LogicalPlan& plan, PlanningContext& ctx) {
    switch (plan.Type())
    {
    case LogicalPlanType::Scan: {
        auto& scan = static_cast<const logical::LogicalScan&>(plan);
        auto table = ctx.table_mgr->OpenTable(scan.TableName());
        return std::make_unique<executor::SeqScanOp>(*table);
    }

    case LogicalPlanType::Filter: {
        auto& filter = static_cast<const logical::LogicalFilter&>(plan);
        auto pred = CompilePredicate(filter.Predicate());
        auto child_op = Build(filter.Child(), ctx);
        return std::make_unique<executor::FilterOp>(
            std::move(child_op), pred
        );
    }
     
    case LogicalPlanType::Project: {
        auto& proj = static_cast<const logical::LogicalProject&>(plan);
        auto child_op = Build(*proj.Children()[0], ctx);
        auto schema = BuildOutputSchema(proj);
        return std::make_unique<executor::ProjectionOp>(
            std::move(child_op), ColumnsToPositions(proj), schema
        );
    }

    case LogicalPlanType::Limit: {
        auto& limit = static_cast<const logical::LogicalLimit&>(plan);
        auto child_op = Build(*limit.Children()[0], ctx);
        return std::make_unique<executor::LimitOp>(
            std::move(child_op), limit.Limit()
        );
    }

    default:
        throw std::runtime_error("Unsupported logical plan.");
    } 


};
    
};
