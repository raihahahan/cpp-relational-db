#include <gtest/gtest.h>

#include "error/dberror.h"
#include "executor/executor.h"
#include "executor/test_db_helper.h"
#include "parser/analyzer.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "planner/logical/logical_planner.h"
#include "planner/physical/physical_planner.h"

using namespace db;
using common::Value;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<TestDB>("integration_test.db");

        std::vector<catalog::RawColumnInfo> schema = {
            {"id", catalog::INT_TYPE, 1},
            {"name", catalog::TEXT_TYPE, 2},
            {"age", catalog::INT_TYPE, 3},
        };
        db_->table_mgr->CreateTable("users", schema);
        auto users = db_->table_mgr->OpenTable("users");
        users->Insert(
            {Value{uint32_t(1)}, Value{std::string("Alice")}, Value{uint32_t(30)}});
        users->Insert(
            {Value{uint32_t(2)}, Value{std::string("Bob")}, Value{uint32_t(25)}});
        users->Insert(
            {Value{uint32_t(3)}, Value{std::string("Carol")}, Value{uint32_t(35)}});
        users->Insert(
            {Value{uint32_t(4)}, Value{std::string("Dave")}, Value{uint32_t(28)}});
        users->Insert(
            {Value{uint32_t(5)}, Value{std::string("Eve")}, Value{uint32_t(22)}});
    }

    std::vector<common::Tuple> run(const std::string &sql) {
        auto ast = parser::Parser::Parse(sql);
        parser::Analyzer analyzer{*db_->catalog};
        auto stmt = analyzer.Analyze(*ast);
        planner::PlanningContext ctx{db_->table_mgr.get()};

        planner::LogicalPlanPtr logical;
        if (stmt->type == parser::StmtType::Select)
            logical = planner::LogicalPlanner::Build(*stmt->select_query);
        else if (stmt->type == parser::StmtType::Insert)
            logical = planner::LogicalPlanner::Build(*stmt->insert_query);
        else if (stmt->type == parser::StmtType::Delete)
            logical = planner::LogicalPlanner::Build(*stmt->delete_query);

        auto physical = planner::PhysicalPlanner::Build(*logical, ctx);
        executor::Executor exec{std::move(physical)};
        return exec.ExecuteAndCollect();
    }

    std::unique_ptr<TestDB> db_;
};

// SELECT *
TEST_F(IntegrationTest, SelectStar) {
    auto rows = run("SELECT * FROM users");
    ASSERT_EQ(rows.size(), 5);
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[0]), 1);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[1]), "Alice");
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[2]), 30);
}

// SELECT specific columns
TEST_F(IntegrationTest, SelectColumns) {
    auto rows = run("SELECT name, age FROM users");
    ASSERT_EQ(rows.size(), 5);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Alice");
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[1]), 30);
}


// WHERE with integer comparison
TEST_F(IntegrationTest, WhereIntGt) {
    auto rows = run("SELECT name FROM users WHERE age > 28");
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Alice");
    EXPECT_EQ(std::get<std::string>(rows[1].GetValues()[0]), "Carol");
}

TEST_F(IntegrationTest, WhereIntEq) {
    auto rows = run("SELECT name FROM users WHERE id = 3");
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Carol");
}

TEST_F(IntegrationTest, WhereIntLt) {
    auto rows = run("SELECT id, name FROM users WHERE age < 26");
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[1]), "Bob");
    EXPECT_EQ(std::get<std::string>(rows[1].GetValues()[1]), "Eve");
}


// WHERE with text comparison
TEST_F(IntegrationTest, WhereTextEq) {
    auto rows = run("SELECT id FROM users WHERE name = 'Dave'");
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[0]), 4);
}


// WHERE with AND / OR
TEST_F(IntegrationTest, WhereAnd) {
    auto rows = run("SELECT name FROM users WHERE age > 24 AND age < 30");
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Bob");
    EXPECT_EQ(std::get<std::string>(rows[1].GetValues()[0]), "Dave");
}

TEST_F(IntegrationTest, WhereOr) {
    auto rows = run("SELECT name FROM users WHERE id = 1 OR id = 5");
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Alice");
    EXPECT_EQ(std::get<std::string>(rows[1].GetValues()[0]), "Eve");
}


// LIMIT
TEST_F(IntegrationTest, Limit) {
    auto rows = run("SELECT * FROM users LIMIT 2");
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[0]), 1);
    EXPECT_EQ(std::get<uint32_t>(rows[1].GetValues()[0]), 2);
}


// WHERE + LIMIT
TEST_F(IntegrationTest, WhereAndLimit) {
    auto rows = run("SELECT name FROM users WHERE age >= 25 LIMIT 3");
    ASSERT_EQ(rows.size(), 3);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Alice");
    EXPECT_EQ(std::get<std::string>(rows[1].GetValues()[0]), "Bob");
    EXPECT_EQ(std::get<std::string>(rows[2].GetValues()[0]), "Carol");
}


// NOT
TEST_F(IntegrationTest, WhereNot) {
    auto rows = run("SELECT name FROM users WHERE NOT id = 1");
    ASSERT_EQ(rows.size(), 4);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Bob");
}


// Complex query
TEST_F(IntegrationTest, ComplexQuery) {
    auto rows =
        run("SELECT id, name FROM users WHERE (age >= 25 AND age <= 30) OR "
            "name = 'Eve' LIMIT 4");
    ASSERT_EQ(rows.size(), 4);
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[0]), 1);
    EXPECT_EQ(std::get<uint32_t>(rows[1].GetValues()[0]), 2);
    EXPECT_EQ(std::get<uint32_t>(rows[2].GetValues()[0]), 4);
    EXPECT_EQ(std::get<uint32_t>(rows[3].GetValues()[0]), 5);
}


// Error handling: undefined table
TEST_F(IntegrationTest, UndefinedTable) {
    EXPECT_THROW(run("SELECT * FROM nonexistent"), DbError);
}


// Error handling: undefined column
TEST_F(IntegrationTest, UndefinedColumn) {
    EXPECT_THROW(run("SELECT missing FROM users"), DbError);
}


// DELETE integration
TEST_F(IntegrationTest, DeleteWithWhere) {
    auto result = run("DELETE FROM users WHERE id = 3");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(result[0].GetValues()[0]), 1);

    auto remaining = run("SELECT * FROM users");
    EXPECT_EQ(remaining.size(), 4);

    for (const auto& row : remaining) {
        EXPECT_NE(std::get<uint32_t>(row.GetValues()[0]), uint32_t(3));
    }
}

TEST_F(IntegrationTest, DeleteAllRows) {
    auto result = run("DELETE FROM users");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(result[0].GetValues()[0]), 5);

    auto remaining = run("SELECT * FROM users");
    EXPECT_EQ(remaining.size(), 0);
}

TEST_F(IntegrationTest, DeleteNoMatch) {
    auto result = run("DELETE FROM users WHERE id = 999");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(result[0].GetValues()[0]), 0);

    auto remaining = run("SELECT * FROM users");
    EXPECT_EQ(remaining.size(), 5);
}

TEST_F(IntegrationTest, DeleteUndefinedTableThrows) {
    EXPECT_THROW(run("DELETE FROM ghosts"), DbError);
}


// INSERT integration
TEST_F(IntegrationTest, InsertSingleRow) {
    auto result = run(
        "INSERT INTO users (id, name, age) VALUES (6, 'Frank', 40)");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(result[0].GetValues()[0]), 1);

    auto rows = run("SELECT * FROM users WHERE id = 6");
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[0]), 6);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[1]), "Frank");
    EXPECT_EQ(std::get<uint32_t>(rows[0].GetValues()[2]), 40);
}

TEST_F(IntegrationTest, InsertMultiRow) {
    auto result = run(
        "INSERT INTO users VALUES (6, 'Frank', 40), (7, 'Grace', 33)");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(result[0].GetValues()[0]), 2);

    auto rows = run("SELECT * FROM users");
    EXPECT_EQ(rows.size(), 7);
}

TEST_F(IntegrationTest, InsertWithoutColumnList) {
    auto result = run("INSERT INTO users VALUES (10, 'Zoe', 50)");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<uint32_t>(result[0].GetValues()[0]), 1);

    auto rows = run("SELECT name FROM users WHERE id = 10");
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(std::get<std::string>(rows[0].GetValues()[0]), "Zoe");
}

TEST_F(IntegrationTest, InsertTypeMismatchThrows) {
    EXPECT_THROW(
        run("INSERT INTO users (id, name, age) VALUES ('bad', 'X', 1)"),
        DbError);
}

TEST_F(IntegrationTest, InsertUndefinedTableThrows) {
    EXPECT_THROW(
        run("INSERT INTO ghosts VALUES (1)"), DbError);
}

TEST_F(IntegrationTest, InsertThenDeleteRoundTrip) {
    run("INSERT INTO users (id, name, age) VALUES (99, 'Temp', 18)");
    auto before = run("SELECT * FROM users WHERE id = 99");
    ASSERT_EQ(before.size(), 1);

    run("DELETE FROM users WHERE id = 99");
    auto after = run("SELECT * FROM users WHERE id = 99");
    EXPECT_EQ(after.size(), 0);
}