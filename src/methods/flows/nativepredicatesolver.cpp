#include "../../../include/methods/flows/nativepredicatesolver.hpp"
#include "../../../binaryninjaapi/mediumlevelilinstruction.h"

#include <cxxabi.h>  

#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <queue>
#include <condition_variable>

using namespace BinaryNinja;
using namespace Flows;

NativePredicateSolver::NativePredicateSolver()
    : IDeobfuscationMethod("Recursively patch opaque predicates in all functions until none remain", DeobfuscationCategory::Function)
{
    this->isEnabled = true;
}

void NativePredicateSolver::execute(const Ref<AnalysisContext>& analysisContext) {
    int status;
    const char* mangled = typeid(*this).name();
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    std::string className = (status == 0 && demangled) ? demangled : mangled;
    free(demangled);  // Prevent memory leak

    std::string prefix = std::string(PROJECT_NAME) + "\\" + className;

    PluginCommand::RegisterForFunction(prefix, "Recursively patch opaque predicates in all functions until none remain",
        [](BinaryView* view, Function* func) {
            auto mlil = func->GetMediumLevelIL();
            if (!mlil) {
                LogWarn("No MLIL available for function at 0x%lx", func->GetStart());
                return;
            }

            auto arch = func->GetArchitecture();
            if (!arch) {
                LogWarn("Failed to get architecture for function");
                return;
            }

            std::string funcName = func->GetSymbol()->GetFullName();

            Ref<BinaryView> viewRef = view;
            Ref<Function> funcRef = func;
            Ref<Architecture> archRef = arch;

            std::thread([viewRef, funcRef, archRef, funcName]() mutable {
                Ref<BackgroundTask> task = new BackgroundTask("Patching opaque predicates", true);
                task->SetProgressText("Processing " + funcName);
                
                auto startTime = std::chrono::high_resolution_clock::now();

                int totalPatches = 0; int pass = 1;
                const int maxPasses = 20;

                while (pass <= maxPasses) {
                    if (task->IsCancelled()) {
                        LogWarn("Operation cancelled by user");
                        break;
                    }

                    task->SetProgressText("Pass " + std::to_string(pass) + "/" + std::to_string(maxPasses) + " for " + funcName);
                    auto mlil = funcRef->GetMediumLevelIL();
                    if (!mlil) {
                        break;
                    }

                    int patchCount = 0;
                    size_t instructionCount = mlil->GetInstructionCount();
                    for (size_t i = 0; i < instructionCount; ++i) {
                        if (i % 100 == 0 && task->IsCancelled()) {
                            break;
                        }
                        
                        auto instr = mlil->GetInstruction(i);
                        if (instr.operation != MLIL_IF)
                            continue;

                        auto val = mlil->GetExprValue(instr.GetConditionExpr());
                        if (val.state == BNRegisterValueType::ConstantValue) {
                            if (val.value == 0) {
                                if (viewRef->IsNeverBranchPatchAvailable(archRef, instr.address)) {
                                    viewRef->ConvertToNop(archRef, instr.address);
                                    patchCount++;
                                }
                            }
                            else {
                                if (viewRef->IsAlwaysBranchPatchAvailable(archRef, instr.address)) {
                                    viewRef->AlwaysBranch(archRef, instr.address);
                                    patchCount++;
                                }
                            }
                        }
                    }

                    totalPatches += patchCount;
                    if (patchCount == 0)
                        break;

                    viewRef->UpdateAnalysis();
                    auto updatedFunctions = viewRef->GetAnalysisFunctionsContainingAddress(funcRef->GetStart());
                    if (!updatedFunctions.empty()) {
                        funcRef = updatedFunctions[0];
                    }
                    
                    pass++;
                }

                task->Finish();
                
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                LogInfo("[+] Completed: %d patches applied to %s in %ld ms", totalPatches, funcName.c_str(), duration.count());
            }).detach();
        }
    );
}