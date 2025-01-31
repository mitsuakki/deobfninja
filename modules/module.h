#ifndef MODULE_H
#define MODULE_H

#include <string>

enum ModuleCategory {
  Command,
  Workflow
};

class Module {
  public:
    std::string name, description;
    ModuleCategory category;

    bool isEnabled = false;
    Module(std::string name, ModuleCategory category, std::string description);
};

#endif //MODULE_H