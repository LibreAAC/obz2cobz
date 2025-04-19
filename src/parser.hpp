#pragma once
#ifndef H_PARSER
#define H_PARSER
#include "cJSON.h"
#include "cobz.hpp"
#include "zip.h"

struct JSON
{
  cJSON* handle;
  JSON() {}
  JSON(cJSON* j) : handle(j) {}
  JSON(const JSON& j) : handle(j.handle) {}
  JSON(const JSON&& j) : handle(j.handle) {}
  JSON& operator = (const JSON& j) { handle = j.handle; return *this; }
  JSON& operator = (const JSON&& j) { handle = j.handle; return *this; }
  cJSON* to_c() const { return handle; }
  JSON& drop()
  {
    assert(!(handle->type & cJSON_IsReference));
    handle->type |= cJSON_IsReference;
    return *this;
  }
  string force_cast_to_string()
  {
    string ret;
    if (is(cJSON_String))
      return string::own(drop().to_str());
    else if (is(cJSON_Number))
    {
      ret.init();
      ret.prealloc(16);
      snprintf(ret.data(), 15, "%i", to_int());
      ret.data()[15] = 0;
      return ret;
    }
    else
    {
      todo();
      return ret;
    }
  }
  char* to_str() const
  { assert(cJSON_IsString(handle)); return cJSON_GetStringValue(handle); }
  int to_int() const
  { assert(cJSON_IsNumber(handle)); return cJSON_GetNumberValue(handle); }
  float to_float() const
  { assert(cJSON_IsNumber(handle)); return cJSON_GetNumberValue(handle); }
  bool to_bool() const
  { assert(cJSON_IsBool(handle)); return handle->type == cJSON_True; }
  bool is(int cjson_type) const
  { return handle->type == cjson_type; }
  bool has(const char* key)
  { assert(cJSON_IsObject(handle)); return cJSON_HasObjectItem(handle, key); }
  int len() const
  {
    assert(cJSON_IsArray(handle) || cJSON_IsObject(handle));
    if (cJSON_IsArray(handle))
      return cJSON_GetArraySize(handle);
    else
    {
      int ret = 0;
      cJSON* child = handle->child;
      while (child != nullptr)
      {
        ret++;
        child = child->next;
      }
      return ret;
    }
  }
  operator cJSON* ()
  { return handle; }
  inline JSON operator [] (int index)
  { assert(cJSON_IsArray(handle)); return cJSON_GetArrayItem(*this, index); }
  inline JSON operator [] (const char* key)
  { assert(cJSON_IsObject(handle)); return cJSON_GetObjectItem(*this, key); }
};

ivec2 find_position(const char* s, JSON node);
void begin_board_parsing();
void end_board_parsing();
Board parse_board(
  zip_t* z,
  COBZ cobz,
  const char* obf,
  const char* obz_id,
  cJSON manifest
);
COBZ parse_file(const char* obz);


#define cJSON_ObjectForEach(key, value, obj) \
for ( \
  ( \
    value = obj != nullptr ? obj->child : nullptr, \
    key = value->string \
  ); \
  value != nullptr; \
  (value = value->next, key = value != nullptr ? value->string : nullptr) \
)


#endif
