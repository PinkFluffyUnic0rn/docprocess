#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "tg_parser.h"
#include "tg_dstring.h"

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
static char **tg_text;
static int tg_curline;

static int curc;
static int nextc;
static int getcinit = 0;

static struct tg_node *nodes;
static int nodescount;
static int maxnodes;

// string stream to char stream
// redo getc interface, character should be returned as int
// like in usual getc

static int _tg_getc(int raw)
{
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
		if ((r = getline(tg_text + tg_curline, &lsz, f)) < 0)
			goto error;
		else if (r == 0) {
			return (-1);
		}
		
		l = tg_text[tg_curline++];
	}
	
	return (*l++);

error:
	return EOF;
}

// count lines
static int tg_getc()
{
	int c;

	if (getcinit == 0) {
		nextc = _tg_getc(0);
		getcinit = 1;
	}

	curc = nextc;

	c = curc;

	nextc = _tg_getc(0);

	return c;
}

static int tg_getcraw()
{
	int c;

	if (getcinit == 0) {
		nextc = _tg_getc(1);
		getcinit = 1;
	}

	curc = nextc;

	c = curc;

	nextc = _tg_getc(1);

	return c;
}

static int tg_getcrestore()
{
	// do something with comments
	if (isspace(nextc))
		nextc = ' ';

	return 0;
}

static int tg_peekc()
{
	if (getcinit == 0) {
		nextc = _tg_getc(0);
		getcinit = 1;
	}

	return nextc;
}

// lexical analyzer
int tg_nexttoken(struct tg_token *t)
{
	struct tg_dstring dstr;
	int c;

	if (tg_dstrcreate(&dstr, "") < 0)
		goto error;

	while ((tg_peekc()) == ' ')
		tg_getc();

	if ((c = tg_getc()) == EOF)
		goto error;

	if (c == '\"') {
		c = tg_getcraw();
		while (c != '\"') {
			if (c == '\\') {
				size_t lsz;
				ssize_t r;

				c = tg_getcraw();
				if (c == EOF)
					goto error;
				else if (c == '\n') {
				}
				else if (c == 'n')
					tg_dstraddstr(&dstr, "\n");
				else if (c == 'r')
					tg_dstraddstr(&dstr, "\r");
				else if (c == 't')
					tg_dstraddstr(&dstr, "\t");
				else if (c == '0')
					tg_dstraddstr(&dstr, "\0");
				else {
					tg_dstraddstrn(&dstr,
						(char *) &c, 1);
				}

				c = tg_getcraw();
			}
			else if (c == EOF) {
				goto error;
			}
			
			tg_dstraddstrn(&dstr, (char *) &c, 1);
			c = tg_getcraw();
		}
	 	
		tg_getcrestore();
		t->type = TG_T_STRING;
	}
	else if (isalpha(c) || c == '_') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);

		while (1) {
			c = tg_peekc();
			
			if (!isalpha(c) && !isdigit(c) && c != '_')
				break;
		
			tg_dstraddstrn(&dstr, (char *) &c, 1);
			c = tg_getc();
		}

		tg_getcrestore();

		if (strcmp(dstr.str, "global") == 0)
			t->type = TG_T_GLOBAL;
		else if (strcmp(dstr.str, "function") == 0)
			t->type = TG_T_FUNCTION;
		else if (strcmp(dstr.str, "for") == 0)
			t->type = TG_T_FOR;
		else if (strcmp(dstr.str, "if") == 0)
			t->type = TG_T_IF;
		else if (strcmp(dstr.str, "else") == 0)
			t->type = TG_T_ELSE;
		else if (strcmp(dstr.str, "continue") == 0)
			t->type = TG_T_CONTINUE;
		else if (strcmp(dstr.str, "break") == 0)
			t->type = TG_T_BREAK;
		else if (strcmp(dstr.str, "return") == 0)
			t->type = TG_T_RETURN;
		else if (strcmp(dstr.str, "in") == 0)
			t->type = TG_T_IN;
		else
	 		t->type = TG_T_ID;
	}
	else if (isdigit(c)) {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
	 	t->type = TG_T_INT;

		while (1) {
			c = tg_peekc();
			
			if (!isdigit(c))
				break;
		
			tg_dstraddstrn(&dstr, (char *) &c, 1);
			c = tg_getc();
		}

		c = tg_peekc();
		if (c == '.') {
			c = tg_getc();
			
			tg_dstraddstrn(&dstr, (char *) &c, 1);
	 		t->type = TG_T_FLOAT;
		
			while (1) {
				c = tg_peekc();
				
				if (!isdigit(c))
					break;
			
				tg_dstraddstrn(&dstr, (char *) &c, 1);
				c = tg_getc();
			}
		}

		c = tg_peekc();
		if (c == 'e' || c == 'E') {
			c = tg_getc();
		
			tg_dstraddstrn(&dstr, (char *) &c, 1);
	 		t->type = TG_T_FLOAT;
		
			while (1) {
				c = tg_peekc();
				
				if (!isdigit(c))
					break;
			
				tg_dstraddstrn(&dstr, (char *) &c, 1);
				c = tg_getc();
			}
		}
	}
	else if (c == '=') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_ASSIGN;
		
		c = tg_peekc();
		if (c == '=') {
			tg_dstraddstrn(&dstr, (char *) &c, 1);
			
			t->type = TG_T_RELOP;
			c = tg_getc();
		}
	}
	else if (c == '!') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_NOT;
	
		c = tg_peekc();
		if (c == '=') {
			tg_dstraddstrn(&dstr, (char *) &c, 1);
			
			t->type = TG_T_RELOP;
			c = tg_getc();
		}
	}
	else if (c == '>') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_RELOP;
		
		c = tg_peekc();
		if (c == '=') {
			tg_dstraddstrn(&dstr, (char *) &c, 1);
		
			t->type = TG_T_RELOP;
			c = tg_getc();
		}

	}
	else if (c == '<') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_RELOP;

		c = tg_peekc();
		if (c == '=') {
			tg_dstraddstrn(&dstr, (char *) &c, 1);
		
			t->type = TG_T_RELOP;
			c = tg_getc();
		}
		else if (c == '-') {
			tg_dstraddstrn(&dstr, (char *) &c, 1);
		
			t->type = TG_T_NEXTTOOP;
			c = tg_getc();
		}
	}
	else if (c == '^') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_NEXTTOOP;	
	}
	else if (c == '(') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_LPAR;
	}
	else if (c == ')') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_RPAR;
	}
	else if (c == '&') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_AND;
	}
	else if (c == '|') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_OR;
	}
	else if (c == '$') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_DOL;
	}
	else if (c == '~') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_TILDA;
	}
	else if (c == '.') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_DOT;
	
		tg_peekc(&c);
		if (c == '.') {
			tg_dstraddstrn(&dstr, (char *) &c, 1);
			t->type = TG_T_DDOT;
		}
	}
	else if (c == ',') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_COMMA;
	}
	else if (c == ':') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_COLON;
	}
	else if (c == ';') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_SEMICOL;
	}
	else if (c == '*' || c == '/') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_MULTOP;
	}
	else if (c == '-' || c == '+') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_ADDOP;
	}
	else if (c == '{') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_LBRC;
	}
	else if (c == '}') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_RBRC;
	}
	else if (c == '[') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_LBRK;
	}
	else if (c == ']') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_RBRK;
	}
	else if (c == '?') {
		tg_dstraddstrn(&dstr, (char *) &c, 1);
		t->type = TG_T_QUEST;
	}
	else {
		goto error;
	}

	t->val = dstr.str;
	t->line = tg_curline;
	
	return 0;

error:
	return (-1);
}

static struct tg_token *tg_gettoken()
{

}

static struct tg_token *tg_peektoken()
{

}

static struct tg_token *tg_gettokentype(enum TG_T_TYPE type)
{

}

// syntax analizer
static struct tg_node *node_add(struct tg_node *parent,
	enum TG_N_TYPE type, struct tg_token *token)
{

}

int node_remove(struct tg_node *n)
{

}

struct tg_node *tg_template(const char *p)
{
	tg_text = malloc(sizeof(char *) * 1024);
	tg_path = p;

	struct tg_token t;

	while (tg_nexttoken(&t) >= 0) {
		printf("token value: |%s|\ntoken type: |%s|\nline: %d\n",
			t.val, tg_tstrsym[t.type], t.line);
	}

	printf("HERE!\n");
}
