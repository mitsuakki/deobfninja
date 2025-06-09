#include "../../../include/methods/instructions/mbasimplifier.hpp"
#include "../../../binaryninjaapi/lowlevelilinstruction.h"

using namespace BinaryNinja;

void MBASimplifier::execute(const Ref<AnalysisContext>& analysisContext)
{
    const Ref<Function> func = analysisContext->GetFunction();
    const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
    if (llil)
    {
        for (Ref<BasicBlock>& block : llil->GetBasicBlocks())
        {
            for (size_t instrIndex = block->GetStart(); instrIndex < block->GetEnd(); instrIndex++)
            {
                LowLevelILInstruction instr = (*llil)[instrIndex];
                if (instr.operation == LLIL_SET_REG)
                {
                    LowLevelILInstruction srcExpr = instr.GetSourceExpr<LLIL_SET_REG>();
                    if (srcExpr.operation == LLIL_ADD)
                    {
                        LowLevelILInstruction leftExpr = srcExpr.GetLeftExpr<LLIL_ADD>();
                        LowLevelILInstruction rightExpr = srcExpr.GetRightExpr<LLIL_ADD>();

                        if (leftExpr.operation == LLIL_REG && rightExpr.operation == LLIL_REG)
                        {
                            const ExprId newInstr = llil->AddExpr(
                                LLIL_MUL, srcExpr.size, srcExpr.flags,
                                llil->Register(leftExpr.size, leftExpr.GetSourceRegister()),
                                llil->Register(rightExpr.size, rightExpr.GetSourceRegister())
                            );
                            
                            llil->ReplaceExpr(srcExpr.exprIndex, newInstr);
                            llil->GenerateSSAForm();
                        }
                    }
                }
            }
        }
    }
}