#include "../include/deobfuscator.hpp"
#include "../include/methods/instructions/mbasimplifier.hpp"

bool Deobfuscator::registerMethod(IDeobfuscationMethod* method) {
    if (!method)
        return false;

    IDeobfuscationMethod* deobMethod = dynamic_cast<IDeobfuscationMethod*>(method);
    if (!deobMethod)
        return false;

    if (std::any_of(methods.begin(), methods.end(),
        [deobMethod](const IDeobfuscationMethod* m) {
            return typeid(*m) == typeid(*deobMethod);
        })) {
        return false;
    }

    methods.push_back(deobMethod);
    return true;
}

void Deobfuscator::init() {
    registerMethod(new MBASimplifier());
}