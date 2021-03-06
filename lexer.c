#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

void lexer_skip_whitespace(struct lexer *l);
int lexer_end(struct lexer *l);
char lexer_peek(struct lexer *l);
char lexer_peeknext(struct lexer *l);
char lexer_forward(struct lexer *l);
int lexer_match(struct lexer *l, char c);
void lexer_error(struct lexer *l, char *errmsg);

int iskeyword(char *s, int len);

struct token mktoken(struct lexer *l, token_t type);

token_t lex_number(struct lexer *l);
token_t lex_ident(struct lexer *l);
token_t lex_string(struct lexer *l);

void lexer_skip_whitespace(struct lexer *l)
{
  for (;;) {
    char c = lexer_peek(l);
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      lexer_forward(l);
      break;
    // new line
    case '\n':
      l->line++;
      lexer_forward(l);
      break;
    // comment
    case '/':
      if (lexer_peeknext(l) == '/') {
        while (!lexer_end(l) && lexer_forward(l) != '\n')
          ;
        l->line++;
        break;
      } else {
        // single slash at peek, just return
        return;
      }
    default:
      return;
    }
  }
}

int lexer_end(struct lexer *l) { return l->end >= l->len; }

char lexer_peek(struct lexer *l) { return lexer_end(l) ? 0 : l->src[l->end]; }

char lexer_peeknext(struct lexer *l)
{
  return (l->end + 1 >= l->len) ? 0 : l->src[l->end + 1];
}

char lexer_forward(struct lexer *l)
{
  if (lexer_end(l)) {
    return 0;
  }
  char ret = l->src[l->end];
  l->end++;
  return ret;
}

int lexer_match(struct lexer *l, char c)
{
  if (lexer_peek(l) == c) {
    lexer_forward(l);
    return 1;
  }
  return 0;
}

void lexer_error(struct lexer *l, char *errmsg)
{
  l->err = 1;
  if (lexer_end(l)) {
    sprintf(l->errmsg, "[line %d] Error at end: %s", l->line, errmsg);
  } else {
    sprintf(l->errmsg, "[line %d] Error at '%c': %s", l->line, lexer_peek(l),
            errmsg);
  }
}

int iskeyword(char *s, int len) { return clox_keyword(s, len); }

struct token mktoken(struct lexer *l, token_t type)
{
  struct token tk = {
    .type = type,
    .line = l->line,
    .at = l->src + l->start,
    .len = l->end - l->start,
  };
  return tk;
}

token_t lex_number(struct lexer *l)
{
  int hasDot = 0;
  for (;;) {
    char peek = lexer_peek(l);
    if (isdigit(peek)) {
      lexer_forward(l);
    } else if (peek == '.') {
      if (hasDot) {
        lexer_error(l, "expect digit");
        return TK_ERR;
      }
      hasDot = 1;
      lexer_forward(l);
      if (!isdigit(lexer_peek(l))) {
        lexer_error(l, "Expect property name after '.'.");
        return TK_ERR;
      }
    } else {
      return TK_NUMBER;
    }
  }
}

token_t lex_ident(struct lexer *l)
{
  char c = lexer_peek(l);
  while (isdigit(c) || isalpha(c)) {
    lexer_forward(l);
    c = lexer_peek(l);
  }
  int tk = iskeyword(l->src + l->start, l->end - l->start);
  if (tk > 0) {
    return tk;
  }
  return TK_IDENT;
}

token_t lex_string(struct lexer *l)
{
  while (lexer_forward(l) != '"') {
    if (lexer_end(l)) {
      lexer_error(l, "unclosed \" for string literal");
      return TK_ERR;
    }
  }
  return TK_STRING;
}

struct lexer *lex_new(char *src, int len)
{
  struct lexer *l = (struct lexer *)malloc(sizeof(struct lexer));
  lex_init(l, src, len);
  return l;
}

void lex_init(struct lexer *l, char *src, int len)
{
  l->start = 0;
  l->end = 0;
  l->line = 1;
  l->len = len;
  l->src = src;
  l->err = 0;
}

struct token lex(struct lexer *l)
{
  if (l->err)
    return mktoken(l, TK_ERR);

  lexer_skip_whitespace(l);
  if (lexer_end(l))
    return mktoken(l, TK_EOF);

  l->start = l->end;

  char c = lexer_forward(l);
  // notation
  switch (c) {
  // Single-character tokens.
  case '(':
    return mktoken(l, TK_LEFT_PAREN);
  case ')':
    return mktoken(l, TK_RIGHT_PAREN);
  case '{':
    return mktoken(l, TK_LEFT_BRACE);
  case '}':
    return mktoken(l, TK_RIGHT_BRACE);
  case ',':
    return mktoken(l, TK_COMMA);
  case '.':
    return mktoken(l, TK_DOT);
  case '-':
    return mktoken(l, TK_MINUS);
  case '+':
    return mktoken(l, TK_PLUS);
  case ';':
    return mktoken(l, TK_SEMICOLON);
  case '/':
    return mktoken(l, TK_SLASH);
  case '*':
    return mktoken(l, TK_STAR);

  // Single-character tokens.
  case '!':
    return mktoken(l, lexer_match(l, '=') ? TK_BANG_EQUAL : TK_BANG);
  case '=':
    return mktoken(l, lexer_match(l, '=') ? TK_EQUAL_EQUAL : TK_EQUAL);
  case '>':
    return mktoken(l, lexer_match(l, '=') ? TK_GREATER_EQUAL : TK_GREATER);
  case '<':
    return mktoken(l, lexer_match(l, '=') ? TK_LESS_EQUAL : TK_LESS);
  }

  // string
  if (c == '"')
    return mktoken(l, lex_string(l));
  // number
  if (isdigit(c))
    return mktoken(l, lex_number(l));
  // identifier
  if (isalpha(c))
    return mktoken(l, lex_ident(l));
  // error
  lexer_error(l, "lex error");
  return mktoken(l, TK_ERR);
}

char *lex_error(struct lexer *l)
{
  if (l->err) {
    return l->errmsg;
  }
  return 0;
}

token_t token_type(struct token *token) { return token->type; }

int token_line(struct token *token) { return token->line; }

char *token_lexem(struct token *token, char *dest)
{
  sprintf(dest, "%.*s", token->len, token->at);
  return dest;
}

char *token_lexem_start(struct token *token) { return token->at; }

int token_lexem_len(struct token *token) { return token->len; }
