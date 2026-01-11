#pragma once
#include "parser/ast.h"

namespace db::parser {
class Parser {
public:
    static std::unique_ptr<AstNode> Parse(const std::string& sql);
};
}