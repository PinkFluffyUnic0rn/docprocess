#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "tg_value.h"
#include "tg_inout.h"

void tg_createinput(struct tg_input *in,
		const char *type, const char *path)
{
	in->type = tg_strdup(type);
	in->path = tg_strdup(path);
}

void tg_destroyinput(struct tg_input *in)
{
	free(in->type);
	free(in->path);
}

void tg_createoutput(struct tg_output *out,
		const char *type, FILE *file)
{
	out->type = tg_strdup(type);
	out->file = file;
}

void tg_destroyoutput(struct tg_output *out)
{
	free(out->type);
}

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

struct tg_val *tg_csvrecord(FILE *f, const struct tg_val *h)
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

struct tg_val *tg_readsource(const struct tg_input *in,
	const struct tg_val *args)
{
	char conv[PATH_MAX];
	FILE *f;
	int pid;
	int p[2];

	snprintf(conv, PATH_MAX, "%s/in_%s.sh", CONVERTERS_PATH, in->type);

	if ((f = fopen(conv, "r")) == NULL) {
		TG_WARNING("Cannot open converter for input type %s.", in->type);
		return NULL;
	}

	TG_ASSERT(pipe(p) >= 0, "Error while opening pipe: %s.",
		strerror(errno));
	
	TG_ASSERT((pid = fork()) >= 0,
		"Error while forking a process: %s.", strerror(errno));

	if (pid == 0) {
		char **argv;
		int i;
		
		close(p[0]);

		TG_ASSERT(close(1) >= 0, "Cannot close file descriptor");
		TG_ASSERT(dup(p[1]) >= 0, "Cannot duplicate file descriptor");

		TG_ASSERT(argv = malloc(sizeof(char **) * (args->arrval.arr.cnt + 3)),
			"Allocation error.");

		args = tg_castval(args, TG_VAL_ARRAY);
		
		for (i = 0; i < args->arrval.arr.cnt; ++i) {
			struct tg_val *sv;

			sv = tg_castval(tg_arrget(args, i), TG_VAL_STRING);

			TG_ASSERT(sv != NULL, tg_error);

			argv[i + 2] = sv->strval.str;
		}
		
		argv[0] = conv;

		TG_ASSERT(argv[1] = malloc(PATH_MAX), "Allocation error.");
		strcpy(argv[1], in->path);

		argv[args->arrval.arr.cnt + 2] = NULL;
		
		TG_ASSERT(execv(conv, argv) >= 0, strerror(errno));
		
		exit(1);
	}

	close(p[1]);
	fclose(f);

	TG_ASSERT((f = fdopen(p[0], "r")) != NULL,
		"Error while openging file descriptor for buffered IO");

	return tg_csv2table(f);
}

struct tg_val *tg_writeval(struct tg_output *out,
	const struct tg_val *v)
{
	printf("return value: ");
	tg_printval(stdout, v);
	printf("\n");

	return tg_emptyval();
}

