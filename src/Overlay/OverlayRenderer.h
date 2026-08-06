/*
 * OverlayRenderer.h
 */

#ifndef _VIS_OVERLAY_RENDERER_H
#define _VIS_OVERLAY_RENDERER_H

#include "Domain/Settings.h"
#include "Overlay/OverlayMetadata.h"
#include "Writer/NcursesWriter.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vis
{

std::vector<int32_t> distribute_status_spacing(size_t chunk_count,
                                               int32_t spare_width);

std::vector<int32_t> aligned_status_columns(
    const std::vector<int32_t> &column_widths, int32_t content_left,
    int32_t total_width);

std::vector<std::vector<size_t>> pack_status_pages_with_adaptive_gaps(
    const std::vector<int32_t> &chunk_widths, int32_t available_width,
    int32_t preferred_gap);

struct StatusPageLayout
{
    std::vector<size_t> pinned;
    std::vector<std::vector<size_t>> pages;
    bool unhealthy_overflow{false};
};

StatusPageLayout layout_status_pages(
    const std::vector<int32_t> &chunk_widths,
    const std::vector<bool> &unhealthy,
    const std::vector<int32_t> &severity_priorities,
    int32_t available_width, int32_t gap);

struct StatusPageFrame
{
    size_t current_page{0};
    size_t next_page{0};
    double transition_progress{0.0};
    bool transitioning{false};
};

StatusPageFrame status_page_frame(size_t page_count, uint64_t elapsed_ms,
                                  uint32_t hold_ms,
                                  uint32_t transition_ms);

std::string status_segments_signature(
    const std::vector<StatusSegment> &segments);

std::string status_family_carousel_signature(
    const std::vector<StatusSegment> &segments);

struct StatusFamilyRows
{
    std::vector<StatusSegment> moodle;
    std::vector<StatusSegment> workplace;
};

StatusFamilyRows status_family_rows(
    const std::vector<StatusSegment> &segments);

struct FlightOverlayLayout
{
    int32_t row{0};
    bool shares_playback_row{false};
};

FlightOverlayLayout flight_overlay_layout(
    const Settings &settings, const OverlayMetadata &playback_metadata);

class OverlayRenderer
{
  public:
    bool draw_overlay(const Settings &settings,
                      const OverlayMetadata &metadata,
                      NcursesWriter *writer,
                      const std::vector<std::vector<uint8_t>> *occupied_cells =
                          nullptr);
    bool draw_marquee(const Settings &settings,
                      const OverlayMetadata &metadata,
                      NcursesWriter *writer);
    bool draw_progress(const Settings &settings,
                       const OverlayMetadata &metadata,
                       NcursesWriter *writer);
    bool draw_flight_progress(const Settings &settings,
                              const OverlayMetadata &metadata,
                              const OverlayMetadata &playback_metadata,
                              NcursesWriter *writer,
                              const std::vector<std::vector<uint8_t>>
                                  *occupied_cells = nullptr);
    bool draw_status(const Settings &settings,
                     const std::vector<StatusSegment> &segments,
                     NcursesWriter *writer);
    bool has_fall_animation() const noexcept;
    bool is_fall_animation_active() const noexcept;
    bool is_status_transition_active() const noexcept
    {
        return m_status_transition_active;
    }

  private:
    struct Glyph
    {
        std::wstring text;
        int32_t width{0};
    };

    struct RenderedGlyph
    {
        std::wstring text;
        int32_t row{0};
        int32_t column{0};
        int32_t width{0};
    };

    struct FallingGlyph
    {
        RenderedGlyph glyph;
        double delay_seconds{0.0};
        double speed_rows_per_second{0.0};
    };

    struct StatusPagingState
    {
        std::string signature;
        std::chrono::steady_clock::time_point started_at;
    };

    static std::wstring utf8_to_wstring(const std::string &text);
    static std::vector<Glyph> text_to_glyphs(const std::wstring &text,
                                             int32_t *cell_width);
    static int32_t glyphs_width(const std::vector<Glyph> &glyphs);
    static bool is_live_metadata(const OverlayMetadata &metadata);
    static uint32_t glyph_seed(const RenderedGlyph &glyph);
    static void draw_glyphs(NcursesWriter *writer, int32_t row,
                            int32_t column, const ColorDefinition &color,
                            const std::vector<Glyph> &glyphs,
                            int32_t win_width,
                            const std::vector<std::vector<uint8_t>>
                                *occupied_cells,
                            std::vector<RenderedGlyph> *rendered);
    static void draw_glyphs_clipped(NcursesWriter *writer, int32_t row,
                                    int32_t column,
                                    const ColorDefinition &color,
                                    const std::vector<Glyph> &glyphs,
                                    int32_t clip_left, int32_t clip_right,
                                    const std::vector<std::vector<uint8_t>>
                                        *occupied_cells,
                                    std::vector<RenderedGlyph> *rendered);

    bool draw_marquee(const Settings &settings,
                      const OverlayMetadata &metadata, NcursesWriter *writer,
                      const std::vector<std::vector<uint8_t>> *occupied_cells,
                      std::vector<RenderedGlyph> *rendered);
    bool draw_progress(const Settings &settings,
                       const OverlayMetadata &metadata, NcursesWriter *writer,
                       const std::vector<std::vector<uint8_t>> *occupied_cells,
                       std::vector<RenderedGlyph> *rendered);
    bool draw_status_row(const Settings &settings,
                         const std::vector<StatusSegment> &segments,
                         NcursesWriter *writer, int32_t row,
                         const std::wstring &prefix,
                         int32_t content_left_override,
                         const std::vector<int32_t> &aligned_columns,
                         StatusPagingState *paging_state);
    bool draw_status_family_carousel(
        const Settings &settings,
        const std::vector<StatusSegment> &segments,
        NcursesWriter *writer, int32_t row,
        StatusPagingState *paging_state);
    void start_fall();
    bool draw_falling_overlay(
        const Settings &settings, NcursesWriter *writer,
        const std::vector<std::vector<uint8_t>> *occupied_cells);

    std::string m_last_title;
    std::chrono::steady_clock::time_point m_title_started_at;
    std::vector<RenderedGlyph> m_last_live_overlay;
    std::vector<FallingGlyph> m_falling_overlay;
    std::chrono::steady_clock::time_point m_fall_started_at;
    bool m_last_frame_was_live{false};
    StatusPagingState m_status_single_state;
    StatusPagingState m_status_moodle_state;
    StatusPagingState m_status_workplace_state;
    StatusPagingState m_status_family_carousel_state;
    bool m_status_transition_active{false};
};

} // namespace vis

#endif
