#include <gtest/gtest.h>

#include <string>
#include <vector>
#include "parser/lexer.h"
#include "error/dberror.h"

using db::parser::Lexer;
using db::parser::Token;
using db::parser::TokenType;

static std::vector<Token> lex(std::string_view input) {
    Lexer lexer(input);
    return lexer.LexicalParse();
}

#define EXPECT_TOKEN(tok, t, lexeme_)   \
    EXPECT_EQ((tok).type, (t));         \
    EXPECT_EQ((tok).lexeme, (lexeme_))


TEST(LexerBasic, EmptyInput) {
    auto tokens = lex("");

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
}

TEST(LexerBasic, WhitespaceOnly) {
    auto tokens = lex("   \n\t  ");

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
}

TEST(LexerIdentifiers, SimpleIdentifier) {
    auto tokens = lex("students");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::Identifier, "students");
}

TEST(LexerIdentifiers, UnderscoreAndDigits) {
    auto tokens = lex("student_123");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::Identifier, "student_123");
}

TEST(LexerKeywords, Lowercase) {
    auto tokens = lex("select");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::Keyword, "select");
}

TEST(LexerKeywords, Uppercase) {
    auto tokens = lex("SELECT");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::Keyword, "SELECT");
}

TEST(LexerKeywords, MixedCase) {
    auto tokens = lex("SeLeCt");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::Keyword, "SeLeCt");
}

TEST(LexerNumbers, SimpleNumber) {
    auto tokens = lex("12345");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::Number, "12345");
}

TEST(LexerNumbers, NumberStopsAtNonDigit) {
    auto tokens = lex("123abc");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_TOKEN(tokens[0], TokenType::Number, "123");
    EXPECT_TOKEN(tokens[1], TokenType::Identifier, "abc");
}

TEST(LexerOperators, SingleCharOperators) {
    auto tokens = lex("+-*/");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_TOKEN(tokens[0], TokenType::Operator, "+");
    EXPECT_TOKEN(tokens[1], TokenType::Operator, "-");
    EXPECT_TOKEN(tokens[2], TokenType::Operator, "*");
    EXPECT_TOKEN(tokens[3], TokenType::Operator, "/");
}

TEST(LexerOperators, TwoCharOperators) {
    auto tokens = lex("<= >= != <>");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_TOKEN(tokens[0], TokenType::Operator, "<=");
    EXPECT_TOKEN(tokens[1], TokenType::Operator, ">=");
    EXPECT_TOKEN(tokens[2], TokenType::Operator, "!=");
    EXPECT_TOKEN(tokens[3], TokenType::Operator, "<>");
}

TEST(LexerOperators, InvalidSingleOperator) {
    EXPECT_THROW(lex("!"), DbError);
}

TEST(LexerOperators, InvalidOperatorsSplit) {
    auto tokens = lex(">>");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_TOKEN(tokens[0], TokenType::Operator, ">");
    EXPECT_TOKEN(tokens[1], TokenType::Operator, ">");
}

TEST(LexerPunctuation, BasicPunctuation) {
    auto tokens = lex(",();");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_TOKEN(tokens[0], TokenType::Comma, ",");
    EXPECT_TOKEN(tokens[1], TokenType::LParen, "(");
    EXPECT_TOKEN(tokens[2], TokenType::RParen, ")");
    EXPECT_TOKEN(tokens[3], TokenType::Semicolon, ";");
}

TEST(LexerPosition, CorrectPositions) {
    auto tokens = lex("select x");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].pos, 0); // select
    EXPECT_EQ(tokens[1].pos, 7); // x
}

TEST(LexerIntegration, SimpleSelect) {
    auto tokens = lex("SELECT id FROM students WHERE id = 2;");

    std::vector<TokenType> expected = {
        TokenType::Keyword,
        TokenType::Identifier,
        TokenType::Keyword,
        TokenType::Identifier,
        TokenType::Keyword,
        TokenType::Identifier,
        TokenType::Operator,
        TokenType::Number,
        TokenType::Semicolon,
        TokenType::EndOfFile
    };

    ASSERT_EQ(tokens.size(), expected.size());

    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
}


TEST(LexerErrors, InvalidCharacterThrows) {
    EXPECT_THROW(
        lex("select @ from table"),
        DbError
    );
}

TEST(LexerStringLiteral, SimpleString) {
    auto tokens = lex("'hello'");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::StringLiteral, "'hello'");
}

TEST(LexerStringLiteral, EmptyString) {
    auto tokens = lex("''");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::StringLiteral, "''");
}

TEST(LexerStringLiteral, EscapedQuote) {
    auto tokens = lex("'it''s'");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::StringLiteral, "'it''s'");
}

TEST(LexerStringLiteral, StringInExpression) {
    auto tokens = lex("name = 'alice'");

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_TOKEN(tokens[0], TokenType::Identifier, "name");
    EXPECT_TOKEN(tokens[1], TokenType::Operator, "=");
    EXPECT_TOKEN(tokens[2], TokenType::StringLiteral, "'alice'");
}

TEST(LexerStringLiteral, UnterminatedThrows) {
    EXPECT_THROW(lex("'unterminated"), DbError);
}

TEST(LexerDot, SimpleDot) {
    auto tokens = lex("a.b");

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_TOKEN(tokens[0], TokenType::Identifier, "a");
    EXPECT_TOKEN(tokens[1], TokenType::Dot, ".");
    EXPECT_TOKEN(tokens[2], TokenType::Identifier, "b");
}

TEST(LexerDot, QualifiedColumnInSelect) {
    auto tokens = lex("SELECT t.id FROM t");

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_TOKEN(tokens[0], TokenType::Keyword, "SELECT");
    EXPECT_TOKEN(tokens[1], TokenType::Identifier, "t");
    EXPECT_TOKEN(tokens[2], TokenType::Dot, ".");
    EXPECT_TOKEN(tokens[3], TokenType::Identifier, "id");
    EXPECT_TOKEN(tokens[4], TokenType::Keyword, "FROM");
    EXPECT_TOKEN(tokens[5], TokenType::Identifier, "t");
}

TEST(LexerKeywords, AllNewKeywords) {
    std::vector<std::string> kws = {
        "and", "or", "not", "as", "is", "null",
        "create", "table", "insert", "into", "values",
        "update", "set", "delete",
        "order", "by", "asc", "desc", "group", "having", "distinct",
        "join", "left", "right", "inner", "outer", "on", "cross",
        "in", "between", "like", "exists", "true", "false",
        "primary", "key"
    };

    for (const auto& kw : kws) {
        auto tokens = lex(kw);
        ASSERT_GE(tokens.size(), 2u) << "keyword: " << kw;
        EXPECT_EQ(tokens[0].type, TokenType::Keyword) << "keyword: " << kw;
    }
}

TEST(LexerKeywords, NonKeywordIdentifier) {
    auto tokens = lex("foobar");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_TOKEN(tokens[0], TokenType::Identifier, "foobar");
}

TEST(LexerIntegration, SelectWithStringAndDot) {
    auto tokens = lex("SELECT t.name FROM t WHERE t.val = 'hello'");

    std::vector<TokenType> expected = {
        TokenType::Keyword,       // SELECT
        TokenType::Identifier,    // t
        TokenType::Dot,           // .
        TokenType::Identifier,    // name
        TokenType::Keyword,       // FROM
        TokenType::Identifier,    // t
        TokenType::Keyword,       // WHERE
        TokenType::Identifier,    // t
        TokenType::Dot,           // .
        TokenType::Identifier,    // val
        TokenType::Operator,      // =
        TokenType::StringLiteral, // 'hello'
        TokenType::EndOfFile
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]) << "token index " << i;
    }
}

TEST(LexerIntegration, BooleanKeywordsInWhere) {
    auto tokens = lex("WHERE a = 1 AND b > 2 OR NOT c < 3");

    EXPECT_TOKEN(tokens[0], TokenType::Keyword, "WHERE");
    EXPECT_TOKEN(tokens[4], TokenType::Keyword, "AND");
    EXPECT_TOKEN(tokens[8], TokenType::Keyword, "OR");
    EXPECT_TOKEN(tokens[9], TokenType::Keyword, "NOT");
}

// stress test
TEST(LexerStress, ManyIdentifiers) {
    std::string input;
    for (int i = 0; i < 1000; ++i) {
        input += "col" + std::to_string(i) + " ";
    }

    auto tokens = lex(input);
    EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}
