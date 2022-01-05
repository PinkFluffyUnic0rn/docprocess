#ifndef DSTRING_H
#define DSTRING_H

struct tg_dstring {
	int bufs;
	char *str;
	union {
		char c[2];
		int eln;
	};
	size_t len;
};

#define tg_dstraddstr(dstr, src) \
	tg_dstraddstrn((dstr), (src), strlen((src)))

int tg_dstrcreate(struct tg_dstring *dstr, const char *src);

int tg_dstraddstrn(struct tg_dstring *dstr, const char *src,
	size_t len);

int tg_dstrcat(struct tg_dstring *dstr1, struct tg_dstring *dstr2);

int tg_dstrdestroy(struct tg_dstring *dstr);

int tg_dstrwipe();

int buftest();

#endif
