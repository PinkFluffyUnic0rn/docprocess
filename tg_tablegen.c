#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <regex.h>

#include "tg_parser.h"
#include "tg_value.h"
#include "tg_inout.h"

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

struct tg_darray tg_currow;

struct tg_val *tg_valprinterr(struct tg_val *v)
{
	if (v == NULL)
		TG_WARNING("%s", tg_error);

	return v;
}

static void tg_initsymtable()
{
	tg_darrinit(&symtable, sizeof(struct tg_hash));
	tg_allocinit(&symalloc, sizeof(struct tg_symbol)); 
}

static void tg_newscope()
{
	struct tg_hash st;
	
	tg_inithash(TG_HASH_SYMBOL, &st);

	tg_darrpush(&symtable, &st); 
}

static void tg_popscope()
{
	struct tg_hash st;
	
	if (tg_darrpop(&symtable, &st) >= 0)
		tg_destroyhash(TG_HASH_SYMBOL, &st);
}

static void tg_destroysymtable()
{
	tg_darrclear(&symtable);
	tg_allocdestroy(&symalloc, NULL); 
}

static void tg_symboladd(const char *name, struct tg_symbol *s)
{
	struct tg_hash *st;

	st = tg_darrget(&symtable, symtable.cnt - 1);
		
	tg_hashset(TG_HASH_SYMBOL, st, name, s);
}

static struct tg_symbol *tg_symbolget(const char *name)
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

static const struct tg_val *tg_symbolgetval(const char *name)
{
	struct tg_symbol *sym;

	if ((sym = tg_symbolget(name)) == NULL)
		return NULL;

	if (sym->type == TG_SYMTYPE_VARIABLE)
		return sym->var.val;
	if (sym->type == TG_SYMTYPE_INPUT)
		return tg_readsource(&(sym->input), tg_createval(TG_VAL_ARRAY));
	else if (sym->type == TG_SYMTYPE_FUNCTION)
		return tg_emptyval();
		
	TG_WARNING("Symbol %s has unknown type.", name);

	return NULL;
}

void tg_printsymbols()
{
	const char *key;
	const struct tg_symbol *s;
	const struct tg_hash *st;
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

static int tg_readsourceslist(const char *sources)
{
	char *s, *ss;
	const char *p;

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
		tg_createinput(&(in->input), type, path);

		tg_symboladd(name, in);

	} while ((p = strtok(NULL, ";")) != NULL);

	free(s);
	free(ss);

	return 0;
}

static struct tg_val *tg_runnode(int ni);

static struct tg_val *tg_getattrlvalue(int eni, struct tg_val *a)
{
	const char *attrname;

	attrname = tg_nodetoken(tg_nodechild(eni, 0))->val.str;
	return tg_valgetattrre(a, attrname, tg_emptyval());
}
	
static struct tg_val *tg_setlvalue(int ni, const struct tg_val *v)
{
	struct tg_darray idxr;
	struct tg_symbol *s;
	struct tg_val *a;
	const char *name;
	int ini;
	int i;

	tg_darrinit(&idxr, sizeof(struct tg_val *));
	
	if (tg_nodetype(ni) == TG_N_ID)
		ini = tg_nodechild(ni, 0);
	else if (tg_nodetype(ni) == TG_N_ADDRESS)
		ini = tg_nodechild(tg_nodechild(ni, 0), 0);
	else
		goto notlvalue;

	name = tg_nodetoken(ini)->val.str;

	if ((s = tg_symbolget(name)) != NULL
			&& s->type != TG_SYMTYPE_VARIABLE) {
		tg_darrclear(&idxr);

		TG_WARNING("Identificator %s is already used.", name);
		return NULL;
	}

	for (i = 1; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *idx;
		int eni;

		eni = tg_nodechild(ni, i);

		if (tg_nodetype(eni) == TG_N_ATTR)
			continue;

		if (tg_nodeccnt(eni) > 1)
			goto notlvalue;
		
		eni = tg_nodechild(eni, 0);

		if (tg_nodetype(eni) == TG_N_RANGE
				|| tg_nodetype(eni) == TG_N_FILTER)
			goto notlvalue;

		if((idx = tg_runnode(eni)) == NULL)
			goto notlvalue;
	
		tg_darrset(&idxr, i, &idx);
	}

	if (s == NULL) {
		s = tg_alloc(&symalloc);
		s->type = TG_SYMTYPE_VARIABLE;
		s->var.val = tg_emptyval();

		tg_symboladd(tg_nodetoken(ini)->val.str, s);
	}

	s = tg_symbolget(name);

	a = s->var.val;

	if (tg_nodetype(ni) != TG_N_ADDRESS)
		goto skipaddress;

	for (i = 1; i < tg_nodeccnt(ni); ++i) {
		struct tg_val *idx;

		if (tg_nodetype(tg_nodechild(ni, i)) == TG_N_ATTR) {
			a = tg_getattrlvalue(tg_nodechild(ni, i), a);
			continue;
		}

		idx = *((struct tg_val **) tg_darrget(&idxr, i));

		if (tg_isscalar(a->type))
			tg_moveval(a, tg_castval(a, TG_VAL_ARRAY));

		if (idx->type == TG_VAL_INT)
			a = tg_arrgetre(a, idx->intval, tg_emptyval());
		else {
			idx = tg_castval(idx, TG_VAL_STRING);
			a = tg_arrgethre(a, idx->strval.str, tg_emptyval());
		}
	}

skipaddress:
	tg_darrclear(&idxr);
	
	tg_moveval(a, v);
	
	return s->var.val;

notlvalue:
	tg_darrclear(&idxr);
	
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
	const struct tg_val *r;
			
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

	TG_NULLQUIT(tg_runnode(initnode));

	while (1) {
		TG_NULLQUIT(r = tg_runnode(cmpnode));
		if (!tg_istrueval(r))
			break;

		TG_NULLQUIT(r = tg_runnode(blocknode));
		if (r->intval == TG_STATE_RETURN)
			goto forend;
		else if (r->intval == TG_STATE_BREAK)
			break;

		TG_NULLQUIT(tg_runnode(stepnode));
	}

	r = tg_intval(TG_STATE_SUCCESS);

forend:
	tg_popscope();

	return r;
}

static struct tg_val *tg_fortable(int ni)
{
	struct tg_val *r, *t;
	int idnode;
	int tablenode;
	int blocknode;
	int i;
		
	idnode = tg_nodechild(ni, 0);
	tablenode = tg_nodechild(ni, 1);
	blocknode = tg_nodechild(ni, 2);

	tg_newscope();

	TG_NULLQUIT(t = tg_runnode(tablenode));

	for (i = 0; i < t->arrval.arr.cnt; ++i) {
		struct tg_val *v;
	
		TG_ASSERT((v = tg_arrgetr(t, i)) != NULL,
			"Error while iterating through array,");

		TG_NULLQUIT(tg_setlvalue(idnode, v));

		TG_NULLQUIT(r = tg_runnode(blocknode));
		if (r->intval == TG_STATE_RETURN)
			goto forend;
		else if (r->intval == TG_STATE_BREAK)
			break;
	}

	r = tg_intval(TG_STATE_SUCCESS);

forend:
	tg_popscope();

	return r;
}

static struct tg_val *tg_for(int ni)
{
	if (tg_nodeccnt(ni) == 4) {
		return tg_forclassic(ni);
	}
	else
		return tg_fortable(ni);
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
	tg_popscope();
	
	return r;
}

static struct tg_val *tg_checkargs(const char *name,
	int has, int expect)
{
	if (has != expect) {
		TG_WARNING("%s: expected %d arguments, got %d.",
			name, expect, has);
		return NULL;
	}

	return tg_intval(1);
}

static struct tg_val *tg_identificator(int ni)
{
	struct tg_val *r;
	struct tg_symbol *s;
	const char *name;
	int ani, fani, fni;
	int i;

	if (tg_nodeccnt(ni) == 1) {
		const struct tg_val *rc;

		if ((rc = tg_symbolgetval(tg_nodetoken(
				tg_nodechild(ni, 0))->val.str)) == NULL)
			return tg_emptyval();

		return tg_copyval(rc);
	}
	
	ani = tg_nodechild(ni, 1);

	name = tg_nodetoken(tg_nodechild(ni, 0))->val.str;
	if (strcmp(name, "type") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
	
		if (v->type == TG_VAL_EMPTY)
			return tg_stringval("empty");
		else if (v->type == TG_VAL_INT)
			return tg_stringval("int");
		else if (v->type == TG_VAL_FLOAT)
			return tg_stringval("float");
		else if (v->type == TG_VAL_STRING)
			return tg_stringval("string");
		else if (v->type == TG_VAL_TABLE)
			return tg_stringval("table");
		else if (v->type == TG_VAL_ARRAY)
			return tg_stringval("array");
		else if (v->type == TG_VAL_DELETED)
			return tg_stringval("deleted");
		else
			return tg_stringval("unknown");
	}
	if (strcmp(name, "int") == 0) {
		struct tg_val *v;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_INT);
	}
	else if (strcmp(name, "float") == 0) {
		struct tg_val *v;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_FLOAT);
	}
	else if (strcmp(name, "string") == 0) {
		struct tg_val *v;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_STRING);
	}
	else if (strcmp(name, "array") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));
		
		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_ARRAY);
	}
	else if (strcmp(name, "table") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));
		
		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_castval(v, TG_VAL_TABLE);
	}
	else if (strcmp(name, "print") == 0) {
		struct tg_val *v;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		printf("%s", tg_castval(v, TG_VAL_STRING)->strval.str);
		
		return tg_emptyval();
	}
	else if (strcmp(name, "dumpval") == 0) {
		struct tg_val *v;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));
		
		return tg_printval(stdout, v);
	}
	else if (strcmp(name, "substr") == 0) {
		struct tg_val *s, *b, *l;

		if (tg_nodeccnt(ani) < 2) {
			TG_WARNING("%s: expected at least %d arguments, got %d.",
				name, 2, tg_nodeccnt(ani));
			return NULL;
		}

		TG_NULLQUIT(s = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(b = tg_runnode(tg_nodechild(ani, 1)));

		if (tg_nodeccnt(ani) > 2)
			TG_NULLQUIT(l = tg_runnode(tg_nodechild(ani, 2)));
		else
			l = tg_intval(-1);

		return tg_substr(s, b, l);
	}
	else if (strcmp(name, "match") == 0) {
		struct tg_val *s, *reg;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 2));

		TG_NULLQUIT(s = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(reg = tg_runnode(tg_nodechild(ani, 1)));

		return tg_match(s, reg);
	}
	else if (strcmp(name, "size") == 0) {
		struct tg_val *a;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(a = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(a = tg_castval(a, TG_VAL_ARRAY));

		return tg_intval(a->arrval.arr.cnt);
	}
	else if (strcmp(name, "length") == 0) {
		struct tg_val *s;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(s = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(s = tg_castval(s, TG_VAL_STRING));

		return tg_intval(s->strval.len);
	}
	else if (strcmp(name, "defval") == 0) {
		struct tg_val *v;
		
		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 2));

		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 0)));

		if (v->type == TG_VAL_EMPTY)
			return tg_runnode(tg_nodechild(ani, 1));

		return v;
	}
	else if (strcmp(name, "rudate") == 0) {
		struct tg_val *d, *m, *y;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 3));

		TG_NULLQUIT(d = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(m = tg_runnode(tg_nodechild(ani, 1)));
		TG_NULLQUIT(y = tg_runnode(tg_nodechild(ani, 2)));

		return tg_rudate(d, m, y);
	}
	else if (strcmp(name, "day") == 0) {
		struct tg_val *d;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(d = tg_runnode(tg_nodechild(ani, 0)));

		return tg_day(d);
	}
	else if (strcmp(name, "month") == 0) {
		struct tg_val *d;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(d = tg_runnode(tg_nodechild(ani, 0)));

		return tg_month(d);
	}
	else if (strcmp(name, "year") == 0) {
		struct tg_val *d;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(d = tg_runnode(tg_nodechild(ani, 0)));

		return tg_year(d);
	}
	else if (strcmp(name, "weekday") == 0) {
		struct tg_val *d;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 1));

		TG_NULLQUIT(d = tg_runnode(tg_nodechild(ani, 0)));

		return tg_weekday(d);
	}
	else if (strcmp(name, "monthend") == 0) {
		struct tg_val *d, *m;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 2));

		TG_NULLQUIT(d = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(m = tg_runnode(tg_nodechild(ani, 1)));

		return tg_monthend(d, m);
	}
	else if (strcmp(name, "datecmp") == 0) {
		struct tg_val *d1, *d2;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 2));

		TG_NULLQUIT(d1 = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(d2 = tg_runnode(tg_nodechild(ani, 1)));

		return tg_datecmp(d1, d2);
	}
	else if (strcmp(name, "datediff") == 0) {
		struct tg_val *d1, *d2, *p;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 3));

		TG_NULLQUIT(d1 = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(d2 = tg_runnode(tg_nodechild(ani, 1)));
		TG_NULLQUIT(p = tg_runnode(tg_nodechild(ani, 2)));

		return tg_datediff(d1, d2, p);
	}
	else if (strcmp(name, "dateadd") == 0) {
		struct tg_val *d, *v, *p;

		TG_NULLQUIT(tg_checkargs(name, tg_nodeccnt(ani), 3));

		TG_NULLQUIT(d = tg_runnode(tg_nodechild(ani, 0)));
		TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, 1)));
		TG_NULLQUIT(p = tg_runnode(tg_nodechild(ani, 2)));

		return tg_dateadd(d, v, p);
	}

	if ((s = tg_symbolget(tg_nodetoken(
			tg_nodechild(ni, 0))->val.str)) == NULL) {
		return tg_emptyval();
	}
	
	if (s->type == TG_SYMTYPE_INPUT) {
		struct tg_val *args;

		args = tg_createval(TG_VAL_ARRAY);

		for (i = 0; i < tg_nodeccnt(ani); ++i) {
			struct tg_val *v;
			
			TG_NULLQUIT(v = tg_runnode(tg_nodechild(ani, i)));
			tg_arrpush(args, v); 
		}
		
		return tg_readsource(&(s->input), args);
	}

	if (s->type != TG_SYMTYPE_FUNCTION)
		return tg_emptyval();

	fni = s->func.startnode;	
	fani = tg_nodechild(fni, 1);

	TG_NULLQUIT(tg_checkargs(name,
		tg_nodeccnt(ani), tg_nodeccnt(fani)));

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
	else if (t->type == TG_T_EMPTY)
		return tg_emptyval();

	TG_ERROR("Unknown constant value type: %d", t->type);
}

static struct tg_val *tg_getindexfilter(int fni, const struct tg_val *a)
{
	struct tg_val *v, *b;
	struct tg_val *r;
	int i;
	
	fni = tg_nodechild(fni, 0);
	
	r = tg_createval(TG_VAL_ARRAY);

	TG_ARRFOREACH(a, i, v,
		if (tg_isscalar(v->type))
			v = tg_castval(v, TG_VAL_ARRAY);

		tg_darrpush(&tg_currow, &v);
	
		if ((b = tg_runnode(fni)) == NULL) {
			tg_darrpop(&tg_currow, NULL);
			return NULL;
		}

		if (tg_istrueval(b))
			tg_arrpush(r, tg_intval(v->arrpos));

		tg_darrpop(&tg_currow, NULL);
	);

	return r;
}

static struct tg_val *tg_getindexexpr(int fni, const struct tg_val *a)
{
	struct tg_val *idx;

	TG_NULLQUIT(idx = tg_runnode(fni));

	if (idx->type == TG_VAL_INT) {
		if ((a = tg_arrgetr(a, idx->intval)) == NULL)
			return tg_emptyval();
	}
	else {
		idx = tg_castval(idx, TG_VAL_STRING);

		if ((a = tg_arrgethr(a, idx->strval.str)) == NULL)
			return tg_emptyval();
	}


	return tg_intval(a->arrpos);
}

static struct tg_val *tg_getindexrange(int fni, const struct tg_val *a)
{
	struct tg_val *idxb, *idxe;
	struct tg_val *r;
	int i;

	idxb = tg_getindexexpr(tg_nodechild(fni, 0), a);
	idxe = tg_getindexexpr(tg_nodechild(fni, 1), a);

	if (idxb->type == TG_VAL_EMPTY || idxe->type == TG_VAL_EMPTY )
		return tg_emptyval();

	r = tg_createval(TG_VAL_ARRAY);

	for (i = idxb->intval; i <= idxe->intval; ++i)
		tg_arrpush(r, tg_intval(i));

	return r;
}

int tg_idxsort_f(const void *v1, const void *v2)
{
	int i1, i2;

	i1 = (*((struct tg_val **) v1))->intval;
	i2 = (*((struct tg_val **) v2))->intval;

	if (i1 == i2)		return 0;
	else if (i1 < i2)	return -1;
	else			return 1;
}

static struct tg_val *tg_getindexval(int ini, const struct tg_val *a)
{
	struct tg_val *ri, *r, *v;
	int prevri;
	int i;
	
	if (tg_isscalar(a->type))
		a = tg_castval(a, TG_VAL_ARRAY);

	ri = tg_createval(TG_VAL_ARRAY);
	
	for (i = 0; i < tg_nodeccnt(ini); ++i) {
		struct tg_val *rr;
		int fni;
		int j;

		fni = tg_nodechild(ini, i);

		if (tg_nodetype(fni) == TG_N_RANGE)
			rr = tg_getindexrange(fni, a);
		else if (tg_nodetype(fni) == TG_N_FILTER)
			rr = tg_getindexfilter(fni, a);
		else {
			rr = tg_castval(tg_getindexexpr(fni, a),
				TG_VAL_ARRAY);
		}

		if (rr->type == TG_VAL_EMPTY)
			continue;

		TG_ARRFOREACH(rr, j, v, tg_arrpush(ri, v));
	}

	tg_darrsort(&(ri->arrval.arr), tg_idxsort_f);

	r = tg_createval(TG_VAL_ARRAY);

	prevri = -1;
	TG_ARRFOREACH(ri, i, v,
		if (v->intval == prevri)
			continue;

		prevri = v->intval;

		v = tg_arrgetr(a, v->intval);

		if (v->ishashed)
			tg_arrseth(r, tg_hashkey(TG_HASH_ARRAY, *v), v);
		else
			tg_arrpush(r, v);	
	);

	if (r->arrval.arr.cnt == 0)
		return tg_emptyval();

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
	struct tg_val *cr;
	struct tg_val *r;

	TG_NULLQUIT(r = tg_runnode(tg_nodechild(ni, 0)));

	cr = *((struct tg_val **) tg_darrget(&tg_currow,
			tg_currow.cnt - 1));

	if (r->type == TG_VAL_INT)
		return tg_arrgete(cr, r->intval, tg_emptyval());
		
	r = tg_castval(r, TG_VAL_STRING);

	return tg_arrgethe(cr, r->strval.str, tg_emptyval());
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

const char **tg_buildpathv(const char *path, char **buf)
{
	const char **pathv;
	struct tg_darray patharr;
	char *s;
	char *p;
	int i;

	tg_darrinit(&patharr, sizeof(const char *));
	pathv = NULL;

	s = tg_strdup(path);

	p = strtok(s, ";");
	while (p != NULL) {
		tg_darrpush(&(patharr), &p);
	
		p = strtok(NULL, ";");
	}

	TG_ASSERT((pathv = malloc(patharr.cnt + 1)) != NULL,
		"Error while allocating memory.");

	for (i = 0; i < patharr.cnt; ++i) {
		const char *p;
		FILE *f;

		p  = *((const char **) tg_darrget(&(patharr), i));		
		
		TG_ASSERT(f = fopen(p, "r"),
			"Can't open sources file %s for reading.", p);
	
		pathv[i] = p;
	}

	if (*buf != NULL)
		*buf = s;

	pathv[patharr.cnt] = NULL;
	
	return pathv;
}

int main(int argc, const char *argv[])
{
	const char **pathv;
	char *pathvbuf;
	struct tg_output out;
	struct tg_val *r;
	int tpl;
	
	if (argc < 2)
		TG_ERROR("Usage: %s [source file]", "tablegen");
	
	tg_initsymtable();
	tg_startframe();
	tg_newscope();

	tg_darrinit(&tg_currow, sizeof(struct tg_val *));

	if (argc > 2)
		tg_readsourceslist(argv[2]);	

	pathv = tg_buildpathv(argv[1], &pathvbuf);

	if ((tpl = tg_getparsetree(pathv)) < 0)
		TG_ERROR("%s", tg_error);

	if ((r = tg_runnode(tpl)) == NULL)
		return 1;

	if (argc > 3) {
		tg_createoutput(&out, argv[3], stdout);
		if (tg_writeval(&out, r) == NULL)
			return 1;
	}
	
	tg_darrclear(&tg_currow);

	tg_popscope();
	tg_endframe();	

	tg_destroysymtable();

	free(pathvbuf);

	return 0;
}
