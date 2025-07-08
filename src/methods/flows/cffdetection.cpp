#include "../../../include/methods/flows/cffdetection.hpp"
#include <cxxabi.h>  

using namespace Flows;

CFFDetection::CFFDetection()
    : IDeobfuscationMethod("CFF Detection", DeobfuscationCategory::Function)
{
    this->isEnabled = true;
}

int CFFDetection::CalcCyclomaticComplexity(Ref<Function> func)
{
    std::vector<Ref<BasicBlock>> basicBlocks = func->GetBasicBlocks();
    int numOfBlocks = basicBlocks.size();
    int numOfEdges = 0;

    for (const auto& block : basicBlocks)
        numOfEdges += block->GetOutgoingEdges().size();

    return numOfEdges - numOfBlocks + 2;
}

std::set<Ref<BasicBlock>> CFFDetection::getDominatedBy(const Ref<BasicBlock>& dominator)
{
    std::set<Ref<BasicBlock>> result;
    std::vector<Ref<BasicBlock>> worklist = { dominator };

    while (!worklist.empty()) {
        Ref<BasicBlock> block = worklist.back();
        worklist.pop_back();
        result.insert(block);

        for (const auto& child : block->GetDominatorTreeChildren()) {
            if (!result.count(child))
                worklist.push_back(child);
        }
    }

    return result;
}

double CFFDetection::CalcFlatteningScore(const Ref<Function>& function)
{
    double score = 0.0;
    std::vector<Ref<BasicBlock>> blocks = function->GetBasicBlocks();

    for (const auto& block : blocks) {
        std::set<Ref<BasicBlock>> dominated = getDominatedBy(block);

        bool hasBackEdge = false;
        for (const auto& edge : block->GetIncomingEdges()) {
            if (dominated.count(edge.target)) {
                hasBackEdge = true;
                break;
            }
        }

        if (!hasBackEdge)
            continue;

        score = std::max(score, static_cast<double>(dominated.size()) / static_cast<double>(blocks.size()));
    }

    return score;
}

Variable CFFDetection::GetMostAssignedVar(const Ref<Function>& func)
{
    std::map<Variable, size_t> varAssignementCount;
    Ref<HighLevelILFunction> hlil = func->GetHighLevelIL();

    if (!hlil)
        throw std::runtime_error("No HLIL available for this function");

    for (const auto& var : hlil->GetVariables())
        varAssignementCount[var] = hlil->GetVariableDefinitions(var).size();

    if (varAssignementCount.empty())
        throw std::runtime_error("No variables found");

    auto mostUsed = std::max_element(
        varAssignementCount.begin(), varAssignementCount.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    return mostUsed->first;
}

std::vector<Variable> CFFDetection::GetVarDependencies(const Ref<HighLevelILFunction>& hlilFunc, const Variable& var)
{
    std::vector<Variable> deps;
    std::set<size_t> defines = hlilFunc->GetVariableDefinitions(var);

    for (const auto& def : defines) {
        // TODO: Future implementation for analyzing expressions and dependencies.
    }

    return deps;
}


void CFFDetection::execute(const Ref<AnalysisContext>& context)
{
    int status;
    const char* mangled = typeid(*this).name();
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    std::string className = (status == 0 && demangled) ? demangled : mangled;
    free(demangled);  // Prevent memory leak

    std::string prefix = std::string(PROJECT_NAME) + "\\" + className + "\\";

    PluginCommand::RegisterForFunction(
        prefix + "Cyclomatic Complexity",
        "Calculate the cyclomatic complexity of the function",
        [](BinaryView* view, Function* func) {
            Flows::CFFDetection detector;
            int complexity = detector.CalcCyclomaticComplexity(func);
            LogInfo("Cyclomatic complexity of %s: %d", func->GetSymbol()->GetFullName().c_str(), complexity);
        }
    );

    PluginCommand::RegisterForFunction(
        prefix + "Flattening Score",
        "Estimate the flattening score of the function",
        [](BinaryView* view, Function* func) {
            Flows::CFFDetection detector;
            double score = detector.CalcFlatteningScore(func);
            LogInfo("Flattening score of %s: %.2f", func->GetSymbol()->GetFullName().c_str(), score);
        }
    );

    PluginCommand::RegisterForFunction(
        prefix + "Dispatcher Detection",
        "Try to detect the dispatcher variable",
        [](BinaryView* view, Function* func) {
            Flows::CFFDetection detector;
            try {
                Variable var = detector.GetMostAssignedVar(func);
                LogInfo("Most assigned variable in %s: %s", func->GetSymbol()->GetFullName().c_str(), func->GetVariableName(var).c_str());
            } catch (const std::exception& e) {
                LogWarn("Dispatcher detection failed: %s", e.what());
            }
        }
    );
}
