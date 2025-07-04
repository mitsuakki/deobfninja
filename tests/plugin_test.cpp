#include <gtest/gtest.h>

class MockBinaryView {
public:
    bool metadataExists = false;
    std::string storedMetadata;
    std::string platformName = "windows-x86_64";

    void StoreMetadata(const std::string&, const std::string& data, bool) {
        storedMetadata = data;
        metadataExists = true;
    }
    std::string QueryMetadata(const std::string&) {
        return metadataExists ? storedMetadata : "";
    }
    std::string GetDefaultPlatform() { return platformName; }
};

class MockAnalysisContext {
    MockBinaryView* view;
public:
    explicit MockAnalysisContext(MockBinaryView* v) : view(v) {}
    MockBinaryView* GetBinaryView() { return view; }
};

TEST(PluginTest, MetadataExistsReturnsFalseWhenNoMetadata) {
    MockBinaryView view;
    view.metadataExists = false;
    EXPECT_EQ(view.QueryMetadata("rtti"), "");
}

TEST(PluginTest, StoreMetadataSetsMetadata) {
    MockBinaryView view;
    view.StoreMetadata("rtti", "testdata", true);
    EXPECT_EQ(view.QueryMetadata("rtti"), "testdata");
}

TEST(PluginTest, PlatformNameIsWindows) {
    MockBinaryView view;
    EXPECT_NE(view.GetDefaultPlatform().find("windows"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}