#include "../include/plugin.h"
#include "binaryninjaapi.h"

using namespace BinaryNinja;
using namespace std;

constexpr const char* PROJECT_NAME = "deobfninja";

extern "C"
{
    BINARYNINJAPLUGIN uint32_t CorePluginABIVersion() { return BN_CURRENT_CORE_ABI_VERSION; }
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        Deobfuscator deobfuscator;
        deobfuscator.registerAll();

        auto customWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("core.function.deobfuscation");
        std::string activityName = std::string("extension.") + PROJECT_NAME;

        customWorkflow->RegisterActivity(new Activity(activityName, [&deobfuscator](const Ref<AnalysisContext>& context) {
            for (const auto& method : deobfuscator.getMethods()) {
                if (method->isRegisteredAsWorkflow()) {
                    method->execute(context);
                }
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