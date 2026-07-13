/*
 * OverlayMetadata.cpp
 */

#include "Overlay/OverlayMetadata.h"

#include <cctype>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>

namespace
{

void skip_ws(const std::string &json, size_t *pos)
{
    while (*pos < json.size() &&
           std::isspace(static_cast<unsigned char>(json[*pos])) != 0)
    {
        ++(*pos);
    }
}

bool consume(const std::string &json, size_t *pos, char expected)
{
    skip_ws(json, pos);
    if (*pos >= json.size() || json[*pos] != expected)
    {
        return false;
    }
    ++(*pos);
    return true;
}

bool parse_json_string(const std::string &json, size_t *pos,
                       std::string *value)
{
    skip_ws(json, pos);
    if (*pos >= json.size() || json[*pos] != '"')
    {
        return false;
    }
    ++(*pos);

    value->clear();
    while (*pos < json.size())
    {
        const auto ch = json[*pos];
        ++(*pos);

        if (ch == '"')
        {
            return true;
        }

        if (ch != '\\')
        {
            value->push_back(ch);
            continue;
        }

        if (*pos >= json.size())
        {
            return false;
        }

        const auto escaped = json[*pos];
        ++(*pos);
        switch (escaped)
        {
        case '"':
        case '\\':
        case '/':
            value->push_back(escaped);
            break;
        case 'b':
            value->push_back('\b');
            break;
        case 'f':
            value->push_back('\f');
            break;
        case 'n':
            value->push_back('\n');
            break;
        case 'r':
            value->push_back('\r');
            break;
        case 't':
            value->push_back('\t');
            break;
        default:
            return false;
        }
    }

    return false;
}

bool parse_json_int(const std::string &json, size_t *pos, int64_t *value)
{
    skip_ws(json, pos);
    if (*pos >= json.size())
    {
        return false;
    }

    const auto start = *pos;
    if (json[*pos] == '-')
    {
        ++(*pos);
    }

    if (*pos >= json.size() ||
        std::isdigit(static_cast<unsigned char>(json[*pos])) == 0)
    {
        return false;
    }

    while (*pos < json.size() &&
           std::isdigit(static_cast<unsigned char>(json[*pos])) != 0)
    {
        ++(*pos);
    }

    char *end = nullptr;
    const auto parsed = std::strtoll(json.c_str() + start, &end, 10);
    if (end != json.c_str() + *pos)
    {
        return false;
    }

    *value = parsed;
    return true;
}

bool parse_json_double(const std::string &json, size_t *pos, double *value)
{
    skip_ws(json, pos);
    if (*pos >= json.size())
    {
        return false;
    }

    const auto start = *pos;
    if (json[*pos] == '-')
    {
        ++(*pos);
    }

    if (*pos >= json.size() ||
        std::isdigit(static_cast<unsigned char>(json[*pos])) == 0)
    {
        return false;
    }

    while (*pos < json.size() &&
           std::isdigit(static_cast<unsigned char>(json[*pos])) != 0)
    {
        ++(*pos);
    }

    if (*pos < json.size() && json[*pos] == '.')
    {
        ++(*pos);
        if (*pos >= json.size() ||
            std::isdigit(static_cast<unsigned char>(json[*pos])) == 0)
        {
            return false;
        }
        while (*pos < json.size() &&
               std::isdigit(static_cast<unsigned char>(json[*pos])) != 0)
        {
            ++(*pos);
        }
    }

    if (*pos < json.size() && (json[*pos] == 'e' || json[*pos] == 'E'))
    {
        ++(*pos);
        if (*pos < json.size() && (json[*pos] == '-' || json[*pos] == '+'))
        {
            ++(*pos);
        }
        if (*pos >= json.size() ||
            std::isdigit(static_cast<unsigned char>(json[*pos])) == 0)
        {
            return false;
        }
        while (*pos < json.size() &&
               std::isdigit(static_cast<unsigned char>(json[*pos])) != 0)
        {
            ++(*pos);
        }
    }

    char *end = nullptr;
    const auto parsed = std::strtod(json.c_str() + start, &end);
    if (end != json.c_str() + *pos)
    {
        return false;
    }

    *value = parsed;
    return true;
}

bool skip_json_literal(const std::string &json, size_t *pos,
                       const std::string &literal)
{
    skip_ws(json, pos);
    if (json.compare(*pos, literal.size(), literal) != 0)
    {
        return false;
    }
    *pos += literal.size();
    return true;
}

bool parse_json_bool(const std::string &json, size_t *pos, bool *value)
{
    if (skip_json_literal(json, pos, "true"))
    {
        *value = true;
        return true;
    }

    if (skip_json_literal(json, pos, "false"))
    {
        *value = false;
        return true;
    }

    return false;
}

bool skip_json_value(const std::string &json, size_t *pos)
{
    skip_ws(json, pos);
    if (*pos >= json.size())
    {
        return false;
    }

    if (json[*pos] == '"')
    {
        std::string unused;
        return parse_json_string(json, pos, &unused);
    }

    if (json[*pos] == '-' ||
        std::isdigit(static_cast<unsigned char>(json[*pos])) != 0)
    {
        double unused = 0.0;
        return parse_json_double(json, pos, &unused);
    }

    return skip_json_literal(json, pos, "true") ||
           skip_json_literal(json, pos, "false") ||
           skip_json_literal(json, pos, "null");
}

} // namespace

bool vis::parse_overlay_metadata_json(const std::string &json,
                                      vis::OverlayMetadata *metadata)
{
    if (metadata == nullptr)
    {
        return false;
    }

    OverlayMetadata parsed;
    size_t pos = 0;

    if (!consume(json, &pos, '{'))
    {
        return false;
    }

    skip_ws(json, &pos);
    if (pos < json.size() && json[pos] == '}')
    {
        *metadata = parsed;
        return true;
    }

    while (pos < json.size())
    {
        std::string key;
        if (!parse_json_string(json, &pos, &key) || !consume(json, &pos, ':'))
        {
            return false;
        }

        if (key == "title")
        {
            if (!parse_json_string(json, &pos, &parsed.title))
            {
                return false;
            }
        }
        else if (key == "category")
        {
            if (!parse_json_string(json, &pos, &parsed.category))
            {
                return false;
            }
        }
        else if (key == "playback")
        {
            if (!parse_json_string(json, &pos, &parsed.playback))
            {
                return false;
            }
        }
        else if (key == "audio_output_kind")
        {
            if (!parse_json_string(json, &pos, &parsed.audio_output_kind))
            {
                return false;
            }
        }
        else if (key == "position_ms")
        {
            if (!parse_json_int(json, &pos, &parsed.position_ms))
            {
                return false;
            }
        }
        else if (key == "duration_ms")
        {
            if (!parse_json_int(json, &pos, &parsed.duration_ms))
            {
                return false;
            }
        }
        else if (key == "flight_active")
        {
            if (!parse_json_bool(json, &pos, &parsed.flight_active))
            {
                return false;
            }
        }
        else if (key == "flight_callsign")
        {
            if (!parse_json_string(json, &pos, &parsed.flight_callsign))
            {
                return false;
            }
        }
        else if (key == "flight_left_label")
        {
            if (!parse_json_string(json, &pos, &parsed.flight_left_label))
            {
                return false;
            }
        }
        else if (key == "flight_right_label")
        {
            if (!parse_json_string(json, &pos, &parsed.flight_right_label))
            {
                return false;
            }
        }
        else if (key == "flight_direction_label")
        {
            if (!parse_json_string(json, &pos, &parsed.flight_direction_label))
            {
                return false;
            }
        }
        else if (key == "flight_target_label")
        {
            if (!parse_json_string(json, &pos, &parsed.flight_target_label))
            {
                return false;
            }
        }
        else if (key == "flight_markers")
        {
            if (!parse_json_string(json, &pos, &parsed.flight_markers))
            {
                return false;
            }
        }
        else if (key == "flight_progress_per_mille")
        {
            if (!parse_json_int(json, &pos,
                                &parsed.flight_progress_per_mille))
            {
                return false;
            }
        }
        else if (key == "flight_eta_seconds")
        {
            if (!parse_json_int(json, &pos, &parsed.flight_eta_seconds))
            {
                return false;
            }
        }
        else if (key == "flight_lat")
        {
            if (!parse_json_double(json, &pos, &parsed.flight_lat))
            {
                return false;
            }
        }
        else if (key == "flight_lon")
        {
            if (!parse_json_double(json, &pos, &parsed.flight_lon))
            {
                return false;
            }
        }
        else if (key == "flight_alt_ft")
        {
            if (!parse_json_int(json, &pos, &parsed.flight_alt_ft))
            {
                return false;
            }
        }
        else if (key == "flight_ground_speed_kt")
        {
            if (!parse_json_int(json, &pos,
                                &parsed.flight_ground_speed_kt))
            {
                return false;
            }
        }
        else if (key == "flight_track_deg")
        {
            if (!parse_json_int(json, &pos, &parsed.flight_track_deg))
            {
                return false;
            }
        }
        else if (!skip_json_value(json, &pos))
        {
            return false;
        }

        skip_ws(json, &pos);
        if (pos < json.size() && json[pos] == ',')
        {
            ++pos;
            continue;
        }

        if (pos < json.size() && json[pos] == '}')
        {
            ++pos;
            skip_ws(json, &pos);
            if (pos != json.size())
            {
                return false;
            }

            *metadata = parsed;
            return true;
        }

        return false;
    }

    return false;
}

bool vis::parse_status_segment_json(const std::string &json,
                                    vis::StatusSegment *segment)
{
    if (segment == nullptr)
    {
        return false;
    }

    StatusSegment parsed;
    size_t pos = 0;
    if (!consume(json, &pos, '{'))
    {
        return false;
    }

    skip_ws(json, &pos);
    if (pos < json.size() && json[pos] == '}')
    {
        return false;
    }

    while (pos < json.size())
    {
        std::string key;
        if (!parse_json_string(json, &pos, &key) || !consume(json, &pos, ':'))
        {
            return false;
        }

        if (key == "series")
        {
            if (!parse_json_string(json, &pos, &parsed.series))
            {
                return false;
            }
        }
        else if (key == "text")
        {
            if (!parse_json_string(json, &pos, &parsed.text))
            {
                return false;
            }
        }
        else if (key == "compact")
        {
            if (!parse_json_string(json, &pos, &parsed.compact))
            {
                return false;
            }
        }
        else if (key == "narrow")
        {
            if (!parse_json_string(json, &pos, &parsed.narrow))
            {
                return false;
            }
        }
        else if (key == "severity")
        {
            if (!parse_json_string(json, &pos, &parsed.severity))
            {
                return false;
            }
        }
        else if (!skip_json_value(json, &pos))
        {
            return false;
        }

        skip_ws(json, &pos);
        if (pos < json.size() && json[pos] == ',')
        {
            ++pos;
            continue;
        }
        if (pos < json.size() && json[pos] == '}')
        {
            ++pos;
            skip_ws(json, &pos);
            if (pos != json.size() || parsed.text.empty())
            {
                return false;
            }
            if (parsed.compact.empty())
            {
                parsed.compact = parsed.text;
            }
            if (parsed.narrow.empty())
            {
                parsed.narrow = parsed.compact;
            }
            if (parsed.severity.empty())
            {
                parsed.severity = "info";
            }
            *segment = parsed;
            return true;
        }
        return false;
    }

    return false;
}

bool vis::parse_status_segments_ndjson(
    const std::string &ndjson, std::vector<vis::StatusSegment> *segments)
{
    if (segments == nullptr)
    {
        return false;
    }

    std::istringstream lines{ndjson};
    std::string line;
    std::vector<StatusSegment> parsed;
    while (std::getline(lines, line))
    {
        if (line.find_first_not_of(" \t\r") == std::string::npos)
        {
            continue;
        }
        StatusSegment segment;
        if (!parse_status_segment_json(line, &segment))
        {
            return false;
        }
        parsed.push_back(segment);
    }
    if (parsed.empty())
    {
        return false;
    }

    *segments = parsed;
    return true;
}
