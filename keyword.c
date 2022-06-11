#include "keyword.h"

int CloxKeyword(char *s, int len) { return __step_(s, len); }

int __step_(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 't':
    return __step_t(++s, --len);
  case 'v':
    return __step_v(++s, --len);
  case 'w':
    return __step_w(++s, --len);
  case 'c':
    return __step_c(++s, --len);
  case 'f':
    return __step_f(++s, --len);
  case 'p':
    return __step_p(++s, --len);
  case 'r':
    return __step_r(++s, --len);
  case 's':
    return __step_s(++s, --len);
  case 'a':
    return __step_a(++s, --len);
  case 'e':
    return __step_e(++s, --len);
  case 'i':
    return __step_i(++s, --len);
  case 'n':
    return __step_n(++s, --len);
  case 'o':
    return __step_o(++s, --len);
  }
  return -1;
}

int __step_a(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'n':
    return __step_an(++s, --len);
  }
  return -1;
}

int __step_an(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'd':
    return __step_and(++s, --len);
  }
  return -1;
}

int __step_and(char *s, int len)
{
  // KEYWORD -- and
  if (len == 0)
    return TK_AND;
  return -1;
}

int __step_e(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'l':
    return __step_el(++s, --len);
  }
  return -1;
}

int __step_el(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 's':
    return __step_els(++s, --len);
  }
  return -1;
}

int __step_els(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'e':
    return __step_else(++s, --len);
  }
  return -1;
}

int __step_else(char *s, int len)
{
  // KEYWORD -- else
  if (len == 0)
    return TK_ELSE;
  return -1;
}

int __step_i(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'f':
    return __step_if(++s, --len);
  }
  return -1;
}

int __step_if(char *s, int len)
{
  // KEYWORD -- if
  if (len == 0)
    return TK_IF;
  return -1;
}

int __step_n(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'i':
    return __step_ni(++s, --len);
  }
  return -1;
}

int __step_ni(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'l':
    return __step_nil(++s, --len);
  }
  return -1;
}

int __step_nil(char *s, int len)
{
  // KEYWORD -- nil
  if (len == 0)
    return TK_NIL;
  return -1;
}

int __step_o(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'r':
    return __step_or(++s, --len);
  }
  return -1;
}

int __step_or(char *s, int len)
{
  // KEYWORD -- or
  if (len == 0)
    return TK_OR;
  return -1;
}

int __step_w(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'h':
    return __step_wh(++s, --len);
  }
  return -1;
}

int __step_wh(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'i':
    return __step_whi(++s, --len);
  }
  return -1;
}

int __step_whi(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'l':
    return __step_whil(++s, --len);
  }
  return -1;
}

int __step_whil(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'e':
    return __step_while(++s, --len);
  }
  return -1;
}

int __step_while(char *s, int len)
{
  // KEYWORD -- while
  if (len == 0)
    return TK_WHILE;
  return -1;
}

int __step_c(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'l':
    return __step_cl(++s, --len);
  }
  return -1;
}

int __step_cl(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'a':
    return __step_cla(++s, --len);
  }
  return -1;
}

int __step_cla(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 's':
    return __step_clas(++s, --len);
  }
  return -1;
}

int __step_clas(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 's':
    return __step_class(++s, --len);
  }
  return -1;
}

int __step_class(char *s, int len)
{
  // KEYWORD -- class
  if (len == 0)
    return TK_CLASS;
  return -1;
}

int __step_f(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'a':
    return __step_fa(++s, --len);
  case 'o':
    return __step_fo(++s, --len);
  case 'u':
    return __step_fu(++s, --len);
  }
  return -1;
}

int __step_fa(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'l':
    return __step_fal(++s, --len);
  }
  return -1;
}

int __step_fal(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 's':
    return __step_fals(++s, --len);
  }
  return -1;
}

int __step_fals(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'e':
    return __step_false(++s, --len);
  }
  return -1;
}

int __step_false(char *s, int len)
{
  // KEYWORD -- false
  if (len == 0)
    return TK_FALSE;
  return -1;
}

int __step_fo(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'r':
    return __step_for(++s, --len);
  }
  return -1;
}

int __step_for(char *s, int len)
{
  // KEYWORD -- for
  if (len == 0)
    return TK_FOR;
  return -1;
}

int __step_fu(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'n':
    return __step_fun(++s, --len);
  }
  return -1;
}

int __step_fun(char *s, int len)
{
  // KEYWORD -- fun
  if (len == 0)
    return TK_FUN;
  return -1;
}

int __step_p(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'r':
    return __step_pr(++s, --len);
  }
  return -1;
}

int __step_pr(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'i':
    return __step_pri(++s, --len);
  }
  return -1;
}

int __step_pri(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'n':
    return __step_prin(++s, --len);
  }
  return -1;
}

int __step_prin(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 't':
    return __step_print(++s, --len);
  }
  return -1;
}

int __step_print(char *s, int len)
{
  // KEYWORD -- print
  if (len == 0)
    return TK_PRINT;
  return -1;
}

int __step_r(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'e':
    return __step_re(++s, --len);
  }
  return -1;
}

int __step_re(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 't':
    return __step_ret(++s, --len);
  }
  return -1;
}

int __step_ret(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'u':
    return __step_retu(++s, --len);
  }
  return -1;
}

int __step_retu(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'r':
    return __step_retur(++s, --len);
  }
  return -1;
}

int __step_retur(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'n':
    return __step_return(++s, --len);
  }
  return -1;
}

int __step_return(char *s, int len)
{
  // KEYWORD -- return
  if (len == 0)
    return TK_RETURN;
  return -1;
}

int __step_s(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'u':
    return __step_su(++s, --len);
  }
  return -1;
}

int __step_su(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'p':
    return __step_sup(++s, --len);
  }
  return -1;
}

int __step_sup(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'e':
    return __step_supe(++s, --len);
  }
  return -1;
}

int __step_supe(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'r':
    return __step_super(++s, --len);
  }
  return -1;
}

int __step_super(char *s, int len)
{
  // KEYWORD -- super
  if (len == 0)
    return TK_SUPER;
  return -1;
}

int __step_t(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'h':
    return __step_th(++s, --len);
  case 'r':
    return __step_tr(++s, --len);
  }
  return -1;
}

int __step_th(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'i':
    return __step_thi(++s, --len);
  }
  return -1;
}

int __step_thi(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 's':
    return __step_this(++s, --len);
  }
  return -1;
}

int __step_this(char *s, int len)
{
  // KEYWORD -- this
  if (len == 0)
    return TK_THIS;
  return -1;
}

int __step_tr(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'u':
    return __step_tru(++s, --len);
  }
  return -1;
}

int __step_tru(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'e':
    return __step_true(++s, --len);
  }
  return -1;
}

int __step_true(char *s, int len)
{
  // KEYWORD -- true
  if (len == 0)
    return TK_TRUE;
  return -1;
}

int __step_v(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'a':
    return __step_va(++s, --len);
  }
  return -1;
}

int __step_va(char *s, int len)
{
  if (len == 0)
    return -1;
  switch (*s) {
  case 'r':
    return __step_var(++s, --len);
  }
  return -1;
}

int __step_var(char *s, int len)
{
  // KEYWORD -- var
  if (len == 0)
    return TK_VAR;
  return -1;
}
