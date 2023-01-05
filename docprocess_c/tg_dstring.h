#ifndef TG_DSTRING_H
#define TG_DSTRING_H

#include <string.h>

struct tg_dstring {
	int bufs;
	char *str;
	union {
		int eln;
		size_t sz;
	};
	size_t len;
};

#define tg_dstrcreate(dstr, src) \
	tg_dstrcreaten((dstr), (src), strlen((src)))

#define tg_dstraddstr(dstr, src) \
	tg_dstraddstrn((dstr), (src), strlen((src)))

void tg_dstrcreaten(struct tg_dstring *dstr, const char *src,
	size_t len);

void tg_dstrcreatestatic(struct tg_dstring *dstr, const char *src);

void tg_dstraddstrn(struct tg_dstring *dstr, const char *src,
	size_t len);

void tg_dstrcat(struct tg_dstring *dstr1, struct tg_dstring *dstr2);

void tg_dstrdestroy(struct tg_dstring *dstr);

// int tg_dstrwipe();

#endif
