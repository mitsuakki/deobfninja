#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include <string>
#include <vector>

#include "module.h"

class ModuleManager {
  public:
    std::vector<class Module*> modules;
    std::vector<ModuleCategory> categories;

    void init();

    class Module* getModuleByName(const std::string& name);
    std::vector<class Module*> getModulesFromCategory(const ModuleCategory& Category);


};

#endif //MODULEMANAGER_H
