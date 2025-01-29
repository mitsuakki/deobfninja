#ifndef MLILPARSER_H
#define MLILPARSER_H

#include <cinttypes>
#include <string>
#include <vector>
#include "../../binaryninjaapi/binaryninjacore.h"
#include "../../binaryninjaapi/mediumlevelilinstruction.h"

void PrintMLILOperation(BNMediumLevelILOperation operation);
void PrintVariable(BinaryNinja::MediumLevelILFunction* func, const BinaryNinja::Variable& var);
void PrintMLILExpr(const BinaryNinja::MediumLevelILInstruction& instr, size_t indent);
void AnalyzeMLILFunction(const BinaryNinja::Ref<BinaryNinja::Function>& func);

#endif // MLILPARSER_H