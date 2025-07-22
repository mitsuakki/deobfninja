#include <fstream>
#include <iostream>
#include <streambuf>
#include "../include/plugin.h"

using namespace BinaryNinja;

// C macros to avoid compilation problem with CMake macros
#ifndef PROJECT_NAME
    #define PROJECT_NAME "nameless-project"
#endif

namespace {
    /**
     * @brief Execute function of a certains types of deobfucation methods registered in the given deobfuscator instance.
     *  Check is the method is enabled before executing it.
     * 
     * @param deobfuscator The deobfuscator where all methods are registered.
     * @param category     The type of methods you want to execute  [Function, Workflow, etc...] (cf. DeobfuscationCategory enumeration)
     * @param context      The binary view analysis context in which the method will be executed. (can be null)
     */
    void executeEnabledMethods(const std::shared_ptr<Deobfuscator>& deobfuscator, DeobfuscationCategory category, const Ref<AnalysisContext>& context = nullptr)
    {
        for (const auto& method : deobfuscator->getMethodsByCategory(category)) {
            if (!method->isEnabled)
                continue;
            method->execute(context);
        }
    }
}

// Extern C to match C symbols with C++ symbols and be detectable by binja plugins system
extern "C"
{
    // Used to check if the API version you are using matches with your Binary Ninja's installation
    BINARYNINJAPLUGIN uint32_t CorePluginABIVersion() { return BN_CURRENT_CORE_ABI_VERSION; }

    // Main entry of the plugin
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        // Init the deobfuscator to have access to all registered deobfuscation methods
        auto deobfuscator = std::make_shared<Deobfuscator>();
        deobfuscator->init();

        // Copy the base workflow
        auto customWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("core.function.deobfuscation");
        const std::string activityName = std::string("extension.") + PROJECT_NAME;

        customWorkflow->RegisterActivity(new Activity(activityName, [deobfuscator](const Ref<AnalysisContext>& context) {
            // TODO: Constant folding and renaming before the workflow to simplify analysis types
            executeEnabledMethods(deobfuscator, DeobfuscationCategory::Workflow, context);
        }));

        // Insertion of the new workflow
        customWorkflow->Insert("core.function.generateHighLevelIL", activityName);
        Workflow::RegisterWorkflow(customWorkflow,
            R"#({
                "title" : "Deobfuscation",
                "description" : "Deobfuscates functions using various methods.",
                "targetType" : "function"
            })#"
        );

        // Execution of enabled methods of 'Function' category
        // This category is mainly about registering methods and/or plugin commands.
        executeEnabledMethods(deobfuscator, DeobfuscationCategory::Function);
        return true;
    }
}
