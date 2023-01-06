#ifndef TG_INOUT_H
#define TG_INOUT_H

#define CONVERTERS_PATH "/home/qwerty/docprocess/docprocess_c/converters"

struct tg_input {
	char *type;
	char *path;
};

struct tg_output {
	char *type;
	FILE *file;
};

void tg_createinput(struct tg_input *in,
		const char *type, const char *path);

void tg_destroyinput(struct tg_input *in);

void tg_createoutput(struct tg_output *out,
		const char *type, FILE *file);

void tg_destroyoutput(struct tg_output *in);

struct tg_val *tg_readsource(const struct tg_input *in,
	const struct tg_val *args);

struct tg_val *tg_writeval(struct tg_output *out,
	const struct tg_val *v);

#endif
