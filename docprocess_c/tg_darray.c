#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tg_darray.h"

int tg_darrinit(struct tg_darray *darr, size_t sz)
{
	assert(darr != NULL);
	
	darr->sz = sz;

	if ((darr->data = malloc(sz)) == NULL)
		return (-1);

	darr->cnt = 0;
	darr->max = 1;

	return 0;
}

int tg_darrpush(struct tg_darray *darr, void *el)
{
	assert(darr != NULL);
	assert(el != NULL);

	if (darr->cnt >= darr->max) {
		darr->max *= 2;
		if ((darr->data = realloc(darr->data,
			darr->sz * darr->max)) == NULL)
			return (-1);
	}

	memcpy(darr->data + darr->cnt * darr->sz, el, darr->sz);
	
	++darr->cnt;

	return 0;
}

int tg_darrpop(struct tg_darray *darr, void *el)
{
	assert(darr != NULL);
	
	if (darr->cnt == 0)
		return (-1);

	--darr->cnt;
	
	if (el == NULL)
		return 0;
	
	memcpy(el, darr->data + darr->cnt * darr->sz, darr->sz);

	return 0;
}
