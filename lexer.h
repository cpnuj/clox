#ifndef clox_lexer_h
#define clox_lexer_h

#include "keyword.h" // include keyword.h to include keyword tokens

typedef int token_t;

#define TK_ERR -1
#define TK_EOF 0

#define TK_NUMBER 1
#define TK_IDENT 2
#define TK_STRING 3

// Single-character tokens.
#define TK_LEFT_PAREN 4
#define TK_RIGHT_PAREN 5
#define TK_LEFT_BRACE 6
#define TK_RIGHT_BRACE 7
#define TK_COMMA 8
#define TK_DOT 9
#define TK_MINUS 10
#define TK_PLUS 11
#define TK_SEMICOLON 12
#define TK_SLASH 13
#define TK_STAR 14

// One or two character tokens.
#define TK_BANG 15
#define TK_BANG_EQUAL 16
#define TK_EQUAL 17
#define TK_EQUAL_EQUAL 18
#define TK_GREATER 19
#define TK_GREATER_EQUAL 20
#define TK_LESS 21
#define TK_LESS_EQUAL 22

typedef struct {
  int start;
  int end;
  int line; // current line
  int len;
  char *src;

  int err;
  char errmsg[128];
} Lexer;

typedef struct {
  token_t type;
  int line;
  char *at; // the lexem begins at source
  int len;  // the lexem's len
} Token;

Lexer *lex_new(char *src, int len);
void lex_init(Lexer *l, char *src, int len);
Token lex(Lexer *l);
char *lex_error(Lexer *l);

char *token_lexem(Token *token, char *dst);
char *token_lexem_start(Token *token);
int token_lexem_len(Token *token);

#endif
