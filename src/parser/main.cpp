#include "parser/lexer.h"
#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::ostringstream buf;
    buf << std::cin.rdbuf();
    std::string input = buf.str();

    db::parser::Lexer lexer{input};
    auto res = lexer.LexicalParse();

    std::cout << "Input: " << input << std::endl;
    for (const auto& token : res) {
        std::cout << "Type: " << \
            db::parser::ToString(token.type) << ", Lexeme: " << \
            token.lexeme << ", pos: " <<  token.pos << std::endl;
    }

    return 0;
}