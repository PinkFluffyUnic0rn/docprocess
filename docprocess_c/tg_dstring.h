#ifndef DSTRING_H
#define DSTRING_H

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

int tg_dstrcreaten(struct tg_dstring *dstr, const char *src,
	size_t len);

int tg_dstrcreatestatic(struct tg_dstring *dstr, const char *src);

int tg_dstraddstrn(struct tg_dstring *dstr, const char *src,
	size_t len);

int tg_dstrcat(struct tg_dstring *dstr1, struct tg_dstring *dstr2);

int tg_dstrdestroy(struct tg_dstring *dstr);

// int tg_dstrwipe();

#endif
