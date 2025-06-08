#pragma once

#include "../searcher.h"
#include "lowlevelilinstruction.h"

using namespace BinaryNinja;

class LLIL_ADD_Searcher : public ILPatternSearcher {
protected:
    bool MatchPattern(MediumLevelILInstruction &instr) override { return false; }
    bool MatchPattern(LowLevelILInstruction& instr) override {
        if (instr.operation == LLIL_SET_REG) {
            LowLevelILInstruction srcExpr = instr.GetSourceExpr<LLIL_SET_REG>();

            if (srcExpr.operation == LLIL_ADD) {
                LowLevelILInstruction leftExpr = srcExpr.GetLeftExpr<LLIL_ADD>();
                LowLevelILInstruction rightExpr = srcExpr.GetRightExpr<LLIL_ADD>();

                if (leftExpr.operation == LLIL_REG && rightExpr.operation == LLIL_REG) {
                    return true;
                }
            }
        }
        return false;
    }

    void ApplyChanges(MediumLevelILInstruction &instr, Ref<MediumLevelILFunction> il) override { return; }
    void ApplyChanges(LowLevelILInstruction &instr, Ref<LowLevelILFunction> il) override {
        const LowLevelILInstruction srcExpr = instr.GetSourceExpr<LLIL_SET_REG>();
        const LowLevelILInstruction leftExpr = srcExpr.GetLeftExpr<LLIL_ADD>();
        const LowLevelILInstruction rightExpr = srcExpr.GetRightExpr<LLIL_ADD>();

        const ExprId newInstr = il->AddExpr(
            LLIL_MUL, srcExpr.size, srcExpr.flags,
            il->Register(leftExpr.size, leftExpr.GetSourceRegister()),
            il->Register(rightExpr.size, rightExpr.GetSourceRegister())
        );

        il->ReplaceExpr(srcExpr.exprIndex, newInstr);
        il->GenerateSSAForm();
    };
};