#ifndef TG_DARRAY_H
#define TG_DARRAY_H

struct tg_darray {
	void *data;
	size_t sz;
	size_t cnt;
	size_t max;
};

void tg_darrinit(struct tg_darray *darr, size_t sz);

#define tg_darrpush(darr, el) tg_darrset((darr), (darr)->cnt, (el))

int tg_darrset(struct tg_darray *darr, size_t pos, void *el);

int tg_darrpop(struct tg_darray *darr, void *el);

void tg_darrclear(struct tg_darray *darr);

void *tg_darrget(const struct tg_darray *darr, int n);

#define tg_darrsort(darr, sortfunc) 				\
	qsort((darr)->data, (darr)->cnt, (darr)->sz, (sortfunc));

#endif
