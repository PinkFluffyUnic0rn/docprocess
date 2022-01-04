#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "tg_parser.h"
#include "tg_dstring.h"

#define BUFSZ 32

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

static char **strbufs;
static size_t strbufscnt;
static size_t strbufsmax;
static char *strbufend;

static int strinitbuf()
{
	if ((strbufs = malloc(sizeof(char *))) == NULL)
		return (-1);

	if ((strbufs[0] = malloc(BUFSZ)) == NULL)
		return (-1);

	strbufend = strbufs[0];
	strbufscnt = 1;
	strbufsmax = 1;

	return 0;
}

static char *strcreate(const char *src, size_t len)
{
	char *p;

	if (strbufs[strbufscnt - 1] + BUFSZ - strbufend <= len + 1) {
		if (strbufscnt >= strbufsmax) {
			strbufsmax *= 2;
			if ((strbufs = realloc(strbufs,
				sizeof(char *) * strbufsmax)) == NULL)
				return NULL;
		}
		
		if ((strbufs[strbufscnt++] = malloc(
			len + 1 > BUFSZ ? len + 1 : BUFSZ)) == NULL)
			return NULL;

		strbufend = strbufs[strbufscnt - 1];
	}

	if (src != NULL) {
		strncpy(strbufend, src, len);
		strbufend[len] = '\0';
	}

	p = strbufend;

	strbufend += len + 1;

	return p;
}

static void strclearall()
{
	int i;

	for (i = 0; i < strbufscnt; ++i)
		free(strbufs[i]);

	free(strbufs);
}

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
	
	printf("start l = |%s|\n", l);
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

	// put token separated token value somewhere

	char *v;
	if (l[0] == '\"') {
		v = ++l;
		while (l[0] != '\"') {	
			if (l[0] == '\\') {
				size_t lsz;
				ssize_t r;
				
				l++;

				switch (l[0]) {
				case '\n':
					lsz = 0;
					if ((r = getline(text + curline,
						&lsz, f)) < 0)
						goto error;

					l = text[curline];
					
					l[strlen(l) - 1] = '\0'; //!

					++curline;
					printf("\t\t\tl = |%s|\n", l);
					break;

				case 'n':
					break;

				case 'r':
					break;

				case 't':
					break;
				}
			}

			++l;
		}

	 	t.type = TG_T_STRING;
	}
	else if (isalpha(l[0] | l[0] == '_')) {
		v = l;
		while (isalpha(l[0]) | isdigit(l[0]) | l[0] == '_')
			++l;
	//	if (text[curline] == "global")
	//		...
	//	else
	// 		t.type = TG_T_ID
	
		t.type = TG_T_ID;
	}
	else if (l[0] == '=') {
		v = l;
	
		t.type = TG_T_ASSIGN;
		++l;	
		
		if (l[0] == '=') {
			t.type = TG_T_RELOP;
			++l;
		}
	}
	else if (l[0] == '<') {
		v = l;
	
		t.type = TG_T_RELOP;
		++l;
		
		if (l[0] == '-') {
			t.type = TG_T_NEXTTOOP;
			++l;
		}

	}

	// }
	// else if (text[curline] == 0-9)
	// 	t.type = TG_T_INT
	// else if strtod(text[curline])
	// 	t.type == TG_T_DOUBLE
	// else if text[curline] == "==" | "!=" | ">=" | "<=" | ">" | "<"
	// 	t.type = TG_T_RELOP
	// else
	// 	...

	*l++ = '\0';
	
	printf("end l = |%s|\n", l);
	printf("v = |%s|\n\n\n\n", v);

	t.val = v;
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
	buftest();
	
	return NULL;
	
	
	path = p;

	strinitbuf();
	

/*
	printf("%s\n", strcreate("fdsh2hds", strlen("fdsh2hds")));
	printf("%s\n", strcreate("fd212hds", strlen("fd212hds")));
	printf("%s\n", strcreate("fdsh24212hds", strlen("fdsh24212hds")));
	printf("%s\n", strcreate("fds", strlen("fds")));
	printf("%s\n", strcreate("fdhds", strlen("fdhds")));

	printf("%s\n", strcreate("fdsh2hds", strlen("fdsh2hds")));
	printf("%s\n", strcreate("fd212hds", strlen("fd212hds")));
	printf("%s\n", strcreate("fdsh24212hds", strlen("fdsh24212hds")));
	printf("%s\n", strcreate("fds", strlen("fds")));
	printf("%s\n", strcreate("fdhds", strlen("fdhds")));

	printf("%s\n", strcreate("fdsh2hds", strlen("fdsh2hds")));
	printf("%s\n", strcreate("fd212hds", strlen("fd212hds")));
	printf("%s\n", strcreate("fdsh24212hds", strlen("fdsh24212hds")));
	printf("%s\n", strcreate("fds", strlen("fds")));
	printf("%s\n", strcreate("fdhds", strlen("fdhds")));
*/
	
	int i;
	char *strs[1024];

	for (i = 0; i < 1024; ++i) {
		char s[256];

		sprintf(s, "%d", i);

		strs[i] = strcreate(s, strlen(s));
		printf("|%s| ", strs[i]);
	}

	printf("\n\n");

	for (i = 0; i < 1024; ++i) {
		if (i % 32 == 0)
			printf("\n");

	//	printf("|%s| ", strs[i]);
	}
	
	strclearall();

	text = malloc(sizeof(char *) * 1024);

	int c = 0;
	while (tg_nexttoken() != NULL) {
		
		if (++c == 6)
			break;

	}
}
