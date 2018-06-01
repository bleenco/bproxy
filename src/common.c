#include "common.h"

/*
 * n represents size of string s. size of find is determinate by strlen
 */
char *strnstr(char *s, size_t n, char *find)
{
  size_t find_len = strlen(find);
  if (find_len == 0)
  {
    return s;
  }
  if (n < find_len)
  {
    return NULL;
  }
  char *s_end = &s[n - find_len + 1];
  while (1)
  {
    while ((*s) != (*find) && s != s_end)
      ++s;
    if (s == s_end)
    {
      return NULL;
    }
    if (strncmp(s++, find, find_len) == 0)
    {
      return --s;
    }
  }
}
