#include "parser.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
const char* help =
"Usage:\n"
"obz2cobz.py <src>.obz <dst>.cobz\n"
"\n"
"Descr:\n"
"Compiles Open Board Zip files (exported from Board Builder),\n"
"into a Compiled Open Board Zip for this custom AAC program I wrote (AACpp).\n"
"\n";

int main(int argc, const char** argv)
{
  if (argc < 3)
  {
    puts(help);
    return 1;
  }
  COBZ cobz = parse_file(argv[1]);
  Stream f = {fopen(argv[2], "wb")};
  i64 len;
  f << (len = cobz.boards.len());
  for (int i = 0; i < len; i++)
  {
    f.write_anchor("BRD");
    cobz.boards[i].serialize(f);
  }
  f << (len = cobz.textures.len());
  const long seek_rects = 8+ftell(f._f);
  const long seek_texs = 8+seek_rects+len*Rect::SERIALIZED_LENGTH;
  // NOTE: count of spritesheets is already given by board count
  const i64 tex_count =
    cobz.gen_and_serialize_all_spritesheets(f, seek_texs);
  {
    fseek(f._f, seek_rects, SEEK_SET);
    const int rect_count = cobz.textures.len();
    for (int i = 0; i < rect_count; i++)
    {
      cobz.textures[i].rect.serialize(f);
    }
  }
  for (int i = 0; i < cobz.boards.len(); i++)
  {
    printf("=== Board [%s] ===\n", cobz.boards[i].name.data());
    for (int j = 0; j < cobz.boards[i].cells.len(); j++)
    {
      const Cell& c = cobz.boards[i].cells[j];
      printf(
        " [%s] at (%i,%i), tex_id=%i, rect=(%f,%f,%f,%f), ssid=%i\n",
        c.name.data(),
        c.obz_xy.x, c.obz_xy.y,
        c.tex_id,
        cobz.textures[c.tex_id].rect.x,
        cobz.textures[c.tex_id].rect.y,
        cobz.textures[c.tex_id].rect.w,
        cobz.textures[c.tex_id].rect.h,
        cobz.textures[c.tex_id].rect.spritesheet_id
      );
    }
  }
  cobz.destroy();
  fseek(f._f, seek_rects-8, SEEK_SET);
  static_assert(sizeof(i64) == 8);
  f << (i64&) tex_count;
  fclose(f._f);
  puts("Done.");
  return 0;
}
