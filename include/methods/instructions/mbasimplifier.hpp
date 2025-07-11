#ifndef METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP
#define METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP

#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include "../../deobfuscator.hpp"

// Forward declarations
namespace BinaryNinja {
    class Function;
    class HighLevelILFunction;
    class HighLevelILInstruction;
    class AnalysisContext;
    template<typename T> class Ref;
}

namespace Instructions {
    // Token types for the expression parser
    enum class TokenType {
        OPERATOR,
        OPERAND,
        LPAREN,
        RPAREN,
        UNKNOWN
    };

    // Represents a parsed token
    struct Token {
        TokenType type;
        std::string value;
        int hlilOpcode;
        
        Token(TokenType t, std::string v, int op = -1) 
            : type(t), value(std::move(v)), hlilOpcode(op) {}
    };

    // Expression tree node
    struct ExprNode {
        Token token;
        std::vector<std::unique_ptr<ExprNode>> children;
        
        explicit ExprNode(Token t) : token(std::move(t)) {}
    };

    // Pattern matching structure
    struct MBAPattern {
        std::string original;
        std::string obfuscated;
        
        MBAPattern(std::string orig, std::string obf) 
            : original(std::move(orig)), obfuscated(std::move(obf)) {}
    };

    class MBASimplifier : public IDeobfuscationMethod
    {
    public:
        MBASimplifier();

        // Pattern loading and management
        bool loadPatternsFromCSV(const std::string& csvFilePath);
        bool loadPatternsFromDirectory(const std::string& directory);

        size_t getPatternCount() const { return patterns.size(); }
        std::vector<MBAPattern>& getPatterns() { return patterns; }

        size_t getMatchesCount() const { return matches.size(); }
        std::vector<std::tuple<std::string, size_t, const BinaryNinja::HighLevelILInstruction*>>& getMatches() { return matches; }

        // Tokenization and parsing
        std::vector<Token> tokenize(const std::string& expr) const;
        std::unique_ptr<ExprNode> parseExpression(const std::vector<Token>& tokens) const;
        std::string expressionTreeToString(const ExprNode* node) const;

        // Pattern matching and simplification
        std::vector<std::tuple<
            std::string, size_t,
            BinaryNinja::HighLevelILInstruction
        >> findMatches(const BinaryNinja::Ref<BinaryNinja::Function>& func);

        size_t replaceObfuscatedWithSimple(
            const BinaryNinja::Ref<BinaryNinja::HighLevelILFunction>& hlil,
            const BinaryNinja::HighLevelILInstruction& srcExpr,
            const BinaryNinja::HighLevelILInstruction& leftExpr,
            const BinaryNinja::HighLevelILInstruction& rightExpr);

        // Main execution
        void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext) override;

    private:
        std::vector<MBAPattern> patterns;
        std::vector<std::tuple<std::string, size_t, const BinaryNinja::HighLevelILInstruction*>> matches;
        
        // Symbol mapping
        static const std::unordered_map<std::string, int> symbolToHLIL;
        static const std::unordered_map<int, std::string> hlilToSymbol;

        std::string OperationToString(BNHighLevelILOperation op) const;
        static const std::unordered_map<char, int> operatorPrecedence;
        
        // Helper methods
        int mapSymbolToHLIL(const std::string& symbol) const;
        int getOperatorPrecedence(const std::string& op) const;
        bool isOperator(const std::string& token) const;
        bool isOperand(const std::string& token) const;
        TokenType classifyToken(const std::string& token) const;
        
        // Expression parsing helpers
        std::unique_ptr<Instructions::ExprNode> parseExpressionRecursive(
            const std::vector<Instructions::Token>& tokens, 
            size_t& pos, 
            int minPrecedence = 0) const;
        
        std::unique_ptr<Instructions::ExprNode> parsePrimary(
            const std::vector<Instructions::Token>& tokens, 
            size_t& pos) const;
        
        // Pattern matching
        bool matchPattern(const Instructions::ExprNode* patternNode, const BinaryNinja::HighLevelILInstruction& ilNode) const;
        std::unique_ptr<Instructions::ExprNode> extractHLILSubtree(const BinaryNinja::HighLevelILInstruction& instr) const;
        
        // Utility methods
        std::string trim(const std::string& str) const;
        std::vector<std::string> split(const std::string& str, char delimiter) const;
        bool isValidCSVLine(const std::string& line) const;
    };
}

#endif // METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP