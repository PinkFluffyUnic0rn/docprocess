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
	TG_SYMTYPE_FUNCTION,
	TG_SYMTYPE_VARIABLE,
	TG_SYMTYPE_INPUT
};

enum TG_STATE {
	TG_STATE_SUCCESS = 0,
	TG_STATE_RETURN = 1,
	TG_STATE_BREAK = 2,
	TG_STATE_CONTINUE = 3
};

struct tg_variable {
	struct tg_val *val;
};

struct tg_function {
	int startnode;
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
struct tg_darray symtable;

struct tg_allocator retalloc;
struct tg_val *retval;

struct tg_val *tg_valprinterr(struct tg_val *v)
{
	if (v == NULL)
		TG_WARNING("%s", tg_error);

	return v;
}

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

void tg_initsymtable()
{
	tg_darrinit(&symtable, sizeof(struct tg_hash));
	tg_allocinit(&symalloc, sizeof(struct tg_symbol)); 
}

void tg_newscope()
{
	struct tg_hash st;
	
	tg_inithash(TG_HASH_SYMBOL, &st);

	tg_darrpush(&symtable, &st); 
}

void tg_popscope()
{
	struct tg_hash st;
	
	if (tg_darrpop(&symtable, &st) >= 0)
		tg_destroyhash(TG_HASH_SYMBOL, &st);
}

void tg_destroysymtable()
{
	tg_darrclear(&symtable);
	tg_allocdestroy(&symalloc, NULL); 
}

void tg_symboladd(const char *name, struct tg_symbol *s)
{
	struct tg_hash *st;

	st = tg_darrget(&symtable, symtable.cnt - 1);

	tg_hashset(TG_HASH_SYMBOL, st, name, s);
}

struct tg_symbol *tg_symbolget(const char *name)
{
	struct tg_hash *st;
	int i;

	for (i = symtable.cnt - 1; i >= 0; --i) {
		struct tg_symbol *sym;
	
		st = tg_darrget(&symtable, i);
		if ((sym = tg_hashget(TG_HASH_SYMBOL, st, name)) != NULL)
			return sym;
	}

	return NULL;
}

struct tg_val *tg_symbolgetval(const char *name)
{
	struct tg_symbol *sym;

	if ((sym = tg_symbolget(name)) == NULL)
		return NULL;

	if (sym->type == TG_SYMTYPE_VARIABLE)
		return sym->var.val;
	if (sym->type == TG_SYMTYPE_INPUT)
		return readsource(&(sym->input), NULL, 1);
	else if (sym->type == TG_SYMTYPE_FUNCTION) {
		TG_WARNING("Cannot convert function %s to value.",
			name);
		return NULL;
	}
		
	TG_WARNING("Symbol %s has unknown type.", name);

	return NULL;
}

void tg_inputdestroy(struct tg_input *in)
{
	free(in->type);
	free(in->path);
}

void tg_printsymbols()
{
	const char *key;
	struct tg_symbol *s;
	struct tg_hash *st;

	st = tg_darrget(&symtable, symtable.cnt - 1);

	TG_HASHFOREACH(struct tg_symbol, TG_HASH_SYMBOL, *st, key,
		s = tg_hashget(TG_HASH_SYMBOL, st, key);

		printf("%s: ", key);
		if (s->type == TG_SYMTYPE_FUNCTION) {
			printf("function\n");
		}
		else if (s->type == TG_SYMTYPE_VARIABLE) {
			printf("variable{");
			tg_printval(stdout, tg_symbolgetval(key));
			printf("}\n");
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

static struct tg_val *run_node(int ni);

struct tg_val *tg_setlvalue(int ni, struct tg_val *v)
{
	struct tg_symbol *s;
	
	if (tg_nodegettype(ni) != TG_N_ID) {
		TG_WARNING("Trying to assign into rvalue.");
		return NULL;
	}

	ni = tg_nodegetchild(ni, 0);

	if ((s = tg_symbolget(tg_nodegettoken(ni)->val.str)) != NULL) {
		if (s->type != TG_SYMTYPE_VARIABLE) {
			TG_WARNING("Trying to re-assign non-variable.");
			return NULL;
		}

		s->var.val = v;
		
		return v;
	}

	s = tg_alloc(&symalloc);
	s->type = TG_SYMTYPE_VARIABLE;
	s->var.val = v;

	tg_symboladd(tg_nodegettoken(ni)->val.str, s);

	return v;
}

struct tg_val *tg_funcdef(int ni)
{
	struct tg_symbol *s;
	
	s = tg_alloc(&symalloc);
	s->type = TG_SYMTYPE_FUNCTION;
	s->func.startnode = ni;

	return tg_intval(TG_STATE_SUCCESS);
}

struct tg_val *tg_if(int ni)
{
	struct tg_val *r;
			
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));

	if (tg_istrueval(r))
		return run_node(tg_nodegetchild(ni, 1));
	else if (tg_nodeccnt(ni) > 2)
		return run_node(tg_nodegetchild(ni, 2));

	return tg_intval(TG_STATE_SUCCESS);
}

struct tg_val *tg_forclassic(int ni)
{
	struct tg_val *r;
	int initnode;
	int cmpnode;
	int stepnode;
	int blocknode;
		
	initnode = tg_nodegetchild(ni, 0);
	cmpnode = tg_nodegetchild(ni, 1);
	stepnode = tg_nodegetchild(ni, 2);
	blocknode = tg_nodegetchild(ni, 3);
	
	tg_newscope();

	TG_NULLQUIT(run_node(tg_nodegetchild(initnode, 0)));

	while (1) {
		TG_NULLQUIT(r = run_node(tg_nodegetchild(cmpnode, 0)));
		if (!tg_istrueval(r))
			break;

		TG_NULLQUIT(r = run_node(blocknode));
		if (r->intval == TG_STATE_RETURN)
			goto forend;
		else if (r->intval == TG_STATE_BREAK)
			break;

		TG_NULLQUIT(run_node(tg_nodegetchild(stepnode, 0)));
	}

	r = tg_intval(TG_STATE_SUCCESS);

forend:
	tg_popscope();

	return r;
}

struct tg_val *tg_for(int ni)
{
	if (tg_nodeccnt(ni) == 4
		&& tg_nodegettype(tg_nodegetchild(ni, 0)) == TG_N_FOREXPR
		&& tg_nodegettype(tg_nodegetchild(ni, 1)) == TG_N_FOREXPR
		&& tg_nodegettype(tg_nodegetchild(ni, 2)) == TG_N_FOREXPR) {
		return tg_forclassic(ni);
	}
	else
		return tg_intval(TG_STATE_SUCCESS); // TODO: iterate
						// through table
}

struct tg_val *tg_block(int ni)
{
	struct tg_val *r;
	int i;
	
	tg_newscope();

	for (i = 0; i < tg_nodeccnt(ni); ++i) {
		int cni;

		cni = tg_nodegetchild(ni, i);

		if (tg_nodegettype(cni) == TG_N_RETURN) {
			if (tg_nodeccnt(cni) > 0) {
				TG_NULLQUIT(retval = run_node(
					tg_nodegetchild(cni, 0)));
			}

			r = tg_intval(TG_STATE_RETURN);
			goto blockend;
		}
		else if (tg_nodegettype(cni) == TG_T_BREAK) {
			r = tg_intval(TG_STATE_BREAK);
			goto blockend;
		}
		else if (tg_nodegettype(cni) == TG_T_CONTINUE) {
			r = tg_intval(TG_STATE_CONTINUE);
			goto blockend;
		}

		TG_NULLQUIT(r = run_node(cni));
		if (tg_isflownode(tg_nodegettype(cni))
			&& r->intval != TG_STATE_SUCCESS) {
			goto blockend;
		}
	}

	r = tg_intval(TG_STATE_SUCCESS);

blockend:
	tg_printsymbols(); //!!!

	tg_popscope();
	
	return r;
}

struct tg_val *tg_identificator(int ni)
{
	return tg_symbolgetval(tg_nodegettoken(tg_nodegetchild(ni, 0))
		->val.str);
}

struct tg_val *tg_const(int ni)
{
	struct tg_token *t;
	
	t = tg_nodegettoken(tg_nodegetchild(ni, 0));

	if (t->type == TG_T_STRING)
		return tg_stringval(t->val.str);
	else if (t->type == TG_T_INT)
		return tg_intval(atoi(t->val.str));
	else if (t->type == TG_T_FLOAT)
		return tg_floatval(atof(t->val.str));

	TG_ERROR("Unknown constant value type: %d", t->type);
}

struct tg_val *tg_address(int ni)
{
	return NULL;
}

struct tg_val *tg_prestep(int ni)
{
	struct tg_val *r;
	const char *s;

	s = tg_nodegettoken(tg_nodegetchild(ni, 0))->val.str;
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 1)));

	if (strcmp(s, "++") == 0)
		TG_NULLQUIT(r = tg_valadd(r, tg_intval(1)));
	if (strcmp(s, "--") == 0)
		TG_NULLQUIT(r = tg_valsub(r, tg_intval(1)));
	
	return tg_setlvalue(tg_nodegetchild(ni, 1), r);
}

struct tg_val *tg_sign(int ni)
{
	struct tg_val *r;
	int sign;

	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 1)));

	sign = (tg_nodegettoken(tg_nodegetchild(ni, 0))
			->val.str[0] == '-') ? -1 : 1;
		
	return tg_valprinterr(tg_valmult(r, tg_intval(sign)));
}

struct tg_val *tg_not(int ni)
{
	struct tg_val *r;
	
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni, 0)));
	
	return (tg_istrueval(r)) ? tg_intval(0) : tg_intval(1);
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
			TG_NULLQUIT(tg_valprinterr(r = tg_valmult(r, v)));
		else if (strcmp(s, "/") == 0)
			TG_NULLQUIT(tg_valprinterr(r = tg_valdiv(r, v)));
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
			TG_NULLQUIT(tg_valprinterr(r = tg_valadd(r, v)));
		else if (strcmp(s, "-") == 0)
			TG_NULLQUIT(tg_valprinterr(r = tg_valsub(r, v)));
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
		
		TG_NULLQUIT(tg_valprinterr(r = tg_valcat(r, v)));
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
		if (tg_nodeccnt(ni) > i + 2
				&& tg_nodegettype(tg_nodegetchild(ni, i + 2))
				== TG_N_NEXTTOOPTS) {
			span = 1;
			++i;
		}
	
		if (strcmp(s, "<-") == 0) {
			TG_NULLQUIT(tg_valprinterr(r = tg_valnextto(
				r, v, 0, span)));
		}
		else if (strcmp(s, "^") == 0) {
			TG_NULLQUIT(tg_valprinterr(r = tg_valnextto(
				r, v, 1, span)));
		}
	}

	return r;
}

struct tg_val *tg_rel(int ni)
{
	struct tg_val *r1, *r2;
	const char *s;
	
	TG_NULLQUIT(r1 = run_node(tg_nodegetchild(ni, 0)));
	TG_NULLQUIT(r2 = run_node(tg_nodegetchild(ni, 2)));

	s = tg_nodegettoken(tg_nodegetchild(ni, 1))->val.str;

	if (strcmp(s, "==") == 0)
		return tg_valprinterr(tg_valeq(r1, r2));
	else if (strcmp(s, "!=") == 0)
		return tg_valprinterr(tg_valneq(r1, r2));
	else if (strcmp(s, "<") == 0)
		return tg_valprinterr(tg_valls(r1, r2));
	else if (strcmp(s, ">") == 0)
		return tg_valprinterr(tg_valgr(r1, r2));
	else if (strcmp(s, "<=") == 0)
		return tg_valprinterr(tg_vallseq(r1, r2));
	else if (strcmp(s, ">=") == 0)
		return tg_valprinterr(tg_valgreq(r1, r2));

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

struct tg_val *tg_assign(int ni)
{
	int i;
	struct tg_val *r;
	
	TG_NULLQUIT(r = run_node(tg_nodegetchild(ni,
		tg_nodeccnt(ni) - 1)));

	for (i = tg_nodeccnt(ni) - 2; i > 0; i -= 2)
		TG_NULLQUIT(tg_setlvalue(tg_nodegetchild(ni, i - 1), r));

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
	tg_startframe();
	
	retval = tg_emptyval();

	TG_NULLQUIT(tg_block(ni));

	tg_setcustomallocer(&retalloc);
	retval = tg_copyval(retval);
	tg_removecustomallocer(&retalloc);

	tg_endframe();	

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

static struct tg_val *run_node(int ni)
{
	int t;

	t = tg_nodegettype(ni) - TG_N_TEMPLATE;

	if (t < 0 || t > TG_N_ATTR)
		return tg_unexpected(ni);
	
	return node_handler[t](ni);
}

int main(int argc, const char *argv[])
{
	int tpl;
	
	if (argc < 2)
		TG_ERROR("Usage: %s [source file]", "tablegen");
	
	if (argc > 2) {
		// example: "test1:csv:./test1.csv;test2:script:./test2.sh"
		tg_readsourceslist(argv[2]);
	}

	tpl = tg_getparsetree(argv[1]);

	tg_printnode(tpl, 0);

	
	tg_initsymtable();
	tg_allocinit(&retalloc, sizeof(struct tg_val)); 
	
	tg_template(tpl);

	printf("return value: ");
	tg_printval(stdout, retval);
	printf("\n");

	tg_destroysymtable();

	return 0;
}
