#include "../../include/utils/ilprinter.hpp"
#include <magic_enum/magic_enum.hpp>
#include <iostream>

using namespace BinaryNinja;

namespace Utils {

void PrintIndent(const size_t indent) {
    for (size_t i = 0; i < indent; i++)
        printf("    ");
}

void PrintOperation(const BNLowLevelILOperation operation) {
    if (magic_enum::enum_contains<BNLowLevelILOperation>(operation)) {
        std::cout << static_cast<int>(operation) << ": " << magic_enum::enum_name(operation) << '\n';
    } else {
        std::cout << "Unknown HLIL operation: " << static_cast<int>(operation) << '\n';
    }
}

void PrintFlagCondition(const BNLowLevelILFlagCondition cond) {
    if (magic_enum::enum_contains<BNLowLevelILFlagCondition>(cond)) {
        std::cout << static_cast<int>(cond) << ": " << magic_enum::enum_name(cond);
    } else {
        std::cout << "Unknown flag condition: " << static_cast<int>(cond);
    }
}

void PrintRegister(const LowLevelILFunction* func, const uint32_t reg) {
    if (LLIL_REG_IS_TEMP(reg))
        printf("temp%d", LLIL_GET_TEMP_REG_INDEX(reg));
    else {
        const std::string name = func->GetArchitecture()->GetRegisterName(reg);
        if (name.empty())
            printf("<no name>");
        else
            printf("%s", name.c_str());
    }
}

void PrintFlag(const LowLevelILFunction* func, const uint32_t flag) {
    if (LLIL_REG_IS_TEMP(flag))
        printf("cond:%d", LLIL_GET_TEMP_REG_INDEX(flag));
    else {
        const std::string name = func->GetArchitecture()->GetFlagName(flag);
        if (name.empty())
            printf("<no name>");
        else
            printf("%s", name.c_str());
    }
}

void PrintILExpr(const LowLevelILInstruction& instr, size_t indent) {
    PrintIndent(indent);
    PrintOperation(instr.operation);
    printf("\n");

    indent++;

    for (auto& operand : instr.GetOperands()) {
        switch (operand.GetType()) {
            case IntegerLowLevelOperand:
                PrintIndent(indent);
                printf("int 0x%" PRIx64 "\n", operand.GetInteger());
                break;

            case IndexLowLevelOperand:
                PrintIndent(indent);
                printf("index %" PRIdPTR "\n", operand.GetIndex());
                break;

            case ExprLowLevelOperand:
                PrintILExpr(operand.GetExpr(), indent);
                break;

            case RegisterLowLevelOperand:
                PrintIndent(indent);
                printf("reg ");
                PrintRegister(instr.function, operand.GetRegister());
                printf("\n");
                break;

            case FlagLowLevelOperand:
                PrintIndent(indent);
                printf("flag ");
                PrintFlag(instr.function, operand.GetFlag());
                printf("\n");
                break;

            case FlagConditionLowLevelOperand:
                PrintIndent(indent);
                printf("flag condition ");
                PrintFlagCondition(operand.GetFlagCondition());
                printf("\n");
                break;

            case SSARegisterLowLevelOperand:
                PrintIndent(indent);
                printf("ssa reg ");
                PrintRegister(instr.function, operand.GetSSARegister().reg);
                printf("#%" PRIdPTR "\n", operand.GetSSARegister().version);
                break;

            case SSAFlagLowLevelOperand:
                PrintIndent(indent);
                printf("ssa flag ");
                PrintFlag(instr.function, operand.GetSSAFlag().flag);
                printf("#%" PRIdPTR " ", operand.GetSSAFlag().version);
                break;

            case IndexListLowLevelOperand:
                PrintIndent(indent);
                printf("index list ");
                for (auto i : operand.GetIndexList())
                    printf("%" PRIdPTR " ", i);
                printf("\n");
                break;

            case SSARegisterListLowLevelOperand:
                PrintIndent(indent);
                printf("ssa reg list ");
                for (auto i : operand.GetSSARegisterList()) {
                    PrintRegister(instr.function, i.reg);
                    printf("#%" PRIdPTR " ", i.version);
                }
                printf("\n");
                break;

            case SSAFlagListLowLevelOperand:
                PrintIndent(indent);
                printf("ssa reg list ");
                for (auto i : operand.GetSSAFlagList()) {
                    PrintFlag(instr.function, i.flag);
                    printf("#%" PRIdPTR " ", i.version);
                }
                printf("\n");
                break;

            default:
                PrintIndent(indent);
                printf("<invalid operand>\n");
                break;
        }
    }
}

} // namespace Utils