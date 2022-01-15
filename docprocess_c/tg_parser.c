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
	";", ":", "$", "addition or substraction",
	"multiplication or division", "..", "=", "?",
	"next to operator", "global", "function", "for", "if", "else",
	"continue", "break", "return", "in", ".", "{", "}", "[", "]",
	"end of file", "error", "", "template", "function definition",
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

static struct tg_char cbuf[2];
static int getcinit = 0;

static struct tg_token tbuf[2];
static int gettinit = 0;

static struct tg_darray nodes;

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
		if ((f = fopen(tg_path, "r")) == NULL) {
			TG_SETERROR("Cannot open file %s: %s.",
				tg_path, strerror(errno));
			goto error;
		}

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
		if ((r = getline(&l, &lsz, f)) <= 0) {
			TG_SETERROR("Cannot read line: %s.",
				strerror(errno));
			goto error;
		}
		
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
	strncpy(c.error, tg_error, TG_MSGMAXSIZE);

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
	cbuf[1] = _tg_getc(1);

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
		TG_SETERROR("Unknown token.");
		goto error;
	}

	return 0;

error:
	if (tg_createtoken(t, tg_error, TG_T_ERROR,
			tg_curline, tg_curpos) < 0)
		goto error;

	return (-1);
}

int tg_gettoken(struct tg_token *t)
{
	struct tg_token tt;

	if (gettinit == 0) {
		if (tg_nexttoken(tbuf + 1) < 0)
			return (-1);

		gettinit = 1;
	}

	tbuf[0] = tbuf[1];
	tt = tbuf[0];

	if (tg_nexttoken(tbuf + 1) < 0)
		return (-1);

	*t = tt;

	return 0;
}

int tg_gettokentype(struct tg_token *t, enum TG_TYPE type)
{
	if (tg_gettoken(t) < 0)
		return (-1);

	if (t->type != type) {
		TG_SETERROR("Wrong token type: expected %s, got %s.",
			tg_strsym[t->type], tg_strsym[type]);
		return (-1);
	}

	return 0;
}

int tg_peektoken(struct tg_token *t)
{
	if (gettinit == 0) {
		tg_nexttoken(tbuf + 1);
		gettinit = 1;
	}

	*t = tbuf[1];
	
	return 0;
}

// syntax analizer
int node_add(int pi, enum TG_TYPE type, struct tg_token *token)
{	
	struct tg_node n;
	struct tg_node *np;
	struct tg_node *parent;
	int ni;
	int i;

	n.type = type;
	
	if (token != NULL)
		n.token = *token;

	if (tg_darrinit(&(n.children), sizeof(int)) < 0)
		return (-1);

	if (tg_darrinit(&(n.siblings), sizeof(int)) < 0)
		return (-1);

	if ((ni = tg_darrpush(&nodes, &n)) < 0)
		return (-1);
	
	if ((np = tg_darrget(&nodes, ni)) == NULL)
		return (-1);
	
	if (pi < 0)
		return ni;

	parent = tg_darrget(&nodes, pi);

	for (i = 0; i < parent->children.cnt; ++i) {
/*
		struct tg_node *cp;

		if ((cp = tg_darrget(&(parent->children), i)) == NULL)
			return NULL;

		if (tg_darrpush(&(cp->siblings), np) < 0)
			return NULL;

		add ni to all siblings
		add all siblings to np
*/
	}

	if (tg_darrpush(&(parent->children), &ni) < 0)
		return (-1);

	return ni;
}

/*
int node_remove(struct tg_node *n)
{
	return 0;
}
*/

int tg_indent(int t)
{
	int i;

	for (i = 0; i < t; ++i)
		printf("\t");

	return 0;
}

int tg_printnode(struct tg_node *n, int depth)
{
	int i;

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
		
			tg_printnode(tg_darrget(&nodes, ni), depth + 2);	

		//	printf("!@#!@#%s\n", n->token.val.str);
		}
		tg_indent(depth + 1);
		printf("}\n");
	}

	tg_indent(depth);
	printf("}\n");

	return 0;
}

struct tg_node *tg_template(const char *p)
{
	tg_initparser(p);

	tg_darrinit(&nodes, sizeof(struct tg_node)); // !!!

	int ni;

	ni = node_add(-1, TG_N_TEMPLATE, NULL);
	
	struct tg_token t;

	tg_gettoken(&t);

	node_add(ni, TG_T_FLOAT, &t);

	tg_printnode(tg_darrget(&nodes, ni), 0);

/*
	struct tg_token t;

	while (tg_gettoken(&t) >= 0) {
		printf("token value: |%s|\ntoken type: |%s|\nline: %d\npos: %d\n\n",
			t.val.str, tg_strsym[t.type], t.line, t.pos);

		tg_peektoken(&t);
		printf("next token value: |%s|\ntoken type: |%s|\nline: %d\npos: %d\n\n",
			t.val.str, tg_strsym[t.type], t.line, t.pos);
	
		printf("-----------------------------------\n\n");
	}

	printf("HERE!\n");
*/
	return NULL;
}
