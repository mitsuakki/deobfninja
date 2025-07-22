#ifndef METHODS_HPP
#define METHODS_HPP

#include <string>
#include "../../binaryninjaapi/binaryninjaapi.h"

/**
 * @enum DeobfuscationCategory
 * @brief Represents the category to which a deobfuscation method belongs.
 */
enum class DeobfuscationCategory {
    Workflow,  ///< Applies to the overall analysis workflow.
    Function   ///< Applies to a specific function.
};

/**
 * @class IDeobfuscationMethod
 * @brief Interface for implementing a deobfuscation method.
 *
 * This abstract base class defines the contract for all deobfuscation methods,
 * including their description, category, and the execute() interface.
 */
class IDeobfuscationMethod {
public:
    std::string description;                 ///< Description of the method.
    DeobfuscationCategory category;          ///< Category of the method.
    bool isEnabled = false;                  ///< Flag indicating whether the method is enabled.

    /**
     * @brief Constructs a new IDeobfuscationMethod.
     * @param description A textual description of the method.
     * @param category The category of the method.
     */
    IDeobfuscationMethod(std::string description, DeobfuscationCategory category)
        : description(std::move(description)), category(category) {}

    /// Virtual destructor.
    virtual ~IDeobfuscationMethod() = default;

    /**
     * @brief Executes the deobfuscation method.
     * @param analysisContext (Optional) The Binary Ninja analysis context in which to execute.
     */
    virtual void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext = nullptr) = 0;

    /**
     * @brief Returns the description of the method.
     * @return Method description.
     */
    std::string getDescription() const { return description; }

    /**
     * @brief Returns the category of the method.
     * @return Method category.
     */
    DeobfuscationCategory getCategory() const { return category; }
};

#endif // METHODS_HPP