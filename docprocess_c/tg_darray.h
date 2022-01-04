#ifndef TG_DARRAY_H
#define TG_DARRAY_H

struct tg_darray {
	void *data;
	size_t sz;
	size_t cnt;
	size_t max;
};

int tg_darrinit(struct tg_darray *darr, size_t sz);

int tg_darrpush(struct tg_darray *darr, void *el);

int tg_darrpop(struct tg_darray *darr, void *el);

#endif
