#include "parser.hpp"
#include "cJSON.h"
#include "img.hpp"
#include "utils.hpp"
#include "zip.h"
#define MINIZ_HEADER_FILE_ONLY
#include "miniz.h"
#include "colors.hpp"
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <thread>

auto _obf_load_buf = ByteBuffer::init();
auto obf_load = DynByteBuffer::from(_obf_load_buf);

void begin_board_parsing()
{
  assert(obf_load.data<void>() == nullptr);
  obf_load.prealloc_at_least(2048);
}
void end_board_parsing()
{
  assert(obf_load.buffer.owned);
  obf_load.buffer.destroy();
}

void insert_ordered(list<Cell>& cells, Cell new_cell)
{
  int i = 0;
  for (; i < cells.len() && cells[i].obz_xy < new_cell.obz_xy; i++);
  cells.insert(new_cell, i);
}

ivec2 find_position(const char* s, JSON node)
{
  assert(node.is(cJSON_Array));
  const int h = node.len();
  for (int y = 0; y < h; y++)
  {
    const int w = node[y].len();
    for (int x = 0; x < w; x++)
    {
      const JSON j = node[y][x];
      if (j.is(cJSON_Number))
      {
        const int stoi = atoi(s);
        if (stoi == j.to_int())
          return {x, y};
      }
      else if (j.is(cJSON_NULL))
      {
        continue;
      }
      else
      {
        assert(j.is(cJSON_String));
        if (str_eq(j.to_str(), s))
          return {x, y};
      }
    }
  }
  return {-1, -1};
}

Board parse_board(
  zip_t* z,
  COBZ& cobz,
  const char* obf_path,
  const char* obz_id,
  JSON manifest
) {
  auto board = Board::init();
  zip_entry_opencasesensitive(z, obf_path);
  {
    const u64 size = zip_entry_size(z);
    obf_load.prealloc_at_least(size+1);
    zip_entry_noallocread(z, obf_load.data<void>(), size);
    obf_load.set_len(size);
    obf_load.data<char>()[size] = 0;
  }
  zip_entry_close(z);
  JSON obf = cJSON_Parse(obf_load.data<char>());
  if (!obf)
  {
    const char* err = cJSON_GetErrorPtr();
    fprintf(stderr, "ERR: [cJSON] %s", err);
    return board;
  }

  { // id
    board.obz_id = string::own(obf["id"].drop().to_str());
    if (!str_eq(board.obz_id.data(),obz_id))
    {
      fprintf(stderr,
  "WARN: ID mismatch (manifest specifies %s while description specifies %s)\n",
        obz_id, board.obz_id.data()
      );
    }
  }

  { // general stuff
    board.name = string::ref(obf["name"].to_str()).realloc();
    fprintf(stderr, "Loading board named %s\n", board.name.data());
    board.parent_idx = -1;
    board.w = obf["grid"]["columns"].to_int();
    board.h = obf["grid"]["rows"].to_int();
  }

  { // cells
    JSON b;
    board.cells.prealloc(obf["buttons"].len());
    cJSON_ArrayForEach(b.handle, obf["buttons"].handle)
    {
      auto c = Cell::init();
      JSON idcont = b["id"];
      c.obz_id = idcont.force_cast_to_string();
      if (b.has("label"))
      {
        c.name = string::own(b["label"].drop().to_str());
      }
      if (b.has("image_id"))
      {
        const int texi = cobz.has_texture_with_id(b["image_id"].to_str());
        Obj* tex = &cobz.textures[texi];
        if (tex)
        {
          // both live for the same amount of time
          // no need to do unecessary allocation
          c.obz_tex_id.ref(tex->obz_tex_id);
          tex->obz_board_id.ref(board.obz_id);
        }
        else
        {
          const char* img_src = nullptr;
          JSON i;
          cJSON_ArrayForEach(i.handle, obf["images"].handle)
          {
            if (i["id"] == b["id"])
            {
              if (i.has("url"))
                img_src = i["url"].to_str();
              else if (i.has("data"))
                img_src = i["data"].to_str();
              else
                fprintf(stderr, "ERR: No known image source found.\n");
              break;
            }
          }
          auto obj = Obj::init();
          obj.img = load_img(z, img_src, c.name.data());
          if (obj.img.is_valid())
          {
            obj.obz_tex_id = string::own(b["image_id"].drop().to_str());
            obj.obz_board_id.ref(board.obz_id);
            // Rect is left undefined here.
            c.obz_tex_id.ref(obj.obz_tex_id);
            cobz.textures.push(obj);
          }
          else
            c.tex_id = -1;
        }
      }
      if (b.has("load_board"))
      {
        if (b["load_board"].has("id"))
        {
          c.obz_child_id =
            string::own(b["load_board"]["id"].drop().to_str());
        }
        else
        {
          // TODO: handle other ways of linking boards
          assert(b["load_board"].has("path"));
          JSON obj = manifest["paths"]["boards"], val;
          char* key;
          const char* ld_board_path = b["load_board"]["path"].to_str();
          cJSON_ObjectForEach(key, val.handle, obj.handle)
          {
            if (str_eq(ld_board_path, val.to_str()))
            {
              c.obz_child_id = string::own(key);
              val.handle->string = nullptr; // drop ownership of the key 
            }
          }
          if (c.obz_child_id.data() == nullptr)
            fprintf(stderr, "ERR: Could not find board with path %s", ld_board_path);
        }
      }
      if (b.has("actions"))
      {
        JSON j_actions = b["actions"], it;
        c.actions.prealloc(j_actions.len());
        cJSON_ArrayForEach(it.handle, j_actions.handle)
        {
          c.actions.push(string::own(it.drop().to_str()));
        }
      }
      else if (b.has("action"))
      {
        c.actions.push(string::own(b["action"].drop().to_str()));
        assert(c.actions.len() == 1);
      }
      else if (c.name.data() != nullptr)
      {
        c.actions.push(string::ref(c.name.data()).prextend({(char*)"+ ",2,2}));
      }
      if (b.has("background_color"))
        c.background = parse_color(b["background_color"].to_str(), 0);
      if (b.has("border_color"))
        c.border = parse_color(b["border_color"].to_str(), 0);
      c.obz_xy = find_position(c.obz_id.data(), obf["grid"]["order"]);
      insert_ordered(board.cells, c);
      if (c.obz_xy.x < 0)
      {
        fprintf(
          stderr,
          "WARN: Cell '%s' will be misplaced because it doesn't have a "
          "grid position.\n",
          c.name.data()
        );
      }
    }
  }
  // NOTE: missing cells are only added when serializing
  // (they won't take any memory)
  obf_load.clear();
  cJSON_Delete(obf.handle);
  return board;
}


#define MZ_ZIP_LOCAL_DIR_HEADER_SIZE 30

// Do not hide your structures please. For the love of whoever you want.
// Do not hide your structures.
// Do. not. hide. your. structures.
typedef struct {
  mz_zip_archive *m_pZip;
  mz_uint64 m_cur_archive_file_ofs;
  mz_uint64 m_comp_size;
} mz_zip_writer_add_state;
struct zip_entry_t {
  ssize_t index;
  char *name;
  mz_uint64 uncomp_size;
  mz_uint64 comp_size;
  mz_uint32 uncomp_crc32;
  mz_uint64 dir_offset;
  mz_uint8 header[MZ_ZIP_LOCAL_DIR_HEADER_SIZE];
  mz_uint64 header_offset;
  mz_uint16 method;
  mz_zip_writer_add_state state;
  tdefl_compressor comp;
  mz_uint32 external_attr;
  time_t m_time;
};
struct zip_t {
  mz_zip_archive archive;
  mz_uint level;
  struct zip_entry_t entry;
};

constexpr int zip_t_sizeof = sizeof(zip_t);

struct Batch
{
  char z[zip_t_sizeof];
  JSON jason; // not owned
  View<Obj> inout;
  int th_id;
};
struct Thread
{ // this is ridiculous
  pthread_t th;
  void start(void*(*func)(void*), void* arg)
  { pthread_create(&th, nullptr, func, arg); }
  void join()
  { pthread_join(th, nullptr); }
};
struct ThreadData
{
  Batch thread_batch;
  Thread thread;
};

void* _image_preloader_batch(
  void* _batch
)
{
  Batch& b = *(Batch*)_batch;
  for (int i = 0; i < b.inout.len; i++)
  {
    // const int W = atoi(getenv("COLUMNS"));
    // const int INFO_W = 6 + 2; // info width + square brackets
    // printf("\033[%iB[", b.th_id);
    // int j = 1;
    // for (; j < i*(W-INFO_W)/b.inout.len; j++)
    //   putchar('=');
    // putchar('>');
    // for (j++; j < W-INFO_W; j++)
    //   putchar(' ');
    // printf("] %03i%% \r\033[%iA",(int)(i*100/b.inout.len), b.th_id);
    // fflush(stdout);
    auto obj = Obj::init();
    obj.obz_tex_id = string::ref(b.jason.handle->string).realloc();
    obj.img = load_img((zip_t*)(char*)b.z, b.jason.to_str(), obj.obz_tex_id.data());
    b.inout[i] = obj;
    b.jason.handle = b.jason.handle->next;
  }
  return NULL;
}

COBZ parse_file(const char *obz)
{
  constexpr int MIN_BATCH_SIZE = 256;
  auto cobz = COBZ::init();
  zip_t* z = nullptr;
  int i, n, cpu_count;
  int manifest_index = -1;
  JSON manifest = {nullptr};
  
  z = zip_open(obz, 0, 'r');

  { // find manifest.json
    n = zip_entries_total(z);
    for (i = 0; i < n; ++i) {
      zip_entry_openbyindex(z, i);
      const char *name = zip_entry_name(z);
      if (str_eq(name, "manifest.json"))
      {
        manifest_index = i;
        break;
      }
      zip_entry_close(z);
    }

    if (manifest_index == -1)
    {
      fprintf(stderr, "ERR: Missing 'manifest.json'.\n");
      goto ERROR;
    }
  }

  begin_board_parsing();

  { // load and check manifest.json
    zip_entry_openbyindex(z, manifest_index);
    {
      const u64 size = zip_entry_size(z);
      obf_load.prealloc_at_least(size+1);
      zip_entry_noallocread(z, obf_load.data<void>(), size);
      obf_load.set_len(size);
      obf_load.data<char>()[size] = 0;
    }
    zip_entry_close(z);
    
    manifest = cJSON_Parse(obf_load.data<char>());
    obf_load.clear();

    if (!manifest.has("format") || !str_eq(manifest["format"].to_str(), "open-board-0.1"))
    {
      fprintf(stderr, "ERR: Unknown board set format: %s\n", manifest["format"].to_str());
      goto ERROR;
    }
  }

  if (manifest["paths"].has("sounds") && manifest["paths"]["sounds"].len() != 0)
  {
    fprintf(stderr, "ERR: Custom sounds are not yet supported.\n");
    goto ERROR;
  }

  { // images preloading
    printf("Preloading referenced images:\n");
    const int total = manifest["paths"]["images"].len();
    if (total >= MIN_BATCH_SIZE*2)
    {
      list<ThreadData> threads;
      JSON ls_ptr = {manifest["paths"]["images"].handle->child};
      cpu_count = std::thread::hardware_concurrency();
      if (cpu_count == 0)
      {
        cpu_count = 4; // Default to 4 threads in case of error
fprintf(stderr, "WARN: Retrieving number of core failed, defaulting to 4.\n");
      }
      threads.init();
      threads.prealloc(cpu_count-1);
      cobz.textures.prealloc(total);
      cobz.textures.set_len(total);
      const int BATCH_SIZE = total / cpu_count;
      const int LEFT_OVER = total - BATCH_SIZE*(cpu_count-1); // yummy !
      for (int i = 0; i < cpu_count-1; i++)
      {
        threads.push({});
        ThreadData& th = threads[-1];
        th.thread_batch.inout =
          {cobz.textures.data()+BATCH_SIZE*i,BATCH_SIZE,0};
        th.thread_batch.jason = ls_ptr;
        th.thread_batch.th_id = i;
        memcpy(th.thread_batch.z, z, zip_t_sizeof);
        th.thread.start(_image_preloader_batch, &th.thread_batch);
        for (int j = 0; j < BATCH_SIZE; j++)
          ls_ptr.handle = ls_ptr.handle->next;
      }
      Batch current_th_batch;
      current_th_batch.inout = {cobz.textures.data()+BATCH_SIZE*(cpu_count-1),LEFT_OVER,0};
      current_th_batch.jason = ls_ptr;
      current_th_batch.th_id = cpu_count-1;
      memcpy(current_th_batch.z, z, zip_t_sizeof);
      _image_preloader_batch(&current_th_batch);
      for (int i = 0; i < cpu_count-1; i++)
      {
        threads[i].thread.join();
      }
      threads.destroy();
    }
    else
    {
      Batch current_th_batch;
      cobz.textures.prealloc(total);
      cobz.textures.set_len(total);
      current_th_batch.inout = {cobz.textures.data(),total,0};
      current_th_batch.jason = {manifest["paths"]["images"].handle->child};
      current_th_batch.th_id = 0;
      memcpy(current_th_batch.z, z, zip_t_sizeof);
      _image_preloader_batch(&current_th_batch);
    }
  }

  { // board parsing
    JSON dict = manifest["paths"]["boards"], board_path;
    const char* board_id;
    cJSON_ObjectForEach(board_id, board_path.handle, dict.handle)
    {
      cobz.boards.push(parse_board(
        z, cobz, board_path.to_str(), board_id, manifest
      ));
    }
  }

  // Board optimization/actual compilation
  { // 1. Put root first
    const char* root_path = manifest["root"].to_str();
    const int board_count = cobz.boards.len();
    const char* iter_board_id;
    JSON board_list = manifest["paths"]["boards"], path;
    int root_idx = 0;
    Board temp;
    cJSON_ObjectForEach(iter_board_id, path.handle, board_list.handle)
    {
      if (str_eq(path.to_str(), root_path))
        break;
      root_idx++;
    }
    if (root_idx >= board_count)
    {
      fprintf(stderr, "ERR: Failed to found root board (%s).\n", root_path);
      goto ERROR;
    }
    temp = cobz.boards[0];
    cobz.boards[0] = cobz.boards[root_idx];
    cobz.boards[root_idx] = temp;
  }

  { // 2. Resolve indicies
    const int board_count = cobz.boards.len();
    const int tex_count = cobz.textures.len();
    for (int i = 0; i < board_count; i++)
    {
      Board& board = cobz.boards[i];
      const int cell_count = board.cells.len();
      for (int j = 0; j < cell_count; j++)
      {
        Cell& cell = board.cells[j];
        const int p = cobz.has_texture_with_id(cell.obz_tex_id.data());
        if (p != -1)
        {
          cell.tex_id = p;
        }
        else
        {
          printf("WARN: texture with id [%s] not found !\n", cell.obz_tex_id.data());
          cell.tex_id = -1;
        }
        for (int k = 0; k < board_count; k++)
        {
          if (
            (cell.obz_child_id.is_empty() &&
             !cell.obz_child_id.is_owned() &&
             (cobz.boards[k].name == cell.name)
            ) || (cobz.boards[k].obz_id == cell.obz_child_id)
          ) {
            cell.set_child_idx(cobz, i, k);
            break;
          }
        } // goofy aahh stairs
      }
    }
  }

  zip_close(z);
  cJSON_Delete(manifest.handle);
  end_board_parsing();
  return cobz;

ERROR:
  if (z)
    zip_close(z);
  if (manifest.handle)
    cJSON_Delete(manifest.handle);
  if (_obf_load_buf.owned)
    end_board_parsing();
  cobz.destroy();
  return cobz;
}

