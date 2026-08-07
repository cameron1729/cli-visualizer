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
#include <cstdlib>
#include <cwchar>
#include <locale>
#include <limits>
#include <stdexcept>

namespace
{

const std::string k_align_right{"right"};
const std::string k_audio_output_headphones{"headphones"};
const std::string k_audio_output_speakers{"speakers"};
const std::string k_playback_playing{"playing"};
const std::string k_playback_paused{"paused"};
const std::string k_playback_stopped{"stopped"};

double ease_in_out(const double progress)
{
    const auto clamped = std::min(1.0, std::max(0.0, progress));
    return clamped * clamped * (3.0 - 2.0 * clamped);
}

struct FlightMarker
{
    int64_t progress_per_mille{0};
    std::string target_label;
    int64_t eta_seconds{-1};
};

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool has_live_playback(const vis::OverlayMetadata &metadata)
{
    const auto playback = lowercase(metadata.playback);
    const auto has_playback_payload =
        !metadata.title.empty() || !metadata.playback.empty() ||
        metadata.duration_ms > 0 || metadata.position_ms > 0;

    return has_playback_payload && playback != k_playback_stopped;
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

int32_t wstring_cell_width(const std::wstring &text)
{
    int32_t width = 0;
    for (const auto ch : text)
    {
        const auto ch_width = static_cast<int32_t>(wcwidth(ch));
        if (ch_width > 0)
        {
            width += ch_width;
        }
        else if (ch_width < 0)
        {
            ++width;
        }
    }
    return width;
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

int32_t shared_slot_width(const vis::Settings &settings,
                          const vis::OverlayMetadata &playback_metadata,
                          const int32_t configured_width,
                          const int32_t win_width)
{
    if (configured_width > 0)
    {
        return std::min(configured_width, win_width);
    }

    const auto left_width = wstring_cell_width(
        status_left(settings, playback_metadata));
    const auto right_width = wstring_cell_width(
        status_right(settings, playback_metadata));
    const auto status_gap = left_width > 0 && right_width > 0 ? 1 : 0;
    const auto min_status_width = left_width + status_gap + right_width;
    const auto block_width = std::max(
        min_status_width,
        static_cast<int32_t>(settings.get_overlay_progress_width()));
    const auto gap = 2;

    return lowercase(settings.get_overlay_progress_align()) == k_align_right
               ? win_width - block_width - gap
               : win_width;
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

std::wstring format_eta(const int64_t seconds)
{
    if (seconds < 0)
    {
        return L"";
    }

    const auto hours = seconds / 3600;
    const auto minutes = (seconds % 3600) / 60;

    char buffer[32];
    if (hours > 0)
    {
        std::snprintf(buffer, sizeof(buffer), "%lldh%02lldm",
                      static_cast<long long>(hours),
                      static_cast<long long>(minutes));
    }
    else
    {
        std::snprintf(buffer, sizeof(buffer), "%lldm",
                      static_cast<long long>(minutes));
    }

    return ascii_to_wstring(buffer);
}

std::vector<std::string> split_string(const std::string &text,
                                      const char delimiter)
{
    std::vector<std::string> parts;
    std::string current;
    for (const auto ch : text)
    {
        if (ch == delimiter)
        {
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    return parts;
}

bool parse_int64(const std::string &text, int64_t *value)
{
    if (value == nullptr || text.empty())
    {
        return false;
    }

    char *end = nullptr;
    const auto parsed = std::strtoll(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0')
    {
        return false;
    }

    *value = parsed;
    return true;
}

std::vector<FlightMarker> flight_markers(
    const vis::OverlayMetadata &metadata)
{
    std::vector<FlightMarker> markers;

    for (const auto &entry : split_string(metadata.flight_markers, ';'))
    {
        if (entry.empty())
        {
            continue;
        }

        const auto parts = split_string(entry, '|');
        if (parts.size() < 2)
        {
            continue;
        }

        FlightMarker marker;
        if (!parse_int64(parts[0], &marker.progress_per_mille))
        {
            continue;
        }
        marker.target_label = parts[1];
        if (parts.size() >= 3)
        {
            int64_t eta_seconds = -1;
            if (parse_int64(parts[2], &eta_seconds))
            {
                marker.eta_seconds = eta_seconds;
            }
        }
        markers.push_back(marker);
    }

    if (markers.empty())
    {
        markers.push_back(FlightMarker{metadata.flight_progress_per_mille,
                                       metadata.flight_target_label,
                                       metadata.flight_eta_seconds});
    }

    return markers;
}

std::wstring flight_marker_plane(const vis::Settings &settings,
                                 const vis::OverlayMetadata &metadata,
                                 const std::string &target_label)
{
    const auto left_plane = configured_or_default(
        settings.get_overlay_flight_plane_left(),
        VisConstants::k_default_overlay_flight_plane_left);
    const auto right_plane = configured_or_default(
        settings.get_overlay_flight_plane_right(),
        VisConstants::k_default_overlay_flight_plane_right);

    const auto target = lowercase(target_label);
    if (!target.empty() && target == lowercase(metadata.flight_left_label))
    {
        return left_plane;
    }

    if (!target.empty() && target == lowercase(metadata.flight_right_label))
    {
        return right_plane;
    }

    return right_plane;
}

int32_t marker_position(const int32_t route_cells, const int32_t marker_width,
                        const int64_t progress_per_mille)
{
    const auto progress =
        std::max<int64_t>(0, std::min<int64_t>(1000, progress_per_mille));
    return static_cast<int32_t>(
        std::round((static_cast<double>(progress) / 1000.0) *
                   static_cast<double>(route_cells - marker_width)));
}

bool marker_fits(const std::vector<uint8_t> &occupied, const int32_t start,
                 const int32_t width)
{
    if (start < 0 || width <= 0 ||
        start + width > static_cast<int32_t>(occupied.size()))
    {
        return false;
    }

    for (auto i = start; i < start + width; ++i)
    {
        if (occupied[static_cast<size_t>(i)] != 0)
        {
            return false;
        }
    }

    return true;
}

int32_t find_marker_position(const std::vector<uint8_t> &occupied,
                             const int32_t preferred, const int32_t width)
{
    if (marker_fits(occupied, preferred, width))
    {
        return preferred;
    }

    for (auto offset = 1; offset < static_cast<int32_t>(occupied.size());
         ++offset)
    {
        const auto right = preferred + offset;
        if (marker_fits(occupied, right, width))
        {
            return right;
        }

        const auto left = preferred - offset;
        if (marker_fits(occupied, left, width))
        {
            return left;
        }
    }

    return -1;
}

std::wstring flight_route(const vis::Settings &settings,
                          const vis::OverlayMetadata &metadata,
                          const int32_t cell_width)
{
    if (!metadata.flight_active || metadata.flight_left_label.empty() ||
        metadata.flight_right_label.empty())
    {
        return L"";
    }

    const auto left = ascii_to_wstring(metadata.flight_left_label);
    const auto right = ascii_to_wstring(metadata.flight_right_label);
    const auto markers = flight_markers(metadata);
    auto suffix = std::wstring{};
    const auto eta = markers.size() == 1
                         ? format_eta(markers.front().eta_seconds)
                         : L"";
    if (!eta.empty())
    {
        suffix.push_back(L' ');
        suffix.append(eta);
    }

    const auto left_width = wstring_cell_width(left);
    const auto right_width = wstring_cell_width(right);
    const auto suffix_width = wstring_cell_width(suffix);
    auto max_marker_width = 1;
    for (const auto &marker : markers)
    {
        max_marker_width = std::max(
            max_marker_width,
            std::max(1, wstring_cell_width(flight_marker_plane(
                            settings, metadata, marker.target_label))));
    }
    const auto min_route_cells =
        std::max(3, max_marker_width * static_cast<int32_t>(markers.size()));
    const auto min_width =
        left_width + right_width + suffix_width + min_route_cells + 2;
    const auto total_width = std::max(min_width, cell_width);
    const auto route_cells =
        std::max(min_route_cells,
                 total_width - left_width - right_width - suffix_width - 2);

    std::vector<std::wstring> cells(static_cast<size_t>(route_cells),
                                    L"\u2500");
    std::vector<uint8_t> occupied(static_cast<size_t>(route_cells), 0);
    for (const auto &marker : markers)
    {
        const auto plane =
            flight_marker_plane(settings, metadata, marker.target_label);
        const auto plane_width = std::max(1, wstring_cell_width(plane));
        const auto preferred = marker_position(
            route_cells, plane_width, marker.progress_per_mille);
        const auto position =
            find_marker_position(occupied, preferred, plane_width);
        if (position < 0)
        {
            continue;
        }

        cells[static_cast<size_t>(position)] = plane;
        for (auto i = position; i < position + plane_width; ++i)
        {
            occupied[static_cast<size_t>(i)] = 1;
            if (i != position)
            {
                cells[static_cast<size_t>(i)] = L"";
            }
        }
    }

    std::wstring route;
    route.append(left);
    route.push_back(L' ');
    for (const auto &cell : cells)
    {
        route.append(cell);
    }
    route.push_back(L' ');
    route.append(right);
    route.append(suffix);

    return route;
}

vis::ColorDefinition status_color(const vis::Settings &settings,
                                  const std::string &severity)
{
    const auto &colors = settings.get_colors();
    if (colors.empty())
    {
        return vis::ColorDefinition{0, 0, 0, 0};
    }

    auto target_red = 220;
    auto target_green = 220;
    auto target_blue = 220;
    auto preferred_index = static_cast<vis::ColorIndex>(7);
    const auto normalized = lowercase(severity);
    if (normalized == "ok")
    {
        target_red = 45;
        target_green = 220;
        target_blue = 100;
        preferred_index = 2;
    }
    else if (normalized == "warning")
    {
        target_red = 255;
        target_green = 195;
        target_blue = 30;
        preferred_index = 3;
    }
    else if (normalized == "error")
    {
        target_red = 255;
        target_green = 55;
        target_blue = 70;
        preferred_index = 1;
    }

    const auto preferred = std::find_if(
        colors.begin(), colors.end(), [preferred_index](const auto &color) {
            return color.get_color_index() == preferred_index;
        });
    const auto has_rgb = std::any_of(colors.begin(), colors.end(),
                                     [](const auto &color) {
                                         return color.get_red() >= 0 &&
                                                color.get_green() >= 0 &&
                                                color.get_blue() >= 0;
                                     });
    if (!has_rgb)
    {
        return preferred != colors.end() ? *preferred : colors.front();
    }

    auto nearest = colors.begin();
    auto nearest_distance = std::numeric_limits<int64_t>::max();
    for (auto candidate = colors.begin(); candidate != colors.end();
         ++candidate)
    {
        if (candidate->get_red() < 0 || candidate->get_green() < 0 ||
            candidate->get_blue() < 0)
        {
            continue;
        }
        const auto red = static_cast<int64_t>(candidate->get_red()) - target_red;
        const auto green =
            static_cast<int64_t>(candidate->get_green()) - target_green;
        const auto blue =
            static_cast<int64_t>(candidate->get_blue()) - target_blue;
        const auto distance = red * red + green * green + blue * blue;
        if (distance < nearest_distance)
        {
            nearest = candidate;
            nearest_distance = distance;
        }
    }
    return *nearest;
}

} // namespace

std::vector<int32_t> vis::distribute_status_spacing(const size_t chunk_count,
                                                    const int32_t spare_width)
{
    if (chunk_count < 2)
    {
        return {};
    }

    const auto gap_count = chunk_count - 1;
    const auto available = std::max<int32_t>(0, spare_width);
    const auto base = available / static_cast<int32_t>(gap_count);
    const auto remainder = available % static_cast<int32_t>(gap_count);
    std::vector<int32_t> spacing(gap_count, base);

    auto remaining = remainder;
    if (gap_count % 2 == 1 && remaining % 2 == 1)
    {
        ++spacing[gap_count / 2];
        --remaining;
    }
    for (size_t index = 0; remaining >= 2; ++index)
    {
        ++spacing[index];
        ++spacing[gap_count - index - 1];
        remaining -= 2;
    }
    if (remaining == 1)
    {
        // Even gap counts cannot represent an odd spare width symmetrically.
        ++spacing[(gap_count - 1) / 2];
    }
    return spacing;
}

std::vector<int32_t> vis::aligned_status_columns(
    const std::vector<int32_t> &column_widths, const int32_t content_left,
    const int32_t total_width)
{
    if (column_widths.empty() || content_left < 0 ||
        content_left >= total_width)
    {
        return {};
    }

    auto columns_width = 0;
    for (const auto width : column_widths)
    {
        if (width <= 0)
        {
            return {};
        }
        columns_width += width;
    }
    const auto available_width = total_width - content_left;
    if (columns_width > available_width)
    {
        return {};
    }

    const auto spacing = distribute_status_spacing(
        column_widths.size(), available_width - columns_width);
    std::vector<int32_t> columns;
    columns.reserve(column_widths.size());
    auto column = content_left;
    for (size_t index = 0; index < column_widths.size(); ++index)
    {
        columns.push_back(column);
        column += column_widths[index];
        if (index < spacing.size())
        {
            column += spacing[index];
        }
    }
    return columns;
}

std::vector<std::vector<size_t>>
vis::pack_status_pages_with_adaptive_gaps(
    const std::vector<int32_t> &chunk_widths,
    const int32_t available_width, const int32_t preferred_gap)
{
    std::vector<std::vector<size_t>> pages;
    if (chunk_widths.empty() || available_width <= 0)
    {
        return pages;
    }

    const auto preferred = std::max<int32_t>(1, preferred_gap);
    constexpr int32_t minimum_gap = 1;
    std::vector<size_t> page;
    auto chunks_width = 0;

    for (size_t index = 0; index < chunk_widths.size(); ++index)
    {
        const auto width = chunk_widths[index];
        if (width <= 0)
        {
            continue;
        }
        if (width > available_width)
        {
            if (!page.empty())
            {
                pages.push_back(page);
                page.clear();
                chunks_width = 0;
            }
            pages.push_back({index});
            continue;
        }

        const auto gap_count = static_cast<int32_t>(page.size());
        const auto preferred_width =
            chunks_width + width + gap_count * preferred;
        const auto minimum_width =
            chunks_width + width + gap_count * minimum_gap;
        if (!page.empty() && preferred_width > available_width &&
            minimum_width > available_width)
        {
            pages.push_back(page);
            page.clear();
            chunks_width = 0;
        }

        page.push_back(index);
        chunks_width += width;
    }

    if (!page.empty())
    {
        pages.push_back(page);
    }
    return pages;
}

vis::StatusPageLayout vis::layout_status_pages(
    const std::vector<int32_t> &chunk_widths,
    const std::vector<bool> &unhealthy,
    const std::vector<int32_t> &severity_priorities,
    const int32_t available_width, const int32_t requested_gap)
{
    StatusPageLayout layout;
    if (chunk_widths.empty() || chunk_widths.size() != unhealthy.size() ||
        chunk_widths.size() != severity_priorities.size() ||
        available_width <= 0)
    {
        return layout;
    }

    const auto gap = std::max<int32_t>(1, requested_gap);
    std::vector<size_t> unhealthy_indices;
    std::vector<size_t> healthy_indices;
    for (size_t index = 0; index < chunk_widths.size(); ++index)
    {
        if (unhealthy[index])
        {
            unhealthy_indices.push_back(index);
        }
        else
        {
            healthy_indices.push_back(index);
        }
    }
    std::stable_sort(
        unhealthy_indices.begin(), unhealthy_indices.end(),
        [&](const size_t left, const size_t right) {
            return severity_priorities[left] < severity_priorities[right];
        });

    auto indices_width = [&](const std::vector<size_t> &indices) {
        int32_t width = 0;
        for (size_t index = 0; index < indices.size(); ++index)
        {
            width += chunk_widths[indices[index]];
            if (index + 1 < indices.size())
            {
                width += gap;
            }
        }
        return width;
    };
    auto pack_pages = [&](const std::vector<size_t> &indices,
                          const int32_t width) {
        std::vector<std::vector<size_t>> pages;
        std::vector<size_t> page;
        int32_t used = 0;
        for (const auto index : indices)
        {
            if (chunk_widths[index] > width)
            {
                if (!page.empty())
                {
                    pages.push_back(page);
                    page.clear();
                    used = 0;
                }
                pages.push_back({index});
                continue;
            }
            const auto required =
                chunk_widths[index] + (page.empty() ? 0 : gap);
            if (!page.empty() && used + required > width)
            {
                pages.push_back(page);
                page.clear();
                used = 0;
            }
            used += chunk_widths[index] + (page.empty() ? 0 : gap);
            page.push_back(index);
        }
        if (!page.empty())
        {
            pages.push_back(page);
        }
        return pages;
    };

    const auto unhealthy_width = indices_width(unhealthy_indices);
    if (!unhealthy_indices.empty() && unhealthy_width > available_width)
    {
        layout.unhealthy_overflow = true;
        layout.pages = pack_pages(unhealthy_indices, available_width);
        return layout;
    }

    layout.pinned = unhealthy_indices;
    auto healthy_width = available_width - unhealthy_width;
    if (!layout.pinned.empty() && !healthy_indices.empty())
    {
        healthy_width -= gap;
    }
    if (!healthy_indices.empty() && healthy_width > 0)
    {
        layout.pages = pack_pages(healthy_indices, healthy_width);
    }
    if (layout.pages.empty())
    {
        layout.pages.emplace_back();
    }
    return layout;
}

vis::StatusPageFrame vis::status_page_frame(
    const size_t page_count, const uint64_t elapsed_ms,
    const uint32_t hold_ms, const uint32_t transition_ms)
{
    StatusPageFrame frame;
    if (page_count <= 1)
    {
        return frame;
    }

    const auto cycle_ms =
        static_cast<uint64_t>(hold_ms) + transition_ms;
    if (cycle_ms == 0)
    {
        return frame;
    }
    frame.current_page =
        static_cast<size_t>((elapsed_ms / cycle_ms) % page_count);
    frame.next_page = (frame.current_page + 1) % page_count;
    const auto phase_ms = elapsed_ms % cycle_ms;
    if (transition_ms > 0 && phase_ms >= hold_ms)
    {
        frame.transitioning = true;
        frame.transition_progress =
            static_cast<double>(phase_ms - hold_ms) / transition_ms;
        frame.transition_progress =
            ease_in_out(frame.transition_progress);
    }
    return frame;
}

std::string vis::status_segments_signature(
    const std::vector<StatusSegment> &segments)
{
    std::string signature;
    for (const auto &segment : segments)
    {
        signature.append(segment.series);
        signature.push_back('\n');
        signature.append(segment.state_key.empty() ? segment.severity
                                                   : segment.state_key);
        signature.push_back('\n');
        signature.append(segment.severity);
        signature.push_back('\n');
        signature.append(segment.full_width ? "full-width" : "inline");
        signature.push_back('\n');
    }
    return signature;
}

std::string vis::status_family_carousel_signature(
    const std::vector<StatusSegment> &segments)
{
    std::string signature;
    for (const auto &segment : segments)
    {
        signature.append(segment.series);
        signature.push_back('\n');
        signature.append(segment.full_width ? "full-width" : "inline");
        signature.push_back('\n');
    }
    return signature;
}

vis::StatusFamilyRows vis::status_family_rows(
    const std::vector<StatusSegment> &segments)
{
    StatusFamilyRows rows;
    for (const auto &segment : segments)
    {
        auto family_segment = segment;
        const auto is_workplace =
            segment.series.compare(0, 3, "mwp") == 0;
        const auto marker = is_workplace ? 'W' : 'M';
        auto strip_marker = [&](std::string *label) {
            if (label != nullptr && label->size() > 1 &&
                (*label)[0] == marker &&
                std::isdigit(static_cast<unsigned char>((*label)[1])))
            {
                label->erase(0, 1);
            }
        };
        strip_marker(&family_segment.compact);
        strip_marker(&family_segment.narrow);

        if (is_workplace)
        {
            rows.workplace.push_back(family_segment);
        }
        else
        {
            rows.moodle.push_back(family_segment);
        }
    }
    return rows;
}

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

vis::SharedSlotOverlayLayout vis::shared_slot_overlay_layout(
    const vis::Settings &settings,
    const vis::OverlayMetadata &playback_metadata)
{
    const auto shares_playback_row = has_live_playback(playback_metadata);
    if (shares_playback_row)
    {
        return SharedSlotOverlayLayout{
            static_cast<int32_t>(settings.get_overlay_progress_row()), true};
    }

    const auto status_rows =
        lowercase(settings.get_overlay_status_layout()) == "families" ? 2 : 1;
    const auto row = settings.is_overlay_status_enabled()
                         ? static_cast<int32_t>(
                               settings.get_overlay_status_row()) +
                               status_rows
                         : 0;
    return SharedSlotOverlayLayout{row, false};
}

bool vis::baiyan_overlay_visible(
    const vis::OverlayMetadata &flight_metadata,
    const vis::OverlayMetadata &baiyan_metadata)
{
    return !flight_metadata.flight_active && baiyan_metadata.baiyan_available;
}

std::string vis::baiyan_overlay_text(
    const vis::OverlayMetadata &metadata, const bool compact)
{
    return metadata.baiyan_available
               ? (compact ? metadata.baiyan_compact
                          : metadata.baiyan_expanded)
               : "";
}

bool vis::OverlayRenderer::is_live_metadata(
    const vis::OverlayMetadata &metadata)
{
    return has_live_playback(metadata);
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

bool vis::OverlayRenderer::draw_flight_progress(
    const vis::Settings &settings, const vis::OverlayMetadata &metadata,
    const vis::OverlayMetadata &playback_metadata, vis::NcursesWriter *writer,
    const std::vector<std::vector<uint8_t>> *occupied_cells)
{
    if (!settings.is_overlay_enabled() ||
        !settings.is_overlay_flight_enabled() ||
        !settings.is_overlay_progress_enabled() || writer == nullptr ||
        !metadata.flight_active)
    {
        return false;
    }

    const auto win_width = NcursesUtils::get_window_width();
    const auto win_height = NcursesUtils::get_window_height();
    const auto layout =
        shared_slot_overlay_layout(settings, playback_metadata);
    const auto row = layout.row;
    if (win_width <= 0 || win_height <= 0 || row < 0 || row >= win_height)
    {
        return false;
    }

    auto route_width = layout.shares_playback_row
                           ? static_cast<int32_t>(
                                 settings.get_overlay_flight_width())
                           : win_width;
    if (layout.shares_playback_row)
    {
        route_width = shared_slot_width(
            settings, playback_metadata,
            static_cast<int32_t>(settings.get_overlay_flight_width()),
            win_width);
    }
    route_width = std::max(3, std::min(route_width, win_width));

    const auto route = flight_route(settings, metadata, route_width);
    if (route.empty())
    {
        return false;
    }

    const auto color = writer->to_color_pair(0, 0, settings.get_colors(), true);
    const auto glyphs = text_to_glyphs(route, nullptr);
    draw_glyphs(writer, row, 0, color, glyphs, win_width, occupied_cells,
                nullptr);
    return true;
}

bool vis::OverlayRenderer::draw_baiyan_status(
    const vis::Settings &settings, const vis::OverlayMetadata &metadata,
    const vis::OverlayMetadata &playback_metadata,
    vis::NcursesWriter *writer,
    const std::vector<std::vector<uint8_t>> *occupied_cells)
{
    if (!settings.is_overlay_enabled() ||
        !settings.is_overlay_baiyan_enabled() ||
        !settings.is_overlay_progress_enabled() || writer == nullptr ||
        !metadata.baiyan_available)
    {
        return false;
    }

    const auto win_width = NcursesUtils::get_window_width();
    const auto win_height = NcursesUtils::get_window_height();
    const auto layout =
        shared_slot_overlay_layout(settings, playback_metadata);
    if (win_width <= 0 || win_height <= 0 || layout.row < 0 ||
        layout.row >= win_height)
    {
        return false;
    }

    auto width = layout.shares_playback_row
                     ? shared_slot_width(
                           settings, playback_metadata,
                           static_cast<int32_t>(
                               settings.get_overlay_baiyan_width()),
                           win_width)
                     : win_width;
    width = std::max(1, std::min(width, win_width));
    const auto text = baiyan_overlay_text(
        metadata, layout.shares_playback_row);
    if (text.empty())
    {
        return false;
    }

    const auto color = writer->to_color_pair(0, 0, settings.get_colors(), true);
    const auto glyphs = text_to_glyphs(utf8_to_wstring(text), nullptr);
    draw_glyphs_clipped(writer, layout.row, 0, color, glyphs, 0, width,
                        occupied_cells, nullptr);
    return true;
}

bool vis::OverlayRenderer::draw_status(
    const vis::Settings &settings,
    const std::vector<vis::StatusSegment> &segments,
    vis::NcursesWriter *writer)
{
    m_status_transition_active = false;

    if (!settings.is_overlay_enabled() ||
        !settings.is_overlay_status_enabled() || writer == nullptr ||
        segments.empty())
    {
        return false;
    }

    const auto row = static_cast<int32_t>(settings.get_overlay_status_row());
    const auto status_layout =
        lowercase(settings.get_overlay_status_layout());
    if (status_layout == "family-carousel" &&
        !(segments.size() == 1 && segments.front().full_width))
    {
        return draw_status_family_carousel(
            settings, segments, writer, row,
            &m_status_family_carousel_state);
    }
    if (status_layout != "families" ||
        (segments.size() == 1 && segments.front().full_width))
    {
        if (status_layout == "families")
        {
            const auto win_height = NcursesUtils::get_window_height();
            if (row + 1 >= 0 && row + 1 < win_height)
            {
                writer->clear_line(row + 1, 0);
            }
        }
        return draw_status_row(settings, segments, writer, row, L"",
                               -1, {}, &m_status_single_state);
    }

    const auto family_rows = status_family_rows(segments);
    const auto win_width = NcursesUtils::get_window_width();
    const auto left = configured_or_default(
        settings.get_overlay_status_boundary_left(),
        VisConstants::k_default_overlay_status_boundary_left);
    const auto right = configured_or_default(
        settings.get_overlay_status_boundary_right(),
        VisConstants::k_default_overlay_status_boundary_right);
    const auto shared_content_left = std::min(
        win_width,
        std::max(
            wstring_cell_width(
                settings.get_overlay_status_moodle_prefix()),
            wstring_cell_width(
                settings.get_overlay_status_workplace_prefix())) +
            1);
    const auto column_count =
        std::max(family_rows.moodle.size(), family_rows.workplace.size());
    std::vector<int32_t> column_widths(column_count, 0);
    auto measure_columns = [&](const std::vector<StatusSegment> &row_segments) {
        for (size_t index = 0; index < row_segments.size(); ++index)
        {
            const auto label =
                left + utf8_to_wstring(row_segments[index].compact) + right;
            column_widths[index] =
                std::max(column_widths[index], wstring_cell_width(label));
        }
    };
    measure_columns(family_rows.moodle);
    measure_columns(family_rows.workplace);
    const auto aligned_columns = aligned_status_columns(
        column_widths, shared_content_left, win_width);

    const auto win_height = NcursesUtils::get_window_height();
    auto drew = false;
    if (family_rows.moodle.empty())
    {
        if (row >= 0 && row < win_height)
        {
            writer->clear_line(row, 0);
        }
    }
    else
    {
        drew = draw_status_row(
                   settings, family_rows.moodle, writer, row,
                   settings.get_overlay_status_moodle_prefix(),
                   shared_content_left, aligned_columns,
                   &m_status_moodle_state) ||
               drew;
    }

    const auto workplace_row = row + 1;
    if (family_rows.workplace.empty())
    {
        if (workplace_row >= 0 && workplace_row < win_height)
        {
            writer->clear_line(workplace_row, 0);
        }
    }
    else
    {
        drew = draw_status_row(
                   settings, family_rows.workplace, writer, workplace_row,
                   settings.get_overlay_status_workplace_prefix(),
                   shared_content_left, aligned_columns,
                   &m_status_workplace_state) ||
               drew;
    }
    return drew;
}

bool vis::OverlayRenderer::draw_status_family_carousel(
    const vis::Settings &settings,
    const std::vector<vis::StatusSegment> &segments,
    vis::NcursesWriter *writer, const int32_t row,
    StatusPagingState *paging_state)
{
    if (!settings.is_overlay_enabled() ||
        !settings.is_overlay_status_enabled() || writer == nullptr ||
        paging_state == nullptr || segments.empty())
    {
        return false;
    }

    const auto win_width = NcursesUtils::get_window_width();
    const auto win_height = NcursesUtils::get_window_height();
    if (win_width <= 0 || win_height <= 0 || row < 0 || row >= win_height)
    {
        return false;
    }

    struct FamilyChunk
    {
        std::vector<Glyph> glyphs;
        ColorDefinition color;
        int32_t width;
    };
    struct FamilyPage
    {
        std::vector<Glyph> prefix_glyphs;
        std::vector<FamilyChunk> chunks;
        std::vector<int32_t> columns;
    };

    const auto left = configured_or_default(
        settings.get_overlay_status_boundary_left(),
        VisConstants::k_default_overlay_status_boundary_left);
    const auto right = configured_or_default(
        settings.get_overlay_status_boundary_right(),
        VisConstants::k_default_overlay_status_boundary_right);
    const auto gap =
        std::max<int32_t>(1, settings.get_overlay_status_gap());
    const auto family_rows = status_family_rows(segments);
    std::vector<FamilyPage> pages;

    auto append_family_pages =
        [&](const std::vector<StatusSegment> &family_segments,
            const std::wstring &prefix) {
            if (family_segments.empty())
            {
                return;
            }

            const auto prefix_glyphs = text_to_glyphs(prefix, nullptr);
            const auto prefix_width = glyphs_width(prefix_glyphs);
            const auto content_left = std::min(
                win_width,
                prefix_width + (prefix.empty() ? 0 : 1));
            const auto content_width = win_width - content_left;
            std::vector<FamilyChunk> chunks;
            for (const auto &segment : family_segments)
            {
                if (segment.compact.empty())
                {
                    continue;
                }
                auto text =
                    left + utf8_to_wstring(segment.compact) + right;
                int32_t width = 0;
                auto glyphs = text_to_glyphs(text, &width);
                if (width > content_width && !segment.narrow.empty() &&
                    segment.narrow != segment.compact)
                {
                    text =
                        left + utf8_to_wstring(segment.narrow) + right;
                    glyphs = text_to_glyphs(text, &width);
                }
                chunks.push_back(FamilyChunk{
                    glyphs, status_color(settings, segment.severity), width});
            }

            if (chunks.empty() || content_width <= 0)
            {
                pages.push_back(FamilyPage{prefix_glyphs, {}, {}});
                return;
            }

            std::vector<int32_t> chunk_widths;
            chunk_widths.reserve(chunks.size());
            for (const auto &chunk : chunks)
            {
                chunk_widths.push_back(chunk.width);
            }
            const auto family_pages =
                pack_status_pages_with_adaptive_gaps(
                    chunk_widths, content_width, gap);

            for (const auto &indices : family_pages)
            {
                auto chunks_width = 0;
                for (const auto index : indices)
                {
                    chunks_width += chunks[index].width;
                }
                const auto spacing = distribute_status_spacing(
                    indices.size(), content_width - chunks_width);
                FamilyPage page;
                page.prefix_glyphs = prefix_glyphs;
                auto column = content_left;
                for (size_t position = 0; position < indices.size();
                     ++position)
                {
                    page.columns.push_back(column);
                    page.chunks.push_back(chunks[indices[position]]);
                    column += chunks[indices[position]].width;
                    if (position < spacing.size())
                    {
                        column += spacing[position];
                    }
                }
                pages.push_back(page);
            }
        };

    append_family_pages(
        family_rows.moodle,
        settings.get_overlay_status_moodle_prefix());
    append_family_pages(
        family_rows.workplace,
        settings.get_overlay_status_workplace_prefix());
    if (pages.empty())
    {
        return false;
    }

    const auto signature = status_family_carousel_signature(segments);
    if (signature != paging_state->signature)
    {
        paging_state->signature = signature;
        paging_state->started_at = std::chrono::steady_clock::now();
    }

    writer->clear_line(row, 0);
    const auto prefix_color = status_color(settings, "info");
    auto draw_page = [&](const FamilyPage &page, const int32_t offset) {
        draw_glyphs_clipped(writer, row, offset, prefix_color,
                            page.prefix_glyphs, 0, win_width, nullptr,
                            nullptr);
        for (size_t index = 0; index < page.chunks.size(); ++index)
        {
            draw_glyphs_clipped(
                writer, row, offset + page.columns[index],
                page.chunks[index].color, page.chunks[index].glyphs, 0,
                win_width, nullptr, nullptr);
        }
    };

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() -
                          paging_state->started_at)
                          .count();
    elapsed_ms = std::max<int64_t>(0, elapsed_ms);
    const auto frame = status_page_frame(
        pages.size(), static_cast<uint64_t>(elapsed_ms),
        settings.get_overlay_status_page_hold_ms(),
        settings.get_overlay_status_page_transition_ms());
    m_status_transition_active =
        m_status_transition_active || frame.transitioning;
    if (!frame.transitioning)
    {
        draw_page(pages[frame.current_page], 0);
        return true;
    }

    const auto offset = static_cast<int32_t>(
        std::lround(frame.transition_progress * win_width));
    draw_page(pages[frame.current_page], -offset);
    draw_page(pages[frame.next_page], win_width - offset);
    return true;
}

bool vis::OverlayRenderer::draw_status_row(
    const vis::Settings &settings,
    const std::vector<vis::StatusSegment> &segments,
    vis::NcursesWriter *writer, const int32_t row,
    const std::wstring &prefix, const int32_t content_left_override,
    const std::vector<int32_t> &aligned_columns,
    StatusPagingState *paging_state)
{
    if (!settings.is_overlay_enabled() ||
        !settings.is_overlay_status_enabled() || writer == nullptr ||
        paging_state == nullptr || segments.empty())
    {
        return false;
    }

    const auto win_width = NcursesUtils::get_window_width();
    const auto win_height = NcursesUtils::get_window_height();
    if (win_width <= 0 || win_height <= 0 || row < 0 || row >= win_height)
    {
        return false;
    }

    struct StatusChunk
    {
        std::vector<Glyph> glyphs;
        ColorDefinition color;
        int32_t width;
        bool unhealthy;
        int32_t severity_priority;
    };

    const auto left = configured_or_default(
        settings.get_overlay_status_boundary_left(),
        VisConstants::k_default_overlay_status_boundary_left);
    const auto right = configured_or_default(
        settings.get_overlay_status_boundary_right(),
        VisConstants::k_default_overlay_status_boundary_right);

    const auto signature = status_segments_signature(segments);
    if (signature != paging_state->signature)
    {
        paging_state->signature = signature;
        paging_state->started_at = std::chrono::steady_clock::now();
    }

    if (segments.size() == 1 && segments.front().full_width)
    {
        const auto &segment = segments.front();
        auto left_glyphs = text_to_glyphs(left, nullptr);
        auto right_glyphs = text_to_glyphs(right, nullptr);
        auto label_glyphs = text_to_glyphs(
            utf8_to_wstring(segment.compact), nullptr);
        const auto left_width = glyphs_width(left_glyphs);
        const auto right_width = glyphs_width(right_glyphs);
        auto label_width = glyphs_width(label_glyphs);

        if (left_width + label_width + right_width > win_width)
        {
            label_glyphs = text_to_glyphs(
                utf8_to_wstring(segment.narrow), nullptr);
            label_width = glyphs_width(label_glyphs);
        }

        const auto right_column =
            std::max(left_width, win_width - right_width);
        const auto available_label_width = right_column - left_width;
        const auto label_column = left_width + std::max(
            0, (available_label_width - label_width) / 2);
        const auto color = status_color(settings, segment.severity);

        writer->clear_line(row, 0);
        draw_glyphs(writer, row, 0, color, left_glyphs, win_width, nullptr,
                    nullptr);
        draw_glyphs_clipped(writer, row, label_column, color, label_glyphs,
                            left_width, right_column, nullptr, nullptr);
        draw_glyphs(writer, row, right_column, color, right_glyphs, win_width,
                    nullptr, nullptr);
        return true;
    }

    writer->clear_line(row, 0);
    const auto prefix_glyphs = text_to_glyphs(prefix, nullptr);
    const auto prefix_width = glyphs_width(prefix_glyphs);
    const auto natural_content_left =
        prefix_width + (prefix.empty() ? 0 : 1);
    const auto requested_content_left =
        content_left_override >= 0
            ? std::max(natural_content_left, content_left_override)
            : natural_content_left;
    const auto content_left =
        std::min(win_width, requested_content_left);
    const auto content_width = win_width - content_left;
    if (!prefix_glyphs.empty())
    {
        const auto color = status_color(settings, "info");
        draw_glyphs_clipped(writer, row, 0, color, prefix_glyphs, 0,
                            win_width, nullptr, nullptr);
    }
    if (content_width <= 0)
    {
        return !prefix.empty();
    }

    std::vector<StatusChunk> chunks;
    for (const auto &segment : segments)
    {
        if (segment.compact.empty())
        {
            continue;
        }

        auto text = left + utf8_to_wstring(segment.compact) + right;
        int32_t width = 0;
        auto glyphs = text_to_glyphs(text, &width);
        if (width > content_width && !segment.narrow.empty() &&
            segment.narrow != segment.compact)
        {
            text = left + utf8_to_wstring(segment.narrow) + right;
            glyphs = text_to_glyphs(text, &width);
        }

        const auto severity = lowercase(segment.severity);
        auto priority = 3;
        if (severity == "error")
        {
            priority = 0;
        }
        else if (severity == "warning")
        {
            priority = 1;
        }
        else if (severity == "stale")
        {
            priority = 2;
        }
        chunks.push_back(StatusChunk{
            glyphs, status_color(settings, segment.severity), width,
            severity != "ok", priority});
    }
    if (chunks.empty())
    {
        return !prefix.empty();
    }

    auto total_width = 0;
    for (const auto &chunk : chunks)
    {
        total_width += chunk.width;
    }

    auto draw_indices = [&](const std::vector<size_t> &indices,
                            const int32_t start, const int32_t clip_left,
                            const int32_t clip_right,
                            const std::vector<int32_t> &spacing) {
        auto column = start;
        for (size_t position = 0; position < indices.size(); ++position)
        {
            const auto index = indices[position];
            draw_glyphs_clipped(
                writer, row, column, chunks[index].color,
                chunks[index].glyphs, clip_left, clip_right, nullptr, nullptr);
            column += chunks[index].width;
            if (position < spacing.size())
            {
                column += spacing[position];
            }
        }
    };

    if (aligned_columns.size() >= chunks.size())
    {
        const std::vector<int32_t> no_spacing;
        for (size_t index = 0; index < chunks.size(); ++index)
        {
            draw_indices({index}, aligned_columns[index], content_left,
                         win_width, no_spacing);
        }
        return true;
    }

    if (total_width <= content_width)
    {
        std::vector<size_t> indices(chunks.size());
        for (size_t index = 0; index < indices.size(); ++index)
        {
            indices[index] = index;
        }
        const auto spacing = distribute_status_spacing(
            chunks.size(), content_width - total_width);
        draw_indices(indices, content_left, content_left, win_width, spacing);
        return true;
    }

    const auto gap =
        std::max<int32_t>(1, settings.get_overlay_status_gap());
    std::vector<int32_t> widths;
    std::vector<bool> unhealthy;
    std::vector<int32_t> priorities;
    widths.reserve(chunks.size());
    unhealthy.reserve(chunks.size());
    priorities.reserve(chunks.size());
    for (const auto &chunk : chunks)
    {
        widths.push_back(chunk.width);
        unhealthy.push_back(chunk.unhealthy);
        priorities.push_back(chunk.severity_priority);
    }

    const auto layout =
        layout_status_pages(widths, unhealthy, priorities, content_width, gap);
    if (layout.pages.empty())
    {
        return false;
    }

    auto pinned_width = 0;
    for (size_t position = 0; position < layout.pinned.size(); ++position)
    {
        pinned_width += chunks[layout.pinned[position]].width;
        if (position + 1 < layout.pinned.size())
        {
            pinned_width += gap;
        }
    }
    if (!layout.pinned.empty())
    {
        std::vector<int32_t> pinned_spacing(
            layout.pinned.size() > 1 ? layout.pinned.size() - 1 : 0, gap);
        draw_indices(layout.pinned, content_left, content_left, win_width,
                     pinned_spacing);
    }

    auto page_left = content_left + pinned_width;
    if (!layout.pinned.empty())
    {
        page_left += gap;
    }
    page_left = std::min(page_left, win_width);
    const auto page_width = win_width - page_left;
    if (page_width <= 0)
    {
        return true;
    }

    auto draw_page = [&](const std::vector<size_t> &page,
                         const int32_t start) {
        if (page.empty())
        {
            return;
        }
        auto width = 0;
        for (const auto index : page)
        {
            width += chunks[index].width;
        }
        const auto spacing =
            distribute_status_spacing(page.size(), page_width - width);
        draw_indices(page, start, page_left, win_width, spacing);
    };

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() -
                          paging_state->started_at)
                          .count();
    elapsed_ms = std::max<int64_t>(0, elapsed_ms);
    const auto frame = status_page_frame(
        layout.pages.size(), static_cast<uint64_t>(elapsed_ms),
        settings.get_overlay_status_page_hold_ms(),
        settings.get_overlay_status_page_transition_ms());
    m_status_transition_active =
        m_status_transition_active || frame.transitioning;
    if (!frame.transitioning)
    {
        draw_page(layout.pages[frame.current_page], page_left);
        return true;
    }

    const auto offset = static_cast<int32_t>(
        std::lround(frame.transition_progress * page_width));
    draw_page(layout.pages[frame.current_page], page_left - offset);
    draw_page(layout.pages[frame.next_page],
              page_left + page_width - offset);
    return true;
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
