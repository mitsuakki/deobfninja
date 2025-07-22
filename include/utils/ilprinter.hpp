#ifndef UTILS_ILPRINTER_HPP
#define UTILS_ILPRINTER_HPP

#pragma once

#include <cinttypes>
#include <cstdio>
#include <string>
#include <vector>

#include "../../binaryninjaapi/binaryninjaapi.h"
#include "../../binaryninjaapi/lowlevelilinstruction.h"

/**
 * @namespace Utils
 * @brief Provides utility functions for displaying LowLevelIL instructions in Binary Ninja.
 *
 * This namespace includes functions to print operations, registers, flag conditions,
 * and IL expressions used in Binary Ninja's LowLevelIL functions.
 */
namespace Utils {

    /**
     * @brief Utility macro to print the name of an enum constant.
     * @param op The name of the enum constant.
     * 
     * Typically used in a switch statement to print the enum as a string.
     */
    #define ENUM_PRINTER(op) \
    case op:              \
        printf(#op);      \
        break;

    /**
     * @brief Prints an indentation using spaces.
     * @param indent The number of spaces to print.
     */
    void PrintIndent(const size_t indent);

    /**
     * @brief Prints the name of a LowLevelIL operation.
     * @param operation The operation to print.
     */
    void PrintOperation(BNLowLevelILOperation operation);

    /**
     * @brief Prints the name of a flag condition used in conditional instructions.
     * @param cond The flag condition to print.
     */
    void PrintFlagCondition(BNLowLevelILFlagCondition cond);

    /**
     * @brief Prints the name of a register given its identifier.
     * @param func The LowLevelIL function context containing the register.
     * @param reg The register ID to print.
     */
    void PrintRegister(const BinaryNinja::LowLevelILFunction* func, uint32_t reg);

    /**
     * @brief Prints the name of a flag given its identifier.
     * @param func The LowLevelIL function context containing the flag.
     * @param flag The flag ID to print.
     */
    void PrintFlag(const BinaryNinja::LowLevelILFunction* func, uint32_t flag);

    /**
     * @brief Recursively prints a LowLevelIL instruction with indentation.
     * @param instr The instruction to print.
     * @param indent The base indentation level to use (default is 0).
     */
    void PrintILExpr(const BinaryNinja::LowLevelILInstruction& instr, size_t indent = 0);
}


#endif // UTILS_ILPRINTER_HPP