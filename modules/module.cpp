#include "module.h"

Module::Module(std::string name, ModuleCategory category, std::string description) {
    this->name = name;
    this->category = category;
    this->description = description;
}