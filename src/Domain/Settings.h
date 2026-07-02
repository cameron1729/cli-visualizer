/*
 * Settings.h
 *
 * Created on: Jul 30, 2015
 *     Author: dpayne
 */

#ifndef _VIS_SETTINGS_H
#define _VIS_SETTINGS_H

#include "Domain/ColorDefinition.h"
#include <string>
#include <vector>

namespace vis
{

class Settings
{
  public:
    explicit Settings(std::string config_path);

    double get_scaling_multiplier() const noexcept
    {
        return m_scaling_multiplier;
    }

    void set_scaling_multiplier(const double scaling_multiplier)
    {
        m_scaling_multiplier = scaling_multiplier;
    }

    uint32_t get_sampling_frequency() const noexcept
    {
        return m_sampling_frequency;
    }

    void set_sampling_frequency(const uint32_t sampling_frequency)
    {
        m_sampling_frequency = sampling_frequency;
    }

    uint32_t get_low_cutoff_frequency() const noexcept
    {
        return m_low_cutoff_frequency;
    }

    void set_low_cutoff_frequency(const uint32_t low_cutoff_frequency)
    {
        m_low_cutoff_frequency = low_cutoff_frequency;
    }

    uint32_t get_high_cutoff_frequency() const noexcept
    {
        return m_high_cutoff_frequency;
    }

    void set_high_cutoff_frequency(const uint32_t high_cutoff_frequency)
    {
        m_high_cutoff_frequency = high_cutoff_frequency;
    }

    uint32_t get_fps() const noexcept
    {
        return m_fps;
    }

    void set_fps(const uint32_t fps)
    {
        m_fps = fps;
    }

    uint32_t get_sample_size() const noexcept
    {
        return m_sampling_frequency / m_fps;
    }

    const std::string &get_mpd_fifo_path() const noexcept
    {
        return m_mpd_fifo_path;
    }

    void set_mpd_fifo_path(const std::string &path)
    {
        m_mpd_fifo_path = path;
    }

    const std::string &get_shmem_name() const noexcept
    {
        return m_shmem_name;
    }

    void set_shmem_name(const std::string &path)
    {
        m_shmem_name = path;
    }

    const std::vector<std::string> &get_visualizers() const noexcept
    {
        return m_visualizers;
    }

    void set_visualizers(const std::vector<std::string> &visualizers)
    {
        m_visualizers = visualizers;
    }

    const std::string &get_audio_source() const noexcept
    {
        return m_audio_source;
    }

    void set_audio_source(const std::string &audio_source)
    {
        m_audio_source = audio_source;
    }

    void set_is_stereo_enabled(bool is_stereo_enabled)
    {
        m_is_stereo_enabled = is_stereo_enabled;
    }

    bool is_stereo_enabled() const noexcept
    {
        return m_is_stereo_enabled;
    }

    const std::vector<vis::ColorDefinition> &get_colors() const noexcept
    {
        return m_colors;
    }

    void set_colors(const std::vector<vis::ColorDefinition> &colors)
    {
        m_colors = colors;
    }

    void set_is_override_terminal_colors(bool is_override_terminal_colors)
    {
        m_is_override_terminal_colors = is_override_terminal_colors;
    }

    bool is_override_terminal_colors() const noexcept
    {
        return m_is_override_terminal_colors;
    }

    wchar_t get_spectrum_character() const noexcept
    {
        return m_spectrum_character;
    }

    void set_spectrum_character(wchar_t spectrum_character)
    {
        m_spectrum_character = spectrum_character;
    }

    wchar_t get_lorenz_character() const noexcept
    {
        return m_lorenz_character;
    }

    void set_lorenz_character(wchar_t lorenz_character)
    {
        m_lorenz_character = lorenz_character;
    }

    wchar_t get_ellipse_character() const noexcept
    {
        return m_ellipse_character;
    }

    void set_ellipse_character(wchar_t ellipse_character)
    {
        m_ellipse_character = ellipse_character;
    }

    uint32_t get_ellipse_radius() const noexcept
    {
        return m_ellipse_radius;
    }

    void set_ellipse_radius(uint32_t ellipse_radius)
    {
        m_ellipse_radius = ellipse_radius;
    }

    void set_monstercat_smoothing_factor(double monstercat_smoothing_factor)
    {
        m_monstercat_smoothing_factor = monstercat_smoothing_factor;
    }

    double get_monstercat_smoothing_factor() const noexcept
    {
        return m_monstercat_smoothing_factor;
    }

    void set_sgs_smoothing_points(uint32_t sgs_smoothing_points)
    {
        m_sgs_smoothing_points = sgs_smoothing_points;
    }

    uint32_t get_sgs_smoothing_points() const noexcept
    {
        return m_sgs_smoothing_points;
    }

    void set_sgs_smoothing_passes(uint32_t sgs_smoothing_passes)
    {
        m_sgs_smoothing_passes = sgs_smoothing_passes;
    }

    uint32_t get_sgs_smoothing_passes() const noexcept
    {
        return m_sgs_smoothing_passes;
    }

    uint32_t get_spectrum_bar_width() const noexcept
    {
        return m_spectrum_bar_width;
    }

    void set_spectrum_bar_width(const uint32_t spectrum_bar_width)
    {
        m_spectrum_bar_width = spectrum_bar_width;
    }

    uint32_t get_spectrum_bar_spacing() const noexcept
    {
        return m_spectrum_bar_spacing;
    }

    void set_spectrum_bar_spacing(const uint32_t spectrum_bar_spacing)
    {
        m_spectrum_bar_spacing = spectrum_bar_spacing;
    }

    SmoothingMode get_spectrum_smoothing_mode() const noexcept
    {
        return m_spectrum_smoothing_mode;
    }

    void set_spectrum_smoothing_mode(SmoothingMode spectrum_smoothing_mode)
    {
        m_spectrum_smoothing_mode = spectrum_smoothing_mode;
    }

    void set_spectrum_falloff_mode(const vis::FalloffMode falloff_mode)
    {
        m_spectrum_falloff_mode = falloff_mode;
    }

    vis::FalloffMode get_spectrum_falloff_mode() const noexcept
    {
        return m_spectrum_falloff_mode;
    }

    void set_spectrum_falloff_weight(const double spectrum_falloff_weight)
    {
        m_spectrum_falloff_weight = spectrum_falloff_weight;
    }

    double get_spectrum_falloff_weight() const noexcept
    {
        return m_spectrum_falloff_weight;
    }

    void set_spectrum_top_margin(const double spectrum_top_margin)
    {
        m_spectrum_top_margin = spectrum_top_margin;
    }

    double get_spectrum_top_margin() const noexcept
    {
        return m_spectrum_top_margin;
    }

    void set_spectrum_bottom_margin(const double spectrum_bottom_margin)
    {
        m_spectrum_bottom_margin = spectrum_bottom_margin;
    }

    double get_spectrum_bottom_margin() const noexcept
    {
        return m_spectrum_bottom_margin;
    }

    void set_spectrum_right_margin(const double spectrum_right_margin)
    {
        m_spectrum_right_margin = spectrum_right_margin;
    }

    double get_spectrum_right_margin() const noexcept
    {
        return m_spectrum_right_margin;
    }

    void set_spectrum_left_margin(const double spectrum_left_margin)
    {
        m_spectrum_left_margin = spectrum_left_margin;
    }

    double get_spectrum_left_margin() const noexcept
    {
        return m_spectrum_left_margin;
    }

    void set_is_spectrum_reversed(const bool is_spectrum_reversed)
    {
        m_is_spectrum_reversed = is_spectrum_reversed;
    }

    bool is_spectrum_reversed() const noexcept
    {
        return m_is_spectrum_reversed;
    }

    void set_rotation_interval(const int64_t rotation_interval)
    {
        m_rotation_interval = rotation_interval;
    }

    int64_t get_rotation_interval() const noexcept
    {
        return m_rotation_interval;
    }

    void set_port_audio_source(const std::string &port_audio_source)
    {
        m_port_audio_source = port_audio_source;
    }

    const std::string &get_port_audio_source() const noexcept
    {
        return m_port_audio_source;
    }

    void set_pulse_audio_source(const std::string &pulse_audio_source)
    {
        m_pulse_audio_source = pulse_audio_source;
    }

    const std::string &get_pulse_audio_source() const noexcept
    {
        return m_pulse_audio_source;
    }

    void set_color_schemes(const std::vector<std::string> &color_schemes)
    {
        m_color_schemes = color_schemes;
    }

    const std::vector<std::string> &get_color_schemes() const noexcept
    {
        return m_color_schemes;
    }

    bool is_overlay_enabled() const noexcept
    {
        return m_overlay_enabled;
    }

    void set_overlay_enabled(bool overlay_enabled)
    {
        m_overlay_enabled = overlay_enabled;
    }

    const std::string &get_overlay_command() const noexcept
    {
        return m_overlay_command;
    }

    void set_overlay_command(const std::string &overlay_command)
    {
        m_overlay_command = overlay_command;
    }

    uint32_t get_overlay_poll_ms() const noexcept
    {
        return m_overlay_poll_ms;
    }

    void set_overlay_poll_ms(uint32_t overlay_poll_ms)
    {
        m_overlay_poll_ms = overlay_poll_ms;
    }

    bool is_overlay_marquee_enabled() const noexcept
    {
        return m_overlay_marquee_enabled;
    }

    void set_overlay_marquee_enabled(bool overlay_marquee_enabled)
    {
        m_overlay_marquee_enabled = overlay_marquee_enabled;
    }

    uint32_t get_overlay_marquee_row() const noexcept
    {
        return m_overlay_marquee_row;
    }

    void set_overlay_marquee_row(uint32_t overlay_marquee_row)
    {
        m_overlay_marquee_row = overlay_marquee_row;
    }

    double get_overlay_marquee_speed() const noexcept
    {
        return m_overlay_marquee_speed;
    }

    void set_overlay_marquee_speed(double overlay_marquee_speed)
    {
        m_overlay_marquee_speed = overlay_marquee_speed;
    }

    uint32_t get_overlay_marquee_gap() const noexcept
    {
        return m_overlay_marquee_gap;
    }

    void set_overlay_marquee_gap(uint32_t overlay_marquee_gap)
    {
        m_overlay_marquee_gap = overlay_marquee_gap;
    }

    bool is_overlay_marquee_boundary_enabled() const noexcept
    {
        return m_overlay_marquee_boundary_enabled;
    }

    void set_overlay_marquee_boundary_enabled(
        bool overlay_marquee_boundary_enabled)
    {
        m_overlay_marquee_boundary_enabled = overlay_marquee_boundary_enabled;
    }

    const std::wstring &get_overlay_marquee_boundary_left() const noexcept
    {
        return m_overlay_marquee_boundary_left;
    }

    void set_overlay_marquee_boundary_left(
        const std::wstring &overlay_marquee_boundary_left)
    {
        m_overlay_marquee_boundary_left = overlay_marquee_boundary_left;
    }

    const std::wstring &get_overlay_marquee_boundary_right() const noexcept
    {
        return m_overlay_marquee_boundary_right;
    }

    void set_overlay_marquee_boundary_right(
        const std::wstring &overlay_marquee_boundary_right)
    {
        m_overlay_marquee_boundary_right = overlay_marquee_boundary_right;
    }

    bool is_overlay_progress_enabled() const noexcept
    {
        return m_overlay_progress_enabled;
    }

    void set_overlay_progress_enabled(bool overlay_progress_enabled)
    {
        m_overlay_progress_enabled = overlay_progress_enabled;
    }

    uint32_t get_overlay_progress_row() const noexcept
    {
        return m_overlay_progress_row;
    }

    void set_overlay_progress_row(uint32_t overlay_progress_row)
    {
        m_overlay_progress_row = overlay_progress_row;
    }

    uint32_t get_overlay_progress_width() const noexcept
    {
        return m_overlay_progress_width;
    }

    void set_overlay_progress_width(uint32_t overlay_progress_width)
    {
        m_overlay_progress_width = overlay_progress_width;
    }

    const std::string &get_overlay_progress_align() const noexcept
    {
        return m_overlay_progress_align;
    }

    void set_overlay_progress_align(const std::string &overlay_progress_align)
    {
        m_overlay_progress_align = overlay_progress_align;
    }

    bool is_overlay_progress_status_enabled() const noexcept
    {
        return m_overlay_progress_status_enabled;
    }

    void set_overlay_progress_status_enabled(
        bool overlay_progress_status_enabled)
    {
        m_overlay_progress_status_enabled = overlay_progress_status_enabled;
    }

    bool is_overlay_progress_time_enabled() const noexcept
    {
        return m_overlay_progress_time_enabled;
    }

    void set_overlay_progress_time_enabled(bool overlay_progress_time_enabled)
    {
        m_overlay_progress_time_enabled = overlay_progress_time_enabled;
    }

    const std::wstring &get_overlay_progress_filled() const noexcept
    {
        return m_overlay_progress_filled;
    }

    void set_overlay_progress_filled(
        const std::wstring &overlay_progress_filled)
    {
        m_overlay_progress_filled = overlay_progress_filled;
    }

    const std::wstring &get_overlay_progress_empty() const noexcept
    {
        return m_overlay_progress_empty;
    }

    void set_overlay_progress_empty(const std::wstring &overlay_progress_empty)
    {
        m_overlay_progress_empty = overlay_progress_empty;
    }

    const std::wstring &get_overlay_status_headphones_icon() const noexcept
    {
        return m_overlay_status_headphones_icon;
    }

    void set_overlay_status_headphones_icon(
        const std::wstring &overlay_status_headphones_icon)
    {
        m_overlay_status_headphones_icon = overlay_status_headphones_icon;
    }

    const std::wstring &get_overlay_status_speakers_icon() const noexcept
    {
        return m_overlay_status_speakers_icon;
    }

    void set_overlay_status_speakers_icon(
        const std::wstring &overlay_status_speakers_icon)
    {
        m_overlay_status_speakers_icon = overlay_status_speakers_icon;
    }

    const std::wstring &get_overlay_status_playing_icon() const noexcept
    {
        return m_overlay_status_playing_icon;
    }

    void set_overlay_status_playing_icon(
        const std::wstring &overlay_status_playing_icon)
    {
        m_overlay_status_playing_icon = overlay_status_playing_icon;
    }

    const std::wstring &get_overlay_status_paused_icon() const noexcept
    {
        return m_overlay_status_paused_icon;
    }

    void set_overlay_status_paused_icon(
        const std::wstring &overlay_status_paused_icon)
    {
        m_overlay_status_paused_icon = overlay_status_paused_icon;
    }

    const std::wstring &get_overlay_status_stopped_icon() const noexcept
    {
        return m_overlay_status_stopped_icon;
    }

    void set_overlay_status_stopped_icon(
        const std::wstring &overlay_status_stopped_icon)
    {
        m_overlay_status_stopped_icon = overlay_status_stopped_icon;
    }

    void set_config_path(const std::string &config_path)
    {
        m_config_path = config_path;
    }

    const std::string &get_config_path() const noexcept
    {
        return m_config_path;
    }

    void reset_reloadable_settings();

  private:
    std::string m_mpd_fifo_path;
    std::string m_shmem_name;
    double m_scaling_multiplier;
    uint32_t m_fps;
    uint32_t m_sampling_frequency;
    uint32_t m_low_cutoff_frequency;
    uint32_t m_high_cutoff_frequency;
    bool m_is_stereo_enabled;
    std::string m_audio_source;
    std::vector<std::string> m_visualizers;
    std::vector<vis::ColorDefinition> m_colors;
    bool m_is_override_terminal_colors;
    wchar_t m_spectrum_character;
    wchar_t m_lorenz_character;
    wchar_t m_ellipse_character;
    uint32_t m_ellipse_radius;
    uint32_t m_spectrum_bar_width;
    uint32_t m_spectrum_bar_spacing;
    double m_monstercat_smoothing_factor;
    uint32_t m_sgs_smoothing_points;
    uint32_t m_sgs_smoothing_passes;
    SmoothingMode m_spectrum_smoothing_mode;
    FalloffMode m_spectrum_falloff_mode;
    double m_spectrum_falloff_weight;
    double m_spectrum_top_margin;
    double m_spectrum_bottom_margin;
    double m_spectrum_right_margin;
    double m_spectrum_left_margin;
    bool m_is_spectrum_reversed;
    int64_t m_rotation_interval;
    std::string m_port_audio_source;
    std::string m_pulse_audio_source;
    std::vector<std::string> m_color_schemes;
    bool m_overlay_enabled;
    std::string m_overlay_command;
    uint32_t m_overlay_poll_ms;
    bool m_overlay_marquee_enabled;
    uint32_t m_overlay_marquee_row;
    double m_overlay_marquee_speed;
    uint32_t m_overlay_marquee_gap;
    bool m_overlay_marquee_boundary_enabled;
    std::wstring m_overlay_marquee_boundary_left;
    std::wstring m_overlay_marquee_boundary_right;
    bool m_overlay_progress_enabled;
    uint32_t m_overlay_progress_row;
    uint32_t m_overlay_progress_width;
    std::string m_overlay_progress_align;
    bool m_overlay_progress_status_enabled;
    bool m_overlay_progress_time_enabled;
    std::wstring m_overlay_progress_filled;
    std::wstring m_overlay_progress_empty;
    std::wstring m_overlay_status_headphones_icon;
    std::wstring m_overlay_status_speakers_icon;
    std::wstring m_overlay_status_playing_icon;
    std::wstring m_overlay_status_paused_icon;
    std::wstring m_overlay_status_stopped_icon;
    std::string m_config_path;
};
} // namespace vis

#endif
