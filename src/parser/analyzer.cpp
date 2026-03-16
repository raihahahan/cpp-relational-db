#include "parser/analyzer.h"
#include "catalog/catalog_bootstrap.h"
#include "error/dberror.h"
#include <algorithm>

namespace db::parser {

namespace {

std::string type_name(catalog::type_id_t type_id) {
    if (type_id == catalog::INT_TYPE)
        return "INT";
    if (type_id == catalog::TEXT_TYPE)
        return "TEXT";
    return "UNKNOWN";
}

}

std::unique_ptr<AnalyzedExpr> clone(const AnalyzedExpr& expr) {
    if (auto* col = dynamic_cast<const AnalyzedColumnRef*>(&expr)) {
        auto n = std::make_unique<AnalyzedColumnRef>();
        n->result_type = col->result_type;
        n->column = col->column;
        return n;
    }
    if (auto* lit = dynamic_cast<const AnalyzedLiteral*>(&expr)) {
        auto n = std::make_unique<AnalyzedLiteral>();
        n->result_type = lit->result_type;
        n->value = lit->value;
        n->lit_type = lit->lit_type;
        return n;
    }
    if (auto* bin = dynamic_cast<const AnalyzedBinaryExpr*>(&expr)) {
        auto n = std::make_unique<AnalyzedBinaryExpr>();
        n->result_type = bin->result_type;
        n->op = bin->op;
        n->lhs = clone(*bin->lhs);
        n->rhs = clone(*bin->rhs);
        return n;
    }
    if (auto* un = dynamic_cast<const AnalyzedUnaryExpr*>(&expr)) {
        auto n = std::make_unique<AnalyzedUnaryExpr>();
        n->result_type = un->result_type;
        n->op = un->op;
        n->operand = clone(*un->operand);
        return n;
    }
    return nullptr;
}

Analyzer::Analyzer(catalog::Catalog& catalog) : _catalog(catalog) {}

// ENTRYPOINTS
std::unique_ptr<Query> Analyzer::Analyze(const AstNode& parse_tree) {
    if (auto* select = dynamic_cast<const SelectStmt*>(&parse_tree)) {
        return analyze_select(*select);
    }
    throw DbError(ErrorCode::ParseError, "unsupported statement type");
}

std::unique_ptr<Query> Analyzer::analyze_select(const SelectStmt& stmt) {
    auto query = std::make_unique<Query>();

    // 1. resolve FROM table
    query->range_table = resolve_table(stmt.from_table);
    query->table_columns =
        _catalog.GetTableColumns(query->range_table.table_id);

    // sort columns by ordinal position for consistent ordering
    std::sort(query->table_columns.begin(), query->table_columns.end(),
              [](const catalog::ColumnInfo& a, const catalog::ColumnInfo& b) {
                  return a.ordinal_position < b.ordinal_position;
              });

    // 2. analyze target list
    query->target_list =
        analyze_target_list(stmt.target_list, query->table_columns);

    // 3. analyze WHERE clause
    if (stmt.where) {
        query->where_clause = analyze_expr(*stmt.where, query->table_columns);
    }

    // 4. carry LIMIT through
    query->limit_count = stmt.limit;

    return query;
}


// NAME RESOLUTION
catalog::TableInfo Analyzer::resolve_table(const std::string& table_name) {
    auto info = _catalog.LookupTable(table_name);
    if (!info.has_value()) {
        throw DbError(ErrorCode::UndefinedTable,
                       "table \"" + table_name + "\" does not exist");
    }
    return info.value();
}

catalog::ColumnInfo Analyzer::resolve_column(
    const std::string& col_name,
    const std::vector<catalog::ColumnInfo>& table_cols) {
    for (const auto& col : table_cols) {
        if (col.col_name == col_name)
            return col;
    }
    throw DbError(ErrorCode::UndefinedColumn,
                   "column \"" + col_name + "\" does not exist");
}


// TARGET LIST ANALYSIS
std::vector<TargetEntry> Analyzer::analyze_target_list(
    const std::vector<SelectTarget>& raw_targets,
    const std::vector<catalog::ColumnInfo>& table_cols) {

    std::vector<TargetEntry> entries;
    uint16_t resno = 1;

    for (const auto& target : raw_targets) {
        if (std::holds_alternative<StarTarget>(target)) {
            auto expanded = expand_star(table_cols);
            for (auto& te : expanded) {
                te.resno = resno++;
                entries.push_back(std::move(te));
            }
        } else {
            auto& res = std::get<ResTarget>(target);
            entries.push_back(analyze_res_target(res, table_cols, resno++));
        }
    }

    return entries;
}

std::vector<TargetEntry> Analyzer::expand_star(
    const std::vector<catalog::ColumnInfo>& table_cols) {
    std::vector<TargetEntry> entries;
    entries.reserve(table_cols.size());

    for (const auto& col : table_cols) {
        TargetEntry te;
        te.name = col.col_name;
        te.column = col;
        te.resno = 0; // set by caller
        entries.push_back(std::move(te));
    }

    return entries;
}

TargetEntry Analyzer::analyze_res_target(
    const ResTarget& target,
    const std::vector<catalog::ColumnInfo>& table_cols,
    uint16_t resno) {

    auto analyzed = analyze_expr(*target.val, table_cols);

    TargetEntry te;
    te.resno = resno;

    // Derive output name: alias > column name > "?column?"
    if (!target.alias.empty()) {
        te.name = target.alias;
    } else if (auto* col_ref =
                   dynamic_cast<AnalyzedColumnRef*>(analyzed.get())) {
        te.name = col_ref->column.col_name;
    } else {
        te.name = "?column?";
    }

    // Derive column info from analyzed expression
    if (auto* col_ref = dynamic_cast<AnalyzedColumnRef*>(analyzed.get())) {
        te.column = col_ref->column;
    } else {
        // For non-column expressions, create a synthetic ColumnInfo
        te.column.col_name = te.name;
        te.column.type_id = analyzed->result_type;
        te.column.ordinal_position = resno;
        te.column.table_id = {};
    }

    return te;
}


// Expression analysis
std::unique_ptr<AnalyzedExpr> Analyzer::analyze_expr(
    const Expr& expr,
    const std::vector<catalog::ColumnInfo>& table_cols) {

    if (auto* col = dynamic_cast<const ColumnRef*>(&expr))
        return analyze_column_ref(*col, table_cols);
    if (auto* lit = dynamic_cast<const Literal*>(&expr))
        return analyze_literal(*lit);
    if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr))
        return analyze_binary_expr(*bin, table_cols);
    if (auto* un = dynamic_cast<const UnaryExpr*>(&expr))
        return analyze_unary_expr(*un, table_cols);

    throw DbError(ErrorCode::ParseError, "unsupported expression type");
}

std::unique_ptr<AnalyzedExpr> Analyzer::analyze_column_ref(
    const ColumnRef& ref,
    const std::vector<catalog::ColumnInfo>& table_cols) {

    auto node = std::make_unique<AnalyzedColumnRef>();
    node->column = resolve_column(ref.name, table_cols);
    node->result_type = node->column.type_id;
    return node;
}

std::unique_ptr<AnalyzedExpr> Analyzer::analyze_literal(const Literal& lit) {
    auto node = std::make_unique<AnalyzedLiteral>();
    node->value = lit.value;
    node->lit_type = lit.lit_type;
    node->result_type = infer_literal_type(lit);
    return node;
}

std::unique_ptr<AnalyzedExpr> Analyzer::analyze_binary_expr(
    const BinaryExpr& expr,
    const std::vector<catalog::ColumnInfo>& table_cols) {

    auto node = std::make_unique<AnalyzedBinaryExpr>();
    node->op = expr.op;
    node->lhs = analyze_expr(*expr.lhs, table_cols);
    node->rhs = analyze_expr(*expr.rhs, table_cols);

    // Type check comparisons and arithmetic
    if (expr.op == "AND" || expr.op == "OR") {
        node->result_type = catalog::INT_TYPE;
    } else {
        check_comparison_types(*node->lhs, expr.op, *node->rhs, 0);

        if (expr.op == "+" || expr.op == "-") {
            node->result_type = catalog::INT_TYPE;
        } else {
            // Comparison operators produce INT (boolean)
            node->result_type = catalog::INT_TYPE;
        }
    }

    return node;
}

std::unique_ptr<AnalyzedExpr> Analyzer::analyze_unary_expr(
    const UnaryExpr& expr,
    const std::vector<catalog::ColumnInfo>& table_cols) {

    auto node = std::make_unique<AnalyzedUnaryExpr>();
    node->op = expr.op;
    node->operand = analyze_expr(*expr.operand, table_cols);
    node->result_type = catalog::INT_TYPE;
    return node;
}

// Type checking
void Analyzer::check_comparison_types(
    const AnalyzedExpr& lhs,
    const std::string& op,
    const AnalyzedExpr& rhs,
    size_t pos) {

    auto ltype = lhs.result_type;
    auto rtype = rhs.result_type;

    // NULL is compatible with anything
    if (ltype == 0 || rtype == 0)
        return;

    // Equality operators: both sides must match
    if (op == "=" || op == "!=" || op == "<>") {
        if (ltype != rtype) {
            throw DbError(
                ErrorCode::TypeMismatch,
                "operator " + op + " cannot compare " +
                    type_name(ltype) + " and " + type_name(rtype),
                pos);
        }
        return;
    }

    // Ordering / arithmetic operators: INT only
    if (op == "<" || op == ">" || op == "<=" || op == ">=" ||
        op == "+" || op == "-") {
        if (ltype != catalog::INT_TYPE || rtype != catalog::INT_TYPE) {
            throw DbError(
                ErrorCode::TypeMismatch,
                "operator " + op + " cannot compare " +
                    type_name(ltype) + " and " + type_name(rtype),
                pos);
        }
        return;
    }
}

catalog::type_id_t Analyzer::infer_literal_type(const Literal& lit) {
    switch (lit.lit_type) {
    case Literal::LiteralType::Integer:
        return catalog::INT_TYPE;
    case Literal::LiteralType::String:
        return catalog::TEXT_TYPE;
    case Literal::LiteralType::Null:
        return 0; // deferred: compatible with any type
    }
    return 0;
}

}
