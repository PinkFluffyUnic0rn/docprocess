#ifndef TG_DARRAY_H
#define TG_DARRAY_H

struct tg_darray {
	void *data;
	size_t sz;
	size_t cnt;
	size_t max;
};

void tg_darrinit(struct tg_darray *darr, size_t sz);

int tg_darrpush(struct tg_darray *darr, void *el);

int tg_darrpop(struct tg_darray *darr, void *el);

void tg_darrclear(struct tg_darray *darr);

void *tg_darrget(struct tg_darray *darr, int n);

#endif
