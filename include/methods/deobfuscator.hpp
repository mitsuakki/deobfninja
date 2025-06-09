#ifndef DEOBFUSCATOR_HPP
#define DEOBFUSCATOR_HPP

#pragma once

#include <vector>
#include <memory>
#include <string>

#include "../../binaryninjaapi/binaryninjaapi.h"

using namespace BinaryNinja;

/**
 * @brief Interface for deobfuscation methods.
 *
 * This abstract class defines the interface that all deobfuscation method implementations must follow.
 * It provides a contract for executing the deobfuscation process and retrieving the method's name.
 */
class IDeobfuscationMethod {
public:
    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup of derived classes.
     */
    virtual ~IDeobfuscationMethod() = default;

    /**
     * @brief Executes the deobfuscation process.
     *
     * This pure virtual function must be implemented by derived classes to perform
     * the specific deobfuscation logic.
     */
   virtual void execute(const Ref<AnalysisContext>& analysisContext) = 0;

    /**
     * @brief Retrieves the name of the deobfuscation method.
     *
     * @return A constant character pointer to the name of the method.
     */
    virtual const std::string name() const = 0;

    /**
     * @brief Retrieves a description of the deobfuscation method.
     *
     * This function provides a brief description of what the method does.
     *
     * @return A constant character pointer to the description of the method.
     */
    virtual const std::string description() const = 0;

    /**
     * @brief Indicates whether the method should be used in the workflow.
     * 
     * This method can be overridden by implementations to allow dynamic filtering.
     */
    virtual bool isRegisteredAsWorkflow() const { return true; }
};

/**
 * @class Deobfuscator
 * @brief Manages and applies deobfuscation methods.
 *
 * The Deobfuscator class allows registration and management of multiple
 * deobfuscation methods, which can be used to process and deobfuscate code or data.
 *
 * @note This class takes ownership of the registered deobfuscation methods.
 */
class Deobfuscator {
public:
    /**
     * @brief Registers a new deobfuscation method.
     *
     * Takes ownership of the provided IDeobfuscationMethod instance and adds it to the list
     * of available deobfuscation methods. The method will be managed and used by the deobfuscator.
     *
     * @param method A unique pointer to an IDeobfuscationMethod to be registered.
     */
    void registerMethod(std::unique_ptr<IDeobfuscationMethod> method);

    /**
     * @brief Registers all available deobfuscation methods.
     *
     * This function initializes and registers all supported deobfuscation
     * algorithms or techniques, making them available for use within the
     * deobfuscator system.
     */
    void registerAll();

    /**
     * @brief Accessor to all registered methods.
     *
     * @return A vector of raw pointers to the methods.
     */
    std::vector<IDeobfuscationMethod*> getMethods() const;

private:
    // A vector to hold all registered deobfuscation methods.
    // This vector uses unique_ptr to manage the lifetime of the methods automatically.
    std::vector<std::unique_ptr<IDeobfuscationMethod>> methods;
};

#endif // DEOBFUSCATOR_HPP