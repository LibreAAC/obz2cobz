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
"\n"
"This is intended to be nothing more than a prototype. A full C++ (probably)\n"
"version will be written in the future. It's just that zip+json is such a "
"hassle\n"
"to parse in C.";



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
  const long seek_rects = ftell(f._f);
  const long seek_texs = seek_rects+len*Rect::SERIALIZED_LENGTH;
  // NOTE: count of spritesheets is already given by board count
  cobz.gen_and_serialize_all_spritesheets(f, seek_rects, seek_texs);
  cobz.destroy();
  fclose(f._f);
  puts("Done.");
  return 0;
}
