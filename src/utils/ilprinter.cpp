#include "../../include/utils/ilprinter.hpp"
#include <magic_enum/magic_enum.hpp>
#include <iostream>

using namespace BinaryNinja;

namespace Utils {

void PrintIndent(const size_t indent) {
    for (size_t i = 0; i < indent; i++)
        printf("    ");
}

void PrintOperation(const BNHighLevelILOperation operation) {
    if (magic_enum::enum_contains<BNHighLevelILOperation>(operation)) {
        std::cout << static_cast<int>(operation) << ": " << magic_enum::enum_name(operation) << '\n';
    } else {
        std::cout << "Unknown HLIL operation: " << static_cast<int>(operation) << '\n';
    }
}

void PrintVariable(const HighLevelILFunction* func, const Variable& var) {
    Architecture* arch = func->GetArchitecture();
    if (!arch) {
        printf("<no arch>");
        return;
    }

    if (var.type == RegisterVariableSourceType) {
        const std::string name = arch->GetRegisterName(var.index);
        if (name.empty()) {
            printf("<unnamed register>");
        } else {
            printf("%s", name.c_str());
        }
    } else {
        printf("<unknown variable type>");
    }
}

void PrintFlag(const HighLevelILFunction* func, const uint32_t flag) {
    const std::string name = func->GetArchitecture()->GetFlagName(flag);
    if (name.empty())
        printf("<no name>");
    else
        printf("%s", name.c_str());
}

void PrintILExpr(const HighLevelILInstruction& instr, size_t indent) {
    if (!instr.function) {
        PrintIndent(indent);
        printf("<null instruction function>\n");
        return;
    }

    PrintIndent(indent);
    PrintOperation(instr.operation);
    printf("\n");

    indent++;

    for (auto& operand : instr.GetOperands()) {
        switch (operand.GetType()) {
            case IntegerHighLevelOperand:
                PrintIndent(indent);
                printf("int 0x%" PRIx64 "\n", operand.GetInteger());
                break;

            case IndexHighLevelOperand:
                PrintIndent(indent);
                printf("index %" PRIdPTR "\n", operand.GetIndex());
                break;

            case ExprHighLevelOperand:
                PrintILExpr(operand.GetExpr(), indent);
                break;

            case VariableHighLevelOperand:
                PrintIndent(indent);
                printf("var ");
                PrintVariable(instr.function, operand.GetVariable());
                printf("\n");
                break;

            case SSAVariableHighLevelOperand:
                PrintIndent(indent);
                printf("ssa var ");
                PrintVariable(instr.function, operand.GetSSAVariable().var);
                printf("#%" PRIdPTR "\n", operand.GetSSAVariable().version);
                break;

            case IndexListHighLevelOperand:
                PrintIndent(indent);
                printf("index list ");
                for (auto i : operand.GetIndexList())
                    printf("%" PRIdPTR " ", i);
                printf("\n");
                break;

            case SSAVariableListHighLevelOperand:
                PrintIndent(indent);
                printf("ssa var list ");
                for (auto& v : operand.GetSSAVariableList()) {
                    PrintVariable(instr.function, v.var);
                    printf("#%" PRIdPTR " ", v.version);
                }
                printf("\n");
                break;

            default:
                PrintIndent(indent);
                printf("<invalid or unhandled HLIL operand>\n");
                break;
        }
    }
}

} // namespace Utils
