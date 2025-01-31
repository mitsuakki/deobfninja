#include <set>
#include <map>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cinttypes>

#include "library.h"
#include "binaryninjaapi.h"

#include "patterns/examples/llil_add.hpp"
#include "patterns/examples/mlil_add.hpp"

#include "highlevelilinstruction.h"

using namespace BinaryNinja;
using namespace std;


void PatchFunction(BinaryView* bv, Function* func) {
    std::vector<BasicBlock*> cffHeads;

    for (auto& block : func->GetHighLevelIL()->GetBasicBlocks()) {
        int dominatedEdges = 0;
        for (auto& edge : block->GetIncomingEdges()) {
            if (edge.target->GetDominatorTreeChildren().count(block))
                dominatedEdges++;
        }
        if (dominatedEdges >= 3)
            cffHeads.push_back(block);
    }

    if (cffHeads.empty()) return;
    BasicBlock* cffHead = cffHeads[0];

    std::set<BasicBlock*> blocks;
    std::vector<BasicBlock*> toVisit = {cffHead};
    while (!toVisit.empty()) {
        BasicBlock* block = toVisit.back();
        toVisit.pop_back();
        for (auto& edge : block->GetIncomingEdges()) {
            BasicBlock* candidateBlock = edge.target;
            if (cffHead->GetDominatorTreeChildren().count(candidateBlock) && blocks.count(candidateBlock) == 0) {
                blocks.insert(candidateBlock);
                toVisit.push_back(candidateBlock);
            }
        }
    }

    std::set<HighLevelILInstruction> conditions;
    for (auto* block : blocks) {
        for (auto& edge : block->GetIncomingEdges()) {
            auto condition = func->GetHighLevelIL()->GetInstruction(edge.target->GetEnd() - 1);
            if (condition.operation == HLIL_IF)
                conditions.insert(condition);
        }
    }

    std::map<Variable, int> varCounts;
    for (const auto& condition : conditions) {
        for (const auto& var : condition.GetOperands()[0].GetVariable()) {
            varCounts[var]++;
        }
    }

    auto targetVar = std::max_element(varCounts.begin(), varCounts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; })->first;

    std::map<uint64_t, BasicBlock*> codeLookup;
    for (const auto& condition : conditions) {
        if (condition.operation != HLIL_CMP_E)
            throw std::runtime_error("Unsupported condition type");

        auto trueBranch = condition.GetTrueBranch<BasicBlock>();
        auto constValue = condition.GetOperands()[1].GetConstant();
        if (!constValue)
            throw std::runtime_error("No constant value found");

        codeLookup[constValue->GetValue()] = trueBranch->GetBasicBlock();
    }

    std::map<uint64_t, BasicBlock*> blockExits;
    for (auto* block : blocks) {
        for (size_t i = block->GetStart(); i < block->GetEnd(); i++) {
            auto instr = func->GetHighLevelIL()->GetInstruction(i);
            if (instr.operation == HLIL_ASSIGN && instr.GetOperands()[0].IsVariable(targetVar)) {
                blockExits[instr.GetOperands()[1].GetConstant()->GetValue()] = block;
            }
        }
    }

    for (const auto& [code, block] : blockExits) {
        auto asmBlock = func->GetBasicBlockAt(block->GetStart());
        auto lastInstr = asmBlock->GetInstructionText().back();
        if (lastInstr[0].m_text != "jmp")
            throw std::runtime_error("Block does not end with jmp");

        auto address = lastInstr[0].m_address;
        auto length = asmBlock->GetEnd() - address;

        auto outBlock = codeLookup[code];
        auto outAsmBlock = func->GetBasicBlockAt(outBlock->GetStart());
        auto outAddress = outAsmBlock->GetStart();

        auto bytecode = bv->Read(address, length);
        std::vector<uint8_t> newBytecode;

        if (length == 2 && bytecode[0] == 0xEB) {
            int8_t delta = outAddress - asmBlock->GetEnd();
            if (std::abs(delta) > 0x7F) continue;
            newBytecode = {0xEB, static_cast<uint8_t>(delta)};
        } else if (length == 5 && bytecode[0] == 0xE9) {
            int32_t delta = outAddress - asmBlock->GetEnd();
            newBytecode = {0xE9, static_cast<uint8_t>(delta & 0xFF), static_cast<uint8_t>((delta >> 8) & 0xFF),
                           static_cast<uint8_t>((delta >> 16) & 0xFF), static_cast<uint8_t>((delta >> 24) & 0xFF)};
        } else {
            continue;
        }

        bv->Write(address, newBytecode);
    }
}

void MergeBasicBlocks(const Ref<AnalysisContext>& analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<MediumLevelILFunction> il = func->GetMediumLevelIL();

    if (!il) {
        printf("\tDoes not have MLIL\n\n");
        return;
    }

    // hardcoded for testing purposes since we are just testing basic blocks feature
    if (func->GetSymbol()->GetFullName() == "simple_branch") {
        printf("[TEST] %s\n", func->GetSymbol()->GetFullName().c_str());

        std::vector<Ref<BasicBlock>> blocks = il->GetBasicBlocks();
        for (size_t i = 0; i < blocks.size(); ++i) {
            Ref<BasicBlock> block = blocks[i];
            printf("[TEST] Block start: %" PRIx64 ", end: %" PRIx64 "\n", block->GetStart(), block->GetEnd());

            // if (block->GetOutgoingEdges().size() == 1) {
            //     Ref<BasicBlock> targetBlock = block->GetOutgoingEdges()[0].target;
            //     printf("[TEST] Target block start: %" PRIx64 ", end: %" PRIx64 "\n", targetBlock->GetStart(), targetBlock->GetEnd());
            //
            //     for (size_t instrIndex = targetBlock->GetStart(); instrIndex < targetBlock->GetEnd(); ++instrIndex) {
            //         MediumLevelILInstruction instr = (*il)[instrIndex];
            //         printf("[TEST] %lu\n", instr.exprIndex);
            //         il->AddInstruction(instr.exprIndex);
            //     }
            //
            //     blocks.erase(blocks.begin() + i + 1);
            //     --i;
            // }
        }

        il->GenerateSSAForm();
    }
}

extern "C"
{
    /**
     * @brief Get the core plugin ABI version.
     * @return The core plugin ABI version.
     */
    BINARYNINJAPLUGIN uint32_t CorePluginABIVersion() { return BN_CURRENT_CORE_ABI_VERSION ; }

    /**
     * @brief Initialize the core plugin.
     * @return True if initialization is successful, false otherwise.
     */
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        // if (freopen("out/libeShard.log", "w", stdout) == nullptr) {
        //     perror("Failed to redirect stdout to log file");
        // }

        PluginCommand::Register("eshard\\All actvities", "Print all activities", [](BinaryView* view) {
            const Ref<Workflow> defaultWf = Workflow::Instance("core.function.baseAnalysis");
            for (const auto& activity : defaultWf->GetSubactivities()) {
                LogInfo("Activity: %s", activity.c_str());
            }
        });

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

        Ref<Workflow> customMergeBasicBlocksWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("MergeBasicBlocksWorkflow");
        customMergeBasicBlocksWorkflow->RegisterActivity(new Activity("extension.mergebasicblocks", [](const Ref<AnalysisContext>& context) {
            MergeBasicBlocks(context);
        }));

        customMergeBasicBlocksWorkflow->Insert("core.function.generateHighLevelIL", "extension.mergebasicblocks");
        Workflow::RegisterWorkflow(customMergeBasicBlocksWorkflow,
            R"#({
                "title" : "Merge Basic Blocks",
                "description" : "This workflow merges basic blocks in the medium level IL.",
                "targetType" : "function"
            })#"
        );

        PluginCommand::Register("Patch CFF", "Patch code flow flattening", [](BinaryView* bv) {
            for (auto& func : bv->GetAnalysisFunctions()) {
                PatchFunction(bv, func);
            }
        });

        return true;
    }
}