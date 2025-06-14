#include "img.hpp"
#include "list.hpp"
#include "utils.hpp"
struct ivec2 { int x,y; };
inline bool operator < (ivec2 a, ivec2 b)
{
  if (a.y == b.y)
    return a.x < b.x;
  return a.y < b.y;
}
inline bool operator == (ivec2 a, ivec2 b)
{
  return a.x == b.x && a.y == b.y;
}
inline bool operator != (ivec2 a, ivec2 b)
{ return !(a == b); }
struct Rect
{
  static constexpr int SERIALIZED_LENGTH = sizeof(float)*4 + sizeof(int);
  float x,y,w,h;
  bool locked;
  void serialize(Stream s);
};
struct Obj
{
  string obz_tex_id;
  string obz_board_id;
  ImageData img;
  Rect rect;
  static inline Obj init()
  {
    Obj self;
    self.obz_tex_id.init();
    self.obz_board_id.init();
    self.img = ImageData::init();
    self.rect = {0.f, 0.f, 0.f, 0.f, false};
    return self;
  }
  void destroy();
};
struct Fit
{
  Rect rect;
  int obj_idx;
  bool could_contain(Rect rsmol);
};

struct COBZ;
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
  static inline Cell init()
  {
    Cell self = {{}, -1, -1, 0, 0, {}, {}, {}, {}, {0,0}};
    self.name.init();
    self.actions.init();
    self.obz_child_id.init();
    self.obz_tex_id.init();
    self.obz_id.init();
    return self;
  }
  void set_child_obz(COBZ& cobz, string& obz_id);
  void set_child_idx(COBZ& cobz, int board_idx, int child_idx);
  void serialize(Stream s);
  inline void destroy()
  {
    name.destroy();
    actions.destroy();
    obz_child_id.destroy();
    obz_tex_id.destroy();
    obz_id.destroy();
  }
};

struct Board
{
  int w, h;
  list<Cell> cells;
  string name;
  string obz_id;
  static inline Board init() {
    Board r = { 0, 0, {}, {}, {} };
    r.cells.init();
    r.name.init();
    r.obz_id.init();
    return r;
  }
  inline void make_invalid() { w = h = -1; }
  inline bool is_invalid() { return w <= 0 || h <= 0; }
  void serialize(Stream s);
  void destroy();
};

struct COBZ
{
  list<Obj> textures;
  list<Board> boards;

  static inline COBZ init()
  {
    COBZ self;
    self.textures.init();
    self.boards.init();
    return self;
  }
  
  int has_texture_with_id(const char* id)
  {
    const u32 l = textures.len();
    for (u32 i = 0; i < l; i++)
    {
      if (str_eq(textures[i].obz_tex_id.data(), id))
        return i;
    }
    return -1;
  }
  ivec2 gen_spritesheet_precursors(list<Obj*>& objs, list<Fit>& fit_buf);
  void gen_one_spritesheet(
    ImageData& buffer,
    list<Obj*>& objs
  );
  i64 gen_and_serialize_all_spritesheets(Stream s, long seek_texs);
  void destroy();
};



