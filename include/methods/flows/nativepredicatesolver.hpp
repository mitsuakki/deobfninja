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
        
        // Method to patch a single function (useful for testing or targeted patching)
        int patchSingleFunction(const BinaryNinja::Ref<BinaryNinja::BinaryView>& view, 
                               const BinaryNinja::Ref<BinaryNinja::Function>& func,
                               int maxPasses = 10);

    private:
        // Helper method to process a batch of functions in a worker thread
        void processFunctionBatch(BinaryNinja::Ref<BinaryNinja::BinaryView> viewRef, 
                                 const std::vector<BinaryNinja::Ref<BinaryNinja::Function>>& funcBatch,
                                 int maxPassesPerFunction,
                                 std::atomic<int>& patchCount,
                                 std::atomic<bool>& shouldCancel,
                                 std::mutex& updateMutex,
                                 std::atomic<size_t>& processedFunctions);
    };
}

#endif // METHODS_FLOWS_NATIVE_PREDICATE_SOLVER_HPP