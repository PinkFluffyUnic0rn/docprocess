#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "tg_parser.h"
#include "tg_value.h"

#define CONVERTERS_PATH "/home/qwerty/docprocess/docprocess_c/converters"

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

void tg_symboldel(const char *name)
{
	struct tg_hash *st;

	st = tg_darrget(&symtable, symtable.cnt - 1);

	tg_hashdel(TG_HASH_SYMBOL, st, name);
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
	else if (sym->type == TG_SYMTYPE_FUNCTION)
		return tg_emptyval();
		
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
	int j;

	for (j = 0; j < symtable.cnt; ++j) {
		int k;

		st = tg_darrget(&symtable, j);
		

		TG_HASHFOREACH(struct tg_symbol, TG_HASH_SYMBOL, *st, key,
			s = tg_hashget(TG_HASH_SYMBOL, st, key);

			for (k = 0; k < j; ++k) printf("\t");

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

static struct tg_val *tg_runnode(int ni);

static struct tg_val *tg_getindexexpr(int fni, struct tg_val *a)
{
	struct tg_val *idx;
	
	TG_NULLQUIT(idx = tg_runnode(fni));

	if (idx->type == TG_VAL_INT)
		a = tg_arrgetre(a, idx->intval, tg_emptyval());
	else {
		idx = tg_castval(idx, TG_VAL_STRING);
		a = tg_arrgethre(a, idx->strval.str, tg_emptyval());
	}

	return a;
}

static struct tg_val *tg_getindexlvalue(int ini, struct tg_val *a)
{
	int eni;

	if (tg_nodeccnt(ini) > 1)
		return NULL;
	
	eni = tg_nodechild(ini, 0);

	if (tg_nodetype(eni) == TG_N_RANGE
			|| tg_nodetype(eni) == TG_N_FILTER)
		return NULL;

	if (tg_isscalar(a->type))
		tg_moveval(a, tg_castval(a, TG_VAL_ARRAY));

	return tg_getindexexpr(eni, a);
}

static struct tg_val *tg_getattrlvalue(int eni, struct tg_val *a)
{
	const char *attrname;

	attrname = tg_nodetoken(tg_nodechild(eni, 0))->val.str;
	return tg_valgetattrre(a, attrname, tg_emptyval());
}
	
static struct tg_val *tg_setlvalue(int ni, struct tg_val *v)
{
	struct tg_symbol *s;
	struct tg_val *a;
	int ini;
	int i;
	
	if (tg_nodetype(ni) == TG_N_ID)
		ini = tg_nodechild(ni, 0);
	else if (tg_nodetype(ni) == TG_N_ADDRESS)
		ini = tg_nodechild(tg_nodechild(ni, 0), 0);
	else
		goto notlvalue;

	if (tg_symbolget(tg_nodetoken(ini)->val.str) == NULL) {
		s = tg_alloc(&symalloc);
		s->type = TG_SYMTYPE_VARIABLE;
		s->var.val = tg_emptyval();;
	
		tg_symboladd(tg_nodetoken(ini)->val.str, s);
	}

	s = tg_symbolget(tg_nodetoken(ini)->val.str);
	if (s->type != TG_SYMTYPE_VARIABLE)
		goto notlvalue;

	if (tg_nodetype(ni) != TG_N_ADDRESS)
		return (s->var.val = v);

	a = s->var.val;
	for (i = 1; i < tg_nodeccnt(ni); ++i) {
		int eni;

		eni = tg_nodechild(ni, i);
		if (tg_nodetype(eni) == TG_N_INDEX) {
			if ((a = tg_getindexlvalue(eni, a)) == NULL)
				goto notlvalue;
		}
		else if (tg_nodetype(eni) == TG_N_ATTR)
			a = tg_getattrlvalue(eni, a);
	}

	tg_moveval(a, v);

	return v;

notlvalue:
	TG_WARNING("Trying to assign into rvalue.");

	return NULL;
}

static struct tg_val *tg_funcdef(int ni)
{
	struct tg_symbol *s;
	
	s = tg_alloc(&symalloc);
	s->type = TG_SYMTYPE_FUNCTION;
	s->func.startnode = ni;
	
	tg_symboladd(tg_nodetoken(tg_nodechild(ni, 0))->val.str, s);
	
	return tg_intval(TG_STATE_SUCCESS);
}

static struct tg_val *tg_if(int ni)
{
	struct tg_val *r;
			
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	if (tg_istrueval(r))
		return tg_runnode(tg_nodechild(ni, 1));
	else if (tg_nodeccnt(ni) > 2)
		return tg_runnode(tg_nodechild(ni, 2));

	return tg_intval(TG_STATE_SUCCESS);
}

static struct tg_val *tg_forclassic(int ni)
{
	struct tg_val *r;
	int initnode;
	int cmpnode;
	int stepnode;
	int blocknode;
		
	initnode = tg_nodechild(ni, 0);
	cmpnode = tg_nodechild(ni, 1);
	stepnode = tg_nodechild(ni, 2);
	blocknode = tg_nodechild(ni, 3);
	
	tg_newscope();

	TG_NULLQUIT(tg_runnode(tg_nodechild(initnode, 0)));

	while (1) {
		TG_NULLQUIT(r = tg_runnode(tg_nodechild(cmpnode, 0)));
		if (!tg_istrueval(r))
			break;

		TG_NULLQUIT(r = tg_runnode(blocknode));
		if (r->intval == TG_STATE_RETURN)
			goto forend;
		else if (r->intval == TG_STATE_BREAK)
			break;

		TG_NULLQUIT(tg_runnode(tg_nodechild(stepnode, 0)));
	}

	r = tg_intval(TG_STATE_SUCCESS);

forend:
	tg_popscope();

	return r;
}

static struct tg_val *tg_for(int ni)
{
	if (tg_nodeccnt(ni) == 4
		&& tg_nodetype(tg_nodechild(ni, 0)) == TG_N_FOREXPR
		&& tg_nodetype(tg_nodechild(ni, 1)) == TG_N_FOREXPR
		&& tg_nodetype(tg_nodechild(ni, 2)) == TG_N_FOREXPR) {
		return tg_forclassic(ni);
	}
	else {
		return tg_intval(TG_STATE_SUCCESS); // TODO: iterate
						// through array or table
	}
}

static struct tg_val *tg_block(int ni)
{
	struct tg_val *r;
	int i;
	
	tg_newscope();

	for (i = 0; i < tg_nodeccnt(ni); ++i) {
		int cni;

		cni = tg_nodechild(ni, i);

		if (tg_nodetype(cni) == TG_N_RETURN) {
			if (tg_nodeccnt(cni) > 0) {
				TG_NULLQUIT(r = tg_runnode(
					tg_nodechild(cni, 0)));
				tg_setretval(r);
			}

			r = tg_intval(TG_STATE_RETURN);
			goto blockend;
		}
		else if (tg_nodetype(cni) == TG_T_BREAK) {
			r = tg_intval(TG_STATE_BREAK);
			goto blockend;
		}
		else if (tg_nodetype(cni) == TG_T_CONTINUE) {
			r = tg_intval(TG_STATE_CONTINUE);
			goto blockend;
		}

		TG_NULLQUIT(r = tg_runnode(cni));
		if (tg_isflownode(tg_nodetype(cni))
			&& r->intval != TG_STATE_SUCCESS) {
			goto blockend;
		}
	}

	r = tg_intval(TG_STATE_SUCCESS);

blockend:
//	tg_printsymbols(); // !!!	
	tg_popscope();
	
	return r;
}

static struct tg_val *tg_identificator(int ni)
{
	struct tg_val *r;
	struct tg_symbol *s;
	const char *name;
	int ani, fani, fni;
	int i;

	if (tg_nodeccnt(ni) == 1) {
		return tg_symbolgetval(tg_nodetoken(
			tg_nodechild(ni, 0))->val.str);
	}
	
	ani = tg_nodechild(ni, 1);

	name = tg_nodetoken(tg_nodechild(ni, 0))->val.str;
	if (strcmp(name, "int") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_INT);
	}
	else if (strcmp(name, "float") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_FLOAT);
	}
	else if (strcmp(name, "string") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_STRING);
	}
	else if (strcmp(name, "array") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_ARRAY);
	}
	else if (strcmp(name, "table") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_TABLE);
	}
	else if (strcmp(name, "print") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		printf("%s", tg_castval(v, TG_VAL_STRING)->strval.str);
		
		return tg_emptyval();
	}
	else if (strcmp(name, "dumpval") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		tg_printval(stdout, v);
		
		return tg_emptyval();
	}
	else if (strcmp(name, "substr") == 0) {
		return tg_emptyval();
	}
	else if (strcmp(name, "match") == 0) {
		return tg_emptyval();
	}
	else if (strcmp(name, "size") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(v = tg_castval(v, TG_VAL_ARRAY));

		return tg_intval(v->arrval.arr.cnt);
	}
	else if (strcmp(name, "length") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(v = tg_castval(v, TG_VAL_STRING));

		return tg_intval(v->strval.len);
	}
	else if (strcmp(name, "defval") == 0) {
		return tg_emptyval();
	}

	s = tg_symbolget(tg_nodetoken(tg_nodechild(ni, 0))->val.str);

	if (s == NULL || s->type != TG_SYMTYPE_FUNCTION)
		return tg_emptyval();

	fni = s->func.startnode;	
	fani = tg_nodechild(fni, 1);

	if (tg_nodeccnt(fani) != tg_nodeccnt(ani))
		return tg_emptyval();

	tg_startframe();
	tg_newscope();

	tg_setretval(tg_emptyval());
	
	for (i = 0; i < tg_nodeccnt(fani); ++i) {
		struct tg_symbol *ss;
		const char *sn;

		sn = tg_nodetoken(tg_nodechild(fani, i))->val.str;

		ss = tg_alloc(&symalloc);
		ss->type = TG_SYMTYPE_VARIABLE;
		if ((ss->var.val = tg_runnode(tg_nodechild(ani, i)))
				== NULL) {
			goto error;
		}
	
		tg_symboladd(sn, ss);
	}

	if (tg_runnode(tg_nodechild(fni, 2)) == NULL)
		goto error;

	r = tg_getretval();

	tg_popscope();
	tg_endframe();
	
	return r;

error:
	tg_popscope();
	tg_endframe();	
	return NULL;
}

static struct tg_val *tg_const(int ni)
{
	struct tg_token *t;
	
	t = tg_nodetoken(tg_nodechild(ni, 0));

	if (t->type == TG_T_STRING)
		return tg_stringval(t->val.str);
	else if (t->type == TG_T_INT)
		return tg_intval(atoi(t->val.str));
	else if (t->type == TG_T_FLOAT)
		return tg_floatval(atof(t->val.str));

	TG_ERROR("Unknown constant value type: %d", t->type);
}

static struct tg_val *tg_getindexfilter(int fni, struct tg_val *a)
{
	// run filter
		
	return tg_emptyval(); // !!!
}

static struct tg_val *tg_getindexrange(int fni, struct tg_val *a)
{
	struct tg_val *idxb, *idxe;
	struct tg_val *r;
	int i;

	idxb = tg_getindexexpr(tg_nodechild(fni, 0), a);
	idxe = tg_getindexexpr(tg_nodechild(fni, 1), a);

	if (idxb->arrpos < 0 || idxe->arrpos < 0)
		return tg_emptyval();

	r = tg_createval(TG_VAL_ARRAY);

	for (i = idxb->arrpos; i <= idxe->arrpos; ++i) {
		struct tg_val *v;

		v = tg_arrgetr(a, i);

		if (v->ishashed)
			tg_arrseth(r, tg_hashkey(TG_HASH_ARRAY, *v), v);
		else
			tg_arrpush(r, v);
	}

	return r;
}

static struct tg_val *tg_getindexval(int ini, struct tg_val *a)
{
	struct tg_val *r;
	int i;
	
	r = tg_createval(TG_VAL_ARRAY);

	if (tg_isscalar(a->type))
		a = tg_castval(a, TG_VAL_ARRAY);

	for (i = 0; i < tg_nodeccnt(ini); ++i) {
		struct tg_val *rr, *v;
		int fni;
		int j;

		fni = tg_nodechild(ini, i);

		if (tg_nodetype(fni) == TG_N_RANGE)
			rr = tg_getindexrange(fni, a);
		else if (tg_nodetype(fni) == TG_N_FILTER)
			rr = tg_getindexfilter(fni, a);
		else {
			tg_arrpush(r, tg_getindexexpr(fni, a));
			continue;
		}

		TG_ARRFOREACH(rr, j, v,
			if (v->ishashed)
				tg_arrseth(r, tg_hashkey(TG_HASH_ARRAY, *v), v);
			else
				tg_arrpush(r, v);	
		);
	}

	if (r->arrval.arr.cnt == 1)
		return tg_arrget(r, 0);

	return r;
}

static struct tg_val *tg_address(int ni)
{
	struct tg_symbol *s;
	struct tg_val *a;
	int ini;
	int i;

	ini = tg_nodechild(tg_nodechild(ni, 0), 0);

	if ((s = tg_symbolget(tg_nodetoken(ini)->val.str)) != NULL)
		a = s->var.val;
	else
		a = tg_createval(TG_VAL_ARRAY);

	for (i = 1; i < tg_nodeccnt(ni); ++i) {
		int eni;

		eni = tg_nodechild(ni, i);
		if (tg_nodetype(eni) == TG_N_INDEX)
			TG_NULLQUIT(a = tg_getindexval(eni, a));
		else if (tg_nodetype(eni) == TG_N_ATTR) {
			const char *attrname;
	
			attrname = tg_nodetoken(
				tg_nodechild(eni, 0))->val.str;

			a = tg_valgetattre(a, attrname, tg_emptyval());
		}
	}

	return a;
}

static struct tg_val *tg_prestep(int ni)
{
	struct tg_val *r;
	const char *s;

	s = tg_nodetoken(tg_nodechild(ni, 0))->val.str;
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 1)));

	if (strcmp(s, "++") == 0)
		TG_NULLQUIT(r = tg_valadd(r, tg_intval(1)));
	if (strcmp(s, "--") == 0)
		TG_NULLQUIT(r = tg_valsub(r, tg_intval(1)));
	
	return tg_setlvalue(tg_nodechild(ni, 1), r);
}

static struct tg_val *tg_sign(int ni)
{
	struct tg_val *r;
	int sign;

	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 1)));

	sign = (tg_nodetoken(tg_nodechild(ni, 0))
			->val.str[0] == '-') ? -1 : 1;

	return tg_valprinterr(tg_valmult(r, tg_intval(sign)));
}

static struct tg_val *tg_not(int ni)
{
	struct tg_val *r;

	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	return (tg_istrueval(r)) ? tg_intval(0) : tg_intval(1);
}

static struct tg_val *tg_ref(int ni)
{
	return NULL;
}

static struct tg_val *tg_mult(int ni)
{
	int i;
	struct tg_val *r;
	
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); i += 2) {
		struct tg_val *v;
		const char *s;
	
		s = tg_nodetoken(tg_nodechild(ni, i))->val.str;
		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ni, i + 1)));
		
		if (strcmp(s, "*") == 0)
			TG_NULLQUIT(tg_valprinterr(r = tg_valmult(r, v)));
		else if (strcmp(s, "/") == 0)
			TG_NULLQUIT(tg_valprinterr(r = tg_valdiv(r, v)));
	}

	return r;
}

static struct tg_val *tg_add(int ni)
{
	int i;
	struct tg_val *r;
	
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); i += 2) {
		struct tg_val *v;
		const char *s;
	
		s = tg_nodetoken(tg_nodechild(ni, i))->val.str;
		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ni, i + 1)));
		
		if (strcmp(s, "+") == 0)
			TG_NULLQUIT(tg_valprinterr(r = tg_valadd(r, v)));
		else if (strcmp(s, "-") == 0)
			TG_NULLQUIT(tg_valprinterr(r = tg_valsub(r, v)));
	}

	return r;
}

static struct tg_val *tg_cat(int ni)
{
	int i;
	struct tg_val *r;
	
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *v;
	
		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ni, i)));
		
		TG_NULLQUIT(tg_valprinterr(r = tg_valcat(r, v)));
	}

	return r;
}

static struct tg_val *tg_nextto(int ni)
{
	int i;
	struct tg_val *r;

	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); i += 2) {
		struct tg_val *v;
		const char *s;
		int span;
	
		s = tg_nodetoken(tg_nodechild(ni, i))->val.str;
		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ni, i + 1)));
	
		span = 0;
		if (tg_nodeccnt(ni) > i + 2
				&& tg_nodetype(tg_nodechild(ni, i + 2))
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

static struct tg_val *tg_rel(int ni)
{
	struct tg_val *r1, *r2;
	const char *s;
	
	TG_NULLQUIT(r1 = tg_runnode(tg_nodechild(ni, 0)));
	TG_NULLQUIT(r2 = tg_runnode(tg_nodechild(ni, 2)));

	s = tg_nodetoken(tg_nodechild(ni, 1))->val.str;

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

static struct tg_val *tg_and(int ni)
{
	int i;

	for (i = 0; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *r;
		
		TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, i)));
		if (!tg_istrueval(r))
			return tg_intval(0);
	}

	return tg_intval(1);
}

static struct tg_val *tg_or(int ni)
{
	int i;

	for (i = 0; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *r;
		
		TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, i)));
		if (tg_istrueval(r))
			return tg_intval(1);
	}

	return tg_intval(0);
}

static struct tg_val *tg_ternary(int ni)
{
	struct tg_val *r;
		
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	return tg_runnode(tg_nodechild(ni, (tg_istrueval(r) ? 1 : 2)));
}

static struct tg_val *tg_assign(int ni)
{
	int i;
	struct tg_val *r;
	
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni,
		tg_nodeccnt(ni) - 1)));

	for (i = tg_nodeccnt(ni) - 2; i > 0; i -= 2)
		TG_NULLQUIT(tg_setlvalue(tg_nodechild(ni, i - 1), r));

	return r;
}

static struct tg_val *tg_expr(int ni)
{
	int i;
	struct tg_val *r;
			
	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	for (i = 1; i < tg_nodeccnt(ni); ++i)
		TG_NULLQUIT(tg_runnode(tg_nodechild(ni, i)));

	return r;
}

static struct tg_val *tg_template(int ni)
{
	tg_startframe();
	tg_newscope();
	
	tg_setretval(tg_emptyval());
	
	TG_NULLQUIT(tg_block(ni));

	tg_popscope();
	tg_endframe();	
	
	return tg_getretval();
}

static struct tg_val *tg_unexpected(int ni)
{
	TG_WARNING("Unexpected node type: %s",
		tg_strsym[tg_nodetype(ni)]);

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

static struct tg_val *tg_runnode(int ni)
{
	int t;

	t = tg_nodetype(ni) - TG_N_TEMPLATE;

	if (t < 0 || t > TG_N_ATTR)
		return tg_unexpected(ni);
	
	return node_handler[t](ni);
}

int main(int argc, const char *argv[])
{
	struct tg_val *r;
	int tpl;
	
	if (argc < 2)
		TG_ERROR("Usage: %s [source file]", "tablegen");
	
	if (argc > 2)
		tg_readsourceslist(argv[2]);

	if ((tpl = tg_getparsetree(argv[1])) < 0)
		TG_ERROR("%s", tg_error);

	tg_initsymtable();
	tg_startframe();
	
	tg_setretval(tg_emptyval());
	
	if ((r = tg_runnode(tpl)) == NULL)
		return 1;

	printf("return value: ");
	tg_printval(stdout, r);
	printf("\n");
	
	tg_endframe();	
	tg_destroysymtable();

	return 0;
}
