#include "parser/parser.h"
#include "error/dberror.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace db::parser;

static void print_expr(const Expr* expr, int indent);

static void pad(int indent) {
    for (int i = 0; i < indent; ++i)
        std::cout << "  ";
}

static void print_expr(const Expr* expr, int indent) {
    if (!expr) {
        pad(indent);
        std::cout << "(null)\n";
        return;
    }

    if (auto* col = dynamic_cast<const ColumnRef*>(expr)) {
        pad(indent);
        if (!col->table_qualifier.empty())
            std::cout << "ColumnRef: " << col->table_qualifier << "." << col->name << "\n";
        else
            std::cout << "ColumnRef: " << col->name << "\n";
        return;
    }

    if (auto* lit = dynamic_cast<const Literal*>(expr)) {
        pad(indent);
        std::cout << "Literal(";
        switch (lit->lit_type) {
        case Literal::LiteralType::Integer: std::cout << "int"; break;
        case Literal::LiteralType::String: std::cout << "str"; break;
        case Literal::LiteralType::Null: std::cout << "null"; break;
        }
        std::cout << "): " << lit->value << "\n";
        return;
    }

    if (auto* bin = dynamic_cast<const BinaryExpr*>(expr)) {
        pad(indent);
        std::cout << "BinaryExpr: " << bin->op << "\n";
        print_expr(bin->lhs.get(), indent + 1);
        print_expr(bin->rhs.get(), indent + 1);
        return;
    }

    if (auto* un = dynamic_cast<const UnaryExpr*>(expr)) {
        pad(indent);
        std::cout << "UnaryExpr: " << un->op << "\n";
        print_expr(un->operand.get(), indent + 1);
        return;
    }

    pad(indent);
    std::cout << "(unknown expr)\n";
}

int main() {
    std::ostringstream buf;
    buf << std::cin.rdbuf();
    std::string input = buf.str();

    std::cout << "Input: " << input << "\n";

    try {
        auto ast = Parser::Parse(input);
        auto* stmt = dynamic_cast<SelectStmt*>(ast.get());
        if (!stmt) {
            std::cout << "Error: parsed node is not a SelectStmt\n";
            return 1;
        }

        std::cout << "SelectStmt\n";
        std::cout << "  from_table: " << stmt->from_table << "\n";

        std::cout << "  target_list:\n";
        for (size_t i = 0; i < stmt->target_list.size(); ++i) {
            auto& t = stmt->target_list[i];
            if (std::holds_alternative<StarTarget>(t)) {
                std::cout << "    [" << i << "] *\n";
            } else {
                auto& res = std::get<ResTarget>(t);
                std::cout << "    [" << i << "]";
                if (!res.alias.empty())
                    std::cout << " AS " << res.alias;
                std::cout << "\n";
                print_expr(res.val.get(), 3);
            }
        }

        if (stmt->where) {
            std::cout << "  where:\n";
            print_expr(stmt->where.get(), 2);
        } else {
            std::cout << "  where: (none)\n";
        }

        if (stmt->limit.has_value())
            std::cout << "  limit: " << stmt->limit.value() << "\n";
        else
            std::cout << "  limit: (none)\n";

    } catch (const DbError& e) {
        std::cout << "Error [pos " << e.position() << "]: " << e.what() << "\n";
        return 1;
    }

    return 0;
}