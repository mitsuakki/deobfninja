#include "../../../include/methods/instructions/mbasimplifier.hpp"
#include "../../../binaryninjaapi/lowlevelilinstruction.h"

#include <fstream>
#include <iostream>
#include <filesystem>

using namespace BinaryNinja;

MBASimplifier::MBASimplifier() : IDeobfuscationMethod("MBA Simplifier", DeobfuscationCategory::Workflow) {
    this->isEnabled = true;
    std::string searchDir = "resources/";

    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDir)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find("mba-dataset") != std::string::npos && entry.path().extension() == ".csv") {
                loadPatternsFromCSV(entry.path().string());
            }
        }
    }

    patternsToString();
}

bool MBASimplifier::loadPatternsFromCSV(const std::string& csvFilePath)
{ 
    std::ifstream file(csvFilePath);
    if (!file.is_open())
        return {};

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#' || line == "Original,Obfuscated")
            continue;

        std::stringstream ss(line);
        std::string pattern, replacement;

        if (!std::getline(ss, pattern, ','))
            continue;

        if (!std::getline(ss, replacement))
            continue;
        patterns.emplace_back(std::move(pattern), std::move(replacement));
    }

    return !patterns.empty();
}
std::vector<std::variant<int, std::vector<int>>> MBASimplifier::tokenizeAndMapToLLIL(const std::string& expr) const
{
    std::vector<std::variant<int, std::vector<int>>> llilOps;
    std::vector<std::vector<int>> stack;
    std::vector<int> current;

    std::string token;
    size_t i = 0;
    while (i < expr.size())
    {
        if (isspace(expr[i])) {
            ++i;
            continue;
        }

        if (expr[i] == '(') {
            stack.push_back(current);
            current.clear();
            ++i;
            continue;
        }
        if (expr[i] == ')') {
            if (!stack.empty()) {
                std::vector<int> group = current;
                current = stack.back();
                stack.pop_back();
                llilOps.push_back(group);
            }
            ++i;
            continue;
        }

        if (isalpha(expr[i])) {
            size_t start = i;
            while (i < expr.size() && (isalnum(expr[i]) || expr[i] == '_'))
                ++i;
            token = expr.substr(start, i - start);
        } else {
            token = expr.substr(i, 1);
            ++i;
        }

        int op = mapSymbolToLLIL(token);
        if (op != -1)
            current.push_back(op);
    }

    if (!current.empty())
        llilOps.push_back(current);

    return llilOps;
}

std::vector<std::tuple<std::string, size_t, LowLevelILInstruction, LowLevelILInstruction, LowLevelILInstruction>> MBASimplifier::searchPatternsInFunction(const Ref<Function>& func) const
{
    std::vector<std::tuple<std::string, size_t, LowLevelILInstruction, LowLevelILInstruction, LowLevelILInstruction>> matches;
    const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
    if (llil)
    {
        for (Ref<BasicBlock>& block : llil->GetBasicBlocks())
        {
            for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++) 
            {
                LowLevelILInstruction instr = (*llil)[instrIndex];
                for (const auto& pattern : patterns)
                {
                    // TODO
                }
            }
        }
    }

    return matches;
}

size_t MBASimplifier::replaceObfuscatedWithSimple(const Ref<LowLevelILFunction>& llil,
    const LowLevelILInstruction& srcExpr,
    const LowLevelILInstruction& leftExpr,
    const LowLevelILInstruction& rightExpr)
{
    ExprId newInstr = llil->AddExpr(
        LLIL_MUL, srcExpr.size, srcExpr.flags,
        llil->Register(leftExpr.size, leftExpr.GetSourceRegister()),
        llil->Register(rightExpr.size, rightExpr.GetSourceRegister())
    );

    llil->ReplaceExpr(srcExpr.exprIndex, newInstr);
    llil->GenerateSSAForm();

    return 1;
}

void MBASimplifier::execute(const Ref<AnalysisContext>& analysisContext)
{
    // const Ref<Function> func = analysisContext->GetFunction();
    // auto matches = searchPatternsInFunction(func);
    // const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();

    // for (const auto& match : matches)
    // {
    //     const auto& [pattern, instrIndex, srcExpr, leftExpr, rightExpr] = match;
    //     replaceObfuscatedWithSimple(llil, srcExpr, leftExpr, rightExpr);
    // }
}