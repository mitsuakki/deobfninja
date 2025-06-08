#ifndef SEARCHER_H
#define SEARCHER_H

#include "binaryninjaapi.h"

// #include "../binaryninjaapi/examples/llil_parser/src/llil_parser.cpp"
// #include "../binaryninjaapi/examples/mlil_parser/src/mlil_parser.cpp"

class ILPatternSearcher {
protected:
    virtual bool MatchPattern(BinaryNinja::LowLevelILInstruction& instr) = 0;
    virtual bool MatchPattern(BinaryNinja::MediumLevelILInstruction& instr) = 0;

    virtual void ApplyChanges(BinaryNinja::LowLevelILInstruction& instr, BinaryNinja::Ref<BinaryNinja::LowLevelILFunction> il) = 0;
    virtual void ApplyChanges(BinaryNinja::MediumLevelILInstruction& instr, BinaryNinja::Ref<BinaryNinja::MediumLevelILFunction> il) = 0;

public:
    virtual ~ILPatternSearcher() = default;

    void SearchLLIL(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext);
    void SearchMLIL(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext);
};

#endif // SEARCHER_H
