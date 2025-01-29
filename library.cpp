#include <cstdio>
#include <cinttypes>

#include "library.h"

#include "binaryninjaapi.h"
#include "lowlevelilinstruction.h"

#include "recipes/parser/ilparser.h"

using namespace BinaryNinja;
using namespace std;

void Test(BinaryView *bv) {
    for (auto& func : bv->GetAnalysisFunctionList()) {
        AnalyzeFunction(func);
    }
}

void Test2(BinaryView *bv) {
    for (auto& func : bv->GetAnalysisFunctionList()) {
        Ref<Symbol> sym = func->GetSymbol();
        printf("%s", sym->GetFullName().c_str());

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

void Test4(const Ref<AnalysisContext> &analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<Symbol> sym = func->GetSymbol();
    printf("%s", sym->GetFullName().c_str());

    if (sym->GetFullName() == "main") {
        const Ref<LowLevelILFunction> il = func->GetLowLevelIL();
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

                            const ExprId newInstr = il->AddExpr(
                                LLIL_MUL, srcExpr.size, srcExpr.flags,
                                il->Register(leftExpr.size, leftExpr.GetSourceRegister()),
                                il->Register(rightExpr.size, rightExpr.GetSourceRegister())
                            );

                            il->ReplaceExpr(srcExpr.exprIndex, newInstr);
                            PrintILExpr(instr, 2);
                        }
                    }
                }
            }
        }
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
        PluginCommand::Register("eshard\\Test", "Print LLIL of all functions", Test);
        PluginCommand::Register("eshard\\Test2", "Isolate an ADD manipulation in the main function", Test2);

        PluginCommand::Register("eshard\\Test3", "Print all activities", [](BinaryView* view) {
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

        Ref<Workflow> customPatcherWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("PatcherWorkflow");
        customPatcherWorkflow->RegisterActivity(new Activity("extension.patcher", &Test4));
        customPatcherWorkflow->Insert("core.function.generateMediumLevelIL", "extension.patcher");
        Workflow::RegisterWorkflow(customPatcherWorkflow,
            R"#({
			    "title" : "Test4",
			    "description" : "This analysis stands in as an example to demonstrate Binary Ninja's extensible analysis APIs.",
			    "targetType" : "function"
			})#"
        );

        return true;
    }
}