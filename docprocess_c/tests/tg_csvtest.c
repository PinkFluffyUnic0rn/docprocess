#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "../tg_value.h"
#include "../tg_common.h"

struct tg_val *tg_csvtoken(FILE *f)
{
	struct tg_val *r;
	static FILE *file;
	int c, cc;

	if (f != NULL)
		file = f;

	c = fgetc(file);
	if (c == EOF) {
		if (!feof(file))
			goto ioerror;

		return tg_emptyval();
	}
	else if (c == '\"') {
		struct tg_val *r;
		int cc;

		r = tg_stringval("");
		while ((cc = fgetc(file)) != EOF) {
			if (cc == '"') {
				cc = fgetc(file);
				if (cc == '"')
					tg_dstraddstrn(&(r->strval), (char *) &cc, 1);
				else if (cc == EOF) {
					TG_SETERROR("Unexpected EOF.");
					return NULL;
				}
				else
					return r;

			}
			else
				tg_dstraddstrn(&(r->strval), (char *) &cc, 1);
		}

		goto ioerror;
	}
	else if (c == ';' || c == '\n') {
		return tg_intval(c == ';' ? 1 : 2);
	}
	
	r = tg_stringval("");
	
	tg_dstraddstrn(&(r->strval), (char *) &c, 1);
	while ((cc = fgetc(file)) != EOF) {
		if (cc == '\"' || cc == ';' || cc == '\n') {
			if (ungetc(cc, file) == EOF)
				goto ioerror;

			break;
		}

		tg_dstraddstrn(&(r->strval), (char *) &cc, 1);
	}

	if (cc == EOF && !feof(file))
		goto ioerror;

	return r;

ioerror:
	TG_SETERROR("IO error while reading a FILE.");
	return NULL;
}


// f == NULL, continue
// f == file, start parsing
// h == array, hash by its elements
// return value -> array
struct tg_val *tg_csvrecord(FILE *f, struct tg_val *h)
{
	struct tg_val *s, *r;
	int p;

	r = tg_createval(TG_VAL_ARRAY);

	p = 0;
	while ((s = tg_csvtoken(f)) != NULL) {
		if (s->type == TG_VAL_EMPTY)
			return s;
		else if (s->type == TG_VAL_INT && s->intval == 1)
			++p;
		else if (s->type == TG_VAL_INT && s->intval == 2)
			return r;
		else {
			struct tg_val *v;	

			if (h != NULL) {
				if ((v = tg_arrget(h, p)) == NULL) {
					TG_SETERROR("Header string doesn't have enough values.");
					return NULL;
				}

				v = tg_castval(v, TG_VAL_STRING);
				v = tg_arrgethre(r, v->strval.str, tg_stringval(""));
			}
			else
				v = tg_arrgetre(r, p, tg_stringval(""));
			
			tg_dstraddstr(&(v->strval), s->strval.str);
		}
	}

	return NULL;
}

struct tg_val *tg_csv2table(FILE *f)
{
	struct tg_val *h, *r, *rr;

	r = tg_createval(TG_VAL_ARRAY);
	
	TG_NULLQUIT(h = tg_csvrecord(f, NULL));

	while ((rr = tg_csvrecord(f, h)) != NULL) {
		if (rr->type == TG_VAL_EMPTY)
			return tg_castval(r, TG_VAL_TABLE);

		tg_arrpush(r, rr);
	}

	return NULL;
}

int main(int argc, const char *argv[])
{
	FILE *f;

	if (argc < 2)
		TG_ERROR("Should have at least 1 argument.");

	if ((f = fopen(argv[1], "r")) == NULL)
		TG_ERROR("Cannot open file %s.", argv[1]);

	tg_startframe();

	tg_printval(stdout, tg_csv2table(f));
	printf("\n");

	tg_endframe();

	fclose(f);

	return 0;
}
