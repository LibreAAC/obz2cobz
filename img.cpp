#include "plutosvg.h"
#include "utils.hpp"
#include "img.hpp"
#include "stb_image.h"

void svg2png(ByteBuffer in, ByteBuffer& out, const char* const info)
{
  out = ByteBuffer::init();
  const plutosvg_document_t* doc = plutosvg_document_load_from_data(
    (i8*)in.data, in.len, 128, 128, nullptr, nullptr
  );
  if (doc == nullptr)
  { printf("ERROR: [%s] Failed to read svg document.", info); return; }
  const plutovg_surface_t* surf = plutosvg_document_render_to_surface(
    doc, nullptr, 128, 128, nullptr, nullptr, nullptr
  );
  struct Pack { ByteBuffer& out; u64 cap; }
    pack = { out, 0 };
  out.dyn_begin(pack.cap);
  plutovg_surface_write_to_png_stream(
    surf,
    [](void* _pack, void* data, int size)
    {
      auto pack = (Pack*)_pack;
      pack->out.dyn_push(pack->cap, ByteBuffer::ref((u8*)data, size));
    },
    &pack
  );
  out.dyn_end(pack.cap);
  return;
}

ImageData ImageData::from(const char* filename)
{
  ImageData ret;
  int channels;
  ret._data = (rgba32*)stbi_load(filename, &ret._w, &ret._h, &channels, 1);
  
}


