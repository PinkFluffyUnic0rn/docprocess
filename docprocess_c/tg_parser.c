#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "tg_parser.h"
#include "tg_dstring.h"

const char *tg_tstrsym[] = {
	"identificator", "integer", "float", "relational operator",
	"{", "}", "&", "|", "!", "$", "~", ",", ":", ";", "..",
	"multiplication or division", "addition or substraction",
	"quoted string", "error", "EOF"
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

static const char *path;
static char **text;

static struct tg_node *nodes;
static int nodescount;
static int maxnodes;

int _tg_getc(char *c, int rawness)
{
	static FILE *f;
	static char *l = "";
	static int curline = 0;

	if (path != NULL) {
		if ((f = fopen(path, "r")) == NULL)
			goto error;

		curline = 0;
		path = NULL;
	}
/*
	if (*l != '\0') {
		*c = *l++;
		printf("_getting: %c\n", *c);
		return 0;
	}
*/
	while (1) {
		size_t lsz;
		ssize_t r;

		if (!rawness) {
			while (l[0] == ' ' || l[0] == '\t')
				++l;
			
			if (l[0] == '#')
				l = "";
		}

		if (l[0] != '\0')
			break;

		lsz = 0;
		if ((r = getline(text + curline, &lsz, f)) < 0)
			goto error;
		else if (r == 0) {
			// EOF
		}
		
		l = text[curline];
		l[strlen(l) - 1] = '\0'; //!
		++curline;
	}
	
	*c = *l++;

//	printf("getting: %c\n", *c);
	return 0;

error:
	return (-1);
}

static char curc;
static int curr;
static char nextc;
static int nextr;
static int getcinit = 0;

int tg_getc(char *c)
{
	int r;

	if (getcinit == 0) {
		nextr = _tg_getc(&nextc, 0);
		getcinit = 1;
	}

// printf("get | cur: %c, next: |%c|\n", curc, nextc);

	curr = nextr;
	curc = nextc;

	r = curr;
	*c = curc;

	nextr = _tg_getc(&nextc, 0);

// printf("get | cur: %c, next: %c\n\n", curc, nextc);

	return r;
}

int tg_getcraw(char *c)
{
	int r;

	if (getcinit == 0) {
		nextr = _tg_getc(&nextc, 1);
		getcinit = 1;
	}

	curr = nextr;
	curc = nextc;

	r = curr;
	*c = curc;

	nextr = _tg_getc(&nextc, 1);

	return r;
}

int tg_getcrestore()
{
	char c;

	// do something with comments
	if (nextc == ' ' || nextc == '\t')
		return tg_getc(&c);
}

int tg_peekc(char *c)
{
	if (getcinit == 0) {
		nextr = _tg_getc(&nextc, 0);
		getcinit = 1;
	}
//	printf("peek | cur: %c, next: |%c|\n", curc, nextc);

	*c = nextc;


	return nextr;
}

// do something with self-reference in dstring
struct tg_token *tg_nexttoken()
{
	static struct tg_token t;

	struct tg_dstring dstr;
	char c;

	if (tg_dstrcreate(&dstr, "") < 0)
		goto error;
	
	tg_getc(&c);
	
	if (c == '\"') {
		tg_getcraw(&c);
		while (c != '\"') {
			if (c == '\\') {
				size_t lsz;
				ssize_t r;
			
				tg_getcraw(&c);
					
				switch (c) {
				case 'n':
					tg_dstraddstr(&dstr, "\n");
					break;

				case 'r':
					tg_dstraddstr(&dstr, "\r");
					break;

				case 't':
					tg_dstraddstr(&dstr, "\t");
					break;
				
				default:
					tg_dstraddstrn(&dstr, &c, 1);
					break;
				}
				
				tg_getcraw(&c);
			}
			
			tg_dstraddstrn(&dstr, &c, 1);
			tg_getcraw(&c);
		}
	 	
		tg_getcrestore();
		t.type = TG_T_STRING;
	}
	else if (isalpha(c) || c == '_') {
			
		tg_dstraddstrn(&dstr, &c, 1);

		// whitespaces matters for adjacent id and keyword!!!
		while (1) {
			tg_peekc(&c);
			
			if (!isalpha(c) && !isdigit(c) && c != '_')
				break;
		
			tg_dstraddstrn(&dstr, &c, 1);
			tg_getc(&c);
		}

		tg_getcrestore();

		if (strcmp(dstr.str, "global") == 0)
			t.type = TG_T_GLOBAL;
		else if (strcmp(dstr.str, "function") == 0)
			t.type = TG_T_FUNCTION;
		else if (strcmp(dstr.str, "for") == 0)
			t.type = TG_T_FOR;
		else if (strcmp(dstr.str, "if") == 0)
			t.type = TG_T_IF;
		else if (strcmp(dstr.str, "else") == 0)
			t.type = TG_T_ELSE;
		else if (strcmp(dstr.str, "continue") == 0)
			t.type = TG_T_CONTINUE;
		else if (strcmp(dstr.str, "break") == 0)
			t.type = TG_T_BREAK;
		else if (strcmp(dstr.str, "return") == 0)
			t.type = TG_T_RETURN;
		else if (strcmp(dstr.str, "in") == 0)
			t.type = TG_T_IN;
		else
	 		t.type = TG_T_ID;
	}
	else if (isdigit(c)) {
		// number
	}
	else if (c == '=') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_ASSIGN;
		
		tg_peekc(&c);
		if (c == '=') {
			tg_dstraddstrn(&dstr, &c, 1);
			
			t.type = TG_T_RELOP;
			tg_getc(&c);
		}
	}
	else if (c == '!') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_NOT;
	
		tg_peekc(&c);
		if (c == '=') {
			tg_dstraddstrn(&dstr, &c, 1);
			
			t.type = TG_T_RELOP;
			tg_getc(&c);
		}
	}
	else if (c == '>') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_RELOP;
		
		tg_peekc(&c);
		if (c == '=') {
			tg_dstraddstrn(&dstr, &c, 1);
		
			t.type = TG_T_RELOP;
			tg_getc(&c);
		}

	}
	else if (c == '<') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_RELOP;

		tg_peekc(&c);
		if (c == '=') {
			tg_dstraddstrn(&dstr, &c, 1);
		
			t.type = TG_T_RELOP;
			tg_getc(&c);
		}
		else if (c == '-') {
			tg_dstraddstrn(&dstr, &c, 1);
		
			t.type = TG_T_NEXTTOOP;
			tg_getc(&c);
		}
	}
	else if (c == '^') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_NEXTTOOP;	
	}
	else if (c == '(') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_LPAR;
	}
	else if (c == ')') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_RPAR;
	}
	else if (c == '&') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_AND;
	}
	else if (c == '|') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_OR;
	}
	else if (c == '$') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_DOL;
	}
	else if (c == '~') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_TILDA;
	}
	else if (c == '.') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_DOT;
	
		tg_peekc(&c);
		if (c == '.') {
			tg_dstraddstrn(&dstr, &c, 1);
			t.type = TG_T_DDOT;
		}

	}
	else if (c == ',') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_COMMA;
	}
	else if (c == ':') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_COLON;
	}
	else if (c == ';') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_SEMICOL;
	}
	else if (c == '*' || c == '/') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_MULTOP;
	}
	else if (c == '-' || c == '+') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_ADDOP;
	}
	else if (c == '{') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_LBRC;
	}
	else if (c == '}') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_RBRC;
	}
	else if (c == '[') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_LBRK;
	}
	else if (c == ']') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_RBRK;
	}
	else if (c == '?') {
		tg_dstraddstrn(&dstr, &c, 1);
		t.type = TG_T_QUEST;
	}

	printf("|%s|\n", dstr.str);

	// }
	// else if (text[curline] == 0-9)
	// 	t.type = TG_T_INT
	// else if strtod(text[curline])
	// 	t.type == TG_T_DOUBLE
	// else if text[curline] == "==" | "!=" | ">=" | "<=" | ">" | "<"
	// 	t.type = TG_T_RELOP
	// else
	// 	...

	t.val = dstr.str;
//	t.line = curline;

	return &t;

error:
	return NULL;

	// t.val = [error text]
	// t.type = TG_T_ERROR
	// t.line = curline
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

static struct tg_node *node_add(struct tg_node *parent,
	enum TG_N_TYPE type, struct tg_token *token)
{

}

int node_remove(struct tg_node *n)
{

}

struct tg_node *tg_template(const char *p)
{
	text = malloc(sizeof(char *) * 1024);
	path = p;

	int c = 0;
	while (tg_nexttoken() != NULL) {
		if (++c == 50)
			break;


		//printf("token value: %s\n", t.val);
	}
}
