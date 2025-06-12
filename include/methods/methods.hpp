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
    {
        BinaryNinja::LogInfo("IDeobfuscationMethod created: %s (Derived class: %s)", this->description.c_str(), typeid(*this).name());
    }

    virtual ~IDeobfuscationMethod() = default;
    virtual void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>& analysisContext) = 0;

    const std::string getDescription() const {
        return this->description;
    }

    const DeobfuscationCategory getCategory() const {
        return this->category;
    }
};