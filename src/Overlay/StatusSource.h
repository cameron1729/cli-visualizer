/*
 * StatusSource.h
 */

#ifndef _VIS_STATUS_SOURCE_H
#define _VIS_STATUS_SOURCE_H

#include "Overlay/OverlayMetadata.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace vis
{

class StatusSource
{
  public:
    StatusSource();
    ~StatusSource();

    StatusSource(const StatusSource &) = delete;
    StatusSource(const StatusSource &&) = delete;
    StatusSource &operator=(const StatusSource &) = delete;
    StatusSource &operator=(StatusSource &&) = delete;

    void configure(bool enabled, const std::string &command,
                   uint32_t poll_ms);
    std::vector<StatusSegment> get_segments() const;

  private:
    void stop();
    void start();
    void run();
    bool poll_once(std::vector<StatusSegment> *segments) const;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_thread;
    bool m_stop_requested;
    bool m_enabled;
    std::string m_command;
    uint32_t m_poll_ms;
    std::vector<StatusSegment> m_segments;
};

} // namespace vis

#endif
