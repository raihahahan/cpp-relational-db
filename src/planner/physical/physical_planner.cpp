#include "planner/physical/physical_planner.h"
#include "planner/logical/logical_plan.h"

#include "planner/logical/nodes/scan.h"
#include "planner/logical/nodes/filter.h"
#include "planner/logical/nodes/project.h"
#include "planner/logical/nodes/limit.h"

#include "executor/operators/seq_scan_op.h"
#include "executor/operators/filter_op.h"
#include "executor/operators/limit_op.h"
#include "executor/operators/projection_op.h"
#include "executor/predicate.h"
#include "parser/analyzer.h"
#include "catalog/catalog_bootstrap.h"

#include <stdexcept>
#include <string>

namespace db::planner {


// CompilePredicate. Converts an AnalyzedExpr tree into a runtime Predicate
namespace {

using Tuple = common::Tuple;
using Value = common::Value;

std::function<bool(const Tuple &)>
compile(const parser::AnalyzedExpr &expr) {
    if (auto* col =
            dynamic_cast<const parser::AnalyzedColumnRef*>(&expr)) {
        uint16_t pos = col->column.ordinal_position;
        catalog::type_id_t type = col->column.type_id;
        return [pos, type](const Tuple &t) -> bool {
            auto& v = t.GetValues()[pos - 1];
            if (type == catalog::INT_TYPE)
                return std::get<uint32_t>(v) != 0;
            return !std::get<std::string>(v).empty();
        };
    }

    if (auto* lit =
            dynamic_cast<const parser::AnalyzedLiteral*>(&expr)) {
        if (lit->lit_type == parser::Literal::LiteralType::Null)
            return [](const Tuple &) { return false; };
        if (lit->result_type == catalog::INT_TYPE) {
            uint32_t val = static_cast<uint32_t>(std::stoul(lit->value));
            return [val](const Tuple &) { return val != 0; };
        }
        std::string val = lit->value;
        return [val](const Tuple &) { return !val.empty(); };
    }

    if (auto* bin =
            dynamic_cast<const parser::AnalyzedBinaryExpr*>(&expr)) {
        auto lhs_fn = compile(*bin->lhs);
        auto rhs_fn = compile(*bin->rhs);
        const std::string &op = bin->op;

        if (op == "AND") {
            return [lhs_fn, rhs_fn](const Tuple &t) {
                return lhs_fn(t) && rhs_fn(t);
            };
        }
        if (op == "OR") {
            return [lhs_fn, rhs_fn](const Tuple &t) {
                return lhs_fn(t) || rhs_fn(t);
            };
        }

        // Comparison / arithmetic - extract values from sub-expressions
        // For comparisons the LHS and RHS are column refs or literals.
        // We need value-returning lambdas, not bool-returning ones.
        // Re-compile as value extractors.
        auto* lhs_col =
            dynamic_cast<const parser::AnalyzedColumnRef*>(bin->lhs.get());
        auto* rhs_col =
            dynamic_cast<const parser::AnalyzedColumnRef*>(bin->rhs.get());
        auto* lhs_lit =
            dynamic_cast<const parser::AnalyzedLiteral*>(bin->lhs.get());
        auto* rhs_lit =
            dynamic_cast<const parser::AnalyzedLiteral*>(bin->rhs.get());

        // Build generic value extractors for each side
        catalog::type_id_t cmp_type = bin->lhs->result_type;
        if (cmp_type == 0)
            cmp_type = bin->rhs->result_type;

        // INT comparisons
        if (cmp_type == catalog::INT_TYPE) {
            std::function<uint32_t(const Tuple &)> get_l, get_r;

            if (lhs_col) {
                uint16_t p = lhs_col->column.ordinal_position;
                get_l = [p](const Tuple &t) {
                    return std::get<uint32_t>(t.GetValues()[p - 1]);
                };
            } else if (lhs_lit) {
                uint32_t v = static_cast<uint32_t>(std::stoul(lhs_lit->value));
                get_l = [v](const Tuple &) { return v; };
            } else {
                auto nested = compile(*bin->lhs);
                get_l = [nested](const Tuple &t) -> uint32_t {
                    return nested(t) ? 1u : 0u;
                };
            }

            if (rhs_col) {
                uint16_t p = rhs_col->column.ordinal_position;
                get_r = [p](const Tuple &t) {
                    return std::get<uint32_t>(t.GetValues()[p - 1]);
                };
            } else if (rhs_lit) {
                uint32_t v = static_cast<uint32_t>(std::stoul(rhs_lit->value));
                get_r = [v](const Tuple &) { return v; };
            } else {
                auto nested = compile(*bin->rhs);
                get_r = [nested](const Tuple &t) -> uint32_t {
                    return nested(t) ? 1u : 0u;
                };
            }

            if (op == "=")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) == get_r(t);
                };
            if (op == "!=" || op == "<>")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) != get_r(t);
                };
            if (op == "<")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) < get_r(t);
                };
            if (op == ">")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) > get_r(t);
                };
            if (op == "<=")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) <= get_r(t);
                };
            if (op == ">=")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) >= get_r(t);
                };
        }

        // TEXT comparisons
        if (cmp_type == catalog::TEXT_TYPE) {
            std::function<std::string(const Tuple &)> get_l, get_r;

            if (lhs_col) {
                uint16_t p = lhs_col->column.ordinal_position;
                get_l = [p](const Tuple &t) {
                    return std::get<std::string>(t.GetValues()[p - 1]);
                };
            } else if (lhs_lit) {
                std::string v = lhs_lit->value;
                get_l = [v](const Tuple &) { return v; };
            }

            if (rhs_col) {
                uint16_t p = rhs_col->column.ordinal_position;
                get_r = [p](const Tuple &t) {
                    return std::get<std::string>(t.GetValues()[p - 1]);
                };
            } else if (rhs_lit) {
                std::string v = rhs_lit->value;
                get_r = [v](const Tuple &) { return v; };
            }

            if (op == "=")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) == get_r(t);
                };
            if (op == "!=" || op == "<>")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) != get_r(t);
                };
            if (op == "<")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) < get_r(t);
                };
            if (op == ">")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) > get_r(t);
                };
            if (op == "<=")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) <= get_r(t);
                };
            if (op == ">=")
                return [get_l, get_r](const Tuple &t) {
                    return get_l(t) >= get_r(t);
                };
        }

        throw std::runtime_error("CompilePredicate: unsupported binary op '" +
                                 op + "'");
    }

    if (auto* un =
            dynamic_cast<const parser::AnalyzedUnaryExpr*>(&expr)) {
        auto operand_fn = compile(*un->operand);
        if (un->op == "NOT")
            return [operand_fn](const Tuple &t) { return !operand_fn(t); };
        throw std::runtime_error("CompilePredicate: unsupported unary op '" +
                                 un->op + "'");
    }

    throw std::runtime_error("CompilePredicate: unknown expression type");
}
}

executor::Predicate
CompilePredicate(const parser::AnalyzedExpr &expr) {
    auto fn = compile(expr);
    return executor::Predicate{
        [fn](executor::Tuple t) { return fn(t); }};
}

// BuildOutputSchema / ColumnsToPositions
std::shared_ptr<const common::Schema>
BuildOutputSchema(const logical::LogicalProject &proj) {
    common::Schema schema;
    const auto& names = proj.Columns();
    const auto& positions = proj.Positions();
    for (size_t i = 0; i < names.size(); ++i) {
        catalog::ColumnInfo ci;
        ci.table_id = {};
        ci.col_name = names[i];
        ci.type_id = 0;
        ci.ordinal_position = positions[i];
        schema.push_back(ci);
    }
    return std::make_shared<const common::Schema>(std::move(schema));
}

std::unordered_set<uint16_t>
ColumnsToPositions(const logical::LogicalProject &proj) {
    const auto& positions = proj.Positions();
    return {positions.begin(), positions.end()};
}

// PhysicalPlanner::Build
std::unique_ptr<executor::Operator>
PhysicalPlanner::Build(const LogicalPlan &plan, PlanningContext &ctx) {
    switch (plan.Type()) {
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
