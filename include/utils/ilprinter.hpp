#ifndef UTILS_ILPRINTER_HPP
#define UTILS_ILPRINTER_HPP

#pragma once

#include <cinttypes>
#include <cstdio>
#include <string>
#include <vector>

#include "../../binaryninjaapi/binaryninjaapi.h"
#include "../../binaryninjaapi/lowlevelilinstruction.h"

namespace Utils {
    #define ENUM_PRINTER(op) \
    case op:              \
        printf(#op);      \
        break;

    void PrintIndent(const size_t indent);

    void PrintOperation(BNLowLevelILOperation operation);
    void PrintFlagCondition(BNLowLevelILFlagCondition cond);

    void PrintRegister(const BinaryNinja::LowLevelILFunction* func, uint32_t reg);
    void PrintFlag(const BinaryNinja::LowLevelILFunction* func, uint32_t flag);
    void PrintILExpr(const BinaryNinja::LowLevelILInstruction& instr, size_t indent = 0);
}

#endif // UTILS_ILPRINTER_HPP