#include <gtest/gtest.h>
#include "../include/deobfuscator.hpp"
#include "../include/methods/instructions/mbasimplifier.hpp"

class DummyDeobfuscationMethod : public IDeobfuscationMethod {
public:
    DummyDeobfuscationMethod(const std::string& desc, DeobfuscationCategory cat)
        : IDeobfuscationMethod(desc, cat) {}

    void execute(const BinaryNinja::Ref<BinaryNinja::AnalysisContext>&) override {}
};

TEST(IDeobfuscationMethodTest, ConstructionAndGetters) {
    DummyDeobfuscationMethod method("TestMethod", DeobfuscationCategory::Function);

    EXPECT_EQ(method.getDescription(), "TestMethod");
    EXPECT_EQ(method.getCategory(), DeobfuscationCategory::Function);
    EXPECT_FALSE(method.isEnabled);
}
