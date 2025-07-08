#ifndef METHODS_HPP
#define METHODS_HPP

#include <string>
#include "../../binaryninjaapi/binaryninjaapi.h"

enum class DeobfuscationCategory {
    Workflow,
    Function
};

class IDeobfuscationMethod {
public:
    std::string description;
    DeobfuscationCategory category;

    bool isEnabled = false;
    IDeobfuscationMethod(std::string description, DeobfuscationCategory category)
        : description(std::move(description)), category(category) {}

    virtual ~IDeobfuscationMethod() = default;
    virtual void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext = nullptr) = 0;


    const std::string getDescription() const { return this->description; }
    const DeobfuscationCategory getCategory() const { return this->category; }
};

#endif // METHODS_HPP
