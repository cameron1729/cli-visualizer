#include <Overlay/OverlayRenderer.h>
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
