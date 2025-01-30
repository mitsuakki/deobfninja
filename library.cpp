#include <cstdio>
#include <cinttypes>

#include "library.h"

#include "binaryninjaapi.h"
#include "lowlevelilinstruction.h"
#include "mediumlevelilinstruction.h"

#include "binaryninjaapi/examples/mlil_parser/src/mlil_parser.cpp"
#include "binaryninjaapi/examples/llil_parser/src/llil_parser.cpp"

using namespace BinaryNinja;
using namespace std;

void SearchLLIL_ADD(BinaryView *bv) {
    for (auto& func : bv->GetAnalysisFunctionList()) {
        Ref<Symbol> sym = func->GetSymbol();
        // printf("%s", sym->GetFullName().c_str());

        if (sym->GetFullName() == "main") {
            Ref<LowLevelILFunction> il = func->GetLowLevelIL();
            if (!il) {
                printf("    Does not have LLIL\n\n");
                return;
            }

            for (auto& block : il->GetBasicBlocks()) {
                for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++) {
                    LowLevelILInstruction instr = (*il)[instrIndex];
                    if (instr.operation == LLIL_SET_REG) {
                        auto srcExpr = instr.GetSourceExpr<LLIL_SET_REG>();
                        if (srcExpr.operation == LLIL_ADD) {
                            auto leftExpr = srcExpr.GetLeftExpr<LLIL_ADD>();
                            auto rightExpr = srcExpr.GetRightExpr<LLIL_ADD>();
                            if (leftExpr.operation == LLIL_REG && rightExpr.operation == LLIL_REG) {
                                printf("\nProcessing instruction at 0x%" PRIx64 " with operation %d at block 0x%" PRIx64 "\n", instr.address, instr.operation, block->GetStart());
                                PrintILExpr(instr, 2);
                            }
                        }
                    }
                }
            }
        }
    }
}

void ReplaceLLIL_ADDIntoLLIL_MUL(const Ref<AnalysisContext> &analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<Symbol> sym = func->GetSymbol();
    // printf("%s", sym->GetFullName().c_str());

    if (sym->GetFullName() == "main") {
        const Ref<LowLevelILFunction> il = func->GetLowLevelIL();
        if (!il) {
            printf("    Does not have LLIL\n\n");
            return;
        }

        bool updated = false;
        for (auto& block : il->GetBasicBlocks()) {
            for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++) {
                LowLevelILInstruction instr = (*il)[instrIndex];
                if (instr.operation == LLIL_SET_REG) {
                    auto srcExpr = instr.GetSourceExpr<LLIL_SET_REG>();

                    if (srcExpr.operation == LLIL_ADD) {
                        auto leftExpr = srcExpr.GetLeftExpr<LLIL_ADD>();
                        auto rightExpr = srcExpr.GetRightExpr<LLIL_ADD>();

                        if (leftExpr.operation == LLIL_REG && rightExpr.operation == LLIL_REG) {
                            printf("\nProcessing instruction at 0x%" PRIx64 " with operation %d at block 0x%" PRIx64 "\n", instr.address, instr.operation, block->GetStart());
                            PrintILExpr(instr, 2);

                            const ExprId newInstr = il->AddExpr(
                                LLIL_MUL, srcExpr.size, srcExpr.flags,
                                il->Register(leftExpr.size, leftExpr.GetSourceRegister()),
                                il->Register(rightExpr.size, rightExpr.GetSourceRegister())
                            );

                            il->ReplaceExpr(srcExpr.exprIndex, newInstr);
                            updated = true;

                            // PrintILExpr(instr, 2);
                        }
                    }
                }
            }
        }

        if (updated)
            il->GenerateSSAForm();
    }
}

void SearchMLIL_ADD(BinaryView *view) {
    for (auto& func : view->GetAnalysisFunctionList()) {
        Ref<Symbol> sym = func->GetSymbol();
        // printf("%s", sym->GetFullName().c_str());

        if (sym->GetFullName() == "main") {
            Ref<MediumLevelILFunction> il = func->GetMediumLevelIL();
            if (!il)
            {
                printf("    Does not have MLIL\n\n");
                return;
            }

            // Loop through all blocks in the function
            for (auto& block : il->GetBasicBlocks()) {
                // Loop though each instruction in the block
                for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++)
                {
                    MediumLevelILInstruction instr = (*il)[instrIndex];
                    if (instr.operation == MLIL_SET_VAR) {
                        auto srcExpr = instr.GetSourceExpr<MLIL_SET_VAR>();
                        if (srcExpr.operation == MLIL_ADD) {
                            auto leftExpr = srcExpr.GetLeftExpr<MLIL_ADD>();
                            auto rightExpr = srcExpr.GetRightExpr<MLIL_ADD>();
                            if (leftExpr.operation == MLIL_VAR && rightExpr.operation == MLIL_VAR) {
                                printf("\nProcessing instruction at 0x%" PRIx64 " with operation %d at block 0x%" PRIx64 "\n", instr.address, instr.operation, block->GetStart());
                                PrintILExpr(instr, 2);
                            }
                        }
                    }
                }
            }
        }
    }
}

void ReplaceMLIL_ADDIntoMLIL_MUL(const Ref<AnalysisContext> &analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<Symbol> sym = func->GetSymbol();
    // printf("%s", sym->GetFullName().c_str());

    if (sym->GetFullName() == "main") {
        const Ref<MediumLevelILFunction> il = func->GetMediumLevelIL();
        if (!il) {
            printf("    Does not have MLIL\n\n");
            return;
        }

        bool updated = false;
        for (auto& block : il->GetBasicBlocks()) {
            // Loop though each instruction in the block
            for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++)
            {
                MediumLevelILInstruction instr = (*il)[instrIndex];
                if (instr.operation == MLIL_SET_VAR) {
                    auto srcExpr = instr.GetSourceExpr<MLIL_SET_VAR>();
                    if (srcExpr.operation == MLIL_ADD) {
                        auto leftExpr = srcExpr.GetLeftExpr<MLIL_ADD>();
                        auto rightExpr = srcExpr.GetRightExpr<MLIL_ADD>();
                        if (leftExpr.operation == MLIL_VAR && rightExpr.operation == MLIL_VAR) {
                            printf("\nProcessing instruction at 0x%" PRIx64 " with operation %d at block 0x%" PRIx64 "\n", instr.address, instr.operation, block->GetStart());
                            PrintILExpr(instr, 2);

                            const ExprId newInstr = il->AddExpr(
                                MLIL_MUL, srcExpr.size,
                                il->Var(leftExpr.size, leftExpr.GetSourceVariable()),
                                il->Var(rightExpr.size, rightExpr.GetSourceVariable())
                            );

                            il->ReplaceExpr(srcExpr.exprIndex, newInstr);
                            updated = true;
                            PrintILExpr(instr, 2);
                        }
                    }
                }
            }
        }

        if (updated)
            il->GenerateSSAForm();
    }
}

// ATM all the code is stored in main file
// I'll do some cleaning about this when necessary
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
        if (freopen("libeShard.log", "w", stdout) == nullptr) {
            perror("Failed to redirect stdout to log file");
        }

        PluginCommand::Register("eshard\\SearchLLIL_ADD", "Isolate an ADD manipulation in the main function", SearchLLIL_ADD);

        PluginCommand::Register("eshard\\All actvities", "Print all activities", [](BinaryView* view) {
            const Ref<Workflow> defaultWf = Workflow::Instance("core.function.baseAnalysis");
            for (const auto& activity : defaultWf->GetSubactivities()) {
                LogInfo("Activity: %s", activity.c_str());
            }
        });

        // Check if Test2 is working on some basics activities of core.function.baseAnalysis
        //  *   core.function.baseAnalysis, multiple entries error
        //  *   core.function.basicBlockAnalysis, work but seems to call func 3 times
        //
        //  *   core.function.generateLiftedIL, work and return "Does not have LLIL" see code of Test4 for it
        //  *   core.function.generateMediumLevelIL, work but seems to call func 2 times
        //
        //  *   core.function.basicAnalysis, not working
        //  *   core.function.intermediateAnalysis, not working
        //  *   core.function.advancedAnalysis, not working
        //
        // We can observe that LLIL is created after generateLiftedIL workflow, and we can access it in the before the generation of MLIL
        // In the workflow generateMediumLevelIL...

        Ref<Workflow> customLLILPatcherWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("LLILPatcherWorkflow");
        customLLILPatcherWorkflow->RegisterActivity(new Activity("extension.llilpatcher", &ReplaceLLIL_ADDIntoLLIL_MUL));
        customLLILPatcherWorkflow->Insert("core.function.generateMediumLevelIL", "extension.llilpatcher");
        Workflow::RegisterWorkflow(customLLILPatcherWorkflow,
            R"#({
			    "title" : "Test4",
			    "description" : "This analysis stands in as an example to demonstrate Binary Ninja's extensible analysis APIs.",
			    "targetType" : "function"
			})#"
        );

        PluginCommand::Register("eshard\\SearchMLIL_ADD", "Isolate an ADD manipulation in the main function", SearchMLIL_ADD);

        Ref<Workflow> customMLILPatcherWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("MLILPatcherWorkflow");
        customMLILPatcherWorkflow->RegisterActivity(new Activity("extension.mlilpatcher", &ReplaceMLIL_ADDIntoMLIL_MUL));
        customMLILPatcherWorkflow->Insert("core.function.generateHighLevelIL", "extension.mlilpatcher");
        Workflow::RegisterWorkflow(customMLILPatcherWorkflow,
            R"#({
			    "title" : "Test7",
			    "description" : "This analysis stands in as an example to demonstrate Binary Ninja's extensible analysis APIs.",
			    "targetType" : "function"
			})#"
        );

        return true;
    }
}