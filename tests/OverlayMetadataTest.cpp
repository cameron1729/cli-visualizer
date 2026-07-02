#include <Overlay/OverlayMetadata.h>
#include <gtest/gtest.h>

TEST(OverlayMetadataTest, ParsesExpectedFlatJson)
{
    vis::OverlayMetadata metadata;

    const auto parsed = vis::parse_overlay_metadata_json(
        "{\"title\":\"DJ Wukong\",\"category\":\"Singapore\","
        "\"playback\":\"playing\",\"audio_output_kind\":\"headphones\","
        "\"position_ms\":123,"
        "\"duration_ms\":456}",
        &metadata);

    EXPECT_TRUE(parsed);
    EXPECT_EQ("DJ Wukong", metadata.title);
    EXPECT_EQ("Singapore", metadata.category);
    EXPECT_EQ("playing", metadata.playback);
    EXPECT_EQ("headphones", metadata.audio_output_kind);
    EXPECT_EQ(123, metadata.position_ms);
    EXPECT_EQ(456, metadata.duration_ms);
}

TEST(OverlayMetadataTest, UnescapesJsonStrings)
{
    vis::OverlayMetadata metadata;

    const auto parsed = vis::parse_overlay_metadata_json(
        "{\"title\":\"Quote: \\\"yes\\\"\\\\ok\",\"category\":\"\","
        "\"playback\":\"paused\",\"position_ms\":0,\"duration_ms\":0}",
        &metadata);

    EXPECT_TRUE(parsed);
    EXPECT_EQ("Quote: \"yes\"\\ok", metadata.title);
    EXPECT_EQ("paused", metadata.playback);
}

TEST(OverlayMetadataTest, RejectsMalformedJson)
{
    vis::OverlayMetadata metadata;

    EXPECT_FALSE(vis::parse_overlay_metadata_json("{\"title\":", &metadata));
}
