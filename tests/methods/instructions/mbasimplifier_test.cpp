#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

#include "../../../binaryninjaapi/binaryninjaapi.h"
#include "../../../binaryninjaapi/highlevelilinstruction.h"
#include "../../../include/methods/instructions/mbasimplifier.hpp"

using namespace BinaryNinja;
using namespace Instructions;

class MBASimplifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test CSV file
        std::filesystem::create_directories("test_resources");
        std::ofstream file("test_resources/mba-dataset-test.csv");
        file << "Original,Obfuscated\n";
        file << "a+b,a*b\n";
        file << "x^y,x&y\n";
        file << "(a+b)*c,(a^b)+c\n";
        file.close();
    }

    void TearDown() override {
        std::filesystem::remove_all("test_resources");
    }
};

TEST_F(MBASimplifierTest, LoadPatternsFromCSVWorks) {
    MBASimplifier simplifier;
    simplifier.getPatterns().clear();

    ASSERT_TRUE(simplifier.loadPatternsFromCSV("test_resources/mba-dataset-test.csv"));
    
    const auto& patterns = simplifier.getPatterns();
    ASSERT_EQ(patterns.size(), 3);
    
    EXPECT_EQ(patterns[0].original, "a+b");
    EXPECT_EQ(patterns[0].obfuscated, "a*b");
    EXPECT_EQ(patterns[1].original, "x^y");
    EXPECT_EQ(patterns[1].obfuscated, "x&y");
    EXPECT_EQ(patterns[2].original, "(a+b)*c");
    EXPECT_EQ(patterns[2].obfuscated, "(a^b)+c");
}

TEST_F(MBASimplifierTest, LoadPatternsFromDirectoryWorks) {
    MBASimplifier simplifier;
    ASSERT_TRUE(simplifier.loadPatternsFromDirectory("test_resources"));
    EXPECT_GT(simplifier.getPatternCount(), 0);
}

TEST_F(MBASimplifierTest, TokenizeSimpleExpression) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("a+b");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::OPERAND);
    EXPECT_EQ(tokens[0].value, "a");
    EXPECT_EQ(tokens[0].hlilOpcode, HLIL_VAR);
    
    EXPECT_EQ(tokens[1].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[1].value, "+");
    EXPECT_EQ(tokens[1].hlilOpcode, HLIL_ADD);
    
    EXPECT_EQ(tokens[2].type, TokenType::OPERAND);
    EXPECT_EQ(tokens[2].value, "b");
    EXPECT_EQ(tokens[2].hlilOpcode, HLIL_VAR);
}

TEST_F(MBASimplifierTest, TokenizeComplexExpression) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("(a*b)+c");

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[1].type, TokenType::OPERAND);
    EXPECT_EQ(tokens[1].value, "a");
    EXPECT_EQ(tokens[2].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[2].value, "*");
    EXPECT_EQ(tokens[3].type, TokenType::OPERAND);
    EXPECT_EQ(tokens[3].value, "b");
    EXPECT_EQ(tokens[4].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[5].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[5].value, "+");
    EXPECT_EQ(tokens[6].type, TokenType::OPERAND);
    EXPECT_EQ(tokens[6].value, "c");
}

TEST_F(MBASimplifierTest, TokenizeMultiCharacterOperators) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("a<<b>>c");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].value, "a");
    EXPECT_EQ(tokens[1].value, "<<");
    EXPECT_EQ(tokens[1].hlilOpcode, HLIL_LSL);
    EXPECT_EQ(tokens[2].value, "b");
    EXPECT_EQ(tokens[3].value, ">>");
    EXPECT_EQ(tokens[3].hlilOpcode, HLIL_LSR);
    EXPECT_EQ(tokens[4].value, "c");
}

TEST_F(MBASimplifierTest, TokenizeConstants) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("123+456");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].value, "123");
    EXPECT_EQ(tokens[0].hlilOpcode, HLIL_CONST);
    EXPECT_EQ(tokens[1].value, "+");
    EXPECT_EQ(tokens[2].value, "456");
    EXPECT_EQ(tokens[2].hlilOpcode, HLIL_CONST);
}

TEST_F(MBASimplifierTest, ParseSimpleExpression) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("a+b");
    auto tree = simplifier.parseExpression(tokens);

    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->token.value, "+");
    EXPECT_EQ(tree->children.size(), 2);
    EXPECT_EQ(tree->children[0]->token.value, "a");
    EXPECT_EQ(tree->children[1]->token.value, "b");
}

TEST_F(MBASimplifierTest, ParseComplexExpression) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("(a*b)+c");
    auto tree = simplifier.parseExpression(tokens);

    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->token.value, "+");
    EXPECT_EQ(tree->children.size(), 2);
    
    // Left child should be the multiplication
    ASSERT_NE(tree->children[0], nullptr);
    EXPECT_EQ(tree->children[0]->token.value, "*");
    EXPECT_EQ(tree->children[0]->children.size(), 2);
    EXPECT_EQ(tree->children[0]->children[0]->token.value, "a");
    EXPECT_EQ(tree->children[0]->children[1]->token.value, "b");
    
    // Right child should be 'c'
    EXPECT_EQ(tree->children[1]->token.value, "c");
}

TEST_F(MBASimplifierTest, ParsePrecedence) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("a+b*c");
    auto tree = simplifier.parseExpression(tokens);

    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->token.value, "+");
    EXPECT_EQ(tree->children.size(), 2);
    
    // Left child should be 'a'
    EXPECT_EQ(tree->children[0]->token.value, "a");
    
    // Right child should be multiplication (b*c)
    ASSERT_NE(tree->children[1], nullptr);
    EXPECT_EQ(tree->children[1]->token.value, "*");
    EXPECT_EQ(tree->children[1]->children.size(), 2);
    EXPECT_EQ(tree->children[1]->children[0]->token.value, "b");
    EXPECT_EQ(tree->children[1]->children[1]->token.value, "c");
}

TEST_F(MBASimplifierTest, ParseUnaryOperator) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("~a");
    auto tree = simplifier.parseExpression(tokens);

    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->token.value, "~");
    EXPECT_EQ(tree->children.size(), 1);
    EXPECT_EQ(tree->children[0]->token.value, "a");
}

TEST_F(MBASimplifierTest, HandleEmptyExpression) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("");
    auto tree = simplifier.parseExpression(tokens);

    EXPECT_EQ(tree, nullptr);
}

TEST_F(MBASimplifierTest, HandleWhitespaceExpression) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("  a  +  b  ");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].value, "a");
    EXPECT_EQ(tokens[1].value, "+");
    EXPECT_EQ(tokens[2].value, "b");
}

TEST_F(MBASimplifierTest, HandleMultipleParenthesisExpression) {
    MBASimplifier simplifier;
    auto tokens = simplifier.tokenize("((x ^ 1) + (x & 1))");

    ASSERT_FALSE(tokens.empty());
    auto tree = simplifier.parseExpression(tokens);
    ASSERT_NE(tree, nullptr);

    EXPECT_EQ(tree->token.value, "+");
    ASSERT_EQ(tree->children.size(), 2);

    auto* left = tree->children[0].get();
    ASSERT_EQ(left->token.value, "^");
    ASSERT_EQ(left->children.size(), 2);
    EXPECT_EQ(left->children[0]->token.value, "x");
    EXPECT_EQ(left->children[1]->token.value, "1");

    auto* right = tree->children[1].get();
    ASSERT_EQ(right->token.value, "&");
    ASSERT_EQ(right->children.size(), 2);
    EXPECT_EQ(right->children[0]->token.value, "x");
    EXPECT_EQ(right->children[1]->token.hlilOpcode, HLIL_CONST);
    EXPECT_EQ(right->children[1]->token.value, "1");
}

// TEST(MBASimplifierTest, LoadManualPatternAndMatch)
// {
//     Ref<Architecture> arch = Architecture::GetByName("x86");
//     Ref<Platform> platform = arch->GetStandalonePlatform();

//     HighLevelILFunction* llil = new HighLevelILFunction(arch);

//     ExprId var0 = llil->Register(4, 0); // a
//     ExprId var1 = llil->Register(4, 1); // b
//     ExprId add = llil->AddExpr(HLIL_ADD, 4, var0, var1); // a + b

//     // eax = a + b
//     llil->AddExpr(HLIL_SET_VAR, 4, llil->GetRegisters()["eax"], add);
//     llil->Finalize();

//     BNCreateHighLevelILFunction(arch, new BNFunction(arch, ))
//     Ref<Function> func = new Function(arch, platform, 0, nullptr);
//     BNSetHighLevelILFunction(nullptr, *llil);

//     MBASimplifier simplifier;
//     simplifier.getPatterns() = {
//         MBAPattern{"a+b", "a*b"}  // volontairement faux pour test
//     };

//     auto matches = simplifier.findMatches(func);
//     EXPECT_EQ(matches.size(), 1);
// }