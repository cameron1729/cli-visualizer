/*
 * OverlayMetadata.h
 */

#ifndef _VIS_OVERLAY_METADATA_H
#define _VIS_OVERLAY_METADATA_H

#include <cstdint>
#include <string>

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

    bool empty() const noexcept
    {
        return title.empty() && category.empty() && playback.empty() &&
               audio_output_kind.empty() && position_ms == 0 &&
               duration_ms == 0;
    }
};

bool parse_overlay_metadata_json(const std::string &json,
                                 OverlayMetadata *metadata);

} // namespace vis

#endif
