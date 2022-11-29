#include "tg_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tg_dstring.h"
#include "tg_darray.h"
#include "tg_common.h"

#define TG_STACKMAXDEPTH 64
#define TG_FRAMEBLOCKSIZE 32

TG_HASHED(struct tg_val, TG_HASH_ARRAY)

struct tg_allocator tg_stack[TG_STACKMAXDEPTH];
int tg_stackdepth = 0;

void tg_initstack()
{
	int i;

	for (i = 0; i < TG_STACKMAXDEPTH; ++i)
		tg_allocinit(tg_stack + i, sizeof(struct tg_val));
}

#define tg_allocval() tg_alloc(tg_stack + tg_stackdepth)

// start frame, call every time
// when block starts
int tg_startframe()
{
	if (tg_stackdepth == TG_STACKMAXDEPTH) {
		// error
	}
		
	++tg_stackdepth;

	return 0;
}

// end frame, destroy all variables,
// allocated in this frame
void tg_endframe()
{
	tg_allocdestroy(tg_stack + tg_stackdepth);

	--tg_stackdepth;
}

struct tg_val *tg_createval(enum TG_VALTYPE t)
{
	struct tg_val *newv;

	newv = tg_allocval();

	if (t == TG_VAL_INT)
		newv->intval = 0;
	else if (t == TG_VAL_FLOAT)
		newv->floatval = 0.0;
	else if (t == TG_VAL_STRING)
		tg_dstrcreate(&(newv->strval), "");
	else if (t == TG_VAL_ARRAY) {
		tg_inithash(TG_HASH_ARRAY, &(newv->arrval.hash));
		tg_darrinit(&(newv->arrval.arr),
			sizeof(struct tg_val *));
	}

	newv->type = t;

	return newv;
}

struct tg_val *tg_intval(int v)
{
	struct tg_val *newv;

	newv = tg_allocval();

	newv->intval = v;
	newv->type = TG_VAL_INT;
	
	return newv;
}

struct tg_val *tg_floatval(float v)
{
	struct tg_val *newv;

	newv = tg_allocval();

	newv->floatval = v;
	newv->type = TG_VAL_FLOAT;
	
	return newv;
}

struct tg_val *tg_stringval(const char *v)
{
	struct tg_val *newv;

	newv = tg_allocval();

	tg_dstrcreate(&(newv->strval), v);
	newv->type = TG_VAL_STRING;
	
	return newv;
}

struct tg_val *tg_copyval(struct tg_val *v)
{
	struct tg_val *newv;

	newv = tg_allocval();

	if (v->type == TG_VAL_INT)
		newv->intval = v->intval;
	else if (v->type == TG_VAL_FLOAT)
		newv->floatval = v->floatval;
	else if (v->type == TG_VAL_STRING)
		tg_dstrcreate(&(newv->strval), v->strval.str);
	else if (v->type == TG_VAL_ARRAY) {
		int i;

		tg_inithash(TG_HASH_ARRAY, &(newv->arrval.hash));
		tg_darrinit(&(newv->arrval.arr),
			sizeof(struct tg_val *));

		for (i = 0; i < v->arrval.arr.cnt; ++i) {
			struct tg_val *vv;

			vv = *((struct tg_val **) tg_darrget(
				&(v->arrval.arr), i));
			vv = tg_copyval(vv);

			tg_darrpush(&(newv->arrval.arr), &vv);
		}
	}

	newv->type = v->type;

	return newv;
}

// is_scalar(enum TG_VALTYPE t) ?

int tg_isscalar(enum TG_VALTYPE t)
{
	return (t != TG_VAL_ARRAY && t != TG_VAL_TABLE);
}

struct tg_val *tg_castval(struct tg_val *v, enum TG_VALTYPE t)
{
	struct tg_val *newv;
	enum TG_VALTYPE vt;

	vt = v->type;

	if (vt == TG_VAL_FUNCTION) {
		TG_SETERROR("%s", "Cannot cast a function id.");
		return NULL;
	}
	
	if (vt == TG_VAL_SCRIPT) {
		TG_SETERROR("%s", "Cannot cast a function id.");
		return NULL;
	}

	if (t == TG_VAL_EMPTY) {
		TG_SETERROR("%s", "Cannot cast to an empty value.");
		return NULL;
	}

	if (t == TG_VAL_FUNCTION) {
		TG_SETERROR("%s", "Cannot cast to a function id.");
		return NULL;
	}

	if (t == TG_VAL_SCRIPT) {
		TG_SETERROR("%s", "Cannot cast to a script id.");
		return NULL;
	}

	if (vt == TG_VAL_EMPTY)
		newv = tg_createval(t);
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
		// create 2d array
		// copy v to [0,0]
	}
	else if (vt == TG_VAL_ARRAY && tg_isscalar(t)) {
		struct tg_val **vv;

		if (v->arrval.arr.cnt != 1) {
			TG_SETERROR("%s%s%s", "Trying to cast array,",
				" that has more than to element to",
				" a scalar type");
			return NULL;
		}

		vv = tg_darrget(&(v->arrval.arr), 0);

		newv = tg_castval(*vv, t);
	}
	else if (vt == TG_VAL_TABLE && tg_isscalar(t)) {
		struct tg_val **vv;

		if (v->arrval.arr.cnt != 1) {
			TG_SETERROR("%s%s%s", "Trying to cast array,",
				" that has more than to element to",
				" a scalar type");
			return NULL;
		}

		vv = tg_darrget(&(v->arrval.arr), 0);
		vv = tg_darrget(&((*vv)->arrval.arr), 0);

		newv = tg_castval(*vv, t);
	}
	else if (vt == TG_VAL_TABLE && t == TG_VAL_ARRAY) {
		newv = tg_copyval(v);
		newv->type = TG_VAL_TABLE;
	}
	else if (vt == TG_VAL_ARRAY && t == TG_VAL_TABLE) {
		// assert that array is table-like
	}
	else
		newv = tg_copyval(v);

	return newv;
}

struct tg_val *tg_typeprom1val(struct tg_val *v, enum TG_VALTYPE mtype)
{
	if (v->type > mtype)
		return tg_castval(v, mtype);

	return tg_copyval(v);
}

struct tg_val *tg_typeprom2val(struct tg_val *v1,
	enum TG_VALTYPE v2type, enum TG_VALTYPE mtype)
{
	if (v1->type > mtype || v2type > mtype)
		return tg_castval(v1, mtype);
	else if (v1->type < v2type)
		return tg_castval(v1, v2type);

	return tg_copyval(v1);
}

int tg_istrueval(struct tg_val *v)
{
	if (v->type == TG_VAL_INT)
		return v->intval;
	else if (v->type == TG_VAL_FLOAT)
		return (v->floatval != 0.0);
	else if (v->type == TG_VAL_STRING)
		return strlen(v->strval.str);
	else if (v->type == TG_VAL_ARRAY)
		return v->arrval.arr.cnt;

	return 0;
}

void tg_arrpush(struct tg_val *arr, struct tg_val *v)
{
	struct tg_val *newv;
	
	newv = tg_copyval(v);

	tg_darrpush(&(arr->arrval.arr), &newv);
}

void tg_printval(FILE *f, struct tg_val *v)
{
	int isfirst;
	int i;
	struct tg_val **vv;

	switch (v->type) {
	case TG_VAL_EMPTY:
		fprintf(f, "empty");
		break;
	case TG_VAL_FUNCTION:
		fprintf(f, "function");
		break;
	case TG_VAL_SCRIPT:
		fprintf(f, "script");
		break;
	case TG_VAL_INT:
		fprintf(f, "int{%d}", v->intval);
		break;
	case TG_VAL_FLOAT:
		fprintf(f, "float{%f}", v->floatval);
		break;
	case TG_VAL_STRING:
		fprintf(f, "string{%s}", v->strval.str);
		break;
	case TG_VAL_TABLE:
	case TG_VAL_ARRAY:
		fprintf(f, "array{");
		
		isfirst = 1;

		for (i = 0; i < v->arrval.arr.cnt; ++i) {
			if (!isfirst) fprintf(f, ", ");

			isfirst = 0;

			vv = tg_darrget(&(v->arrval.arr), i);

			tg_printval(f, *vv);
		}
			

		fprintf(f, "}");

		break;
	}
}


struct tg_val *tg_valcat(struct tg_val *v1, struct tg_val *v2)
{
	struct tg_val *r;
	
	if ((v1 = tg_castval(v1, TG_VAL_STRING)) == NULL)
		return NULL;

	if ((v2 = tg_castval(v2, TG_VAL_STRING)) == NULL)
		return NULL;

	r = tg_copyval(v1);

	tg_dstraddstr(&(r->strval), v2->strval.str);

	return r;
}

#define TG_NUMOPEXPR(r, v1, v2)					\
do {								\
	if (op == TG_NUMOP_ADD)		(r) = (v1) + (v2);	\
	else if (op == TG_NUMOP_SUB)	(r) = (v1) - (v2);	\
	else if (op == TG_NUMOP_MULT)	(r) = (v1) * (v2);	\
	else if (op == TG_NUMOP_DIV)	(r) = (v1) / (v2);	\
} while (0);

struct tg_val *_tg_numop(struct tg_val *v1, struct tg_val *v2,
	enum TG_NUMOP op)
{
	struct tg_val *r;
	
	if ((v1 = tg_typeprom2val(v1, v2->type, TG_VAL_FLOAT)) == NULL)
		return NULL;

	if ((v2 = tg_typeprom2val(v2, v1->type, TG_VAL_FLOAT)) == NULL)
		return NULL;

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

struct tg_val *tg_valor(struct tg_val *v1, struct tg_val *v2)
{
	struct tg_val *r;

	r = tg_createval(TG_VAL_INT);

	r->intval = tg_istrueval(v1) || tg_istrueval(v2);
	
	return r;
}

struct tg_val *tg_valand(struct tg_val *v1, struct tg_val *v2)
{
	struct tg_val *r;

	r = tg_createval(TG_VAL_INT);

	r->intval = tg_istrueval(v1) && tg_istrueval(v2);
	
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

struct tg_val *_tg_valcmp(struct tg_val *v1, struct tg_val *v2,
	enum TG_RELOP op)
{
	struct tg_val *r;

	if ((v1 = tg_typeprom2val(v1, v2->type, TG_VAL_STRING)) == NULL)
		return NULL;

	if ((v2 = tg_typeprom2val(v2, v1->type, TG_VAL_STRING)) == NULL)
		return NULL;

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

struct tg_val *tg_valnextto(struct tg_val *v1, struct tg_val *v2,
	int span)
{
//	struct tg_val r;

	if ((v1 = tg_typeprom2val(v1, v2->type, TG_VAL_TABLE)) == NULL)
		return NULL;

	if ((v2 = tg_typeprom2val(v2, v1->type, TG_VAL_TABLE)) == NULL)
		return NULL;

	if (span) {

	}

	// tg_table(struct tg_val)
	// for every key in v1:
	// 	cast v1[i]
	// 		if v1[i] == TG_VAL_ARRAY
	// 			return tg_val_none;

	return NULL;
}
/*
struct tg_val *tg_valindex(struct tg_val *v1, struct tg_val *v2)
{
	struct tg_val r;

	return NULL;
}
*/

struct tg_val *tg_valattr(struct tg_val *v1, const char *key)
{
	struct tg_val *r;

	if ((r = tg_hashget(TG_HASH_ARRAY, v1->attrs, key)) == NULL) {
		TG_SETERROR("No such attribute: %s", key);
		return NULL;
	}

	return r;
}
