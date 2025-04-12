#include "plutosvg.h"
#include "tables.hpp"
#include "utils.hpp"
#include "img.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <curl/curl.h>
#include <cassert>
#include <curl/easy.h>
#include "zip.h"

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
  auto pack = DynByteBuffer::from(out);
  plutovg_surface_write_to_png_stream(
    surf,
    [](void* _pack, void* data, int size)
    {
      auto pack = (DynByteBuffer*)_pack;
      pack->push(ByteBuffer::ref((u8*)data, size));
    },
    &pack
  );
  return;
}

ImageData ImageData::from(const char* filename)
{
  ImageData ret;
  int channels;
  ret._data = (rgba32*)stbi_load(filename, &ret._w, &ret._h, &channels, 4);
  assert(channels == 4);
  return ret;
}
ImageData ImageData::from(ByteBuffer buffer)
{
  ImageData r;
  int channels;
  r._data = (rgba32*)stbi_load_from_memory(
    buffer.data, buffer.len, &r._w, &r._h, &channels, 4
  );
  assert(channels == 4);
  return r;
}
void ImageData::destroy()
{
  stbi_image_free(_data);
}
void ImageData::downscale_pow2(int factor)
{
  for (int y = 0; y < (_h>>factor)+(_h&1); y++)
  {
    for (int x = 0; x < (_w>>factor)+(_w&1); x++)
    {
      _data[x+y*(_w>>factor)] = _data[(x<<factor)+(y<<factor)*_w];
    }
  }
}
void ImageData::paste(ImageData& img, int x, int y)
{
  const int end = y+img.height();
  const int paste_width = std::min(x + img.width(), width()) - x;
  for (; y < end; y++)
  {
    memcpy(_data+y*height(), img._data+y*img.height(), sizeof(rgba32)*paste_width);
  }
}
void ImageData::serialize(Stream s)
{
  s.write_anchor("IMG");
  const int len = stbi_write_png_to_func(
    [](void* f_, void* data, int size)
    {
      auto f = (FILE*)f_;
      fwrite(data, size, 1, f);
    }, s._f, _w, _h, 4, _data, _w*sizeof(rgba32)
  );
  printf("INFO: Written image is %i bytes long.", len);
}

ImageData load_img(
  zip_t* z,
  const char* src,
  const char* const cell_name
)
{
  if (src == nullptr) return INVALID_IMAGE;
  ByteBuffer raw = {0, 0, 1};
  if (str_startswith(src, "http"))
  {
    CURL* handle;
    long status;
    auto dynbuf = DynByteBuffer::from(raw);
    handle = curl_easy_init();
    if (!handle)
    {
      fprintf(stderr, "ERR: [%s] Failed to create curl handle.", cell_name);
      return INVALID_IMAGE;
    }
    curl_easy_setopt(handle, CURLOPT_URL, src);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "curl/7.42.0"); // i won't bother with the version
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 2L);
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION,
      [](void* ptr, size_t size, size_t nmemb, DynByteBuffer* buf)
      {
        if (buf)
          buf->push(ByteBuffer::ref((u8*)ptr, size*nmemb));
      }
    );
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &dynbuf);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, nullptr);
    curl_easy_perform(handle);
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
    if (status != 200)
    {
      fprintf(stderr, "ERR: [%s] Request failed with status code %li instead of 200.\n", cell_name, status);
      curl_easy_cleanup(handle);
      return INVALID_IMAGE;
    }
    curl_easy_cleanup(handle);
  }
  else if (str_startswith(src, "data:image/"))
  {
    todo();
  }
  else
  {
    const int len = str_len(src);
    const int MAX_EXTENSION_LEN = 4;
    size_t temp_size;
    char extension_load[MAX_EXTENSION_LEN+1];
    char* extension = extension_load + MAX_EXTENSION_LEN;
    int i = len;
    {
      while (src[i] != '.' && len-i <= MAX_EXTENSION_LEN)
        *(extension--) = src[i--];
      if (src[i] != '.')
      {
        fprintf(
          stderr, "ERR: [%s] Unrecognized extension (%s).\n",
          cell_name, extension
        );
        return INVALID_IMAGE;
      }
      if (!is_image_ext_supported(extension))
        fprintf(
          stderr, "ERR: [%s] Extension not supported (%s).\n",
          cell_name, extension
        );
    }
    zip_entry_open(z, src);
    {
      raw.owned = true;
      zip_entry_read(z, (void**)&raw.data, &temp_size);
      raw.len = temp_size;
    }
    zip_entry_close(z);
  }
  return ImageData::from(raw);
}
