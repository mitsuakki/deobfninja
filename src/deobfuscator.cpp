#include <cxxabi.h>

#include "../include/deobfuscator.hpp"
#include "../include/methods/instructions/mbasimplifier.hpp"
#include "../include/methods/flows/cffdetection.hpp"
#include "../include/methods/flows/nativepredicatesolver.hpp"

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

    int status;
    const char* mangled = typeid(*deobMethod).name();
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    std::string className = (status == 0 && demangled) ? demangled : mangled;
    free(demangled);  // Prevent memory leak

    BinaryNinja::LogInfo("Registering method: %s", className.c_str());

    methods.push_back(deobMethod);
    return true;
}

void Deobfuscator::init() {
    registerMethod(new Instructions::MBASimplifier());
    registerMethod(new Flows::CFFDetection());
    registerMethod(new Flows::NativePredicateSolver());
}