#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "catalog/catalog.h"
#include "parser/ast.h"

namespace db::parser {

// Analyzed expression nodes (carry type information)
struct AnalyzedExpr {
    virtual ~AnalyzedExpr() = default;
    catalog::type_id_t result_type = 0;
};

struct AnalyzedColumnRef : AnalyzedExpr {
    catalog::ColumnInfo column;
};

struct AnalyzedLiteral : AnalyzedExpr {
    std::string value;
    Literal::LiteralType lit_type;
};

struct AnalyzedBinaryExpr : AnalyzedExpr {
    std::string op;
    std::unique_ptr<AnalyzedExpr> lhs;
    std::unique_ptr<AnalyzedExpr> rhs;
};

struct AnalyzedUnaryExpr : AnalyzedExpr {
    std::string op;
    std::unique_ptr<AnalyzedExpr> operand;
};

// Resolved projection entry
struct TargetEntry {
    std::string name;
    catalog::ColumnInfo column;
    uint16_t resno;
};

// Fully analyzed query
struct Query {
    catalog::TableInfo range_table;
    std::vector<catalog::ColumnInfo> table_columns;

    std::vector<TargetEntry> target_list;

    std::unique_ptr<AnalyzedExpr> where_clause;

    std::optional<size_t> limit_count;
};

// Analyzed INSERT output
struct AnalyzedInsert {
    catalog::TableInfo table;
    std::vector<catalog::ColumnInfo> target_columns;
    std::vector<std::vector<std::unique_ptr<AnalyzedExpr>>> values;
};

// Analyzed UPDATE output
struct AnalyzedUpdate {
    catalog::TableInfo table;
    std::vector<catalog::ColumnInfo> table_columns;
    std::vector<std::pair<catalog::ColumnInfo, std::unique_ptr<AnalyzedExpr>>> assignments;
    std::unique_ptr<AnalyzedExpr> where_clause; // nullable
};

// Analyzed DELETE output
struct AnalyzedDelete {
    catalog::TableInfo table;
    std::vector<catalog::ColumnInfo> table_columns;
    std::unique_ptr<AnalyzedExpr> where_clause; // nullable
};

// Analyzed CREATE TABLE output
struct AnalyzedCreateTable {
    std::string table_name;
    std::vector<catalog::RawColumnInfo> columns;
};

// Statement type tag
enum class StmtType { Select, Insert, Update, Delete, CreateTable };

// Generic wrapper returned by Analyze()
struct AnalyzedStmt {
    StmtType type;
    std::unique_ptr<Query> select_query;
    std::unique_ptr<AnalyzedInsert> insert_query;
    std::unique_ptr<AnalyzedUpdate> update_query;
    std::unique_ptr<AnalyzedDelete> delete_query;
    std::unique_ptr<AnalyzedCreateTable> create_table;
};

// Deep-copy an AnalyzedExpr tree
std::unique_ptr<AnalyzedExpr> clone(const AnalyzedExpr& expr);

// Analyzer
class Analyzer {
public:
    explicit Analyzer(catalog::Catalog& catalog);

    std::unique_ptr<AnalyzedStmt> Analyze(const AstNode& parse_tree);

private:
    std::unique_ptr<Query> analyze_select(const SelectStmt& stmt);
    std::unique_ptr<AnalyzedInsert> analyze_insert(const InsertStmt& stmt);
    std::unique_ptr<AnalyzedUpdate> analyze_update(const UpdateStmt& stmt);
    std::unique_ptr<AnalyzedDelete> analyze_delete(const DeleteStmt& stmt);
    std::unique_ptr<AnalyzedCreateTable> analyze_create_table(const CreateTableStmt& stmt);

    // Name resolution
    catalog::TableInfo resolve_table(const std::string& table_name);
    catalog::ColumnInfo resolve_column(
        const std::string& col_name,
        const std::vector<catalog::ColumnInfo>& table_cols);

    // Target list analysis
    std::vector<TargetEntry> analyze_target_list(
        const std::vector<SelectTarget>& raw_targets,
        const std::vector<catalog::ColumnInfo>& table_cols);
    std::vector<TargetEntry> expand_star(
        const std::vector<catalog::ColumnInfo>& table_cols);
    TargetEntry analyze_res_target(
        const ResTarget& target,
        const std::vector<catalog::ColumnInfo>& table_cols,
        uint16_t resno);

    // Expression analysis
    std::unique_ptr<AnalyzedExpr> analyze_expr(
        const Expr& expr,
        const std::vector<catalog::ColumnInfo>& table_cols);
    std::unique_ptr<AnalyzedExpr> analyze_column_ref(
        const ColumnRef& ref,
        const std::vector<catalog::ColumnInfo>& table_cols);
    std::unique_ptr<AnalyzedExpr> analyze_literal(const Literal& lit);
    std::unique_ptr<AnalyzedExpr> analyze_binary_expr(
        const BinaryExpr& expr,
        const std::vector<catalog::ColumnInfo>& table_cols);
    std::unique_ptr<AnalyzedExpr> analyze_unary_expr(
        const UnaryExpr& expr,
        const std::vector<catalog::ColumnInfo>& table_cols);

    // Type checking
    void check_comparison_types(
        const AnalyzedExpr& lhs,
        const std::string& op,
        const AnalyzedExpr& rhs,
        size_t pos);
    catalog::type_id_t infer_literal_type(const Literal& lit);

    catalog::Catalog& _catalog;
};

}
