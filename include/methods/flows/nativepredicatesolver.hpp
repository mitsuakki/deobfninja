#ifndef METHODS_FLOWS_NATIVE_PREDICATE_SOLVER_HPP
#define METHODS_FLOWS_NATIVE_PREDICATE_SOLVER_HPP

#pragma once

#include "binaryninjaapi.h"
#include "../../deobfuscator.hpp"

#include <atomic>
#include <mutex>
#include <vector>

namespace Flows {
    struct PatchInfo {
        BinaryNinja::Ref<BinaryNinja::Architecture> arch;
        uint64_t address;
        bool alwaysBranch;
    };

    class NativePredicateSolver : public IDeobfuscationMethod {
    public:
        NativePredicateSolver();
        void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext) override;
    };
}

#endif // METHODS_FLOWS_NATIVE_PREDICATE_SOLVER_HPP