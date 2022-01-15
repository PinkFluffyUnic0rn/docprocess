#ifndef TG_PARSER_H
#define TG_PARSER_H

#include "tg_darray.h"
#include "tg_dstring.h"

enum TG_TYPE {
	TG_T_ID,		TG_T_INT,	TG_T_FLOAT,
	TG_T_STRING,		TG_T_RELOP,	TG_T_LPAR,
	TG_T_RPAR,		TG_T_AND,	TG_T_OR,
	TG_T_TILDA,		TG_T_COMMA,	TG_T_NOT,
	TG_T_SEMICOL,		TG_T_COLON,	TG_T_DOL,
	TG_T_ADDOP,		TG_T_MULTOP,	TG_T_DDOT,
	TG_T_ASSIGN,		TG_T_QUEST,	TG_T_NEXTTOOP,
	TG_T_GLOBAL,		TG_T_FUNCTION,	TG_T_FOR,
	TG_T_IF,		TG_T_ELSE,	TG_T_CONTINUE,
	TG_T_BREAK,		TG_T_RETURN,	TG_T_IN,
	TG_T_DOT,		TG_T_LBRC,	TG_T_RBRC,
	TG_T_LBRK,		TG_T_RBRK,	TG_T_EOF,
	TG_T_ERROR,		TG_T_END,
	TG_N_TEMPLATE,		TG_N_FUNCDEF,	TG_N_DEFARGS,
	TG_N_STMT,		TG_N_RETURN,	TG_N_IF,
	TG_N_FOR,		TG_N_FOREXPR,	TG_N_FORCLASSIC,
	TG_N_BODY,		TG_N_BLOCK,	TG_N_EXPR,
	TG_N_ASSIGN,		TG_N_TERNARY,	TG_N_OR,
	TG_N_AND,		TG_N_REL,	TG_N_NEXTTO,
	TG_N_NEXTTOOPTS,	TG_N_CAT,	TG_N_ADD,
	TG_N_MULT,		TG_N_UNARY,	TG_N_REF,
	TG_N_NOT,		TG_N_SIGN,	TG_N_ADDRESS,
	TG_N_VAL,		TG_N_CONST,	TG_N_INDEX,
	TG_N_FILTER,		TG_N_RANGE,	TG_N_ARGS,
	TG_N_ATTR
};

extern const char *tg_tstrsym[];
extern const char *tg_nstrsym[];

struct tg_token {
	struct tg_dstring val;
	enum TG_TYPE type;
	int line;
	int pos;
};

struct tg_node {
	struct tg_token token;
	enum TG_TYPE type;
	struct tg_node *parent;
	struct tg_darray children;
	struct tg_darray siblings;
};

struct tg_node *tg_template(const char *p);

#endif
