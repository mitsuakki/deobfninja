#ifndef DEOBFUSCATOR_HPP
#define DEOBFUSCATOR_HPP

#pragma once

#include <vector>
#include "methods/methods.hpp"

/**
 * @class Deobfuscator
 * @brief Manages the registration and retrieval of deobfuscation methods.
 *
 * This class allows registering multiple deobfuscation methods and retrieving them,
 * optionally filtered by category.
 */
class Deobfuscator {
public:
    /**
     * @brief Initializes the deobfuscator by registering all available methods.
     *
     * Typically used as an entry point to register built-in deobfuscation methods.
     * Acts similarly to an "init" or "registerAllMethods" function.
     */
    void init();

    /**
     * @brief Registers a deobfuscation method.
     * @param method Pointer to the method to register.
     * @return true if registration succeeded (i.e., method was added), false otherwise.
     */
    bool registerMethod(IDeobfuscationMethod* method);

    /**
     * @brief Retrieves all registered methods.
     * @see DeobfuscatorTest::GetMethods
     * @return A copy of the internal vector of registered methods.
     */
    std::vector<IDeobfuscationMethod*> getMethods() const {
        return methods;
    }

    /**
     * @brief Retrieves all registered methods for a specific category.
     * @param category The category to filter methods by.
     * @see DeobfuscatorTest::GetMethodsByCategory
     * @return A vector of methods that belong to the given category.
     */
    std::vector<IDeobfuscationMethod*> getMethodsByCategory(DeobfuscationCategory category) const {
        std::vector<IDeobfuscationMethod*> filtered;

        for (auto* method : methods) {
            if (method->category == category) {
                filtered.push_back(method);
            }
        }

        return filtered;
    }

private:
    /**
     * @brief Vector holding all registered deobfuscation methods.
     * 
     * Note: Ownership is not managed here; make sure methods are properly allocated/freed externally.
     */
    std::vector<IDeobfuscationMethod*> methods;
};

#endif // DEOBFUSCATOR_HPP
