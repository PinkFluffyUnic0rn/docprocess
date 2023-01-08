#ifndef TG_VALUE_H
#define TG_VALUE_H

#include <stddef.h>

#include "tg_common.h"
#include "tg_dstring.h"

#define tg_isscalar(t) ((t) != TG_VAL_ARRAY && (t) != TG_VAL_TABLE)

enum TG_NUMOP {
	TG_NUMOP_ADD,
	TG_NUMOP_SUB,
	TG_NUMOP_MULT,
	TG_NUMOP_DIV
};

enum TG_RELOP {
	TG_RELOP_EQUAL,
	TG_RELOP_NEQUAL,
	TG_RELOP_LESS,
	TG_RELOP_GREATER,
	TG_RELOP_LESSEQ,
	TG_RELOP_GREATEREQ,
};

enum TG_VALTYPE {
	TG_VAL_DELETED = 0,
	TG_VAL_EMPTY = 1,
	TG_VAL_INT = 2,
	TG_VAL_FLOAT = 3,
	TG_VAL_STRING = 4,
	TG_VAL_TABLE = 5,
	TG_VAL_ARRAY = 6,
};

struct tg_val {
	union {
		struct tg_dstring strval;
		int intval;
		float floatval;
		struct {
			struct tg_hash hash;
			struct tg_darray arr;
		} arrval;
	};
	enum TG_VALTYPE type;

	struct tg_hash attrs;

	int arrpos;
	int ishashed;

	TG_HASHFIELDS(struct tg_val, TG_HASH_ARRAY)
};

// stack operations
int tg_startframe();

void tg_endframe();

void tg_setretval(const struct tg_val *v);
struct tg_val *tg_getretval();

// -------------------------------------------------------------------

// value operations
struct tg_val *tg_createval(enum TG_VALTYPE t);

void tg_freeval(struct tg_val *v);

#define tg_emptyval() tg_createval(TG_VAL_EMPTY)
#define tg_arrval() tg_createval(TG_VAL_ARRAY)
struct tg_val *tg_intval(int v);
struct tg_val *tg_floatval(float v);
struct tg_val *tg_stringval(const char *v);

void tg_moveval(struct tg_val *d, const struct tg_val *s);
struct tg_val *tg_copyval(const struct tg_val *v);

struct tg_val *tg_castval(const struct tg_val *v, enum TG_VALTYPE t);
struct tg_val *tg_castvalr(struct tg_val *v, enum TG_VALTYPE t);

struct tg_val *tg_typeprom1val(const struct tg_val *v,
	enum TG_VALTYPE mtype);

struct tg_val *tg_typeprom2val(const struct tg_val *v1,
	enum TG_VALTYPE v2type, enum TG_VALTYPE mtype);

void tg_valsetattr(struct tg_val *v, const char *key,
	const struct tg_val *attr);
struct tg_val *tg_valgetattr(const struct tg_val *v1, const char *key);
struct tg_val *tg_valgetattre(const struct tg_val *v1, const char *key,
	const struct tg_val *e);
struct tg_val *tg_valgetattrr(const struct tg_val *v1, const char *key);
struct tg_val *tg_valgetattrre(struct tg_val *v, const char *key,
	const struct tg_val *e);

struct tg_val *tg_printval(FILE *f, const struct tg_val *v);

int tg_istrueval(const struct tg_val *v);

struct tg_val *tg_valand(const struct tg_val *v1,
	const struct tg_val *v2);
struct tg_val *tg_valor(const struct tg_val *v1,
	const struct tg_val *v2);

struct tg_val *_tg_numop(const struct tg_val *v1,
	const struct tg_val *v2, enum TG_NUMOP op);

#define tg_valadd(v1, v2) _tg_numop(v1, v2, TG_NUMOP_ADD)
#define tg_valsub(v1, v2) _tg_numop(v1, v2, TG_NUMOP_SUB)
#define tg_valmult(v1, v2) _tg_numop(v1, v2, TG_NUMOP_MULT)
#define tg_valdiv(v1, v2) _tg_numop(v1, v2, TG_NUMOP_DIV)

struct tg_val *_tg_valcmp(const struct tg_val *v1,
	const struct tg_val *v2, enum TG_RELOP op);

#define tg_valeq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_EQUAL)
#define tg_valneq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_NEQUAL)
#define tg_valls(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_LESS)
#define tg_valgr(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_GREATER)
#define tg_vallseq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_LESSEQ)
#define tg_valgreq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_GREATEREQ)

struct tg_val *tg_valcat(const struct tg_val *v1,
	const struct tg_val *v2);

struct tg_val *tg_substr(const struct tg_val *s,
	const struct tg_val *b, const struct tg_val *l);

struct tg_val *tg_match(const struct tg_val *s,
	const struct tg_val *reg);

struct tg_val *tg_rudate(const struct tg_val *day,
	const struct tg_val *month, const struct tg_val *year);

struct tg_val *tg_day(const struct tg_val *d);
struct tg_val *tg_month(const struct tg_val *d);
struct tg_val *tg_year(const struct tg_val *d);
struct tg_val *tg_weekday(const struct tg_val *d);
struct tg_val *tg_monthend(const struct tg_val *month,
	const struct tg_val *year);
struct tg_val *tg_datecmp(const struct tg_val *d1,
	const struct tg_val *d2);
struct tg_val *tg_datediff(const struct tg_val *d1,
	const struct tg_val *d2, const struct tg_val *p);
struct tg_val *tg_dateadd(const struct tg_val *d,
	const struct tg_val *v, const struct tg_val *p);

void tg_tablesetcellr(struct tg_val *t, int r, int c, struct tg_val *v);
struct tg_val *tg_tablegetcellr(struct tg_val *t, int row, int col);
struct tg_val *tg_tablegetcellre(struct tg_val *t,
	int row, int col, struct tg_val *e);

void tg_arrset(struct tg_val *arr, int i, const struct tg_val *v);
void tg_arrpush(struct tg_val *arr, const struct tg_val *v);

struct tg_val *tg_arrgetr(const struct tg_val *v, int i);
struct tg_val *tg_arrgetre(struct tg_val *v, int i,
	const struct tg_val *e);
struct tg_val *tg_arrget(const struct tg_val *v, int i);
struct tg_val *tg_arrgete(struct tg_val *v, int i,
	const struct tg_val *e);

void tg_arrseth(struct tg_val *arr, const char *k,
	const struct tg_val *v);
struct tg_val *tg_arrgethr(const struct tg_val *v, const char *k);
struct tg_val *tg_arrgethre(struct tg_val *v, const char *k,
	const struct tg_val *e);
struct tg_val *tg_arrgeth(const struct tg_val *v, const char *k);
struct tg_val *tg_arrgethe(const struct tg_val *v, const char *k,
	const struct tg_val *e);

#define TG_ARRFOREACH(v, pos, el, action) 			\
do {								\
	for ((pos) = 0; (pos) < (v)->arrval.arr.cnt; ++(pos)) {	\
		(el) = tg_arrgetr((v), (pos));			\
		action;						\
	}							\
} while (0);

struct tg_val *tg_tablespan(struct tg_val *t,
	int newside, int vert);

struct tg_val *tg_valnextto(const struct tg_val *v1,
	const struct tg_val *v2, int vert, int span);

// -------------------------------------------------------------------

#endif
