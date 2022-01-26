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
			if (tg_createtoken(t, "==", TG_T_RELOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, "=", TG_T_ASSIGNOP,
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
		if (tg_peekc() == '=') {
			if (tg_createtoken(t, "~=", TG_T_ASSIGNOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, "~", TG_T_TILDA,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
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
		if (tg_peekc() == '=') {
			if (tg_createtoken(t, "*=", TG_T_ASSIGNOP,
					tg_curline, tg_curpos) < 0)
				goto error;
			
			tg_getc();
		}
		else {
			if (tg_createtoken(t, "*", TG_T_MULTOP,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == '/') {
		if (tg_peekc() == '=') {
			if (tg_createtoken(t, "/=", TG_T_ASSIGNOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, "/", TG_T_MULTOP,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == '-') {
		c = tg_peekc();
		if (c == '-') {
			if (tg_createtoken(t, "--", TG_T_STEPOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else if (c == '=') {
			if (tg_createtoken(t, "-=", TG_T_ASSIGNOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, "-", TG_T_ADDOP,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
	}
	else if (c == '+') {
		c = tg_peekc();
		if (c == '+') {
			if (tg_createtoken(t, "++", TG_T_STEPOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else if (c == '=') {
			if (tg_createtoken(t, "+=", TG_T_ASSIGNOP,
					tg_curline, tg_curpos) < 0)
				goto error;

			tg_getc();
		}
		else {
			if (tg_createtoken(t, "+", TG_T_ADDOP,
					tg_curline, tg_curpos) < 0)
				goto error;
		}
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

	return 0;
}

int tg_gettoken(struct tg_token *t)
{
	if (gettinit == 0) {
		if (tg_nexttoken(tbuf + 1) < 0)
			return (-1);

		gettinit = 1;
	}

	if (tbuf[0].type == TG_T_EOF || tbuf[0].type == TG_T_ERROR) {
		*t = tbuf[0];
		return 0;
	}

	tbuf[0] = tbuf[1];
	*t = tbuf[0];

	if (tg_nexttoken(tbuf + 1) < 0)
		return (-1);

	return t->type;
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

	return t->type;
}

int tg_peektoken(struct tg_token *t)
{
	if (gettinit == 0) {
		tg_nexttoken(tbuf + 1);
		gettinit = 1;
	}

	if (tbuf[0].type == TG_T_EOF || tbuf[0].type == TG_T_ERROR) {
		*t = tbuf[0];
		return 0;
	}

	*t = tbuf[1];
	
	return t->type;
}

// syntax analizer
int tg_nodeadd(int pi, enum TG_TYPE type, struct tg_token *token)
{	
	struct tg_node n;
//	struct tg_node *np;
	struct tg_node *parent;
	int ni;
//	int i;

	n.type = type;
	
	if (token != NULL)
		n.token = *token;

	if (tg_darrinit(&(n.children), sizeof(int)) < 0)
		return (-1);

	n.parent = pi;

//	if (tg_darrinit(&(n.siblings), sizeof(int)) < 0)
//		return (-1);

	if ((ni = tg_darrpush(&nodes, &n)) < 0)
		return (-1);
	
	if (pi < 0)
		return ni;

	if ((parent = tg_darrget(&nodes, pi)) == NULL)
		return (-1);
/*
	for (i = 0; i < parent->children.cnt; ++i) {

		struct tg_node *cp;

		if ((cp = tg_darrget(&(parent->children), i)) == NULL)
			return NULL;

		if (tg_darrpush(&(cp->siblings), np) < 0)
			return NULL;

		add ni to all siblings
		add all siblings to np
	}
*/
	if (tg_darrpush(&(parent->children), &ni) < 0)
		return (-1);

	return ni;
}

int tg_attachnode(int pi, int ni)
{
	struct tg_node *node;
	struct tg_node *parent;

	parent = tg_darrget(&nodes, pi);
	node = tg_darrget(&nodes, ni);

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
	
	p = tg_darrget(&nodes, ni);

	if ((pci = tg_darrget(&(p->children), i)) == NULL)
		return (-1);

	return *pci;
}

/*
int tg_attachtree(int pi, int ti)
{
	struct tg_node *treeroot;
	struct tg_node *parent;
	int i;

	parent = tg_darrget(&nodes, pi);
	treeroot = tg_darrget(&nodes, ti);

	for (i = 0; i < treeroot->children.cnt; ++i) {
		struct tg_node *cp;
		int ci;

		ci = *((int *) tg_darrget(&(treeroot->children), i)); //!!!

		if ((cp = tg_darrget(&nodes, ci)) == NULL)
			return (-1);

		cp->parent = pi;

		if (tg_darrpush(&(parent->children), &ci) < 0)
			return (-1);
	}

	// remove root ?

	return 0;
}
*/

// int tg_getnode(int ni) ?

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
		printf("  ");
		//printf("\t");

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

int tg_id(int ni)
{
	struct tg_token t;

	tg_gettokentype(&t, TG_T_ID);

	tg_nodeadd(ni, TG_T_ID, &t);

	return 0;
}

int tg_constant(int ni)
{
	struct tg_token t;

	tg_gettoken(&t);

	if (t.type != TG_T_STRING && t.type != TG_T_INT
		&& t.type != TG_T_FLOAT) {
		// error
	}
	
	tg_nodeadd(ni, t.type, &t);

	return 0;
}

int tg_expr(int ni);
int tg_assign(int ni);
int tg_stmt(int ni);

int tg_val(int ni)
{
	struct tg_token t;

	tg_peektoken(&t);

	switch (t.type) {
	case TG_T_ID:
		tg_id(ni);
		break;

	case TG_T_LPAR:
		tg_gettoken(&t);

		ni = tg_nodeadd(ni, TG_N_EXPR, NULL);

		tg_expr(ni);

		tg_gettokentype(&t, TG_T_RPAR);
		break;

	case TG_T_FLOAT:
	case TG_T_INT:
	case TG_T_STRING:
		tg_constant(ni);
		break;

	default:
		// error
		break;
	}

	return 0;
}

int tg_args(int ni)
{
	struct tg_token t;
	int ani;
	
	tg_gettokentype(&t, TG_T_LPAR);

	if (tg_peektoken(&t) == TG_T_RPAR) {
		tg_gettokentype(&t, TG_T_RPAR);
		return 0;
	}

	ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL);
	tg_assign(ani);

	while (tg_peektoken(&t) == TG_T_COMMA) {
		tg_gettokentype(&t, TG_T_COMMA);
	
		ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL);
		tg_assign(ani);
	}

	tg_gettokentype(&t, TG_T_RPAR);

	return 0;
}

int tg_filter(int ni)
{
	struct tg_token t;
	int ani;

	ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL);
	tg_assign(ani);

	tg_gettokentype(&t, TG_T_DDOT);
	tg_nodeadd(ni, TG_T_DDOT, &t);

	ani = tg_nodeadd(ni, TG_N_ASSIGN, NULL);
	tg_assign(ani);

	return 0;
}

int tg_index(int ni)
{
	struct tg_token t;
	int fni;

	tg_gettokentype(&t, TG_T_LBRK);
	
	fni = tg_nodeadd(ni, TG_N_FILTER, NULL);
	tg_filter(fni);

	while (tg_peektoken(&t) == TG_T_COMMA) {
		tg_gettokentype(&t, TG_T_COMMA);
	
		fni = tg_nodeadd(ni, TG_N_FILTER, NULL);
		tg_filter(fni);
	}
	tg_gettokentype(&t, TG_T_RBRK);

	return 0;
}

int tg_attr(int ni)
{
	struct tg_token t;

	tg_gettokentype(&t, TG_T_DOT);
	
	tg_gettokentype(&t, TG_T_ID);
	
	tg_nodeadd(ni, TG_T_ID, &t);

	return 0;
}

int tg_address(int ni)
{
	struct tg_token t;
	enum TG_TYPE type;
	int ani;

	tg_val(ni);

	while ((type = tg_peektoken(&t)) == TG_T_LPAR
		|| type == TG_T_LBRK || type == TG_T_DOT) {
		switch (type) {
		case TG_T_LPAR:
			ani = tg_nodeadd(ni, TG_N_ARGS, NULL);
			tg_args(ani);
			break;

		case TG_T_LBRK:
			ani = tg_nodeadd(ni, TG_N_INDEX, NULL);
			tg_index(ani);
			break;

		case TG_T_DOT:
			ani = tg_nodeadd(ni, TG_N_ATTR, NULL);
			tg_attr(ani);
			break;

		default:
			break;
		}
	}

	return 0;
}

int tg_prestep(int ni)
{
	struct tg_token t;
	
	tg_gettokentype(&t, TG_T_STEPOP);

	tg_nodeadd(ni, TG_T_STEPOP, &t);
	tg_address(ni);

	return 0;

}

int tg_ref(int ni)
{
	struct tg_token t;
	
	tg_gettokentype(&t, TG_T_DOL);

	tg_address(ni);

	return 0;
}

int tg_not(int ni)
{
	struct tg_token t;
	
	tg_gettokentype(&t, TG_T_NOT);

	tg_address(ni);

	return 0;
}

int tg_sign(int ni)
{
	struct tg_token t;
	
	tg_gettokentype(&t, TG_T_ADDOP);

	tg_nodeadd(ni, TG_T_ADDOP, &t);
	tg_address(ni);

	return 0;
}

int tg_unary(int ni)
{
	struct tg_token t;

	tg_peektoken(&t);

	switch(t.type) {
	case TG_T_STEPOP:
		ni = tg_nodeadd(ni, TG_N_PRESTEP, &t);
		tg_prestep(ni);
		break;

	case TG_T_DOL:
		ni = tg_nodeadd(ni, TG_N_REF, &t);
		tg_ref(ni);
		break;

	case TG_T_NOT:
		ni = tg_nodeadd(ni, TG_N_NOT, &t);
		tg_not(ni);
		break;

	case TG_T_ADDOP:
		ni = tg_nodeadd(ni, TG_N_SIGN, &t);
		tg_sign(ni);
		break;

	default:
		tg_address(ni);
	}

	return 0;
}

int tg_mult(int ni)
{
	struct tg_token t;
	
	tg_unary(ni);

	while (tg_peektoken(&t) == TG_T_MULTOP) {
		tg_gettokentype(&t, TG_T_MULTOP);
	
		tg_nodeadd(ni, TG_T_MULTOP, &t);

		tg_unary(ni);
	}

	return 0;
}

int tg_add(int ni)
{
	struct tg_token t;
	int mni;

	mni = tg_nodeadd(-1, TG_N_MULT, &t);
	tg_mult(mni);

	if (tg_nodeccnt(mni) == 1)
		mni = tg_nodegetchild(mni, 0);
	
	tg_attachnode(ni, mni);

	while (tg_peektoken(&t) == TG_T_ADDOP) {
		tg_gettokentype(&t, TG_T_ADDOP);

		tg_nodeadd(ni, TG_T_ADDOP, &t);

		mni = tg_nodeadd(-1, TG_N_MULT, &t);
		tg_mult(mni);
	
		if (tg_nodeccnt(mni) == 1)
			mni = tg_nodegetchild(mni, 0);
		
		tg_attachnode(ni, mni);
	}

	return 0;
}

int tg_cat(int ni)
{
	struct tg_token t;
	int ani;

	ani = tg_nodeadd(-1, TG_N_ADD, &t);
	tg_add(ani);

	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodegetchild(ani, 0);
	
	tg_attachnode(ni, ani);

	while (tg_peektoken(&t) == TG_T_TILDA) {
		tg_gettokentype(&t, TG_T_TILDA);
	
		tg_nodeadd(ni, TG_T_TILDA, &t);

		ani = tg_nodeadd(-1, TG_N_ADD, &t);
		tg_add(ani);
	
		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodegetchild(ani, 0);
		
		tg_attachnode(ni, ani);
	}

	return 0;
}

int tg_nexttoopts(int ni)
{
	struct tg_token t;
	
	tg_gettokentype(&t, TG_T_COLON);
	
	tg_gettokentype(&t, TG_T_STRING);
		
	tg_nodeadd(ni, TG_T_STRING, &t);

	return 0;
}

int tg_nextto(int ni)
{
	struct tg_token t;
	int cni;

	cni = tg_nodeadd(-1, TG_N_CAT, &t);
	tg_cat(cni);

	if (tg_nodeccnt(cni) == 1)
		cni = tg_nodegetchild(cni, 0);
	
	tg_attachnode(ni, cni);

	while (tg_peektoken(&t) == TG_T_NEXTTOOP) {
		tg_gettokentype(&t, TG_T_NEXTTOOP);
	
		tg_nodeadd(ni, TG_T_NEXTTOOP, &t);

		cni = tg_nodeadd(-1, TG_N_CAT, &t);
		tg_cat(cni);

		if (tg_nodeccnt(cni) == 1)
			cni = tg_nodegetchild(cni, 0);
		
		tg_attachnode(ni, cni);

		if (tg_peektoken(&t) == TG_T_COLON)
			tg_nexttoopts(ni);
	}

	return 0;
}

int tg_rel(int ni)
{
	struct tg_token t;
	int nni;

	nni = tg_nodeadd(-1, TG_N_NEXTTO, &t);
	tg_nextto(nni);
	
	if (tg_nodeccnt(nni) == 1)
		nni = tg_nodegetchild(nni, 0);
	
	tg_attachnode(ni, nni);

	if (tg_peektoken(&t) == TG_T_RELOP) {
		tg_gettokentype(&t, TG_T_RELOP);
	
		tg_nodeadd(ni, TG_T_RELOP, &t);

		nni = tg_nodeadd(-1, TG_N_NEXTTO, &t);
		tg_nextto(nni);

		if (tg_nodeccnt(nni) == 1)
			nni = tg_nodegetchild(nni, 0);
		
		tg_attachnode(ni, nni);
	}

	return 0;

}

int tg_and(int ni)
{
	struct tg_token t;
	int rni;

	rni = tg_nodeadd(-1, TG_N_REL, &t);
	tg_rel(rni);
	
	if (tg_nodeccnt(rni) == 1)
		rni = tg_nodegetchild(rni, 0);
	
	tg_attachnode(ni, rni);

	while (tg_peektoken(&t) == TG_T_AND) {
		tg_gettokentype(&t, TG_T_AND);
	
		rni = tg_nodeadd(-1, TG_N_REL, &t);
		tg_rel(rni);

		if (tg_nodeccnt(rni) == 1)
			rni = tg_nodegetchild(rni, 0);
		
		tg_attachnode(ni, rni);
	}

	return 0;
}

int tg_or(int ni)
{
	struct tg_token t;
	int ani;

	ani = tg_nodeadd(-1, TG_N_AND, &t);
	tg_and(ani);
	
	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodegetchild(ani, 0);
	
	tg_attachnode(ni, ani);

	while (tg_peektoken(&t) == TG_T_OR) {
		tg_gettokentype(&t, TG_T_OR);
	
		ani = tg_nodeadd(-1, TG_N_AND, &t);
		tg_and(ani);

		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodegetchild(ani, 0);
		
		tg_attachnode(ni, ani);
	}

	return 0;
}

int tg_ternary(int ni)
{
	struct tg_token t;
	int oni;

	oni = tg_nodeadd(-1, TG_N_OR, &t);
	tg_or(oni);

	if (tg_nodeccnt(oni) == 1)
		oni = tg_nodegetchild(oni, 0);
	
	tg_attachnode(ni, oni);

	while (tg_peektoken(&t) == TG_T_QUEST) {
		tg_gettokentype(&t, TG_T_QUEST);
	
		oni = tg_nodeadd(-1, TG_N_OR, &t);
		tg_or(oni);

		if (tg_nodeccnt(oni) == 1)
			oni = tg_nodegetchild(oni, 0);
		
		tg_attachnode(ni, oni);

		tg_gettokentype(&t, TG_T_COLON);

		oni = tg_nodeadd(-1, TG_N_OR, &t);
		tg_or(oni);

		if (tg_nodeccnt(oni) == 1)
			oni = tg_nodegetchild(oni, 0);
		
		tg_attachnode(ni, oni);
	}

	return 0;
}

int tg_assign(int ni)
{
	struct tg_token t;
	int tni;

	tni = tg_nodeadd(-1, TG_N_TERNARY, &t);
	tg_ternary(tni);
	
	if (tg_nodeccnt(tni) == 1)
		tni = tg_nodegetchild(tni, 0);
	
	tg_attachnode(ni, tni);

	while (tg_peektoken(&t) == TG_T_ASSIGNOP) {
		tg_gettokentype(&t, TG_T_ASSIGNOP);
		
		tg_nodeadd(ni, TG_T_ASSIGNOP, &t);
		
		tni = tg_nodeadd(-1, TG_N_TERNARY, &t);
		tg_ternary(tni);
	
		if (tg_nodeccnt(tni) == 1)
			tni = tg_nodegetchild(tni, 0);
		
		tg_attachnode(ni, tni);
	}

	return 0;
}

int tg_expr(int ni)
{
	struct tg_token t;
	int ani;

	ani = tg_nodeadd(-1, TG_N_ASSIGN, &t);
	tg_assign(ani);
		
	if (tg_nodeccnt(ani) == 1)
		ani = tg_nodegetchild(ani, 0);
	
	tg_attachnode(ni, ani);

	while (tg_peektoken(&t) == TG_T_COMMA) {
		tg_gettokentype(&t, TG_T_COMMA);
		
		ani = tg_nodeadd(-1, TG_N_ASSIGN, &t);
		tg_assign(ani);

		if (tg_nodeccnt(ani) == 1)
			ani = tg_nodegetchild(ani, 0);
		
		tg_attachnode(ni, ani);
	}

	return 0;
}

int tg_block(int ni)
{
	struct tg_token t;
	
	tg_gettokentype(&t, TG_T_LBRC);

	while (tg_peektoken(&t) != TG_T_RBRC)
		tg_stmt(ni);

	tg_gettokentype(&t, TG_T_RBRC);

	return 0;
}

int tg_defargs(int ni)
{
	struct tg_token t;
	
	tg_gettokentype(&t, TG_T_ID);
	tg_nodeadd(ni, TG_T_ID, &t);
	
	while (tg_peektoken(&t) == TG_T_COMMA) {
		tg_gettokentype(&t, TG_T_COMMA);
			
		tg_gettokentype(&t, TG_T_ID);
		tg_nodeadd(ni, TG_T_ID, &t);
	}

	return 0;
}

int tg_funcdef(int ni)
{
	struct tg_token t;
	int nni;

	tg_gettokentype(&t, TG_T_FUNCTION);

	tg_gettokentype(&t, TG_T_ID);

	tg_gettokentype(&t, TG_T_LPAR);

	nni = tg_nodeadd(ni, TG_N_DEFARGS, &t);
	tg_defargs(nni);
	
	tg_gettokentype(&t, TG_T_RPAR);

	nni = tg_nodeadd(ni, TG_N_BLOCK, &t);
	tg_block(nni);

	return 0;
}

int tg_body(int ni)
{
	struct tg_token t;

	if (tg_peektoken(&t) == TG_T_LBRC)
		tg_block(ni);
	else
		tg_stmt(ni);

	return 0;
}

int tg_forexpr(int ni)
{
	struct tg_token t;
	int eni;

	tg_peektoken(&t);

	if (t.type == TG_T_SEMICOL || t.type == TG_T_RPAR)
		return 0;

	eni = tg_nodeadd(ni, TG_N_EXPR, NULL);
	tg_expr(eni);

	return 0;
}

int tg_fortable(int ni)
{
	struct tg_token t;
	int eni;

	tg_gettokentype(&t, TG_T_IN);

	eni = tg_nodeadd(ni, TG_N_EXPR, NULL);
	tg_expr(eni);

	tg_peektoken(&t);
	if (t.type == TG_T_COLUMNS) {
		tg_gettokentype(&t, TG_T_COLUMNS);
		tg_nodeadd(ni, TG_T_COLUMNS, &t);
	}
	
	return 0;
}

int tg_forclassic(int ni)
{
	struct tg_token t;
	int fni;

	tg_gettokentype(&t, TG_T_SEMICOL);

	fni = tg_nodeadd(ni, TG_N_FOREXPR, NULL);
	tg_forexpr(fni);

	tg_gettokentype(&t, TG_T_SEMICOL);

	fni = tg_nodeadd(ni, TG_N_FOREXPR, NULL);
	tg_forexpr(fni);

	return 0;
}

int tg_for(int ni)
{
	struct tg_token t;
	int nni;

	tg_gettokentype(&t, TG_T_FOR);
	tg_gettokentype(&t, TG_T_LPAR);

	nni = tg_nodeadd(ni, TG_N_FOREXPR, NULL);
	
	tg_forexpr(nni);

	if (tg_peektoken(&t) == TG_T_IN) {
		tg_fortable(ni);
	}
	else {
		tg_forclassic(ni);
	}

	tg_gettokentype(&t, TG_T_RPAR);

	nni = tg_nodeadd(ni, TG_N_BLOCK, NULL);
	tg_body(nni);

	return 0;
}

int tg_if(int ni)
{
	struct tg_token t;
	int nni;

	tg_gettokentype(&t, TG_T_FOR);
	tg_gettokentype(&t, TG_T_LPAR);

	nni = tg_nodeadd(ni, TG_N_EXPR, NULL);
	tg_expr(nni);

	tg_gettokentype(&t, TG_T_RPAR);

	nni = tg_nodeadd(ni, TG_N_BLOCK, NULL);
	tg_body(nni);

	if (tg_peektoken(&t) == TG_T_ELSE) {
		tg_gettokentype(&t, TG_T_ELSE);

		nni = tg_nodeadd(ni, TG_N_BLOCK, NULL);
		tg_body(nni);
	}

	return 0;
}

int tg_return(int ni)
{
	struct tg_token t;
	int eni;

	tg_gettokentype(&t, TG_T_RETURN);

	if (tg_peektoken(&t) != TG_T_SEMICOL) {
		eni = tg_nodeadd(ni, TG_N_EXPR, NULL);
		tg_expr(eni);
	}

	tg_gettokentype(&t, TG_T_SEMICOL);

	return 0;
}

int tg_stmt(int ni)
{
	struct tg_token t;
	int nni;

	switch (tg_peektoken(&t)) {
	case TG_T_FOR:
		nni = tg_nodeadd(ni, TG_N_FOR, NULL);
		tg_for(nni);
		break;

	case TG_T_IF:
		nni = tg_nodeadd(ni, TG_N_IF, NULL);
		tg_if(nni);
		break;

	case TG_T_RETURN:
		nni = tg_nodeadd(ni, TG_N_RETURN, NULL);
		tg_return(nni);
		break;

	case TG_T_BREAK:
		tg_nodeadd(ni, TG_T_BREAK, &t);
		break;

	case TG_T_CONTINUE:
		tg_nodeadd(ni, TG_T_CONTINUE, &t);
		break;

	default:
		nni = tg_nodeadd(ni, TG_N_EXPR, NULL);
		tg_expr(nni);
		tg_gettokentype(&t, TG_T_SEMICOL);
		break;
	}

	return 0;
}

struct tg_node *tg_template(const char *p)
{
	struct tg_token t;
	int type;
	int ni;
	
	tg_initparser(p);
	
	tg_darrinit(&nodes, sizeof(struct tg_node)); // !!!

	ni = tg_nodeadd(-1, TG_N_TEMPLATE, NULL);

	while ((type = tg_peektoken(&t)) != TG_T_EOF
		&& type != TG_T_ERROR) {
		if (t.type == TG_T_FUNCTION)
			tg_funcdef(ni);
		else
			tg_stmt(ni);	
	}

	tg_printnode(ni, 0);


/*
	int ni;

	ni = node_add(-1, TG_N_TEMPLATE, NULL);
	
	struct tg_token t;

	tg_gettoken(&t);

	node_add(ni, TG_T_FLOAT, &t);
	node_add(ni, TG_T_INT, &t);
	node_add(ni, TG_T_STRING, &t);
	
	tg_printnode(ni, 0);
*/
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
