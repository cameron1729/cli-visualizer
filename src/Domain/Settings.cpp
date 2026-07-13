/*
 * Settings.cpp
 *
 * Created on: Jul 30, 2015
 *     Author: dpayne
 */

#include "Domain/Settings.h"

#include "Domain/VisConstants.h"
#include <utility>

vis::Settings::Settings(std::string config_path)
    : m_scaling_multiplier{VisConstants::k_default_scaling_multiplier},
      m_fps{VisConstants::k_default_fps},
      m_sampling_frequency{VisConstants::k_default_sampling_frequency},
      m_low_cutoff_frequency{VisConstants::k_default_low_cutoff_frequency},
      m_high_cutoff_frequency{VisConstants::k_default_high_cutoff_frequency},
      m_is_stereo_enabled{true}, m_is_override_terminal_colors{true},
      m_spectrum_character{VisConstants::k_default_spectrum_character},
      m_lorenz_character{VisConstants::k_default_lorenz_character},
      m_ellipse_character{VisConstants::k_default_ellipse_character},
      m_ellipse_radius{VisConstants::k_default_ellipse_radius},
      m_spectrum_bar_width{VisConstants::k_default_spectrum_bar_width},
      m_spectrum_bar_spacing{VisConstants::k_default_spectrum_bar_spacing},
      m_monstercat_smoothing_factor{
          VisConstants::k_default_monstercat_smoothing_factor},
      m_sgs_smoothing_points{VisConstants::k_default_sgs_smoothing_points},
      m_sgs_smoothing_passes{VisConstants::k_default_sgs_smoothing_passes},
      m_spectrum_smoothing_mode{
          VisConstants::k_default_spectrum_smoothing_mode},
      m_spectrum_falloff_mode{VisConstants::k_default_spectrum_falloff_mode},
      m_spectrum_falloff_weight{
          VisConstants::k_default_spectrum_falloff_weight},
      m_spectrum_top_margin{VisConstants::k_default_spectrum_top_margin},
      m_spectrum_bottom_margin{VisConstants::k_default_spectrum_bottom_margin},
      m_spectrum_right_margin{VisConstants::k_default_spectrum_right_margin},
      m_spectrum_left_margin{VisConstants::k_default_spectrum_left_margin},
      m_is_spectrum_reversed{VisConstants::k_default_spectrum_reversed},
      m_rotation_interval{VisConstants::k_default_visualizer_rotation_interval},
      m_overlay_enabled{VisConstants::k_default_overlay_enabled},
      m_overlay_command{VisConstants::k_default_overlay_command},
      m_overlay_poll_ms{VisConstants::k_default_overlay_poll_ms},
      m_overlay_bgv_enabled{VisConstants::k_default_overlay_bgv_enabled},
      m_overlay_bgv_command{VisConstants::k_default_overlay_bgv_command},
      m_overlay_bgv_poll_ms{VisConstants::k_default_overlay_bgv_poll_ms},
      m_overlay_flight_enabled{
          VisConstants::k_default_overlay_flight_enabled},
      m_overlay_flight_command{
          VisConstants::k_default_overlay_flight_command},
      m_overlay_flight_poll_ms{
          VisConstants::k_default_overlay_flight_poll_ms},
      m_overlay_flight_width{VisConstants::k_default_overlay_flight_width},
      m_overlay_flight_plane_left{
          VisConstants::k_default_overlay_flight_plane_left},
      m_overlay_flight_plane_right{
          VisConstants::k_default_overlay_flight_plane_right},
      m_overlay_status_enabled{VisConstants::k_default_overlay_status_enabled},
      m_overlay_status_command{VisConstants::k_default_overlay_status_command},
      m_overlay_status_poll_ms{
          VisConstants::k_default_overlay_status_poll_ms},
      m_overlay_status_row{VisConstants::k_default_overlay_status_row},
      m_overlay_status_speed{VisConstants::k_default_overlay_status_speed},
      m_overlay_status_gap{VisConstants::k_default_overlay_status_gap},
      m_overlay_status_boundary_left{
          VisConstants::k_default_overlay_status_boundary_left},
      m_overlay_status_boundary_right{
          VisConstants::k_default_overlay_status_boundary_right},
      m_overlay_marquee_enabled{
          VisConstants::k_default_overlay_marquee_enabled},
      m_overlay_marquee_row{VisConstants::k_default_overlay_marquee_row},
      m_overlay_marquee_speed{VisConstants::k_default_overlay_marquee_speed},
      m_overlay_marquee_gap{VisConstants::k_default_overlay_marquee_gap},
      m_overlay_marquee_boundary_enabled{
          VisConstants::k_default_overlay_marquee_boundary_enabled},
      m_overlay_marquee_boundary_left{
          VisConstants::k_default_overlay_marquee_boundary_left},
      m_overlay_marquee_boundary_right{
          VisConstants::k_default_overlay_marquee_boundary_right},
      m_overlay_progress_enabled{
          VisConstants::k_default_overlay_progress_enabled},
      m_overlay_progress_row{VisConstants::k_default_overlay_progress_row},
      m_overlay_progress_width{VisConstants::k_default_overlay_progress_width},
      m_overlay_progress_align{VisConstants::k_default_overlay_progress_align},
      m_overlay_progress_status_enabled{
          VisConstants::k_default_overlay_progress_status_enabled},
      m_overlay_progress_time_enabled{
          VisConstants::k_default_overlay_progress_time_enabled},
      m_overlay_progress_filled{
          VisConstants::k_default_overlay_progress_filled},
      m_overlay_progress_empty{VisConstants::k_default_overlay_progress_empty},
      m_overlay_status_headphones_icon{
          VisConstants::k_default_overlay_status_headphones_icon},
      m_overlay_status_speakers_icon{
          VisConstants::k_default_overlay_status_speakers_icon},
      m_overlay_status_playing_icon{
          VisConstants::k_default_overlay_status_playing_icon},
      m_overlay_status_paused_icon{
          VisConstants::k_default_overlay_status_paused_icon},
      m_overlay_status_stopped_icon{
          VisConstants::k_default_overlay_status_stopped_icon},
      m_config_path{std::move(config_path)}
{
}
