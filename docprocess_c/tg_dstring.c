#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "tg_darray.h"
#include "tg_dstring.h"

#define BUFSZ 4096

struct tg_dstrbufs {
	struct tg_darray bufs;
	struct tg_darray free;
	void *end;
	size_t lastid;
	size_t sz;
};

// struct tg_darray mbufs;
static struct tg_dstrbufs bufs8;
static struct tg_dstrbufs bufs16;
static int bufs8init = 0;
static int bufs16init = 0;
// static int mbufsinit = 0;

static int tg_dstrbufsclear(struct tg_dstrbufs *bufs)
{
	int i;
	char *p;

	while (tg_darrpop(&(bufs->bufs), &p) == 0)
		free(p);

	while (tg_darrpop(&(bufs->free), NULL) == 0);
	
	bufs->end = NULL;
	bufs->lastid = 0;
	bufs->sz = 0;
}

static int tg_dstrbufsinit(struct tg_dstrbufs *bufs, size_t sz)
{
	char *buf;

	assert(bufs != NULL);

	bufs->sz = sz;
	bufs->lastid = 0;

	if (tg_darrinit(&(bufs->bufs), sizeof(void *)) < 0)
		return (-1);

	if ((buf = malloc(BUFSZ)) == NULL)
		return (-1);
	
	if (tg_darrpush(&(bufs->bufs), &buf) < 0)
		return (-1);

	bufs->end = ((void **)bufs->bufs.data)[bufs->bufs.cnt - 1];
	
	if (tg_darrinit(&(bufs->free), sizeof(int)) < 0)
		return (-1);

	return 0;
}

static int tg_dstrbufsalloc(struct tg_dstrbufs *bufs, size_t len)
{
	int eln;

	assert(bufs != NULL);

	if (tg_darrpop(&(bufs->free), &eln) == 0)
		return eln;

	if (((void **) bufs->bufs.data)[bufs->bufs.cnt - 1]
			+ BUFSZ - bufs->end < bufs->sz) {
		char *buf;

		if ((buf = malloc(BUFSZ)) == NULL)
			return (-1);

		if (tg_darrpush(&(bufs->bufs), &buf) < 0)
			return (-1);

		bufs->end = ((void **)bufs->bufs.data)[bufs->bufs.cnt - 1];
	}

	bufs->end += bufs->sz;

	return bufs->lastid++;
}

static char *tg_dstrbufsget(struct tg_dstrbufs *bufs, int eln)
{
	int bufn, bufeln;
	char *p;
	
	assert(bufs != NULL);

	bufn = eln / (BUFSZ / bufs->sz);
	bufeln = eln % (BUFSZ / bufs->sz);

	return ((void **) bufs->bufs.data)[bufn] + bufeln * bufs->sz;
}

static int tg_dstrbufsadd(struct tg_dstrbufs *bufs, const char *str)
{
	int eln;
	char *p;
	size_t len;

	len = strlen(str);

	if ((eln = tg_dstrbufsalloc(bufs, len)) < 0)
		return (-1);
	
	p = tg_dstrbufsget(bufs, eln);

	memcpy(p, str, len + 1);
	p[len] = '\0';


	return eln;
}

static int tg_dstrbufsremove(struct tg_dstrbufs *bufs, int eln)
{
	assert(bufs != NULL);

	if (tg_darrpush(&(bufs->free), &eln) < 0)
		return (-1);

	return 0;
}

// init variables when clearing buf with wipe
static int tg_dstrinitbuf()
{
/*
	if (!mbufsinit) {
		if (tg_darrinit(&mbufs, sizeof(char *)) < 0)
			return (-1);

		mbufsinit = 1;
	}
*/	
	if (!bufs8init) {
		if (tg_dstrbufsinit(&bufs8, 8) < 0)
			return (-1);

		bufs8init = 1;
	}

	if (!bufs16init) {
		if (tg_dstrbufsinit(&bufs16, 16) < 0)
			return (-1);
		
		bufs16init = 1;
	}

	return 0;
}

int tg_dstralloc(struct tg_dstring *dstr, size_t len)
{
	struct tg_dstrbufs *bufs;
	
	if (tg_dstrinitbuf() < 0)
		return (-1);
	
	dstr->len = len;

	if (dstr->len < 8) {
		dstr->bufs = 8;
		bufs = &bufs8;
	}
	else if (dstr->len < 16) {
		dstr->bufs = 16;
		bufs = &bufs16;
	}
	else {
		dstr->bufs = 0;
		if ((dstr->str = malloc(dstr->len + 1)) == NULL)
			return (-1);

		dstr->sz = dstr->len + 1;

	//	if (tg_darrpush(&mbufs, dstr->str) < 0)
	//		return (-1);
	}

	if (dstr->bufs == 8 || dstr->bufs == 16) {
		if ((dstr->eln = tg_dstrbufsalloc(bufs, dstr->len)) < 0)
			return (-1);
	
		dstr->str = tg_dstrbufsget(bufs, dstr->eln);
	}

	return 0;
}

int tg_dstrcreaten(struct tg_dstring *dstr, const char *src, size_t len)
{
	if (tg_dstralloc(dstr, len) < 0)
		return (-1);
	
	memcpy(dstr->str, src, len);
	dstr->str[len] = '\0';

	return 0;
}

/*
int tg_dstrcreate(struct tg_dstring *dstr, const char *src)
{
	size_t len;

	len = strlen(src);

	if (tg_dstralloc(dstr, len) < 0)
		return (-1);
	
	memcpy(dstr->str, src, len);
	dstr->str[len] = '\0';

	return 0;
}
*/

int tg_dstrdestroy(struct tg_dstring *dstr)
{
	struct tg_dstrbufs *bufs;

	if (dstr->len < 8)
		bufs = &bufs8;
	else if (dstr->len < 16)
		bufs = &bufs16;
	else {
		bufs = NULL;
		free(dstr->str);
	}

	if (bufs != NULL) {
		if (tg_dstrbufsremove(bufs, dstr->eln) < 0)
			return (-1);
	}

	return 0;
}

int tg_dstraddstrn(struct tg_dstring *dstr, const char *src, size_t len)
{
	struct tg_dstring tmpdstr;

	if (dstr->bufs == 0) {	
		if (dstr->len + len + 1 > dstr->sz) {
			dstr->sz = (dstr->len + len + 1) * 3 / 2;
			if ((dstr->str = realloc(dstr->str,
				dstr->sz)) == NULL)
				return (-1);
		}
		
		memcpy(dstr->str + dstr->len, src, len);
		dstr->str[dstr->len + len] = '\0';
		
		dstr->len = dstr->len + len;
	
		return 0;
	}

	if (tg_dstralloc(&tmpdstr, dstr->len + len) < 0)
		return (-1);
	
	memcpy(tmpdstr.str, dstr->str, dstr->len + 1);
	memcpy(tmpdstr.str + dstr->len, src, len);

	tmpdstr.str[dstr->len + len] = '\0';
	
	tg_dstrdestroy(dstr);

	*dstr = tmpdstr;

	return 0;
}

/*
int tg_dstrwipe()
{
	char *p;

	tg_dstrbufsclear(&bufs8);
	tg_dstrbufsclear(&bufs16);

	while (tg_darrpop(&mbufs, &p) == 0)
		free(p);

	mbufsinit = bufs8init = bufs16init = 0;

	return 0;
}
*/

int buftest()
{
	struct tg_dstring str1;
	struct tg_dstring str2;
/*
	tg_dstrcreate(&str1, "xh3ksjx");
	tg_dstraddstr(&str1, "asdf");
	tg_dstraddstr(&str1, "sdfhesde_23sdf");

	tg_dstrcreate(&str2, "x");

	printf("%s\n", str1.str);
	printf("%s\n", str2.str);


	tg_dstrdestroy(&str1);
	tg_dstrdestroy(&str2);
	tg_dstrwipe();
*/
/*
	struct tg_dstrbufs bufs;
	int eln[1024 * 1024];
	char s[1024 * 1024];
	int i;

	tg_dstrbufsinit(&bufs, 2);

	int cnt;

	cnt = 1024 * 4;

	int j;
	int t;
	int added, removed;

	t = 0;
	for (j = 0; j < 64; ++j) {
		added = rand() % cnt;

		t += added;
		printf("adding %d elements\n", added);
		for (i = 0; i < added; ++i) {
			sprintf(s, "%d", i);
			
			printf("%d\n", i);
			
			eln[i] = tg_dstrbufsadd(&bufs, s);
		
			printf("%s\n", tg_dstrbufsget(&bufs, eln[i]));
		}
	
		if (added == 0)
			continue;

		removed = rand() % added;

		t -= removed;
		printf("removing %d elements\n", removed);
		for (i = 0; i < removed; ++i)
			tg_dstrbufsremove(&bufs, eln[i]);

		printf("\n");
	}

	printf("last added: %d\n", added);
	printf("last removed: %d\n", removed);
*/
}


