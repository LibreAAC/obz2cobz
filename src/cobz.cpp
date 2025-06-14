#include "img.hpp"
#include "list.hpp"
#include <cmath>
#include <cstdio>
#include "cobz.hpp"

void Rect::serialize(Stream s)
{ s << x << y << w << h; }
bool Fit::could_contain(Rect rsmol)
{ return rect.w >= rsmol.w && rect.h >= rsmol.h; }
void Obj::destroy()
{
  obz_tex_id.destroy();
  obz_board_id.destroy();
  img.destroy();
}

ivec2 COBZ::gen_spritesheet_precursors(list<Obj*>& objs, list<Fit>& fit_buf)
{
  if (objs.len() == 0)
    return {0,0};
  // TODO: ROTATE THE ALGORITHM BECAUSE PASTING LONG LINES IS FASTER
  // THAN LONG COLUMNS (given currently implemented algorithms)
  int minw = 0;
  int maxw = 0;
  for (int i = 0; i < objs.len(); i++)
  {
    int downscale_factor = 0;
    objs[i]->rect.w = objs[i]->img.width();
    objs[i]->rect.h = objs[i]->img.height();
    while (objs[i]->rect.w > 500)
    {
      objs[i]->rect.w = floorf(objs[i]->rect.w / 2);
      objs[i]->rect.h = floorf(objs[i]->rect.h / 2);
      downscale_factor++;
    }
    if (objs[i]->rect.w > maxw)
      maxw = objs[i]->rect.w;
    objs[i]->img.downscale_pow2(downscale_factor);
  }
  // yeah... im lazy...
  // but who wants to write a sorting algorithm in big 2025 anyway ???
  std::sort(objs.data(), objs.data()+(objs.len()), [](Obj* a, Obj* b){
    // NOTE: C++ for increasing order wants a '<'
    // here we want reverse, so '>' it is !
    if (a->rect.w == b->rect.w)
      return a->rect.h > b->rect.h;
    return a->rect.w > b->rect.w;
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
        objs[i]->rect.x+objs[i]->rect.w,
        objs[i]->rect.y,
        maxw - objs[i]->rect.w,
        objs[i]->rect.h
      }, i
    });
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
    if (maxw - (objs[i]->rect.x + objs[i]->rect.w) <= minw)
    {
      base_fixed = i+1;
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
  list<Obj*>& objs
) {
  for (int i = 0; i < objs.len(); i++)
    buffer.paste(objs[i]->img, objs[i]->rect.x, objs[i]->rect.y);
}

i64 COBZ::gen_and_serialize_all_spritesheets(
  Stream s,
  long seek_texs
) {
  // char buf[1024];
  const int board_count = boards.len();
  list<Obj*> objs;
  list<Fit> fit_buf;
  ivec2 ssdims;
  i64 tex_count = 0;
  auto img = ImageData::init();
  objs.init();
  fit_buf.init();
  for (int i = 0; i < board_count; i++)
  {
    printf("Generating spritesheets... %i%%\r", i*100/board_count);
    fflush(stdout);
    objs.clear();
    for (int j = 0; j < textures.len(); j++)
      if (textures[j].obz_board_id == boards[i].obz_id)
        objs.push(&textures[j]);
    if (objs.len() == 0)
      continue;
    ssdims = gen_spritesheet_precursors(objs, fit_buf);
    if (ssdims.x == 0 || ssdims.y == 0)
      continue;
    img = ImageData::create(ssdims.x, ssdims.y);
    gen_one_spritesheet(img, objs);
    tex_count++;
    
    // memset(buf, 0, 1024);
    // snprintf(buf, 1024, "%li.png", tex_count);
    // FILE* temp = fopen(buf, "wb");
    // img.save({temp});
    // fclose(temp);

    {
      fseek(s._f, seek_texs, SEEK_SET);
      s.write_anchor("IMG");
      s << i;
      img.serialize(s);
      seek_texs = ftell(s._f);
      img.destroy();
      // sadly, because of the way stbi works, we have to destroy the image
      // and reallocate it everytime
    }
  }
  puts("Generated spritesheets. 100%                ");
  fit_buf.destroy();
  objs.destroy();
  return tex_count;
}

void COBZ::destroy()
{
  textures.destroy();
  boards.destroy();
}

void Board::serialize(Stream s)
{
  const i64 cell_count = cells.len();
  s << w << h;
  ivec2 pos = {0,0};
  // first give a position to cells without one
  int valid_start = 0;
  for (;
         valid_start < cell_count
      && cells[valid_start].obz_xy.x < 0;
      valid_start++
  ) {}
  if (valid_start > 0)
  {
    int insert_pos = valid_start;
    for (int i = 0; i < valid_start; i++)
    {
      while (pos == cells[insert_pos].obz_xy)
      {
        if (pos.y >= h) break;
        insert_pos++;
        pos.x++;
        if (pos.x >= w)
        {
          pos.x = 0;
          pos.y++;
        }
      }
      if (pos.y >= h)
      {
        fprintf(stderr, "WARN: Failed to find a valid alternative position "
                        "for cell %s. This one and all the other in this case "
                        "will be skipped.\n", cells[0].name.data());
        break;
      }
      cells[0].obz_xy = pos;
      const Cell temp = cells[0];
      for (int j = 0; j < insert_pos-1; j++)
        cells[j] = cells[j+1];
      cells[insert_pos-1] = temp;
      insert_pos--;
    }
    pos = {0,0};
  }
  
  // then serialize all the cells once in order
  for (int i = 0; i < cell_count; i++)
  {
    if (cells[i].obz_xy == pos)
      cells[i].serialize(s);
    else
    {
      Cell::init().serialize(s);
      i--;
    }
    pos.x++;
    if (pos.x >= w)
    {
      pos.x = 0;
      pos.y++;
    }
    if (pos.y >= h)
      break;
  }
  while (pos.y < h)
  {
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
void Cell::set_child_idx(COBZ& cobz, int board_idx, int child_idx)
{ child = child_idx; }
void Cell::serialize(Stream s)
{
  i64 len;
  s.write_anchor("CLL");
  s << (i32&) tex_id;
  len = name.len();
  fwrite(&len, sizeof(len), 1, s._f);
  if (name.data())
    fwrite(name.data(), len, 1, s._f);
  len = actions.len();
  fwrite(&len, sizeof(len), 1, s._f);
  for (i64 i = 0; i < len; i++)
  {
    i64 lena = actions[i].len();
    fwrite(&lena, sizeof(lena), 1, s._f);
    if (actions[i].data())
      fwrite(actions[i].data(), lena, 1, s._f);
  }
  s << (i32&) child;
  s << (u32&) background << (u32&) border;
}




