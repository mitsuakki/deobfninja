#include "modulemanager.h"

ModuleManager moduleManager;

void ModuleManager::init() {
    // modules.push_back();

    for (const auto& module : modules) {
        bool exists = false;
        for (const auto& currentCategory : categories) {
            if (currentCategory == module->category) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            categories.push_back(module->category);
        }
    }
}

Module* ModuleManager::getModuleByName(const std::string& name) {
    for (const auto& module : modules) {
        if (module->name == name) {
            return module;
        }
    }
    return nullptr;
}

std::vector<Module*> ModuleManager::getModulesFromCategory(const ModuleCategory& category) {
    std::vector<Module*> modulesArray;
    for (const auto& module : modules) {
        if (module->category == category) {
            modulesArray.push_back(module);
        }
    }
    return modulesArray;
}