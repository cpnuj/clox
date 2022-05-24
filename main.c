#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "lexer.h"
#include "vm.h"

VM vm;

void interprete (char *src)
{
  vm_init (&vm);
  compile (src, &vm.chunk);
  // debug_chunk(&vm.chunk, "hello kitty");
  vm_run (&vm);
  if (!vm.error)
  {
    value_print (vm_pop (&vm));
    printf ("\n");
  }
}

static void repl ()
{
  char line[1024];
  Lexer l;
  for (;;)
  {
    printf ("> ");
    if (!fgets (line, sizeof (line), stdin))
    {
      printf ("\n");
      break;
    }
    interprete (line);
  }
}

int main (int argc, char **argv)
{
  repl ();
  return 0;
}
