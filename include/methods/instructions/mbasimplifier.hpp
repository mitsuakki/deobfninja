#ifndef METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP
#define METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP

#pragma once

#include "../../deobfuscator.hpp"

/**
 * @brief The MBASimplifier class implements the IDeobfuscationMethod interface
 * to simplify MBA instructions in a binary.
 */
class MBASimplifier : public IDeobfuscationMethod
{
public:
    MBASimplifier();

    bool loadPatternsFromCSV(const std::string& csvFilePath);

    /**
     * @brief Tokenizes an MBA expression and maps it to LLIL operations.
     */
    std::vector<std::variant<int, std::vector<int>>> tokenizeAndMapToLLIL(const std::string& expr) const;

    std::vector<std::tuple<
        std::string, size_t,
        BinaryNinja::LowLevelILInstruction,
        BinaryNinja::LowLevelILInstruction,
        BinaryNinja::LowLevelILInstruction
    >> searchPatternsInFunction(const BinaryNinja::Ref<BinaryNinja::Function>& func) const;

    size_t replaceObfuscatedWithSimple(
        const BinaryNinja::Ref<BinaryNinja::LowLevelILFunction>& llil,
        const BinaryNinja::LowLevelILInstruction& srcExpr,
        const BinaryNinja::LowLevelILInstruction& leftExpr,
        const BinaryNinja::LowLevelILInstruction& rightExpr);

    void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext) override;

private:
    std::vector<std::pair<std::string, std::string>> patterns;

    /**
     * @brief Maps a simple operation symbol (e.g., '+') to the corresponding BinaryNinja LLIL operation constant.
     */
    int mapSymbolToLLIL(const std::string& opSymbol) const {
        if (opSymbol == "+") return LLIL_ADD;
        if (opSymbol == "-") return LLIL_SUB;
        if (opSymbol == "*") return LLIL_MUL;
        if (opSymbol == "^") return LLIL_XOR;
        if (opSymbol == "&") return LLIL_AND;
        if (opSymbol == "|") return LLIL_OR;
        if (opSymbol == "~") return LLIL_NOT;
        if (opSymbol == "/") return LLIL_DIVU;
        if (opSymbol == "%") return LLIL_MODU;
        if (opSymbol == "<<" || opSymbol == "<<<") return LLIL_LSL;
        if (opSymbol == ">>" || opSymbol == ">>>") return LLIL_LSR;
        if (opSymbol == "==") return LLIL_CMP_E;
        if (opSymbol == "!=") return LLIL_CMP_NE;
        if (opSymbol == "<") return LLIL_CMP_ULT;
        if (opSymbol == "<=") return LLIL_CMP_ULE;
        if (opSymbol == ">") return LLIL_CMP_UGT;
        if (opSymbol == ">=") return LLIL_CMP_UGE;
        if (opSymbol == "x" || opSymbol == "y") return LLIL_REG;
        
        try {
            size_t idx;
            std::stoi(opSymbol, &idx);

            if (idx == opSymbol.size())
                return LLIL_CONST;
                
        } catch (const std::invalid_argument&) {}

        return -1;
    }

    void patternsToString() {
        std::cout << "Number of patterns loaded: " << patterns.size() << std::endl;
        for (const auto& pattern : patterns) {
            auto tokensFirst = tokenizeAndMapToLLIL(pattern.first);
            auto tokensSecond = tokenizeAndMapToLLIL(pattern.second);

            std::cout << "original " << pattern.first << " ([";
            for (size_t i = 0; i < tokensFirst.size(); ++i) {
                std::visit([](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, int>) {
                        std::cout << arg;
                    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                        std::cout << "[";
                        for (size_t j = 0; j < arg.size(); ++j) {
                            std::cout << arg[j];
                            if (j != arg.size() - 1) std::cout << " ";
                        }
                        std::cout << "]";
                    }
                }, tokensFirst[i]);
                if (i != tokensFirst.size() - 1) std::cout << ", ";
            }

            std::cout << "]) -> obfuscated " << pattern.second << " ([";
            for (size_t i = 0; i < tokensSecond.size(); ++i) {
                std::visit([](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, int>) {
                        std::cout << arg;
                    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                        std::cout << "[";
                        for (size_t j = 0; j < arg.size(); ++j) {
                            std::cout << arg[j];
                            if (j != arg.size() - 1) std::cout << " ";
                        }
                        std::cout << "]";
                    }
                }, tokensSecond[i]);
                if (i != tokensSecond.size() - 1) std::cout << ", ";
            }
            std::cout << "])" << std::endl;
        }
    }
};

#endif // METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP
