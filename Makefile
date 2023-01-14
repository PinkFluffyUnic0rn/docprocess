all: build

build:
	gcc -Wall tg_common.c tg_value.c tg_parser.c tg_darray.c tg_dstring.c tg_inout.c tg_tablegen.c -o tablegen -lm
deb: build
	mkdir -p docprocess/usr/bin
	mkdir -p docprocess/usr/share/docprocess/utils
	mkdir -p docprocess/usr/share/docprocess/converters
	mkdir -p docprocess/usr/share/docprocess/include
	cp *.awk docprocess/usr/share/docprocess
	cp utils/* docprocess/usr/share/docprocess/utils
	cp converters/* docprocess/usr/share/docprocess/converters
	cp include/* docprocess/usr/share/docprocess/include
	cp template docprocess/usr/bin/
	cp tablegen docprocess/usr/bin/
	dpkg-deb --build --root-owner-group docprocess
cleandeb:
	rm -r docprocess/usr
	rm docprocess.deb
