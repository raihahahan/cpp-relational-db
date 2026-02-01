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

TEST(LexerStress, ManyIdentifiers) {
    std::string input;
    for (int i = 0; i < 1000; ++i) {
        input += "col" + std::to_string(i) + " ";
    }

    auto tokens = lex(input);
    EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}
