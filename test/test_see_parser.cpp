#include <gtest/gtest.h>
#include <openitup/chart/see_parser.h>

using namespace openitup;

TEST(SeeParser, ReturnsEmptyVectorForSeeFile) {
    std::string see_content = "encrypted binary data here";

    auto reader = [see_content](const std::filesystem::path&) { return see_content; };
    SeeParser parser(reader);

    auto charts = parser.parse("/fake/path.see");

    // SEE format is not yet supported, should return empty vector
    EXPECT_EQ(charts.size(), 0);
}

TEST(SeeParser, DoesNotThrowOnSeeFile) {
    std::string see_content = "fake encrypted data";

    auto reader = [see_content](const std::filesystem::path&) { return see_content; };
    SeeParser parser(reader);

    // Should not throw, just return empty
    EXPECT_NO_THROW({
        auto charts = parser.parse("/fake/path.see");
        EXPECT_EQ(charts.size(), 0);
    });
}
