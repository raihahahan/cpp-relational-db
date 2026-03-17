#include <gtest/gtest.h>

#include <string>
#include "parser/parser.h"
#include "error/dberror.h"

using namespace db::parser;

static SelectStmt* parse_select(const std::string& sql,
                                std::unique_ptr<AstNode>& owner) {
    owner = Parser::Parse(sql);
    return dynamic_cast<SelectStmt*>(owner.get());
}


// Basic SELECT
TEST(ParserSelect, SelectStar) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM users", ast);

    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->target_list.size(), 1);
    EXPECT_TRUE(std::holds_alternative<StarTarget>(stmt->target_list[0]));
    EXPECT_EQ(stmt->from_table, "users");
    EXPECT_EQ(stmt->where, nullptr);
    EXPECT_FALSE(stmt->limit.has_value());
}

TEST(ParserSelect, SelectColumns) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT name, age FROM users", ast);

    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->target_list.size(), 2);

    auto& t0 = std::get<ResTarget>(stmt->target_list[0]);
    auto* col0 = dynamic_cast<ColumnRef*>(t0.val.get());
    ASSERT_NE(col0, nullptr);
    EXPECT_EQ(col0->name, "name");

    auto& t1 = std::get<ResTarget>(stmt->target_list[1]);
    auto* col1 = dynamic_cast<ColumnRef*>(t1.val.get());
    ASSERT_NE(col1, nullptr);
    EXPECT_EQ(col1->name, "age");
}

TEST(ParserSelect, SelectWithAlias) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT name AS n FROM t", ast);

    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->target_list.size(), 1);

    auto& t0 = std::get<ResTarget>(stmt->target_list[0]);
    EXPECT_EQ(t0.alias, "n");
    auto* col = dynamic_cast<ColumnRef*>(t0.val.get());
    ASSERT_NE(col, nullptr);
    EXPECT_EQ(col->name, "name");
}

TEST(ParserSelect, QualifiedColumn) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT t.id FROM t", ast);

    ASSERT_NE(stmt, nullptr);
    auto& t0 = std::get<ResTarget>(stmt->target_list[0]);
    auto* col = dynamic_cast<ColumnRef*>(t0.val.get());
    ASSERT_NE(col, nullptr);
    EXPECT_EQ(col->table_qualifier, "t");
    EXPECT_EQ(col->name, "id");
}


// WHERE clause
TEST(ParserWhere, SimpleComparison) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE id = 1", ast);

    ASSERT_NE(stmt, nullptr);
    ASSERT_NE(stmt->where, nullptr);

    auto* bin = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "=");

    auto* lhs = dynamic_cast<ColumnRef*>(bin->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->name, "id");

    auto* rhs = dynamic_cast<Literal*>(bin->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->lit_type, Literal::LiteralType::Integer);
    EXPECT_EQ(rhs->value, "1");
}

TEST(ParserWhere, AndExpression) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE a = 1 AND b = 2", ast);

    auto* bin = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "AND");

    EXPECT_NE(dynamic_cast<BinaryExpr*>(bin->lhs.get()), nullptr);
    EXPECT_NE(dynamic_cast<BinaryExpr*>(bin->rhs.get()), nullptr);
}

TEST(ParserWhere, OrExpression) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE a = 1 OR b = 2", ast);

    auto* bin = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "OR");
}

TEST(ParserWhere, NotExpression) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE NOT a = 1", ast);

    auto* un = dynamic_cast<UnaryExpr*>(stmt->where.get());
    ASSERT_NE(un, nullptr);
    EXPECT_EQ(un->op, "NOT");

    auto* inner = dynamic_cast<BinaryExpr*>(un->operand.get());
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op, "=");
}

TEST(ParserWhere, Precedence_AndBeforeOr) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE a = 1 OR b = 2 AND c = 3", ast);

    // Should parse as: a=1 OR (b=2 AND c=3)
    auto* top = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(top, nullptr);
    EXPECT_EQ(top->op, "OR");

    // RHS of OR should be AND
    auto* rhs = dynamic_cast<BinaryExpr*>(top->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->op, "AND");
}

TEST(ParserWhere, ParenthesizedExpr) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE (a = 1 OR b = 2) AND c = 3", ast);

    // Should parse as: (a=1 OR b=2) AND c=3
    auto* top = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(top, nullptr);
    EXPECT_EQ(top->op, "AND");

    // LHS of AND should be OR
    auto* lhs = dynamic_cast<BinaryExpr*>(top->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->op, "OR");
}


// Literals
TEST(ParserLiterals, StringLiteral) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE name = 'alice'", ast);

    auto* bin = dynamic_cast<BinaryExpr*>(stmt->where.get());
    auto* rhs = dynamic_cast<Literal*>(bin->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->lit_type, Literal::LiteralType::String);
    EXPECT_EQ(rhs->value, "alice");
}

TEST(ParserLiterals, NullLiteral) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE val = NULL", ast);

    auto* bin = dynamic_cast<BinaryExpr*>(stmt->where.get());
    auto* rhs = dynamic_cast<Literal*>(bin->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->lit_type, Literal::LiteralType::Null);
}

TEST(ParserLiterals, TrueFalse) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE a = TRUE AND b = FALSE", ast);

    auto* top = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(top, nullptr);
    EXPECT_EQ(top->op, "AND");

    auto* lhs_cmp = dynamic_cast<BinaryExpr*>(top->lhs.get());
    auto* true_lit = dynamic_cast<Literal*>(lhs_cmp->rhs.get());
    ASSERT_NE(true_lit, nullptr);
    EXPECT_EQ(true_lit->lit_type, Literal::LiteralType::Integer);
    EXPECT_EQ(true_lit->value, "1");

    auto* rhs_cmp = dynamic_cast<BinaryExpr*>(top->rhs.get());
    auto* false_lit = dynamic_cast<Literal*>(rhs_cmp->rhs.get());
    ASSERT_NE(false_lit, nullptr);
    EXPECT_EQ(false_lit->value, "0");
}


// Arithmetic
TEST(ParserArithmetic, Addition) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT a + 1 FROM t", ast);

    auto& t0 = std::get<ResTarget>(stmt->target_list[0]);
    auto* bin = dynamic_cast<BinaryExpr*>(t0.val.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "+");
}

TEST(ParserArithmetic, UnaryMinus) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t WHERE a = -5", ast);

    auto* cmp = dynamic_cast<BinaryExpr*>(stmt->where.get());
    auto* rhs = dynamic_cast<UnaryExpr*>(cmp->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->op, "-");

    auto* lit = dynamic_cast<Literal*>(rhs->operand.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, "5");
}


// LIMIT
TEST(ParserLimit, WithLimit) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t LIMIT 10", ast);

    ASSERT_TRUE(stmt->limit.has_value());
    EXPECT_EQ(stmt->limit.value(), 10u);
}

TEST(ParserLimit, NoLimit) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t", ast);

    EXPECT_FALSE(stmt->limit.has_value());
}

TEST(ParserLimit, LimitZero) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t LIMIT 0", ast);

    ASSERT_TRUE(stmt->limit.has_value());
    EXPECT_EQ(stmt->limit.value(), 0u);
}


// Comparison operators
TEST(ParserComparison, AllOperators) {
    std::vector<std::string> ops = {"=", "!=", "<>", "<", ">", "<=", ">="};
    for (const auto& op : ops) {
        std::string sql = "SELECT * FROM t WHERE a " + op + " 1";
        std::unique_ptr<AstNode> ast;
        auto* stmt = parse_select(sql, ast);

        auto* bin = dynamic_cast<BinaryExpr*>(stmt->where.get());
        ASSERT_NE(bin, nullptr) << "op: " << op;
        EXPECT_EQ(bin->op, op) << "op: " << op;
    }
}


// Error handling
TEST(ParserErrors, MissingTargetList) {
    EXPECT_THROW(Parser::Parse("SELECT FROM t"), DbError);
}

TEST(ParserErrors, MissingFromClause) {
    EXPECT_THROW(Parser::Parse("SELECT a b FROM t"), DbError);
}

TEST(ParserErrors, MissingTableName) {
    EXPECT_THROW(Parser::Parse("SELECT a FROM"), DbError);
}

TEST(ParserErrors, UnexpectedTrailingToken) {
    EXPECT_THROW(Parser::Parse("SELECT * FROM t GARBAGE"), DbError);
}

TEST(ParserErrors, EmptyInput) {
    EXPECT_THROW(Parser::Parse(""), DbError);
}


// Integration: complex query
TEST(ParserIntegration, ComplexQuery) {
    std::string sql =
        "SELECT name, age + 1 AS next_age, 'active' AS status "
        "FROM users "
        "WHERE (age >= 18 AND NOT name = 'admin') "
        "OR (score > 3 AND active != NULL) "
        "LIMIT 25";

    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select(sql, ast);

    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->from_table, "users");
    ASSERT_EQ(stmt->target_list.size(), 3);

    // target 0: name (no alias)
    auto& t0 = std::get<ResTarget>(stmt->target_list[0]);
    EXPECT_TRUE(t0.alias.empty());
    EXPECT_NE(dynamic_cast<ColumnRef*>(t0.val.get()), nullptr);

    // target 1: age + 1 AS next_age
    auto& t1 = std::get<ResTarget>(stmt->target_list[1]);
    EXPECT_EQ(t1.alias, "next_age");
    EXPECT_NE(dynamic_cast<BinaryExpr*>(t1.val.get()), nullptr);

    // target 2: 'active' AS status
    auto& t2 = std::get<ResTarget>(stmt->target_list[2]);
    EXPECT_EQ(t2.alias, "status");
    auto* lit = dynamic_cast<Literal*>(t2.val.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->lit_type, Literal::LiteralType::String);
    EXPECT_EQ(lit->value, "active");

    // WHERE is OR at top level
    auto* top_or = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(top_or, nullptr);
    EXPECT_EQ(top_or->op, "OR");

    // LIMIT 25
    ASSERT_TRUE(stmt->limit.has_value());
    EXPECT_EQ(stmt->limit.value(), 25u);
}

TEST(ParserIntegration, CaseInsensitiveKeywords) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("select * from T where A = 1 limit 5", ast);

    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->from_table, "T");
    ASSERT_TRUE(stmt->limit.has_value());
    EXPECT_EQ(stmt->limit.value(), 5u);
}

TEST(ParserIntegration, TrailingSemicolon) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_select("SELECT * FROM t;", ast);

    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->from_table, "t");
}


// DELETE parsing

static DeleteStmt* parse_delete(const std::string& sql,
                                std::unique_ptr<AstNode>& owner) {
    owner = Parser::Parse(sql);
    return dynamic_cast<DeleteStmt*>(owner.get());
}

TEST(ParserDelete, DeleteWithWhere) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_delete("DELETE FROM users WHERE id = 1", ast);

    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->table_name, "users");
    ASSERT_NE(stmt->where, nullptr);

    auto* bin = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "=");

    auto* lhs = dynamic_cast<ColumnRef*>(bin->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->name, "id");
}

TEST(ParserDelete, DeleteAllRows) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_delete("DELETE FROM users", ast);

    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->table_name, "users");
    EXPECT_EQ(stmt->where, nullptr);
}

TEST(ParserDelete, DeleteWithSemicolon) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_delete("DELETE FROM users;", ast);

    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->table_name, "users");
}

TEST(ParserDelete, DeleteComplexWhere) {
    std::unique_ptr<AstNode> ast;
    auto* stmt = parse_delete("DELETE FROM users WHERE age > 30 AND name = 'admin'", ast);

    ASSERT_NE(stmt, nullptr);
    auto* top = dynamic_cast<BinaryExpr*>(stmt->where.get());
    ASSERT_NE(top, nullptr);
    EXPECT_EQ(top->op, "AND");
}

TEST(ParserDelete, DeleteMissingFrom) {
    EXPECT_THROW(Parser::Parse("DELETE users"), DbError);
}

TEST(ParserDelete, DeleteMissingTable) {
    EXPECT_THROW(Parser::Parse("DELETE FROM"), DbError);
}
