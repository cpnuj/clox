#include "keyword.h"
#include <stdio.h>

int main ()
{
  if (CloxKeyword ("for", 3) != TK_FOR)
  {
    printf ("fail\n");
  }
  else
  {
    printf ("ok\n");
  }
  if (CloxKeyword ("while", 5) != TK_WHILE)
  {
    printf ("fail\n");
  }
  else
  {
    printf ("ok\n");
  }
  if (CloxKeyword ("nokeyword", 9) != -1)
  {
    printf ("fail\n");
  }
  else
  {
    printf ("ok\n");
  }
  return 0;
}
