#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "tg_parser.h"
#include "tg_darray.h"
#include "tg_dstring.h"
#include "tg_common.h"

#define TG_C_EOF -1

struct tg_char {
	int c;
	char error[TG_MSGMAXSIZE];
	int line;
	int pos;
};

const char *tg_strsym[] = {
	"identificator", "integer", "float", "quoted string",
	"relational operator", "(", ")", "&", "|", "~", ",", "!",
	"increment or decrement operator", ";", ":", "cell reference",
	"%", "addition or substraction operator",
	"multiplication or division operator", "..",
	"assignment operator", "?", "next to operator", "empty",
	"global", "function", "for", "if", "else", "continue", "break",
	"return", "in", "columns", ".", "{", "}", "[", "]",
	"end of file", "error", "", "template", "function definition",
	"function definition arguments", "statement", "return", "if",
	"for", "for (expression)", "for (classic)", "statement body",
	"block", "expression", "assignment", "ternary operator",
	"logical or", "logical and", "relation", "next to",
	"next to options", "string concationation",
	"addtion or substraction", "multiplication or division",
	"unary operator", "reference", "logical not", "sign",
	"preincrement or predecrement", "id usage", "value",
	"identificator", "constant", "array access",
	"array access expression", "filter", "range",
	"function arguments", "object attribute access"
};

static FILE *_tg_file;
static char *_tg_lbuf, *_tg_lbufpos;
static size_t _tg_lbufsize;
static int _tg_curline;

struct tg_darray tg_text;
static int tg_curline;
static int tg_curpos;

static struct tg_char cbuf[3];
static struct tg_token tbuf[2];

static struct tg_darray nodes;

// string stream to char stream
static struct tg_char _tg_getc()
{
	struct tg_char c;

	while (1) {
		ssize_t r;
		char *t;

		if (_tg_lbufpos[0] == '#')
			_tg_lbufpos = "";

		if (_tg_lbufpos[0] != '\0')
			break;
	
		if ((r = getline(&_tg_lbuf, &_tg_lbufsize,
				_tg_file)) < 0) {
			if (!feof(_tg_file)) {
				TG_ERROR("Cannot read line: %s.",
					strerror(errno));
			}

			c.c = TG_C_EOF;
			c.pos = _tg_lbufpos - _tg_lbuf;
			c.line = _tg_curline;
				
			return c;
		}

		_tg_lbufpos = _tg_lbuf;

		t = tg_strdup(_tg_lbufpos);
		if (t[r - 1] == '\n')
			t[r - 1] = '\0';

		tg_darrpush(&tg_text, &t);

		_tg_curline++;
	}
	
	c.pos = _tg_lbufpos - _tg_lbuf;
	c.line = _tg_curline;
	c.c = (*_tg_lbufpos++);
	
	return c;
}

static int tg_getc()
{
	cbuf[0] = cbuf[1];
	cbuf[1] = cbuf[2];

	if (cbuf[0].c != TG_C_EOF)
		cbuf[2] = _tg_getc();

	tg_curline = cbuf[0].line;
	tg_curpos = cbuf[0].pos;
	
	return cbuf[0].c;
}

static int tg_peekc()
{
	return cbuf[1].c;
}

static int tg_peekc2()
{
	return cbuf[2].c;
}

// lexical analyzer
static void tg_createtoken(struct tg_token *t, const char *val,
	enum TG_TYPE type, int line, int pos)
{
	tg_dstrcreate(&(t->val), val);

	t->type = type;
	t->line = line;
	t->pos = pos;
}

static void tg_nexttoken(struct tg_token *t)
{
	enum TG_TYPE type;
	const char *v;
	int c;

	while (isspace(tg_peekc()))
		tg_getc();

	c = tg_getc();
	
	if (c == TG_C_EOF) {
		tg_createtoken(t, "", TG_T_EOF, tg_curline, tg_curpos);
		return;
	}

	if (c == '\"') {
		tg_dstrcreate(&(t->val), "");
	
		t->type = TG_T_STRING;
		t->line = tg_curline;
		t->pos = tg_curpos;
		
		c = tg_getc();
		while (c != '\"') {
			if (c == '\\') {
				c = tg_getc();
				
				if (c == TG_C_EOF) {
					TG_SETERROR("Unexpected EOF.");
					tg_dstrdestroy(&(t->val));
					goto error;
				}
				else if (c == 'n')	c = '\n';
				else if (c == 'r')	c = '\r';
				else if (c == 't')	c = '\t';
				else if (c == '0')	c = '\0';
			}
			else if (c == TG_C_EOF) {
				TG_SETERROR("Unexpected EOF.");
				tg_dstrdestroy(&(t->val));
				goto error;
			}
			
			tg_dstraddstrn(&(t->val), (char *) &c, 1);

			c = tg_getc();
		}

		return;
	}
	
	if (isalpha(c) || c == '_') {
	 	t->line = tg_curline;
	 	t->pos = tg_curpos;
		
		tg_dstrcreaten(&(t->val), (char *) &c, 1);

		while (1) {
			c = tg_peekc();
			
			if (!isalpha(c) && !isdigit(c) && c != '_')
				break;
		
			tg_dstraddstrn(&(t->val), (char *) &c, 1);

			c = tg_getc();
		}
		if (strcmp(t->val.str, "empty") == 0)
			t->type = TG_T_EMPTY;
		else if (strcmp(t->val.str, "global") == 0)
			t->type = TG_T_GLOBAL;
		else if (strcmp(t->val.str, "function") == 0)
			t->type = TG_T_FUNCTION;
		else if (strcmp(t->val.str, "for") == 0)
			t->type = TG_T_FOR;
		else if (strcmp(t->val.str, "if") == 0)
			t->type = TG_T_IF;
		else if (strcmp(t->val.str, "else") == 0)
			t->type = TG_T_ELSE;
		else if (strcmp(t->val.str, "continue") == 0)
			t->type = TG_T_CONTINUE;
		else if (strcmp(t->val.str, "break") == 0)
			t->type = TG_T_BREAK;
		else if (strcmp(t->val.str, "return") == 0)
			t->type = TG_T_RETURN;
		else if (strcmp(t->val.str, "in") == 0)
			t->type = TG_T_IN;
		else if (strcmp(t->val.str, "columns") == 0)
			t->type = TG_T_COLUMNS;
		else
	 		t->type = TG_T_ID;

		return;
	}

	if (isdigit(c)) {
		int p, e;

		tg_dstrcreaten(&(t->val), (char *) &c, 1);

	 	t->type = TG_T_INT;
	 	t->line = tg_curline;
	 	t->pos = tg_curpos;

		p = e = 0;
		while (1) {
			while (1) {
				c = tg_peekc();

				if (!isdigit(c))
					break;

				tg_dstraddstrn(&(t->val),
					(char *) &c, 1);

				c = tg_getc();
			}

			if ((c == '.' && isdigit(tg_peekc2()))
					|| c == 'e' || c == 'E') {
	 			t->type = TG_T_FLOAT;
				
				if (c == '.') {
					if (e || p) {
						TG_SETERROR("Wrong decimal format.");
						tg_dstrdestroy(&(t->val));
						goto error;
					}

					p = 1;
				}
				if (c == 'e' || c == 'E') {
					if (e) {
						TG_SETERROR("Wrong decimal format.");
						tg_dstrdestroy(&(t->val));
						goto error;
					}
					
					e = 1;
				}

				c = tg_getc();

				tg_dstraddstrn(&(t->val),
					(char *) &c, 1);
			}
			else
				break;
		}

		return;
	}
	
	if (c == '=') {
		c = tg_peekc();
		if (c == '=') {
			v = "==";
			type = TG_T_RELOP;

			tg_getc();
		}
		else {
			v = "=";
			type = TG_T_ASSIGNOP;
		}
	}
	else if (c == '!') {
		if (tg_peekc() == '=') {
			v = "!=";
			type = TG_T_RELOP;

			tg_getc();
		}
		else {
			v = "!";
			type = TG_T_NOT;
		}
	}
	else if (c == '>') {
		if (tg_peekc() == '=') {
			v = ">=";
			type = TG_T_RELOP;

			tg_getc();
		}
		else {
			v = ">";
			type = TG_T_RELOP;
		}
	}
	else if (c == '<') {
		c = tg_peekc();
		if (c == '=') {
			v = "<=";
			type = TG_T_RELOP;
		
			tg_getc();
		}
		else if (c == '-') {
			v = "<-";
			type = TG_T_NEXTTOOP;
		
			tg_getc();
		}
		else {
			v = "<";
			type = TG_T_RELOP;
		}
	}
	else if (c == '^') {
		v = "^";
		type = TG_T_NEXTTOOP;
	}
	else if (c == '(') {
		v = "(";
		type = TG_T_LPAR;
	}
	else if (c == ')') {
		v = ")";
		type = TG_T_RPAR;
	}
	else if (c == '&') {
		v = "&";
		type = TG_T_AND;
	}
	else if (c == '|') {
		v = "|";
		type = TG_T_OR;
	}
	else if (c == '$') {
		v = "$";
		type = TG_T_REFOP;
	}
	else if (c == '%') {
		v = "%";
		type = TG_T_PERCENT;
	}
	else if (c == '~') {
		if (tg_peekc() == '=') {
			v = "~=";
			type = TG_T_ASSIGNOP;

			tg_getc();
		}
		else {
			v = "~";
			type = TG_T_TILDA;
		}
	}
	else if (c == '.') {
		if (tg_peekc() == '.') {
			v = "..";
			type = TG_T_DDOT;

			tg_getc();
		}
		else {
			v = ".";
			type = TG_T_DOT;
		}
	}
	else if (c == ',') {
		v = ",";
		type = TG_T_COMMA;
	}
	else if (c == ':') {
		v = ":";
		type = TG_T_COLON;
	}
	else if (c == ';') {
		v = ";";
		type = TG_T_SEMICOL;
	}
	else if (c == '*') {
		if (tg_peekc() == '=') {
			v = "*=";
			type = TG_T_ASSIGNOP;

			tg_getc();
		}
		else {
			v = "*";
			type = TG_T_MULTOP;
		}
	}
	else if (c == '/') {
		if (tg_peekc() == '=') {
			v = "/=";
			type = TG_T_ASSIGNOP;

			tg_getc();
		}
		else {
			v = "/";
			type = TG_T_MULTOP;
		}
	}
	else if (c == '-') {
		c = tg_peekc();
		if (c == '-') {
			v = "--";
			type = TG_T_STEPOP;
		
			tg_getc();
		}
		else if (c == '=') {
			v = "-=";
			type = TG_T_ASSIGNOP;

			tg_getc();
		}
		else {
			v = "-";
			type = TG_T_ADDOP;
		}
	}
	else if (c == '+') {
		c = tg_peekc();
		if (c == '+') {
			v = "++";
			type = TG_T_STEPOP;

			tg_getc();
		}
		else if (c == '=') {
			v = "+=";
			type = TG_T_ASSIGNOP;

			tg_getc();
		}
		else {
			v = "+";
			type = TG_T_ADDOP;
		}
	}
	else if (c == '{') {
		v = "{";
		type = TG_T_LBRC;
	}
	else if (c == '}') {
		v = "}";
		type = TG_T_RBRC;
	}
	else if (c == '[') {
		v = "[";
		type = TG_T_LBRK;
	}
	else if (c == ']') {
		v = "]";
		type = TG_T_RBRK;
	}
	else if (c == '?') {
		v = "?";
		type = TG_T_QUEST;
	}
	else {
		TG_SETERROR("Unknown token.");
		goto error;
	}

	tg_createtoken(t, v, type, tg_curline, tg_curpos);
	
	return;

error:
	tg_createtoken(t, tg_error, TG_T_ERROR, tg_curline, tg_curpos);

	return;
}

static int tg_gettoken(struct tg_token *t)
{
	if (tbuf[0].type != TG_T_EOF && tbuf[0].type != TG_T_ERROR) {
		tbuf[0] = tbuf[1];
		tg_nexttoken(tbuf + 1);
	}

	if (t != NULL)
		*t = tbuf[0];

	return tbuf[0].type;
}

static int tg_peektoken(struct tg_token *t)
{
	if (tbuf[0].type == TG_T_EOF || tbuf[0].type == TG_T_ERROR)
		return tg_gettoken(t);

	if (t != NULL)
		*t = tbuf[1];
	
	return tbuf[1].type;
}

// node API
static int tg_nodeadd(int pi, enum TG_TYPE type, struct tg_token *token)
{	
	struct tg_node n;
	struct tg_node *parent;
	int ni;

	n.type = type;
	
	if (token != NULL)
		n.token = *token;

	tg_darrinit(&(n.children), sizeof(int));

	n.parent = pi;

	ni = tg_darrpush(&nodes, &n);
	
	if (pi < 0)
		return ni;

	TG_ASSERT((parent = tg_darrget(&nodes, pi)) != NULL,
		"Node %d does not exist", pi);

	tg_darrpush(&(parent->children), &ni);

	return ni;
}

static void tg_nodeattach(int pi, int ni)
{
	struct tg_node *node;
	struct tg_node *parent;

	TG_ASSERT((parent = tg_darrget(&nodes, pi)) != NULL,
		"Node %d does not exist", pi);

	TG_ASSERT((node = tg_darrget(&nodes, ni)) != NULL,
		"Node %d does not exist", ni);

	node->parent = pi;

	tg_darrpush(&(parent->children), &ni);
}

int tg_nodeccnt(int ni)
{
	struct tg_node *p;
	
	TG_ASSERT((p = tg_darrget(&nodes, ni)) != NULL,
		"Node %d does not exist", ni);

	return p->children.cnt;

}

int tg_nodechild(int ni, int i)
{
	struct tg_node *p;
	int *pci;
	
	TG_ASSERT((p = tg_darrget(&nodes, ni)) != NULL,
		"Node %d does not exist", ni);

	TG_ASSERT((pci = tg_darrget(&(p->children), i)) != NULL,
		"Child %d of Node %d does not exist", i, ni);

	return *pci;
}

enum TG_TYPE tg_nodetype(int ni)
{
	return ((struct tg_node *) tg_darrget(&nodes, ni))->type;
}

struct tg_token *tg_nodetoken(int ni)
{
	return &(((struct tg_node *) tg_darrget(&nodes, ni))->token);
}

int tg_indent(int t)
{
	int i;

	for (i = 0; i < t; ++i)
		printf("|   ");

	return 0;
}

int tg_printnode(int ni, int depth)
{
	int i;
	struct tg_node *n;

	n = tg_darrget(&nodes, ni);

	tg_indent(depth);
	printf("node {\n");
	tg_indent(depth + 1);
	printf("type: %s\n", tg_strsym[n->type]);

	// differ terminal and non-terminal
	if (n->type < TG_T_END) {
		tg_indent(depth + 1);
		printf("token {\n");
		tg_indent(depth + 2);
		printf("value: %s\n", n->token.val.str);
		tg_indent(depth + 2);
		printf("type: %s\n", tg_strsym[n->token.type]);
		tg_indent(depth + 2);
		printf("line: %d\n", n->token.line);
		tg_indent(depth + 2);
		printf("pos: %d\n", n->token.pos);
		tg_indent(depth + 1);
		printf("}\n");
	}

	tg_indent(depth + 1);
	printf("children count: %ld\n", n->children.cnt);
	if (n->children.cnt != 0) {
		tg_indent(depth + 1);
		printf("children {\n");
		for (i = 0; i < n->children.cnt; ++i) {
			int ni;

			ni = *((int *) tg_darrget(&(n->children), i));
		
			tg_printnode(ni, depth + 2);	
		}
		tg_indent(depth + 1);
		printf("}\n");
	}

	tg_indent(depth);
	printf("}\n");

	return 0;
}

// syntax analizer

static int tg_expr(int ni);
static int tg_assign(int ni);
static int tg_stmt(int ni);

static int tg_setparsererror(struct tg_token *t, const char *err)
{
	int c;
	int i;

	c = snprintf(tg_error, TG_MSGMAXSIZE,
		"%d:%d:\n", t->line, t->pos);
	
	c += snprintf(tg_error + c, TG_MSGMAXSIZE,
		"%s\n", *((char **) tg_darrget(&tg_text, t->line - 1)));

	for (i = 0; i < t->pos; ++i)
		c += snprintf(tg_error + c, TG_MSGMAXSIZE, " ");
	c += snprintf(tg_error + c, TG_MSGMAXSIZE, "^\n");

	for (i = 0; i < t->pos; ++i)
		c += snprintf(tg_error + c, TG_MSGMAXSIZE, " ");
	c += snprintf(tg_error + c, TG_MSGMAXSIZE, "%s\n", err);

	return 0;
}

static int tg_gettokentype(struct tg_token *t, enum TG_TYPE type)
{
	tg_gettoken(t);

	if (t->type != type) {
		char strerr[1024];
		char *e;

		if (t->type == TG_T_ERROR)
			e = t->val.str;
		else {
			snprintf(strerr, 4096,
				"Wrong token type: expected %s, got %s.",
				tg_strsym[type], tg_strsym[t->type]);

			e = strerr;
		}

		tg_setparsererror(t, e);

		return (-1);
	}

	return t->type;
}

static int tg_args(int ni)
{
	struct tg_token t;
	int ani;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	if (tg_peektoken(&t) == TG_T_RPAR) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));
		return 0;
	}

	ani = tg_nodeadd(-1, TG_N_ASSIGN, NULL);
	TG_ERRQUIT(tg_assign(ani));
		
	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);
	
	tg_nodeattach(ni, ani);

	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));
	
		ani = tg_nodeadd(-1, TG_N_ASSIGN, NULL);
		TG_ERRQUIT(tg_assign(ani));

		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodechild(ani, 0);
		
		tg_nodeattach(ni, ani);
	}

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	return 0;
}

static int tg_id(int ni)
{
	struct tg_token t;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));

	ni = tg_nodeadd(ni, TG_N_ID, NULL);

	tg_nodeadd(ni, TG_T_ID, &t);

	if (tg_peektoken(&t) == TG_T_LPAR) {
		int ani;

		ani = tg_nodeadd(ni, TG_N_ARGS, NULL);
		TG_ERRQUIT(tg_args(ani));
	}

	return 0;
}

static int tg_val(int ni)
{
	struct tg_token t;
	char strerr[1024];

	tg_peektoken(&t);
	
	switch (t.type) {
	case TG_T_ID:
		TG_ERRQUIT(tg_id(ni));
		break;

	case TG_T_LPAR:
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

		ni = tg_nodeadd(ni, TG_N_EXPR, NULL);

		TG_ERRQUIT(tg_expr(ni));

		TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));
		break;

	case TG_T_FLOAT:
	case TG_T_INT:
	case TG_T_STRING:
	case TG_T_EMPTY:
		TG_ERRQUIT(tg_gettoken(&t));
	
		ni = tg_nodeadd(ni, TG_N_CONST, NULL);

		tg_nodeadd(ni, t.type, &t);
		break;

	case TG_T_ERROR:
		tg_setparsererror(&t, t.val.str);
		return (-1);

	default:
		snprintf(strerr, 4096,
			"Wrong token type: expected value, got %s.",
			tg_strsym[t.type]);

		tg_setparsererror(&t, strerr);
		return (-1);
	}

	return 0;
}

static int tg_filter(int ni)
{
	struct tg_token t;
	int ani;

	if (tg_peektoken(&t) == TG_T_LBRC) {
		int fni;

		TG_ERRQUIT(tg_gettokentype(&t, TG_T_LBRC));
		
		fni = tg_nodeadd(ni, TG_N_FILTER, NULL);

		ani = tg_nodeadd(-1, TG_N_ASSIGN, NULL);
		TG_ERRQUIT(tg_assign(ani));
			
		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodechild(ani, 0);
		
		tg_nodeattach(fni, ani);

		TG_ERRQUIT(tg_gettokentype(&t, TG_T_RBRC));
	
		return 0;
	}

	ani = tg_nodeadd(-1, TG_N_ASSIGN, NULL);
	TG_ERRQUIT(tg_assign(ani));
		
	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);
	
	if (tg_peektoken(&t) == TG_T_DDOT) {
		int rni;

		TG_ERRQUIT(tg_gettokentype(&t, TG_T_DDOT));

		rni = tg_nodeadd(ni, TG_N_RANGE, NULL);
		tg_nodeattach(rni, ani);

		ani = tg_nodeadd(-1, TG_N_ASSIGN, NULL);
		TG_ERRQUIT(tg_assign(ani));
			
		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodechild(ani, 0);
		
		tg_nodeattach(rni, ani);
	}
	else
		tg_nodeattach(ni, ani);

	return 0;
}

static int tg_indexexpr(int ni)
{
	struct tg_token t;

	TG_ERRQUIT(tg_filter(ni));

	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));
		TG_ERRQUIT(tg_filter(ni));
	}

	return 0;
}

static int tg_index(int ni)
{
	struct tg_token t;
	int ini;

	if (tg_peektoken(&t) == TG_T_PERCENT) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_PERCENT));
		tg_nodeadd(ni, TG_T_PERCENT, &t);
	}

	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LBRK));

	ini = tg_nodeadd(ni, TG_N_INDEXEXPR, NULL);
	TG_ERRQUIT(tg_indexexpr(ini));

	if (tg_peektoken(&t) == TG_T_SEMICOL) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));

		ini = tg_nodeadd(ni, TG_N_INDEXEXPR, NULL);
		TG_ERRQUIT(tg_indexexpr(ini));
	}
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RBRK));

	return 0;
}

static int tg_attr(int ni)
{
	struct tg_token t;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_DOT));
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));
	
	tg_nodeadd(ni, TG_T_ID, &t);

	return 0;
}

static int tg_address(int ni)
{
	struct tg_token t;
	enum TG_TYPE type;
	char strerr[1024];
	int ani, ini;

	TG_ERRQUIT(tg_val(ni));

	while ((type = tg_peektoken(&t)) == TG_T_LPAR
		|| type == TG_T_PERCENT	|| type == TG_T_LBRK
		|| type == TG_T_DOT) {
		switch (type) {
		case TG_T_LBRK:
		case TG_T_PERCENT:
			ini = tg_nodeadd(ni, TG_N_INDEX, NULL);
			TG_ERRQUIT(tg_index(ini));
			break;

		case TG_T_DOT:
			ani = tg_nodeadd(ni, TG_N_ATTR, NULL);
			TG_ERRQUIT(tg_attr(ani));
			break;

		default:
			snprintf(strerr, 4096,
				"Wrong token type: expected '(', '[' or '.', got %s.",
				tg_strsym[t.type]);

			tg_setparsererror(&t, strerr);
			break;
		}
	}

	return 0;
}

static int tg_prestep(int ni)
{
	struct tg_token t;
	int ani;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_STEPOP));
	tg_nodeadd(ni, TG_T_STEPOP, &t);

	ani = tg_nodeadd(-1, TG_N_ADDRESS, NULL);
	TG_ERRQUIT(tg_address(ani));

	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);

	tg_nodeattach(ni, ani);

	return 0;
}

static int tg_ref(int ni)
{
	struct tg_token t;
	int ani;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_REFOP));

	ani = tg_nodeadd(-1, TG_N_ADDRESS, NULL);
	TG_ERRQUIT(tg_address(ani));

	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);

	tg_nodeattach(ni, ani);

	return 0;
}

static int tg_not(int ni)
{
	struct tg_token t;
	int ani;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_NOT));

	ani = tg_nodeadd(-1, TG_N_ADDRESS, NULL);
	TG_ERRQUIT(tg_address(ani));

	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);

	tg_nodeattach(ni, ani);

	return 0;
}

static int tg_sign(int ni)
{
	struct tg_token t;
	int ani;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ADDOP));
	tg_nodeadd(ni, TG_T_ADDOP, &t);

	ani = tg_nodeadd(-1, TG_N_ADDRESS, NULL);
	TG_ERRQUIT(tg_address(ani));

	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);

	tg_nodeattach(ni, ani);

	return 0;
}

static int tg_unary(int ni)
{
	struct tg_token t;
	int ani;

	tg_peektoken(&t);

	switch(t.type) {
	case TG_T_STEPOP:
		ni = tg_nodeadd(ni, TG_N_PRESTEP, NULL);
		TG_ERRQUIT(tg_prestep(ni));
		break;

	case TG_T_REFOP:
		ni = tg_nodeadd(ni, TG_N_REF, NULL);
		TG_ERRQUIT(tg_ref(ni));
		break;

	case TG_T_NOT:
		ni = tg_nodeadd(ni, TG_N_NOT, NULL);
		TG_ERRQUIT(tg_not(ni));
		break;

	case TG_T_ADDOP:
		ni = tg_nodeadd(ni, TG_N_SIGN, NULL);
		TG_ERRQUIT(tg_sign(ni));
		break;

	default:
		ani = tg_nodeadd(-1, TG_N_ADDRESS, NULL);
		TG_ERRQUIT(tg_address(ani));

		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodechild(ani, 0);

		tg_nodeattach(ni, ani);
	}

	return 0;
}

static int tg_mult(int ni)
{
	struct tg_token t;

	TG_ERRQUIT(tg_unary(ni));

	while (tg_peektoken(&t) == TG_T_MULTOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_MULTOP));
	
		tg_nodeadd(ni, TG_T_MULTOP, &t);

		TG_ERRQUIT(tg_unary(ni));
	}

	return 0;
}

static int tg_add(int ni)
{
	struct tg_token t;
	int mni;

	mni = tg_nodeadd(-1, TG_N_MULT, NULL);
	TG_ERRQUIT(tg_mult(mni));

	if (tg_nodeccnt(mni) == 1)
		mni = tg_nodechild(mni, 0);

	tg_nodeattach(ni, mni);

	while (tg_peektoken(&t) == TG_T_ADDOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ADDOP));

		tg_nodeadd(ni, TG_T_ADDOP, &t);

		mni = tg_nodeadd(-1, TG_N_MULT, NULL);
		TG_ERRQUIT(tg_mult(mni));
	
		if (tg_nodeccnt(mni) == 1)
			mni = tg_nodechild(mni, 0);
		
		tg_nodeattach(ni, mni);
	}

	return 0;
}

static int tg_cat(int ni)
{
	struct tg_token t;
	int ani;

	ani = tg_nodeadd(-1, TG_N_ADD, NULL);
	TG_ERRQUIT(tg_add(ani));

	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);
	
	tg_nodeattach(ni, ani);

	while (tg_peektoken(&t) == TG_T_TILDA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_TILDA));
	
		ani = tg_nodeadd(-1, TG_N_ADD, NULL);
		TG_ERRQUIT(tg_add(ani));
	
		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodechild(ani, 0);
		
		tg_nodeattach(ni, ani);
	}

	return 0;
}

static int tg_nexttoopts(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_COLON));
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_STRING));
		
	tg_nodeadd(ni, TG_T_STRING, &t);

	return 0;
}

static int tg_nextto(int ni)
{
	struct tg_token t;
	int cni;

	cni = tg_nodeadd(-1, TG_N_CAT, NULL);
	TG_ERRQUIT(tg_cat(cni));

	if (tg_nodeccnt(cni) == 1)
		cni = tg_nodechild(cni, 0);
	
	tg_nodeattach(ni, cni);

	while (tg_peektoken(&t) == TG_T_NEXTTOOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_NEXTTOOP));
	
		tg_nodeadd(ni, TG_T_NEXTTOOP, &t);

		cni = tg_nodeadd(-1, TG_N_CAT, NULL);
		TG_ERRQUIT(tg_cat(cni));

		if (tg_nodeccnt(cni) == 1)
			cni = tg_nodechild(cni, 0);
		
		tg_nodeattach(ni, cni);

		if (tg_peektoken(&t) == TG_T_COLON) {
			int oni;

			oni = tg_nodeadd(-1, TG_N_NEXTTOOPTS, NULL);

			TG_ERRQUIT(tg_nexttoopts(oni));
	
			tg_nodeattach(ni, oni);
		}
	}

	return 0;
}

static int tg_rel(int ni)
{
	struct tg_token t;
	int nni;

	nni = tg_nodeadd(-1, TG_N_NEXTTO, NULL);
	TG_ERRQUIT(tg_nextto(nni));
	
	if (tg_nodeccnt(nni) == 1)
		nni = tg_nodechild(nni, 0);
	
	tg_nodeattach(ni, nni);

	if (tg_peektoken(&t) == TG_T_RELOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_RELOP));
	
		tg_nodeadd(ni, TG_T_RELOP, &t);

		nni = tg_nodeadd(-1, TG_N_NEXTTO, NULL);
		TG_ERRQUIT(tg_nextto(nni));

		if (tg_nodeccnt(nni) == 1)
			nni = tg_nodechild(nni, 0);
		
		tg_nodeattach(ni, nni);
	}

	return 0;
}

static int tg_and(int ni)
{
	struct tg_token t;
	int rni;

	rni = tg_nodeadd(-1, TG_N_REL, NULL);
	TG_ERRQUIT(tg_rel(rni));
	
	if (tg_nodeccnt(rni) == 1)
		rni = tg_nodechild(rni, 0);
	
	tg_nodeattach(ni, rni);

	while (tg_peektoken(&t) == TG_T_AND) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_AND));
	
		rni = tg_nodeadd(-1, TG_N_REL, NULL);
		TG_ERRQUIT(tg_rel(rni));

		if (tg_nodeccnt(rni) == 1)
			rni = tg_nodechild(rni, 0);
		
		tg_nodeattach(ni, rni);
	}

	return 0;
}

static int tg_or(int ni)
{
	struct tg_token t;
	int ani;

	ani = tg_nodeadd(-1, TG_N_AND, NULL);
	TG_ERRQUIT(tg_and(ani));
	
	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);
	
	tg_nodeattach(ni, ani);

	while (tg_peektoken(&t) == TG_T_OR) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_OR));
	
		ani = tg_nodeadd(-1, TG_N_AND, NULL);
		TG_ERRQUIT(tg_and(ani));

		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodechild(ani, 0);
		
		tg_nodeattach(ni, ani);
	}

	return 0;
}

static int tg_ternary(int ni)
{
	struct tg_token t;
	int oni;

	oni = tg_nodeadd(-1, TG_N_OR, NULL);
	TG_ERRQUIT(tg_or(oni));

	if (tg_nodeccnt(oni) == 1)
		oni = tg_nodechild(oni, 0);
	
	tg_nodeattach(ni, oni);

	while (tg_peektoken(&t) == TG_T_QUEST) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_QUEST));
	
		oni = tg_nodeadd(-1, TG_N_OR, NULL);
		TG_ERRQUIT(tg_or(oni));

		if (tg_nodeccnt(oni) == 1)
			oni = tg_nodechild(oni, 0);
		
		tg_nodeattach(ni, oni);

		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COLON));

		oni = tg_nodeadd(-1, TG_N_OR, NULL);
		TG_ERRQUIT(tg_or(oni));

		if (tg_nodeccnt(oni) == 1)
			oni = tg_nodechild(oni, 0);
		
		tg_nodeattach(ni, oni);
	}

	return 0;
}

static int tg_assign(int ni)
{
	struct tg_token t;
	int tni;

	tni = tg_nodeadd(-1, TG_N_TERNARY, NULL);
	TG_ERRQUIT(tg_ternary(tni));
	
	if (tg_nodeccnt(tni) == 1)
		tni = tg_nodechild(tni, 0);
	
	tg_nodeattach(ni, tni);

	while (tg_peektoken(&t) == TG_T_ASSIGNOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ASSIGNOP));
		
		tg_nodeadd(ni, TG_T_ASSIGNOP, &t);
		
		tni = tg_nodeadd(-1, TG_N_TERNARY, NULL);
		TG_ERRQUIT(tg_ternary(tni));
	
		if (tg_nodeccnt(tni) == 1)
			tni = tg_nodechild(tni, 0);
		
		tg_nodeattach(ni, tni);
	}

	return 0;
}

static int tg_expr(int ni)
{
	struct tg_token t;
	int ani;

	ani = tg_nodeadd(-1, TG_N_ASSIGN, NULL);
	TG_ERRQUIT(tg_assign(ani));
		
	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodechild(ani, 0);
	
	tg_nodeattach(ni, ani);

	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));
		
		ani = tg_nodeadd(-1, TG_N_ASSIGN, NULL);
		TG_ERRQUIT(tg_assign(ani));

		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodechild(ani, 0);
		
		tg_nodeattach(ni, ani);
	}

	return 0;
}

static int tg_block(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LBRC));

	while (tg_peektoken(&t) != TG_T_RBRC)
		TG_ERRQUIT(tg_stmt(ni));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RBRC));

	return 0;
}

static int tg_defargs(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));
	tg_nodeadd(ni, TG_T_ID, &t);
	
	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));
			
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));
		tg_nodeadd(ni, TG_T_ID, &t);
	}

	return 0;
}

static int tg_funcdef(int ni)
{
	struct tg_token t;
	int ani, bni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_FUNCTION));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));
	tg_nodeadd(ni, TG_T_ID, &t);

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	ani = tg_nodeadd(ni, TG_N_DEFARGS, NULL);
	TG_ERRQUIT(tg_defargs(ani));
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	bni = tg_nodeadd(ni, TG_N_BLOCK, NULL);
	TG_ERRQUIT(tg_block(bni));

	return 0;
}

static int tg_body(int ni)
{
	struct tg_token t;

	if (tg_peektoken(&t) == TG_T_LBRC)
		TG_ERRQUIT(tg_block(ni));
	else
		TG_ERRQUIT(tg_stmt(ni));

	return 0;
}

static int tg_forexpr(int ni)
{
	struct tg_token t;
	int eni;

	tg_peektoken(&t);

	if (t.type == TG_T_SEMICOL || t.type == TG_T_RPAR)
		return 0;

	eni = tg_nodeadd(-1, TG_N_EXPR, NULL);
	TG_ERRQUIT(tg_expr(eni));

	if (tg_nodeccnt(eni) == 1)
		eni = tg_nodechild(eni, 0);
	
	tg_nodeattach(ni, eni);

	return 0;
}

static int tg_fortable(int ni)
{
	struct tg_token t;
	int eni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_IN));

	eni = tg_nodeadd(ni, TG_N_EXPR, NULL);
	TG_ERRQUIT(tg_expr(eni));

	if (tg_peektoken(&t) == TG_T_COLUMNS) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COLUMNS));
		tg_nodeadd(ni, TG_T_COLUMNS, &t);
	}
	
	return 0;
}

static int tg_forclassic(int ni)
{
	struct tg_token t;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));

	TG_ERRQUIT(tg_forexpr(ni));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));

	TG_ERRQUIT(tg_forexpr(ni));

	return 0;
}

static int tg_for(int ni)
{
	struct tg_token t;
	int bni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_FOR));
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	TG_ERRQUIT(tg_forexpr(ni));

	if (tg_peektoken(&t) == TG_T_IN) {
		TG_ERRQUIT(tg_fortable(ni));
	}
	else
		TG_ERRQUIT(tg_forclassic(ni));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	bni = tg_nodeadd(ni, TG_N_BLOCK, NULL);
	TG_ERRQUIT(tg_body(bni));

	return 0;
}

static int tg_if(int ni)
{
	struct tg_token t;
	int eni, bni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_IF));
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	eni = tg_nodeadd(ni, TG_N_EXPR, NULL);
	TG_ERRQUIT(tg_expr(eni));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	bni = tg_nodeadd(ni, TG_N_BLOCK, NULL);
	TG_ERRQUIT(tg_body(bni));

	if (tg_peektoken(&t) == TG_T_ELSE) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ELSE));

		bni = tg_nodeadd(ni, TG_N_BLOCK, NULL);
		TG_ERRQUIT(tg_body(bni));
	}

	return 0;
}

static int tg_return(int ni)
{
	struct tg_token t;
	int eni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RETURN));

	if (tg_peektoken(&t) != TG_T_SEMICOL) {
		eni = tg_nodeadd(ni, TG_N_EXPR, NULL);
		TG_ERRQUIT(tg_expr(eni));
	}

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));

	return 0;
}

static int tg_stmt(int ni)
{
	struct tg_token t;
	int sni;

	switch (tg_peektoken(&t)) {
	case TG_T_FOR:
		sni = tg_nodeadd(ni, TG_N_FOR, NULL);
		TG_ERRQUIT(tg_for(sni));
		break;

	case TG_T_IF:
		sni = tg_nodeadd(ni, TG_N_IF, NULL);
		TG_ERRQUIT(tg_if(sni));
		break;

	case TG_T_RETURN:
		sni = tg_nodeadd(ni, TG_N_RETURN, NULL);
		TG_ERRQUIT(tg_return(sni));
		break;

	case TG_T_BREAK:
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_BREAK));
		tg_nodeadd(ni, TG_T_BREAK, &t);
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));
		break;

	case TG_T_CONTINUE:
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_CONTINUE));
		tg_nodeadd(ni, TG_T_CONTINUE, &t);
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));
		break;

	default:
		sni = tg_nodeadd(ni, TG_N_EXPR, NULL);
		TG_ERRQUIT(tg_expr(sni));
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));
		break;
	}

	return 0;
}

static int tg_template(int ni)
{
	struct tg_token t;
	int type;

	while ((type = tg_peektoken(&t)) != TG_T_EOF) {
		if (t.type == TG_T_FUNCTION) {
			int fni;

			fni = tg_nodeadd(ni, TG_N_FUNCDEF, NULL);
			TG_ERRQUIT(tg_funcdef(fni));
		}
		else {
			TG_ERRQUIT(tg_stmt(ni));
		}
	}

	return 0;
}

static int tg_initgetc(const char *path)
{
	tg_darrinit(&tg_text, sizeof(char *));

	if ((_tg_file = fopen(path, "r")) == NULL) {
		TG_SETERROR("Cannot open file %s: %s.",
			path, strerror(errno));
		return (-1);
	}

	_tg_curline = 0;
	_tg_lbufsize = 0;
	_tg_lbuf = NULL;
	_tg_lbufpos = "";

	cbuf[1] = _tg_getc();
	cbuf[2] = _tg_getc();

	return 0;
}

static int tg_initparser(const char *path)
{
	if (tg_initgetc(path) < 0)
		return (-1);

	tg_nexttoken(tbuf + 1);

	return 0;
}

static int tg_finilizeparser()
{
	int i;

	for (i = 0; i < tg_text.cnt; ++i)
		free(*((char **) tg_darrget(&tg_text, i)));
	
	tg_darrclear(&tg_text);

	if (_tg_lbuf != NULL)
		free(_tg_lbuf);

	return 0;
}

int tg_getparsetree(const char **pathv)
{
	const char **p;
	int ni;

	tg_darrinit(&nodes, sizeof(struct tg_node));

	if ((ni = tg_nodeadd(-1, TG_N_TEMPLATE, NULL)) < 0)
		return (-1);

	for (p = pathv; *p != NULL; ++p) {
		if (tg_initparser(*p) < 0)
			goto error;

		if (tg_template(ni) < 0)
			goto error;

		tg_finilizeparser();
	}

	return ni;

error:
	tg_finilizeparser();

	return (-1);
}
