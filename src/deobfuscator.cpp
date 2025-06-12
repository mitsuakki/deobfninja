#include "../include/deobfuscator.hpp"
#include "../include/methods/instructions/mbasimplifier.hpp"

void Deobfuscator::init() {
    methods.push_back(new MBASimplifier());
}