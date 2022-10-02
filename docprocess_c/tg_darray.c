#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tg_common.h"
#include "tg_darray.h"

void tg_darrinit(struct tg_darray *darr, size_t sz)
{
	assert(darr != NULL);
	
	darr->sz = sz;

	darr->data = malloc(sz);
	TG_ASSERT(darr->data != NULL,
		"%s%s", "Cannot allocate memory while",
		" initilizing a dynamic array");
	
	darr->cnt = 0;
	darr->max = 1;
}

int tg_darrpush(struct tg_darray *darr, void *el)
{
	assert(darr != NULL);
	assert(el != NULL);

	if (darr->cnt >= darr->max) {
		char *p;
		
		p = realloc(darr->data, darr->sz * darr->max * 2);
		TG_ASSERT(p != NULL, "%s%s", "Cannot allocate memory",
			" for a dynamic array");

		darr->max *= 2;
		darr->data = p;
	}

	memcpy(darr->data + darr->cnt * darr->sz, el, darr->sz);

	return darr->cnt++;
}

int tg_darrpop(struct tg_darray *darr, void *el)
{
	assert(darr != NULL);
	
	if (darr->cnt == 0) {
		TG_SETERROR("%s%s", "Trying to pop out of an empty",
			" dynamic array");
		return (-1);
	}

	--darr->cnt;
	
	if (el == NULL)
		return 0;
	
	memcpy(el, darr->data + darr->cnt * darr->sz, darr->sz);

	return 0;
}

void tg_darrclear(struct tg_darray *darr)
{
	free(darr->data);
}

void *tg_darrget(struct tg_darray *darr, int n)
{
	assert(darr != NULL);
	
	if (n < 0 || n >= darr->cnt) {
		TG_SETERROR("%s%s", "Trying to get non-existing",
			" element out of a dynamyc array");
		return NULL;
	}
	
	return darr->data + n * darr->sz;
}
