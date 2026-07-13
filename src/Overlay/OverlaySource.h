/*
 * OverlaySource.h
 */

#ifndef _VIS_OVERLAY_SOURCE_H
#define _VIS_OVERLAY_SOURCE_H

#include "Domain/Settings.h"
#include "Overlay/OverlayMetadata.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace vis
{

class OverlaySource
{
  public:
    OverlaySource();
    ~OverlaySource();

    OverlaySource(const OverlaySource &) = delete;
    OverlaySource(const OverlaySource &&) = delete;
    OverlaySource &operator=(const OverlaySource &) = delete;
    OverlaySource &operator=(OverlaySource &&) = delete;

    void configure(const std::shared_ptr<const Settings> &settings);
    void configure(bool enabled, const std::string &command,
                   uint32_t poll_ms);
    OverlayMetadata get_metadata() const;

  private:
    void stop();
    void start();
    void run();
    bool poll_once(OverlayMetadata *metadata) const;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_thread;
    bool m_stop_requested;

    bool m_enabled;
    std::string m_command;
    uint32_t m_poll_ms;
    OverlayMetadata m_metadata;
};

} // namespace vis

#endif
