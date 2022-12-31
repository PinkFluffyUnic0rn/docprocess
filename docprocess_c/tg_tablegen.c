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

void *tg_symbolget(const char *name, enum TG_SYMTYPE type)
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
		TG_ERROR("Cannot convert function %s to value.", name);
		return NULL;
	}
		
	TG_ERROR("Symbol %s has unknown type.", name);

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

struct tg_val *tg_attr(int ni)
{
	return NULL;
}

struct tg_val *tg_args(int ni)
{
	return NULL;
}

struct tg_val *tg_index(int ni)
{
	return NULL;
}

struct tg_val *tg_identificator(int ni)
{
	return NULL;
}

struct tg_val *tg_const(int ni)
{
	return NULL;
}

struct tg_val *tg_valhandler(int ni)
{
	return NULL;
}

struct tg_val *tg_address(int ni)
{
	return NULL;
}

struct tg_val *tg_prestep(int ni)
{
	return NULL;
}

struct tg_val *tg_sign(int ni)
{
	return NULL;
}

struct tg_val *tg_not(int ni)
{
	return NULL;
}

struct tg_val *tg_ref(int ni)
{
	return NULL;
}

struct tg_val *tg_unary(int ni)
{
	return NULL;
}

struct tg_val *tg_mult(int ni)
{
	return NULL;
}

struct tg_val *tg_add(int ni)
{
	return NULL;
}

struct tg_val *tg_cat(int ni)
{
	return NULL;
}

struct tg_val *tg_nextto(int ni)
{
	return NULL;
}

struct tg_val *tg_rel(int ni)
{
	return NULL;
}

struct tg_val *tg_and(int ni)
{
	return NULL;
}

struct tg_val *tg_or(int ni)
{
	return NULL;
}

struct tg_val *tg_ternary(int ni)
{
	return NULL;
}

struct tg_val *tg_assign(int ni)
{
	return NULL;
}

struct tg_val *tg_expr(int ni)
{
	int i;
	struct tg_val *r;
		
	r = run_node(tg_nodegetchild(ni, 0));

	for (i = 1; i < tg_nodeccnt(ni); ++i)
		run_node(tg_nodegetchild(ni, i));

	return r;
}

struct tg_val *tg_template(int ni)
{
	int i;

	for (i = 0; i < tg_nodeccnt(ni); ++i)
		run_node(tg_nodegetchild(ni, i));

	return 0;
}

struct tg_val *tg_unexpected(int ni)
{
	TG_ERROR("Unexpected node type: %s",
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
	tg_mult,	tg_unary,	tg_ref,
	tg_not,		tg_sign, 	tg_prestep,
	tg_address,	tg_valhandler,	tg_identificator,
	tg_const,	tg_index,	tg_unexpected,
	tg_unexpected,	tg_args,	tg_attr
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
