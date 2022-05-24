#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

void lexer_skip_whitespace (Lexer *l);
int lexer_end (Lexer *l);
char lexer_peek (Lexer *l);
char lexer_peeknext (Lexer *l);
char lexer_forward (Lexer *l);
int lexer_match (Lexer *l, char c);
void lexer_error (Lexer *l, char *errmsg);

int iskeyword (char *s, int len);

Token mktoken (Lexer *l, TokenType type);

TokenType lex_number (Lexer *l);
TokenType lex_ident (Lexer *l);
TokenType lex_string (Lexer *l);

void lexer_skip_whitespace (Lexer *l)
{
  for (;;)
  {
    char c = lexer_peek (l);
    switch (c)
    {
      case ' ':
      case '\r':
      case '\t':
        lexer_forward (l);
        break;
      // new line
      case '\n':
        l->line++;
        lexer_forward (l);
        break;
      // comment
      case '/':
        if (lexer_peeknext (l) == '/')
        {
          while (!lexer_end (l) && lexer_forward (l) != '\n')
          {
          }
          return;
        }
      default:
        return;
    }
  }
}

int lexer_end (Lexer *l) { return l->end >= l->len; }

char lexer_peek (Lexer *l) { return lexer_end (l) ? 0 : l->src[l->end]; }

char lexer_peeknext (Lexer *l)
{
  return (l->end + 1 >= l->len) ? 0 : l->src[l->end + 1];
}

char lexer_forward (Lexer *l)
{
  if (lexer_end (l))
  {
    return 0;
  }
  char ret = l->src[l->end];
  l->end++;
  return ret;
}

int lexer_match (Lexer *l, char c)
{
  if (lexer_peek (l) == c)
  {
    lexer_forward (l);
    return 1;
  }
  return 0;
}

void lexer_error (Lexer *l, char *errmsg)
{
  l->err = 1;
  l->errmsg = errmsg;
}

int iskeyword (char *s, int len) { return CloxKeyword (s, len); }

Token mktoken (Lexer *l, TokenType type)
{
  Token tk = {
    .type = type,
    .line = l->line,
    .at = l->src + l->start,
    .len = l->end - l->start,
  };
  return tk;
}

TokenType lex_number (Lexer *l)
{
  int hasDot = 0;
  for (;;)
  {
    char peek = lexer_peek (l);
    if (isdigit (peek))
    {
      lexer_forward (l);
    }
    else if (peek == '.')
    {
      if (hasDot)
      {
        lexer_error (l, "expect digit");
        return TK_ERR;
      }
      hasDot = 1;
      lexer_forward (l);
    }
    else
    {
      return TK_NUMBER;
    }
  }
}

TokenType lex_ident (Lexer *l)
{
  char c = lexer_peek (l);
  while (isdigit (c) || isalpha (c))
  {
    lexer_forward (l);
    c = lexer_peek (l);
  }
  int tk = iskeyword (l->src + l->start, l->end - l->start);
  if (tk > 0)
  {
    return tk;
  }
  return TK_IDENT;
}

TokenType lex_string (Lexer *l)
{
  while (lexer_forward (l) != '"')
  {
    if (lexer_end (l))
    {
      lexer_error (l, "unclosed \" for string literal");
      return TK_ERR;
    }
  }
  return TK_STRING;
}

Lexer *lex_new (char *src, int len)
{
  Lexer *l = (Lexer *)malloc (sizeof (Lexer));
  lex_init (l, src, len);
  return l;
}

void lex_init (Lexer *l, char *src, int len)
{
  l->start = 0;
  l->end = 0;
  l->line = 1;
  l->len = len;
  l->src = src;
  l->err = 0;
}

Token lex (Lexer *l)
{
  if (l->err)
    return mktoken (l, TK_ERR);

  lexer_skip_whitespace (l);
  if (lexer_end (l))
    return mktoken (l, TK_EOF);

  l->start = l->end;

  char c = lexer_forward (l);
  // notation
  switch (c)
  {
    // Single-character tokens.
    case '(':
      return mktoken (l, TK_LEFT_PAREN);
    case ')':
      return mktoken (l, TK_RIGHT_PAREN);
    case '{':
      return mktoken (l, TK_LEFT_BRACE);
    case '}':
      return mktoken (l, TK_RIGHT_BRACE);
    case ',':
      return mktoken (l, TK_COMMA);
    case '.':
      return mktoken (l, TK_DOT);
    case '-':
      return mktoken (l, TK_MINUS);
    case '+':
      return mktoken (l, TK_PLUS);
    case ';':
      return mktoken (l, TK_SEMICOLON);
    case '/':
      return mktoken (l, TK_SLASH);
    case '*':
      return mktoken (l, TK_STAR);

    // Single-character tokens.
    case '!':
      return mktoken (l, lexer_match (l, '=') ? TK_BANG_EQUAL : TK_BANG);
    case '=':
      return mktoken (l, lexer_match (l, '=') ? TK_EQUAL_EQUAL : TK_EQUAL);
    case '>':
      return mktoken (l,
                        lexer_match (l, '=') ? TK_GREATER_EQUAL : TK_GREATER);
    case '<':
      return mktoken (l, lexer_match (l, '=') ? TK_LESS_EQUAL : TK_LESS);
  }

  // string
  if (c == '"')
    return mktoken (l, lex_string (l));
  // number
  if (isdigit (c))
    return mktoken (l, lex_number (l));
  // identifier
  if (isalpha (c))
    return mktoken (l, lex_ident (l));
  // error
  lexer_error (l, "lex error");
  return mktoken (l, TK_ERR);
}

char *lex_error (Lexer *l)
{
  if (l->err)
  {
    return l->errmsg;
  }
  return 0;
}

TokenType token_type (Token *token) { return token->type; }

int token_line (Token *token) { return token->line; }

char *token_lexem (Token *token, char *dest)
{
  sprintf (dest, "%.*s", token->len, token->at);
  return dest;
}

char *token_lexem_start (Token *token) { return token->at; }

int token_lexem_len (Token *token) { return token->len; }
