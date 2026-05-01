#include <gtest/gtest.h>
#include <openitup/chart/sma_parser.h>
#include <openitup/chart/chart_builder.h>

using namespace openitup;

TEST(SmaParser, ParsesBasicSmaFile) {
    std::string sma_content = R"(
#TITLE:Test Song;
#ARTIST:Test Artist;
#BPMS:0.000=120.000;
#NOTES:
pump-single5:
:
Easy:
5:
:
00100
10001
01110
,
10001
00100
01110
;
)";

    auto reader = [sma_content](const std::filesystem::path&) { return sma_content; };
    SmaParser parser(reader);

    auto charts = parser.parse("/fake/path.sma");

    ASSERT_EQ(charts.size(), 1);
    EXPECT_EQ(charts[0].metadata().title, "Test Song");
    EXPECT_EQ(charts[0].metadata().artist, "Test Artist");
    EXPECT_EQ(charts[0].metadata().difficulty_name, "Easy");
    EXPECT_EQ(charts[0].metadata().mode, PlayMode::SINGLE);
}

TEST(SmaParser, ParsesMultipleCharts) {
    std::string sma_content = R"(
#TITLE:Multi;
#BPMS:0.000=140.000;
#NOTES:
pump-single5:
:
Easy:
3:
:
10001
;
#NOTES:
pump-single5:
:
Hard:
10:
:
11111
;
)";

    auto reader = [sma_content](const std::filesystem::path&) { return sma_content; };
    SmaParser parser(reader);

    auto charts = parser.parse("/fake/path.sma");

    ASSERT_EQ(charts.size(), 2);
    EXPECT_EQ(charts[0].metadata().difficulty_name, "Easy");
    EXPECT_EQ(charts[0].metadata().difficulty_rating, 3);
    EXPECT_EQ(charts[1].metadata().difficulty_name, "Hard");
    EXPECT_EQ(charts[1].metadata().difficulty_rating, 10);
}

TEST(SmaParser, SkipsUnsupportedChartTypes) {
    std::string sma_content = R"(
#TITLE:Dance;
#BPMS:0.000=120.000;
#NOTES:
dance-single:
:
Easy:
5:
:
1000
;
#NOTES:
pump-single5:
:
Normal:
7:
:
10001
;
)";

    auto reader = [sma_content](const std::filesystem::path&) { return sma_content; };
    SmaParser parser(reader);

    auto charts = parser.parse("/fake/path.sma");

    ASSERT_EQ(charts.size(), 1);
    EXPECT_EQ(charts[0].metadata().difficulty_name, "Normal");
}

TEST(SmaParser, ParsesStops) {
    std::string sma_content = R"(
#TITLE:Stop Test;
#BPMS:0.000=120.000;
#STOPS:4.000=1.000;
#NOTES:
pump-single5:
:
Easy:
5:
:
10001
;
)";

    auto reader = [sma_content](const std::filesystem::path&) { return sma_content; };
    SmaParser parser(reader);

    auto charts = parser.parse("/fake/path.sma");

    ASSERT_EQ(charts.size(), 1);
    // Verify that the chart has timing data with a stop
    const auto& timing = charts[0].timing_data();
    EXPECT_GT(timing.size(), 1); // Should have at least BPM + stop
}
