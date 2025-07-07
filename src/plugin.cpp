#include <fstream>
#include <iostream>
#include <streambuf>
#include "../include/plugin.h"

using namespace BinaryNinja;

#ifndef PROJECT_NAME
    #define PROJECT_NAME "nameless-project"
#endif

namespace {
    void executeEnabledMethods(const std::shared_ptr<Deobfuscator>& deobfuscator, DeobfuscationCategory category, const Ref<AnalysisContext>& context)
    {
        for (const auto& method : deobfuscator->getMethodsByCategory(category)) {
            if (!method->isEnabled)
                continue;
            method->execute(context);
        }
    }
}

extern "C"
{
    BINARYNINJAPLUGIN uint32_t CorePluginABIVersion() { return BN_CURRENT_CORE_ABI_VERSION; }

    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        auto deobfuscator = std::make_shared<Deobfuscator>();
        deobfuscator->init();

        auto customWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("core.function.deobfuscation");
        const std::string activityName = std::string("extension.") + PROJECT_NAME;

        customWorkflow->RegisterActivity(new Activity(activityName, [deobfuscator](const Ref<AnalysisContext>& context) {
            // TODO: Constant folding and renaming before the workflow to simplify analysis types
            executeEnabledMethods(deobfuscator, DeobfuscationCategory::Workflow, context);
        }));

        customWorkflow->Insert("core.function.generateHighLevelIL", activityName);
        Workflow::RegisterWorkflow(customWorkflow,
            R"#({
                "title" : "Deobfuscation",
                "description" : "Deobfuscates functions using various methods.",
                "targetType" : "function"
            })#"
        );


        // executeEnabledMethods(deobfuscator, DeobfuscationCategory::Function, context);
        return true;
    }
}
