/*
 * AudioOutputInfo.cpp
 */

#include "Overlay/AudioOutputInfo.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#ifdef _ENABLE_PULSE
#include <pulse/pulseaudio.h>
#endif

namespace
{

const std::string k_output_headphones{"headphones"};
const std::string k_output_speakers{"speakers"};
const std::vector<std::string> k_headphone_terms{
    "headphone", "headphones", "headset", "earbud", "earbuds"};

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool contains_any(const std::string &haystack,
                  const std::vector<std::string> &needles)
{
    for (const auto &needle : needles)
    {
        if (haystack.find(needle) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

std::string classify_output_text(const std::string &text)
{
    const auto lower = lowercase(text);

    if (contains_any(lower, k_headphone_terms))
    {
        return k_output_headphones;
    }

    if (contains_any(lower,
                     {"speaker", "speakers", "hdmi", "displayport",
                      "lineout", "line-out", "analog-output-speaker"}))
    {
        return k_output_speakers;
    }

    return "";
}

bool is_generic_analog_output(const std::string &text)
{
    const auto lower = lowercase(text);
    return lower.find("analog-output") != std::string::npos ||
           lower.find("analog output") != std::string::npos;
}

#ifdef _ENABLE_PULSE
struct DetectionState
{
    pa_mainloop *mainloop{nullptr};
    std::string detected_kind;
};

void append_if_present(std::string *text, const char *value)
{
    if (text == nullptr || value == nullptr)
    {
        return;
    }

    if (!text->empty())
    {
        text->push_back(' ');
    }
    text->append(value);
}

void sink_info_callback(pa_context *, const pa_sink_info *info, int eol,
                        void *userdata)
{
    auto *state = reinterpret_cast<DetectionState *>(userdata);
    if (state == nullptr)
    {
        return;
    }

    if (eol > 0)
    {
        pa_mainloop_quit(state->mainloop, 0);
        return;
    }

    if (info == nullptr)
    {
        return;
    }

    std::string text;
    std::string port_text;
    append_if_present(&text, info->name);
    append_if_present(&text, info->description);
    append_if_present(&text, pa_proplist_gets(info->proplist,
                                              "device.description"));
    append_if_present(&text, pa_proplist_gets(info->proplist,
                                              "device.product.name"));
    append_if_present(&text, pa_proplist_gets(info->proplist,
                                              "device.profile.description"));
    append_if_present(&text, pa_proplist_gets(info->proplist,
                                              "alsa.card_name"));
    append_if_present(&text, pa_proplist_gets(info->proplist, "alsa.name"));
    if (info->active_port != nullptr)
    {
        append_if_present(&port_text, info->active_port->name);
        append_if_present(&port_text, info->active_port->description);
    }

    state->detected_kind = classify_output_text(port_text);
    if (!state->detected_kind.empty())
    {
        return;
    }

    state->detected_kind = classify_output_text(text);
    if (!state->detected_kind.empty())
    {
        return;
    }

    if (is_generic_analog_output(port_text) &&
        !contains_any(lowercase(text), k_headphone_terms))
    {
        state->detected_kind = k_output_speakers;
        return;
    }

    state->detected_kind = k_output_speakers;
}

void server_info_callback(pa_context *context, const pa_server_info *info,
                          void *userdata)
{
    auto *state = reinterpret_cast<DetectionState *>(userdata);
    if (state == nullptr)
    {
        return;
    }

    if (context == nullptr || info == nullptr ||
        info->default_sink_name == nullptr)
    {
        pa_mainloop_quit(state->mainloop, 0);
        return;
    }

    auto *operation = pa_context_get_sink_info_by_name(
        context, info->default_sink_name, sink_info_callback, userdata);
    if (operation == nullptr)
    {
        pa_mainloop_quit(state->mainloop, 0);
        return;
    }

    pa_operation_unref(operation);
}

void context_state_callback(pa_context *context, void *userdata)
{
    auto *state = reinterpret_cast<DetectionState *>(userdata);
    if (context == nullptr || state == nullptr)
    {
        return;
    }

    switch (pa_context_get_state(context))
    {
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
        break;

    case PA_CONTEXT_READY:
    {
        auto *operation =
            pa_context_get_server_info(context, server_info_callback, userdata);
        if (operation == nullptr)
        {
            pa_mainloop_quit(state->mainloop, 0);
            return;
        }
        pa_operation_unref(operation);
        break;
    }

    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        pa_mainloop_quit(state->mainloop, 0);
        break;
    }
}
#endif

} // namespace

std::string vis::detect_audio_output_kind()
{
#ifdef _ENABLE_PULSE
    DetectionState state;
    state.mainloop = pa_mainloop_new();
    if (state.mainloop == nullptr)
    {
        return k_output_speakers;
    }

    auto *mainloop_api = pa_mainloop_get_api(state.mainloop);
    auto *context = pa_context_new(mainloop_api, "vis output info");
    if (context == nullptr)
    {
        pa_mainloop_free(state.mainloop);
        return k_output_speakers;
    }

    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
    {
        pa_context_unref(context);
        pa_mainloop_free(state.mainloop);
        return k_output_speakers;
    }

    pa_context_set_state_callback(context, context_state_callback, &state);

    int ret = 0;
    pa_mainloop_run(state.mainloop, &ret);

    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_mainloop_free(state.mainloop);

    return state.detected_kind.empty() ? k_output_speakers
                                       : state.detected_kind;
#else
    return k_output_speakers;
#endif
}
