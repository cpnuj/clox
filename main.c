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
  initVM (&vm);
  Compile (src, &vm.chunk);
  // disassembleChunk(&vm.chunk, "hello kitty");
  VMrun (&vm);
  if (!vm.error)
  {
    printValue (VMpop (&vm));
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
