#include "img.hpp"
#include "list.hpp"
#include <cmath>
#include <cstdio>
#include "cobz.hpp"

void Rect::serialize(Stream s)
{ s << spritesheet_id << x << y << w << h; }
bool Fit::could_contain(Rect rsmol)
{ return rect.w >= rsmol.w && rect.h >= rsmol.h; }
void Obj::destroy()
{
  obz_tex_id.destroy();
  obz_board_id.destroy();
}

ivec2 COBZ::gen_spritesheet_precursors(list<Obj*>& objs, list<Fit>& fit_buf)
{
  // TODO: ROTATE THE ALGORITHM BECAUSE PASTING LONG LINES IS FASTER
  // THAN LONG COLUMNS (given currently implemented algorithms)
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
      downscale_factor++;
    }
    if (objs[i]->rect.w > maxw)
      maxw = objs[i]->rect.w;
    objs[i]->img.downscale_pow2(downscale_factor);
  }
  maxw *= 2;
  // yeah... im lazy...
  // but who wants to write a sorting algorithm in big 2025 anyway ???
  std::sort(objs.data(), objs.data()+(objs.len()), [](Obj* a, Obj* b){
    if (a->rect.w == b->rect.w)
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
  list<Fit>& fits = fit_buf;
  if (fits.cap() < objs.len())
    fits.prealloc(objs.len());
  fits.clear();
  for (int i = 0; i < objs.len(); i++)
  {
    fits.push(Fit{
      {
        objs[i]->rect.w,
        objs[i]->rect.y,
        maxw - objs[i]->rect.w,
        objs[i]->rect.h
      }, i
    });
    printf("%f,%f,%f,%f\n", fits[-1].rect.x,fits[-1].rect.y,fits[-1].rect.w,fits[-1].rect.h);
  }
  
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
void COBZ::gen_one_spritesheet(
  ImageData& buffer,
  list<Obj*>& objs,
  int spritesheet_id
) {
  for (int i = 0; i < objs.len(); i++)
  {
    assert(objs[i]->rect.spritesheet_id == -1 || objs[i]->rect.spritesheet_id == spritesheet_id);
    objs[i]->rect.spritesheet_id = spritesheet_id;
    buffer.paste(objs[i]->img, objs[i]->rect.x, objs[i]->rect.y);
  }
}

void COBZ::gen_and_serialize_all_spritesheets(
  Stream s,
  long seek_rects,
  long seek_texs
) {
  const int board_count = boards.len();
  list<Obj*> objs;
  list<Fit> fit_buf;
  ivec2 ssdims;
  auto img = ImageData::init();
  objs.init();
  fit_buf.init();
  for (int i = 0; i < board_count; i++)
  {
    printf("Generating spritesheets... %i%%\r", i*100/board_count);
    objs.clear();
    for (int j = 0; j < textures.len(); j++)
      if (textures[j].obz_board_id == boards[i].obz_id)
        objs.push(&textures[j]);
    ssdims = gen_spritesheet_precursors(objs, fit_buf);
    img = ImageData::create(ssdims.x, ssdims.y);
    gen_one_spritesheet(img, objs, i);

    {
      fseek(s._f, seek_rects, SEEK_SET);
      for (int j = 0; j < objs.len(); j++)
      {
        objs[j]->rect.serialize(s);
      }
      seek_rects = ftell(s._f);
    }

    {
      fseek(s._f, seek_texs, SEEK_SET);
      img.serialize(s);
      seek_texs = ftell(s._f);
      img.destroy();
      // sadly, because of the way stbi works, we have to destroy the image
      // and reallocate it everytime
    }
  }
  puts("Generated spritesheets. 100%                ");
}

void COBZ::destroy()
{
  textures.destroy();
  boards.destroy();
}

void Board::serialize(Stream s)
{
  const i64 cell_count = cells.len();
  s << w << h << parent_idx << cell_count;
  ivec2 pos = {0,0};
  for (int i = 0; i < cell_count; i++)
  {
    if (cells[i].obz_xy == pos)
      cells[i].serialize(s);
    else
      Cell::init().serialize(s);
    pos.x++;
    if (pos.x >= w)
    {
      pos.x = 0;
      pos.y++;
    }
  }
}

void Board::destroy()
{
  cells.destroy();
  name.destroy();
  obz_id.destroy();
}

void Cell::set_child_obz(COBZ& cobz, string& obz_id)
{ obz_child_id.hold(obz_id); }
void Cell::set_child_idx(COBZ& cobz, int idx)
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
void Cell::serialize(Stream s)
{
  i64 len;
  s.write_anchor("CLL");
  s << (i32&) tex_id;
  len = name.len()+1;
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




