#include "tg_common.h"

#define TG_FRAMEBLOCKSIZE 32

char tg_error[TG_MSGMAXSIZE];

void tg_allocinit(struct tg_allocator *allocer, size_t sz)
{
	tg_darrinit(&(allocer->blocks), sizeof(void *));
	tg_darrinit(&(allocer->freed), sizeof(void *));
	allocer->valcount = 0;
	allocer->sz = sz;
}

void *tg_alloc(struct tg_allocator *allocer)
{
	void *p;

	if (tg_darrpop(&(allocer->freed), &p) >= 0)
		return p;

	if (allocer->valcount / TG_FRAMEBLOCKSIZE >= allocer->blocks.cnt) {	
		p = malloc(allocer->sz * TG_FRAMEBLOCKSIZE);
		TG_ASSERT(p != NULL,
			"Cannot allocate new value in stack.");

		tg_darrpush(&(allocer->blocks), &p);
	}

	p = *((void **) tg_darrget(&(allocer->blocks),
		allocer->blocks.cnt - 1));

	return p + ((allocer->valcount++) % TG_FRAMEBLOCKSIZE)
		* allocer->sz;
}

void tg_free(struct tg_allocator *allocer, void *p)
{
	tg_darrpush(&(allocer->freed), &p);
}

void tg_allocdestroy(struct tg_allocator *allocer)
{
	void *p;

	while (tg_darrpop(&(allocer->blocks), &p) >= 0)
		free(p);

	tg_darrclear(&(allocer->blocks));
	tg_darrclear(&(allocer->freed));

	allocer->valcount = 0;

}
