#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "tg_value.h"

static int tabletest()
{
	struct tg_val *s1, *s2, *s3, *s4, *s5, *s6, *s7, *t;
		
	s1 = tg_stringval("test");
	s2 = tg_stringval("asdf");
	s3 = tg_stringval("sd");
	s4 = tg_stringval("asd");

	s5 = t = tg_valnextto(s1, s2, 1, 0);
	
	t = tg_valnextto(t, s3, 1, 0);
	
	t = tg_valnextto(t, s4, 0, 0);

	t = tg_valnextto(t, s5, 0, 0);

	s6 = tg_valnextto(tg_intval(67), tg_stringval("432"), 0, 0);

	t = tg_valnextto(t, s6, 1, 0);

	s7 = tg_valnextto(tg_intval(67), tg_stringval("432"), 0, 0);
	s7 = tg_valnextto(s7, s2, 0, 0);
	s7 = tg_valnextto(s7, s2, 0, 0);
	s7 = tg_valnextto(s7, s2, 0, 0);

	t = tg_valnextto(t, s7, 1, 0);

	tg_printval(stdout, t);
	printf("\n");

	return 0;
}

static void spantest()
{
	struct tg_val *s1, *s2;
		
	s1 = tg_valnextto(tg_stringval("test"), tg_intval(32), 0, 0);
	s1 = tg_valnextto(s1, tg_intval(2352), 0, 0);
	s1 = tg_valnextto(s1, tg_stringval("asdsa"), 0, 0);

	s2 = tg_valnextto(tg_intval(482), tg_stringval("HELLO"), 0, 0);
	s2 = tg_valnextto(s2, tg_stringval("523"), 0, 0);
	s2 = tg_valnextto(s2, tg_stringval("zds"), 0, 0);
	s1 = tg_valnextto(s1, s2, 1, 0);

	s2 = tg_valnextto(tg_stringval("234"), tg_stringval("value"), 0, 0);
	s1 = tg_valnextto(s1, s2, 1, 1);

	s1 = tg_valnextto(s1, tg_stringval("axs"), 1, 1);

	s2 = tg_valnextto(tg_intval(234), tg_intval(5254), 1, 0);
	s1 = tg_valnextto(s1, s2, 0, 1);

	s1 = tg_valnextto(s1, tg_intval(32), 0, 1);
	
	s1 = tg_valnextto(s1, tg_stringval("xzda"), 1, 0);
	
	s2 = tg_valnextto(tg_intval(234), tg_intval(5254), 0, 0);
	s1 = tg_valnextto(s1, s2, 1, 1);

	s1 = tg_valnextto(s1, tg_stringval("dhj3h"), 0, 0);

	s2 = tg_valnextto(tg_intval(-121), tg_stringval("xds"), 1, 0);
	s1 = tg_valnextto(s1, s2, 0, 1);

	tg_printval(stdout, tg_castval(s1, TG_VAL_TABLE));
	printf("\n");
}

static void operatortest()
{
	struct tg_val *vint1, *vint2,
		*vfloat1, *vfloat2,
		*vstr1, *vstr2,
		*vnumstr1, *vnumstr2,
		*varr1, *varr2,
		*vsarr1, *vsarr2;

	// scalar values
	printf("scalar values (%d, %d, %f, %f, %s, %s, %s, %s):\n",
		245, 83, 31.5, 812.49, "testing1", "testing2",
		"532.2", "2836");

	vint1 = tg_intval(245);
	printf("\t");
	tg_printval(stdout, vint1);
	printf("\n");
	
	vint2 = tg_intval(83);
	printf("\t");
	tg_printval(stdout, vint2);
	printf("\n");


	vfloat1 = tg_floatval(31.5);
	printf("\t");
	tg_printval(stdout, vfloat1);
	printf("\n");
	
	vfloat2 = tg_floatval(812.49);
	printf("\t");
	tg_printval(stdout, vfloat2);
	printf("\n");

	vstr1 = tg_stringval("testing1");
	printf("\t");
	tg_printval(stdout, vstr1);
	printf("\n");

	vstr2 = tg_stringval("testing2");
	printf("\t");
	tg_printval(stdout, vstr2);
	printf("\n");

	vnumstr1 = tg_stringval("532.2");
	printf("\t");
	tg_printval(stdout, vnumstr1);
	printf("\n");

	vnumstr2 = tg_stringval("2836");
	printf("\t");
	tg_printval(stdout, vnumstr2);
	printf("\n");

	varr1 = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(varr1, vint2);
	tg_arrpush(varr1, vfloat1);
	printf("\t");
	tg_printval(stdout, varr1);
	printf("\n");

	varr2 = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(varr2, vint1);
	tg_arrpush(varr2, vfloat2);
	printf("\t");
	tg_printval(stdout, varr2);
	printf("\n");

	vsarr1 = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(vsarr1, vint1);
	printf("\t");
	tg_printval(stdout, vsarr1);
	printf("\n");

	vsarr2 = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(vsarr2, vnumstr1);
	printf("\t");
	tg_printval(stdout, vsarr2);
	printf("\n\n");

	struct tg_val *vres;

	vres = tg_valadd(vint1, vint2);
	printf("\tint + int:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valadd(vfloat1, vfloat2);
	printf("\tfloat + float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valadd(vstr1, vstr2);
	printf("\tstr + str:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valadd(vnumstr1, vnumstr2);
	printf("\tnstr + nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valadd(vint1, vfloat1);
	printf("\tint + float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valadd(vint1, vstr1);
	printf("\tint + string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valadd(vint1, vstr1);
	printf("\tint + nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valadd(vfloat1, vstr1);
	printf("\tfloat + string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valadd(vfloat1, vnumstr1);
	printf("\tfloat + nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valadd(vstr1, vnumstr1);
	printf("\tstring + nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////

	vres = tg_valsub(vint1, vint2);
	printf("\tint - int:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valsub(vfloat1, vfloat2);
	printf("\tfloat - float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valsub(vstr1, vstr2);
	printf("\tstr - str:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valsub(vnumstr1, vnumstr2);
	printf("\tnstr - nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valsub(vint1, vfloat1);
	printf("\tint - float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valsub(vint1, vstr1);
	printf("\tint - string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valsub(vint1, vstr1);
	printf("\tint - nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valsub(vfloat1, vstr1);
	printf("\tfloat - string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valsub(vfloat1, vnumstr1);
	printf("\tfloat - nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valsub(vstr1, vnumstr1);
	printf("\tstring - nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////

	vres = tg_valmult(vint1, vint2);
	printf("\tint * int:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valmult(vfloat1, vfloat2);
	printf("\tfloat * float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valmult(vstr1, vstr2);
	printf("\tstr * str:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valmult(vnumstr1, vnumstr2);
	printf("\tnstr * nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valmult(vint1, vfloat1);
	printf("\tint * float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valmult(vint1, vstr1);
	printf("\tint * string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valmult(vint1, vstr1);
	printf("\tint * nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valmult(vfloat1, vstr1);
	printf("\tfloat * string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valmult(vfloat1, vnumstr1);
	printf("\tfloat * nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valmult(vstr1, vnumstr1);
	printf("\tstring * nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////

	vres = tg_valdiv(vint1, vint2);
	printf("\tint / int:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valdiv(vfloat1, vfloat2);
	printf("\tfloat / float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valdiv(vstr1, vstr2);
	printf("\tstr / str:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valdiv(vnumstr1, vnumstr2);
	printf("\tnstr / nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valdiv(vint1, vfloat1);
	printf("\tint / float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valdiv(vint1, vstr1);
	printf("\tint / string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valdiv(vint1, vstr1);
	printf("\tint / nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valdiv(vfloat1, vstr1);
	printf("\tfloat / string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valdiv(vfloat1, vnumstr1);
	printf("\tfloat / nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valdiv(vstr1, vnumstr1);
	printf("\tstring / nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////

	vres = tg_valcat(vint1, vint2);
	printf("\tint ~ int:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valcat(vfloat1, vfloat2);
	printf("\tfloat ~ float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valcat(vstr1, vstr2);
	printf("\tstr ~ str:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valcat(vnumstr1, vnumstr2);
	printf("\tnstr ~ nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valcat(vint1, vfloat1);
	printf("\tint ~ float:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valcat(vint1, vstr1);
	printf("\tint ~ string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valcat(vint1, vstr1);
	printf("\tint ~ nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valcat(vfloat1, vstr1);
	printf("\tfloat ~ string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valcat(vfloat1, vnumstr1);
	printf("\tfloat ~ nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valcat(vstr1, vnumstr1);
	printf("\tstring ~ nstr:\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_valeq(vint1, vint1);
	printf("\tint == int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valeq(vint1, vint2);
	printf("\tint == int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valeq(vfloat1, vfloat2);
	printf("\tfloat == float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valeq(vstr1, vstr2);
	printf("\tstr == str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valeq(vnumstr1, vnumstr2);
	printf("\tnstr == nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valeq(vint1, vfloat1);
	printf("\tint == float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valeq(vint1, vstr1);
	printf("\tint == string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valeq(vint1, vstr1);
	printf("\tint == nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valeq(vfloat1, vstr1);
	printf("\tfloat == string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valeq(vfloat1, vnumstr1);
	printf("\tfloat == nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valeq(vstr1, vnumstr1);
	printf("\tstring == nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_valneq(vint1, vint1);
	printf("\tint != int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valneq(vint1, vint2);
	printf("\tint != int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valneq(vfloat1, vfloat2);
	printf("\tfloat != float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valneq(vstr1, vstr2);
	printf("\tstr != str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valneq(vnumstr1, vnumstr2);
	printf("\tnstr != nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valneq(vint1, vfloat1);
	printf("\tint != float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valneq(vint1, vstr1);
	printf("\tint != string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valneq(vint1, vstr1);
	printf("\tint != nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valneq(vfloat1, vstr1);
	printf("\tfloat != string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valneq(vfloat1, vnumstr1);
	printf("\tfloat != nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valneq(vstr1, vnumstr1);
	printf("\tstring != nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_valls(vint1, vint1);
	printf("\tint < int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valls(vint1, vint2);
	printf("\tint < int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valls(vfloat1, vfloat2);
	printf("\tfloat < float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valls(vstr1, vstr2);
	printf("\tstr < str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valls(vnumstr1, vnumstr2);
	printf("\tnstr < nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valls(vint1, vfloat1);
	printf("\tint < float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valls(vint1, vstr1);
	printf("\tint < string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valls(vint1, vstr1);
	printf("\tint < nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valls(vfloat1, vstr1);
	printf("\tfloat < string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valls(vfloat1, vnumstr1);
	printf("\tfloat < nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valls(vstr1, vnumstr1);
	printf("\tstring < nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_valgr(vint1, vint1);
	printf("\tint > int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgr(vint1, vint2);
	printf("\tint > int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgr(vfloat1, vfloat2);
	printf("\tfloat > float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgr(vstr1, vstr2);
	printf("\tstr > str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgr(vnumstr1, vnumstr2);
	printf("\tnstr > nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valgr(vint1, vfloat1);
	printf("\tint > float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgr(vint1, vstr1);
	printf("\tint > string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgr(vint1, vstr1);
	printf("\tint > nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valgr(vfloat1, vstr1);
	printf("\tfloat > string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgr(vfloat1, vnumstr1);
	printf("\tfloat > nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valgr(vstr1, vnumstr1);
	printf("\tstring > nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_vallseq(vint1, vint1);
	printf("\tint <= int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_vallseq(vint1, vint2);
	printf("\tint <= int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_vallseq(vfloat1, vfloat2);
	printf("\tfloat <= float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_vallseq(vstr1, vstr2);
	printf("\tstr <= str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_vallseq(vnumstr1, vnumstr2);
	printf("\tnstr <= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_vallseq(vint1, vfloat1);
	printf("\tint <= float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_vallseq(vint1, vstr1);
	printf("\tint <= string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_vallseq(vint1, vstr1);
	printf("\tint <= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_vallseq(vfloat1, vstr1);
	printf("\tfloat <= string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_vallseq(vfloat1, vnumstr1);
	printf("\tfloat <= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_vallseq(vstr1, vnumstr1);
	printf("\tstring <= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_valgreq(vint1, vint1);
	printf("\tint >= int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgreq(vint1, vint2);
	printf("\tint >= int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgreq(vfloat1, vfloat2);
	printf("\tfloat >= float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgreq(vstr1, vstr2);
	printf("\tstr >= str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgreq(vnumstr1, vnumstr2);
	printf("\tnstr >= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valgreq(vint1, vfloat1);
	printf("\tint >= float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgreq(vint1, vstr1);
	printf("\tint >= string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgreq(vint1, vstr1);
	printf("\tint >= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valgreq(vfloat1, vstr1);
	printf("\tfloat >= string:\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valgreq(vfloat1, vnumstr1);
	printf("\tfloat >= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valgreq(vstr1, vnumstr1);
	printf("\tstring >= nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_valand(vint1, vint1);
	printf("\tint & int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valand(vint1, vint2);
	printf("\tint & int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valand(vfloat1, vfloat2);
	printf("\tfloat & float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valand(vstr1, vstr2);
	printf("\tstr & str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valand(vnumstr1, vnumstr2);
	printf("\tnstr & nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valand(vint1, vfloat1);
	printf("\tint & float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valand(vint1, vstr1);
	printf("\tint & string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valand(vint1, vstr1);
	printf("\tint & nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valand(vfloat1, vstr1);
	printf("\tfloat & string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valand(vfloat1, vnumstr1);
	printf("\tfloat & nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valand(vstr1, vnumstr1);
	printf("\tstring & nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

///////////////////////////////////////////////////////////////////////
	vres = tg_valor(vint1, vint1);
	printf("\tint | int (same):\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valor(vint1, vint2);
	printf("\tint | int:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valor(vfloat1, vfloat2);
	printf("\tfloat | float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valor(vstr1, vstr2);
	printf("\tstr | str:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valor(vnumstr1, vnumstr2);
	printf("\tnstr | nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valor(vint1, vfloat1);
	printf("\tint | float:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valor(vint1, vstr1);
	printf("\tint | string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valor(vint1, vstr1);
	printf("\tint | nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valor(vfloat1, vstr1);
	printf("\tfloat | string:\t\t");
	tg_printval(stdout, vres);
	printf("\n");

	vres = tg_valor(vfloat1, vnumstr1);
	printf("\tfloat | nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");

	vres = tg_valor(vstr1, vnumstr1);
	printf("\tstring | nstr:\t\t");
	tg_printval(stdout, vres);
	printf("\n\n");
}


static void casttest()
{
	struct tg_val *vint, *vfloat, *vstr, *vnumstr;
	struct tg_val *vintarr, *vfloatarr, *vstrarr, *vnumstrarr;
	struct tg_val *varr;

	// scalar values
	printf("scalar values (%d %f %s %s):\n",
		415, 522.34, "teststring", "832.23");

	vint = tg_intval(415);
	printf("\t");
	tg_printval(stdout, vint);
	printf("\n");

	vfloat = tg_floatval(522.34);
	printf("\t");
	tg_printval(stdout, vfloat);
	printf("\n");

	vstr = tg_stringval("teststring");
	printf("\t");
	tg_printval(stdout, vstr);
	printf("\n");

	vnumstr = tg_stringval("832.23");
	printf("\t");
	tg_printval(stdout, vnumstr);
	printf("\n\n");

	// single-element arrays with scalar values
	printf("single-element array values:\n");

	vintarr = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(vintarr, vint);
	printf("\t");
	tg_printval(stdout, vintarr);
	printf("\n");

	vfloatarr = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(vfloatarr, vfloat);
	printf("\t");
	tg_printval(stdout, vfloatarr);
	printf("\n");

	vstrarr = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(vstrarr, vstr);
	printf("\t");
	tg_printval(stdout, vstrarr);
	printf("\n");

	vnumstrarr = tg_createval(TG_VAL_ARRAY);
	tg_arrpush(vnumstrarr, vnumstr);
	printf("\t");
	tg_printval(stdout, vnumstrarr);
	printf("\n\n");

	// array value
	varr = tg_createval(TG_VAL_ARRAY);

	tg_arrpush(varr, vint);
	tg_arrpush(varr, vfloat);
	tg_arrpush(varr, vstr);
	tg_arrpush(varr, vnumstr);
	tg_arrpush(varr, vnumstrarr);

	tg_printval(stdout, varr);
	printf("\n");
	printf("\n");

	
	//struct tg_val *v7, *v8, *v9, *v10, *v11, *v12, *v13, *v14;
	struct tg_val *itofv, *itosv,
		      *ftoiv, *ftosv,
		      *stoiv, *stofv, *nstrtoiv, *nstrtofv;

	printf("casting scalar types:\n");
	
	itofv = tg_castval(vint, TG_VAL_FLOAT);
	printf("\tint -> float: ");
	tg_printval(stdout, itofv);
	printf("\n");

	itosv = tg_castval(vint, TG_VAL_STRING);
	printf("\tint -> string: ");
	tg_printval(stdout, itosv);
	printf("\n");

	ftoiv = tg_castval(vfloat, TG_VAL_INT);
	printf("\tfloat -> int: ");
	tg_printval(stdout, ftoiv);
	printf("\n");

	ftosv = tg_castval(vfloat, TG_VAL_STRING);
	printf("\tfloat -> string: ");
	tg_printval(stdout, ftosv);
	printf("\n");

	stoiv = tg_castval(vstr, TG_VAL_INT);
	printf("\tstring -> int: ");
	tg_printval(stdout, stoiv);
	printf("\n");

	stofv = tg_castval(vstr, TG_VAL_FLOAT);
	printf("\tstring -> float: ");
	tg_printval(stdout, stofv);
	printf("\n");

	nstrtoiv = tg_castval(vnumstrarr, TG_VAL_INT);
	nstrtofv = tg_castval(vnumstrarr, TG_VAL_FLOAT);

	printf("\tnumeric string -> int: ");
	tg_printval(stdout, nstrtoiv);
	printf("\n");
	printf("\tnumeric string -> float: ");
	tg_printval(stdout, nstrtofv);
	printf("\n");
	printf("\n");


	struct tg_val *itoav, *ftoav, *stoav, *nstoav,
		*iarrtoiv, *iarrtofv, *iarrtosv,
		*farrtoiv, *farrtofv, *farrtosv,
		*sarrtoiv, *sarrtofv, *sarrtosv,
		*nsarrtoiv, *nsarrtofv, *nsarrtosv;

	printf("casting with arrays:\n");

	itoav = tg_castval(vint, TG_VAL_ARRAY);
	printf("\tint -> array: ");
	tg_printval(stdout, itoav);
	printf("\n");
	
	ftoav = tg_castval(vfloat, TG_VAL_ARRAY);
	printf("\tfloat -> array: ");
	tg_printval(stdout, ftoav);
	printf("\n");
	
	stoav = tg_castval(vstr, TG_VAL_ARRAY);
	printf("\tstring -> array: ");
	tg_printval(stdout, stoav);
	printf("\n");
	
	nstoav = tg_castval(vnumstr, TG_VAL_ARRAY);
	printf("\tnumeric string -> array: ");
	tg_printval(stdout, nstoav);
	printf("\n");

	iarrtoiv = tg_castval(vint, TG_VAL_INT);
	printf("\tone-element array (int) -> int: ");
	tg_printval(stdout, iarrtoiv);
	printf("\n");

	iarrtofv = tg_castval(vint, TG_VAL_FLOAT);
	printf("\tone-element array (int) -> float: ");
	tg_printval(stdout, iarrtofv);
	printf("\n");
		
	iarrtosv = tg_castval(vint, TG_VAL_STRING);
	printf("\tone-element array (int) -> string: ");
	tg_printval(stdout, iarrtosv);
	printf("\n");


	farrtoiv = tg_castval(vfloat, TG_VAL_INT);
	printf("\tone-element array (float) -> int: ");
	tg_printval(stdout, farrtoiv);
	printf("\n");

	farrtofv = tg_castval(vfloat, TG_VAL_FLOAT);
	printf("\tone-element array (float) -> float: ");
	tg_printval(stdout, farrtofv);
	printf("\n");
		
	farrtosv = tg_castval(vfloat, TG_VAL_STRING);
	printf("\tone-element array (float) -> string: ");
	tg_printval(stdout, farrtosv);
	printf("\n");

	sarrtoiv = tg_castval(vstr, TG_VAL_INT);
	printf("\tone-element array (string) -> int: ");
	tg_printval(stdout, sarrtoiv);
	printf("\n");

	sarrtofv = tg_castval(vstr, TG_VAL_FLOAT);
	printf("\tone-element array (string) -> float: ");
	tg_printval(stdout, sarrtofv);
	printf("\n");
		
	sarrtosv = tg_castval(vstr, TG_VAL_STRING);
	printf("\tone-element array (string) -> string: ");
	tg_printval(stdout, sarrtosv);
	printf("\n");

	nsarrtoiv = tg_castval(vnumstr, TG_VAL_INT);
	printf("\tone-element array (numeric string) -> int: ");
	tg_printval(stdout, nsarrtoiv);
	printf("\n");

	nsarrtofv = tg_castval(vnumstr, TG_VAL_FLOAT);
	printf("\tone-element array (numeric string) -> float: ");
	tg_printval(stdout, nsarrtofv);
	printf("\n");
		
	nsarrtosv = tg_castval(vnumstr, TG_VAL_STRING);
	printf("\tone-element array (numeric string) -> string: ");
	tg_printval(stdout, nsarrtosv);
	printf("\n\n");

}

static void arraytest(int bigarrsize, int deeparrsize)
{
	struct tg_val *vbigarr;
	struct tg_val *vdeeparr;
	struct tg_val *p, *pp;
	int i;

	vbigarr = tg_createval(TG_VAL_ARRAY);

	for (i = 0; i < bigarrsize; ++i)
		tg_arrpush(vbigarr, tg_intval(i));
	
	tg_printval(stdout, vbigarr);
	printf("\n^ a big array");
	printf("\n\n");

	vdeeparr = pp = tg_createval(TG_VAL_ARRAY);

	for (i = 0; i < deeparrsize; ++i) {
		p = tg_createval(TG_VAL_ARRAY);

		tg_arrpush(pp, p);

		pp = tg_arrgetr(pp, 0);
	}
	
	tg_arrpush(pp, tg_stringval("test"));
	
	tg_printval(stdout, vdeeparr);
	printf("\n^ a deep array");
	printf("\n\n");
}

static int allocatortest()
{
	int i;

	tg_startframe();
	
	for (i = 0; i < 1000; ++i) {
		tg_stringval("test");
	}
	
	tg_endframe();

	return 0;
}

void tg_testvalues()
{
/*
	int i;

	for (i = 0; i < 100000; ++i) {
		tg_startframe();
		allocatortest();
		tg_endframe();
	}
*/
/*
	int i;
	
	for (i = 0; i < 100000; ++i) {
		tg_startframe();
		tabletest();
		tg_endframe();
	}
*/	
		
	allocatortest();

	tabletest();
	
	casttest();
	
	arraytest(100, 100);

	operatortest();

	spantest();
}

int main(int argc, const char *argv[])
{
	tg_startframe();
	
	tg_testvalues();

	tg_endframe();
}
