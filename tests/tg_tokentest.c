#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "../tg_parser.h"
#include "../tg_darray.h"
#include "../tg_dstring.h"
#include "../tg_common.h"

#include "../tg_parser.c"

int main(int argc, const char *argv[])
{
	struct tg_token t;
	int type;

	if (argc < 2)
		TG_ERROR("Usage: %s [source file]", "tokentest");

	if (tg_initparser(argv[1]) < 0)
		return 1;

	while ((type = tg_peektoken(&t)) != TG_T_EOF) {
		if (type == TG_T_ERROR)
			return 1;

		tg_gettoken(&t);
		printf("%s\n", t.val.str);
	}

	return 0;
}
