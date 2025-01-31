#include "searcher.h"

#include <string>
#include <cstdio>
#include <cinttypes>

#include "binaryninjaapi.h"
#include "lowlevelilinstruction.h"
#include "mediumlevelilinstruction.h"

using namespace BinaryNinja;

void ILPatternSearcher::SearchLLIL(const Ref<AnalysisContext>& analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<LowLevelILFunction> il = func->GetLowLevelIL();

    if (!il) {
        printf("\tDoes not have LLIL\n\n");
        return;
    }

    for (Ref<BasicBlock>& block : il->GetBasicBlocks()) {
        for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++) {
            LowLevelILInstruction instr = (*il)[instrIndex];
            if (MatchPattern(instr)) {
                printf("\nProcessing instruction at 0x%" PRIx64 " with operation %d at block 0x%" PRIx64 "\n", instr.address, instr.operation, block->GetStart());
                ApplyChanges(instr, il);
            }
        }
    }
}

void ILPatternSearcher::SearchMLIL(const Ref<AnalysisContext>& analysisContext) {
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<MediumLevelILFunction> il = func->GetMediumLevelIL();

    if (!il) {
        printf("\tDoes not have MLIL\n\n");
        return;
    }

    for (Ref<BasicBlock>& block : il->GetBasicBlocks()) {
        for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++) {
            MediumLevelILInstruction instr = (*il)[instrIndex];
            if (MatchPattern(instr)) {
                printf("\nProcessing instruction at 0x%" PRIx64 " with operation %d at block 0x%" PRIx64 "\n", instr.address, instr.operation, block->GetStart());
                ApplyChanges(instr, il);
            }
        }
    }
}