#ifndef TG_VALUE_H
#define TG_VALUE_H

#include <stddef.h>

#include "tg_common.h"
#include "tg_dstring.h"

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
	TG_VAL_EMPTY = 0,
	TG_VAL_FUNCTION = 1,
	TG_VAL_SCRIPT = 2,
	TG_VAL_INT = 3,
	TG_VAL_FLOAT = 4,
	TG_VAL_STRING = 5,
	TG_VAL_TABLE = 6,
	TG_VAL_ARRAY = 7,
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

	TG_HASHFIELDS(struct tg_val, TG_HASH_ARRAY)
};

// stack operations
int tg_startframe();

void tg_endframe();

// -------------------------------------------------------------------

// value operations
struct tg_val *tg_createval(enum TG_VALTYPE t);

void tg_freeval(struct tg_val *v);

#define tg_emptyval() tg_createval(TG_VAL_EMPTY)
#define tg_arrval() tg_createval(TG_VAL_ARRAY)
struct tg_val *tg_intval(int v);
struct tg_val *tg_floatval(float v);
struct tg_val *tg_stringval(const char *v);

struct tg_val *tg_copyval(struct tg_val *v);

struct tg_val *tg_castval(struct tg_val *v, enum TG_VALTYPE t);

struct tg_val *tg_typeprom1val(struct tg_val *v, enum TG_VALTYPE mtype);

struct tg_val *tg_typeprom2val(struct tg_val *v1,
	enum TG_VALTYPE v2type, enum TG_VALTYPE mtype);

struct tg_val *tg_valgetattr(struct tg_val *v1, const char *key);
struct tg_val *tg_valgetattrr(struct tg_val *v1, const char *key);
void tg_valsetattr(struct tg_val *v, const char *key,
	struct tg_val *attr);

void tg_arrpush(struct tg_val *arr, struct tg_val *v);
void tg_arrset(struct tg_val *arr, int i, struct tg_val *v);
struct tg_val *tg_arrget(struct tg_val *v, int i);
struct tg_val *tg_arrgete(struct tg_val *v, int i, struct tg_val *e);
struct tg_val *tg_arrgetr(struct tg_val *v, int i);
struct tg_val *tg_arrgetre(struct tg_val *v, int i, struct tg_val *e);
struct tg_val *tg_arrgeth(struct tg_val *v, const char *k);
#define tg_arrgethr(v, k) \
	tg_hashget(TG_HASH_ARRAY, &((v)->arrval.hash), (k));

#define TG_ARRFOREACH(v, pos, el, action) 			\
do {								\
	for ((pos) = 0; (pos) < (v)->arrval.arr.cnt; ++(pos)) {	\
		(el) = tg_arrgetr((v), (pos));			\
		action						\
	}							\
} while (0);

void tg_printval(FILE *f, struct tg_val *v);

int tg_istrueval(struct tg_val *v);

struct tg_val *tg_valcat(struct tg_val *v1, struct tg_val *v2);

struct tg_val *_tg_numop(struct tg_val *v1, struct tg_val *v2,
	enum TG_NUMOP op);

#define tg_valadd(v1, v2) _tg_numop(v1, v2, TG_NUMOP_ADD)
#define tg_valsub(v1, v2) _tg_numop(v1, v2, TG_NUMOP_SUB)
#define tg_valmult(v1, v2) _tg_numop(v1, v2, TG_NUMOP_MULT)
#define tg_valdiv(v1, v2) _tg_numop(v1, v2, TG_NUMOP_DIV)

struct tg_val *_tg_valcmp(struct tg_val *v1, struct tg_val *v2,
	enum TG_RELOP op);

#define tg_valeq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_EQUAL)
#define tg_valneq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_NEQUAL)
#define tg_valls(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_LESS)
#define tg_valgr(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_GREATER)
#define tg_vallseq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_LESSEQ)
#define tg_valgreq(v1, v2) _tg_valcmp(v1, v2, TG_RELOP_GREATEREQ)

struct tg_val *tg_valand(struct tg_val *v1, struct tg_val *v2);
struct tg_val *tg_valor(struct tg_val *v1, struct tg_val *v2);

struct tg_val *tg_valnextto(struct tg_val *v1, struct tg_val *v2,
	int vert, int span);

//struct tg_val *tg_valindex(struct tg_val *v1, struct tg_val *v2);

// -------------------------------------------------------------------

#endif
