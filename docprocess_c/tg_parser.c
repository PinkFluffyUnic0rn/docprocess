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

// do something with self-reference in dstring
struct tg_token *tg_nexttoken()
{
	static FILE *f;
	static struct tg_token t;
	
	static char *l = "";
	static int curline = 0;


	if (path != NULL) {
		if ((f = fopen(path, "r")) == NULL)
			goto error;
		
		curline = 0;
		path = NULL;
	}
	
//	printf("start l = |%s|\n", l);
	while (1) {	
		size_t lsz;
		ssize_t r;

		while (l[0] == ' ' || l[0] == '\t')
			++l;

		if (l[0] == '#')
			l = "";

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
	
//	printf("end l = |%s|\n\n\n", l);

	// put token separated token value somewhere

	struct tg_dstring dstr;
	char *v;

	if (tg_dstrcreate(&dstr, "") < 0)
		goto error;
		
	if (l[0] == '\"') {	
		v = ++l;
		while (l[0] != '\"') {
			if (l[0] == '\\') {
				size_t lsz;
				ssize_t r;
			
				l++;
					
				switch (l[0]) {
				case '\0':
					lsz = 0;
					if ((r = getline(text + curline,
						&lsz, f)) < 0)
						goto error;
					
					l = text[curline];
					l[strlen(l) - 1] = '\0'; //!
					++curline;

					break;

				case 'n':
					l++;
					tg_dstraddstr(&dstr, "\n");
					break;

				case 'r':
					l++;
					tg_dstraddstr(&dstr, "\r");
					break;

				case 't':
					l++;
					tg_dstraddstr(&dstr, "\t");
					break;
				
				default:
					l++;
					tg_dstraddstrn(&dstr, l, 1);
					break;
				}
			}
			
			tg_dstraddstrn(&dstr, l++, 1);
		}	

	 	t.type = TG_T_STRING;
		l++;
	}
	else if (isalpha(l[0] | l[0] == '_')) {
		v = l;
		while (isalpha(l[0]) | isdigit(l[0]) | l[0] == '_')
			++l;
		
		tg_dstraddstrn(&dstr, v, l - v);
	
	//	if (text[curline] == "global")
	//		...
	//	else
	// 		t.type = TG_T_ID
	
		t.type = TG_T_ID;
	}
	else if (l[0] == '=') {
		tg_dstraddstrn(&dstr, l, 1);
	
		t.type = TG_T_ASSIGN;
		++l;	
		
		if (l[0] == '=') {
			tg_dstraddstrn(&dstr, l, 1);
			
			t.type = TG_T_RELOP;
			++l;
		}
	}
	else if (l[0] == '<') {
		tg_dstraddstrn(&dstr, l, 1);
		
		t.type = TG_T_RELOP;
		++l;
		
		if (l[0] == '-') {
			tg_dstraddstrn(&dstr, l, 1);
		
			t.type = TG_T_NEXTTOOP;
			++l;
		}

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
	t.line = curline;

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
		if (++c == 11)
			break;


		//printf("token value: %s\n", t.val);
	}
}
