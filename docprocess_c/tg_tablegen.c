#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "tg_parser.h"
#include "tg_value.h"
#include "tg_tests.h"

#define CONVERTERS_PATH "/home/qwerty/docprocess/docprocess_c/converters"

// symbol table API
enum TG_SYMTYPE {
	TG_SYMTYPE_FUNC,
	TG_SYMTYPE_VARIABLE,
	TG_SYMTYPE_INPUT
};

struct tg_variable {
	struct tg_val *val;
};

struct tg_function {
	int argcount;
};

struct tg_input {
	char *type;
	char *path;
};

struct tg_symbol {
	union {
		struct tg_variable var;
		struct tg_function func;
		struct tg_input input;
	};

	enum TG_SYMTYPE type;

	TG_HASHFIELDS(struct tg_symbol, TG_HASH_SYMBOL)
};

TG_HASHED(struct tg_symbol, TG_HASH_SYMBOL)

struct tg_allocator symalloc;

struct tg_hash symtable;

struct tg_val *csv_to_table(int f)
{
	return NULL;
}

struct tg_val *readsource(struct tg_input *in,
	struct tg_val *argv, int argc)
{
	char conv[PATH_MAX];
	FILE *f;
	int pid;
	int p[2];

	snprintf(conv, PATH_MAX, "%s/in_%s", CONVERTERS_PATH, in->type);

	if ((f = fopen(conv, "r")) != NULL) {
		fprintf(stderr, "No converter for input type %s.",
			in->type);
		return tg_createval(TG_VAL_EMPTY);
	}

	if (pipe(p) < 0) {
		fprintf(stderr, strerror(errno));
		fclose(f);
		return tg_createval(TG_VAL_EMPTY);
	}

	if ((pid = fork()) < 0) {
		fprintf(stderr, strerror(errno));
		fclose(f);
		return tg_createval(TG_VAL_EMPTY);
	}

	if (pid == 0) {
		char **args;
		int i;
		
		close(p[0]);

		TG_ASSERT(args = malloc(sizeof(char **) * (argc + 3)),
			"Allocation error.");

		// argv -- array?
		for (i = 0; i < argc; ++i) {
			struct tg_val *sv;

			sv = tg_castval(argv + i + 2, TG_VAL_STRING);

			TG_ASSERT(sv != NULL, tg_error);

			args[i] = sv->strval.str;
		}

		args[0] = conv;

		args[1] = malloc(PATH_MAX);
		strcpy(args[1], in->path);

		args[argc + 1] = NULL;

		TG_ASSERT(execv(conv, args) >= 0, strerror(errno));
	}

	close(p[1]);
	fclose(f);

	return csv_to_table(p[1]);
}

void tg_symboladd(const char *name, struct tg_symbol *s)
{
	tg_hashset(TG_HASH_SYMBOL, &symtable, name, s);
}

void *tg_symbolget(const char *name)
{
	return tg_hashget(TG_HASH_SYMBOL, &symtable, name);
}

struct tg_val *tg_symbolgetval(const char *name)
{
	struct tg_symbol *sym;

	if ((sym = tg_hashget(TG_HASH_SYMBOL, &symtable, name)) == NULL)
		return NULL;

	if (sym->type == TG_SYMTYPE_VARIABLE)
		return sym->var.val;
	if (sym->type == TG_SYMTYPE_INPUT)
		return readsource(&(sym->input), NULL, 1);
	else if (sym->type == TG_SYMTYPE_FUNC) {
		TG_WARNING("Cannot convert function %s to value.",
			name);
		return NULL;
	}
		
	TG_WARNING("Symbol %s has unknown type.", name);

	return NULL;
}

// symbol call function

void tg_inputdestroy(struct tg_input *in)
{
	free(in->type);
	free(in->path);
}

void tg_printsymbols()
{
	const char *key;
	struct tg_symbol *s;

	TG_HASHFOREACH(struct tg_symbol, TG_HASH_SYMBOL, symtable, key,
		s = tg_hashget(TG_HASH_SYMBOL, &symtable, key);

		if (s->type == TG_SYMTYPE_FUNC) {
			printf("function\n");
		}
		else if (s->type == TG_SYMTYPE_VARIABLE) {
			printf("variable\n");
		}
		if (s->type == TG_SYMTYPE_INPUT) {
			printf("input{type:\"%s\"; path:\"%s\"}\n",
				s->input.type, s->input.path);
		}
	);
}

int tg_readsourceslist(const char *sources)
{
	char *s, *ss;
	char *p;

	s = tg_strdup(sources);

	TG_ASSERT((ss = malloc(strlen(sources))) != NULL,
		"Error while allocating memory.");

	p = strtok(s, ";");
	do {
		const char *name, *type, *path;
		struct tg_symbol *in;
		char *pp;

		strcpy(ss, p);
		pp = ss;

		name = pp;
		TG_ASSERT((pp = strchr(pp, ':')) != NULL,
			"Error while reading sources list.");
		*pp++ = '\0';

		type = pp;
		TG_ASSERT((pp = strchr(pp + 1, ':')) != NULL,
			"Error while reading sources list.");
		*pp++ = '\0';

		path = pp;

		in = tg_alloc(&symalloc);
		in->type = TG_SYMTYPE_INPUT;
		in->input.type = tg_strdup(type);
		in->input.path = tg_strdup(path);

		tg_symboladd(name, in);

	} while ((p = strtok(NULL, ";")) != NULL);

	free(s);
	free(ss);

	return 0;
}

struct tg_val *run_node(int ni);

struct tg_val *tg_funcdef(int n)
{

	return NULL;
}

struct tg_val *tg_if(int ni)
{
	return NULL;
}

struct tg_val *tg_for(int ni)
{

	return NULL;
}

struct tg_val *tg_block(int ni)
{

	return NULL;
}

struct tg_val *tg_identificator(int ni)
{
	// this and tg_const only TG_N_* that can have a token attached
	return tg_symbolgetval(tg_nodegettoken(ni)->val.str);
}

struct tg_val *tg_const(int ni)
{
	struct tg_token *t;

	t = tg_nodegettoken(ni);

	if (t->type == TG_T_STRING)
		return tg_stringval(t->val.str);
	else if (t->type == TG_T_INT)
		return tg_intval(atoi(t->val.str));
	else if (t->type == TG_T_FLOAT)
		return tg_intval(atof(t->val.str));

	TG_ERROR("Unknown constant value type: %d", t->type);
}

struct tg_val *tg_address(int ni)
{
	return NULL;
}

struct tg_val *tg_prestep(int ni)
{
	struct tg_val *r;
	
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));
	
	return tg_valadd(r, tg_intval(1));
}

struct tg_val *tg_sign(int ni)
{
	return NULL;
}

struct tg_val *tg_not(int ni)
{
	struct tg_val *r;
	
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));
	
	if (tg_istrueval(r))
		return tg_intval(0);
	else
		return tg_intval(1);
}

struct tg_val *tg_ref(int ni)
{
	return NULL;
}

struct tg_val *tg_mult(int ni)
{
	int i;
	struct tg_val *r;

	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); i += 2) {
		struct tg_val *v;
		const char *s;
	
		s = tg_nodegettoken(tg_nodegetchild(ni, i))->val.str;
		TG_NULLQUIT(v = run_node(tg_nodegetchild(ni, i + 1)));
		
		if (strcmp(s, "*") == 0)
			TG_NULLQUIT(r = tg_valmult(r, v));
		else if (strcmp(s, "/") == 0)
			TG_NULLQUIT(r = tg_valdiv(r, v));
	}

	return r;
}

struct tg_val *tg_add(int ni)
{
	int i;
	struct tg_val *r;

	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); i += 2) {
		struct tg_val *v;
		const char *s;
	
		s = tg_nodegettoken(tg_nodegetchild(ni, i))->val.str;
		TG_NULLQUIT(v = run_node(tg_nodegetchild(ni, i + 1)));
		
		if (strcmp(s, "+") == 0)
			TG_NULLQUIT(r = tg_valadd(r, v));
		else if (strcmp(s, "-") == 0)
			TG_NULLQUIT(r = tg_valsub(r, v));
	}

	return r;
}

struct tg_val *tg_cat(int ni)
{
	int i;
	struct tg_val *r;

	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *v;
	
		TG_NULLQUIT(v = run_node(tg_nodegetchild(ni, i)));
		
		// print cast error?	
		TG_NULLQUIT(r = tg_valcat(r, v));
	}

	return r;
}

struct tg_val *tg_nextto(int ni)
{
	int i;
	struct tg_val *r;

	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); i += 2) {
		struct tg_val *v;
		const char *s;
		int span;
	
		s = tg_nodegettoken(tg_nodegetchild(ni, i))->val.str;
		TG_NULLQUIT(v = run_node(tg_nodegetchild(ni, i + 1)));

		span = 0;
		if (tg_nodegettype(tg_nodegetchild(ni, i))
				== TG_N_NEXTTOOPTS) {
			span = 1;
			++i;
		}
		
		if (strcmp(s, "->") == 0)
			TG_NULLQUIT(r = tg_valnextto(r, v, 0, span));
		else if (strcmp(s, "^") == 0)
			TG_NULLQUIT(r = tg_valnextto(r, v, 1, span));
	}

	return r;
}

struct tg_val *tg_rel(int ni)
{
	struct tg_val *r1, *r2;
	const char *s;

	TG_NULLQUIT(r1 = run_node(tg_nodegetchild(ni, 0)));
	TG_NULLQUIT(r2 = run_node(tg_nodegetchild(ni, 2)));

	s = tg_nodegettoken(tg_nodegetchild(ni, 2))->val.str;

	if (strcmp(s, "=") == 0)
		return tg_valeq(r1, r2);
	else if (strcmp(s, "!=") == 0)
		return tg_valneq(r1, r2);
	else if (strcmp(s, "<") == 0)
		return tg_valls(r1, r2);
	else if (strcmp(s, ">") == 0)
		return tg_valgr(r1, r2);
	else if (strcmp(s, "<=") == 0)
		return tg_vallseq(r1, r2);
	else if (strcmp(s, ">=") == 0)
		return tg_valgreq(r1, r2);

	TG_WARNING("Unknown relation operator: %s", s);

	return NULL;
}

struct tg_val *tg_and(int ni)
{
	int i;

	for (i = 0; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *r;
		
		TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, i)));
		if (!tg_istrueval(r))
			return tg_intval(0);
	}

	return tg_intval(1);
}

struct tg_val *tg_or(int ni)
{
	int i;

	for (i = 0; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *r;
		
		TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, i)));
		if (tg_istrueval(r))
			return tg_intval(1);
	}

	return tg_intval(0);
}

struct tg_val *tg_ternary(int ni)
{
	struct tg_val *r;
		
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	if (tg_istrueval(r))
		return run_node(tg_nodegetchild(ni, 1));
	else
		return run_node(tg_nodegetchild(ni, 2));
}

struct tg_val *tg_setlvalue(int ni, struct tg_val *v)
{
	struct tg_symbol *s;

	if (tg_nodegettype(ni) != TG_N_ID) {
		TG_WARNING("Trying to assign into rvalue.");
		return NULL;
	}

	s = tg_alloc(&symalloc);
	s->type = TG_SYMTYPE_VARIABLE;
	s->var.val = v;

	tg_symboladd(tg_nodegettoken(ni)->val.str, s);

	return tg_createval(TG_VAL_EMPTY);
}

struct tg_val *tg_assign(int ni)
{
	int i;
	struct tg_val *r;
		
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	for (i = tg_nodeccnt(ni) - 1; i == 0; --i)
		TG_NULLQUIT(tg_setlvalue(tg_nodegetchild(ni, i), r));

	return r;
}

struct tg_val *tg_expr(int ni)
{
	int i;
	struct tg_val *r;
		
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); ++i)
		TG_NULLQUIT(run_node(tg_nodegetchild(ni, i)));

	return r;
}

struct tg_val *tg_template(int ni)
{
	int i;

	for (i = 0; i < tg_nodeccnt(ni); ++i)
		TG_NULLQUIT(run_node(tg_nodegetchild(ni, i)));

	return 0;
}

struct tg_val *tg_unexpected(int ni)
{
	TG_WARNING("Unexpected node type: %s",
		tg_strsym[tg_nodegettype(ni)]);

	return NULL;
}

struct tg_val *(* node_handler[])(int) = {
	tg_template,	tg_funcdef,	tg_unexpected,
	tg_unexpected, 	tg_unexpected,	tg_if,
	tg_for,		tg_unexpected,	tg_unexpected,
	tg_unexpected,	tg_block,	tg_expr,
	tg_assign,	tg_ternary,	tg_or,
	tg_and,		tg_rel,		tg_nextto,
	tg_unexpected,	tg_cat,		tg_add,
	tg_mult,	tg_unexpected,	tg_ref,
	tg_not,		tg_sign, 	tg_prestep,
	tg_address,	tg_unexpected,	tg_identificator,
	tg_const,	tg_unexpected,	tg_unexpected,
	tg_unexpected,	tg_unexpected,	tg_unexpected
};

struct tg_val *run_node(int ni)
{
	int t;

	t = tg_nodegettype(ni) - TG_N_TEMPLATE;

	if (t < 0 || t > TG_N_ATTR)
		return tg_unexpected(ni);
	
	return node_handler[t](ni);
}

int main()
{
	int tpl;
	
	tg_allocinit(&symalloc, sizeof(struct tg_symbol)); 
	
	tg_inithash(TG_HASH_SYMBOL, &symtable);

	tg_startframe();

	tg_readsourceslist(
		"test1:csv:./test1.csv;test2:script:./test2.sh");

	tpl = tg_getparsetree("test.txt");

	tg_printnode(tpl, 0);

	tg_template(tpl);

//	tg_printsymbols();
//	tg_testvalues();


	tg_endframe();

	tg_destroyhash(TG_HASH_SYMBOL, &symtable);
	tg_allocdestroy(&symalloc, NULL); 

	return 0;
}
