#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tg_parser.h"
#include "tg_value.h"

struct tg_symbol {
	struct tg_val *p;
	TG_HASHFIELDS(struct tg_symbol, TG_HASH_SYMBOL)
};

TG_HASHED(struct tg_symbol, TG_HASH_SYMBOL)

struct tg_hash symtable;
/*
void tg_symboladd(const char *name, struct tg_val *v)
{
	

	tg_hashset(TG_HASH_SYMBOL, symtable, name, (alloc symbol));
}
*/
void printval(FILE *f, struct tg_val *v)
{
	int isfirst;
	const char *key;

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
	case TG_VAL_ARRAY:
		fprintf(f, "array{");

		isfirst = 1;

		TG_HASHFOREACH(struct tg_val, TG_HASH_ARRAY,
			v->arrval, key,
			if (!isfirst) fprintf(f, ", ");
		
			isfirst = 0;

			fprintf(f, "%s = ", key);
			printval(f, p);	
		);

		fprintf(f, "}");

		break;
	}
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
	printval(stdout, v1);
	printf("\n");
	
	printval(stdout, v2);
	printf("\n");
	printval(stdout, v3);
	printf("\n");
	printval(stdout, v4);
	printf("\n");


	v5 = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(v5, tg_stringval("321.35"));
	printval(stdout, v5);
	printf("\n");

	v6 = tg_createval(TG_VAL_ARRAY);

	tg_arrpush(v6, v1);
	tg_arrpush(v6, v2);
	tg_arrpush(v6, v3);
	tg_arrpush(v6, v4);
	tg_arrpush(v6, v5);

	printval(stdout, v6);
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
	printval(stdout, v7);
	printf("\n");
	printf("int -> string: ");
	printval(stdout, v8);
	printf("\n");

	printf("float -> int: ");
	printval(stdout, v9);
	printf("\n");
	printf("float -> string: ");
	printval(stdout, v10);
	printf("\n");

	printf("string -> int: ");
	printval(stdout, v11);
	printf("\n");
	printf("string -> float: ");
	printval(stdout, v12);
	printf("\n");

	printf("numeric string -> int: ");
	printval(stdout, v13);
	printf("\n");
	printf("numeric string -> float: ");
	printval(stdout, v14);
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
	printval(stdout, v15);
	printf("\n");
	
	printf("float -> array: ");
	printval(stdout, v16);
	printf("\n");

	printf("string -> array: ");
	printval(stdout, v17);
	printf("\n");

	printf("numeric string -> array: ");
	printval(stdout, v18);
	printf("\n");

	printf("one-element array -> int: ");
	printval(stdout, v19);
	printf("\n");

	printf("one-element array -> float: ");
	printval(stdout, v20);
	printf("\n");
		
	printf("one-element array -> string: ");
	printval(stdout, v21);
	printf("\n");


	struct tg_val *varr;
	int i;

	varr = tg_createval(TG_VAL_ARRAY);

	for (i = 0; i < 10000000; ++i)
		tg_arrpush(varr, tg_intval(i));
	
	printf("a very big array: ");
	printval(stdout, varr);
	printf("\n");


	tg_endframe();

	return 0;
}
