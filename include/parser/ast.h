#pragma once
#include <vector>
#include <string>
#include <memory>

namespace db::parser {
struct AstNode {
    virtual ~AstNode() = default;
};
struct Expr : AstNode {};

struct ColumnRef : Expr {
    std::string name;
};

struct Literal : Expr {
    std::string value;
};

struct BinaryExpr : Expr {
    std::string op; // "=", ">=" etc
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct SelectStmt : AstNode {
    std::vector<std::string> select_list; // column names
    std::string from_table;
    std::unique_ptr<Expr> where; // nullable   
    size_t limit = 0; // 0 = no limit
};
}