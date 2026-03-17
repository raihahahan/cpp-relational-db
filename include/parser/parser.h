#pragma once
#include "parser/ast.h"
#include "parser/lexer.h"

namespace db::parser {
class Parser {
public:
    static std::unique_ptr<AstNode> Parse(const std::string& sql);

private:
    explicit Parser(Lexer lexer);

    // statement parsing
    std::unique_ptr<SelectStmt> parse_select_stmt();
    std::unique_ptr<DeleteStmt> parse_delete_stmt();

    // target list
    std::vector<SelectTarget> parse_target_list();
    ResTarget parse_target_entry();

    // expressions (precedence climbing)
    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Expr> parse_or_expr();
    std::unique_ptr<Expr> parse_and_expr();
    std::unique_ptr<Expr> parse_not_expr();
    std::unique_ptr<Expr> parse_comparison_expr();
    std::unique_ptr<Expr> parse_additive_expr();
    std::unique_ptr<Expr> parse_primary_expr();

    // token helpers
    Token consume(TokenType expected);
    Token consume_keyword(std::string_view keyword);
    bool match_keyword(std::string_view keyword);
    bool check_keyword(std::string_view keyword) const;
    bool check(TokenType type) const;
    Token advance();
    Token peek() const;
    bool at_end() const;

    [[noreturn]] void error(const std::string& msg);
    [[noreturn]] void error_at(const Token& tok, const std::string& msg);

    Lexer _lexer;
    Token _current;
};

}