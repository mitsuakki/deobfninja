#ifndef METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP
#define METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP

#pragma once

#include <string>
#include "../deobfuscator.hpp"

using namespace BinaryNinja;

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
    /**
     * @brief Executes the MBASimplifier deobfuscation process.
     * This method simplifies the MBAS instructions in the binary.
     */
    void execute(const Ref<AnalysisContext>& analysisContext) override;

    /**
     * @brief Returns the name of the MBASimplifier method.
     * @return The name of the method.
     */
    const std::string name() const override
    {
        return "MBASimplifier";
    }

    /**
     * @brief Returns a description of the MBASimplifier method.
     * @return The description of the method.
     */ 
    const std::string description() const override
    {
        return "Simplifies MBAs instructions in the binary.";
    }

    /**
     * @brief Indicates whether the MBASimplifier method should be registered as a workflow.
     * 
     * This method can be overridden to allow dynamic filtering of methods in the workflow.
     * 
     * @return True if the method should be registered, false otherwise.
     */
    bool isRegisteredAsWorkflow() const override
    {
        return true;
    }
};

#endif // METHODS_INSTRUCTIONS_MBA_SIMPLIFIER_HPP