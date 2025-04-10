
#include "img.hpp"
#include "list.hpp"

struct ivec2 { int x,y; };
struct Cell;
struct Board;
struct Rect
{
  float x,y,w,h;
  int spritesheet_id;
  bool locked;
  void serialize(Stream s)
  { s << spritesheet_id << x << y << w << h; }
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
  bool could_contain(Rect rsmol)
  { return rect.w >= rsmol.w && rect.h >= rsmol.h; }
};
struct CompiledOBZ
{
  list<Obj> textures;
  list<Board> boards;
  ivec2 gen_spritesheet_precursors(list<Obj*>& objs)
  {
    int minw = 0;
    int maxw = 0;
    for (int i = 0; i < objs.len(); i++)
    {
      int downscale_factor = 0;
      objs[i]->rect.w = objs[i]->img.width();
      objs[i]->rect.h = objs[i]->img.height();
      while (objs[i]->rect.w > 300)
      {
        objs[i]->rect.w = floorf(objs[i]->rect.w / 2);
        objs[i]->rect.h = floorf(objs[i]->rect.h / 2);
        downscale_factor += 2;
      }
      if (objs[i]->rect.w > maxw)
        maxw = objs[i]->rect.w;
      objs[i]->img.down_scale_pow2(downscale_factor);
    }
    maxw *= 2;
    // yeah... im lazy...
    // but who wants to write a sorting algorithm in big 2025 anyway ???
    std::sort(objs.data(), objs.data()+(objs.len()+1), [](Obj* a, Obj* b){
      if (a->rect.w == a->rect.h)
        return a->rect.h < b->rect.h;
      return a->rect.w < b->rect.w;
    });
    minw = objs[-1]->rect.w;
    // Set y positions accordingly
    float y = 0;
    for (int i = 0; i < objs.len(); i++)
    {
      objs[i]->rect.x = 0;
      objs[i]->rect.y = y;
      y += objs[i]->rect.h;
    }
    list<Fit> fits;
    fits.init();
    fits.prealloc(objs.len());
    ivec2 ssdims = {0,0}; // spritesheet dimensions
    int base_fixed = 0;
    for (int i = 1; i < objs.len(); i++)
    {
      for (int j = base_fixed; j < i; j++)
      {
        if (fits[j].could_contain(objs[i]->rect))
        {
          objs[i]->rect.x = fits[j].rect.x;
          objs[i]->rect.y = fits[j].rect.y;
          fits.insert(Fit{
            {
              objs[i]->rect.x + objs[i]->rect.w,
              objs[i]->rect.y,
              maxw - (objs[i]->rect.x + objs[i]->rect.w),
              objs[i]->rect.h
            },
            i
          },j);
          fits[j+1].rect.y += objs[i]->rect.h;
          fits[j+1].rect.h -= objs[i]->rect.h;
          int fi = j+2;
          for (; fi < fits.len(); fi++)
          {
            if (fits[fi].obj_idx == i)
            {
              fits.rmv(fi);
              break;
            }
          }
          for (; fi < fits.len(); fi++)
            fits[fi].rect.y -= objs[i]->rect.h;
          for (int k = i+1; k < objs.len(); k++)
            objs[k]->rect.y -= objs[i]->rect.h;
          break;
        }
      }
    }
    ssdims.x = maxw;
    for (int i = 0; i < objs.len(); i++)
    {
      const float Y = objs[i]->rect.y+objs[i]->rect.h;
      if (Y > ssdims.y)
      {
        ssdims.y = Y;
      }
    }
    return ssdims;
  }

  ImageData gen_one_spritesheet(list<Obj*>& objs, ivec2 dims, int spritesheet_id)
  {
    auto spritesheet = ImageData::init();
    for (int i = 0; i < objs.len(); i++)
    {
      assert(objs[i]->rect.spritesheet_id == -1);
      objs[i]->rect.spritesheet_id = spritesheet_id;
      spritesheet.paste(objs[i]->img, objs[i]->rect.x, objs[i]->rect.y);
    }
    return spritesheet;
  }

  list<ImageData> gen_all_spritesheet();
};

struct Board
{
  int w, h;
  int parent_idx;
  list<Cell> cells;
  string name;
  string obz_id;
  static inline Board init() {
    Board r = { 0, 0, -1, {}, {}, {} };
    r.cells.init();
    r.name.init();
    r.obz_id.init();
    return r;
  }
  void serialize(Stream s);
};
struct Cell
{
  string name;
  int tex_id;
  int child;
  u32 background, border; // colors
  list<string> actions;
  string obz_child_id;
  string obz_tex_id;
  string obz_id;
  ivec2 obz_xy;
  void set_child_obz(CompiledOBZ& cobz, string& obz_id)
  { obz_child_id.hold(obz_id); }
  void set_child_idx(CompiledOBZ& cobz, int idx)
  {
    assert(obz_child_id.len() != 0);
    child = idx;
    if (cobz.boards[idx].parent_idx == -1)
    {
      cobz.boards[idx].parent_idx = idx;
    }
    else
      assert(cobz.boards[idx].parent_idx == idx);
  }
  void serialize(Stream s)
  {
    i64 len;
    s.write_anchor("CLL");
    s << (i32&) tex_id;
    len = name.len();
    fwrite(&len, sizeof(len), 1, s._f);
    fwrite(name.data(), len, 1, s._f);
    len = actions.len();
    fwrite(&len, sizeof(len), 1, s._f);
    for (i64 i = 0; i < len; i++)
    {
      i64 lena = actions[i].len();
      fwrite(&lena, sizeof(lena), 1, s._f);
      fwrite(actions[i].data(), lena, 1, s._f);
    }
    s << (i32&) child;
    s << (u32&) background << (u32&) border;
  }
};

void Board::serialize(Stream s)
{
  const int cell_count = cells.len();
  s << w << h << parent_idx << cell_count;
  for (int i = 0; i < cell_count; i++)
  {
    cells[i].serialize(s);
  }
}

list<ImageData> CompiledOBZ::gen_all_spritesheet()
{
  list<ImageData> spritesheets;
  list<Obj*> objs;
  ivec2 ssdims;
  objs.init();
  spritesheets.init();
  for (int i = 0; i < boards.len(); i++)
  {
    objs.clear();
    for (int j = 0; j < textures.len(); j++)
      if (textures[j].obz_board_id == boards[i].obz_id)
        objs.push(&textures[j]);
    ssdims = gen_spritesheet_precursors(objs);
    spritesheets.push(gen_one_spritesheet(objs, ssdims, i));
  }
  return spritesheets;
}





