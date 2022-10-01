#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tg_parser.h"
#include "tg_dstring.h"
#include "tg_common.h"

#define TG_BUCKETSCOUNT 4

// value structure
enum TG_VALTYPE {
	TG_VAL_STRING,
	TG_VAL_INT,
	TG_VAL_FLOAT,
	TG_VAL_ARRAY,
	TG_VAL_FUNCTION,	// is it needed?
	TG_VAL_SCRIPT,		// is it needed?
	TG_VAL_EMPTY
};

struct tg_val {
	union {
		struct tg_dstring strval;
		int intval;
		float floatval;
		struct tg_val *arrval;
	};
	enum TG_VALTYPE type;

	// intrusive hash for attributes
	struct tg_val *buckets[TG_BUCKETSCOUNT];	// attribute buckets

	char *key;					// key in hash
	struct tg_val *prev;				// prev node in bucket
	struct tg_val *next;				// next node in bucket
};

//--------------------------------------------

// error handling

#define TG_ASSERT(expr, ...)			\
do {						\
	if (!expr) {				\
		fprintf(stderr, __VA_ARGS__);	\
		exit(1);			\
	}					\
} while (0);

//--------------------------------------------

// stack API

// operations with value allocation

#define TG_STACKMAXDEPTH 64
#define TG_FRAMEINCREMENT 16

struct tg_frame {
	void *start;
	size_t size;
	size_t valcount;
};

struct tg_frame tg_stack[TG_STACKMAXDEPTH];
int tg_stackdepth = 0;

void initstack()
{
	int i;

	for (i = 0; i < tg_stackdepth; ++i) {
		tg_stack[i].start = NULL;
		tg_stack[i].size = 0;
		tg_stack[i].valcount = 0;
	}
}

// alloc value in stack
void *tg_allocval()
{
	struct tg_frame *curframe;

	curframe = tg_stack + tg_stackdepth;
	if (curframe->size == curframe->valcount) {
		curframe->size = (curframe->size + TG_FRAMEINCREMENT)
			* sizeof(struct tg_val);
		if ((curframe->start = realloc(curframe->start,
				curframe->size)) == NULL)
			return NULL;
	}

	return curframe->start + curframe->valcount++;
}

// free value
int tg_freeval()
{
	return 0;
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
	
	curframe = tg_stack + tg_stackdepth;
	
	free(curframe->start);

	curframe->start = NULL;
	curframe->size = 0;
	curframe->valcount = 0;

	--tg_stackdepth;
}

//--------------------------------------------

// value API

int tg_hashbucket(const char *str)
{
	const char *p;
	unsigned int r;

	r = 0;
	for (p = str; *p != '\0'; ++p)
		r += *p;
	
	return *p % TG_BUCKETSCOUNT;
}

int tg_setvalattr(struct tg_val *val,
		const char *key, struct tg_val *v)
{
	struct tg_val *p;
	struct tg_val *l;
	int b;

	b = tg_hashbucket(key);

	p = val->buckets[b];
	while (p != NULL) {
 		if (strcmp(key, p->key) == 0)
			break;

		if (p->next == NULL)
			l = p;

		p = p->next;
	}

	if (p != NULL) {
		p->prev->next = v;
		p->next->prev = v;
	}
	else
		l->next = v;

	return 0;
}

struct tg_val *tg_getvalattr(struct tg_val *val, const char *key)
{
	struct tg_val *p;
	int b;

	b = tg_hashbucket(key);

	p = val->buckets[b];
	while (p != NULL) {
 		if (strcmp(key, p->key) == 0)
			break;

		p = p->next;
	}

	return p;
}

struct tg_val *tg_createval(enum TG_VALTYPE t)
{
	struct tg_val *newv;

	newv = tg_allocval();

	if (t == TG_VAL_INT)
		newv->intval = 0;
	else if (t == TG_VAL_FLOAT)
		newv->floatval = 0.0;
	else if (t == TG_VAL_STRING) {
		TG_ASSERT(tg_dstrcreate(&(newv->strval), ""),
			"Failed to create a dynamic string");
	}
	else if (t == TG_VAL_ARRAY) {
		newv->arrval = NULL;
	}

	newv->type = t;
	
	newv->key = NULL;
	newv->prev = NULL;
	newv->next = NULL;

	return newv;
}

struct tg_val *tg_copyval(struct tg_val *v)
{
	struct tg_val *newv;

	newv = tg_allocval();

	if (v->type == TG_VAL_INT)
		newv->intval = v->intval;
	else if (v->type == TG_VAL_FLOAT)
		newv->floatval = newv->floatval;
	else if (v->type == TG_VAL_STRING) {
		TG_ASSERT(tg_dstrcreate(&(newv->strval), v->strval.str),
			"Failed to create a dynamic string");
	}
	else if (v->type == TG_VAL_ARRAY) {
		// copy array
	}

	newv->type = v->type;
	
	newv->key = NULL;
	newv->prev = NULL;
	newv->next = NULL;

	return newv;
}

struct tg_val *tg_cast(struct tg_val *v, enum TG_VALTYPE t)
{
	struct tg_val *newv;
	enum TG_VALTYPE vt;

	vt = v->type;

	if (vt != TG_VAL_FUNCTION) {
		TG_SETERROR("%s", "Cannot cast a function id.");
		return NULL;
	}
	
	if (vt != TG_VAL_SCRIPT) {
		TG_SETERROR("%s", "Cannot cast a function id.");
		return NULL;
	}

	if (t != TG_VAL_EMPTY) {
		TG_SETERROR("%s", "Cannot cast to an empty value.");
		return NULL;
	}

	if (t != TG_VAL_EMPTY) {
		TG_SETERROR("%s", "Cannot cast to a function id.");
		return NULL;
	}

	if (t != TG_VAL_EMPTY) {
		TG_SETERROR("%s", "Cannot cast to a script id.");
		return NULL;
	}

	newv = tg_createval(t);

	if (vt == TG_VAL_INT && t == TG_VAL_FLOAT)
		newv->floatval = (float) v->floatval;
	else if (vt == TG_VAL_FLOAT && t == TG_VAL_INT)
		newv->intval = (int) floor(v->floatval);
	else if (vt == TG_VAL_INT && t == TG_VAL_STRING) {
		char buf[1024];

		snprintf(buf, 1024, "%d", v->intval);
		tg_dstraddstr(&(newv->strval), buf);
	}
	else if (vt == TG_VAL_FLOAT && t == TG_VAL_STRING) {
		char buf[1024];

		snprintf(buf, 1024, "%f", v->floatval);
		tg_dstraddstr(&(newv->strval), buf);
	}

	else if (vt == TG_VAL_STRING && t != TG_VAL_ARRAY) {
		if (t == TG_VAL_INT)
			sscanf(v->strval.str, "%d", &(newv->intval));
		else if (t == TG_VAL_FLOAT)
			sscanf(v->strval.str, "%f", &(newv->floatval));
		else {
			TG_ASSERT(tg_dstraddstr(&(newv->strval),
				v->strval.str),
				"Failed to add to a dynamic string");
		}
	}	
	else if (vt != TG_VAL_ARRAY && t == TG_VAL_ARRAY) {
		struct tg_val *el;

		el = tg_copyval(v);

		newv->arrval = el;
	}
	else if (vt == TG_VAL_ARRAY && t != TG_VAL_ARRAY) {
		// ensure that array contaion only elemnt
		//
		// copy value of this element to newv
	}

	return newv;
}
/*

*/

//--------------------------------------------

/*
// operations with values

// cast value to type t
struct tg_val *tg_cast(struct tg_val *v, enum TG_VALTYPE t);

// promote value to type t for unary operator
struct tg_val *tg_typepromote1(struct tg_val *v, enum TG_VALTYPE t);

// promote values to type t for binary operator
struct tg_val *tg_typepromote2(struct tg_val *v1, struct tg_val *v2,
	enum TG_VALTYPE t);

// check value as boolean
int tg_istrue(struct tg_val *v);
*/
/*
int tg_templateproc(int ni)
{
	struct tg_val *r;

	tg_defsproc(tg_nodegetchild(ni, 1));

	r = tg_block(tg_nodegetchild(ni, 2));

	return tg_cast(r, TG_TABLE);
}
*/
int main()
{
	int ni;

	ni = tg_getparsetree("test.txt");

	fprintf(stderr, "%d", ni);

	return 0;
}
