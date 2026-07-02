/*
 * OverlayRenderer.cpp
 */

#include "Overlay/OverlayRenderer.h"

#include "Domain/VisConstants.h"
#include "Utils/NcursesUtils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <codecvt>
#include <cstdio>
#include <cwchar>
#include <locale>
#include <stdexcept>

namespace
{

const std::string k_align_right{"right"};
const std::string k_audio_output_headphones{"headphones"};
const std::string k_audio_output_speakers{"speakers"};
const std::string k_playback_playing{"playing"};
const std::string k_playback_paused{"paused"};
const std::string k_playback_stopped{"stopped"};

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

std::wstring ascii_to_wstring(const std::string &text)
{
    return std::wstring{text.begin(), text.end()};
}

std::wstring format_time(const int64_t milliseconds)
{
    auto total_seconds = std::max<int64_t>(0, milliseconds / 1000);
    const auto hours = total_seconds / 3600;
    total_seconds %= 3600;
    const auto minutes = total_seconds / 60;
    const auto seconds = total_seconds % 60;

    char buffer[32];
    if (hours > 0)
    {
        std::snprintf(buffer, sizeof(buffer), "%lld:%02lld:%02lld",
                      static_cast<long long>(hours),
                      static_cast<long long>(minutes),
                      static_cast<long long>(seconds));
    }
    else
    {
        std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld",
                      static_cast<long long>(minutes),
                      static_cast<long long>(seconds));
    }

    return ascii_to_wstring(buffer);
}

void append_part(std::wstring *line, const std::wstring &part)
{
    if (line == nullptr || part.empty())
    {
        return;
    }

    if (!line->empty())
    {
        line->push_back(L' ');
    }
    line->append(part);
}

std::wstring configured_or_default(const std::wstring &configured,
                                   const std::wstring &default_value)
{
    return configured.empty() ? default_value : configured;
}

bool cells_are_free(
    const std::vector<std::vector<uint8_t>> *occupied_cells,
    const int32_t row, const int32_t column, const int32_t width)
{
    if (occupied_cells == nullptr)
    {
        return true;
    }

    if (row < 0 || column < 0 || width <= 0 ||
        row >= static_cast<int32_t>(occupied_cells->size()))
    {
        return false;
    }

    const auto &occupied_row =
        (*occupied_cells)[static_cast<size_t>(row)];
    if (column + width > static_cast<int32_t>(occupied_row.size()))
    {
        return false;
    }

    for (auto cell = column; cell < column + width; ++cell)
    {
        if (occupied_row[static_cast<size_t>(cell)] != 0)
        {
            return false;
        }
    }

    return true;
}

std::wstring status_left(const vis::Settings &settings,
                         const vis::OverlayMetadata &metadata)
{
    std::wstring line;

    if (!settings.is_overlay_progress_status_enabled())
    {
        return line;
    }

    const auto audio_output_kind = lowercase(metadata.audio_output_kind);
    if (audio_output_kind == k_audio_output_headphones)
    {
        append_part(&line, settings.get_overlay_status_headphones_icon());
    }
    else
    {
        append_part(&line, settings.get_overlay_status_speakers_icon());
    }

    const auto playback = lowercase(metadata.playback);
    if (playback == k_playback_playing)
    {
        append_part(&line, settings.get_overlay_status_playing_icon());
    }
    else if (playback == k_playback_paused)
    {
        append_part(&line, settings.get_overlay_status_paused_icon());
    }
    else if (playback == k_playback_stopped)
    {
        append_part(&line, settings.get_overlay_status_stopped_icon());
    }

    return line;
}

std::wstring status_right(const vis::Settings &settings,
                          const vis::OverlayMetadata &metadata)
{
    if (settings.is_overlay_progress_time_enabled() &&
        metadata.duration_ms > 0)
    {
        return format_time(metadata.position_ms) + L" / " +
               format_time(metadata.duration_ms);
    }

    return L"";
}

std::wstring progress_bar(const vis::Settings &settings,
                          const vis::OverlayMetadata &metadata,
                          const int32_t cell_width)
{
    if (metadata.duration_ms <= 0 || cell_width <= 0)
    {
        return L"";
    }

    const auto position = std::max<int64_t>(0, metadata.position_ms);
    const auto duration = std::max<int64_t>(1, metadata.duration_ms);
    const auto ratio =
        std::min(1.0, static_cast<double>(position) / duration);
    const auto filled_count =
        static_cast<int32_t>(
            std::round(ratio * static_cast<double>(cell_width)));
    const auto filled = configured_or_default(
        settings.get_overlay_progress_filled(),
        VisConstants::k_default_overlay_progress_filled);
    const auto empty = configured_or_default(
        settings.get_overlay_progress_empty(),
        VisConstants::k_default_overlay_progress_empty);

    std::wstring bar;
    for (auto i = 0; i < cell_width; ++i)
    {
        bar.append(i < filled_count ? filled : empty);
    }

    return bar;
}

} // namespace

std::wstring vis::OverlayRenderer::utf8_to_wstring(const std::string &text)
{
    try
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(text);
    }
    catch (const std::range_error &)
    {
        return std::wstring{text.begin(), text.end()};
    }
}

std::vector<vis::OverlayRenderer::Glyph>
vis::OverlayRenderer::text_to_glyphs(const std::wstring &text,
                                     int32_t *cell_width)
{
    std::vector<Glyph> glyphs;
    int32_t width = 0;

    for (const auto ch : text)
    {
        auto ch_width = static_cast<int32_t>(wcwidth(ch));
        if (ch_width == 0 && !glyphs.empty())
        {
            glyphs.back().text.push_back(ch);
            continue;
        }

        Glyph glyph;
        if (ch_width < 0)
        {
            glyph.text = L" ";
            glyph.width = 1;
        }
        else
        {
            glyph.text.push_back(ch);
            glyph.width = std::max(ch_width, 1);
        }

        width += glyph.width;
        glyphs.push_back(glyph);
    }

    if (cell_width != nullptr)
    {
        *cell_width = width;
    }

    return glyphs;
}

int32_t vis::OverlayRenderer::glyphs_width(const std::vector<Glyph> &glyphs)
{
    int32_t width = 0;
    for (const auto &glyph : glyphs)
    {
        width += glyph.width;
    }
    return width;
}

bool vis::OverlayRenderer::is_live_metadata(
    const vis::OverlayMetadata &metadata)
{
    const auto playback = lowercase(metadata.playback);
    const auto has_playback_payload =
        !metadata.title.empty() || !metadata.playback.empty() ||
        metadata.duration_ms > 0 || metadata.position_ms > 0;

    return has_playback_payload && playback != k_playback_stopped;
}

uint32_t vis::OverlayRenderer::glyph_seed(const RenderedGlyph &glyph)
{
    auto seed = static_cast<uint32_t>((glyph.row + 1) * 131u) ^
                static_cast<uint32_t>((glyph.column + 17) * 977u) ^
                static_cast<uint32_t>(glyph.width * 37u);
    for (const auto ch : glyph.text)
    {
        seed = (seed * 33u) ^ static_cast<uint32_t>(ch);
    }
    return seed;
}

void vis::OverlayRenderer::draw_glyphs(vis::NcursesWriter *writer,
                                       const int32_t row,
                                       const int32_t column,
                                       const vis::ColorDefinition &color,
                                       const std::vector<Glyph> &glyphs,
                                       const int32_t win_width,
                                       const std::vector<std::vector<uint8_t>>
                                           *occupied_cells,
                                       std::vector<RenderedGlyph> *rendered)
{
    auto cell = column;
    auto run_start = 0;
    auto run_end = 0;
    std::wstring run_text;

    const auto flush_run = [&]() {
        if (!run_text.empty())
        {
            writer->write_text(row, run_start, color, run_text);
            run_text.clear();
        }
    };

    for (const auto &glyph : glyphs)
    {
        if (cell >= 0 && cell + glyph.width <= win_width &&
            cells_are_free(occupied_cells, row, cell, glyph.width))
        {
            if (run_text.empty() || cell != run_end)
            {
                flush_run();
                run_start = cell;
                run_end = cell;
            }
            run_text.append(glyph.text);
            run_end += glyph.width;

            if (rendered != nullptr)
            {
                rendered->push_back(
                    RenderedGlyph{glyph.text, row, cell, glyph.width});
            }
        }
        else
        {
            flush_run();
        }
        cell += glyph.width;
    }
    flush_run();
}

void vis::OverlayRenderer::draw_glyphs_clipped(
    vis::NcursesWriter *writer, const int32_t row, const int32_t column,
    const vis::ColorDefinition &color, const std::vector<Glyph> &glyphs,
    const int32_t clip_left, const int32_t clip_right,
    const std::vector<std::vector<uint8_t>> *occupied_cells,
    std::vector<RenderedGlyph> *rendered)
{
    auto cell = column;
    auto run_start = 0;
    auto run_end = 0;
    std::wstring run_text;

    const auto flush_run = [&]() {
        if (!run_text.empty())
        {
            writer->write_text(row, run_start, color, run_text);
            run_text.clear();
        }
    };

    for (const auto &glyph : glyphs)
    {
        if (cell >= clip_left && cell + glyph.width <= clip_right &&
            cells_are_free(occupied_cells, row, cell, glyph.width))
        {
            if (run_text.empty() || cell != run_end)
            {
                flush_run();
                run_start = cell;
                run_end = cell;
            }
            run_text.append(glyph.text);
            run_end += glyph.width;

            if (rendered != nullptr)
            {
                rendered->push_back(
                    RenderedGlyph{glyph.text, row, cell, glyph.width});
            }
        }
        else
        {
            flush_run();
        }
        cell += glyph.width;
    }
    flush_run();
}

bool vis::OverlayRenderer::draw_overlay(const vis::Settings &settings,
                                        const vis::OverlayMetadata &metadata,
                                        vis::NcursesWriter *writer,
                                        const std::vector<std::vector<uint8_t>>
                                            *occupied_cells)
{
    if (!settings.is_overlay_enabled() || writer == nullptr)
    {
        return false;
    }

    if (is_live_metadata(metadata))
    {
        std::vector<RenderedGlyph> rendered;
        auto drew_overlay = draw_marquee(settings, metadata, writer,
                                         occupied_cells, &rendered);
        drew_overlay =
            draw_progress(settings, metadata, writer, occupied_cells,
                          &rendered) ||
            drew_overlay;

        if (drew_overlay)
        {
            m_last_live_overlay = rendered;
            m_falling_overlay.clear();
            m_last_frame_was_live = true;
        }
        return drew_overlay;
    }

    if (m_last_frame_was_live && !m_last_live_overlay.empty() &&
        m_falling_overlay.empty())
    {
        start_fall();
    }
    m_last_frame_was_live = false;

    return draw_falling_overlay(settings, writer, occupied_cells);
}

bool vis::OverlayRenderer::has_fall_animation() const noexcept
{
    return !m_falling_overlay.empty() ||
           (m_last_frame_was_live && !m_last_live_overlay.empty());
}

bool vis::OverlayRenderer::is_fall_animation_active() const noexcept
{
    return !m_falling_overlay.empty();
}

bool vis::OverlayRenderer::draw_marquee(const vis::Settings &settings,
                                        const vis::OverlayMetadata &metadata,
                                        vis::NcursesWriter *writer)
{
    return draw_marquee(settings, metadata, writer, nullptr, nullptr);
}

bool vis::OverlayRenderer::draw_marquee(
    const vis::Settings &settings, const vis::OverlayMetadata &metadata,
    vis::NcursesWriter *writer,
    const std::vector<std::vector<uint8_t>> *occupied_cells,
    std::vector<RenderedGlyph> *rendered)
{
    if (!settings.is_overlay_enabled() ||
        !settings.is_overlay_marquee_enabled() || metadata.title.empty() ||
        writer == nullptr)
    {
        return false;
    }

    const auto win_width = NcursesUtils::get_window_width();
    const auto win_height = NcursesUtils::get_window_height();
    const auto row = static_cast<int32_t>(settings.get_overlay_marquee_row());
    if (win_width <= 0 || win_height <= 0 || row < 0 || row >= win_height)
    {
        return false;
    }

    if (metadata.title != m_last_title)
    {
        m_last_title = metadata.title;
        m_title_started_at = std::chrono::steady_clock::now();
    }

    int32_t title_width = 0;
    const auto glyphs =
        text_to_glyphs(utf8_to_wstring(metadata.title), &title_width);
    if (glyphs.empty() || title_width <= 0)
    {
        return false;
    }

    const auto has_boundary =
        settings.is_overlay_marquee_boundary_enabled() && win_width > 2;
    auto left_boundary_glyphs = std::vector<Glyph>{};
    auto right_boundary_glyphs = std::vector<Glyph>{};
    auto left_boundary_width = 0;
    auto right_boundary_width = 0;
    auto right_boundary_column = win_width;

    if (has_boundary)
    {
        left_boundary_glyphs = text_to_glyphs(
            configured_or_default(settings.get_overlay_marquee_boundary_left(),
                                  VisConstants::
                                      k_default_overlay_marquee_boundary_left),
            nullptr);
        right_boundary_glyphs = text_to_glyphs(
            configured_or_default(settings.get_overlay_marquee_boundary_right(),
                                  VisConstants::
                                      k_default_overlay_marquee_boundary_right),
            nullptr);
        left_boundary_width = glyphs_width(left_boundary_glyphs);
        right_boundary_width = glyphs_width(right_boundary_glyphs);
        right_boundary_column =
            std::max(left_boundary_width, win_width - right_boundary_width);
    }

    const auto marquee_left = has_boundary ? left_boundary_width : 0;
    const auto marquee_right =
        has_boundary ? std::max(marquee_left, right_boundary_column)
                     : win_width;
    const auto marquee_width = marquee_right - marquee_left;
    if (marquee_width <= 0)
    {
        return false;
    }

    const auto gap = static_cast<int32_t>(settings.get_overlay_marquee_gap());
    const auto track_width = title_width + std::max(gap, 1);
    const auto elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                      m_title_started_at)
            .count();
    const auto speed = settings.get_overlay_marquee_speed();
    const auto offset =
        speed > 0.0 ? static_cast<int32_t>(std::floor(elapsed * speed)) %
                          track_width
                    : 0;
    const auto color = writer->to_color_pair(0, 0, settings.get_colors(), true);

    for (auto repeat_start = marquee_left - offset; repeat_start < marquee_right;
         repeat_start += track_width)
    {
        draw_glyphs_clipped(writer, row, repeat_start, color, glyphs,
                            marquee_left, marquee_right, occupied_cells,
                            rendered);
    }

    if (has_boundary)
    {
        draw_glyphs(writer, row, 0, color, left_boundary_glyphs, win_width,
                    occupied_cells, rendered);
        draw_glyphs(writer, row, right_boundary_column, color,
                    right_boundary_glyphs, win_width, occupied_cells,
                    rendered);
    }

    return true;
}

bool vis::OverlayRenderer::draw_progress(const vis::Settings &settings,
                                         const vis::OverlayMetadata &metadata,
                                         vis::NcursesWriter *writer)
{
    return draw_progress(settings, metadata, writer, nullptr, nullptr);
}

bool vis::OverlayRenderer::draw_progress(
    const vis::Settings &settings, const vis::OverlayMetadata &metadata,
    vis::NcursesWriter *writer,
    const std::vector<std::vector<uint8_t>> *occupied_cells,
    std::vector<RenderedGlyph> *rendered)
{
    if (!settings.is_overlay_enabled() ||
        !settings.is_overlay_progress_enabled() || writer == nullptr)
    {
        return false;
    }

    const auto win_width = NcursesUtils::get_window_width();
    const auto win_height = NcursesUtils::get_window_height();
    const auto row = static_cast<int32_t>(settings.get_overlay_progress_row());
    if (win_width <= 0 || win_height <= 0 || row < 0 || row >= win_height)
    {
        return false;
    }

    const auto left = status_left(settings, metadata);
    const auto right = status_right(settings, metadata);
    auto left_glyphs = text_to_glyphs(left, nullptr);
    auto right_glyphs = text_to_glyphs(right, nullptr);
    const auto left_width = glyphs_width(left_glyphs);
    const auto right_width = glyphs_width(right_glyphs);
    const auto status_gap = left_width > 0 && right_width > 0 ? 1 : 0;
    const auto min_status_width = left_width + status_gap + right_width;
    const auto configured_bar_width =
        static_cast<int32_t>(settings.get_overlay_progress_width());
    const auto block_width =
        std::max(min_status_width, configured_bar_width);
    const auto bar = progress_bar(settings, metadata, block_width);
    if (left_glyphs.empty() && right_glyphs.empty() && bar.empty())
    {
        return false;
    }

    const auto color = writer->to_color_pair(0, 0, settings.get_colors(), true);
    const auto column =
        lowercase(settings.get_overlay_progress_align()) == k_align_right
            ? std::max(0, win_width - block_width)
            : 0;
    bool drew = false;

    if (!left_glyphs.empty())
    {
        draw_glyphs(writer, row, column, color, left_glyphs, win_width,
                    occupied_cells, rendered);
        drew = true;
    }

    if (!right_glyphs.empty())
    {
        draw_glyphs(writer, row, column + block_width - right_width, color,
                    right_glyphs, win_width, occupied_cells, rendered);
        drew = true;
    }

    const auto bar_row = row + 1;
    if (!bar.empty() && bar_row < win_height)
    {
        const auto glyphs = text_to_glyphs(bar, nullptr);
        draw_glyphs(writer, bar_row, column, color, glyphs, win_width,
                    occupied_cells, rendered);
        drew = true;
    }

    return drew;
}

void vis::OverlayRenderer::start_fall()
{
    m_falling_overlay.clear();
    m_fall_started_at = std::chrono::steady_clock::now();

    for (const auto &glyph : m_last_live_overlay)
    {
        const auto seed = glyph_seed(glyph);
        FallingGlyph falling_glyph;
        falling_glyph.glyph = glyph;
        falling_glyph.delay_seconds =
            static_cast<double>((seed / 7u) % 5u) * 0.015;
        falling_glyph.speed_rows_per_second =
            9.0 + static_cast<double>(seed % 6u) * 0.8;
        m_falling_overlay.push_back(falling_glyph);
    }
}

bool vis::OverlayRenderer::draw_falling_overlay(
    const vis::Settings &settings, vis::NcursesWriter *writer,
    const std::vector<std::vector<uint8_t>> *occupied_cells)
{
    if (writer == nullptr || m_falling_overlay.empty())
    {
        return false;
    }

    const auto win_width = NcursesUtils::get_window_width();
    const auto win_height = NcursesUtils::get_window_height();
    if (win_width <= 0 || win_height <= 0)
    {
        m_falling_overlay.clear();
        return false;
    }

    const auto elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                      m_fall_started_at)
            .count();
    const auto color = writer->to_color_pair(0, 0, settings.get_colors(), true);
    const auto last_visible_fall_row = win_height - 1;

    std::vector<FallingGlyph> still_falling;
    std::vector<RenderedGlyph> drawn_glyphs;
    bool drew = false;
    for (const auto &falling_glyph : m_falling_overlay)
    {
        const auto local_elapsed = elapsed - falling_glyph.delay_seconds;
        const auto row_offset =
            local_elapsed <= 0.0
                ? 0
                : static_cast<int32_t>(std::floor(
                      local_elapsed *
                      falling_glyph.speed_rows_per_second));
        const auto row = falling_glyph.glyph.row + row_offset;

        if (row < last_visible_fall_row)
        {
            if (row >= 0 &&
                falling_glyph.glyph.column + falling_glyph.glyph.width <
                    win_width &&
                cells_are_free(occupied_cells, row,
                               falling_glyph.glyph.column,
                               falling_glyph.glyph.width))
            {
                drawn_glyphs.push_back(RenderedGlyph{
                    falling_glyph.glyph.text, row,
                    falling_glyph.glyph.column, falling_glyph.glyph.width});
                drew = true;
            }
            still_falling.push_back(falling_glyph);
        }
    }

    std::sort(drawn_glyphs.begin(), drawn_glyphs.end(),
              [](const RenderedGlyph &a, const RenderedGlyph &b) {
                  if (a.row != b.row)
                  {
                      return a.row < b.row;
                  }
                  return a.column < b.column;
              });

    auto run_row = -1;
    auto run_start = 0;
    auto run_end = 0;
    std::wstring run_text;

    const auto flush_run = [&]() {
        if (!run_text.empty())
        {
            writer->write_text(run_row, run_start, color, run_text);
            run_text.clear();
        }
    };

    for (const auto &glyph : drawn_glyphs)
    {
        if (run_text.empty() || glyph.row != run_row ||
            glyph.column != run_end)
        {
            flush_run();
            run_row = glyph.row;
            run_start = glyph.column;
            run_end = glyph.column;
        }
        run_text.append(glyph.text);
        run_end += glyph.width;
    }
    flush_run();

    m_falling_overlay = still_falling;
    if (m_falling_overlay.empty())
    {
        m_last_live_overlay.clear();
    }

    return drew;
}
