#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tg_parser.h"
#include "tg_value.h"

// symbol table API
struct tg_symbol {
	struct tg_val *p;
	TG_HASHFIELDS(struct tg_symbol, TG_HASH_SYMBOL)
};

TG_HASHED(struct tg_symbol, TG_HASH_SYMBOL)

struct tg_allocator symalloc;
struct tg_hash symtable;

void tg_symboladd(const char *name, struct tg_val *v)
{
	struct tg_symbol *sym;

	sym = tg_alloc(&symalloc);
	sym->p = v;

	tg_hashset(TG_HASH_SYMBOL, &symtable, name, sym);
}

struct tg_val *tg_symbolget(const char *name)
{
	struct tg_symbol *sym;

	sym = tg_hashget(TG_HASH_SYMBOL, &symtable, name);

	return sym->p;
}

int main()
{
	struct tg_val *v1, *v2, *v3, *v4, *v5, *v6;

	tg_initstack();

	tg_startframe();

	v1 = tg_intval(415);
	v2 = tg_floatval(522.34);
	v3 = tg_stringval("teststring");
	v4 = tg_stringval("832.23");

	printf("defined values:\n");
	tg_printval(stdout, v1);
	printf("\n");
	
	tg_printval(stdout, v2);
	printf("\n");
	tg_printval(stdout, v3);
	printf("\n");
	tg_printval(stdout, v4);
	printf("\n");


	v5 = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(v5, tg_stringval("321.35"));
	tg_printval(stdout, v5);
	printf("\n");

	v6 = tg_createval(TG_VAL_ARRAY);

	tg_arrpush(v6, v1);
	tg_arrpush(v6, v2);
	tg_arrpush(v6, v3);
	tg_arrpush(v6, v4);
	tg_arrpush(v6, v5);

	tg_printval(stdout, v6);
	printf("\n");
	printf("\n");

	struct tg_val *v7, *v8, *v9, *v10, *v11, *v12, *v13, *v14;

	v7 = tg_castval(v1, TG_VAL_FLOAT);
	v8 = tg_castval(v1, TG_VAL_STRING);

	v9 = tg_castval(v2, TG_VAL_INT);
	v10 = tg_castval(v2, TG_VAL_STRING);

	v11 = tg_castval(v3, TG_VAL_INT);
	v12 = tg_castval(v3, TG_VAL_FLOAT);

	v13 = tg_castval(v4, TG_VAL_INT);
	v14 = tg_castval(v4, TG_VAL_FLOAT);

	printf("casting scalar types:\n");

	printf("int -> float: ");
	tg_printval(stdout, v7);
	printf("\n");
	printf("int -> string: ");
	tg_printval(stdout, v8);
	printf("\n");

	printf("float -> int: ");
	tg_printval(stdout, v9);
	printf("\n");
	printf("float -> string: ");
	tg_printval(stdout, v10);
	printf("\n");

	printf("string -> int: ");
	tg_printval(stdout, v11);
	printf("\n");
	printf("string -> float: ");
	tg_printval(stdout, v12);
	printf("\n");

	printf("numeric string -> int: ");
	tg_printval(stdout, v13);
	printf("\n");
	printf("numeric string -> float: ");
	tg_printval(stdout, v14);
	printf("\n");
	printf("\n");


	struct tg_val *v15, *v16, *v17, *v18, *v19, *v20, *v21;

	printf("casting with arrays:\n");

	v15 = tg_castval(v1, TG_VAL_ARRAY);
	v16 = tg_castval(v2, TG_VAL_ARRAY);
	v17 = tg_castval(v3, TG_VAL_ARRAY);
	v18 = tg_castval(v4, TG_VAL_ARRAY);
	v19 = tg_castval(v5, TG_VAL_INT);
	v20 = tg_castval(v5, TG_VAL_FLOAT);
	v21 = tg_castval(v5, TG_VAL_STRING);

	printf("int -> array: ");
	tg_printval(stdout, v15);
	printf("\n");
	
	printf("float -> array: ");
	tg_printval(stdout, v16);
	printf("\n");

	printf("string -> array: ");
	tg_printval(stdout, v17);
	printf("\n");

	printf("numeric string -> array: ");
	tg_printval(stdout, v18);
	printf("\n");

	printf("one-element array -> int: ");
	tg_printval(stdout, v19);
	printf("\n");

	printf("one-element array -> float: ");
	tg_printval(stdout, v20);
	printf("\n");
		
	printf("one-element array -> string: ");
	tg_printval(stdout, v21);
	printf("\n");


	struct tg_val *varr;
	int i;

	varr = tg_createval(TG_VAL_ARRAY);

	for (i = 0; i < 10000000; ++i)
		tg_arrpush(varr, tg_intval(i));
	
	printf("a very big array: ");
	tg_printval(stdout, varr);
	printf("\n");


	tg_endframe();

	return 0;
}
