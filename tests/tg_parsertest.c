#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "../tg_parser.h"
#include "../tg_common.h"

int main(int argc, const char *argv[])
{
	int tpl;
	
	if (argc < 2)
		TG_ERROR("Usage: %s [source file]", "parsertest");
	
	if ((tpl = tg_getparsetree(argv[1])) < 0)
		TG_ERROR("%s", tg_error);

	tg_printnode(tpl, 0);
	
	return 0;
}
