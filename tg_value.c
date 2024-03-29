#include "tg_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <regex.h>

#include "tg_dstring.h"
#include "tg_darray.h"
#include "tg_common.h"

#define TG_STACKMAXDEPTH 64
#define TG_FRAMEBLOCKSIZE 32

TG_HASHED(struct tg_val, TG_HASH_ARRAY)

struct tg_allocator *customallocer = NULL;

struct tg_allocator tg_stack[TG_STACKMAXDEPTH];
int tg_stackdepth = 0;

struct tg_allocator retalloc;
struct tg_val *retval;

#define tg_allocval() tg_alloc((customallocer != NULL) \
	? customallocer : (tg_stack + tg_stackdepth))

// start frame, call every time
// when block starts
int tg_startframe()
{
	if (tg_stackdepth == 0)
		tg_allocinit(&retalloc, sizeof(struct tg_val)); 

	if (tg_stackdepth == TG_STACKMAXDEPTH) {
		// error
	}
	
	++tg_stackdepth;

	tg_allocinit(tg_stack + tg_stackdepth, sizeof(struct tg_val));

	return 0;
}

// end frame, destroy all variables,
// allocated in this frame
void tg_endframe()
{
	tg_allocdestroy(tg_stack + tg_stackdepth,
		(void (*)(void *)) tg_freeval);

	--tg_stackdepth;

	if (tg_stackdepth == 0) {
		customallocer = &retalloc;
		tg_allocdestroy(&retalloc, (void (*)(void *)) tg_freeval); 
		customallocer = NULL;
	}
}

void tg_setretval(const struct tg_val *v)
{
	customallocer = &retalloc;
	retval = tg_copyval(v);
	customallocer = NULL;
}

struct tg_val *tg_getretval()
{
	struct tg_val *r;
	
	r = retval;

	return r;
}

struct tg_val *tg_createval(enum TG_VALTYPE t)
{
	struct tg_val *newv;

	newv = tg_allocval();

	tg_inithash(TG_HASH_ARRAY, &(newv->attrs));

	if (t == TG_VAL_INT)
		newv->intval = 0;
	else if (t == TG_VAL_FLOAT)
		newv->floatval = 0.0;
	else if (t == TG_VAL_STRING)
		tg_dstrcreate(&(newv->strval), "");
	else if (t == TG_VAL_TABLE) {
		tg_valsetattr(newv, "rows", tg_intval(0));
		tg_valsetattr(newv, "cols", tg_intval(0));

		tg_inithash(TG_HASH_ARRAY, &(newv->arrval.hash));
		tg_darrinit(&(newv->arrval.arr),
			sizeof(struct tg_val *));
	}	
	else if (t == TG_VAL_ARRAY) {
		tg_inithash(TG_HASH_ARRAY, &(newv->arrval.hash));
		tg_darrinit(&(newv->arrval.arr),
			sizeof(struct tg_val *));
	}

	newv->type = t;
	newv->ishashed = 0;
	newv->arrpos = -1;

	return newv;
}

void tg_freeval(struct tg_val *v)
{
	int i;
	const char *key;

	TG_ASSERT(v != NULL, "Cannot free value");

	if (v->type == TG_VAL_DELETED)
		return;

	if (v->type == TG_VAL_STRING)
		tg_dstrdestroy(&(v->strval));

	TG_HASHFOREACH(struct tg_val, TG_HASH_ARRAY, v->attrs, key,
		tg_freeval(tg_valgetattrr(v, key));
		tg_hashdel(TG_HASH_ARRAY, &(v->attrs), key);
	);

	tg_destroyhash(TG_HASH_ARRAY, &(v->attrs));


	if (!tg_isscalar(v->type)) {
		struct tg_val *vv;

		TG_ARRFOREACH(v, i, vv, tg_freeval(vv););
	
		tg_darrclear(&(v->arrval.arr));
	
		tg_destroyhash(TG_HASH_ARRAY, &(v->arrval.hash));
	}
	
	v->type = TG_VAL_DELETED;

	if (customallocer != NULL) {
		tg_free(customallocer, v);
		return;
	}
		
	tg_free(tg_stack + tg_stackdepth, v);
}

struct tg_val *tg_intval(int v)
{
	struct tg_val *newv;

	newv = tg_allocval();
	
	tg_inithash(TG_HASH_ARRAY, &(newv->attrs));

	newv->intval = v;
	newv->type = TG_VAL_INT;
	
	return newv;
}

struct tg_val *tg_floatval(float v)
{
	struct tg_val *newv;

	newv = tg_allocval();
	
	tg_inithash(TG_HASH_ARRAY, &(newv->attrs));

	newv->floatval = v;
	newv->type = TG_VAL_FLOAT;
	
	return newv;
}

struct tg_val *tg_stringval(const char *v)
{
	struct tg_val *newv;
	
	TG_ASSERT(v != NULL, "Cannot create string value");

	newv = tg_allocval();
	
	tg_inithash(TG_HASH_ARRAY, &(newv->attrs));

	tg_dstrcreate(&(newv->strval), v);
	newv->type = TG_VAL_STRING;
	
	return newv;
}

void tg_moveval(struct tg_val *d, const struct tg_val *s)
{
	int ishashed;
	int arrpos;

	// preserve ishashed and arrpos, cause destination value
	// can still be used in array
	ishashed = d->ishashed;
	arrpos = d->arrpos;

	tg_freeval(d);

	memcpy(d, tg_copyval(s), sizeof(struct tg_val));
	d->ishashed = ishashed;
	d->arrpos = arrpos;
}

struct tg_val *tg_copyval(const struct tg_val *v)
{
	struct tg_val *newv;
	const char *key;

	TG_ASSERT(v != NULL, "Cannot copy value");

	newv = tg_allocval();
	newv->ishashed = 0;
	newv->arrpos = -1;
	tg_inithash(TG_HASH_ARRAY, &(newv->attrs));

	TG_HASHFOREACH(struct tg_val, TG_HASH_ARRAY, v->attrs, key,
		tg_valsetattr(newv, key, tg_valgetattr(v, key));
	);

	if (v->type == TG_VAL_INT)
		newv->intval = v->intval;
	else if (v->type == TG_VAL_FLOAT)
		newv->floatval = v->floatval;
	else if (v->type == TG_VAL_STRING)
		tg_dstrcreate(&(newv->strval), v->strval.str);
	else if (v->type == TG_VAL_ARRAY || v->type == TG_VAL_TABLE) {
		struct tg_val *vv;
		int i;
		
		tg_inithash(TG_HASH_ARRAY, &(newv->arrval.hash));
		tg_darrinit(&(newv->arrval.arr),
			sizeof(struct tg_val *));

		TG_ARRFOREACH(v, i, vv,
			if (vv->ishashed) {
				const char *k;

				k = tg_hashkey(TG_HASH_ARRAY, *vv);
				
				vv = tg_copyval(vv);
				vv->ishashed = 1;
			
				tg_hashset(TG_HASH_ARRAY,
					&(newv->arrval.hash), k, vv);
			}
			else {
				vv = tg_copyval(vv);
			}
		
			vv->arrpos = newv->arrval.arr.cnt;
			tg_darrpush(&(newv->arrval.arr), &vv);
		);
	}

	newv->type = v->type;

	return newv;
}

// should attributes be presersed?
struct tg_val *tg_castval(const struct tg_val *v,
	enum TG_VALTYPE t)
{
	struct tg_val *newv;
	enum TG_VALTYPE vt;
	
	TG_ASSERT(v != NULL, "Cannot cast value");

	vt = v->type;
	
	if (t == TG_VAL_EMPTY)
		return tg_emptyval();
	else if (vt == TG_VAL_EMPTY) {
		newv = tg_createval(t);
	}
	else if (vt == TG_VAL_INT && t == TG_VAL_FLOAT) {
		newv = tg_createval(t);
		newv->floatval = (float) v->intval;
	}
	else if (vt == TG_VAL_FLOAT && t == TG_VAL_INT) {
		newv = tg_createval(t);
		newv->intval = (int) floor(v->floatval);
	}
	else if (vt == TG_VAL_INT && t == TG_VAL_STRING) {
		char buf[1024];
		
		newv = tg_createval(t);

		snprintf(buf, 1024, "%d", v->intval);
		tg_dstraddstr(&(newv->strval), buf);
	}
	else if (vt == TG_VAL_FLOAT && t == TG_VAL_STRING) {
		char buf[1024];
		
		newv = tg_createval(t);

		snprintf(buf, 1024, "%f", v->floatval);
		tg_dstraddstr(&(newv->strval), buf);
	}
	else if (vt == TG_VAL_STRING && tg_isscalar(t)) {
		newv = tg_createval(t);

		if (t == TG_VAL_INT)
			sscanf(v->strval.str, "%d", &(newv->intval));
		else if (t == TG_VAL_FLOAT)
			sscanf(v->strval.str, "%f", &(newv->floatval));
		else
			tg_dstraddstr(&(newv->strval), v->strval.str);
	}
	else if (tg_isscalar(vt) && t == TG_VAL_ARRAY) {
		newv = tg_createval(t);

		tg_arrpush(newv, v);
	}
	else if (tg_isscalar(vt) && t == TG_VAL_TABLE) {
		struct tg_val *vv;

		newv = tg_createval(t);

		tg_valsetattr(newv, "rows", tg_intval(1));
		tg_valsetattr(newv, "cols", tg_intval(1));

		vv = tg_createval(TG_VAL_ARRAY);

		tg_arrpush(vv, v);
		tg_arrpush(newv, vv);
	}
	else if (vt == TG_VAL_ARRAY && tg_isscalar(t)) {
		if (v->arrval.arr.cnt > 1) {
			TG_SETERROR("%s%s%s", "Trying to cast array,",
				" that has more than to element to",
				" a scalar type");
			return NULL;
		}

		if (v->arrval.arr.cnt == 0)
			return tg_emptyval();

		newv = tg_castval(tg_arrgetr(v, 0), t);
	}
	else if (vt == TG_VAL_TABLE && tg_isscalar(t)) {
		struct tg_val *vv;

		if (v->arrval.arr.cnt > 1) {
			TG_SETERROR("%s%s%s", "Trying to cast a table,",
				" that has more than to element to",
				" a scalar type");
			return NULL;
		}

		if (v->arrval.arr.cnt == 0)
			return tg_emptyval();

		vv = tg_arrgetr(v, 0);

		if (vv->arrval.arr.cnt != 1) {
			TG_SETERROR("%s%s%s", "Trying to cast a table,",
				" that has more than to element to",
				" a scalar type");
			return NULL;
		}

		if (v->arrval.arr.cnt == 0)
			return tg_emptyval();

		newv = tg_castval(tg_arrgetr(vv, 0), t);
	}
	else if (vt == TG_VAL_TABLE && t == TG_VAL_ARRAY) {
		newv = tg_copyval(v);

		newv->type = TG_VAL_ARRAY;
	}
	else if (vt == TG_VAL_ARRAY && t == TG_VAL_TABLE) {
		int cols;
		int i;

		newv = tg_createval(t);

		tg_valsetattr(newv, "rows",
			tg_intval(v->arrval.arr.cnt));

		cols = 0;
		for (i = 0; i < v->arrval.arr.cnt; ++i) {
			struct tg_val *row, *cell;
			int j;

			row = tg_castval(tg_arrgetr(v, i),
					TG_VAL_ARRAY);

			if (row->arrval.arr.cnt > cols)
				cols = row->arrval.arr.cnt;

			TG_ARRFOREACH(row, j, cell,
				if (!tg_isscalar(cell->type)) {
					TG_SETERROR("%s%s%s%s",
						"Trying to cast a ",
						"array with no ",
						"table-like structure ",
						"to table.");
					return NULL;
				}
			);

			tg_arrpush(newv, row);

			tg_valsetattr(newv, "cols", tg_intval(cols));
		}
	}
	else
		newv = tg_copyval(v);

	return newv;
}

struct tg_val *tg_castvalr(struct tg_val *v, enum TG_VALTYPE t)
{
	if (v->type == t)
		return v;

	return tg_castval(v, t);
}

struct tg_val *tg_typeprom1val(const struct tg_val *v,
	enum TG_VALTYPE mtype)
{
	TG_ASSERT(v != NULL, "Cannot cast value");

	if (v->type > mtype)
		return tg_castval(v, mtype);

	return tg_copyval(v);
}

struct tg_val *tg_typeprom2val(const struct tg_val *v1,
	enum TG_VALTYPE v2type, enum TG_VALTYPE mtype)
{
	TG_ASSERT(v1 != NULL, "Cannot cast value");

	if (v1->type > mtype || v2type > mtype)
		return tg_castval(v1, mtype);
	else if (v1->type < v2type)
		return tg_castval(v1, v2type);

	return tg_copyval(v1);
}

void tg_valsetattr(struct tg_val *v, const char *key,
	const struct tg_val *attr)
{
	TG_ASSERT(v != NULL, "Cannot set value attribute");
	TG_ASSERT(key != NULL, "Cannot set value attribute");
	TG_ASSERT(attr != NULL, "Cannot set value attribute");

	tg_hashset(TG_HASH_ARRAY, &(v->attrs), key, tg_copyval(attr));
}

struct tg_val *tg_valgetattrr(const struct tg_val *v, const char *key)
{
	struct tg_val *r;
	
	TG_ASSERT(v != NULL, "Cannot get value attribute");
	TG_ASSERT(key != NULL, "Cannot get value attribute");

	if ((r = tg_hashget(TG_HASH_ARRAY, &(v->attrs), key)) == NULL)
		return NULL;

	return r;
}

struct tg_val *tg_valgetattrre(struct tg_val *v, const char *key,
	const struct tg_val *e)
{
	struct tg_val *r;
	
	TG_ASSERT(v != NULL, "Cannot get value attribute");
	TG_ASSERT(key != NULL, "Cannot get value attribute");
	TG_ASSERT(e != NULL, "Cannot get value attribute");

	if ((r = tg_valgetattrr(v, key)) != NULL)
		return r;

	tg_valsetattr(v, key, e);

	return tg_valgetattrr(v, key);
}

struct tg_val *tg_valgetattr(const struct tg_val *v, const char *key)
{
	struct tg_val *r;
	
	TG_ASSERT(v != NULL, "Cannot get value attribute");
	TG_ASSERT(key != NULL, "Cannot get value attribute");

	if ((r = tg_valgetattrr(v, key)) == NULL)
		return tg_createval(TG_VAL_EMPTY);

	return tg_copyval(r);
}

struct tg_val *tg_valgetattre(const struct tg_val *v, const char *key,
	const struct tg_val *e)
{
	struct tg_val *r;
	
	TG_ASSERT(v != NULL, "Cannot get value attribute");
	TG_ASSERT(key != NULL, "Cannot get value attribute");

	if ((r = tg_valgetattrr(v, key)) == NULL)
		return tg_copyval(e);

	return tg_copyval(r);
}

static struct tg_val *tg_printtable(FILE *f, const struct tg_val *v)
{
	int i, j;
	int cols, rows;
	
	TG_ASSERT(f != NULL, "Cannot print table");
	TG_ASSERT(v != NULL, "Cannot print table");

	rows = tg_valgetattr(v, "rows")->intval;
	cols = tg_valgetattr(v, "cols")->intval;

	TG_NULLQUIT(tg_fprintf(f, "table{\n"));
	
	TG_NULLQUIT(tg_fprintf(f, "\t"));
	for (j = 0; j < cols * 15 + 1; ++j)
		TG_NULLQUIT(tg_fprintf(f, "-"));
	TG_NULLQUIT(tg_fprintf(f, "\n"));

	for (i = 0; i < rows; ++i) {
		struct tg_val *row;

		row = tg_arrgetr(v, i);
			
		TG_NULLQUIT(tg_fprintf(f, "\t"));
		for (j = 0; j < cols; ++j) {
			struct tg_val *cell;
			struct tg_val *hspan, *vspan;

			cell = tg_arrgetr(row, j);

			hspan = tg_valgetattrr(cell, "hspan");
			vspan = tg_valgetattrr(cell, "vspan");
			
			TG_NULLQUIT(tg_fprintf(f, "|%d,%d;",
				(hspan != NULL && hspan->intval > 0)
					? hspan->intval : 0,
				(vspan != NULL && vspan->intval > 0)
					? vspan->intval : 0));

			if (cell->type == TG_VAL_EMPTY)
				TG_NULLQUIT(tg_fprintf(f, "%-10s", "empty"));
			else if (cell->type == TG_VAL_INT)
				TG_NULLQUIT(tg_fprintf(f, "%-10d", cell->intval));
			else if (cell->type == TG_VAL_FLOAT)
				TG_NULLQUIT(tg_fprintf(f, "%-10f", cell->floatval));
			else if (cell->type == TG_VAL_STRING)
				TG_NULLQUIT(tg_fprintf(f, "%-10s", cell->strval.str));
		}
		TG_NULLQUIT(tg_fprintf(f, "|"));

		TG_NULLQUIT(tg_fprintf(f, "\n\t"));
		for (j = 0; j < cols * 15 + 1; ++j)
			TG_NULLQUIT(tg_fprintf(f, "-"));
		TG_NULLQUIT(tg_fprintf(f, "\n"));
	}

	TG_NULLQUIT(tg_fprintf(f, "}"));
	
	return tg_emptyval();
}

struct tg_val *tg_printval(FILE *f, const struct tg_val *v)
{
	const char *key;
	int isfirst;
	int i;
	
	TG_ASSERT(f != NULL, "Cannot print value");
	TG_ASSERT(v != NULL, "Cannot print value");

	switch (v->type) {
	case TG_VAL_DELETED:
		TG_NULLQUIT(tg_fprintf(f, "deleted"));
		break;
	case TG_VAL_EMPTY:
		TG_NULLQUIT(tg_fprintf(f, "empty"));
		break;
	case TG_VAL_INT:
		TG_NULLQUIT(tg_fprintf(f, "int{%d}", v->intval));
		break;
	case TG_VAL_FLOAT:
		TG_NULLQUIT(tg_fprintf(f, "float{%f}", v->floatval));
		break;
	case TG_VAL_STRING:
		TG_NULLQUIT(tg_fprintf(f, "string{%s}", v->strval.str));
		break;
	case TG_VAL_TABLE:
		TG_NULLQUIT(tg_printtable(f, v));
		break;
	case TG_VAL_ARRAY:
		TG_NULLQUIT(tg_fprintf(f, "array{"));
		
		isfirst = 1;

		for (i = 0; i < v->arrval.arr.cnt; ++i) {
			if (!isfirst) fprintf(f, ", ");

			isfirst = 0;

			if (tg_arrgetr(v, i)->ishashed)
				printf("*");

			TG_NULLQUIT(tg_printval(f, tg_arrgetr(v, i)));

			if (tg_arrgetr(v, i)->arrpos >= 0) {
				TG_NULLQUIT(tg_fprintf(f, ":%d",
					tg_arrgetr(v, i)->arrpos));
			}
		}

		TG_HASHFOREACH(struct tg_val, TG_HASH_ARRAY,
			v->arrval.hash, key,
			if (!isfirst) {
				TG_NULLQUIT(tg_fprintf(f, ", "));
			}
		
			isfirst = 0;
			
			TG_NULLQUIT(tg_fprintf(f, "%s:", key));

			TG_NULLQUIT(tg_printval(f, tg_arrgeth(v, key)));
		);
	
		TG_NULLQUIT(tg_fprintf(f, "}"));

		break;
	}


	if (v->attrs.count == 0)
		return tg_emptyval();
	
	fprintf(f, "[");
	TG_HASHFOREACH(struct tg_val, TG_HASH_ARRAY, v->attrs, key,
		fprintf(f, "%s:", key);
		tg_printval(f, tg_valgetattrr(v, key));
	);
	fprintf(f, "]");

	return tg_emptyval();
}

int tg_istrueval(const struct tg_val *v)
{
	TG_ASSERT(v != NULL, "Cannot check value's trueness");

	if (v->type == TG_VAL_INT)
		return v->intval;
	else if (v->type == TG_VAL_FLOAT)
		return (v->floatval != 0.0);
	else if (v->type == TG_VAL_STRING)
		return strlen(v->strval.str);
	else if (v->type == TG_VAL_ARRAY || TG_VAL_TABLE)
		return v->arrval.arr.cnt;

	return 0;
}

struct tg_val *tg_valor(const struct tg_val *v1,
	const struct tg_val *v2)
{
	struct tg_val *r;
	
	TG_ASSERT(v1 != NULL, "Cannot operate on numbers");
	TG_ASSERT(v2 != NULL, "Cannot operate on numbers");

	r = tg_createval(TG_VAL_INT);

	r->intval = tg_istrueval(v1) || tg_istrueval(v2);
	
	return r;
}

struct tg_val *tg_valand(const struct tg_val *v1,
	const struct tg_val *v2)
{
	struct tg_val *r;
	
	TG_ASSERT(v1 != NULL, "Cannot operate on numbers");
	TG_ASSERT(v2 != NULL, "Cannot operate on numbers");

	r = tg_createval(TG_VAL_INT);

	r->intval = tg_istrueval(v1) && tg_istrueval(v2);
	
	return r;
}

#define TG_NUMOPEXPR(r, v1, v2)					\
do {								\
	if (op == TG_NUMOP_ADD)		(r) = (v1) + (v2);	\
	else if (op == TG_NUMOP_SUB)	(r) = (v1) - (v2);	\
	else if (op == TG_NUMOP_MULT)	(r) = (v1) * (v2);	\
	else if (op == TG_NUMOP_DIV)	(r) = (v1) / (v2);	\
} while (0);

struct tg_val *_tg_numop(const struct tg_val *v1,
	const struct tg_val *v2, enum TG_NUMOP op)
{
	struct tg_val *r;
		
	TG_ASSERT(v1 != NULL, "Cannot operate on numbers");
	TG_ASSERT(v2 != NULL, "Cannot operate on numbers");

	TG_NULLQUIT(v1 = tg_typeprom2val(v1, v2->type, TG_VAL_FLOAT));
	TG_NULLQUIT(v2 = tg_typeprom2val(v2, v1->type, TG_VAL_FLOAT));

	r = tg_createval(v1->type);

	if (v1->type == TG_VAL_INT) {
		TG_NUMOPEXPR(r->intval, v1->intval, v2->intval);
	}
	else if (v1->type == TG_VAL_FLOAT) {
		TG_NUMOPEXPR(r->floatval, v1->floatval, v2->floatval);
	}
	else if (v1->type == TG_VAL_EMPTY) {
	}
	else {
		TG_SETERROR("%s", "Wrong value type.");
		return NULL;
	}

	return r;
}

#define TG_RELOPEXPR(r, v1, v2)					\
do {									\
	if (op == TG_RELOP_EQUAL)		(r) = (v1) == (v2);	\
	else if (op == TG_RELOP_NEQUAL)		(r) = (v1) != (v2);	\
	else if (op == TG_RELOP_LESS)		(r) = (v1) < (v2);	\
	else if (op == TG_RELOP_GREATER)	(r) = (v1) > (v2);	\
	else if (op == TG_RELOP_LESSEQ)		(r) = (v1) <= (v2);	\
	else if (op == TG_RELOP_GREATEREQ)	(r) = (v1) >= (v2);	\
} while (0);

struct tg_val *_tg_valcmp(const struct tg_val *v1,
	const struct tg_val *v2, enum TG_RELOP op)
{
	struct tg_val *r;
	
	TG_ASSERT(v1 != NULL, "Cannot compare values");
	TG_ASSERT(v2 != NULL, "Cannot compare values");

	TG_NULLQUIT(v1 = tg_typeprom2val(v1, v2->type, TG_VAL_STRING));
	TG_NULLQUIT(v2 = tg_typeprom2val(v2, v1->type, TG_VAL_STRING));

	r = tg_createval(TG_VAL_INT);

	if (v1->type == TG_VAL_INT) {
		TG_RELOPEXPR(r->intval, v1->intval, v2->intval);
	}
	else if (v1->type == TG_VAL_FLOAT) {
		TG_RELOPEXPR(r->intval, v1->floatval, v2->floatval);
	}
	else if (v1->type == TG_VAL_STRING) {
		int sr;

		sr = strcmp(v1->strval.str, v2->strval.str);
		
		TG_RELOPEXPR(r->intval, sr, 0);
	}
	else if (v1->type == TG_VAL_EMPTY) {
	}
	else {
		TG_SETERROR("%s", "Wrong value type.");
		return NULL;
	}

	return r;
}

struct tg_val *tg_valcat(const struct tg_val *v1,
	const struct tg_val *v2)
{
	struct tg_val *r;
	
	TG_ASSERT(v1 != NULL, "Cannot cancatenate strings");
	TG_ASSERT(v2 != NULL, "Cannot cancatenate strings");
	
	TG_NULLQUIT(v1 = tg_castval(v1, TG_VAL_STRING));
	TG_NULLQUIT(v2 = tg_castval(v2, TG_VAL_STRING));

	r = tg_copyval(v1);

	tg_dstraddstr(&(r->strval), v2->strval.str);

	return r;
}

struct tg_val *tg_substr(const struct tg_val *s,
	const struct tg_val *b, const struct tg_val *l)
{
	struct tg_dstring rs;
	struct tg_val *r;
	int bi, li;

	TG_NULLQUIT(s = tg_castval(s, TG_VAL_STRING));
	TG_NULLQUIT(b = tg_castval(b, TG_VAL_INT));
	TG_NULLQUIT(l = tg_castval(l, TG_VAL_INT));

	bi = b->intval;
	li = l->intval;

	if (li < 0)
		li = s->strval.len - bi;

	if (bi <= 0 || bi > li || bi + li > s->strval.len)
		return tg_emptyval();

	tg_dstrcreaten(&rs, s->strval.str + bi, li);
	r = tg_stringval(rs.str);
	tg_dstrdestroy(&rs);

	return r;
}

struct tg_val *tg_match(const struct tg_val *s,
	const struct tg_val *reg)
{
	struct tg_val *r;
	char buf[1024];
	regex_t regex;
	regmatch_t m;
	int ri;
	
	TG_NULLQUIT(s = tg_castval(s, TG_VAL_STRING));
	TG_NULLQUIT(reg = tg_castval(reg, TG_VAL_STRING));

	if ((ri = regcomp(&regex, reg->strval.str, 0)) != 0)
		goto error;

	if ((ri = regexec(&regex, s->strval.str, 1, &m, 0)) != 0) {
		if (ri == REG_NOMATCH) {
			regfree(&regex);
			return tg_emptyval();
		}

		goto error;
	}

	r = tg_intval(m.rm_so);

	regfree(&regex);

	return r;

error:
	regerror(ri, &regex, buf, sizeof(buf));	
	TG_SETERROR("Regular expression %s error: %s.",
		reg->strval.str, buf);

	regfree(&regex);
	
	return NULL;
}

static int tg_datepart2numn(const char *s, size_t l)
{
	char b[1024];

	if (l > 4)
		return 0;

	memcpy(b, s, l);
	b[l] = '\0';

	return atoi(b);
}

static int tg_isyearleap(int y)
{
	return ((y % 4) ? 0 : ((y % 100) ? 1 : ((y % 400) ? 0 : 1)));
}

static int _tg_monthend(int month, int year)
{
	int e[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	TG_ASSERT(month > 0 && month <= 12,
		"Month number should be from 1 to 12.");

	return e[month] + (month == 2 && tg_isyearleap(year) ? 1 : 0);
}

struct tg_val *_tg_rudate(int day, int month, int year)
{
	char buf[16];
	
	sprintf(buf, "%02d.%02d.%04d", day, month, year);

	return tg_stringval(buf);
}

struct tg_val *tg_rudate(const struct tg_val *d,
	const struct tg_val *m, const struct tg_val *y)
{
	int day, month, year;

	TG_NULLQUIT(d = tg_castval(d, TG_VAL_INT));
	TG_NULLQUIT(m = tg_castval(m, TG_VAL_INT));
	TG_NULLQUIT(y = tg_castval(y, TG_VAL_INT));

	day = d->intval;
	month = m->intval;
	year = y->intval;

	if (month <= 0 || month > 12 || day <= 0
		|| day > _tg_monthend(month, year))
		return tg_emptyval();

	return _tg_rudate(day, month, year);	
}

static int tg_dateformat(const char *s,
	int *d, int *m, int *y)
{
	const char *c;
	int day, month, year;

	if ((c = strchr(s, '-')) != NULL) {
		year = tg_datepart2numn(s, c - s);
		s = c + 1;
	
		if ((c = strchr(s, '-')) == NULL)
			return 0;
		month = tg_datepart2numn(s, c - s);
		s = c + 1;

		day = tg_datepart2numn(s, strlen(s));
	}
	else if ((c = strchr(s, '.')) != NULL) {
		day = tg_datepart2numn(s, c - s);
		s = c + 1;

		if ((c = strchr(s, '.')) == NULL)
			return 0;
		month = tg_datepart2numn(s, c - s);
		s = c + 1;
	
		year = tg_datepart2numn(s, strlen(s));
	}
	else if ((c = strchr(s, '/')) != NULL) {
		month = tg_datepart2numn(s, c - s);
		s = c + 1;
	
		if ((c = strchr(s, '/')) == NULL)
			return 0;
		day = tg_datepart2numn(s, c - s);
		s = c + 1;

		year = tg_datepart2numn(s, strlen(s));
	}

	if (month <= 0 || month > 12)
		return 0;

	if (day <= 0 || day > _tg_monthend(month, year))
		return 0;

	if (d != NULL) *d = day;
	if (m != NULL) *m = month;
	if (y != NULL) *y = year;

	return 1;
}

struct tg_val *tg_day(const struct tg_val *d)
{
	int day;

	TG_NULLQUIT(d = tg_castval(d, TG_VAL_STRING));
	
	if (!tg_dateformat(d->strval.str, &day, NULL, NULL))
		return tg_emptyval();

	return tg_intval(day);
}

struct tg_val *tg_month(const struct tg_val *d)
{
	int month;

	TG_NULLQUIT(d = tg_castval(d, TG_VAL_STRING));

	if (!tg_dateformat(d->strval.str, NULL, &month, NULL))
		return tg_emptyval();

	return tg_intval(month);
}

struct tg_val *tg_year(const struct tg_val *d)
{
	int year;

	TG_NULLQUIT(d = tg_castval(d, TG_VAL_STRING));

	if (!tg_dateformat(d->strval.str, NULL, NULL, &year))
		return tg_emptyval();

	return tg_intval(year);
}

struct tg_val *tg_weekday(const struct tg_val *d)
{
	int day, month, year;
	int j, k, h;

	TG_NULLQUIT(d = tg_castval(d, TG_VAL_STRING));

	if (!tg_dateformat(d->strval.str, &day, &month, &year))
		return tg_emptyval();

	if (month == 1 || month == 2) {
		month += 12;
		--year;
	}

	j = year / 100;
	k = year % 100;

	h = (day + (13 * (month + 1) / 5) + k
		+ (k / 4) + (j / 4) + 5 * j) % 7;

	return tg_intval(((h + 5) % 7) + 1);
}

struct tg_val *tg_monthend(const struct tg_val *month,
	const struct tg_val *year)
{
	TG_NULLQUIT(month = tg_castval(month, TG_VAL_INT));
	TG_NULLQUIT(year = tg_castval(year, TG_VAL_INT));

	return tg_intval(_tg_monthend(month->intval, year->intval));
}

struct tg_val *tg_datecmp(const struct tg_val *d1,
	const struct tg_val *d2)
{
	int day1, month1, year1;
	int day2, month2, year2;
	
	TG_NULLQUIT(d1 = tg_castval(d1, TG_VAL_STRING));
	TG_NULLQUIT(d2 = tg_castval(d2, TG_VAL_STRING));

	if (!tg_dateformat(d1->strval.str, &day1, &month1, &year1))
		return tg_emptyval();

	if (!tg_dateformat(d2->strval.str, &day2, &month2, &year2))
		return tg_emptyval();

	if (year1 < year2)
		return tg_intval(-1);
	else if (year1 > year2)
		return tg_intval(1);
	else if (month1 < month2)
		return tg_intval(-1);
	else if (month1 > month2)
		return tg_intval(1);
	else if (day1 < day2)
		return tg_intval(-1);
	else if (day1 > day2)
		return tg_intval(1);
		
	return tg_intval(0);
}

static int tg_datedays(int day, int month, int year)
{
	int m[13] = {0, 31, 59, 90, 120, 151, 181, 212,
		243, 273, 304, 334, 365};
	
	return (365 * (year - 1) + m[month] + day
		+ year / 4 + year / 400 - year / 100);
}

struct tg_val *tg_datediff(const struct tg_val *d1,
	const struct tg_val *d2, const struct tg_val *p)
{
	int day1, month1, year1;
	int day2, month2, year2;
	
	TG_NULLQUIT(d1 = tg_castval(d1, TG_VAL_STRING));
	TG_NULLQUIT(d2 = tg_castval(d2, TG_VAL_STRING));
	TG_NULLQUIT(p = tg_castval(p, TG_VAL_STRING));

	if (!tg_dateformat(d1->strval.str, &day1, &month1, &year1))
		return tg_emptyval();

	if (!tg_dateformat(d2->strval.str, &day2, &month2, &year2))
		return tg_emptyval();

	if (strcmp(p->strval.str, "day") == 0) {
		return tg_intval(tg_datedays(day2, month2, year2)
			- tg_datedays(day1, month1, year1));
	}
	else if (strcmp(p->strval.str, "month") == 0) {
		return tg_intval((year2 - year1) * 12
			+ month2 - month1);
	}
	else if (strcmp(p->strval.str, "year") == 0)
		return tg_intval(year2 - year1);

	return tg_emptyval();
}

struct tg_val *tg_dateadd(const struct tg_val *d,
	const struct tg_val *v, const struct tg_val *p)
{
	int day, month, year;
	
	TG_NULLQUIT(d = tg_castval(d, TG_VAL_STRING));
	TG_NULLQUIT(v = tg_castval(v, TG_VAL_INT));
	TG_NULLQUIT(p = tg_castval(p, TG_VAL_STRING));

	if (!tg_dateformat(d->strval.str, &day, &month, &year))
		return tg_emptyval();

	if (strcmp(p->strval.str, "day") == 0)
		day += v->intval;
	else if (strcmp(p->strval.str, "month") == 0)
		month += v->intval;
	else if (strcmp(p->strval.str, "year") == 0)
		year += v->intval;

	while (day <= 0 || month <= 0 || month > 12
			|| day > _tg_monthend(month, year)) {
		if (day <= 0) {
			year = month > 1 ? year : year - 1;
			month = month > 1 ? month - 1 : 12;
			day += _tg_monthend(month, year);
		}

		if (month <= 0) {
			month += 12;
			--year;
		}

		if (month > 0 && month <= 12
				&& day > _tg_monthend(month, year)) {
			day -= _tg_monthend(month, year);
			++month;
		}

		if (month > 12) {
			month -= 12;
			++year;
		}
	}

	return _tg_rudate(day, month, year);
}

void tg_arrset(struct tg_val *arr, int p, const struct tg_val *v)
{
	struct tg_val *newv;

	TG_ASSERT(arr != NULL, "Cannot set array value");
	TG_ASSERT(!tg_isscalar(arr->type), "Cannot set array value");
	TG_ASSERT(v != NULL, "Cannot set array value");
	
	if (p >= arr->arrval.arr.cnt) {
		int i;

		for (i = arr->arrval.arr.cnt; i < p; ++i) {
			newv = tg_emptyval();
			newv->arrpos = i;
			tg_darrset(&(arr->arrval.arr), i, &newv);
		}
	}

	newv = tg_copyval(v);
	newv->arrpos = p;

	tg_darrset(&(arr->arrval.arr), p, &newv);
}

void tg_arrpush(struct tg_val *arr, const struct tg_val *v)
{
	TG_ASSERT(arr != NULL, "Cannot push to value");
	TG_ASSERT(!tg_isscalar(arr->type), "Cannot push to value");
	TG_ASSERT(v != NULL, "Cannot push to value");

	tg_arrset(arr, arr->arrval.arr.cnt, v);
}

struct tg_val *tg_arrgetr(const struct tg_val *v, int i)
{
	struct tg_val **r;

	TG_ASSERT(v != NULL, "Cannot get array value");
	TG_ASSERT(!tg_isscalar(v->type), "Cannot get array value");

	r = tg_darrget(&(v->arrval.arr), i);

	if (r == NULL)
		return NULL;

	return *((struct tg_val **) r);
}

struct tg_val *tg_arrgetre(struct tg_val *v, int i,
	const struct tg_val *e)
{
	struct tg_val *r;

	TG_ASSERT(v != NULL, "Cannot get array value");
	TG_ASSERT(!tg_isscalar(v->type), "Cannot get array value");
	TG_ASSERT(e != NULL, "Cannot get array value");

	if ((r = tg_arrgetr(v, i)) == NULL) {
		tg_arrset(v, i, e);
		return tg_arrgetr(v, i);
	}

	return r;
}

struct tg_val *tg_arrget(const struct tg_val *v, int i)
{
	struct tg_val *r;

	TG_ASSERT(v != NULL, "Cannot get array value");
	TG_ASSERT(!tg_isscalar(v->type), "Cannot get array value");

	if ((r = tg_arrgetr(v, i)) == NULL)
		return tg_emptyval();

	return tg_copyval(r);
}

struct tg_val *tg_arrgete(struct tg_val *v, int i,
	const struct tg_val *e)
{
	struct tg_val *r;

	TG_ASSERT(v != NULL, "Cannot get array value");
	TG_ASSERT(!tg_isscalar(v->type), "Cannot get array value");
	TG_ASSERT(e != NULL, "Cannot get array value");

	if ((r = tg_arrgetr(v, i)) == NULL)
		return tg_copyval(e);

	return tg_copyval(r);
}

void tg_arrseth(struct tg_val *arr, const char *k, const struct tg_val *v)
{
	struct tg_val *newv;

	TG_ASSERT(arr != NULL, "Cannot set array value");
	TG_ASSERT(!tg_isscalar(arr->type), "Cannot set array value");
	TG_ASSERT(v != NULL, "Cannot set array value");

	newv = tg_copyval(v);
	newv->ishashed = 1;
	newv->arrpos = arr->arrval.arr.cnt;
	
	tg_darrpush(&(arr->arrval.arr), &newv);
	
	tg_hashset(TG_HASH_ARRAY, &(arr->arrval.hash), k, newv);	
}

struct tg_val *tg_arrgethr(const struct tg_val *v, const char *k)
{
	TG_ASSERT(v != NULL, "Cannot get array value");
	TG_ASSERT(!tg_isscalar(v->type), "Cannot get array value");
	TG_ASSERT(k != NULL, "Cannot get array value");

	return tg_hashget(TG_HASH_ARRAY, &(v->arrval.hash), k);
}

struct tg_val *tg_arrgethre(struct tg_val *v, const char *k,
	const struct tg_val *e)
{
	struct tg_val *r;

	TG_ASSERT(v != NULL, "Cannot get array value");
	TG_ASSERT(!tg_isscalar(v->type), "Cannot get array value");
	TG_ASSERT(k != NULL, "Cannot get array value");

	if ((r = tg_arrgethr(v, k)) == NULL) {
		tg_arrseth(v, k, e);
		r = tg_arrgethr(v, k);
	}

	return r;
}

struct tg_val *tg_arrgeth(const struct tg_val *v, const char *k)
{
	struct tg_val *r;

	if ((r = tg_arrgethr(v, k)) == NULL)
		return tg_emptyval();

	return tg_copyval(r);
}

struct tg_val *tg_arrgethe(const struct tg_val *v, const char *k,
	const struct tg_val *e)
{
	struct tg_val *r;
	
	if ((r = tg_arrgeth(v, k))->type == TG_VAL_EMPTY)
		return tg_copyval(e);

	return tg_copyval(r);
}

static void tg_copytable(struct tg_val *dst, const struct tg_val *src,
	int offr, int offc, int rows, int cols)
{
	int i, j;
	int srcrows, srccols;

	TG_ASSERT(dst != NULL, "Cannot copy table");
	TG_ASSERT(dst->type == TG_VAL_TABLE, "Cannot copy table");
	TG_ASSERT(src != NULL, "Cannot copy table");
	TG_ASSERT(src->type == TG_VAL_TABLE, "Cannot copy table");

	srcrows = tg_valgetattr(src, "rows")->intval;
	srccols = tg_valgetattr(src, "cols")->intval;	

	for (i = 0; i < rows; ++i) {
		struct tg_val *dstrow, *srcrow;

		dstrow = tg_arrgetre(dst, offr + i, tg_arrval());
		srcrow = tg_arrget(src, i);

		for (j = 0; j < cols; ++j) {
			if (i >= srcrows || j >= srccols) {
				tg_arrset(dstrow, offc + j,
					tg_emptyval());	
				continue;
			}

			tg_arrset(dstrow, offc + j,
				tg_arrgete(srcrow, j, tg_emptyval()));
		}
	}

	tg_valsetattr(dst, "rows", tg_intval(offr + rows));
	tg_valsetattr(dst, "cols", tg_intval(offc + cols));
}

static struct tg_val *tg_tablegetcellr(struct tg_val *t,
	int row, int col)
{
	TG_ASSERT(t != NULL, "Cannot get table's cell");
	TG_ASSERT(t->type == TG_VAL_TABLE, "Cannot get table's cell");

	return tg_arrgetr(tg_arrgetr(t, row), col);
}

static void tg_tablesetcellr(struct tg_val *t,
	int r, int c, struct tg_val *v)
{
	struct tg_val *row;

	TG_ASSERT(t != NULL, "Cannot get table's cell");
	TG_ASSERT(t->type == TG_VAL_TABLE, "Cannot get table's cell");
	TG_ASSERT(v != NULL, "Cannot get table's cell");

	if ((row = tg_arrgetre(t, r, tg_arrval())) == NULL)
		return;
	
	tg_arrset(row, c, v);

	return;
}

#define TG_CELLINDEX(i, j, vert) \
	((vert) ? (j) : (i)), ((vert) ? (i) : (j))

struct tg_val *tg_tablespan(struct tg_val *t, int newside, int vert)
{
	struct tg_val *r;
	int rows, cols;
	int oldside;
	int otherside;
	int i, j;

	TG_ASSERT(t != NULL, "Cannot span table");
	TG_ASSERT(t->type == TG_VAL_TABLE, "Cannot span table");

	r = tg_createval(TG_VAL_TABLE);

	rows = tg_valgetattr(t, "rows")->intval;
	cols = tg_valgetattr(t, "cols")->intval;	

	oldside = vert ? rows : cols;

	tg_valsetattr(r, "rows", tg_intval(vert ? newside : rows));
	tg_valsetattr(r, "cols", tg_intval(vert ? cols : newside));

	if (newside <= oldside)
		return tg_copyval(t);

	otherside = vert ? cols : rows;

	for (i = 0; i < otherside; ++i) {
		int pj, p;

		p = pj = -1;
		for (j = 0; j < newside; ++j) {
			int oldhspani, oldvspani;
			struct tg_val *oldv, *newv;
			int c;

			c = (int) (oldside * j / newside);

			oldv = tg_tablegetcellr(t, TG_CELLINDEX(i, c, vert));
			oldhspani = tg_valgetattrre(oldv, "hspan", tg_intval(0))->intval;
			oldvspani = tg_valgetattrre(oldv, "vspan", tg_intval(0))->intval;

			if ((vert ? oldvspani : oldhspani) < 0 || c == p) {
				struct tg_val *newhspan, *newvspan;

				tg_tablesetcellr(
					r,
					TG_CELLINDEX(i, j, vert),
					tg_emptyval());

				newv = tg_tablegetcellr(r, TG_CELLINDEX(i, j, vert));
				newhspan = tg_valgetattrre(newv, "hspan", tg_intval(0));
				newvspan = tg_valgetattrre(newv, "vspan", tg_intval(0));

				if (pj >= 0) {
					++(tg_valgetattrre(
						tg_tablegetcellr(r, TG_CELLINDEX(i, pj, vert)),
						(vert ? "vspan" : "hspan"),
						tg_intval(0))->intval
					);
				}

				newhspan->intval = (vert ? oldhspani : -1);
				newvspan->intval = (vert ? -1 : oldvspani);
			}
			else {
				p = c;
				pj = j;
			
				tg_tablesetcellr(
					r,
					TG_CELLINDEX(i, j, vert),
					oldv);

				newv = tg_tablegetcellr(r, TG_CELLINDEX(i, j, vert));
				tg_valgetattrre(newv, "hspan", tg_intval(0))->intval = (vert ? oldhspani : 1);
				tg_valgetattrre(newv, "vspan", tg_intval(0))->intval = (vert ? 1 : oldvspani);
			}
		}
	}

	return r;
}

struct tg_val *tg_valnextto(const struct tg_val *v1,
	const struct tg_val *v2, int vert, int span)
{
	int t1rows, t1cols, t2rows, t2cols, maxrows, maxcols;
	struct tg_val *vc1, *vc2;
	struct tg_val *r;

	TG_ASSERT(v1 != NULL, "Cannot make complex table");
	TG_ASSERT(v2 != NULL, "Cannot make complex table");

	TG_NULLQUIT((vc1 = tg_castval(v1, TG_VAL_TABLE)));
	TG_NULLQUIT((vc2 = tg_castval(v2, TG_VAL_TABLE))); 

	t1rows = tg_valgetattr(vc1, "rows")->intval;
	t1cols = tg_valgetattr(vc1, "cols")->intval;
	t2rows = tg_valgetattr(vc2, "rows")->intval;
	t2cols = tg_valgetattr(vc2, "cols")->intval;

	if (span) {
		vc1 = tg_tablespan(vc1, vert ? t2cols : t2rows, !vert);
		vc2 = tg_tablespan(vc2, vert ? t1cols : t1rows, !vert);
	}

	maxrows = (t1rows > t2rows) ? t1rows : t2rows;
	maxcols = (t1cols > t2cols) ? t1cols : t2cols;

	r = tg_createval(TG_VAL_TABLE);

	tg_copytable(r, vc1, 0, 0,
		vert ? t1rows : maxrows, vert ? maxcols : t1cols);

	tg_copytable(r, vc2,
		vert ? t1rows : 0, vert ? 0 : t1cols, 
		vert ? t2rows : maxrows, vert ? maxcols : t2cols);

	return r;
}

struct tg_val *tg_fprintf(FILE *file, const char *format, ...)
{
	va_list args;
	int r;

	va_start(args, format);

	r = vfprintf(file, format, args);

	va_end(args);

	if (r < 0) {
		TG_SETERROR("Can't print with fprintf: %s.",
			strerror(errno));

		return NULL;
	}

	return tg_emptyval();
}
