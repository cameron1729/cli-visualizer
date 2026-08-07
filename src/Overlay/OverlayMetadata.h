/*
 * OverlayMetadata.h
 */

#ifndef _VIS_OVERLAY_METADATA_H
#define _VIS_OVERLAY_METADATA_H

#include <cstdint>
#include <string>
#include <vector>

namespace vis
{

struct OverlayMetadata
{
    std::string title;
    std::string category;
    std::string playback;
    std::string audio_output_kind;
    int64_t position_ms{0};
    int64_t duration_ms{0};
    bool flight_active{false};
    std::string flight_callsign;
    std::string flight_left_label;
    std::string flight_right_label;
    std::string flight_direction_label;
    std::string flight_target_label;
    std::string flight_markers;
    int64_t flight_progress_per_mille{0};
    int64_t flight_eta_seconds{-1};
    double flight_lat{0.0};
    double flight_lon{0.0};
    int64_t flight_alt_ft{0};
    int64_t flight_ground_speed_kt{0};
    int64_t flight_track_deg{0};
    bool baiyan_available{false};
    std::string baiyan_compact;
    std::string baiyan_expanded;

    bool empty() const noexcept
    {
        return title.empty() && category.empty() && playback.empty() &&
               audio_output_kind.empty() && position_ms == 0 &&
               duration_ms == 0 && !flight_active && flight_callsign.empty() &&
               !baiyan_available;
    }
};

struct StatusSegment
{
    std::string series;
    std::string text;
    std::string compact;
    std::string narrow;
    std::string severity;
    std::string state_key;
    bool full_width{false};

    bool empty() const noexcept
    {
        return text.empty();
    }
};

bool parse_overlay_metadata_json(const std::string &json,
                                 OverlayMetadata *metadata);
bool parse_status_segment_json(const std::string &json,
                               StatusSegment *segment);
bool parse_status_segments_ndjson(const std::string &ndjson,
                                  std::vector<StatusSegment> *segments);

} // namespace vis

#endif
