#include <gtest/gtest.h>

#include "parser/parser.h"
#include "parser/analyzer.h"
#include "catalog/catalog_bootstrap.h"
#include "error/dberror.h"
#include "executor/test_db_helper.h"

using namespace db::parser;
using namespace db::catalog;

class AnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<TestDB>("analyzer_test.db");
        db_->table_mgr->CreateTable("users", {
            {"id", INT_TYPE, 1},
            {"name", TEXT_TYPE, 2},
            {"age", INT_TYPE, 3},
        });
    }

    std::unique_ptr<Query> analyze(const std::string& sql) {
        auto ast = Parser::Parse(sql);
        Analyzer analyzer(*db_->catalog);
        auto stmt = analyzer.Analyze(*ast);
        return std::move(stmt->select_query);
    }

    std::unique_ptr<AnalyzedStmt> analyze_stmt(const std::string& sql) {
        auto ast = Parser::Parse(sql);
        Analyzer analyzer(*db_->catalog);
        return analyzer.Analyze(*ast);
    }

    std::unique_ptr<TestDB> db_;
};


// Table resolution
TEST_F(AnalyzerTest, ResolvesTable) {
    auto q = analyze("SELECT * FROM users");

    EXPECT_EQ(q->range_table.table_name, "users");
    EXPECT_FALSE(q->table_columns.empty());
}

TEST_F(AnalyzerTest, UndefinedTableThrows) {
    EXPECT_THROW(analyze("SELECT * FROM nonexistent"), DbError);

    try {
        analyze("SELECT * FROM nonexistent");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedTable);
    }
}


// Wildcard expansion
TEST_F(AnalyzerTest, StarExpandsToAllColumns) {
    auto q = analyze("SELECT * FROM users");

    ASSERT_EQ(q->target_list.size(), 3);
    EXPECT_EQ(q->target_list[0].name, "id");
    EXPECT_EQ(q->target_list[1].name, "name");
    EXPECT_EQ(q->target_list[2].name, "age");
}

TEST_F(AnalyzerTest, StarResnoIsCorrect) {
    auto q = analyze("SELECT * FROM users");

    EXPECT_EQ(q->target_list[0].resno, 1);
    EXPECT_EQ(q->target_list[1].resno, 2);
    EXPECT_EQ(q->target_list[2].resno, 3);
}

TEST_F(AnalyzerTest, StarColumnTypesAreCorrect) {
    auto q = analyze("SELECT * FROM users");

    EXPECT_EQ(q->target_list[0].column.type_id, INT_TYPE);  // id
    EXPECT_EQ(q->target_list[1].column.type_id, TEXT_TYPE);  // name
    EXPECT_EQ(q->target_list[2].column.type_id, INT_TYPE);   // age
}


// Column resolution
TEST_F(AnalyzerTest, ResolvesNamedColumns) {
    auto q = analyze("SELECT id, name FROM users");

    ASSERT_EQ(q->target_list.size(), 2);
    EXPECT_EQ(q->target_list[0].name, "id");
    EXPECT_EQ(q->target_list[0].column.type_id, INT_TYPE);
    EXPECT_EQ(q->target_list[1].name, "name");
    EXPECT_EQ(q->target_list[1].column.type_id, TEXT_TYPE);
}

TEST_F(AnalyzerTest, UndefinedColumnThrows) {
    EXPECT_THROW(analyze("SELECT bad_col FROM users"), DbError);

    try {
        analyze("SELECT bad_col FROM users");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedColumn);
    }
}


// Aliases
TEST_F(AnalyzerTest, AliasOverridesColumnName) {
    auto q = analyze("SELECT id AS user_id FROM users");

    ASSERT_EQ(q->target_list.size(), 1);
    EXPECT_EQ(q->target_list[0].name, "user_id");
    EXPECT_EQ(q->target_list[0].column.col_name, "id");
}


// WHERE clause analysis
TEST_F(AnalyzerTest, SimpleWhereClause) {
    auto q = analyze("SELECT * FROM users WHERE id = 1");

    ASSERT_NE(q->where_clause, nullptr);
    auto* bin = dynamic_cast<AnalyzedBinaryExpr*>(q->where_clause.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "=");
    EXPECT_EQ(bin->result_type, INT_TYPE);
}

TEST_F(AnalyzerTest, WhereWithColumnRef) {
    auto q = analyze("SELECT * FROM users WHERE id = 1");

    auto* bin = dynamic_cast<AnalyzedBinaryExpr*>(q->where_clause.get());
    auto* lhs = dynamic_cast<AnalyzedColumnRef*>(bin->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->column.col_name, "id");
    EXPECT_EQ(lhs->result_type, INT_TYPE);
}

TEST_F(AnalyzerTest, WhereWithStringLiteral) {
    auto q = analyze("SELECT * FROM users WHERE name = 'alice'");

    auto* bin = dynamic_cast<AnalyzedBinaryExpr*>(q->where_clause.get());
    auto* rhs = dynamic_cast<AnalyzedLiteral*>(bin->rhs.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(rhs->value, "alice");
    EXPECT_EQ(rhs->result_type, TEXT_TYPE);
}

TEST_F(AnalyzerTest, WhereUndefinedColumnThrows) {
    EXPECT_THROW(analyze("SELECT * FROM users WHERE bad = 1"), DbError);

    try {
        analyze("SELECT * FROM users WHERE bad = 1");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedColumn);
    }
}

TEST_F(AnalyzerTest, NoWhereClause) {
    auto q = analyze("SELECT * FROM users");
    EXPECT_EQ(q->where_clause, nullptr);
}


// Type checking
TEST_F(AnalyzerTest, IntEqualsIntOk) {
    EXPECT_NO_THROW(analyze("SELECT * FROM users WHERE id = 1"));
}

TEST_F(AnalyzerTest, TextEqualsTextOk) {
    EXPECT_NO_THROW(analyze("SELECT * FROM users WHERE name = 'alice'"));
}

TEST_F(AnalyzerTest, TextGreaterThanIntThrows) {
    EXPECT_THROW(analyze("SELECT * FROM users WHERE name > 42"), DbError);

    try {
        analyze("SELECT * FROM users WHERE name > 42");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::TypeMismatch);
    }
}

TEST_F(AnalyzerTest, IntEqualsTextThrows) {
    EXPECT_THROW(analyze("SELECT * FROM users WHERE id = 'alice'"), DbError);

    try {
        analyze("SELECT * FROM users WHERE id = 'alice'");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::TypeMismatch);
    }
}

TEST_F(AnalyzerTest, NullComparisonOk) {
    EXPECT_NO_THROW(analyze("SELECT * FROM users WHERE id = NULL"));
    EXPECT_NO_THROW(analyze("SELECT * FROM users WHERE name = NULL"));
}


// LIMIT
TEST_F(AnalyzerTest, LimitCarriedThrough) {
    auto q = analyze("SELECT * FROM users LIMIT 10");

    ASSERT_TRUE(q->limit_count.has_value());
    EXPECT_EQ(q->limit_count.value(), 10u);
}

TEST_F(AnalyzerTest, NoLimitIsNullopt) {
    auto q = analyze("SELECT * FROM users");
    EXPECT_FALSE(q->limit_count.has_value());
}


// Boolean expressions
TEST_F(AnalyzerTest, AndExpression) {
    auto q = analyze("SELECT * FROM users WHERE id = 1 AND age > 18");

    auto* bin = dynamic_cast<AnalyzedBinaryExpr*>(q->where_clause.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "AND");
    EXPECT_EQ(bin->result_type, INT_TYPE);
}

TEST_F(AnalyzerTest, NotExpression) {
    auto q = analyze("SELECT * FROM users WHERE NOT id = 1");

    auto* un = dynamic_cast<AnalyzedUnaryExpr*>(q->where_clause.get());
    ASSERT_NE(un, nullptr);
    EXPECT_EQ(un->op, "NOT");
    EXPECT_EQ(un->result_type, INT_TYPE);
}


// Integration: complex query
TEST_F(AnalyzerTest, ComplexQuery) {
    auto q = analyze(
        "SELECT id AS user_id, name, age "
        "FROM users "
        "WHERE (age >= 18 AND NOT name = 'admin') "
        "OR id = 1 "
        "LIMIT 25");

    EXPECT_EQ(q->range_table.table_name, "users");
    ASSERT_EQ(q->target_list.size(), 3);
    EXPECT_EQ(q->target_list[0].name, "user_id");
    EXPECT_EQ(q->target_list[1].name, "name");
    EXPECT_EQ(q->target_list[2].name, "age");

    ASSERT_NE(q->where_clause, nullptr);
    auto* top_or = dynamic_cast<AnalyzedBinaryExpr*>(q->where_clause.get());
    ASSERT_NE(top_or, nullptr);
    EXPECT_EQ(top_or->op, "OR");

    ASSERT_TRUE(q->limit_count.has_value());
    EXPECT_EQ(q->limit_count.value(), 25u);
}


// DELETE analysis
TEST_F(AnalyzerTest, DeleteResolvesTable) {
    auto stmt = analyze_stmt("DELETE FROM users WHERE id = 1");

    ASSERT_EQ(stmt->type, StmtType::Delete);
    ASSERT_NE(stmt->delete_query, nullptr);
    EXPECT_EQ(stmt->delete_query->table.table_name, "users");
    EXPECT_FALSE(stmt->delete_query->table_columns.empty());
}

TEST_F(AnalyzerTest, DeleteWithWhere) {
    auto stmt = analyze_stmt("DELETE FROM users WHERE id = 1");

    ASSERT_NE(stmt->delete_query->where_clause, nullptr);
    auto* bin = dynamic_cast<AnalyzedBinaryExpr*>(
        stmt->delete_query->where_clause.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "=");
}

TEST_F(AnalyzerTest, DeleteAllRows) {
    auto stmt = analyze_stmt("DELETE FROM users");

    ASSERT_EQ(stmt->type, StmtType::Delete);
    EXPECT_EQ(stmt->delete_query->where_clause, nullptr);
}

TEST_F(AnalyzerTest, DeleteUndefinedTableThrows) {
    EXPECT_THROW(analyze_stmt("DELETE FROM ghosts"), DbError);

    try {
        analyze_stmt("DELETE FROM ghosts");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedTable);
    }
}

TEST_F(AnalyzerTest, DeleteUndefinedColumnInWhereThrows) {
    EXPECT_THROW(
        analyze_stmt("DELETE FROM users WHERE bad_col = 1"), DbError);

    try {
        analyze_stmt("DELETE FROM users WHERE bad_col = 1");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedColumn);
    }
}


// INSERT analysis
TEST_F(AnalyzerTest, InsertResolvesTable) {
    auto stmt = analyze_stmt(
        "INSERT INTO users (id, name, age) VALUES (1, 'Alice', 30)");

    ASSERT_EQ(stmt->type, StmtType::Insert);
    ASSERT_NE(stmt->insert_query, nullptr);
    EXPECT_EQ(stmt->insert_query->table.table_name, "users");
}

TEST_F(AnalyzerTest, InsertResolvesColumns) {
    auto stmt = analyze_stmt(
        "INSERT INTO users (id, name, age) VALUES (1, 'Alice', 30)");

    ASSERT_EQ(stmt->insert_query->target_columns.size(), 3);
    EXPECT_EQ(stmt->insert_query->target_columns[0].col_name, "id");
    EXPECT_EQ(stmt->insert_query->target_columns[1].col_name, "name");
    EXPECT_EQ(stmt->insert_query->target_columns[2].col_name, "age");
}

TEST_F(AnalyzerTest, InsertWithoutColumnsUsesAll) {
    auto stmt = analyze_stmt(
        "INSERT INTO users VALUES (1, 'Alice', 30)");

    ASSERT_EQ(stmt->insert_query->target_columns.size(), 3);
    EXPECT_EQ(stmt->insert_query->target_columns[0].col_name, "id");
    EXPECT_EQ(stmt->insert_query->target_columns[1].col_name, "name");
    EXPECT_EQ(stmt->insert_query->target_columns[2].col_name, "age");
}

TEST_F(AnalyzerTest, InsertMultiRow) {
    auto stmt = analyze_stmt(
        "INSERT INTO users VALUES (1, 'A', 20), (2, 'B', 30)");

    ASSERT_EQ(stmt->insert_query->values.size(), 2);
    ASSERT_EQ(stmt->insert_query->values[0].size(), 3);
    ASSERT_EQ(stmt->insert_query->values[1].size(), 3);
}

TEST_F(AnalyzerTest, InsertTypeMismatchThrows) {
    EXPECT_THROW(
        analyze_stmt("INSERT INTO users (id, name, age) VALUES ('bad', 'X', 1)"),
        DbError);

    try {
        analyze_stmt("INSERT INTO users (id, name, age) VALUES ('bad', 'X', 1)");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::TypeMismatch);
    }
}

TEST_F(AnalyzerTest, InsertWrongColumnCountThrows) {
    EXPECT_THROW(
        analyze_stmt("INSERT INTO users (id, name) VALUES (1, 'A', 99)"),
        DbError);

    try {
        analyze_stmt("INSERT INTO users (id, name) VALUES (1, 'A', 99)");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::ParseError);
    }
}

TEST_F(AnalyzerTest, InsertPartialColumnsThrows) {
    EXPECT_THROW(
        analyze_stmt("INSERT INTO users (name, age) VALUES ('Alice', 30)"),
        DbError);

    try {
        analyze_stmt("INSERT INTO users (name, age) VALUES ('Alice', 30)");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::ParseError);
    }
}

TEST_F(AnalyzerTest, InsertUndefinedColumnThrows) {
    EXPECT_THROW(
        analyze_stmt("INSERT INTO users (id, bad_col) VALUES (1, 'X')"),
        DbError);

    try {
        analyze_stmt("INSERT INTO users (id, bad_col) VALUES (1, 'X')");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedColumn);
    }
}

TEST_F(AnalyzerTest, InsertUndefinedTableThrows) {
    EXPECT_THROW(
        analyze_stmt("INSERT INTO ghosts VALUES (1)"), DbError);

    try {
        analyze_stmt("INSERT INTO ghosts VALUES (1)");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedTable);
    }
}


// UPDATE analysis
TEST_F(AnalyzerTest, UpdateResolvesTable) {
    auto stmt = analyze_stmt("UPDATE users SET name = 'Bob' WHERE id = 1");

    ASSERT_EQ(stmt->type, StmtType::Update);
    ASSERT_NE(stmt->update_query, nullptr);
    EXPECT_EQ(stmt->update_query->table.table_name, "users");
}

TEST_F(AnalyzerTest, UpdateResolvesSetColumn) {
    auto stmt = analyze_stmt("UPDATE users SET name = 'Bob' WHERE id = 1");

    ASSERT_EQ(stmt->update_query->assignments.size(), 1);
    EXPECT_EQ(stmt->update_query->assignments[0].first.col_name, "name");
}

TEST_F(AnalyzerTest, UpdateMultipleSetClauses) {
    auto stmt = analyze_stmt(
        "UPDATE users SET name = 'Bob', age = 40 WHERE id = 1");

    ASSERT_EQ(stmt->update_query->assignments.size(), 2);
    EXPECT_EQ(stmt->update_query->assignments[0].first.col_name, "name");
    EXPECT_EQ(stmt->update_query->assignments[1].first.col_name, "age");
}

TEST_F(AnalyzerTest, UpdateAllRows) {
    auto stmt = analyze_stmt("UPDATE users SET age = 99");

    ASSERT_EQ(stmt->type, StmtType::Update);
    EXPECT_EQ(stmt->update_query->where_clause, nullptr);
}

TEST_F(AnalyzerTest, UpdateWithWhere) {
    auto stmt = analyze_stmt("UPDATE users SET age = 99 WHERE id = 1");

    ASSERT_NE(stmt->update_query->where_clause, nullptr);
    auto* bin = dynamic_cast<AnalyzedBinaryExpr*>(
        stmt->update_query->where_clause.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, "=");
}

TEST_F(AnalyzerTest, UpdateTypeMismatchThrows) {
    EXPECT_THROW(
        analyze_stmt("UPDATE users SET id = 'bad' WHERE id = 1"),
        DbError);

    try {
        analyze_stmt("UPDATE users SET id = 'bad' WHERE id = 1");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::TypeMismatch);
    }
}

TEST_F(AnalyzerTest, UpdateUndefinedColumnThrows) {
    EXPECT_THROW(
        analyze_stmt("UPDATE users SET bad_col = 1"), DbError);

    try {
        analyze_stmt("UPDATE users SET bad_col = 1");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedColumn);
    }
}

TEST_F(AnalyzerTest, UpdateUndefinedTableThrows) {
    EXPECT_THROW(
        analyze_stmt("UPDATE ghosts SET x = 1"), DbError);

    try {
        analyze_stmt("UPDATE ghosts SET x = 1");
    } catch (const DbError& e) {
        EXPECT_EQ(e.code(), ErrorCode::UndefinedTable);
    }
}


// ========== CREATE TABLE ==========

TEST_F(AnalyzerTest, CreateTableBasic) {
    auto stmt = analyze_stmt("CREATE TABLE products (id INT, name TEXT)");
    ASSERT_EQ(stmt->type, StmtType::CreateTable);
    ASSERT_NE(stmt->create_table, nullptr);
    EXPECT_EQ(stmt->create_table->table_name, "products");
    ASSERT_EQ(stmt->create_table->columns.size(), 2);
    EXPECT_EQ(stmt->create_table->columns[0].col_name, "id");
    EXPECT_EQ(stmt->create_table->columns[0].type_id, INT_TYPE);
    EXPECT_EQ(stmt->create_table->columns[0].ordinal_position, 1);
    EXPECT_EQ(stmt->create_table->columns[1].col_name, "name");
    EXPECT_EQ(stmt->create_table->columns[1].type_id, TEXT_TYPE);
    EXPECT_EQ(stmt->create_table->columns[1].ordinal_position, 2);
}

TEST_F(AnalyzerTest, CreateTableDuplicateTableThrows) {
    EXPECT_THROW(
        analyze_stmt("CREATE TABLE users (id INT)"), DbError);
}

TEST_F(AnalyzerTest, CreateTableDuplicateColumnThrows) {
    EXPECT_THROW(
        analyze_stmt("CREATE TABLE t (id INT, id TEXT)"), DbError);
}

TEST_F(AnalyzerTest, CreateTableUnknownTypeThrows) {
    EXPECT_THROW(
        analyze_stmt("CREATE TABLE t (id FLOAT)"), DbError);
}

TEST_F(AnalyzerTest, CreateTableEmptyColumnsThrows) {
    // The parser catches this, so we get a ParseError
    EXPECT_THROW(
        analyze_stmt("CREATE TABLE t ()"), DbError);
}
