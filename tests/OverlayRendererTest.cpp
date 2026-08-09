#include <Overlay/OverlayRenderer.h>
#include <Transformer/SpectrumTransformer.h>
#include <gtest/gtest.h>

TEST(OverlayRendererTest, DistributesSpareWidthAcrossInternalGaps)
{
    const auto spacing = vis::distribute_status_spacing(4, 10);

    ASSERT_EQ(3u, spacing.size());
    EXPECT_EQ(3, spacing[0]);
    EXPECT_EQ(4, spacing[1]);
    EXPECT_EQ(3, spacing[2]);
}

TEST(OverlayRendererTest, KeepsEdgeAlignmentWithUnevenSpacing)
{
    const auto spacing = vis::distribute_status_spacing(4, 11);

    ASSERT_EQ(3u, spacing.size());
    EXPECT_EQ(4, spacing[0]);
    EXPECT_EQ(3, spacing[1]);
    EXPECT_EQ(4, spacing[2]);
}

TEST(OverlayRendererTest, CentresOneSpareColumnBetweenFourChunks)
{
    const auto spacing = vis::distribute_status_spacing(4, 1);

    ASSERT_EQ(3u, spacing.size());
    EXPECT_EQ(0, spacing[0]);
    EXPECT_EQ(1, spacing[1]);
    EXPECT_EQ(0, spacing[2]);
}

TEST(OverlayRendererTest, MirrorsTwoSpareColumnsBetweenFourChunks)
{
    const auto spacing = vis::distribute_status_spacing(4, 2);

    ASSERT_EQ(3u, spacing.size());
    EXPECT_EQ(1, spacing[0]);
    EXPECT_EQ(0, spacing[1]);
    EXPECT_EQ(1, spacing[2]);
}

TEST(OverlayRendererTest, SingleChunkNeedsNoInternalSpacing)
{
    EXPECT_TRUE(vis::distribute_status_spacing(1, 20).empty());
}

TEST(OverlayRendererTest, AlignsFamilyBadgesToSharedLeftEdges)
{
    const auto columns =
        vis::aligned_status_columns({10, 19, 20}, 3, 73);

    EXPECT_EQ((std::vector<int32_t>{3, 24, 53}), columns);
}

TEST(OverlayRendererTest, RejectsSharedColumnsThatCannotFit)
{
    EXPECT_TRUE(
        vis::aligned_status_columns({20, 30, 30}, 3, 73).empty());
}

TEST(OverlayRendererTest, ShrinksPreferredGapsBeforeCreatingAnotherPage)
{
    const auto pages = vis::pack_status_pages_with_adaptive_gaps(
        {10, 9, 18, 9, 9}, 70, 4);

    ASSERT_EQ(1u, pages.size());
    EXPECT_EQ((std::vector<size_t>{0, 1, 2, 3, 4}), pages[0]);
    EXPECT_EQ((std::vector<int32_t>{4, 4, 3, 4}),
              vis::distribute_status_spacing(5, 15));
}

TEST(OverlayRendererTest, PagesHealthyChunksWithoutSplittingThem)
{
    const auto layout = vis::layout_status_pages(
        {10, 10, 10, 10, 10, 10},
        {false, false, false, false, false, false},
        {3, 3, 3, 3, 3, 3}, 34, 4);

    EXPECT_TRUE(layout.pinned.empty());
    ASSERT_EQ(3u, layout.pages.size());
    EXPECT_EQ((std::vector<size_t>{0, 1}), layout.pages[0]);
    EXPECT_EQ((std::vector<size_t>{2, 3}), layout.pages[1]);
    EXPECT_EQ((std::vector<size_t>{4, 5}), layout.pages[2]);
}

TEST(OverlayRendererTest, PinsUnhealthyChunksInSeverityOrder)
{
    const auto layout = vis::layout_status_pages(
        {10, 10, 10, 10, 10},
        {false, true, false, true, false},
        {3, 1, 3, 0, 3}, 44, 4);

    EXPECT_EQ((std::vector<size_t>{3, 1}), layout.pinned);
    ASSERT_EQ(3u, layout.pages.size());
    EXPECT_EQ((std::vector<size_t>{0}), layout.pages[0]);
    EXPECT_EQ((std::vector<size_t>{2}), layout.pages[1]);
    EXPECT_EQ((std::vector<size_t>{4}), layout.pages[2]);
    EXPECT_FALSE(layout.unhealthy_overflow);
}

TEST(OverlayRendererTest, PaginatesUnhealthyChunksWhenTheyCannotAllFit)
{
    const auto layout = vis::layout_status_pages(
        {20, 20, 20, 10},
        {true, true, true, false},
        {1, 0, 0, 3}, 44, 4);

    EXPECT_TRUE(layout.pinned.empty());
    EXPECT_TRUE(layout.unhealthy_overflow);
    ASSERT_EQ(2u, layout.pages.size());
    EXPECT_EQ((std::vector<size_t>{1, 2}), layout.pages[0]);
    EXPECT_EQ((std::vector<size_t>{0}), layout.pages[1]);
}

TEST(OverlayRendererTest, HoldsThenTransitionsAndWrapsStatusPages)
{
    auto frame = vis::status_page_frame(3, 89999, 90000, 1500);
    EXPECT_EQ(0u, frame.current_page);
    EXPECT_FALSE(frame.transitioning);

    frame = vis::status_page_frame(3, 90750, 90000, 1500);
    EXPECT_EQ(0u, frame.current_page);
    EXPECT_EQ(1u, frame.next_page);
    EXPECT_TRUE(frame.transitioning);
    EXPECT_DOUBLE_EQ(0.5, frame.transition_progress);

    frame = vis::status_page_frame(3, 274500, 90000, 1500);
    EXPECT_EQ(0u, frame.current_page);
    EXPECT_EQ(1u, frame.next_page);
}

TEST(OverlayRendererTest, EasesStatusPageTransitionsInAndOut)
{
    auto frame = vis::status_page_frame(2, 90175, 90000, 700);
    EXPECT_TRUE(frame.transitioning);
    EXPECT_DOUBLE_EQ(0.15625, frame.transition_progress);

    frame = vis::status_page_frame(2, 90350, 90000, 700);
    EXPECT_DOUBLE_EQ(0.5, frame.transition_progress);

    frame = vis::status_page_frame(2, 90525, 90000, 700);
    EXPECT_DOUBLE_EQ(0.84375, frame.transition_progress);
}

TEST(OverlayRendererTest, UsesLowIdleCadenceOutsideStatusTransitions)
{
    EXPECT_EQ(100u, vis::idle_render_sleep_milliseconds(
                        20, false, false));
    EXPECT_EQ(50u, vis::idle_render_sleep_milliseconds(
                       20, true, false));
}

TEST(OverlayRendererTest, PreservesExistingFallAnimationCadence)
{
    EXPECT_EQ(25u, vis::idle_render_sleep_milliseconds(
                       20, false, true));
}

TEST(OverlayRendererTest, AgeTextDoesNotChangeStatusPagingIdentity)
{
    vis::StatusSegment earlier;
    earlier.series = "mdl405";
    earlier.text = "M405 green for 1d";
    earlier.compact = "M405 ✓1d";
    earlier.severity = "ok";
    earlier.state_key = "green";
    auto later = earlier;
    later.text = "M405 green for 2d";
    later.compact = "M405 ✓2d";

    EXPECT_EQ(vis::status_segments_signature({earlier}),
              vis::status_segments_signature({later}));

    later.state_key = "patches-wait";
    later.severity = "warning";
    EXPECT_NE(vis::status_segments_signature({earlier}),
              vis::status_segments_signature({later}));
}

TEST(OverlayRendererTest, FamilyCarouselDoesNotJumpForFailureChanges)
{
    vis::StatusSegment healthy;
    healthy.series = "mwp501";
    healthy.severity = "ok";
    healthy.state_key = "green";
    auto failed = healthy;
    failed.severity = "error";
    failed.state_key = "plugins-stuck";

    EXPECT_EQ(vis::status_family_carousel_signature({healthy}),
              vis::status_family_carousel_signature({failed}));

    failed.series = "mwp501r";
    EXPECT_NE(vis::status_family_carousel_signature({healthy}),
              vis::status_family_carousel_signature({failed}));
}

TEST(OverlayRendererTest, SplitsFamilyRowsAndRemovesRedundantMarkers)
{
    vis::StatusSegment moodle;
    moodle.series = "mdl405";
    moodle.compact = "M405 ✓2d";
    moodle.narrow = "M405 ✓2d";
    vis::StatusSegment workplace;
    workplace.series = "mwp501r";
    workplace.compact = "W501R PLUGINS !2h";
    workplace.narrow = "W501R PLUGINS!2h";

    const auto rows = vis::status_family_rows({moodle, workplace});

    ASSERT_EQ(1u, rows.moodle.size());
    ASSERT_EQ(1u, rows.workplace.size());
    EXPECT_EQ("405 ✓2d", rows.moodle[0].compact);
    EXPECT_EQ("501R PLUGINS !2h", rows.workplace[0].compact);
    EXPECT_EQ("501R PLUGINS!2h", rows.workplace[0].narrow);
    EXPECT_EQ("mdl405", rows.moodle[0].series);
    EXPECT_EQ("mwp501r", rows.workplace[0].series);
}

TEST(OverlayRendererTest, FlightUsesFullRowBelowStatusWithoutPlayback)
{
    vis::Settings settings{""};
    settings.set_overlay_status_enabled(true);
    settings.set_overlay_status_row(0);
    settings.set_overlay_progress_row(2);
    const vis::OverlayMetadata playback_metadata;

    const auto layout =
        vis::shared_slot_overlay_layout(settings, playback_metadata);

    EXPECT_EQ(1, layout.row);
    EXPECT_FALSE(layout.shares_playback_row);
}

TEST(OverlayRendererTest, FlightMovesBelowBothFamilyStatusRows)
{
    vis::Settings settings{""};
    settings.set_overlay_status_enabled(true);
    settings.set_overlay_status_row(0);
    settings.set_overlay_status_layout("families");
    settings.set_overlay_progress_row(2);
    const vis::OverlayMetadata playback_metadata;

    const auto layout =
        vis::shared_slot_overlay_layout(settings, playback_metadata);

    EXPECT_EQ(2, layout.row);
    EXPECT_FALSE(layout.shares_playback_row);
}

TEST(OverlayRendererTest, FamilyCarouselUsesOneStatusRow)
{
    vis::Settings settings{""};
    settings.set_overlay_status_enabled(true);
    settings.set_overlay_status_row(0);
    settings.set_overlay_status_layout("family-carousel");
    const vis::OverlayMetadata playback_metadata;

    const auto layout =
        vis::shared_slot_overlay_layout(settings, playback_metadata);

    EXPECT_EQ(1, layout.row);
    EXPECT_FALSE(layout.shares_playback_row);
}

TEST(OverlayRendererTest, FlightSharesProgressRowDuringPlayback)
{
    vis::Settings settings{""};
    settings.set_overlay_status_enabled(true);
    settings.set_overlay_status_row(0);
    settings.set_overlay_progress_row(2);
    vis::OverlayMetadata playback_metadata;
    playback_metadata.title = "Now playing";
    playback_metadata.playback = "playing";

    const auto layout =
        vis::shared_slot_overlay_layout(settings, playback_metadata);

    EXPECT_EQ(2, layout.row);
    EXPECT_TRUE(layout.shares_playback_row);
}

TEST(OverlayRendererTest, StoppedPlaybackDoesNotReserveMediaRows)
{
    vis::Settings settings{""};
    settings.set_overlay_status_enabled(true);
    settings.set_overlay_status_row(0);
    settings.set_overlay_progress_row(2);
    vis::OverlayMetadata playback_metadata;
    playback_metadata.title = "Previous title";
    playback_metadata.playback = "stopped";

    const auto layout =
        vis::shared_slot_overlay_layout(settings, playback_metadata);

    EXPECT_EQ(1, layout.row);
    EXPECT_FALSE(layout.shares_playback_row);
}

TEST(OverlayRendererTest, BaiyanUsesAdaptiveStatusText)
{
    vis::OverlayMetadata metadata;
    metadata.baiyan_available = true;
    metadata.baiyan_scanned = true;
    metadata.baiyan_compact =
        u8"百眼:// 🪟14/✅10/🚨1/💬3/🔎2/🧪9/🎓11/西方無赦🕒1h";
    metadata.baiyan_expanded =
        u8"百眼:// 🪟14 / ✅10 / 🚨1 / 💬3 / 🔎2 / 🧪9 / 🎓11 / 西方無赦 🕒1h";

    EXPECT_EQ(u8"百眼:// 🪟14/✅10/🚨1/💬3/🔎2/🧪9/🎓11/西方無赦🕒1h",
              vis::baiyan_overlay_text(metadata, true));
    EXPECT_EQ(
        u8"百眼:// 🪟14 / ✅10 / 🚨1 / 💬3 / 🔎2 / 🧪9 / 🎓11 / 西方無赦 🕒1h",
        vis::baiyan_overlay_text(metadata, false));
}

TEST(OverlayRendererTest, BaiyanHidesUnscannedAndShowsClearState)
{
    vis::OverlayMetadata metadata;
    metadata.baiyan_available = true;
    metadata.baiyan_compact = u8"百眼:// ❔/🧪🔄/🎓⚠️11/西方無赦🕒1h";
    metadata.baiyan_expanded = u8"百眼:// ❔ / 🧪🔄 / 🎓⚠️11 / 西方無赦 🕒1h";

    EXPECT_TRUE(metadata.empty());
    EXPECT_TRUE(vis::baiyan_overlay_text(metadata, true).empty());
    EXPECT_TRUE(vis::baiyan_overlay_text(metadata, false).empty());

    metadata.baiyan_scanned = true;
    metadata.baiyan_compact = u8"百眼:// ✨/🧪9/🎓11/西方無赦🕒1h";
    metadata.baiyan_expanded = u8"百眼:// ✨ / 🧪9 / 🎓11 / 西方無赦 🕒1h";
    EXPECT_EQ(u8"百眼:// ✨/🧪9/🎓11/西方無赦🕒1h",
              vis::baiyan_overlay_text(metadata, true));
    EXPECT_EQ(u8"百眼:// ✨ / 🧪9 / 🎓11 / 西方無赦 🕒1h",
              vis::baiyan_overlay_text(metadata, false));
}

TEST(OverlayRendererTest, ActiveFlightSuppressesBaiyan)
{
    vis::OverlayMetadata flight;
    vis::OverlayMetadata baiyan;
    baiyan.baiyan_available = true;

    EXPECT_FALSE(vis::baiyan_overlay_visible(flight, baiyan));
    baiyan.baiyan_scanned = true;
    EXPECT_TRUE(vis::baiyan_overlay_visible(flight, baiyan));
    flight.flight_active = true;
    EXPECT_FALSE(vis::baiyan_overlay_visible(flight, baiyan));
    flight.flight_active = false;
    baiyan.baiyan_available = false;
    EXPECT_FALSE(vis::baiyan_overlay_visible(flight, baiyan));
}
