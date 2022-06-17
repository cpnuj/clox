#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "lexer.h"
#include "vm.h"

struct vm vm;

void interprete(char *src)
{
  compile(src, &vm.chunk);
  // debug_chunk(&vm.chunk, "hello kitty");
  vm_run(&vm);
  if (vm.error) {
    printf("%s\n", vm.errmsg);
  }
  // reset error
  vm.error = 0;
}

static void repl()
{
  char line[1024];
  struct lexer l;
  vm_init(&vm);
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interprete(line);
  }
}

int main(int argc, char **argv)
{
  repl();
  return 0;
}
