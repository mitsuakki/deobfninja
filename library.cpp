#include <cstdio>
#include <cinttypes>

#include "library.h"
#include "binaryninjaapi.h"

#include "patterns/examples/llil_add.hpp"
#include "patterns/examples/mlil_add.hpp"

using namespace BinaryNinja;
using namespace std;

extern "C"
{
    /**
     * @brief Get the core plugin ABI version.
     * @return The core plugin ABI version.
     */
    BINARYNINJAPLUGIN uint32_t CorePluginABIVersion() { return BN_CURRENT_CORE_ABI_VERSION ; }

    /**
     * @brief Initialize the core plugin.
     * @return True if initialization is successful, false otherwise.
     */
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        // if (freopen("out/libeShard.log", "w", stdout) == nullptr) {
        //     perror("Failed to redirect stdout to log file");
        // }

        PluginCommand::Register("eshard\\All actvities", "Print all activities", [](BinaryView* view) {
            const Ref<Workflow> defaultWf = Workflow::Instance("core.function.baseAnalysis");
            for (const auto& activity : defaultWf->GetSubactivities()) {
                LogInfo("Activity: %s", activity.c_str());
            }
        });

        LLIL_ADD_Searcher* llilAddSearcher = new LLIL_ADD_Searcher();
        Ref<Workflow> customLLILPatcherWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("LLILPatcherWorkflow");
        customLLILPatcherWorkflow->RegisterActivity(new Activity("extension.llilpatcher", [llilAddSearcher](const Ref<AnalysisContext>& context) {
            llilAddSearcher->SearchLLIL(context);
        }));

        customLLILPatcherWorkflow->Insert("core.function.generateMediumLevelIL", "extension.llilpatcher");
        Workflow::RegisterWorkflow(customLLILPatcherWorkflow,
            R"#({
            "title" : "Test4",
            "description" : "This analysis stands in as an example to demonstrate Binary Ninja's extensible analysis APIs.",
            "targetType" : "function"
        })#"
        );

        MLIL_ADD_Searcher* mlilAddSearcher = new MLIL_ADD_Searcher();
        Ref<Workflow> customMLILPatcherWorkflow = Workflow::Instance("core.function.baseAnalysis")->Clone("MLILPatcherWorkflow");
        customMLILPatcherWorkflow->RegisterActivity(new Activity("extension.mlilpatcher", [mlilAddSearcher](const Ref<AnalysisContext>& context) {
            mlilAddSearcher->SearchMLIL(context);
        }));

        customMLILPatcherWorkflow->Insert("core.function.generateHighLevelIL", "extension.mlilpatcher");
        Workflow::RegisterWorkflow(customMLILPatcherWorkflow,
            R"#({
			    "title" : "Test7",
			    "description" : "This analysis stands in as an example to demonstrate Binary Ninja's extensible analysis APIs.",
			    "targetType" : "function"
			})#"
        );

        return true;
    }
}