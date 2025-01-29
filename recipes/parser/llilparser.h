#ifndef LLILPARSER_H
#define LLILPARSER_H

#include <cinttypes>

#include "../../binaryninjaapi/binaryninjacore.h"
#include "../../binaryninjaapi/lowlevelilinstruction.h"

void PrintIndent(const size_t indent);
void PrintOperation(const BNLowLevelILOperation operation);
void PrintFlagCondition(const BNLowLevelILFlagCondition cond);
void PrintRegister(const BinaryNinja::LowLevelILFunction* func, const uint32_t reg);
void PrintFlag(const BinaryNinja::LowLevelILFunction* func, const uint32_t flag);
void PrintILExpr(const BinaryNinja::LowLevelILInstruction& instr, size_t indent);
void AnalyzeLLILFunction(const BinaryNinja::Ref<BinaryNinja::Function>& func);

#endif // LLILPARSER_H