#include <fstream>
#include <iostream>
#include <streambuf>
#include "../include/plugin.h"
using namespace BinaryNinja;

// Should be defined in CMakeLists.txt as a build macro
#ifndef PROJECT_NAME
    #define PROJECT_NAME "nameless-project"
#endif

extern "C"
{
    BINARYNINJAPLUGIN uint32_t CorePluginABIVersion() { return BN_CURRENT_CORE_ABI_VERSION; }
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        std::shared_ptr<Deobfuscator> deobfuscator = std::make_shared<Deobfuscator>();
        deobfuscator->init();

        Ref<Workflow> customWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("core.function.deobfuscation");
        std::string activityName = std::string("extension.") + PROJECT_NAME;

        customWorkflow->RegisterActivity(new Activity(activityName, [deobfuscator](const Ref<AnalysisContext>& context) {
            // ToDO : A constant folding and renaming before all workflow to simplfy differents types of analysis
            
            for (const auto& method : deobfuscator->getMethodsByCategory(DeobfuscationCategory::Workflow)) {
                if (!method->isEnabled) continue;
                method->execute(context);
            }
        }));
        
        customWorkflow->Insert("core.function.generateHighLevelIL", activityName);
        Workflow::RegisterWorkflow(customWorkflow,
            R"#({
                "title" : "Deobfuscation",
                "description" : "Deobfuscates functions using various methods.",
                "targetType" : "function"
            })#"
        );

        return true;
    }
}