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

#define tg_allocval() tg_alloc(tg_stack + tg_stackdepth)

// start frame, call every time
// when block starts
int tg_startframe()
{
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
}

#define tg_isscalar(t) ((t) != TG_VAL_ARRAY && (t) != TG_VAL_TABLE)

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

	return newv;
}

void tg_freeval(struct tg_val *v)
{
	int i;
	const char *key;

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

	newv = tg_allocval();
	
	tg_inithash(TG_HASH_ARRAY, &(newv->attrs));

	tg_dstrcreate(&(newv->strval), v);
	newv->type = TG_VAL_STRING;
	
	return newv;
}

struct tg_val *tg_copyval(struct tg_val *v)
{
	struct tg_val *newv;
	const char *key;

	newv = tg_allocval();
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
			{
				vv = tg_copyval(vv);

				tg_darrpush(&(newv->arrval.arr), &vv);
			}
		);
	}

	newv->type = v->type;

	return newv;
}

// make a reference version of tg_castval?
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

		newv->type = TG_VAL_TABLE;
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

struct tg_val *tg_valgetattrr(struct tg_val *v, const char *key)
{
	struct tg_val *r;

	if ((r = tg_hashget(TG_HASH_ARRAY, &(v->attrs), key)) == NULL)
		return NULL;

	return r;
}

struct tg_val *tg_valgetattr(struct tg_val *v, const char *key)
{
	struct tg_val *r;

	if ((r = tg_valgetattrr(v, key)) == NULL)
		return tg_createval(TG_VAL_EMPTY);

	return tg_copyval(r);
}

void tg_valsetattr(struct tg_val *v, const char *key, struct tg_val *attr)
{
	tg_hashset(TG_HASH_ARRAY, &(v->attrs), key, tg_copyval(attr));
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

void tg_arrset(struct tg_val *arr, int p, struct tg_val *v)
{
	struct tg_val *newv;

	newv = tg_copyval(v);

	tg_darrset(&(arr->arrval.arr), p, &newv);
}

static void tg_printtable(FILE *f, struct tg_val *v)
{
	int i, j;
	int cols, rows;

	rows = tg_valgetattr(v, "rows")->intval;
	cols = tg_valgetattr(v, "cols")->intval;

	fprintf(f, "table{\n");
	
	fprintf(f, "\t");
	for (j = 0; j < cols * 16 + 1; ++j) fprintf(f, "-");
	fprintf(f, "\n");

	for (i = 0; i < rows; ++i) {
		struct tg_val *row;

		row = tg_arrgetr(v, i);
			
		fprintf(f, "\t|");
		for (j = 0; j < cols; ++j) {
			struct tg_val *cell;

			cell = tg_arrgetr(row, j);

			if (cell->type == TG_VAL_EMPTY)
				fprintf(f, "%-15s|", "empty");
			else if (cell->type == TG_VAL_INT)
				fprintf(f, "%-15d|", cell->intval);
			else if (cell->type == TG_VAL_FLOAT)
				fprintf(f, "%-15f|", cell->floatval);
			else if (cell->type == TG_VAL_STRING)
				fprintf(f, "%-15s|", cell->strval.str);
		}

		fprintf(f, "\n\t");
		for (j = 0; j < cols * 16 + 1; ++j) fprintf(f, "-");
		fprintf(f, "\n");
	}

	fprintf(f, "}");
}

void tg_printval(FILE *f, struct tg_val *v)
{
	int isfirst;
	int i;

	switch (v->type) {
	case TG_VAL_DELETED:
		fprintf(f, "deleted");
		break;
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
		tg_printtable(f, v);
		break;
	case TG_VAL_ARRAY:
		fprintf(f, "array{");
		
		isfirst = 1;

		for (i = 0; i < v->arrval.arr.cnt; ++i) {
			if (!isfirst) fprintf(f, ", ");

			isfirst = 0;

			tg_printval(f, tg_arrgetr(v, i));
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

struct tg_val *tg_arrgetr(struct tg_val *v, int i)
{
	struct tg_val **r;

	r = tg_darrget(&(v->arrval.arr), i);

	if (r == NULL)
		return NULL;

	return *((struct tg_val **) r);
}

struct tg_val *tg_arrget(struct tg_val *v, int i)
{
	struct tg_val *r;

	if ((r = tg_arrgetr(v, i)) == NULL)
		return tg_emptyval();

	return tg_copyval(r);
}

struct tg_val *tg_arrgete(struct tg_val *v, int i, struct tg_val *e)
{
	struct tg_val *r;

	if ((r = tg_arrgetr(v, i)) == NULL)
		return tg_copyval(e);

	return tg_copyval(r);
}

struct tg_val *tg_arrgetre(struct tg_val *v, int i, struct tg_val *e)
{
	struct tg_val **r;

	r = tg_darrget(&(v->arrval.arr), i);
	
	if (r == NULL) {
		tg_arrset(v, i, e);
		return tg_arrgetr(v, i);
	}


	return *((struct tg_val **) r);
}

struct tg_val *tg_arrgeth(struct tg_val *v, const char *k)
{
	struct tg_val *r;

	if ((r = tg_hashget(TG_HASH_ARRAY,
			&(v->arrval.hash), k)) == NULL)
		return tg_createval(TG_VAL_EMPTY);

	return tg_copyval(r);
}

static void tg_copytable(struct tg_val *dst, struct tg_val *src,
	int offr, int offc, int rows, int cols)
{
	int i, j;
	int srcrows, srccols;

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

// no assert
static struct tg_val *tg_tablegetcellr(struct tg_val *t,
	int row, int col)
{
	return tg_arrgetr(tg_arrgetr(t, row), col);
}

static struct tg_val *tg_tablespan(struct tg_val *t,
	int newside, int vert)
{
	struct tg_val *r;
	int rows, cols;
	int oldside;
	int otherside;
	int i, j;

	r = tg_createval(TG_VAL_TABLE);

	rows = tg_valgetattr(t, "rows")->intval;
	cols = tg_valgetattr(t, "cols")->intval;	

	oldside = vert ? rows : cols;

	tg_valsetattr(r, "rows", tg_intval(vert ? newside : rows));
	tg_valsetattr(r, "cols", tg_intval(vert ? newside : cols));

	if (newside <= oldside)
		return tg_copyval(t);

	otherside = vert ? cols : rows;

	for (i = 0; i <= otherside; ++i) {
		int p, pj;

		p = pj = -1;
		for (j = 0; j <= newside; ++j) {
			int c;

			c = (int) (oldside * (j - 1) / newside);

			// ...
		}
	}

	return r;
}

struct tg_val *tg_valnextto(struct tg_val *v1, struct tg_val *v2,
	int vert, int span)
{
	int t1rows, t1cols, t2rows, t2cols, maxrows, maxcols;
	struct tg_val *r;

	if ((v1 = tg_castval(v1, TG_VAL_TABLE)) == NULL)
		return NULL;

	if ((v2 = tg_castval(v2, TG_VAL_TABLE)) == NULL)
		return NULL;

	if (span) {

	}

	t1rows = tg_valgetattr(v1, "rows")->intval;
	t1cols = tg_valgetattr(v1, "cols")->intval;
	t2rows = tg_valgetattr(v2, "rows")->intval;
	t2cols = tg_valgetattr(v2, "cols")->intval;

	maxrows = (t1rows > t2rows) ? t1rows : t2rows;
	maxcols = (t1cols > t2cols) ? t1cols : t2cols;

	r = tg_createval(TG_VAL_TABLE);

	tg_copytable(r, v1, 0, 0,
		vert ? t1rows : maxrows, vert ? maxcols : t1cols);

	tg_copytable(r, v2,
		vert ? t1rows : 0, vert ? 0 : t1cols, 
		vert ? t2rows : maxrows, vert ? maxcols : t2cols);

	return r;
}
/*
struct tg_val *tg_valindex(struct tg_val *v1, struct tg_val *v2)
{
	struct tg_val r;

	return NULL;
}
*/
