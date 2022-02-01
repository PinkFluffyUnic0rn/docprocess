#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "tg_parser.h"
#include "tg_darray.h"
#include "tg_dstring.h"
#include "tg_common.h"

struct tg_char {
	int c;
	char error[TG_MSGMAXSIZE];
	int line;
	int pos;
};

const char *tg_strsym[] = {
	"identificator", "integer", "float", "quoted string",
	"relational operator", "(", ")", "&", "|", "~", ",", "!",
	"increment or decrement operator", ";", ":", "$",
	"addition or substraction operator",
	"multiplication or division operator", "..",
	"assignment operator", "?", "next to operator", "global",
	"function", "for", "if", "else", "continue", "break", "return",
	"in", "columns", ".", "{", "}", "[", "]", "end of file",
	"error", "", "template", "function definition",
	"function definition arguments", "statement", "return", "if",
	"for", "for (expression)", "for (classic)", "statement body",
	"block", "expression", "assignment", "ternary operator",
	"logical or", "logical and", "relation", "next to",
	"next to options", "string concationation",
	"addtion or substraction", "multiplication or division",
	"unary operator", "reference", "logical not", "sign",
	"preincrement or predecrement", "id usage", "value", "constant",
	"array access", "filter", "range", "function arguments",
	"object attribute access"
};

static const char *tg_path;
struct tg_darray tg_text;
static int tg_curline;
static int tg_curpos;

static struct tg_char cbuf[2];
static int getcinit = 0;

static struct tg_token tbuf[2];
static int gettinit = 0;

static struct tg_darray nodes;

static int tg_initparser(const char *path)
{
	if (tg_darrinit(&tg_text, sizeof(char *)) < 0)
		return (-1);

	tg_path = path;

	return 0;
}

static int tg_finilizeparser()
{
	tg_darrclear(&tg_text);

	return 0;
}

#define TG_C_ERROR -1
#define TG_C_EOF -2

// string stream to char stream
static struct tg_char _tg_getc(int raw)
{
	struct tg_char c;
	static FILE *f = NULL;
	static char *b, *l = NULL;
	static size_t lsz = 0;
	static int curline = 0;

	// need to be reworked: has problems with finilization
	if (tg_path != NULL) {
		if ((f = fopen(tg_path, "r")) == NULL) {
			TG_SETERROR("Cannot open file %s: %s.",
				tg_path, strerror(errno));
			goto error;
		}

		curline = 0;
		tg_path = NULL;

		lsz = 1;
		if ((b = l = malloc(lsz)) == NULL)
			goto error;
	
		l[0] = '\0';
	}
	
	while (1) {
		ssize_t r;
		char *t;

		if (!raw) {
			if (isspace(l[0])) { 
				while (isspace(l[1]))
					++l;

				l[0] = ' ';
			}
			
			if (l[0] == '#')
				l = "";
		}

		if (l[0] != '\0')
			break;
	
		if ((r = getline(&b, &lsz, f)) < 0) {
			if (feof(f)) {
				c.pos = l - b;
				c.line = curline;
				c.c = TG_C_EOF;
				return c;
			}

			TG_SETERROR("Cannot read line: %s.",
				strerror(errno));
			goto error;
		}

		l = b;

		// copy of all program text for error handling
		if ((t = malloc(r + 1)) == NULL)
			goto error;

		strcpy(t, l);

		if (t[r - 1] == '\n')
			t[r - 1] = '\0';

		if (tg_darrpush(&tg_text, &t) < 0)
			goto error;

		curline++;
	}
	
	c.pos = l - b;
	c.line = curline;
	c.c = (*l++);

	return c;

error:
	c.c = TG_C_ERROR;
	c.pos = l - b;
	c.line = curline;
	strncpy(c.error, tg_error, TG_MSGMAXSIZE);

	if (f != NULL) {
		fclose(f);
		f = NULL;
	}

	if (b != NULL) {
		free(b);
		b = NULL;
		lsz = 0;
	}
	
	return c;
}

static int tg_getc()
{
	struct tg_char c;

	if (getcinit == 0) {
		cbuf[1] = _tg_getc(0);
		getcinit = 1;
	}

	cbuf[0] = cbuf[1];
	c = cbuf[0];

	if (c.c == TG_C_ERROR || c.c == TG_C_EOF)
		goto error;
	
	cbuf[1] = _tg_getc(0);

error:
	tg_curline = c.line;
	tg_curpos = c.pos;
	strncpy(tg_error, c.error, TG_MSGMAXSIZE);
	
	return c.c;
}

static int tg_getcraw()
{
	struct tg_char c;

	if (getcinit == 0) {
		cbuf[1] = _tg_getc(1);
		getcinit = 1;
	}

	cbuf[0] = cbuf[1];
	c = cbuf[0];

	if (c.c == TG_C_ERROR || c.c == TG_C_EOF)
		goto error;
	
	cbuf[1] = _tg_getc(1);

error:
	tg_curline = c.line;
	tg_curpos = c.pos;
	strncpy(tg_error, c.error, TG_MSGMAXSIZE);
	
	return c.c;
}

static int tg_getcrestore()
{
	// do something with comments
	if (isspace(cbuf[1].c))
		cbuf[1].c = ' ';

	return 0;
}

static int tg_peekc()
{
	if (getcinit == 0) {
		cbuf[1] = _tg_getc(0);
		getcinit = 1;
	}
	
	return cbuf[1].c;
}

// lexical analyzer
static int tg_createtoken(struct tg_token *t, const char *val,
	enum TG_TYPE type, int line, int pos)
{
	if (tg_dstrcreate(&(t->val), val) < 0)
		return (-1);

	t->type = type;
	t->line = line;
	t->pos = pos;

	return 0;
}

static int tg_nexttoken(struct tg_token *t)
{
	static char tg_tokerror[TG_MSGMAXSIZE];
	enum TG_TYPE type;
	const char *v;
	int c;

	while ((tg_peekc()) == ' ')
		tg_getc();

	if ((c = tg_getc()) == TG_C_ERROR)
		goto error;

	if (c == TG_C_EOF) {
		if (tg_createtoken(t, "", TG_T_EOF,
				tg_curline, tg_curpos) < 0)
			goto error;

		return 0;
	}

	if (c == '\"') {
		if (tg_dstrcreate(&(t->val), "") < 0)
			goto error;
	
		t->type = TG_T_STRING;
		t->line = tg_curline;
		t->pos = tg_curpos;
		
		c = tg_getcraw();
		while (c != '\"') {
			if (c == '\\') {
				c = tg_getcraw();
				if (c == TG_C_ERROR)
					goto error;
				else if (c == TG_C_EOF) {
					TG_SETERROR("Unexpected EOF.");
					goto error;
				}
				else if (c == 'n')	c = '\n';
				else if (c == 'r')	c = '\r';
				else if (c == 't')	c = '\t';
				else if (c == '0')	c = '\0';
					
				if (c != '\n') {
					if (tg_dstraddstrn(&(t->val),
						(char *) &c, 1) < 0)
						goto error;
				}

				c = tg_getcraw();
			}
			else if (c == TG_C_EOF) {
				TG_SETERROR("Unexpected EOF.");
				goto error;
			}
			else if (c == TG_C_ERROR)
				goto error;
			
			if (tg_dstraddstrn(&(t->val),
				(char *) &c, 1) < 0)
				goto error;

			c = tg_getcraw();
		}

		tg_getcrestore();

		return 0;
	}
	
	if (isalpha(c) || c == '_') {
	 	t->line = tg_curline;
	 	t->pos = tg_curpos;
		
		if (tg_dstrcreaten(&(t->val), (char *) &c, 1) < 0)
			goto error;

		while (1) {
			c = tg_peekc();
			
			if (!isalpha(c) && !isdigit(c) && c != '_')
				break;
		
			if (tg_dstraddstrn(&(t->val), (char *) &c, 1) < 0)
				goto error;

			c = tg_getc();
		}

		if (strcmp(t->val.str, "global") == 0)
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

		return 0;
	}

	if (isdigit(c)) {
		int p, e;

		if (tg_dstrcreaten(&(t->val), (char *) &c, 1) < 0)
			goto error;

	 	t->type = TG_T_INT;
	 	t->line = tg_curline;
	 	t->pos = tg_curpos;

		p = e = 0;
		while (1) {
			while (1) {
				c = tg_peekc();

				if (!isdigit(c))
					break;

				if (tg_dstraddstrn(&(t->val),
					(char *) &c, 1) < 0)
					goto error;

				c = tg_getc();
			}

			if (c == '.' || c == 'e' || c == 'E') {
	 			t->type = TG_T_FLOAT;
				
				if (c == '.') {
					if (e || p) {
						TG_SETERROR("Wrong decimal format.");
						goto error;
					}

					p = 1;
				}
				if (c == 'e' || c == 'E') {
					if (e) {
						TG_SETERROR("Wrong decimal format.");
						goto error;
					}
					
					e = 1;
				}

				c = tg_getc();

				if (tg_dstraddstrn(&(t->val),
					(char *) &c, 1) < 0)
					goto error;
			}
			else
				break;
		}

		return 0;
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
		type = TG_T_DOL;
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

	if (tg_createtoken(t, v, type, tg_curline, tg_curpos) < 0)
		goto error;

	return 0;

error:
	strncpy(tg_tokerror, tg_error, TG_MSGMAXSIZE);
	tg_dstrcreatestatic(&(t->val), tg_tokerror);
	
	t->line = tg_curline;
	t->pos = tg_curpos;
	t->type = TG_T_ERROR;

	return 0;
}

static int tg_gettoken(struct tg_token *t)
{
	if (gettinit == 0) {
		tg_nexttoken(tbuf + 1);

		gettinit = 1;
	}

	if (tbuf[0].type == TG_T_EOF || tbuf[0].type == TG_T_ERROR) {
		*t = tbuf[0];
		return tbuf[0].type;
	}

	tbuf[0] = tbuf[1];
	
	if (t != NULL)
		*t = tbuf[0];

	tg_nexttoken(tbuf + 1);

	return tbuf[0].type;
}

int tg_peektoken(struct tg_token *t)
{
	if (gettinit == 0) {
		tg_nexttoken(tbuf + 1);
		gettinit = 1;
	}

	if (tbuf[0].type == TG_T_EOF || tbuf[0].type == TG_T_ERROR) {
		*t = tbuf[0];
		return tbuf[0].type;
	}

	if (t != NULL)
		*t = tbuf[1];
	
	return tbuf[1].type;
}

// node API
int tg_nodeadd(int pi, enum TG_TYPE type, struct tg_token *token)
{	
	struct tg_node n;
	struct tg_node *parent;
	int ni;

	n.type = type;
	
	if (token != NULL)
		n.token = *token;

	if (tg_darrinit(&(n.children), sizeof(int)) < 0)
		return (-1);

	n.parent = pi;

	if ((ni = tg_darrpush(&nodes, &n)) < 0)
		return (-1);
	
	if (pi < 0)
		return ni;

	if ((parent = tg_darrget(&nodes, pi)) == NULL)
		return (-1);

	if (tg_darrpush(&(parent->children), &ni) < 0)
		return (-1);

	return ni;
}

int tg_nodeattach(int pi, int ni)
{
	struct tg_node *node;
	struct tg_node *parent;

	if ((parent = tg_darrget(&nodes, pi)) == NULL)
		return (-1);
	
	if ((node = tg_darrget(&nodes, ni)) == NULL)
		return (-1);

	if ((node = tg_darrget(&nodes, ni)) == NULL)
		return (-1);

	node->parent = pi;

	if (tg_darrpush(&(parent->children), &ni) < 0)
		return (-1);

	return 0;
}

int tg_nodeccnt(int ni)
{
	struct tg_node *p;
	
	if ((p = tg_darrget(&nodes, ni)) == NULL)
		return (-1);

	return p->children.cnt;

}

int tg_nodegetchild(int ni, int i)
{
	struct tg_node *p;
	int *pci;
	
	if ((p = tg_darrget(&nodes, ni)) == NULL)
		return (-1);

	if ((pci = tg_darrget(&(p->children), i)) == NULL)
		return (-1);

	return *pci;
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

		//	printf("!@#!@#%s\n", n->token.val.str);
		}
		tg_indent(depth + 1);
		printf("}\n");
	}

	tg_indent(depth);
	printf("}\n");

	return 0;
}

// syntax analizer

#define TG_ERRQUIT(v) do { if ((v) < 0) return (-1); } while (0)

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

static int tg_id(int ni)
{
	struct tg_token t;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));

	TG_ERRQUIT(tg_nodeadd(ni, TG_T_ID, &t));

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

		TG_ERRQUIT(ni = tg_nodeadd(ni, TG_N_EXPR, NULL));

		TG_ERRQUIT(tg_expr(ni));

		TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));
		break;

	case TG_T_FLOAT:
	case TG_T_INT:
	case TG_T_STRING:
		TG_ERRQUIT(tg_gettoken(&t));
	
		TG_ERRQUIT(tg_nodeadd(ni, t.type, &t));
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

static int tg_args(int ni)
{
	struct tg_token t;
	int ani;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	if (tg_peektoken(&t) == TG_T_RPAR) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));
		return 0;
	}

	TG_ERRQUIT(ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL));
	TG_ERRQUIT(tg_assign(ani));

	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));
	
		TG_ERRQUIT(ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL));
		TG_ERRQUIT(tg_assign(ani));
	}

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	return 0;
}

static int tg_filter(int ni)
{
	struct tg_token t;
	int ani;

	TG_ERRQUIT(ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL));
	TG_ERRQUIT(tg_assign(ani));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_DDOT));
	TG_ERRQUIT(tg_nodeadd(ni, TG_T_DDOT, &t));

	TG_ERRQUIT(ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL));
	TG_ERRQUIT(tg_assign(ani));

	return 0;
}

static int tg_index(int ni)
{
	struct tg_token t;
	int fni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LBRK));

	TG_ERRQUIT(fni = tg_nodeadd(ni, TG_N_FILTER, NULL));
	TG_ERRQUIT(tg_filter(fni));

	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));

		TG_ERRQUIT(fni = tg_nodeadd(ni, TG_N_FILTER, NULL));
		TG_ERRQUIT(tg_filter(fni));
	}
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RBRK));

	return 0;
}

static int tg_attr(int ni)
{
	struct tg_token t;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_DOT));
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));
	
	TG_ERRQUIT(tg_nodeadd(ni, TG_T_ID, &t));

	return 0;
}

static int tg_address(int ni)
{
	struct tg_token t;
	enum TG_TYPE type;
	char strerr[1024];
	int ani;

	TG_ERRQUIT(tg_val(ni));

	while ((type = tg_peektoken(&t)) == TG_T_LPAR
		|| type == TG_T_LBRK || type == TG_T_DOT) {
		switch (type) {
		case TG_T_LPAR:
			TG_ERRQUIT(ani = tg_nodeadd(ni,
				TG_N_ARGS, NULL));
			TG_ERRQUIT(tg_args(ani));
			break;

		case TG_T_LBRK:
			TG_ERRQUIT(ani = tg_nodeadd(ni,
				TG_N_INDEX, NULL));
			TG_ERRQUIT(tg_index(ani));
			break;

		case TG_T_DOT:
			TG_ERRQUIT(ani = tg_nodeadd(ni,
				TG_N_ATTR, NULL));
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
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_STEPOP));

	TG_ERRQUIT(tg_nodeadd(ni, TG_T_STEPOP, &t));
	TG_ERRQUIT(tg_address(ni));

	return 0;

}

static int tg_ref(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_DOL));

	TG_ERRQUIT(tg_address(ni));

	return 0;
}

static int tg_not(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_NOT));

	TG_ERRQUIT(tg_address(ni));

	return 0;
}

static int tg_sign(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ADDOP));

	TG_ERRQUIT(tg_nodeadd(ni, TG_T_ADDOP, &t));
	TG_ERRQUIT(tg_address(ni));

	return 0;
}

static int tg_unary(int ni)
{
	struct tg_token t;

	tg_peektoken(&t);

	switch(t.type) {
	case TG_T_STEPOP:
		TG_ERRQUIT(ni = tg_nodeadd(ni, TG_N_PRESTEP, &t));
		TG_ERRQUIT(tg_prestep(ni));
		break;

	case TG_T_DOL:
		TG_ERRQUIT(ni = tg_nodeadd(ni, TG_N_REF, &t));
		TG_ERRQUIT(tg_ref(ni));
		break;

	case TG_T_NOT:
		TG_ERRQUIT(ni = tg_nodeadd(ni, TG_N_NOT, &t));
		TG_ERRQUIT(tg_not(ni));
		break;

	case TG_T_ADDOP:
		TG_ERRQUIT(ni = tg_nodeadd(ni, TG_N_SIGN, &t));
		TG_ERRQUIT(tg_sign(ni));
		break;

	default:
		TG_ERRQUIT(tg_address(ni));
	}

	return 0;
}

static int tg_mult(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_unary(ni));

	while (tg_peektoken(&t) == TG_T_MULTOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_MULTOP));
	
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_MULTOP, &t));

		TG_ERRQUIT(tg_unary(ni));
	}

	return 0;
}

static int tg_add(int ni)
{
	struct tg_token t;
	int mni;

	TG_ERRQUIT(mni = tg_nodeadd(-1, TG_N_MULT, &t));
	TG_ERRQUIT(tg_mult(mni));

	if (tg_nodeccnt(mni) == 1)
		TG_ERRQUIT(mni = tg_nodegetchild(mni, 0));

	TG_ERRQUIT(tg_nodeattach(ni, mni));

	while (tg_peektoken(&t) == TG_T_ADDOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ADDOP));

		TG_ERRQUIT(tg_nodeadd(ni, TG_T_ADDOP, &t));

		TG_ERRQUIT(mni = tg_nodeadd(-1, TG_N_MULT, &t));
		TG_ERRQUIT(tg_mult(mni));
	
		if (tg_nodeccnt(mni) == 1)
			TG_ERRQUIT(mni = tg_nodegetchild(mni, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, mni));
	}

	return 0;
}

static int tg_cat(int ni)
{
	struct tg_token t;
	int ani;

	TG_ERRQUIT(ani = tg_nodeadd(-1, TG_N_ADD, &t));
	TG_ERRQUIT(tg_add(ani));

	if (tg_nodeccnt(ani) == 1)
		TG_ERRQUIT(ani = tg_nodegetchild(ani, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, ani));

	while (tg_peektoken(&t) == TG_T_TILDA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_TILDA));
	
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_TILDA, &t));

		TG_ERRQUIT(ani = tg_nodeadd(-1, TG_N_ADD, &t));
		TG_ERRQUIT(tg_add(ani));
	
		if (tg_nodeccnt(ani) == 1)
			TG_ERRQUIT(ani = tg_nodegetchild(ani, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, ani));
	}

	return 0;
}

static int tg_nexttoopts(int ni)
{
	struct tg_token t;
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_COLON));
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_STRING));
		
	TG_ERRQUIT(tg_nodeadd(ni, TG_T_STRING, &t));

	return 0;
}

static int tg_nextto(int ni)
{
	struct tg_token t;
	int cni;

	TG_ERRQUIT(cni = tg_nodeadd(-1, TG_N_CAT, &t));
	TG_ERRQUIT(tg_cat(cni));

	if (tg_nodeccnt(cni) == 1)
		TG_ERRQUIT(cni = tg_nodegetchild(cni, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, cni));

	while (tg_peektoken(&t) == TG_T_NEXTTOOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_NEXTTOOP));
	
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_NEXTTOOP, &t));

		TG_ERRQUIT(cni = tg_nodeadd(-1, TG_N_CAT, &t));
		TG_ERRQUIT(tg_cat(cni));

		if (tg_nodeccnt(cni) == 1)
			TG_ERRQUIT(cni = tg_nodegetchild(cni, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, cni));

		if (tg_peektoken(&t) == TG_T_COLON)
			TG_ERRQUIT(tg_nexttoopts(ni));
	}

	return 0;
}

static int tg_rel(int ni)
{
	struct tg_token t;
	int nni;

	TG_ERRQUIT(nni = tg_nodeadd(-1, TG_N_NEXTTO, &t));
	TG_ERRQUIT(tg_nextto(nni));
	
	if (tg_nodeccnt(nni) == 1)
		TG_ERRQUIT(nni = tg_nodegetchild(nni, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, nni));

	if (tg_peektoken(&t) == TG_T_RELOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_RELOP));
	
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_RELOP, &t));

		TG_ERRQUIT(nni = tg_nodeadd(-1, TG_N_NEXTTO, &t));
		TG_ERRQUIT(tg_nextto(nni));

		if (tg_nodeccnt(nni) == 1)
			TG_ERRQUIT(nni = tg_nodegetchild(nni, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, nni));
	}

	return 0;

}

static int tg_and(int ni)
{
	struct tg_token t;
	int rni;

	TG_ERRQUIT(rni = tg_nodeadd(-1, TG_N_REL, &t));
	TG_ERRQUIT(tg_rel(rni));
	
	if (tg_nodeccnt(rni) == 1)
		TG_ERRQUIT(rni = tg_nodegetchild(rni, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, rni));

	while (tg_peektoken(&t) == TG_T_AND) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_AND));
	
		TG_ERRQUIT(rni = tg_nodeadd(-1, TG_N_REL, &t));
		TG_ERRQUIT(tg_rel(rni));

		if (tg_nodeccnt(rni) == 1)
			TG_ERRQUIT(rni = tg_nodegetchild(rni, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, rni));
	}

	return 0;
}

static int tg_or(int ni)
{
	struct tg_token t;
	int ani;

	TG_ERRQUIT(ani = tg_nodeadd(-1, TG_N_AND, &t));
	TG_ERRQUIT(tg_and(ani));
	
	if (tg_nodeccnt(ani) == 1)
		TG_ERRQUIT(ani = tg_nodegetchild(ani, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, ani));

	while (tg_peektoken(&t) == TG_T_OR) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_OR));
	
		TG_ERRQUIT(ani = tg_nodeadd(-1, TG_N_AND, &t));
		TG_ERRQUIT(tg_and(ani));

		if (tg_nodeccnt(ani) == 1)
			TG_ERRQUIT(ani = tg_nodegetchild(ani, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, ani));
	}

	return 0;
}

static int tg_ternary(int ni)
{
	struct tg_token t;
	int oni;

	TG_ERRQUIT(oni = tg_nodeadd(-1, TG_N_OR, &t));
	TG_ERRQUIT(tg_or(oni));

	if (tg_nodeccnt(oni) == 1)
		TG_ERRQUIT(oni = tg_nodegetchild(oni, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, oni));

	while (tg_peektoken(&t) == TG_T_QUEST) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_QUEST));
	
		TG_ERRQUIT(oni = tg_nodeadd(-1, TG_N_OR, &t));
		TG_ERRQUIT(tg_or(oni));

		if (tg_nodeccnt(oni) == 1)
			TG_ERRQUIT(oni = tg_nodegetchild(oni, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, oni));

		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COLON));

		TG_ERRQUIT(oni = tg_nodeadd(-1, TG_N_OR, &t));
		TG_ERRQUIT(tg_or(oni));

		if (tg_nodeccnt(oni) == 1)
			TG_ERRQUIT(oni = tg_nodegetchild(oni, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, oni));
	}

	return 0;
}

static int tg_assign(int ni)
{
	struct tg_token t;
	int tni;

	TG_ERRQUIT(tni = tg_nodeadd(-1, TG_N_TERNARY, &t));
	TG_ERRQUIT(tg_ternary(tni));
	
	if (tg_nodeccnt(tni) == 1)
		TG_ERRQUIT(tni = tg_nodegetchild(tni, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, tni));

	while (tg_peektoken(&t) == TG_T_ASSIGNOP) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ASSIGNOP));
		
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_ASSIGNOP, &t));
		
		TG_ERRQUIT(tni = tg_nodeadd(-1, TG_N_TERNARY, &t));
		TG_ERRQUIT(tg_ternary(tni));
	
		if (tg_nodeccnt(tni) == 1)
			TG_ERRQUIT(tni = tg_nodegetchild(tni, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, tni));
	}

	return 0;
}

static int tg_expr(int ni)
{
	struct tg_token t;
	int ani;

	TG_ERRQUIT(ani = tg_nodeadd(-1, TG_N_ASSIGN, &t));
	TG_ERRQUIT(tg_assign(ani));
		
	if (tg_nodeccnt(ani) == 1)
		TG_ERRQUIT(ani = tg_nodegetchild(ani, 0));
	
	TG_ERRQUIT(tg_nodeattach(ni, ani));

	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));
		
		TG_ERRQUIT(ani = tg_nodeadd(-1, TG_N_ASSIGN, &t));
		TG_ERRQUIT(tg_assign(ani));

		if (tg_nodeccnt(ani) == 1)
			TG_ERRQUIT(ani = tg_nodegetchild(ani, 0));
		
		TG_ERRQUIT(tg_nodeattach(ni, ani));
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
	TG_ERRQUIT(tg_nodeadd(ni, TG_T_ID, &t));
	
	while (tg_peektoken(&t) == TG_T_COMMA) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COMMA));
			
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_ID, &t));
	}

	return 0;
}

static int tg_funcdef(int ni)
{
	struct tg_token t;
	int nni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_FUNCTION));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_ID));
	TG_ERRQUIT(tg_nodeadd(ni, TG_T_ID, &t));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_DEFARGS, &t));
	TG_ERRQUIT(tg_defargs(nni));
	
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_BLOCK, &t));
	TG_ERRQUIT(tg_block(nni));

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

	TG_ERRQUIT(eni = tg_nodeadd(ni, TG_N_EXPR, NULL));
	TG_ERRQUIT(tg_expr(eni));

	return 0;
}

static int tg_fortable(int ni)
{
	struct tg_token t;
	int eni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_IN));

	TG_ERRQUIT(eni = tg_nodeadd(ni, TG_N_EXPR, NULL));
	TG_ERRQUIT(tg_expr(eni));

	if (tg_peektoken(&t) == TG_T_COLUMNS) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_COLUMNS));
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_COLUMNS, &t));
	}
	
	return 0;
}

static int tg_forclassic(int ni)
{
	struct tg_token t;
	int fni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));

	TG_ERRQUIT(fni = tg_nodeadd(ni, TG_N_FOREXPR, NULL));
	TG_ERRQUIT(tg_forexpr(fni));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));

	TG_ERRQUIT(fni = tg_nodeadd(ni, TG_N_FOREXPR, NULL));
	TG_ERRQUIT(tg_forexpr(fni));

	return 0;
}

static int tg_for(int ni)
{
	struct tg_token t;
	int nni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_FOR));
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_FOREXPR, NULL));
	
	TG_ERRQUIT(tg_forexpr(nni));

	if (tg_peektoken(&t) == TG_T_IN)
		TG_ERRQUIT(tg_fortable(ni));
	else
		TG_ERRQUIT(tg_forclassic(ni));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_BLOCK, NULL));
	TG_ERRQUIT(tg_body(nni));

	return 0;
}

static int tg_if(int ni)
{
	struct tg_token t;
	int nni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_IF));
	TG_ERRQUIT(tg_gettokentype(&t, TG_T_LPAR));

	TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_EXPR, NULL));
	TG_ERRQUIT(tg_expr(nni));

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RPAR));

	TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_BLOCK, NULL));
	TG_ERRQUIT(tg_body(nni));

	if (tg_peektoken(&t) == TG_T_ELSE) {
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_ELSE));

		TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_BLOCK, NULL));
		TG_ERRQUIT(tg_body(nni));
	}

	return 0;
}

static int tg_return(int ni)
{
	struct tg_token t;
	int eni;

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_RETURN));

	if (tg_peektoken(&t) != TG_T_SEMICOL) {
		TG_ERRQUIT(eni = tg_nodeadd(ni, TG_N_EXPR, NULL));
		TG_ERRQUIT(tg_expr(eni));
	}

	TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));

	return 0;
}

static int tg_stmt(int ni)
{
	struct tg_token t;
	int nni;

	switch (tg_peektoken(&t)) {
	case TG_T_FOR:
		TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_FOR, NULL));
		TG_ERRQUIT(tg_for(nni));
		break;

	case TG_T_IF:
		TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_IF, NULL));
		TG_ERRQUIT(tg_if(nni));
		break;

	case TG_T_RETURN:
		TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_RETURN, NULL));
		TG_ERRQUIT(tg_return(nni));
		break;

	case TG_T_BREAK:
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_BREAK, &t));
		break;

	case TG_T_CONTINUE:
		TG_ERRQUIT(tg_nodeadd(ni, TG_T_CONTINUE, &t));
		break;

	default:
		TG_ERRQUIT(nni = tg_nodeadd(ni, TG_N_EXPR, NULL));
		TG_ERRQUIT(tg_expr(nni));
		TG_ERRQUIT(tg_gettokentype(&t, TG_T_SEMICOL));
		break;
	}

	return 0;
}

int tg_template(const char *p)
{
	struct tg_token t;
	int type;
	int ni;
	
	if (tg_initparser(p) < 0)
		goto error;
	
	if (tg_darrinit(&nodes, sizeof(struct tg_node)) < 0)
		goto error;

	if ((ni = tg_nodeadd(-1, TG_N_TEMPLATE, NULL)) < 0)
		goto error;

	while ((type = tg_peektoken(&t)) != TG_T_EOF) {
		if (t.type == TG_T_FUNCTION) {
			if (tg_funcdef(ni) < 0)
				goto error;
		}
		else {
			if (tg_stmt(ni) < 0)
				goto error;
		}
	}

	tg_printnode(ni, 0);

	tg_finilizeparser();

	return ni;

error:
	printf("tg_error: |\n%s:%s|\n", p, tg_error);

	tg_finilizeparser();

	return ni;
}
