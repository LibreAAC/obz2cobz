

#include "utils.hpp"
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
  inline int width() { return _w; }
  inline int height() { return _h; }
  void down_scale_pow2(int downscale_factor);
  void paste(ImageData& img, int x, int y);
  void serialize(Stream s);
};

