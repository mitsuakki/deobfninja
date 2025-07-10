#include "../../../include/methods/instructions/mbasimplifier.hpp"
#include "../../../binaryninjaapi/lowlevelilinstruction.h"

#include "../../../include/utils/ilprinter.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cctype>

using namespace BinaryNinja;
using namespace Instructions;

// Static member definitions
const std::unordered_map<std::string, int> MBASimplifier::symbolToLLIL = {
    {"+", LLIL_ADD}, {"-", LLIL_SUB}, {"*", LLIL_MUL}, {"^", LLIL_XOR},
    {"&", LLIL_AND}, {"|", LLIL_OR}, {"~", LLIL_NOT}, {"/", LLIL_DIVU},
    {"%", LLIL_MODU}, {"<<", LLIL_LSL}, {">>", LLIL_LSR}, {"<<<", LLIL_LSL},
    {">>>", LLIL_LSR}, {"==", LLIL_CMP_E}, {"!=", LLIL_CMP_NE},
    {"<", LLIL_CMP_ULT}, {"<=", LLIL_CMP_ULE}, {">", LLIL_CMP_UGT},
    {">=", LLIL_CMP_UGE}
};

const std::unordered_map<int, std::string> MBASimplifier::llilToSymbol = []() {
    std::unordered_map<int, std::string> rev;
    for (const auto& [sym, op] : MBASimplifier::symbolToLLIL) {
        rev[op] = sym;
    }
    return rev;
}();

std::string MBASimplifier::OperationToString(BNLowLevelILOperation op) const {
    auto it = llilToSymbol.find(op);
    if (it != llilToSymbol.end()) {
        return it->second;
    }
    return "UNKNOWN::" + op;
}

const std::unordered_map<char, int> MBASimplifier::operatorPrecedence = {
    {'(', 0}, {')', 0},
    {'|', 1}, {'^', 2}, {'&', 3},
    {'=', 4}, {'!', 4}, {'<', 4}, {'>', 4},
    {'+', 5}, {'-', 5},
    {'*', 6}, {'/', 6}, {'%', 6},
    {'~', 7}
};

MBASimplifier::MBASimplifier()
    : IDeobfuscationMethod("Reduce complex operations", DeobfuscationCategory::Workflow) {
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
        std::string obfuscated, original;

        if (std::getline(ss, obfuscated, ',') && std::getline(ss, original)) {
            obfuscated = trim(obfuscated);
            obfuscated = trim(original);
            
            if (!obfuscated.empty() && !original.empty()) {
                patterns.emplace_back(obfuscated, original);
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


std::vector<std::tuple<std::string, size_t, LowLevelILInstruction>>
MBASimplifier::findMatches(const Ref<Function>& func) {
    std::vector<std::tuple<std::string, size_t, LowLevelILInstruction>> matches;
    auto llil = func->GetLowLevelIL();

    if (!llil) {
        return matches;
    }

    for (Ref<BasicBlock>& block : llil->GetBasicBlocks()) {
        for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++) {
            LowLevelILInstruction instr = (*llil)[instrIndex];

            if (instr.operation == LLIL_SET_REG) {
                LowLevelILInstruction srcExpr = instr.GetSourceExpr<LLIL_SET_REG>();

                for (const auto& pattern : patterns) {
                    auto tokens = tokenize(pattern.obfuscated);
                    auto tree = parseExpression(tokens);

                    if (matchPattern(tree.get(), srcExpr)) {
                        matches.emplace_back(pattern.obfuscated, instrIndex, srcExpr);
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
    const LowLevelILInstruction& rightExpr)
{
    ExprId left = llil->Register(leftExpr.size, leftExpr.GetSourceRegister());
    ExprId right = llil->Register(rightExpr.size, rightExpr.GetSourceRegister());
    ExprId simplified = llil->AddExpr(LLIL_MUL, srcExpr.size, left, right);
    llil->ReplaceExpr(srcExpr.exprIndex, simplified);
    llil->GenerateSSAForm();

    return 1;
}
void MBASimplifier::execute(const Ref<AnalysisContext>& analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();

    // auto matches = findMatches(func);
    // std::cout << "[MBASimplifier] Found " << matches.size() << " matches:\n";

<<<<<<< Updated upstream
    size_t replacedCount = 0;
    for (const auto& match : matches) {
        const std::string& pattern = std::get<0>(match);
        size_t index = std::get<1>(match);
        const LowLevelILInstruction& srcExpr = std::get<2>(match); 
=======
    // size_t replacedCount = 0;
    // for (const auto& match : matches) {
    //     const std::string& pattern = std::get<0>(match);
    //     size_t index = std::get<1>(match);
    //     const HighLevelILInstruction& srcExpr = std::get<2>(match); 
>>>>>>> Stashed changes

    //     std::cout << "  [Debug] At index " << index << ", matched pattern: \"" << pattern << "\"\n";
    //     std::cout << "  [Debug] Matched expression (" << std::showbase << std::hex << srcExpr.address << std::dec << "):\n";
    //     Utils::PrintILExpr(srcExpr, 1);

<<<<<<< Updated upstream
        if (srcExpr.operation == LLIL_MUL || srcExpr.operation == LLIL_ADD || srcExpr.operation == LLIL_XOR) {
            const LowLevelILInstruction leftExpr = srcExpr.GetLeftExpr();
            const LowLevelILInstruction rightExpr = srcExpr.GetRightExpr();

            replacedCount += replaceObfuscatedWithSimple(llil, srcExpr, leftExpr, rightExpr);
            std::cout << "  [Info] Replaced pattern \"" << pattern << "\" at instruction index " << index << "\n";
        } else {
            std::cout << "  [Info] Skipped non-binary operation at index " << index << "\n";
        }
    }
=======
    //     if (srcExpr.operation == HLIL_MUL || srcExpr.operation == HLIL_ADD || srcExpr.operation == HLIL_XOR) {
    //         const HighLevelILInstruction leftExpr = srcExpr.GetLeftExpr();
    //         const HighLevelILInstruction rightExpr = srcExpr.GetRightExpr();

    //         replacedCount += replaceObfuscatedWithSimple(hlil, srcExpr, leftExpr, rightExpr);
    //         std::cout << "  [Info] Replaced pattern \"" << pattern << "\" at instruction index " << index << "\n";
    //     } else {
    //         std::cout << "  [Info] Skipped non-binary operation at index " << index << "\n";
    //     }
    // }
>>>>>>> Stashed changes

    // std::cout << "[MBASimplifier] Replaced " << replacedCount << " expressions\n";
    
    if (!hlil) {
        LogAlert("HLIL pas dispo à cette couche");
        return;
    }
}

int MBASimplifier::mapSymbolToLLIL(const std::string& symbol) const {
    auto it = symbolToLLIL.find(symbol);
    if (it != symbolToLLIL.end()) {
        return it->second;
    }

    if (symbol.size() == 1 && std::isalpha(static_cast<unsigned char>(symbol[0]))) {
        return LLIL_REG;
    }
    
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
           trimmed != "Obfuscated,Original" &&
           trimmed.find(',') != std::string::npos;
}

bool MBASimplifier::matchPattern(const ExprNode* patternNode, const BinaryNinja::LowLevelILInstruction& ilNode) const {
    if (!patternNode)
        return false;

    if (patternNode->token.type == TokenType::OPERATOR) {
        if (patternNode->token.llilOpcode != ilNode.operation)
            return false;
    } else if (patternNode->token.type == TokenType::OPERAND) {
        switch (ilNode.operation) {
            case LLIL_CONST: {
                int64_t ilValue = ilNode.GetConstant();
                try {
                    int64_t patternValue = std::stoll(patternNode->token.value);
                    if (patternValue != ilValue)
                        return false;
                } catch (...) {
                    return false;
                }
                break;
            }
            case LLIL_REG:
                // Accept registers and variables as wildcard matches
                break;
            default:
                return false;
        }
    } else {
        return false;
    }

    // Recursively match children nodes
    size_t patternChildCount = patternNode->children.size();
    const auto& operands = ilNode.GetOperands();
    size_t ilOperandCount = operands.size();
    size_t count = std::min(patternChildCount, ilOperandCount);

    for (size_t i = 0; i < count; i++) {
        if (operands[i].GetType() != LowLevelILOperandType::ExprLowLevelOperand)
            return false;

        LowLevelILInstruction childInstr = operands[i].GetExpr();
        if (!matchPattern(patternNode->children[i].get(), childInstr)) {
            return false;
        }
    }

    return true;
}

std::unique_ptr<ExprNode> MBASimplifier::extractLLILSubtree(const LowLevelILInstruction& instr) const {
    Token token(TokenType::OPERATOR, OperationToString(instr.operation), instr.operation);
    auto node = std::make_unique<ExprNode>(token);

    const auto& operands = instr.GetOperands();
    size_t operandCount = operands.size();

    for (size_t i = 0; i < operandCount; ++i) {
        auto operand = instr.GetRawOperandAsExpr(i);
        if (operand.operation == LLIL_NOP)
            continue;

        node->children.push_back(extractLLILSubtree(operand));
    }

    return node;
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