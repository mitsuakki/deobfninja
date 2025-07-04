#include "../../../include/methods/instructions/mbasimplifier.hpp"
#include "../../../binaryninjaapi/lowlevelilinstruction.h"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cctype>

using namespace BinaryNinja;

// Static member definitions
const std::unordered_map<std::string, int> MBASimplifier::symbolToLLIL = {
    {"+", LLIL_ADD}, {"-", LLIL_SUB}, {"*", LLIL_MUL}, {"^", LLIL_XOR},
    {"&", LLIL_AND}, {"|", LLIL_OR}, {"~", LLIL_NOT}, {"/", LLIL_DIVU},
    {"%", LLIL_MODU}, {"<<", LLIL_LSL}, {">>", LLIL_LSR}, {"<<<", LLIL_LSL},
    {">>>", LLIL_LSR}, {"==", LLIL_CMP_E}, {"!=", LLIL_CMP_NE},
    {"<", LLIL_CMP_ULT}, {"<=", LLIL_CMP_ULE}, {">", LLIL_CMP_UGT},
    {">=", LLIL_CMP_UGE}
};

const std::unordered_map<char, int> MBASimplifier::operatorPrecedence = {
    {'(', 0}, {')', 0},
    {'|', 1}, {'^', 2}, {'&', 3},
    {'=', 4}, {'!', 4}, {'<', 4}, {'>', 4},
    {'+', 5}, {'-', 5},
    {'*', 6}, {'/', 6}, {'%', 6},
    {'~', 7}
};

MBASimplifier::MBASimplifier() 
    : IDeobfuscationMethod("MBA Simplifier", DeobfuscationCategory::Workflow) {
    this->isEnabled = true;
    loadPatternsFromDirectory("resources/");
}

bool MBASimplifier::loadPatternsFromDirectory(const std::string& directory) {
    bool loaded = false;
    
    if (!std::filesystem::exists(directory)) {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find("mba-dataset") != std::string::npos && 
                entry.path().extension() == ".csv") {
                if (loadPatternsFromCSV(entry.path().string())) {
                    loaded = true;
                }
            }
        }
    }
    
    if (loaded) {
        compilePatternsFromStrings();
    }
    
    return loaded;
}

bool MBASimplifier::loadPatternsFromCSV(const std::string& csvFilePath) {
    std::ifstream file(csvFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open CSV file: " << csvFilePath << std::endl;
        return false;
    }

    std::string line;
    size_t lineNumber = 0;
    size_t loadedPatterns = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        if (!isValidCSVLine(line)) {
            continue;
        }

        std::stringstream ss(line);
        std::string original, simplified;

        if (std::getline(ss, original, ',') && std::getline(ss, simplified)) {
            original = trim(original);
            simplified = trim(simplified);
            
            if (!original.empty() && !simplified.empty()) {
                patterns.emplace_back(original, simplified);
                loadedPatterns++;
            }
        } else {
            std::cerr << "Invalid CSV format at line " << lineNumber 
                      << " in file: " << csvFilePath << std::endl;
        }
    }

    std::cout << "Loaded " << loadedPatterns << " patterns from " << csvFilePath << std::endl;
    return loadedPatterns > 0;
}

std::vector<Token> MBASimplifier::tokenize(const std::string& expr) const {
    std::vector<Token> tokens;
    std::string cleanExpr = trim(expr);
    
    if (cleanExpr.empty()) {
        return tokens;
    }

    size_t i = 0;
    while (i < cleanExpr.size()) {
        // Skip whitespace
        if (std::isspace(cleanExpr[i])) {
            i++;
            continue;
        }

        // Handle parentheses
        if (cleanExpr[i] == '(') {
            tokens.emplace_back(TokenType::LPAREN, "(");
            i++;
            continue;
        }
        
        if (cleanExpr[i] == ')') {
            tokens.emplace_back(TokenType::RPAREN, ")");
            i++;
            continue;
        }

        // Handle multi-character operators
        if (i + 1 < cleanExpr.size()) {
            std::string twoChar = cleanExpr.substr(i, 2);
            if (symbolToLLIL.find(twoChar) != symbolToLLIL.end()) {
                int llilOp = mapSymbolToLLIL(twoChar);
                tokens.emplace_back(TokenType::OPERATOR, twoChar, llilOp);
                i += 2;
                continue;
            }
        }

        // Handle three-character operators
        if (i + 2 < cleanExpr.size()) {
            std::string threeChar = cleanExpr.substr(i, 3);
            if (symbolToLLIL.find(threeChar) != symbolToLLIL.end()) {
                int llilOp = mapSymbolToLLIL(threeChar);
                tokens.emplace_back(TokenType::OPERATOR, threeChar, llilOp);
                i += 3;
                continue;
            }
        }

        // Handle single character operators
        std::string singleChar(1, cleanExpr[i]);
        if (symbolToLLIL.find(singleChar) != symbolToLLIL.end()) {
            int llilOp = mapSymbolToLLIL(singleChar);
            tokens.emplace_back(TokenType::OPERATOR, singleChar, llilOp);
            i++;
            continue;
        }

        // Handle operands (variables and constants)
        if (std::isalnum(cleanExpr[i]) || cleanExpr[i] == '_') {
            size_t start = i;
            while (i < cleanExpr.size() && 
                   (std::isalnum(cleanExpr[i]) || cleanExpr[i] == '_')) {
                i++;
            }
            
            std::string operand = cleanExpr.substr(start, i - start);
            TokenType type = classifyToken(operand);
            int llilOp = mapSymbolToLLIL(operand);
            tokens.emplace_back(type, operand, llilOp);
            continue;
        }

        // Unknown character - skip but log
        std::cerr << "Unknown character '" << cleanExpr[i] << "' at position " << i << std::endl;
        i++;
    }

    return tokens;
}

std::unique_ptr<ExprNode> MBASimplifier::parseExpression(const std::vector<Token>& tokens) const {
    if (tokens.empty()) {
        return nullptr;
    }

    size_t pos = 0;
    auto result = parseExpressionRecursive(tokens, pos, 0);
    
    if (pos < tokens.size()) {
        std::cerr << "Warning: Not all tokens consumed during parsing. Position: " 
                  << pos << "/" << tokens.size() << std::endl;
    }
    
    return result;
}

std::unique_ptr<ExprNode> MBASimplifier::parseExpressionRecursive(
    const std::vector<Token>& tokens, 
    size_t& pos, 
    int minPrecedence) const {
    
    auto left = parsePrimary(tokens, pos);
    if (!left) {
        return nullptr;
    }

    while (pos < tokens.size()) {
        const Token& token = tokens[pos];
        
        if (token.type != TokenType::OPERATOR) {
            break;
        }

        int precedence = getOperatorPrecedence(token.value);
        if (precedence < minPrecedence) {
            break;
        }

        pos++; // consume operator
        auto right = parseExpressionRecursive(tokens, pos, precedence + 1);
        if (!right) {
            std::cerr << "Expected right operand after operator: " << token.value << std::endl;
            return nullptr;
        }

        auto opNode = std::make_unique<ExprNode>(token);
        opNode->children.push_back(std::move(left));
        opNode->children.push_back(std::move(right));
        left = std::move(opNode);
    }

    return left;
}

std::unique_ptr<ExprNode> MBASimplifier::parsePrimary(
    const std::vector<Token>& tokens, 
    size_t& pos) const {
    
    if (pos >= tokens.size()) {
        return nullptr;
    }

    const Token& token = tokens[pos];

    // Handle parentheses
    if (token.type == TokenType::LPAREN) {
        pos++; // consume '('
        auto expr = parseExpressionRecursive(tokens, pos, 0);
        
        if (pos >= tokens.size() || tokens[pos].type != TokenType::RPAREN) {
            std::cerr << "Expected ')' after expression" << std::endl;
            return nullptr;
        }
        
        pos++; // consume ')'
        return expr;
    }

    // Handle unary operators
    if (token.type == TokenType::OPERATOR && token.value == "~") {
        pos++; // consume operator
        auto operand = parsePrimary(tokens, pos);
        if (!operand) {
            std::cerr << "Expected operand after unary operator: " << token.value << std::endl;
            return nullptr;
        }
        
        auto unaryNode = std::make_unique<ExprNode>(token);
        unaryNode->children.push_back(std::move(operand));
        return unaryNode;
    }

    // Handle operands
    if (token.type == TokenType::OPERAND) {
        pos++; // consume operand
        return std::make_unique<ExprNode>(token);
    }

    std::cerr << "Unexpected token: " << token.value << std::endl;
    return nullptr;
}

bool MBASimplifier::compilePatternsFromStrings() {
    bool allCompiled = true;
    
    for (auto& pattern : patterns) {
        try {
            compilePattern(pattern);
        } catch (const std::exception& e) {
            std::cerr << "Failed to compile pattern '" << pattern.original 
                      << "': " << e.what() << std::endl;
            allCompiled = false;
        }
    }
    
    return allCompiled;
}

void MBASimplifier::compilePattern(MBAPattern& pattern) {
    auto originalTokens = tokenize(pattern.original);
    auto simplifiedTokens = tokenize(pattern.simplified);
    
    pattern.originalTree = parseExpression(originalTokens);
    pattern.simplifiedTree = parseExpression(simplifiedTokens);
    
    if (!pattern.originalTree || !pattern.simplifiedTree) {
        throw std::runtime_error("Failed to parse pattern expressions");
    }
}

std::vector<std::tuple<std::string, size_t, LowLevelILInstruction, LowLevelILInstruction, LowLevelILInstruction>> 
MBASimplifier::searchPatternsInFunction(const Ref<Function>& func) const {
    std::vector<std::tuple<std::string, size_t, LowLevelILInstruction, LowLevelILInstruction, LowLevelILInstruction>> matches;
    
    const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
    if (!llil) {
        return matches;
    }

    for (Ref<BasicBlock>& block : llil->GetBasicBlocks()) {
        for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++) {
            LowLevelILInstruction instr = (*llil)[instrIndex];
            
            std::unique_ptr<ExprNode> instrTree;
            if (extractLLILSubtree(instr, instrTree)) {
                for (const auto& pattern : patterns) {
                    if (pattern.originalTree && matchPattern(pattern.originalTree.get(), instrTree.get())) {
                        matches.emplace_back(pattern.original, instrIndex, instr, instr, instr);
                        logPatternMatch(pattern.original, instrIndex);
                    }
                }
            }
        }
    }

    return matches;
}

size_t MBASimplifier::replaceObfuscatedWithSimple(
    const Ref<LowLevelILFunction>& llil,
    const LowLevelILInstruction& srcExpr,
    const LowLevelILInstruction& leftExpr,
    const LowLevelILInstruction& rightExpr) {
    
    // Hardcode à la mort là
    ExprId newInstr = llil->AddExpr(
        LLIL_MUL, srcExpr.size, srcExpr.flags,
        llil->Register(leftExpr.size, leftExpr.GetSourceRegister()),
        llil->Register(rightExpr.size, rightExpr.GetSourceRegister())
    );

    llil->ReplaceExpr(srcExpr.exprIndex, newInstr);
    llil->GenerateSSAForm();

    return 1;
}

void MBASimplifier::execute(const Ref<AnalysisContext>& analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    auto matches = searchPatternsInFunction(func);

    std::cout << "Found " << matches.size() << " potential MBA patterns in function" << std::endl;

    for (const auto& match : matches) {
        const auto& [pattern, instrIndex, srcExpr, leftExpr, rightExpr] = match;
        // std::cout << "Pattern: " << pattern
        //           << ", InstrIndex: " << instrIndex
        //           << ", SrcExpr.operation: " << srcExpr.operation
        //           << ", LeftExpr.operation: " << leftExpr.operation
        //           << ", RightExpr.operation: " << rightExpr.operation
        //           << std::endl;
        replaceObfuscatedWithSimple(func->GetLowLevelIL(), srcExpr, leftExpr, rightExpr);
    }
}

// Helper method implementations
int MBASimplifier::mapSymbolToLLIL(const std::string& symbol) const {
    auto it = symbolToLLIL.find(symbol);
    if (it != symbolToLLIL.end()) {
        return it->second;
    }
    
    // Check if it's a register (single letter)
    if (symbol.size() == 1 && std::isalpha(static_cast<unsigned char>(symbol[0]))) {
        return LLIL_REG;
    }
    
    // Check if it's a constant (all digits)
    if (std::all_of(symbol.begin(), symbol.end(), ::isdigit)) {
        return LLIL_CONST;
    }
    
    return -1;
}

int MBASimplifier::getOperatorPrecedence(const std::string& op) const {
    if (op.empty()) return -1;
    
    auto it = operatorPrecedence.find(op[0]);
    return (it != operatorPrecedence.end()) ? it->second : -1;
}

bool MBASimplifier::isOperator(const std::string& token) const {
    return symbolToLLIL.find(token) != symbolToLLIL.end() && 
           token != "~"; // Special case for unary
}

bool MBASimplifier::isOperand(const std::string& token) const {
    return std::isalnum(token[0]) || token[0] == '_';
}

TokenType MBASimplifier::classifyToken(const std::string& token) const {
    if (token.empty()) return TokenType::UNKNOWN;
    
    if (token == "(") return TokenType::LPAREN;
    if (token == ")") return TokenType::RPAREN;
    if (isOperator(token)) return TokenType::OPERATOR;
    if (isOperand(token)) return TokenType::OPERAND;
    
    return TokenType::UNKNOWN;
}

std::string MBASimplifier::trim(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool MBASimplifier::isValidCSVLine(const std::string& line) const {
    std::string trimmed = trim(line);
    return !trimmed.empty() && 
           trimmed[0] != '#' && 
           trimmed != "Original,Obfuscated" &&
           trimmed.find(',') != std::string::npos;
}

bool MBASimplifier::matchPattern(const ExprNode* patternNode, const ExprNode* targetNode) const {
    if (!patternNode || !targetNode) {
        return false;
    }
    
    // This is a simplified matching - in practice, you'd need more sophisticated
    // pattern matching that can handle variable binding and substitution
    if (patternNode->token.llilOpcode != targetNode->token.llilOpcode) {
        return false;
    }
    
    if (patternNode->children.size() != targetNode->children.size()) {
        return false;
    }
    
    for (size_t i = 0; i < patternNode->children.size(); i++) {
        if (!matchPattern(patternNode->children[i].get(), targetNode->children[i].get())) {
            return false;
        }
    }
    
    return true;
}

bool MBASimplifier::extractLLILSubtree(const LowLevelILInstruction& instr, std::unique_ptr<ExprNode>& result) const {
    // This is a placeholder - you'd need to implement the actual conversion
    // from LLIL instructions to expression trees
    Token token(TokenType::OPERATOR, "dummy", instr.operation);
    result = std::make_unique<ExprNode>(token);
    return true;
}

void MBASimplifier::logPatternMatch(const std::string& pattern, size_t instrIndex) const {
    std::cout << "Matched pattern '" << pattern << "' at instruction " << instrIndex << std::endl;
}

void MBASimplifier::printExpressionTree(const ExprNode* node, int depth) const {
    if (!node) return;
    
    std::string indent(depth * 2, ' ');
    std::cout << indent << node->token.value << " (LLIL: " << node->token.llilOpcode << ")" << std::endl;
    
    for (const auto& child : node->children) {
        printExpressionTree(child.get(), depth + 1);
    }
}

std::string MBASimplifier::expressionTreeToString(const ExprNode* node) const {
    if (!node) return "";
    
    if (node->children.empty()) {
        return node->token.value;
    }
    
    if (node->children.size() == 1) {
        return node->token.value + expressionTreeToString(node->children[0].get());
    }
    
    if (node->children.size() == 2) {
        return "(" + expressionTreeToString(node->children[0].get()) + 
               node->token.value + 
               expressionTreeToString(node->children[1].get()) + ")";
    }
    
    return node->token.value;
}