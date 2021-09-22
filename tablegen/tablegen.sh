#!/bin/sh

scriptdir=$(dirname $0)

if [ -z "$include" ]
then
	include="$scriptdir/include"
fi;

if [ -z "$awktablespath" ]
then
	awktablespath="/usr/share/awk/awktables"
fi;

(find "$include" -name '*.tg' | xargs cat; cat "$1") \
	| awk \
	-f "$awktablespath/error.awk" -f "$awktablespath/fileread.awk" \
	-f "$awktablespath/csvread.awk" -f "$awktablespath/node.awk" \
	-f "$awktablespath/parser.awk" -f "$awktablespath/table.awk" \
	-f "$scriptdir/parser.awk" -f "$scriptdir/outproc.awk" \
	-f "$scriptdir/tablegen.awk" -v "A_sources=$2" -v "A_outtype=$3"
