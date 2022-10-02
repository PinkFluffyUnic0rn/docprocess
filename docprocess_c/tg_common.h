#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#define TG_MSGMAXSIZE (1024 * 1024)

extern char tg_error[TG_MSGMAXSIZE];

#define TG_SETERROR(...)				\
	snprintf(tg_error, TG_MSGMAXSIZE - 1, __VA_ARGS__)

#define TG_ASSERT(expr, ...)			\
do {						\
	if (!(expr)) {				\
		fprintf(stderr, __VA_ARGS__);	\
		exit(1);			\
	}					\
} while (0);

#endif
