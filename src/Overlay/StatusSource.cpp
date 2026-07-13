/*
 * StatusSource.cpp
 */

#include "Overlay/StatusSource.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <sys/wait.h>

namespace
{
const uint32_t k_default_poll_ms = 60000;
const uint32_t k_max_failed_polls_before_clear = 3;
}

vis::StatusSource::StatusSource()
    : m_stop_requested{false}, m_enabled{false},
      m_poll_ms{k_default_poll_ms}
{
}

vis::StatusSource::~StatusSource()
{
    stop();
}

void vis::StatusSource::configure(const bool enabled,
                                  const std::string &command,
                                  const uint32_t poll_ms)
{
    stop();
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_segments.clear();
        m_enabled = enabled && !command.empty();
        m_command = command;
        m_poll_ms = poll_ms == 0 ? k_default_poll_ms : poll_ms;
    }
    if (m_enabled)
    {
        start();
    }
}

std::vector<vis::StatusSegment> vis::StatusSource::get_segments() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_segments;
}

void vis::StatusSource::start()
{
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_stop_requested = false;
    }
    m_thread = std::thread{&StatusSource::run, this};
}

void vis::StatusSource::stop()
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

bool vis::StatusSource::poll_once(
    std::vector<vis::StatusSegment> *segments) const
{
    if (segments == nullptr)
    {
        return false;
    }

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

    return parse_status_segments_ndjson(output, segments);
}

void vis::StatusSource::run()
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

        std::vector<StatusSegment> segments;
        if (poll_once(&segments))
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_segments = segments;
            failed_polls = 0;
        }
        else
        {
            ++failed_polls;
            if (failed_polls >= k_max_failed_polls_before_clear)
            {
                std::lock_guard<std::mutex> lock{m_mutex};
                m_segments.clear();
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
