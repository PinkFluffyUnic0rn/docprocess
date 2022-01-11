#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "tg_parser.h"
#include "tg_darray.h"
#include "tg_dstring.h"

#define MSGLENMAX 1024

const char *tg_tstrsym[] = {
	"identificator", "integer", "float", "quoted string",
	"relational operator", "(", ")", "&", "|", "~", ",", "!",
	";", ":", "$", "multiplication or division",
	"addition or substraction", "..", "=", "?", "next to operator",
	"global", "function", "for", "if", "else", "continue", "break",
	"return", "in", ".", "{", "}", "[", "]", "end of file", "error"
};

const char *tg_nstrsym[] = {
	"template", "function definition",
	"function definition arguments", "statement", "return", "if",
	"for", "for (expression)", "for (classic)", "statement body",
	"block", "expression", "assignment", "ternary operator",
	"logical or", "logical and", "relation", "next to",
	"next to options", "string concationation",
	"addtion or substraction", "multiplication or division",
	"unary operator", "reference", "logical not", "sign",
	"id usage", "value", "constant", "array access", "filter",
	"range", "function arguments", "object attribute access"
};

static const char *tg_path;
struct tg_darray tg_text;
static int tg_curline;
static int tg_curpos;

/*
static struct tg_node *nodes;
static int nodescount;
static int maxnodes;
*/

struct tg_char {
	int c;
	char error[MSGLENMAX];
	int line;
	int pos;
};

static struct tg_char cbuf[2];
static int getcinit = 0;

int tg_initparser(const char *path)
{
	if (tg_darrinit(&tg_text, sizeof(char *)) < 0)
		return (-1);

	tg_path = path;

	return 0;
}

// string stream to char stream
static struct tg_char _tg_getc(int raw)
{
	struct tg_char c;
	static FILE *f;
	static char *l = "";
	static int curline = 0;

	if (tg_path != NULL) {
		if ((f = fopen(tg_path, "r")) == NULL)
			goto error;

		curline = 0;
		tg_path = NULL;
	}

	while (1) {
		size_t lsz;
		ssize_t r;

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
		
		lsz = 0;
		l = NULL;
		if ((r = getline(&l, &lsz, f)) <= 0)
			goto error;
		
		if (tg_darrpush(&tg_text, &l) < 0)
			goto error;

		curline++;
	}

	c.c = (*l++);
	c.line = curline;
	c.pos = l - ((char **) tg_text.data)[curline - 1];

	return c;

error:
	c.c = EOF;
	c.line = curline;

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
	cbuf[1] = _tg_getc(0);

	tg_curline = c.line;
	tg_curpos = c.pos;
	
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
	cbuf[1] = _tg_getc(1);

	tg_curline = c.line;
	tg_curpos = c.pos;
	
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

static int tg_createtoken(struct tg_token *t, const char *val,
	enum TG_T_TYPE type, int line, int pos)
{
	if (tg_dstrcreate(&(t->val), val) < 0)
		return (-1);

	t->type = type;
	t->line = line;
	t->pos = pos;

	return 0;
}

// lexical analyzer
int tg_nexttoken(struct tg_token *t)
{
	int c;

	while ((tg_peekc()) == ' ')
		tg_getc();

	if ((c = tg_getc()) == EOF)
		goto error;

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
				if (c == EOF)		goto error;
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
			else if (c == EOF)
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
					if (e)	goto error;
					if (p)	goto error;
					else	p = 1;
				}
				if (c == 'e' || c == 'E') {
					if (e)	goto error;
					else	e = 1;
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
			if (tg_createtoken(t, "==", TG_T_RELOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, "=", TG_T_ASSIGN,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == '!') {
		if (tg_peekc() == '=') {
			if (tg_createtoken(t, "!=", TG_T_RELOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, "!", TG_T_NOT,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == '>') {
		if (tg_peekc() == '=') {
			if (tg_createtoken(t, ">=", TG_T_RELOP,
					tg_curline, tg_curpos) < 0)
				goto error;
			
			tg_getc();
		}
		else {
			if (tg_createtoken(t, ">", TG_T_RELOP,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == '<') {
		c = tg_peekc();
		if (c == '=') {
			if (tg_createtoken(t, "<=", TG_T_RELOP,
					tg_curline, tg_curpos) < 0)
				goto error;
			
			tg_getc();
		}
		else if (c == '-') {
			if (tg_createtoken(t, "<-", TG_T_NEXTTOOP,
					tg_curline, tg_curpos) < 0)
				goto error;
			
			tg_getc();
		}
		else {
			if (tg_createtoken(t, "<", TG_T_RELOP,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == '^') {
		if (tg_createtoken(t, "^", TG_T_NEXTTOOP,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '(') {
		if (tg_createtoken(t, "(", TG_T_LPAR,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == ')') {
		if (tg_createtoken(t, ")", TG_T_RPAR,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '&') {
		if (tg_createtoken(t, "&", TG_T_AND,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '|') {
		if (tg_createtoken(t, "|", TG_T_OR,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '$') {
		if (tg_createtoken(t, "$", TG_T_DOL,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '~') {
		if (tg_createtoken(t, "~", TG_T_TILDA,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '.') {
		if (tg_peekc() == '.') {
			if (tg_createtoken(t, "..", TG_T_DDOT,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, ".", TG_T_DOT,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == ',') {
		if (tg_createtoken(t, ",", TG_T_COMMA,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == ':') {
		if (tg_createtoken(t, ":", TG_T_COLON,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == ';') {
		if (tg_createtoken(t, ";", TG_T_SEMICOL,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '*') {
		if (tg_createtoken(t, "*", TG_T_MULTOP,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '/') {
		if (tg_createtoken(t, "/", TG_T_MULTOP,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '-') {
		if (tg_createtoken(t, "-", TG_T_ADDOP,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '+') {
		if (tg_createtoken(t, "+", TG_T_ADDOP,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '{') {
		if (tg_createtoken(t, "{", TG_T_LBRC,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '}') {
		if (tg_createtoken(t, "}", TG_T_RBRC,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '[') {
		if (tg_createtoken(t, "[", TG_T_LBRK,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == ']') {
		if (tg_createtoken(t, "]", TG_T_RBRK,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else if (c == '?') {
		if (tg_createtoken(t, "?", TG_T_QUEST,
				tg_curline, tg_curpos) < 0)
			goto error;
	}
	else {
		goto error;
	}

	return 0;

error:
	if (tg_createtoken(t, "", TG_T_ERROR,
			tg_curline, tg_curpos) < 0)
		goto error;

	return (-1);
}
/*
static struct tg_token *tg_gettoken()
{
	return NULL;
}

static struct tg_token *tg_peektoken()
{
	return NULL;
}

static struct tg_token *tg_gettokentype(enum TG_T_TYPE type)
{
	return NULL;
}

// syntax analizer
static struct tg_node *node_add(struct tg_node *parent,
	enum TG_N_TYPE type, struct tg_token *token)
{
	return NULL;
}
*/
int node_remove(struct tg_node *n)
{
	return 0;
}

struct tg_node *tg_template(const char *p)
{
	tg_initparser(p);

	struct tg_token t;

	while (tg_nexttoken(&t) >= 0) {
		printf("token value: |%s|\ntoken type: |%s|\nline: %d\npos: %d\n\n",
			t.val.str, tg_tstrsym[t.type], t.line, t.pos);
	}

	printf("HERE!\n");

	return NULL;
}
