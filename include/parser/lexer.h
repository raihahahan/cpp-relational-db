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
    Operator,
    Comma,
    LParen,
    RParen,
    Semicolon,
    EndOfFile
};

std::string_view ToString(TokenType type);

inline constexpr std::array<std::pair<std::string_view, TokenType>, 3> keywords = {{
    {"select", TokenType::Keyword},
    {"from",   TokenType::Keyword},
    {"where",  TokenType::Keyword},
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
    Token lex_operator();

    std::string_view _input;
    size_t _pos;
};

}