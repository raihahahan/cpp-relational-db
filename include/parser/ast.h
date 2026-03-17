#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace db::parser {

struct AstNode {
    virtual ~AstNode() = default;
};

struct Expr : AstNode {};

struct ColumnRef : Expr {
    std::string name;
    std::string table_qualifier; // empty if unqualified, for future table.column
};

struct Literal : Expr {
    enum class LiteralType { Integer, String, Null };
    LiteralType lit_type;
    std::string value;
};

struct BinaryExpr : Expr {
    std::string op; // "=", ">=", "AND", "OR", etc
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct UnaryExpr : Expr {
    std::string op; // "NOT", "-"
    std::unique_ptr<Expr> operand;
};

struct ResTarget : AstNode {
    std::unique_ptr<Expr> val;
    std::string alias; // empty if no AS clause
};

struct StarTarget : AstNode {};

using SelectTarget = std::variant<StarTarget, ResTarget>;

struct SelectStmt : AstNode {
    std::vector<SelectTarget> target_list;
    std::string from_table;
    std::unique_ptr<Expr> where; // nullable   
    std::optional<size_t> limit; // nullopt = no limit
};

struct InsertStmt : AstNode {
    std::string table_name;
    std::vector<std::string> columns;                       // optional column list; empty = all columns in order
    std::vector<std::vector<std::unique_ptr<Expr>>> values; // one vector<Expr> per row
};

struct DeleteStmt : AstNode {
    std::string table_name;
    std::unique_ptr<Expr> where; // nullable (no WHERE = delete all rows)
};

}