#pragma once
#include "utils.hpp"
#ifndef H_COLORS
#define H_COLORS

float depercent(char* expr, float max);
u32 hexchar(char c);
u32 rgba(u32 r, u32 g, u32 b, u32 a);
u32 parse_color(char* expr, u32 default_);

#endif
