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
  int err = compile(src, value_as_fun(vm.vmain), &vm.constants);
  if (err) {
    exit(74);
  }
  vm_run(&vm);
  vm.error = 0;
}

static void repl()
{
  char line[1024];
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interprete(line);
  }
}

static char *read_file(const char *filename)
{
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", filename);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fsize = ftell(file);
  rewind(file);

  char *buf = (char *)malloc(fsize + 1);
  if (buf == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", filename);
    exit(74);
  }

  size_t bytes_read = fread(buf, sizeof(char), fsize, file);
  if (bytes_read != fsize) {
    fprintf(stderr, "Could not read file \"%s\".\n", filename);
    exit(74);
  }

  buf[bytes_read] = '\0';

  fclose(file);
  return buf;
}

static void run_file(const char *filename)
{
  char *src = read_file(filename);
  interprete(src);
}

int main(int argc, char **argv)
{
  vm_init(&vm);

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }

  return 0;
}
