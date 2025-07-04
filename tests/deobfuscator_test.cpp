#include <gtest/gtest.h>
#include "../include/deobfuscator.hpp"
#include "../include/methods/instructions/mbasimplifier.hpp"

class DummyMethod : public IDeobfuscationMethod {
public:
    DummyMethod() : IDeobfuscationMethod("Dummy method", DeobfuscationCategory::Function) {}
    void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>&) override {}
    ~DummyMethod() override = default;
};

TEST(DeobfuscatorTest, RegisterNullptrMethodReturnsFalse) {
    Deobfuscator deob;
    EXPECT_FALSE(deob.registerMethod(nullptr));
}

TEST(DeobfuscatorTest, RegisterValidMethodReturnsTrue) {
    Deobfuscator deob;
    DummyMethod* method = new DummyMethod();
    EXPECT_TRUE(deob.registerMethod(method));

    delete method;
}

TEST(DeobfuscatorTest, RegisterSameTypeTwiceReturnsFalse) {
    Deobfuscator deob;
    DummyMethod* method1 = new DummyMethod();
    DummyMethod* method2 = new DummyMethod();
    EXPECT_TRUE(deob.registerMethod(method1));
    EXPECT_FALSE(deob.registerMethod(method2));

    delete method1;
    delete method2;
}

TEST(DeobfuscatorTest, RegisterDifferentTypesReturnsTrue) {
    Deobfuscator deob;
    DummyMethod* dummy = new DummyMethod();
    MBASimplifier* mba = new MBASimplifier();
    EXPECT_TRUE(deob.registerMethod(dummy));
    EXPECT_TRUE(deob.registerMethod(mba));

    delete dummy;
    delete mba;
}
  
TEST(DeobfuscatorTest, InitRegistersMBASimplifier) {
    Deobfuscator deob;
    deob.init();
    // We can't directly check the private methods vector,
    // but we can try to register another MBASimplifier and expect false.
    MBASimplifier* mba = new MBASimplifier();
    EXPECT_FALSE(deob.registerMethod(mba)); 
    delete mba;
}