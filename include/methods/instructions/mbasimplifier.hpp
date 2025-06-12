#ifndef METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP
#define METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP

#pragma once

#include "../../deobfuscator.hpp"

/**
 * @brief The MBASimplifier class implements the IDeobfuscationMethod interface
 * to simplify MBA instructions in a binary.
 * 
 * This class is part of the deobfuscation methods used to analyze and simplify
 * obfuscated binaries, specifically targeting MBA (Multi-Byte Arithmetic) instructions.
 */
class MBASimplifier : public IDeobfuscationMethod
{
public:
    MBASimplifier();

    /**
     * @brief Executes the MBASimplifier deobfuscation process.
     * This method simplifies the MBAS instructions in the binary.
     */
    void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext) override;
};

#endif // METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP