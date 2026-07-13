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

TEST(OverlayMetadataTest, ParsesFlightProgressJson)
{
    vis::OverlayMetadata metadata;

    const auto parsed = vis::parse_overlay_metadata_json(
        "{\"flight_active\":true,\"flight_callsign\":\"MB36Y\","
        "\"flight_left_label\":\"PER\",\"flight_right_label\":\"KGI\","
        "\"flight_direction_label\":\"KGI->PER\","
        "\"flight_target_label\":\"PER\","
        "\"flight_markers\":\"964|PER|5320;214|KGI|1800\","
        "\"flight_progress_per_mille\":964,"
        "\"flight_eta_seconds\":5320,"
        "\"flight_lat\":-30.8313,\"flight_lon\":121.2620,"
        "\"flight_alt_ft\":7200,\"flight_ground_speed_kt\":189,"
        "\"flight_track_deg\":257}",
        &metadata);

    EXPECT_TRUE(parsed);
    EXPECT_TRUE(metadata.flight_active);
    EXPECT_EQ("MB36Y", metadata.flight_callsign);
    EXPECT_EQ("PER", metadata.flight_left_label);
    EXPECT_EQ("KGI", metadata.flight_right_label);
    EXPECT_EQ("KGI->PER", metadata.flight_direction_label);
    EXPECT_EQ("PER", metadata.flight_target_label);
    EXPECT_EQ("964|PER|5320;214|KGI|1800", metadata.flight_markers);
    EXPECT_EQ(964, metadata.flight_progress_per_mille);
    EXPECT_EQ(5320, metadata.flight_eta_seconds);
    EXPECT_NEAR(-30.8313, metadata.flight_lat, 0.0001);
    EXPECT_NEAR(121.2620, metadata.flight_lon, 0.0001);
    EXPECT_EQ(7200, metadata.flight_alt_ft);
    EXPECT_EQ(189, metadata.flight_ground_speed_kt);
    EXPECT_EQ(257, metadata.flight_track_deg);
}

TEST(OverlayMetadataTest, SkipsUnknownDecimalJsonValues)
{
    vis::OverlayMetadata metadata;

    const auto parsed = vis::parse_overlay_metadata_json(
        "{\"title\":\"ok\",\"unknown_decimal\":-30.8313}",
        &metadata);

    EXPECT_TRUE(parsed);
    EXPECT_EQ("ok", metadata.title);
}
