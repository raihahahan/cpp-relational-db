#pragma once
#include <string>
#include <vector>

namespace db::parser {

constexpr bool is_operator_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '>' || c == '<' || c == '!';
}

constexpr std::array<std::string_view, 4> two_char_ops = {
    "<=", ">=", "!=", "<>"
};

enum class TokenType {
    Identifier,
    Keyword,
    Number,
    StringLiteral,
    Operator,
    Comma,
    Dot,
    LParen,
    RParen,
    Semicolon,
    EndOfFile
};

std::string_view ToString(TokenType type);

inline constexpr std::array<std::pair<std::string_view, TokenType>, 42>
    keywords = {{
        {"select", TokenType::Keyword},
        {"from", TokenType::Keyword},
        {"where", TokenType::Keyword},
        {"limit", TokenType::Keyword},
        {"and", TokenType::Keyword},
        {"or", TokenType::Keyword},
        {"not", TokenType::Keyword},
        {"as", TokenType::Keyword},
        {"is", TokenType::Keyword},
        {"null", TokenType::Keyword},
        // near-future: DML
        {"create", TokenType::Keyword},
        {"table", TokenType::Keyword},
        {"insert", TokenType::Keyword},
        {"into", TokenType::Keyword},
        {"values", TokenType::Keyword},
        {"update", TokenType::Keyword},
        {"set", TokenType::Keyword},
        {"delete", TokenType::Keyword},
        // near-future: ordering / grouping
        {"order", TokenType::Keyword},
        {"by", TokenType::Keyword},
        {"asc", TokenType::Keyword},
        {"desc", TokenType::Keyword},
        {"group", TokenType::Keyword},
        {"having", TokenType::Keyword},
        {"distinct", TokenType::Keyword},
        // near-future: joins
        {"join", TokenType::Keyword},
        {"left", TokenType::Keyword},
        {"right", TokenType::Keyword},
        {"inner", TokenType::Keyword},
        {"outer", TokenType::Keyword},
        {"on", TokenType::Keyword},
        {"cross", TokenType::Keyword},
        // near-future: predicates / misc
        {"in", TokenType::Keyword},
        {"between", TokenType::Keyword},
        {"like", TokenType::Keyword},
        {"exists", TokenType::Keyword},
        {"true", TokenType::Keyword},
        {"false", TokenType::Keyword},
        // near-future: DDL
        {"primary", TokenType::Keyword},
        {"key", TokenType::Keyword},
        {"drop", TokenType::Keyword},
        {"if", TokenType::Keyword},
    }};

struct Token {
    TokenType type;
    std::string_view lexeme;
    size_t pos; // byte offset in input
};

class Lexer {
public:
    explicit Lexer(std::string_view input);
    std::vector<Token> LexicalParse();
    Token Next();
    Token Peek();

private:
    void advance();
    char peek() const;
    bool eof() const;

    Token lex_identifier_or_keyword();
    Token lex_number();
    Token lex_string();
    Token lex_operator();

    std::string_view _input;
    size_t _pos;
};

}