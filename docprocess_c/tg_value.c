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

struct tg_frame {
	struct tg_darray blocks;
	size_t valcount;
};

struct tg_frame tg_stack[TG_STACKMAXDEPTH];
int tg_stackdepth = 0;

void tg_initstack()
{
	int i;

	for (i = 0; i < TG_STACKMAXDEPTH; ++i) {
		tg_darrinit(&(tg_stack[i].blocks),
			sizeof(struct tg_val *));
		tg_stack[i].valcount = 0;
	}
}

// alloc value in stack
static void *tg_allocval()
{
	struct tg_frame *curframe;
	struct tg_val *p;
	
	curframe = tg_stack + tg_stackdepth;

	if (curframe->valcount / TG_FRAMEBLOCKSIZE >= curframe->blocks.cnt) {	
		p = malloc(sizeof(struct tg_val) * TG_FRAMEBLOCKSIZE);
		TG_ASSERT(p != NULL,
			"Cannot allocate new value in stack.");

		tg_darrpush(&(curframe->blocks), &p);
	}

	p = *((struct tg_val **) tg_darrget(&(curframe->blocks),
		curframe->blocks.cnt - 1));

	return p + ((curframe->valcount++) % TG_FRAMEBLOCKSIZE);
}

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
	struct tg_frame *curframe;
	struct tg_val *p;
	
	curframe = tg_stack + tg_stackdepth;

	while (tg_darrpop(&(curframe->blocks), &p) >= 0)
		free(p);

	curframe->valcount = 0;
	
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
	else if (t == TG_VAL_ARRAY)
		tg_inithash(TG_HASH_ARRAY, &(newv->arrval));

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
		const char *key;

		tg_inithash(TG_HASH_ARRAY, &(newv->arrval));

		TG_HASHFOREACH(struct tg_val, TG_HASH_ARRAY,
			v->arrval, key,
			tg_hashset(TG_HASH_ARRAY, &(newv->arrval),
				key, tg_copyval(p));
		);
	}

	newv->type = v->type;

	return newv;
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
	else if (vt == TG_VAL_STRING && t != TG_VAL_ARRAY) {
		newv = tg_createval(t);

		if (t == TG_VAL_INT)
			sscanf(v->strval.str, "%d", &(newv->intval));
		else if (t == TG_VAL_FLOAT)
			sscanf(v->strval.str, "%f", &(newv->floatval));
		else
			tg_dstraddstr(&(newv->strval), v->strval.str);
	}	
	else if (vt != TG_VAL_ARRAY && t == TG_VAL_ARRAY) {
		newv = tg_createval(t);

		tg_arrpush(newv, v);
	}
	else if (vt == TG_VAL_ARRAY && t != TG_VAL_ARRAY) {
		if (v->arrval.count != 1) {
			TG_SETERROR("%s%s%s", "Trying to cast array,",
				" that has more than to element to",
				" a scalar type");
			return NULL;
		}

		newv = tg_castval(v->arrval.last, t);	
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
		return v->arrval.count;

	return 0;
}

void tg_arrpush(struct tg_val *arr, struct tg_val *v)
{
	char buf[1024];
	
	TG_ASSERT(arr->type == TG_VAL_ARRAY,
		"Trying to push to a non-array value.");

	snprintf(buf, 1024, "%ld", arr->arrval.count);
	
	tg_hashset(TG_HASH_ARRAY, &(arr->arrval), buf, tg_copyval(v));
}
