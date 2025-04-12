#pragma once
#ifndef H_TABLES
#define H_TABLES
#include "utils.hpp"
constexpr int SUPPORTED_IMAGE_EXT_COUNT = 11;
extern const char* const SUPPORTED_IMAGE_EXT[SUPPORTED_IMAGE_EXT_COUNT];
bool is_image_ext_supported(const char* extension);
constexpr int LEGACY_COLORS_COUNT = 148;
extern const char* const LEGACY_COLOR_NAMES[LEGACY_COLORS_COUNT];
extern const u32 LEGACY_COLOR_BYTES[LEGACY_COLORS_COUNT];
i64 u32_legacy_color(const char* name);
#endif
