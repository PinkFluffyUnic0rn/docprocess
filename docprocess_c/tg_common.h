#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tg_darray.h"

#define TG_MSGMAXSIZE (1024 * 1024)

extern char tg_error[TG_MSGMAXSIZE];

#define TG_SETERROR(...)				\
	snprintf(tg_error, TG_MSGMAXSIZE - 1, __VA_ARGS__)

#define TG_ASSERT(expr, ...)					\
do {								\
	if (!(expr)) {						\
		fprintf(stderr, "file %s, line %d: %s: ",	\
			__FILE__, __LINE__, __func__);		\
		fprintf(stderr, __VA_ARGS__);			\
		fprintf(stderr, "\n");				\
		exit(1);					\
	}							\
} while (0);

#define TG_WARNING(...)					\
do {							\
	fprintf(stderr, __VA_ARGS__);			\
	fprintf(stderr, "\n");				\
} while (0);

#define TG_ERROR(...)					\
do {							\
	fprintf(stderr, "file %s, line %d: %s: ",	\
		__FILE__, __LINE__, __func__);		\
	fprintf(stderr, __VA_ARGS__);			\
	fprintf(stderr, "\n");				\
	exit(1);					\
} while (0);

#define TG_ERRQUIT(v) do { if ((v) < 0) return (-1); } while (0)

#define TG_NULLQUIT(v) do { if ((v) == NULL) return (NULL); } while (0)

#define TG_REHASHFACTOR 1.5

struct tg_hash {
	void **buckets;
	void *last;
	size_t bucketscount;
	size_t count;
};

#define TG_HASHFIELDS(STRT, NAME)	\
struct tg_dstring key##NAME;		\
STRT *prev##NAME;			\
STRT *next##NAME;			\

#define TG_HASHED(STRT, NAME)					\
								\
static void _tg_inithash##NAME(struct tg_hash *h, size_t bucketscount)	\
{								\
	int i;							\
								\
	h->buckets = malloc(sizeof(STRT *) * bucketscount);	\
	TG_ASSERT(h->buckets != NULL,				\
		"Cannot allocate memory for hash");		\
								\
	for (i = 0; i < bucketscount; ++i)			\
		h->buckets[i] = NULL;				\
								\
	h->bucketscount = bucketscount;				\
	h->count = 0;						\
}								\
								\
static void _tg_destroyhash##NAME(struct tg_hash *h)		\
{								\
	free(h->buckets);					\
								\
	h->bucketscount = 0;					\
	h->count = 0;						\
}								\
								\
static int tg_hashbucket##NAME(const char *str, int bucketscount)	\
{								\
	const char *p;						\
	unsigned int r;						\
								\
	r = 0;							\
	for (p = str; *p != '\0'; ++p)				\
		r += *p;					\
								\
	return r % bucketscount;				\
}								\
								\
void tg_hashset##NAME(struct tg_hash *h, const char *key, STRT *v);	\
								\
void tg_rehash##NAME(struct tg_hash *h)				\
{								\
	struct tg_hash hnew;					\
	int i;							\
								\
	_tg_inithash##NAME(&hnew, (h->bucketscount + 1) * TG_REHASHFACTOR);	\
								\
	for (i = 0; i < h->bucketscount; ++i) {			\
		STRT *p;					\
		STRT *next;					\
								\
		for (p = h->buckets[i]; p != NULL;) {		\
			next = p->next##NAME;			\
								\
			tg_dstrdestroy(&(p->key##NAME));	\
								\
			tg_hashset##NAME(&hnew, p->key##NAME.str, p);	\
								\
			p = next;				\
		}						\
	}							\
								\
	memcpy(h, &hnew, sizeof(struct tg_hash));		\
}								\
								\
void tg_hashset##NAME(struct tg_hash *h, const char *key, STRT *v)	\
{								\
	STRT *p;						\
	int b;							\
								\
	if (h->count > 0.8 * h->bucketscount)			\
		tg_rehash##NAME(h);				\
								\
	b = tg_hashbucket##NAME(key, h->bucketscount);		\
	p = h->buckets[b];					\
								\
	v->prev##NAME = v->next##NAME = NULL;			\
								\
	if (p == NULL) {					\
		h->buckets[b] = v;				\
		tg_dstrcreate(&(v->key##NAME), key);		\
		v->prev##NAME = NULL;				\
		v->next##NAME = NULL;				\
	}							\
	else {							\
		while (p != NULL) {				\
			if (strcmp(key, p->key##NAME.str) == 0) {	\
				if (p->prev##NAME != NULL) {		\
					p->prev##NAME->next##NAME = v;	\
					v->prev##NAME = p->prev##NAME;	\
				}				\
				else				\
					h->buckets[b] = v;	\
								\
				if (p->next##NAME != NULL) {		\
					p->next##NAME->prev##NAME = v;	\
					v->next##NAME = p->next##NAME;	\
				}				\
								\
				tg_dstrcreate(&(v->key##NAME), key);	\
								\
				break;				\
			}					\
								\
			if (p->next##NAME == NULL) {		\
				p->next##NAME = v;		\
				v->prev##NAME = p->next##NAME;	\
				tg_dstrcreate(&(v->key##NAME), key);	\
				break;				\
			}					\
								\
			p = p->next##NAME;			\
		}						\
	}							\
								\
	h->count++;						\
}								\
								\
STRT *tg_hashget##NAME(const struct tg_hash *h, const char *key)	\
{								\
	STRT *p;						\
	int b;							\
								\
	b = tg_hashbucket##NAME(key, h->bucketscount);		\
								\
	p = h->buckets[b];					\
	while (p != NULL) {					\
 		if (strcmp(key, p->key##NAME.str) == 0)		\
			break;					\
								\
		p = p->next##NAME;				\
	}							\
								\
	return p;						\
}								\
								\
void tg_hashdel##NAME(struct tg_hash *h, const char *key)	\
{								\
	STRT *p;						\
	int b;							\
								\
	b = tg_hashbucket##NAME(key, h->bucketscount);		\
								\
	p = h->buckets[b];					\
	while (p != NULL) {					\
 		if (strcmp(key, p->key##NAME.str) == 0)		\
			break;					\
								\
		p = p->next##NAME;				\
	}							\
								\
	tg_dstrdestroy(&(p->key##NAME));			\
								\
	if (p->prev##NAME != NULL)				\
		p->prev##NAME->next##NAME = p->next##NAME;	\
	else							\
		h->buckets[b] = p->next##NAME;			\
								\
	if (p->next##NAME != NULL)				\
		p->next##NAME->prev##NAME = p->prev##NAME;	\
								\
	h->count--;						\
}


#define tg_hash(NAME)	tg_hash##NAME

#define tg_inithash(NAME, h) \
	_tg_inithash##NAME((h), 4)

#define tg_destroyhash(NAME, h) _tg_destroyhash##NAME(h)

#define tg_hashset(NAME, h, key, v) \
	tg_hashset##NAME((h), (key), (v))

#define tg_hashget(NAME, h, key) \
	tg_hashget##NAME((h), (key))

#define tg_hashdel(NAME, h, key) \
	tg_hashdel##NAME((h), (key))

#define tg_hashkey(NAME, el) \
	(el).key##NAME.str

#define TG_HASHFOREACH(STRT, NAME, h, KEY, ACTIONS)		\
do {								\
	int i;							\
								\
	for (i = 0; i < (h).bucketscount; ++i) {			\
		STRT *p;					\
								\
		if ((h).buckets[i] == NULL)			\
			continue;				\
								\
		for (p = (h).buckets[i]; p != NULL; p = p->next##NAME) {		\
			KEY = p->key##NAME.str;			\
			ACTIONS					\
		}						\
	}							\
} while (0);

struct tg_allocator {
	struct tg_darray freed;
	struct tg_darray blocks;
	size_t sz;
	size_t valcount;
};

void tg_allocinit(struct tg_allocator *allocer, size_t sz);

void *tg_alloc(struct tg_allocator *allocer);

void tg_free(struct tg_allocator *allocer, void *p);

void tg_allocdestroy(struct tg_allocator *allocer,
	void (*destr)(void *));

char *tg_strdup(const char *s);

#endif
