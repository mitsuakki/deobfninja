#ifndef DEOBFUSCATOR_HPP
#define DEOBFUSCATOR_HPP

#pragma once

#include <vector>
#include "methods/methods.hpp"

class Deobfuscator {
public:
    void init();
    bool registerMethod(IDeobfuscationMethod* method);
    std::vector<IDeobfuscationMethod*> getMethods() const {
        return methods;
    }

    std::vector<IDeobfuscationMethod*> getMethodsByCategory(DeobfuscationCategory category) const {
        std::vector<IDeobfuscationMethod*> filtered;

        for (auto* method : methods) {
            if (method->category == category) {
                filtered.push_back(method);
            }
        }

        return filtered;
    }

private:
    std::vector<IDeobfuscationMethod*> methods;
};

#endif // DEOBFUSCATOR_HPP