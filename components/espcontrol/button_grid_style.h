#pragma once

#include "display_color.h"

// Shared UI colour tokens for device-side LVGL rendering.
// Primary is user configurable; secondary and tertiary are fixed defaults.

constexpr uint32_t DEFAULT_PRIMARY_COLOR_RAW = 0xFF8C00;
constexpr uint32_t DEFAULT_SECONDARY_COLOR_RAW = 0x313131;
constexpr uint32_t DEFAULT_TERTIARY_COLOR_RAW = 0x212121;

// Lightweight glass card tokens. LVGL opacity and a one-pixel rim create a
// translucent impression without blur or per-frame effects.
constexpr uint32_t GLASS_CARD_INACTIVE_COLOR_RAW = 0x536273;
constexpr uint32_t GLASS_CARD_BORDER_COLOR_RAW = 0xFFFFFF;
constexpr uint8_t GLASS_CARD_INACTIVE_OPA = 42;
constexpr uint8_t GLASS_CARD_ACTIVE_OPA = 178;
constexpr uint8_t GLASS_CARD_BORDER_OPA = 72;
constexpr uint8_t GLASS_CARD_ACTIVE_BORDER_OPA = 190;
constexpr uint32_t GLASS_LIGHT_ACTIVE_RAW = 0xFFC247;
constexpr uint32_t GLASS_ACCESS_ACTIVE_RAW = 0xEF5350;
constexpr uint32_t GLASS_CLIMATE_ACTIVE_RAW = 0x42A5F5;
constexpr uint32_t GLASS_MEDIA_ACTIVE_RAW = 0x43B581;

inline uint32_t glass_category_active_color(const std::string &type,
                                            const std::string &entity,
                                            uint32_t fallback) {
  if (type == "garage" || type == "lock" || type == "cover")
    return GLASS_ACCESS_ACTIVE_RAW;
  if (type == "climate" || type == "climate_control")
    return GLASS_CLIMATE_ACTIVE_RAW;
  if (type == "media") return GLASS_MEDIA_ACTIVE_RAW;
  if (type == "light_brightness" || type == "light_switch" ||
      type == "light_temperature" || type == "light_control" ||
      entity.compare(0, 6, "light.") == 0)
    return GLASS_LIGHT_ACTIVE_RAW;
  return fallback;
}

constexpr uint32_t DEFAULT_SLIDER_COLOR = correct_display_color(DEFAULT_PRIMARY_COLOR_RAW);
constexpr uint32_t DEFAULT_OFF_COLOR = correct_display_color(DEFAULT_SECONDARY_COLOR_RAW);
constexpr uint32_t DEFAULT_TERTIARY_COLOR = correct_display_color(DEFAULT_TERTIARY_COLOR_RAW);

constexpr uint32_t SECONDARY_GREY = DEFAULT_OFF_COLOR;
constexpr uint32_t TERTIARY_GREY = DEFAULT_TERTIARY_COLOR;
constexpr uint32_t DARK_TEXT_PRIMARY = 0xFFFFFF;
constexpr uint32_t DARK_TEXT_INVERTED = 0x000000;
constexpr uint32_t DARK_TEXT_MUTED = 0xB0B0B0;
constexpr uint32_t DARK_TEXT_SOFT = DARK_TEXT_PRIMARY;
constexpr uint32_t DARK_BORDER = SECONDARY_GREY;
constexpr uint32_t DARK_CONTROL_NEUTRAL = SECONDARY_GREY;
constexpr uint32_t DARK_OVERLAY = 0x000000;
constexpr uint32_t DARK_TRACK_BACKGROUND = SECONDARY_GREY;

constexpr uint32_t readable_text_color_for_bg(uint32_t bg_color) {
  uint32_t red = (bg_color >> 16) & 0xFF;
  uint32_t green = (bg_color >> 8) & 0xFF;
  uint32_t blue = bg_color & 0xFF;
  uint32_t brightness = (red * 299 + green * 587 + blue * 114) / 1000;
  return brightness > 186 ? TERTIARY_GREY : DARK_TEXT_PRIMARY;
}

static_assert(readable_text_color_for_bg(0xFFFFFF) == TERTIARY_GREY,
              "light backgrounds need dark text");
static_assert(readable_text_color_for_bg(0x000000) == DARK_TEXT_PRIMARY,
              "dark backgrounds need light text");

inline uint32_t &current_button_primary_color_ref() {
  static uint32_t color = DEFAULT_SLIDER_COLOR;
  return color;
}

inline void set_current_button_primary_color(uint32_t color) {
  current_button_primary_color_ref() = color;
}

inline uint32_t current_button_primary_color() {
  return current_button_primary_color_ref();
}
