#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <raylib.h>
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
"to parse in C.\n";


void expect(bool cond, const char* const err_msg)
{
  if (!cond)
  {
    dblog(LOG_FATAL, "Expect failed: %s", err_msg);
    abort();
  }
}
void want(bool cond, const char* const warn_msg)
{
  if (!cond)
    dblog(LOG_WARNING, "%s", warn_msg);
}



int main()
{
  puts("Hello");
  return 0;
}
