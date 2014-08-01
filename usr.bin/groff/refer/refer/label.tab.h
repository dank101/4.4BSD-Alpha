#define TOKEN_LETTER 257
#define TOKEN_LITERAL 258
#define TOKEN_DIGIT 259
typedef union {
  int num;
  expression *expr;
  struct { int ndigits; int val; } dig;
  struct { int start; int len; } str;
} YYSTYPE;
extern YYSTYPE yylval;
