#include "parser/parser.h"
#include "error/dberror.h"
#include <algorithm>
#include <cctype>

namespace db::parser {

namespace {

std::string lowercase(std::string_view sv) {
    std::string result;
    result.reserve(sv.size());
    for (char c : sv)
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return result;
}

bool is_comparison_op(std::string_view lexeme) {
    return lexeme == "=" || lexeme == "!=" || lexeme == "<>" ||
           lexeme == "<" || lexeme == ">" || lexeme == "<=" || lexeme == ">=";
}

bool is_additive_op(std::string_view lexeme) {
    return lexeme == "+" || lexeme == "-";
}

}

Parser::Parser(Lexer lexer)
    : _lexer(lexer), _current(_lexer.Next()) {}

Token Parser::peek() const {
    return _current;
}

bool Parser::at_end() const {
    return _current.type == TokenType::EndOfFile;
}

bool Parser::check(TokenType type) const {
    return _current.type == type;
}

bool Parser::check_keyword(std::string_view keyword) const {
    return _current.type == TokenType::Keyword &&
           lowercase(_current.lexeme) == keyword;
}

Token Parser::advance() {
    Token prev = _current;
    _current = _lexer.Next();
    return prev;
}

Token Parser::consume(TokenType expected) {
    if (_current.type != expected) {
        error("expected " + std::string(ToString(expected)) + ", got " +
              std::string(ToString(_current.type)) + " '" +
              std::string(_current.lexeme) + "'");
    }
    return advance();
}

Token Parser::consume_keyword(std::string_view keyword) {
    if (!check_keyword(keyword)) {
        error("expected keyword " + std::string(keyword) + ", got '" +
              std::string(_current.lexeme) + "'");
    }
    return advance();
}

bool Parser::match_keyword(std::string_view keyword) {
    if (check_keyword(keyword)) {
        advance();
        return true;
    }
    return false;
}

[[noreturn]] void Parser::error(const std::string& msg) {
    throw DbError(ErrorCode::ParseError, msg, _current.pos);
}

[[noreturn]] void Parser::error_at(const Token& tok, const std::string& msg) {
    throw DbError(ErrorCode::ParseError, msg, tok.pos);
}

// ENTRYPOINT
std::unique_ptr<AstNode> Parser::Parse(const std::string& sql) {
    Lexer lexer{sql};
    Parser parser{std::move(lexer)};

    auto stmt = parser.parse_select_stmt();

    if (!parser.at_end() && !parser.check(TokenType::Semicolon)) {
        parser.error("unexpected token after statement: '" +
                     std::string(parser.peek().lexeme) + "'");
    }

    return stmt;
}

// SELECT statement
std::unique_ptr<SelectStmt> Parser::parse_select_stmt() {
    consume_keyword("select");

    auto stmt = std::make_unique<SelectStmt>();
    stmt->target_list = parse_target_list();

    consume_keyword("from");
    Token table_tok = consume(TokenType::Identifier);
    stmt->from_table = std::string(table_tok.lexeme);

    if (match_keyword("where")) {
        stmt->where = parse_expr();
    }

    if (match_keyword("limit")) {
        Token limit_tok = consume(TokenType::Number);
        stmt->limit = std::stoull(std::string(limit_tok.lexeme));
    }

    return stmt;
}

// target list
std::vector<SelectTarget> Parser::parse_target_list() {
    std::vector<SelectTarget> targets;

    // SELECT *
    if (check(TokenType::Operator) && _current.lexeme == "*") {
        advance();
        targets.emplace_back(StarTarget{});
        return targets;
    }

    targets.emplace_back(parse_target_entry());
    while (check(TokenType::Comma)) {
        advance();
        targets.emplace_back(parse_target_entry());
    }

    return targets;
}

ResTarget Parser::parse_target_entry() {
    ResTarget target;
    target.val = parse_expr();

    if (match_keyword("as")) {
        Token alias_tok = consume(TokenType::Identifier);
        target.alias = std::string(alias_tok.lexeme);
    }

    return target;
}

// expression precedence chain
std::unique_ptr<Expr> Parser::parse_expr() {
    return parse_or_expr();
}

std::unique_ptr<Expr> Parser::parse_or_expr() {
    auto left = parse_and_expr();

    while (check_keyword("or")) {
        advance();
        auto right = parse_and_expr();
        auto node = std::make_unique<BinaryExpr>();
        node->op = "OR";
        node->lhs = std::move(left);
        node->rhs = std::move(right);
        left = std::move(node);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parse_and_expr() {
    auto left = parse_not_expr();

    while (check_keyword("and")) {
        advance();
        auto right = parse_not_expr();
        auto node = std::make_unique<BinaryExpr>();
        node->op = "AND";
        node->lhs = std::move(left);
        node->rhs = std::move(right);
        left = std::move(node);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parse_not_expr() {
    if (check_keyword("not")) {
        advance();
        auto operand = parse_not_expr();
        auto node = std::make_unique<UnaryExpr>();
        node->op = "NOT";
        node->operand = std::move(operand);
        return node;
    }

    return parse_comparison_expr();
}

std::unique_ptr<Expr> Parser::parse_comparison_expr() {
    auto left = parse_additive_expr();

    if (check(TokenType::Operator) && is_comparison_op(_current.lexeme)) {
        std::string op(advance().lexeme);
        auto right = parse_additive_expr();
        auto node = std::make_unique<BinaryExpr>();
        node->op = std::move(op);
        node->lhs = std::move(left);
        node->rhs = std::move(right);
        return node;
    }

    return left;
}

std::unique_ptr<Expr> Parser::parse_additive_expr() {
    auto left = parse_primary_expr();

    while (check(TokenType::Operator) && is_additive_op(_current.lexeme)) {
        std::string op(advance().lexeme);
        auto right = parse_primary_expr();
        auto node = std::make_unique<BinaryExpr>();
        node->op = std::move(op);
        node->lhs = std::move(left);
        node->rhs = std::move(right);
        left = std::move(node);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parse_primary_expr() {
    // Identifier (possibly table-qualified: table.column)
    if (check(TokenType::Identifier)) {
        Token id_tok = advance();
        auto col = std::make_unique<ColumnRef>();
        col->name = std::string(id_tok.lexeme);

        if (check(TokenType::Dot)) {
            advance();
            Token col_tok = consume(TokenType::Identifier);
            col->table_qualifier = std::move(col->name);
            col->name = std::string(col_tok.lexeme);
        }

        return col;
    }

    // number literal
    if (check(TokenType::Number)) {
        Token num_tok = advance();
        auto lit = std::make_unique<Literal>();
        lit->lit_type = Literal::LiteralType::Integer;
        lit->value = std::string(num_tok.lexeme);
        return lit;
    }

    // string literal
    if (check(TokenType::StringLiteral)) {
        Token str_tok = advance();
        auto lit = std::make_unique<Literal>();
        lit->lit_type = Literal::LiteralType::String;
        // strip surrounding quotes from the lexeme
        std::string_view raw = str_tok.lexeme;
        lit->value = std::string(raw.substr(1, raw.size() - 2));
        return lit;
    }

    // NULL
    if (check_keyword("null")) {
        advance();
        auto lit = std::make_unique<Literal>();
        lit->lit_type = Literal::LiteralType::Null;
        lit->value = "NULL";
        return lit;
    }

    // TRUE / FALSE as literals
    if (check_keyword("true")) {
        advance();
        auto lit = std::make_unique<Literal>();
        lit->lit_type = Literal::LiteralType::Integer;
        lit->value = "1";
        return lit;
    }
    if (check_keyword("false")) {
        advance();
        auto lit = std::make_unique<Literal>();
        lit->lit_type = Literal::LiteralType::Integer;
        lit->value = "0";
        return lit;
    }

    // parenthesized expression
    if (check(TokenType::LParen)) {
        advance();
        auto expr = parse_expr();
        consume(TokenType::RParen);
        return expr;
    }

    // unary minus
    if (check(TokenType::Operator) && _current.lexeme == "-") {
        advance();
        auto operand = parse_primary_expr();
        auto node = std::make_unique<UnaryExpr>();
        node->op = "-";
        node->operand = std::move(operand);
        return node;
    }

    error("expected expression, got '" + std::string(_current.lexeme) + "'");
}
}
