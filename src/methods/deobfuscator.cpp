#include "../../binaryninjaapi/binaryninjaapi.h"

#include "../../include/methods/deobfuscator.hpp"
#include "../../include/methods/instructions/mbasimplifier.hpp"

void Deobfuscator::registerMethod(std::unique_ptr<IDeobfuscationMethod> method)
{
    if (method)
    {
        methods.push_back(std::move(method));
        LogInfo("Registered deobfuscation method: %s", methods.back()->name().c_str());
    }
}

void Deobfuscator::registerAll()
{
    registerMethod(std::make_unique<MBASimplifier>());
}

std::vector<IDeobfuscationMethod*> Deobfuscator::getMethods() const {
    std::vector<IDeobfuscationMethod*> methodPtrs;
    for (const auto& method : methods) {
        methodPtrs.push_back(method.get());
    }
    return methodPtrs;
}