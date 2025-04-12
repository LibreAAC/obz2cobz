#include "img.hpp"
#include "list.hpp"
#include "utils.hpp"
struct ivec2 { int x,y; };
struct Cell;
struct Board;
struct Rect
{
  float x,y,w,h;
  int spritesheet_id;
  bool locked;
  void serialize(Stream s);
};
struct Obj
{
  string obz_tex_id;
  string obz_board_id;
  ImageData img;
  Rect rect;
};
struct Fit
{
  Rect rect;
  int obj_idx;
  bool could_contain(Rect rsmol);
};



