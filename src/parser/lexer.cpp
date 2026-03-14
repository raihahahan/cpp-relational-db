#include "parser/lexer.h"
#include "error/dberror.h"

namespace db::parser {
Lexer::Lexer(std::string_view input) : _input{input}, _pos{0} {};

std::string_view ToString(TokenType type) {
    switch (type) {
        case TokenType::Identifier: return "Identifier";
        case TokenType::Keyword: return "Keyword";
        case TokenType::Number: return "Number";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::Operator: return "Operator";
        case TokenType::Comma: return "Comma";
        case TokenType::Dot: return "Dot";
        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::EndOfFile: return "EndOfFile";
    }
}

std::vector<Token> Lexer::LexicalParse() {
    std::vector<Token> res;
    while (true) {
        Token t = Next();
        res.push_back(t);
        if (t.type == TokenType::EndOfFile) break;
    }
    return res;
}

Token Lexer::Next() {
    while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (eof()) {
        return {TokenType::EndOfFile, "", _pos};
    }

    char c = peek();
    size_t start = _pos;

    // identifier or keyword
    if (std::isalpha(c) || c == '_') {
        return lex_identifier_or_keyword();
    }

    // number
    if (std::isdigit(c)) {
        return lex_number();
    }

    // string literal
    if (c == '\'') {
        return lex_string();
    }

    // single-char tokens
    switch (c) {
        case ',': advance(); return {TokenType::Comma, _input.substr(start, 1), start};
        case '.': advance(); return {TokenType::Dot, _input.substr(start, 1), start};
        case '(': advance(); return {TokenType::LParen, _input.substr(start, 1), start};
        case ')': advance(); return {TokenType::RParen, _input.substr(start, 1), start};
        case ';': advance(); return {TokenType::Semicolon, _input.substr(start, 1), start};
    }

    // operators
    if (is_operator_char(c)) {
        return lex_operator();
    }

    throw DbError(ErrorCode::SyntaxError,
                   "Unexpected character: " +
                       std::string(_input.substr(_pos, _pos + 1)),
                   _pos);
}

Token Lexer::Peek() {
    size_t saved = _pos;
    Token t = Next();
    _pos = saved;
    return t;
}

bool Lexer::eof() const {
    return _pos == _input.size();
}

char Lexer::peek() const {
    return eof() ? '\0' : _input[_pos];
}

void Lexer::advance() {
    ++_pos;
}

Token Lexer::lex_identifier_or_keyword() {
    size_t start = _pos;

    while (!eof() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }

    auto text = _input.substr(start, _pos - start);

    // keywords are case insensitive
    std::string lower;
    lower.reserve(text.size());
    for (char c : text) lower.push_back(std::tolower(c));

    for (auto& [kw, type] : keywords) {
        if (lower == kw) {
            return {type, text, start};
        }
    }

    return {TokenType::Identifier, text, start};
}

Token Lexer::lex_number() {
    size_t start = _pos;

    while (!eof() && std::isdigit(peek())) {
        advance();
    }

    return {TokenType::Number, _input.substr(start, _pos - start), start};
}

Token Lexer::lex_string() {
    size_t start = _pos;
    advance(); // skip opening quote

    while (!eof()) {
        if (peek() == '\'') {
            advance();
            if (!eof() && peek() == '\'') {
                advance(); // escaped quote (''), keep scanning
            } else {
                return {TokenType::StringLiteral, _input.substr(start, _pos - start), start};
            }
        } else {
            advance();
        }
    }

    throw DbError(
        ErrorCode::SyntaxError,
        "Unterminated string literal",
        start);
}

Token Lexer::lex_operator() {
    size_t start = _pos;

    advance(); // consume first char

    // for cases like >=, <=, !=, <> etc
    if (!eof()) {
        std::string_view candidate = _input.substr(start, 2);
        for (auto op : two_char_ops) {
            if (candidate == op) {
                advance(); // consume second char
                return {TokenType::Operator, candidate, start};
            }
        }
    }

    // fallback: single-char operator
    // discard invalid operators first
    if (_input.substr(start, 1) == "!") {
        throw DbError(ErrorCode::SyntaxError,
                       "Unexpected character: " +
                           std::string(_input.substr(start, 1)),
                       _pos);
    }

    return {TokenType::Operator, _input.substr(start, 1), start};
}

}