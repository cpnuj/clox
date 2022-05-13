#ifndef clox_keyword_h
#define clox_keyword_h

#define TK_ELSE 1024
#define TK_IF 1025
#define TK_OR 1026
#define TK_FALSE 1027
#define TK_WHILE 1028
#define TK_FOR 1029
#define TK_FUN 1030
#define TK_SUPER 1031
#define TK_TRUE 1032
#define TK_VAR 1033
#define TK_AND 1034
#define TK_CLASS 1035
#define TK_NIL 1036
#define TK_PRINT 1037
#define TK_RETURN 1038
#define TK_THIS 1039

int CloxKeyword(char* s, int len);
int __step_(char* s, int len);
int __step_a(char* s, int len);
int __step_an(char* s, int len);
int __step_and(char* s, int len);
int __step_e(char* s, int len);
int __step_el(char* s, int len);
int __step_els(char* s, int len);
int __step_else(char* s, int len);
int __step_i(char* s, int len);
int __step_if(char* s, int len);
int __step_n(char* s, int len);
int __step_ni(char* s, int len);
int __step_nil(char* s, int len);
int __step_o(char* s, int len);
int __step_or(char* s, int len);
int __step_w(char* s, int len);
int __step_wh(char* s, int len);
int __step_whi(char* s, int len);
int __step_whil(char* s, int len);
int __step_while(char* s, int len);
int __step_c(char* s, int len);
int __step_cl(char* s, int len);
int __step_cla(char* s, int len);
int __step_clas(char* s, int len);
int __step_class(char* s, int len);
int __step_f(char* s, int len);
int __step_fa(char* s, int len);
int __step_fal(char* s, int len);
int __step_fals(char* s, int len);
int __step_false(char* s, int len);
int __step_fo(char* s, int len);
int __step_for(char* s, int len);
int __step_fu(char* s, int len);
int __step_fun(char* s, int len);
int __step_p(char* s, int len);
int __step_pr(char* s, int len);
int __step_pri(char* s, int len);
int __step_prin(char* s, int len);
int __step_print(char* s, int len);
int __step_r(char* s, int len);
int __step_re(char* s, int len);
int __step_ret(char* s, int len);
int __step_retu(char* s, int len);
int __step_retur(char* s, int len);
int __step_return(char* s, int len);
int __step_s(char* s, int len);
int __step_su(char* s, int len);
int __step_sup(char* s, int len);
int __step_supe(char* s, int len);
int __step_super(char* s, int len);
int __step_t(char* s, int len);
int __step_th(char* s, int len);
int __step_thi(char* s, int len);
int __step_this(char* s, int len);
int __step_tr(char* s, int len);
int __step_tru(char* s, int len);
int __step_true(char* s, int len);
int __step_v(char* s, int len);
int __step_va(char* s, int len);
int __step_var(char* s, int len);

#endif
