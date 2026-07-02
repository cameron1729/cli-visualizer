/*
 * OverlayRenderer.h
 */

#ifndef _VIS_OVERLAY_RENDERER_H
#define _VIS_OVERLAY_RENDERER_H

#include "Domain/Settings.h"
#include "Overlay/OverlayMetadata.h"
#include "Writer/NcursesWriter.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace vis
{

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
    bool has_fall_animation() const noexcept;
    bool is_fall_animation_active() const noexcept;

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
};

} // namespace vis

#endif
