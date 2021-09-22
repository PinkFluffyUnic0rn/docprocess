# Table[] is a global associative array. Can't be redifined
# Tablecount is a global integer variable. Can't be redifined
#
# Table attribute:
#	Table[tid,attr]
#	, where tid is the table id, and attr is the attribute.
#	
#	possible table attributes:
# 		"rows" -- table rows count 
# 		"cols" -- table cols count
# 		"colname",x -- name of column x
#		"colid",name -- id of column with name [name]
# 		"rowname",y -- name of row y
#		"rowid",name -- id of column with name [name]
#		"attrname",x -- name of cell attr with id x
#		"attrs" -- cell attr count
#
# Table cell attribute:
# 	Table[tid,x,y,attr]
# 	, where tid is the table id,
#	x and y is the coordinates of a cell,
#	and attr is the attribute of a cell
#
#	built in table cell attributes:
# 		"val" -- value of the cell
#		"type" -- value type of the cell
#
# Global:
#	Table
#	Ta_var
#
function ta_printtable(t,	i, j, k, attr)
{
	for (i = 1; i <= Table[t,"cols"]; ++i) {
		printf("|%s %s|%s", Table[t,"colname",i],
			Table[t,"colid",Table[t,"colname",i]],
			i != Table[t,"cols"] ? " " : "\n");
	}

	for (i = 1; i <= Table[t,"rows"]; ++i) {
		for (j = 1; j <= Table[t,"cols"]; ++j) {
			printf("|");
			for (k = 1; k <= Table[t,"attrs"]; ++k) {
				attr = Table[t,"attrname",k];
				printf("%s%s", Table[t,i,j,attr],
					(k != Table[t,"attrs"]) \
					? "," : "");
			}

			printf("|%s", j != Table[t,"cols"] ? " " : "");
		}
		if (i != Table[t,"rows"])
			printf("\n");
	}
}

# reading csv to table
#----------------------------------------------------------------------
function ta_typeof(v)
{
	if (match(v, /^[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?$/))
		return TA_FLOAT;
	else if (match(v, /^[0-9]+$/))
		return TA_INT;

	return TA_STRING;
}

function ta_awkval(val, type)
{
	if (type == TA_INT)
		return val + 0;
	else if (type == TA_FLOAT)
		return val + 0.0;
	else
		return val;
}

function ta_csvtotable(fpath,	defattr,
	l, t, r, fc, fname, row, rows, i, j)
{
	t = ++Tablecount;

	r = (getline l <fpath);

	Table[t,"attrname",1] = "val";
	Table[t,"attrname",2] = "type";
	Table[t,"attrs"] = 2;
	for (i in defattr)
		Table[t,"attrname",++Table[t,"attrs"]] = i;

	fc = split(l, fname, ";");

	for (i = 1; i <= fc; ++i) {
		Table[t,"colid",fname[i]] = i;
		Table[t,"colname",i] = fname[i];
	}

	rows = 0;
	while (csv_getrecord(fpath, row) > 0) {
		++rows;
		for (i = 1; i <= fc; ++i) {
			Table[t,rows,i,"type"] = ta_typeof(row[i]);
			Table[t,rows,i,"val"] = ta_awkval(row[i],
				ta_typeof(row[i]));
			for (j in defattr)
				Table[t,rows,i,j] = defattr[j];
		}
	}

	Table[t,"rows"] = rows;
	Table[t,"cols"] = fc;
	
	return t;
}

# filtering tables
#----------------------------------------------------------------------
function ta_dateformat(s)
{
	if (s in Dateformat)
		return Dateformat[s];
	else if (s ~ /^[0-9][0-9][0-9][0-9]\-[0-9][0-9]\-[0-9][0-9]$/)
		return (Dateformat[s] = TA_ISODATE);
	else if (s ~ /^[0-9][0-9]\.[0-9][0-9]\.[0-9][0-9][0-9][0-9]$/)
		return (Dateformat[s] = TA_RUDATE);
	else if (s ~ /^[0-9]+\/[0-9]+\/[0-9][0-9][0-9][0-9]$/)
		return (Dateformat[s] = TA_USDATE);
	
	return 0;
}

function ta_day(s,	f, a)
{
	assert(f = ta_dateformat(s), "Wrong date format: " s);

	if (f == TA_ISODATE)
		return substr(s, 9, 2) + 0;
	else if (f == TA_RUDATE)
		return substr(s, 1, 2) + 0;
	else if (f == TA_USDATE) {
		split(s, a, /\//);
		return a[2];
	}
}

function ta_month(s,	f, a)
{
	assert(f = ta_dateformat(s), "Wrong date format: " s);

	if (f == TA_ISODATE)
		return substr(s, 6, 2) + 0;
	else if (f == TA_RUDATE)
		return substr(s, 4, 2) + 0;
	else if (f == TA_USDATE) {
		split(s, a, /\//);
		return a[1];
	}
}

function ta_year(s,	f, a)
{
	assert(f = ta_dateformat(s), "Wrong date format: " s);

	if (f == TA_ISODATE)
		return substr(s, 1, 4) + 0;
	else if (f == TA_RUDATE)
		return substr(s, 7, 4) + 0;
	else if (f == TA_USDATE) {
		split(s, a, /\//);
		return a[3];
	}
}

function ta_datedays(d,	M, s, y)
{
	y = ta_year(d);
	split("0;31;59;90;120;151;181;212;243;273;304;334;365", M, ";");

	return (365 * (y - 1) + M[ta_month(d)] + ta_day(d) \
		+ int(y / 4) + int(y / 400) - int(y / 100));
}

function ta_datediff(d1, d2, s)
{
	assert(ta_dateformat(d1), "Wrong date format: " d1);
	assert(ta_dateformat(d2), "Wrong date format: " d2);
	
	if (s == "day")
		return (ta_datedays(d2) - ta_datedays(d1));
	else if (s == "month") {
		return ((ta_year(d2) - ta_year(d1)) * 12 \
			+ (ta_month(d2) - ta_month(d1)));
	}
	else if (s == "year")
		return (ta_year(d2) - ta_year(d1));
}

function ta_datecmp(d1, d2)
{
	if (ta_year(d1) < ta_year(d2))
		return (-1);
	else if (ta_year(d1) > ta_year(d2))
		return 1;
	else {
		if (ta_month(d1) < ta_month(d2))
			return (-1);
		else if (ta_month(d1) > ta_month(d2))
			return 1;
		else {
			if (ta_day(d1) < ta_day(d2))
				return (-1);
			else  if (ta_day(d1) > ta_day(d2))
				return 1;
			else
				return 0;
		}
	}
}

function ta_reftonum(t, l, ftype, val,	id)
{
	if (node_type(val) == FP_T_INT)
		return node_val(val) + 0;
	else if (node_type(val) == FP_T_STRING) {
		if (ftype == TA_RF)
			return Table[t,"colid",node_val(val)];
		else
			return Table[t,"rowid",node_val(val)];
	}
	else if (node_type(val) == FP_T_ID) {
		id = node_val(val);	
		if (Ta_var[id,"type"] == TA_INT)
			return Ta_var[id];
		else if (Ta_var[id,"type"] == TA_FLOAT) {
			error("Variable " id " is used as line " \
				"reference and cannot be a float.");
		}
		else if (Ta_var[id,"type"] == TA_STRING) {
			if (ftype == TA_RF)
				return Table[t,"colid",Ta_var[id]];
			else
				return Table[t,"rowid",Ta_var[id]];
		}
	}
}

function ta_assertargs(args, c)
{
	assert(node_childrencount(args) == c,
		"Expected " c " arguments, but got " \
		node_childrencount(args) ".");
}

function ta_argval(t, l, ftype, a, i)
{
	return ta_expr(t, l, ftype, node_child(a, i));
}

function ta_call(t, l, ftype, v, args, id)
{
	id = node_val(node_child(v, 1));
	args = node_child(v, 2);

	if (id == "match") {
		ta_assertargs(args, 2);
		return match(ta_argval(t, l, ftype, args, 1),
			ta_argval(t, l, ftype, args, 2));
	}
	else if (id == "cat") {
		ta_assertargs(args, 2);
		return ta_argval(t, l, ftype, args, 1) \
			ta_argval(t, l, ftype, args, 2);
	}
	else if (id == "substr") {
		ta_assertargs(args, 3);
		return substr(ta_argval(t, l, ftype, args, 1),
				ta_argval(t, l, ftype, args, 2),
				ta_argval(t, l, ftype, args, 3));
	}
	else if (id == "length") {
		ta_assertargs(args, 1);
		return ta_argval(t, l, ftype, args, 1);
	}
	else if (id == "int") {
		ta_assertargs(args, 1);
		return ta_argval(t, l, ftype, args, 1) + 0;
	}
	else if (id == "float") {
		ta_assertargs(args, 1);
		return ta_argval(t, l, ftype, args, 1) + 0.0;
	}
	else if (id == "string") {
		ta_assertargs(args, 1);
		return ta_argval(t, l, ftype, args, 1) "";
	}
	else if (id == "day") {
		ta_assertargs(args, 1);
		return ta_day(ta_argval(t, l, ftype, args, 1));
	}
	else if (id == "month") {
		ta_assertargs(args, 1);
		return ta_month(ta_argval(t, l, ftype, args, 1));
	}
	else if (id == "year") {
		ta_assertargs(args, 1);
		return ta_year(ta_argval(t, l, ftype, args, 1));
	}
	else if (id == "datediff") {
		ta_assertargs(args, 3);
		return ta_datediff( \
			ta_argval(t, l, ftype, args, 1),
			ta_argval(t, l, ftype, args, 2),
			ta_argval(t, l, ftype, args, 3));
	}
	else if (id == "datecmp") {
		ta_assertargs(args, 2);

		return ta_datecmp( \
			ta_argval(t, l, ftype, args, 1),
			ta_argval(t, l, ftype, args, 2));
	}
	else
		error("Unknown function: " id ".");
}

function ta_id(t, l, ftype, v,	args, id)
{
	if (node_childrencount(v) == 1)
		return Ta_var[node_val(node_child(v, 1))];
	else
		return ta_call(t, l, ftype, v, args, id);
}

function ta_val(t, l, ftype, v,	type, ref)
{
	type = node_type(v)
	if (type == FP_N_EXPR)
		return ta_expr(t, l, ftype, v);
	else if (type == FP_N_NOT)
		return !ta_val(t, l, ftype, node_child(v, 1));
	else if (type == FP_N_ID)
		return ta_id(t, l, ftype, v);
	else if (type == FP_N_SIGN) {
		return  (node_val(node_child(v, 1)) == "-") \
			? -ta_val(node_child(v, 2)) \
			: ta_val(node_child(v, 2))
	}
	else if (type == FP_N_REF) {
		v = ta_reftonum(t, l, ftype, node_child(v, 1));
		
		return (ftype == TA_RF) \
			? Table[t,l,v,"val"] : Table[t,v,l,"val"];
	}
	else if (type == FP_T_INT)
		return (node_val(v) + 0);
	else if (type == FP_T_FLOAT)
		return (node_val(v) + 0.0);
	
	return node_val(v);
}

function ta_mult(t, l, ftype, m,	i, p, v)
{
	p = ta_val(t, l, ftype, node_child(m, 1));
	
	for (i = 2; i <= node_childrencount(m); i += 2) {
		v = ta_val(t, l, ftype, node_child(m, i + 1));
		
		if (node_val(node_child(m, i)) == "*")
			p *= v;
		else
			p /= v;
	}
	
	return p;
}

function ta_add(t, l, ftype, a,	i, s, p)
{
	s = ta_mult(t, l, ftype, node_child(a, 1));

	for (i = 2; i <= node_childrencount(a); i += 2) {
		p = ta_mult(t, l, ftype, node_child(a, i + 1));
		s += (node_val(node_child(a, i)) == "+") ? p : -p;
	}
	
	return s;
}

function ta_rel(t, l, ftype, rl,	v1, v2, op)
{
	v1 = ta_add(t, l, ftype, node_child(rl, 1));
	
	if (node_childrencount(rl) == 1)
		return v1;

	op = node_val(node_child(rl, 2));
	v2 = ta_add(t, l, ftype, node_child(rl,3));

	if (op == "=")
		return v1 == v2;
	else if (op == "!=")
		return v1 != v2;
	else if (op == "<")
		return v1 < v2;
	else if (op == ">")
		return v1 > v2;
	else if (op == "<=")
		return v1 <= v2;
	else if (op == ">=")
		return v1 >= v2;
}

function ta_and(t, l, ftype, a,	i)
{
	if (node_childrencount(a) == 1)
		return ta_rel(t, l, ftype, node_child(a, 1));

	for (i = 1; i <= node_childrencount(a); ++i)
		if (!ta_rel(t, l, ftype, node_child(a, i)))
			return 0;

	return 1;
}

function ta_expr(t, l, ftype, e,	i)
{
	if (node_childrencount(e) == 1)
		return ta_and(t, l, ftype, node_child(e, 1));

	for (i = 1; i <= node_childrencount(e); ++i)
		if (ta_and(t, l, ftype, node_child(e, i)))
			return 1;
	
	return 0;
}

function _ta_filter(t, l, ftype, f,		b, e, q, i, opf)
{
	if (node_childrencount(f) == 0)
		return 1;
	
	for (i = 1; i <= node_childrencount(f); ++i) {
		q = node_child(f, i);
		if (node_type(q) == FP_N_RANGE) {
			ftype = (ftype == TA_RF) ? TA_CF : TA_RF;

			b = ta_reftonum(t, l, ftype, node_child(q, 1));
			e = ta_reftonum(t, l, ftype, node_child(q, 2));
	
			if (l >= b && l <= e)
				return 1;
		}
		else if (node_type(q) == FP_N_LINEID) {
			opf = (ftype == TA_RF) ? TA_CF : TA_RF;
			if (l == ta_reftonum(t, l, opf, node_child(q, 1)))
				return 1;
		}
		else {
			if (ta_expr(t, l, ftype, q) != 0)
				return 1;
		}
	}

	return 0;
}

function ta_initvars(vnode, 	d, v, id, i)
{
	for (i = 1; i <= node_childrencount(vnode); ++i) {
		d = node_child(vnode, i);
		id = node_child(d, 1);
		v = node_child(d, 2);
		
		if (node_type(v) == FP_T_INT) {
			Ta_var[node_val(id),"type"] = TA_INT;
			Ta_var[node_val(id)] = node_val(v) + 0;
		}
		 else if (node_type(v) == FP_T_FLOAT) {
			Ta_var[node_val(id),"type"] = TA_FLOAT;
			Ta_var[node_val(id)] = node_val(v) + 0.0;
		} else {
			Ta_var[node_val(id),"type"] = TA_STRING;
			Ta_var[node_val(id)] = node_val(v);
		}
	}
}

function ta_filter(t, rowfilter, colfilter, defs,
	rfnode, cfnode, vnode, i, j, k, res, r, c, row, col, attr)
{
	res = ++Tablecount;

	rfnode = fp_filter(rowfilter);
	cfnode = fp_filter(colfilter);
	vnode = fp_deflist(defs);
	
	ta_initvars(vnode);

	Table[res,"rows"] = Table[res,"cols"] = 0;

	Table[res,"attrs"] = Table[t,"attrs"];
	for (i = 1; i <= Table[t,"attrs"]; ++i)
		Table[res,"attrname",i] = Table[t,"attrname",i];

	for (i = 1; i <= Table[t,"rows"]; ++i)
		if (_ta_filter(t, i, TA_RF, rfnode)) {
			row[i] = r = ++Table[res,"rows"];
			Table[res,"rowname",r] = Table[t,"rowname",i];
			Table[res,"rowid",Table[t,"rowname",i]] = r;
		}

	for (i = 1; i <= Table[t,"cols"]; ++i)
		if (_ta_filter(t, i, TA_CF, cfnode)) {
			col[i] = c = ++Table[res,"cols"];
			Table[res,"colname",c] = Table[t,"colname",i];
			Table[res,"colid",Table[t,"colname",i]] = c;
		}
	
	for (i = 1; i <= Table[t,"rows"]; ++i)
		for (j = 1; j <= Table[t,"cols"]; ++j) {
			if (!row[i] || !col[j]) 
				continue;

			for (k = 1; k <= Table[t,"attrs"]; ++k) {
				attr = Table[t,"attrname",k];
				Table[res,row[i],col[j],attr] = Table[t,i,j,attr];
			}
		}

	return res;
}

# "gluing" tables
#----------------------------------------------------------------------
function ta_setcell(t, y, x, val, type, vspan, hspan)
{
	Table[t,y,x,"val"] = val;
	Table[t,y,x,"type"] = type;
	Table[t,y,x,"vspan"] = vspan;
	Table[t,y,x,"hspan"] = hspan;
}

function ta_cellindex(i, j, vert)
{
	return vert ? (j SUBSEP i) : (i SUBSEP j);
}

function ta_tablespan(t, newside, vert,
	r, oldside, otherside, p, pi, i, j, c, pj)
{
	r = ++Tablecount;

	oldside = vert ? Table[t,"rows"] : Table[t,"cols"];

	if (newside <= oldside) {
		ta_copytable(r, t, 0, 0, Table[t,"rows"], Table[t,"cols"]);
		return r;
	}
	
	otherside = vert ? Table[t,"cols"] : Table[t,"rows"];
	Table[r,"rows"] = vert ? newside : Table[t,"rows"];
	Table[r,"cols"] = vert ? Table[t,"cols"] :  newside;
	Table[r,"attrs"] = 4;
	Table[r,"attrname",1] = "val";
	Table[r,"attrname",2] = "type";
	Table[r,"attrname",3] = "hspan";
	Table[r,"attrname",4] = "vspan";

	for (i = 1; i <= otherside; ++i) {
		p = pj = -1;
		for (j = 1; j <= newside; ++j) {
			# check!!! indexing changed from 0:l to 1;l
			c = int(oldside * (j - 1) / newside + 1);
			
			if ((vert ? (Table[t, ta_cellindex(i, c, vert),"vspan"] < 0) \
				: (Table[t, ta_cellindex(i, c, vert),"hspan"] < 0)) \
				|| c == p) {
				if (pj >= 0) {
					++Table[r, ta_cellindex(i, pj, vert),\
						(vert ? "vspan" : "hspan")];
				}

				Table[r, ta_cellindex(i, j, vert),"hspan"] = vert \
					? Table[t, ta_cellindex(i, c, vert),"hspan"] : -1;
				Table[r, ta_cellindex(i, j, vert),"vspan"] = vert \
					? -1 : Table[t, ta_cellindex(i, c, vert),"vspan"];
			}
			else {
				p = c;
				pj = j;
				
				Table[r, ta_cellindex(i, j, vert),"val"] \
					= Table[t, ta_cellindex(i, c, vert),"val"];
				Table[r, ta_cellindex(i, j, vert),"type"] \
					= Table[t, ta_cellindex(i, c, vert),"type"];
				Table[r, ta_cellindex(i, j, vert),"hspan"] = vert \
					? Table[t, ta_cellindex(i, c, vert),"hspan"] : 1;
				Table[r, ta_cellindex(i, j, vert),"vspan"] = vert \
					? 1 : Table[t, ta_cellindex(i, c, vert),"vspan"];
			}
		}
	}

	return r;
}

function ta_copytablemeta(dst, src, offr, offc, rows, cols)
{
	Table[dst,"rows"] = rows;
	Table[dst,"cols"] = cols;

	for (i = offr; i <= offr + rows; ++i) {
		Table[dst,"rowname",i] = Table[src,"rowname",i];
		Table[dst,"rowid",Table[src,"rowname",i]] = i;
	}

	for (i = offc; i <= offc + cols; ++i) {
		Table[dst,"colname",i] = Table[src,"colname",i];
		Table[dst,"colid",Table[src,"colname",i]] = i;
	}

	Table[dst,"attrs"] = Table[src,"attrs"];
	for (i = 1; i <= Table[src,"attrs"]; ++i)
		Table[dst,"attrname",i] = Table[src,"attrname",i];
}

function ta_copytable(dst, src, offr, offc, rows, cols,	i, j)
{
	for (i = 1; i <= rows; ++i)
		for (j = 1; j <= cols; ++j) {
			if (i > Table[src,"rows"] || j > Table[src,"cols"])
				ta_setcell(dst, i + offr, j + offc, "", TA_STRING, 1, 1);
			else {
				ta_setcell(dst, i + offr, j + offc, \
					Table[src,i,j,"val"], \
					Table[src,i,j,"type"], \
					Table[src,i,j,"vspan"], \
					Table[src,i,j,"hspan"]);
			}
		}

	Table[dst,"rows"] = offr + rows;
	Table[dst,"cols"] = offc + cols;
	Table[dst,"attrs"] = 4;
	Table[dst,"attrname",1] = "val";
	Table[dst,"attrname",2] = "type";
	Table[dst,"attrname",3] = "hspan";
	Table[dst,"attrname",4] = "vspan";
}

function ta_tablenextto(t1, t2, vert, span,
	tt, r, i, j, rows, cols, offr, offc, maxcols, maxrows)
{
	if (span) {
		t1 = ta_tablespan(t1, Table[t2,(vert ? "cols" : "rows")], !vert);
		t2 = ta_tablespan(t2, Table[t1,(vert ? "cols" : "rows")], !vert);
	}

	maxrows = (Table[t1,"rows"] > Table[t2,"rows"]) \
		? Table[t1,"rows"] : Table[t2,"rows"];
	maxcols = (Table[t1,"cols"] > Table[t2,"cols"]) \
		? Table[t1,"cols"] : Table[t2,"cols"];

	r = ++Tablecount;
	
	ta_copytable(r, t1, 0, 0, \
		vert ? Table[t1,"rows"] : maxrows, \
		vert ? maxcols : Table[t1,"cols"]);
	
	ta_copytable(r, t2, \
		vert ? Table[t1,"rows"] : 0, \
		vert ? 0 : Table[t1,"cols"], \
		vert ? Table[t2,"rows"] : maxrows, \
		vert ? maxcols : Table[t2,"cols"]);

	return r;
}

BEGIN {
	TA_STRING = 1;	TA_INT = 2;	TA_FLOAT = 3;
	
	TA_ISODATE = 1;	TA_RUDATE = 2;	TA_USDATE = 3;

	TA_RF = 1;	TA_CF = 2;
}
