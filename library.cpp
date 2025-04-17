#include <cstdio>
#include <ranges>
#include <numeric>
#include <cinttypes>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>

#include "library.h"
#include "binaryninjaapi.h"

#include "highlevelilinstruction.h"

#include "patterns/examples/llil_add.hpp"
#include "patterns/examples/mlil_add.hpp"

using namespace BinaryNinja;
using namespace std;

extern "C"
{
    BINARYNINJAPLUGIN uint32_t CorePluginABIVersion() { return BN_CURRENT_CORE_ABI_VERSION; }

    void RegisterMBAPatches() {
        LLIL_ADD_Searcher* llilAddSearcher = new LLIL_ADD_Searcher();
        Ref<Workflow> customLLILPatcherWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("LLILPatcherWorkflow");
        customLLILPatcherWorkflow->RegisterActivity(new Activity("extension.llilpatcher", [llilAddSearcher](const Ref<AnalysisContext>& context) {
            llilAddSearcher->SearchLLIL(context);
        }));

        customLLILPatcherWorkflow->Insert("core.function.generateMediumLevelIL", "extension.llilpatcher");
        Workflow::RegisterWorkflow(customLLILPatcherWorkflow,
            R"#({
                "title" : "Test4",
                "description" : "This analysis stands in as an example to demonstrate Binary Ninja's extensible analysis APIs.",
                "targetType" : "function"
            })#"
        );

        MLIL_ADD_Searcher* mlilAddSearcher = new MLIL_ADD_Searcher();
        Ref<Workflow> customMLILPatcherWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("MLILPatcherWorkflow");
        customMLILPatcherWorkflow->RegisterActivity(new Activity("extension.mlilpatcher", [mlilAddSearcher](const Ref<AnalysisContext>& context) {
            mlilAddSearcher->SearchMLIL(context);
        }));

        customMLILPatcherWorkflow->Insert("core.function.generateHighLevelIL", "extension.mlilpatcher");
        Workflow::RegisterWorkflow(customMLILPatcherWorkflow,
            R"#({
			    "title" : "Test7",
			    "description" : "This analysis stands in as an example to demonstrate Binary Ninja's extensible analysis APIs.",
			    "targetType" : "function"
			})#"
        );
    }

    int CalcCyclomaticComplexity(Ref<Function> func) {
        std::vector<Ref<BasicBlock>> basicBlocks = func->GetBasicBlocks();
        int numOfBlocks = basicBlocks.size();
        int numOfEdges = 0;

        for (const auto& block : basicBlocks) {
            numOfEdges += block->GetOutgoingEdges().size();
        }

        return numOfEdges - numOfBlocks + 2;
    }

    std::set<Ref<BasicBlock>> getDominatedBy(const Ref<BasicBlock>& dominator) {
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

    double CalcFlatteningScore(const Ref<Function>& function) {
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

    Variable GetMostAssignedVar(const Ref<Function>& func) {
        std::map<Variable, size_t> varAssignementCount;

        Ref<HighLevelILFunction> hlil = func->GetHighLevelIL();
        if(!hlil)
          throw std::runtime_error("No HLIL available for this function");

        for (const auto& var : hlil->GetVariables()) {
          auto defs = hlil->GetVariableDefinitions(var);
          varAssignementCount[var] = defs.size();
        }

        if (varAssignementCount.empty())
            throw std::runtime_error("No variables found");

        auto mostUsed = std::max_element(
            varAssignementCount.begin(), varAssignementCount.end(),
            [](const auto& a, const auto& b) {
              return a.second < b.second;
            }
        );

        return mostUsed->first;
    }

    std::vector<Variable> GetVarDependencies(const Ref<HighLevelILFunction>& hlilFunc, const Variable& var)
    {
        std::vector<Variable> deps;
        std::set<size_t> defines = hlilFunc->GetVariableDefinitions(var);

        for (const auto& def : defines) {
            // ToDo : Lionel a dit fusionne les basicblocks, ignore les etc
            // L'analyse sera là pour plus tard
        }

        return deps;
    }

    void RegisterCFFPatches() {
        PluginCommand::RegisterForFunction("eshard\\CFF\\Cyclomatic complexity", "Compute cyclomatic complexity", [](BinaryView* view, Function *func) {
            int complexity = CalcCyclomaticComplexity(func);
            LogInfo("Cyclomatic complexity of %s %d", func->GetSymbol()->GetFullName().c_str(), complexity);
        });

        PluginCommand::Register("eshard\\CFF\\Complex Functions", "Find complex functions", [](BinaryView* view) {
            std::vector<Ref<Function>> funcs = view->GetAnalysisFunctionList();

            std::sort(funcs.begin(), funcs.end(), [](const Ref<Function>& a, const Ref<Function>& b) {
                return CalcCyclomaticComplexity(a) < CalcCyclomaticComplexity(b);
            });

            size_t bound = static_cast<size_t>(std::ceil(funcs.size() * 0.10));
            for (auto it = funcs.rbegin(); it != funcs.rbegin() + bound; ++it) {
                std::cout << "0x" << std::hex << (*it)->GetStart() << ": "
                          << std::dec << CalcCyclomaticComplexity(*it) << std::endl;
            }
        });

        PluginCommand::RegisterForFunction("eshard\\CFF\\Flattening score", "Compute flattening score", [](BinaryView* view, Function* func) {
            double score = CalcFlatteningScore(func);
            LogInfo("Flattening score of %s: %.2f", func->GetSymbol()->GetFullName().c_str(), score);
        });

        PluginCommand::RegisterForFunction("eshard\\CFF\\Most Assigned Var", "Find the most assigned variable in HLIL", [](BinaryView* view, Function* func) {
            Variable var = GetMostAssignedVar(func);
            LogInfo("Most assigned var (probably the state variable is %s", func->GetVariableName(var).c_str());
        });
    }

    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        PluginCommand::Register("eshard\\All actvities", "Print all activities", [](BinaryView* view) {
            const Ref<Workflow> defaultWf = Workflow::Instance("core.function.baseAnalysis");
            for (const auto& activity : defaultWf->GetSubactivities()) {
                LogInfo("Activity: %s", activity.c_str());
            }
        });

        RegisterMBAPatches();
        RegisterCFFPatches();

        PluginCommand::RegisterForFunction("eshard\\CFF\\Var dependencies", "Get var dependencies of the most assigned var", [](BinaryView* view, Function* func) {
            auto hlil = func->GetHighLevelIL();
            if (!hlil) {
                LogWarn("No HLIL available for this function.");
                return;
            }

            Variable mostAssignedVar = GetMostAssignedVar(func);
            LogInfo("mostAssignedVar: %s", func->GetVariableName(mostAssignedVar).c_str());

            std::vector<Variable> dependencies = GetVarDependencies(hlil, mostAssignedVar);
            for (const auto& dep : dependencies) {
                LogInfo("Dependency: %s", func->GetVariableName(dep).c_str());
            }
        });

        return true;
    }
}