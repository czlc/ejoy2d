/*
** $Id: llex.h,v 1.79 2016/05/02 14:02:12 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
** �ʷ���������������token���﷨������(lparser)ʹ��
*/

#ifndef llex_h
#define llex_h

#include "lobject.h"
#include "lzio.h"


#define FIRST_RESERVED	257


#if !defined(LUA_ENV)
#define LUA_ENV		"_ENV"
#endif


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_AND = FIRST_RESERVED, /* 257 */
  TK_BREAK,		/* 258 */
  
  TK_DO,		/* 259 */
  TK_ELSE,		/* 260 */
  TK_ELSEIF,	/* 261 */
  TK_END,		/* 262 */
  TK_FALSE,		/* 263 */
  TK_FOR,		/* 264 */
  TK_FUNCTION,	/* 265 */
  
  TK_GOTO,		/* 266 */
  TK_IF,		/* 267 */
  TK_IN,		/* 268 */
  TK_LOCAL,		/* 269 */
  TK_NIL,		/* 270 */
  TK_NOT,		/* 271 */
  TK_OR,		/* 272 */
  TK_REPEAT,	/* 273 */
  
  TK_RETURN,	/* 274 */
  TK_THEN,		/* 275 */
  TK_TRUE,		/* 276 */
  TK_UNTIL,		/* 277 */
  TK_WHILE,		/* 278 */

  /* other terminal symbols */
  TK_IDIV,		/* 279  //	*/
  TK_CONCAT,	/* 280 ..	*/
  TK_DOTS,		/* 281 ...	*/
  TK_EQ,		/* 282 ==	*/
  TK_GE,		/* 283 >=	*/
  TK_LE,		/* 284 <=	*/
  TK_NE,		/* 285 ~=	*/
  
  TK_SHL,		/* 286 << */
  TK_SHR,		/* 287 >> */
  
  TK_DBCOLON,	/* 288 :: */
  TK_EOS,		/* 289 */
  
  TK_FLT,		/* 290 ������ */
  TK_INT,		/* 291 ���� */
  TK_NAME,		/* 292 */
  TK_STRING		/* 293 */
};

/* number of reserved words */
#define NUM_RESERVED	(cast(int, TK_WHILE-FIRST_RESERVED+1))


typedef union {
  lua_Number r;
  lua_Integer i;
  TString *ts;
} SemInfo;  /* semantics(����) information */


typedef struct Token {
  int token; /* token id����RESERVED,Ҳ������ĳ���ַ� */
  SemInfo seminfo; /* token ���� */
} Token;


/* state of the lexer plus state of the parser when shared by all
   functions */
typedef struct LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token 'consumed' */
  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  struct FuncState *fs;  /* current function (parser), ʼ��ָ��ǰ���ڱ������ĺ�������open_func��ʱ���ָ��ǰ������close_func��ʱ����������ĸ����� */
  struct lua_State *L;
  ZIO *z;  /* input stream */
  Mbuffer *buff;  /* buffer for tokens */
  Table *h;  /* to avoid collection/reuse strings���ʷ������е��ַ���������Ҹ��ط��棬���ܻᱻ�������գ��﷨������ò���õ�����Ϊ�������lcode addk */
  struct Dyndata *dyd;  /* dynamic structures used by the parser */
  TString *source;  /* current source name */
  TString *envn;  /* environment variable name */
} LexState;


LUAI_FUNC void luaX_init (lua_State *L);
LUAI_FUNC void luaX_setinput (lua_State *L, LexState *ls, ZIO *z,
                              TString *source, int firstchar);
LUAI_FUNC TString *luaX_newstring (LexState *ls, const char *str, size_t l);
LUAI_FUNC void luaX_next (LexState *ls);
LUAI_FUNC int luaX_lookahead (LexState *ls);
LUAI_FUNC l_noret luaX_syntaxerror (LexState *ls, const char *s);
LUAI_FUNC const char *luaX_token2str (LexState *ls, int token);


#endif
