#include "utils.hpp"
#include "tables.hpp"
float depercent(char* expr, float max)
{
  const int len = str_len(expr);
  if (expr[len-1] == '%')
  {
    expr[len-1] = 0;
    // const float r = strtof(expr, nullptr)/100.f * max;
    const float r = atof(expr)/100.f*max;
    expr[len-1] = '%';
    return r;
  }
  else
    return atof(expr);
}
u32 hexchar(char c)
{
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= '0' && c <= '9') return c - '0';
  fprintf(stderr, "WARN: Invalid hexadecimal value: %c\n", c);
  return 0;
}
#define REVERSE32(X) ((X&0xff)<<24)|(((X>>8)&0xff)<<16)|(((X>>16)&0xff)<<8)|((X>>24)&0xff)
u32 rgba(u32 r, u32 g, u32 b, u32 a) { return r | (g << 8) | (b << 16) | (a << 24); }
u32 parse_color(char* expr, u32 default_)
{
#define FIND_NEXT_CHAR(IDX, CHAR) \
    while (expr[IDX] != CHAR && expr[IDX] != 0) IDX++; \
    if (expr[IDX] == 0) \
    { fprintf(stderr, "WARN: Inavlid rgb color format."); return default_; }
#define READ_COLOR_COMP(COMP, NEXT_END) \
    FIND_NEXT_CHAR(comma, NEXT_END); \
    expr[comma] = 0; \
    COMP = depercent(expr+prev, 255); \
    comma++; prev = comma;

  const int len = str_len(expr);
  if (len == 0)
    return default_;
  if (str_startswith(expr, "rgb(") && str_endswith(expr, ")"))
  {
    int comma = 4, prev = 4;
    u32 r,g,b;
    READ_COLOR_COMP(r, ',');
    READ_COLOR_COMP(g, ',');
    READ_COLOR_COMP(b, ')');
    return rgba(r,g,b,255);
  }
  else if (
    str_startswith(expr, "rgba(") && str_endswith(expr, ")")
  ) {
    int comma = 5, prev = 5;
    u32 r,g,b,a;
    READ_COLOR_COMP(r, ',');
    READ_COLOR_COMP(g, ',');
    READ_COLOR_COMP(b, ',');
    READ_COLOR_COMP(a, ')');
    return rgba(r,g,b,a);
  }
  else if (expr[0] == '#')
  {
    if (len == 7)
    {
      u32 ret = 0;
      for (int i = 0; i < 6; i++)
      { ret |= hexchar(expr[i+1]) << (i*4 + 8); }
      return REVERSE32(ret);
    }
    else if (len == 9)
    {
      u32 ret = 0;
      for (int i = 0; i < 8; i++)
      { ret |= hexchar(expr[i+1]) << (i*4); }
      return REVERSE32(ret);
    }
    else
    {
      fprintf(
        stderr,
        "WARN: Expected 6 or 8 digit hex color format, but got '%s'\n",
        expr
      );
      return default_;
    }
  }
  else
  {
    const i64 legacy_color = u32_legacy_color(expr);
    if (legacy_color != -1)
      return legacy_color;
    fprintf(stderr, "WARN: Unknown color format: '%s'\n", expr);
    return default_;
  }
}
