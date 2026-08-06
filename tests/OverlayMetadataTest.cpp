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

TEST(OverlayMetadataTest, ParsesStatusSegmentJson)
{
    vis::StatusSegment segment;

    const auto parsed = vis::parse_status_segment_json(
        "{\"series\":\"405\","
        "\"text\":\"405 4.5.12+ PATCHES STUCK 5d 2h\","
        "\"compact\":\"405 PATCHES!5d2h\","
        "\"narrow\":\"405 PATCHES!5d\","
        "\"severity\":\"error\",\"state_key\":\"patches-stuck\","
        "\"full_width\":true}",
        &segment);

    EXPECT_TRUE(parsed);
    EXPECT_EQ("405", segment.series);
    EXPECT_EQ("405 4.5.12+ PATCHES STUCK 5d 2h", segment.text);
    EXPECT_EQ("405 PATCHES!5d2h", segment.compact);
    EXPECT_EQ("405 PATCHES!5d", segment.narrow);
    EXPECT_EQ("error", segment.severity);
    EXPECT_EQ("patches-stuck", segment.state_key);
    EXPECT_TRUE(segment.full_width);
}

TEST(OverlayMetadataTest, StatusSegmentRequiresText)
{
    vis::StatusSegment segment;

    EXPECT_FALSE(vis::parse_status_segment_json(
        "{\"series\":\"405\",\"severity\":\"ok\"}", &segment));
}

TEST(OverlayMetadataTest, ParsesMultipleStatusSegmentsFromNdjson)
{
    std::vector<vis::StatusSegment> segments;
    const auto parsed = vis::parse_status_segments_ndjson(
        "{\"series\":\"401E\",\"text\":\"401E OK\","
        "\"compact\":\"401E ✓\",\"narrow\":\"401E ✓\","
        "\"severity\":\"ok\"}\n"
        "\n"
        "{\"series\":\"405\",\"text\":\"405 PATCHES STUCK\","
        "\"compact\":\"405 STUCK\",\"severity\":\"error\"}\n",
        &segments);

    EXPECT_TRUE(parsed);
    ASSERT_EQ(2u, segments.size());
    EXPECT_EQ("401E ✓", segments[0].compact);
    EXPECT_EQ("401E ✓", segments[0].narrow);
    EXPECT_EQ("405 STUCK", segments[1].compact);
    EXPECT_EQ("405 STUCK", segments[1].narrow);
}

TEST(OverlayMetadataTest, CurrentPipelineSeriesHaveNarrowWidthFallback)
{
    const std::vector<std::string> readable_waiting{
        "401E PATCHES~2h59m", "405 PATCHES~2h59m",
        "501 PATCHES~2h59m", "502 PATCHES~2h59m"};
    const std::vector<std::string> narrow_waiting{
        "401E PATCHES~2:59", "405 PATCHES~2:59", "501 PATCHES~2:59",
        "502 PATCHES~2:59"};
    const std::vector<std::string> narrow_stuck{
        "401E PLUGINS!9:59", "405 PLUGINS!9:59", "501 PLUGINS!9:59",
        "502 PLUGINS!9:59"};

    auto bracketed_width = [](const std::vector<std::string> &labels) {
        size_t width = 0;
        for (const auto &label : labels)
        {
            width += label.size() + 2;
        }
        return width;
    };

    EXPECT_EQ(77u, bracketed_width(readable_waiting));
    EXPECT_EQ(73u, bracketed_width(narrow_waiting));
    EXPECT_EQ(73u, bracketed_width(narrow_stuck));
}
