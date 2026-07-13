/*
 * OverlaySource.cpp
 */

#include "Overlay/OverlaySource.h"

#include "Overlay/AudioOutputInfo.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <sys/wait.h>

namespace
{
const uint32_t k_default_poll_ms = 1000;
const uint32_t k_max_failed_polls_before_clear = 3;
}

vis::OverlaySource::OverlaySource()
    : m_stop_requested{false}, m_enabled{false},
      m_poll_ms{k_default_poll_ms}
{
}

vis::OverlaySource::~OverlaySource()
{
    stop();
}

void vis::OverlaySource::configure(
    const std::shared_ptr<const vis::Settings> &settings)
{
    configure(settings != nullptr && settings->is_overlay_enabled() &&
                  settings->is_overlay_bgv_enabled(),
              settings != nullptr ? settings->get_overlay_bgv_command() : "",
              settings != nullptr ? settings->get_overlay_bgv_poll_ms()
                                  : k_default_poll_ms);
}

void vis::OverlaySource::configure(const bool enabled,
                                   const std::string &command,
                                   const uint32_t poll_ms)
{
    stop();

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_metadata = OverlayMetadata{};
        m_enabled = enabled && !command.empty();
        m_command = command;
        m_poll_ms = poll_ms;
        if (m_poll_ms == 0)
        {
            m_poll_ms = k_default_poll_ms;
        }
    }

    if (m_enabled)
    {
        start();
    }
}

vis::OverlayMetadata vis::OverlaySource::get_metadata() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_metadata;
}

void vis::OverlaySource::start()
{
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_stop_requested = false;
    }
    m_thread = std::thread{&OverlaySource::run, this};
}

void vis::OverlaySource::stop()
{
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_stop_requested = true;
    }
    m_condition.notify_all();

    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

bool vis::OverlaySource::poll_once(vis::OverlayMetadata *metadata) const
{
    std::string command;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        command = m_command;
    }

    if (command.empty())
    {
        return false;
    }

    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        return false;
    }

    std::string output;
    std::array<char, 512> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
           nullptr)
    {
        output.append(buffer.data());
    }

    const auto status = pclose(pipe);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        return false;
    }

    if (!parse_overlay_metadata_json(output, metadata))
    {
        return false;
    }

    metadata->audio_output_kind = detect_audio_output_kind();
    return true;
}

void vis::OverlaySource::run()
{
    uint32_t failed_polls = 0;

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            if (m_stop_requested)
            {
                return;
            }
        }

        OverlayMetadata metadata;
        if (poll_once(&metadata))
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_metadata = metadata;
            failed_polls = 0;
        }
        else
        {
            ++failed_polls;
            if (failed_polls >= k_max_failed_polls_before_clear)
            {
                std::lock_guard<std::mutex> lock{m_mutex};
                m_metadata = OverlayMetadata{};
            }
        }

        std::unique_lock<std::mutex> lock{m_mutex};
        if (m_condition.wait_for(
                lock, std::chrono::milliseconds(m_poll_ms),
                [this] { return m_stop_requested; }))
        {
            return;
        }
    }
}
