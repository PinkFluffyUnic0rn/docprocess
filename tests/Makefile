all:
	gcc -Wall ../tg_common.c ../tg_value.c ../tg_darray.c ../tg_dstring.c tg_tokentest.c -o tokentest -lm
	gcc -Wall ../tg_common.c ../tg_value.c ../tg_parser.c ../tg_darray.c ../tg_dstring.c tg_parsertest.c -o parsertest -lm
	gcc -Wall ../tg_common.c ../tg_value.c ../tg_parser.c ../tg_darray.c ../tg_dstring.c tg_valuetest.c -o valuetest -lm
	gcc -Wall tg_csvtest.c ../tg_common.c ../tg_value.c ../tg_darray.c ../tg_dstring.c -o csvtest -lm
