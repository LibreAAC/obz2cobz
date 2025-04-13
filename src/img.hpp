#pragma once
#ifndef H_IMG
#define H_IMG
#include "utils.hpp"
#include "zip.h"
void svg2png(ByteBuffer in, ByteBuffer& out, const char* const info);

struct ImageData
{
  struct rgba32 {
    union {
      struct { u8 r,g,b,a; };
      u32 color32;
    };
  };
  rgba32* _data;
  int _w, _h;
  static inline ImageData init()
  { return { nullptr, 0, 0 }; }
  static ImageData from(const char* filename);
  static ImageData from(ByteBuffer buffer);
  inline bool is_valid() { return _data != nullptr && _w != 0 && _h != 0; }
  inline int width() { return _w; }
  inline int height() { return _h; }
  // 1 means downscale by a factor of 2,
  // 3 means downscale by a factor of 8,
  // etc...
  void downscale_pow2(int pow2_factor);
  void paste(ImageData& img, int x, int y);
  void serialize(Stream s);
  void destroy();
};
constexpr ImageData INVALID_IMAGE = {nullptr, 0, 0};
ImageData load_img(
  zip_t* z,
  const char* src,
  const char* const cell_name
);
#endif
