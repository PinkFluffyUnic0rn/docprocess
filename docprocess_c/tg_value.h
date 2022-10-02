#ifndef TG_VALUE_H
#define TG_VALUE_H

#include <stddef.h>

#include "tg_dstring.h"

enum TG_VALTYPE {
	TG_VAL_EMPTY = 0,
	TG_VAL_FUNCTION = 1,
	TG_VAL_SCRIPT = 2,
	TG_VAL_INT = 3,
	TG_VAL_FLOAT = 4,
	TG_VAL_STRING = 5,
	TG_VAL_ARRAY = 6,
};

struct tg_hash {
	struct tg_val **buckets;
	struct tg_val *last;
	size_t bucketscount;
	size_t count;
};

struct tg_val {
	union {
		struct tg_dstring strval;
		int intval;
		float floatval;
		struct tg_hash arrval;
	};
	enum TG_VALTYPE type;

	struct tg_hash *attrs;

	struct tg_dstring key;				// key in hash
	struct tg_val *prev;				// prev node in bucket
	struct tg_val *next;				// next node in bucket
};

// stack operations
void tg_initstack();

int tg_startframe();

void tg_endframe();

// -------------------------------------------------------------------

// value operations
struct tg_val *tg_createval(enum TG_VALTYPE t);

struct tg_val *tg_intval(int v);

struct tg_val *tg_floatval(float v);

struct tg_val *tg_stringval(const char *v);

// attributes are not copied
struct tg_val *tg_copyval(struct tg_val *v);

// attributes are not copied
struct tg_val *tg_castval(struct tg_val *v, enum TG_VALTYPE t);

struct tg_val *tg_typeprom1val(struct tg_val *v, enum TG_VALTYPE mtype);

struct tg_val *tg_typeprom2val(struct tg_val *v1,
	enum TG_VALTYPE v2type, enum TG_VALTYPE mtype);

int tg_istrueval(struct tg_val *v);

void tg_arrpush(struct tg_val *arr, struct tg_val *v);

// -------------------------------------------------------------------

#endif
