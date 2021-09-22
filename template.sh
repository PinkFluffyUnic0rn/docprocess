#!/bin/sh

export LC_ALL=C

scriptdir=$(cd "$(dirname "$0")"; pwd -P)

trap "[ -n \"\$rmcmd\" ] && eval \"rm \$rmcmd\"" ERR EXIT

if [ $# -lt 2 ]
then
	echo "Usage: template.sh [template] [output extension] [parameters]" \
		1>&2
	exit 1
fi

if [ -z "$awktablespath" ]
then
	awktablespath="/usr/share/awk/awktables"

	# for standalone mode
	if [ ! -e "$awktablespath" ]
	then
		awktablespath="$scriptdir/awktables"
	fi
fi

if [ -z "$utilspath" ]
then
	utilspath="/usr/bin"

	# for standalone mode
	if [ ! -e "$utilspath/sqltocsv" -o ! -e "$utilspath/isqlreq" ]
	then
		utilspath="$scriptdir/utils"
	fi
fi

tablegenpath="$scriptdir/tablegen"

cmd="cat '$1' | awk -f '$awktablespath/error.awk' \
	-f '$scriptdir/sources.awk' -f '$scriptdir/template.awk' \
	-v 'A_scriptpath=$scriptdir' -v 'A_awktablespath=$awktablespath' \
	-v 'A_utilspath=$utilspath' -v 'A_tablegenpath=$tablegenpath'"

if [ $# -eq 3 ]
then
	cmd="$cmd -v 'A_source=$3'"
fi

res="/tmp/res_$$.html"
rmcmd="$rmcmd $res"

eval $cmd >"$res"
if [ $? -ne 0 ]
then
	exit 1
fi

if [ "$2" = "xls" ]
then
	resxls="/tmp/res_$$.xls"
	rmcmd="$rmcmd $resxls"

	libreoffice --headless "--infilter=HTML Document (Calc)" \
		--convert-to xls --outdir "/tmp" "$res" 2>&1 1>"/dev/null"

	res="$resxls"
fi

cat "$res"
if [ $? -ne 0 ]
then
	exit 1
fi

exit 0
