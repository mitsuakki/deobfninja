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
    class LowLevelILFunction;
    class LowLevelILInstruction;
    class AnalysisContext;
    template<typename T> class Ref;
}

namespace Instructions {

    /**
     * @brief Represents the type of a token in an expression.
     */
    enum class TokenType {
        OPERATOR,  ///< Operator like +, -, ^, etc.
        OPERAND,   ///< Operand like variable or constant.
        LPAREN,    ///< Left parenthesis '('
        RPAREN,    ///< Right parenthesis ')'
        UNKNOWN    ///< Unknown or invalid token
    };

    /**
     * @brief Represents a parsed token from a pattern expression.
     */
    struct Token {
        TokenType type;
        std::string value;
        int llilOpcode;

        Token(TokenType t, std::string v, int op = -1) 
            : type(t), value(std::move(v)), llilOpcode(op) {}
    };

    /**
     * @brief Node in an expression tree for MBA pattern matching.
     */
    struct ExprNode {
        Token token;
        std::vector<std::unique_ptr<ExprNode>> children;

        explicit ExprNode(Token t) : token(std::move(t)) {}
    };

    /**
     * @brief Represents a mixed boolean-arithmetic (MBA) pattern.
     */
    struct MBAPattern {
        std::string original;    ///< Simplified/original version of the expression.
        std::string obfuscated;  ///< Obfuscated form to match.

        MBAPattern(std::string orig, std::string obf) 
            : original(std::move(orig)), obfuscated(std::move(obf)) {}
    };

    /**
     * @class MBASimplifier
     * @brief Deobfuscation method to detect and simplify MBA expressions in IL.
     */
    class MBASimplifier : public IDeobfuscationMethod {
    public:
        MBASimplifier();

        // === Pattern loading ===

        /**
         * @brief Load patterns from a CSV file.
         * @param csvFilePath Path to the CSV file.
         * @return true if patterns were successfully loaded.
         */
        bool loadPatternsFromCSV(const std::string& csvFilePath);

        /**
         * @brief Load all patterns from a directory of CSV files.
         * @param directory Directory path.
         * @return true if at least one file was loaded successfully.
         */
        bool loadPatternsFromDirectory(const std::string& directory);

        size_t getPatternCount() const { return patterns.size(); }
        std::vector<MBAPattern>& getPatterns() { return patterns; }

        size_t getMatchesCount() const { return matches.size(); }
        std::vector<std::tuple<std::string, size_t, const BinaryNinja::LowLevelILInstruction*>>& getMatches() { return matches; }

        // === Tokenization and Parsing ===

        /**
         * @brief Tokenize an expression string into tokens.
         */
        std::vector<Token> tokenize(const std::string& expr) const;

        /**
         * @brief Parse a tokenized expression into an expression tree.
         */
        std::unique_ptr<ExprNode> parseExpression(const std::vector<Token>& tokens) const;

        /**
         * @brief Convert an expression tree back into string form.
         */
        std::string expressionTreeToString(const ExprNode* node) const;

        // === Matching & Replacement ===

        /**
         * @brief Find obfuscated patterns in a given function.
         * @param func Function to analyze.
         * @return A vector of matched patterns.
         */
        std::vector<std::tuple<std::string, size_t, BinaryNinja::LowLevelILInstruction>> findMatches(
            const BinaryNinja::Ref<BinaryNinja::Function>& func);

        /**
         * @brief Replace an obfuscated expression with its simplified form.
         */
        size_t replaceObfuscatedWithSimple(
            const BinaryNinja::Ref<BinaryNinja::LowLevelILFunction>& llil,
            const BinaryNinja::LowLevelILInstruction& srcExpr,
            const BinaryNinja::LowLevelILInstruction& leftExpr,
            const BinaryNinja::LowLevelILInstruction& rightExpr);

        /**
         * @brief Executes the MBA simplifier.
         */
        void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext) override;

    private:
        std::vector<MBAPattern> patterns;
        std::vector<std::tuple<std::string, size_t, const BinaryNinja::LowLevelILInstruction*>> matches;

        // === Symbol mappings ===
        static const std::unordered_map<std::string, int> symbolToLLIL;
        static const std::unordered_map<int, std::string> llilToSymbol;

        static const std::unordered_map<char, int> operatorPrecedence;

        std::string OperationToString(BNLowLevelILOperation op) const;
        int mapSymbolToLLIL(const std::string& symbol) const;
        int getOperatorPrecedence(const std::string& op) const;
        bool isOperator(const std::string& token) const;
        bool isOperand(const std::string& token) const;
        TokenType classifyToken(const std::string& token) const;

        // === Expression parsing ===
        std::unique_ptr<ExprNode> parseExpressionRecursive(
            const std::vector<Token>& tokens, 
            size_t& pos, 
            int minPrecedence = 0) const;

        std::unique_ptr<ExprNode> parsePrimary(
            const std::vector<Token>& tokens, 
            size_t& pos) const;

        // === Pattern matching ===
        bool matchPattern(const ExprNode* patternNode, const BinaryNinja::LowLevelILInstruction& ilNode) const;
        std::unique_ptr<ExprNode> extractLLILSubtree(const BinaryNinja::LowLevelILInstruction& instr) const;

        // === Utilities ===
        std::string trim(const std::string& str) const;
        std::vector<std::string> split(const std::string& str, char delimiter) const;
        bool isValidCSVLine(const std::string& line) const;

        // === Debug/Logging ===
        void printExpressionTree(const ExprNode* node, int depth = 0) const;
        void logPatternMatch(const std::string& pattern, size_t instrIndex) const;
    };
}

#endif // METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP
