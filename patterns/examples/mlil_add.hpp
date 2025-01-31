#pragma once

#include "../searcher.h"
#include "mediumlevelilinstruction.h"

using namespace BinaryNinja;

class MLIL_ADD_Searcher : public ILPatternSearcher {
protected:
    bool MatchPattern(LowLevelILInstruction& instr) override { return false; }
    bool MatchPattern(MediumLevelILInstruction& instr) override {
        if (instr.operation == MLIL_SET_VAR) {
            MediumLevelILInstruction srcExpr = instr.GetSourceExpr<MLIL_SET_VAR>();

            if (srcExpr.operation == MLIL_ADD) {
                MediumLevelILInstruction leftExpr = srcExpr.GetLeftExpr<MLIL_ADD>();
                MediumLevelILInstruction rightExpr = srcExpr.GetRightExpr<MLIL_ADD>();

                if (leftExpr.operation == MLIL_VAR && rightExpr.operation == MLIL_VAR) {
                    return true;
                }
            }
        }
        return false;
    }

    void ApplyChanges(LowLevelILInstruction &instr, Ref<LowLevelILFunction> il) override { return; }
    void ApplyChanges(MediumLevelILInstruction &instr, Ref<MediumLevelILFunction> il) override {
        const MediumLevelILInstruction srcExpr = instr.GetSourceExpr<MLIL_SET_VAR>();
        const MediumLevelILInstruction leftExpr = srcExpr.GetLeftExpr<MLIL_ADD>();
        const MediumLevelILInstruction rightExpr = srcExpr.GetRightExpr<MLIL_ADD>();

        const ExprId newInstr = il->AddExpr(
            MLIL_MUL, srcExpr.size,
            il->Var(leftExpr.size, leftExpr.GetSourceVariable()),
            il->Var(rightExpr.size, rightExpr.GetSourceVariable())
        );

        il->ReplaceExpr(srcExpr.exprIndex, newInstr);
        il->GenerateSSAForm();
    }
};
